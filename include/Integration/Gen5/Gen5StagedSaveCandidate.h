#ifndef POKEBANK_GEN5_STAGED_SAVE_CANDIDATE_H
#define POKEBANK_GEN5_STAGED_SAVE_CANDIDATE_H

#include "Integration/Gen5/Gen5SharedReview.h"
#include "Utils/CRC16.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// App-owned export IMAGE, not a source-save writer. This API has no paths,
// provider handles or filesystem calls and never changes either SAV5 source
// copy. Callers must separately implement backup/confirmation/export policy.
// In particular, this does not authorize Inject Save or emulator writes.
struct Gen5StagedSaveCandidate {
    std::vector<uint8_t> bytes;
    size_t changedRecords = 0;
    bool bagChanged = false;
    size_t selectedCopyOffset = 0;
    std::string exactGameId;
};

[[nodiscard]] inline std::optional<Gen5StagedSaveCandidate> buildStagedSaveCandidate(
    const Gen5StagedPokemonWorkspace& workspace, std::string* error = nullptr) {
    using Region = Gen5StagedPokemonWorkspace::Region;
    const auto fail = [&](const char* reason) -> std::optional<Gen5StagedSaveCandidate> {
        if (error) *error = reason;
        return std::nullopt;
    };

    const Gen5ReadOnlySave& source = workspace.sourceSave();
    const auto original = source.sourceBytes();
    const auto pending = workspace.pendingReview();
    const Gen5StagedBag* stagedBag = workspace.bagDirty() ? workspace.stagedBagIfOpened() : nullptr;
    if (original.size() != LayoutInfo::FullSaveSize || (pending.empty() && !stagedBag))
        return fail("Gen V export needs a full validated save and pending staged changes");

    const auto blockTable = source.family() == SaveFamily::BlackWhite
        ? std::span<const LayoutInfo::SaveBlock>(LayoutInfo::BlackWhite)
        : std::span<const LayoutInfo::SaveBlock>(LayoutInfo::Black2White2);
    const size_t copySize = source.family() == SaveFamily::BlackWhite
        ? LayoutInfo::BlackWhiteCopySize : LayoutInfo::Black2White2CopySize;
    const size_t base = source.selectedCopyOffset();
    if ((base != 0 && base != copySize) || base + copySize > original.size())
        return fail("Gen V export refused a noncanonical selected copy");

    Gen5StagedSaveCandidate output;
    output.bytes.assign(original.begin(), original.end());
    output.selectedCopyOffset = base;
    output.exactGameId = std::string(source.exactGameId());

    // Whitelist precise PK5 record ranges and exactly the retail checksums
    // that must change. Everything else (especially the other save copy,
    // trainer, flags, inventory, box metadata and padding) must match bytewise.
    std::vector<uint8_t> allowed(output.bytes.size(), 0);
    std::vector<bool> modified(blockTable.size(), false);
    const auto markChecksum = [&](size_t relative) {
        allowed[base + relative] = 1;
        allowed[base + relative + 1] = 1;
    };
    for (const auto& change : pending) {
        if (!Gen5StagedPokemonWorkspace::canonicalSlot(change.location))
            return fail("Gen V export refused noncanonical PK5 slot identity");
        const bool party = change.location.region == Region::Party;
        const size_t recordSize = party ? Crypto::PartySize : Crypto::StoredSize;
        const size_t pos = party
            ? LayoutInfo::PartyOffset + 8 + change.location.slot * recordSize
            : LayoutInfo::BoxOffset + change.location.box * LayoutInfo::BoxStride +
              change.location.slot * recordSize;
        if (change.before.size() != recordSize || change.after.size() != recordSize ||
            pos + recordSize > copySize)
            return fail("Gen V export refused invalid native PK5 record geometry");

        // Re-verify ordinary native field edits or exact box clone/remove
        // transactions. All PK5 and source-copy provenance is independently
        // checked against the immutable, checksum-valid selected SAV5.
        std::string reviewError;
        if (!verifyStagedReview(change, &reviewError))
            return fail("Gen V export refused unverified staged PK5 field changes");
        const auto sourceRecord = party ? source.partyPokemon(change.location.slot)
            : source.boxPokemon(change.location.box, change.location.slot);
        if (!sourceRecord || !sourceRecord->valid() ||
            (change.kind!=Gen5StagedPokemonWorkspace::ChangeKind::CloneBox &&
             change.kind!=Gen5StagedPokemonWorkspace::ChangeKind::CreateBox &&
             sourceRecord->empty()))
            return fail("Gen V export source slot is absent or invalid");
        const auto bytes = sourceRecord->originalEncryptedBytes();
        if (bytes.size() != change.before.size() ||
            !std::equal(bytes.begin(), bytes.end(), change.before.begin()))
            return fail("Gen V export source slot differs from the staged baseline");
        if(change.kind==Gen5StagedPokemonWorkspace::ChangeKind::CreateBox) {
            const auto& request=change.createRequest;
            if(!request ||
               static_cast<uint32_t>(request->game)!=source.trainer().gameVersion ||
               request->format!=CreateFormat5::Boxed ||
               !sourceRecord->empty() ||
               !std::all_of(bytes.begin(),bytes.end(),
                            [](uint8_t b){return b==0;}))
                return fail("Gen V Create requires exact-title source and empty baseline");
            const auto created=createNative5(*request);
            if(!created || created.encrypted!=change.replacementOriginal)
                return fail("Gen V exported Create did not reverify native baseline");
        }
        if(change.kind==Gen5StagedPokemonWorkspace::ChangeKind::CloneBox) {
            const auto& id=change.cloneSource;
            if(!Gen5StagedPokemonWorkspace::canonicalSlot(id))
                return fail("Gen V export clone source identity is not canonical");
            const auto from=id.region==Region::Party
                ? source.partyPokemon(id.slot)
                : source.boxPokemon(id.box,id.slot);
            if(!from || !from->valid() || from->empty() ||
               from->originalEncryptedBytes().size()!=change.cloneSourceOriginal.size() ||
               !std::equal(from->originalEncryptedBytes().begin(),
                           from->originalEncryptedBytes().end(),
                           change.cloneSourceOriginal.begin()))
                return fail("Gen V clone source differs from immutable SAV5");
        }

        // Only one layout block may own an individual record. Never update a
        // block across a boundary, or guess a block from the currently viewed UI.
        size_t owner = blockTable.size();
        for (size_t i = 0; i + 1 < blockTable.size(); ++i) {
            const auto& block = blockTable[i];
            if (pos >= block.offset && pos + recordSize <= block.offset + block.length) {
                if (owner != blockTable.size())
                    return fail("Gen V export has ambiguous PK5 block ownership");
                owner = i;
            }
        }
        if (owner == blockTable.size())
            return fail("Gen V export PK5 record escapes retail checksum block");
        modified[owner] = true;
        const size_t absolute = base + pos;
        std::copy(change.after.begin(), change.after.end(),
                  output.bytes.begin() + static_cast<std::ptrdiff_t>(absolute));
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(absolute),
                    recordSize, static_cast<uint8_t>(1));
    }

    // Native Bag: the staged Items block may differ from the source only inside
    // its pocket slots (Gen5StagedBag audits every edit); the source baseline it
    // was built from must still be the selected copy's exact Items block.
    if (stagedBag) {
        const auto sourceBlock = sourceItemsBlock(source);
        const auto originalBlock = stagedBag->original();
        const auto stagedBlock = stagedBag->staged();
        if (stagedBag->family() != source.family() || sourceBlock.empty() ||
            originalBlock.size() != sourceBlock.size() || stagedBlock.size() != sourceBlock.size() ||
            !std::equal(sourceBlock.begin(), sourceBlock.end(), originalBlock.begin()) ||
            !decodeBag5(source.family(), stagedBlock))
            return fail("Gen V export bag baseline differs from the selected source copy");
        const size_t blockAt = base + itemsBlockOffset(source.family());
        for (size_t pocket = 0; pocket < BagPocket5Count; ++pocket) {
            const auto spec = bagPocketLayout5(source.family(), pocket);
            std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(blockAt + spec.offset),
                        spec.slots * 4, static_cast<uint8_t>(1));
        }
        std::copy(stagedBlock.begin(), stagedBlock.end(),
                  output.bytes.begin() + static_cast<std::ptrdiff_t>(blockAt));
        modified[ItemsBlockIndex] = true;
    }

    auto partition = std::span<uint8_t>(output.bytes).subspan(base, copySize);
    // A block's mirrored checksum lies in the terminal checksum-table block.
    // Therefore the dependent terminal CRC is recalculated LAST, once.
    for (size_t i = 0; i + 1 < blockTable.size(); ++i) {
        if (!modified[i]) continue;
        const auto& block = blockTable[i];
        if (block.offset + block.length > copySize ||
            block.checksumOffset + 2 > copySize ||
            block.mirrorOffset + 2 > copySize)
            return fail("Gen V export block checksum geometry is invalid");
        const uint16_t crc = Utils::crc16ccitt(partition.data()+block.offset,block.length);
        Crypto::write16(partition,block.checksumOffset,crc);
        Crypto::write16(partition,block.mirrorOffset,crc);
        markChecksum(block.checksumOffset);
        markChecksum(block.mirrorOffset);
    }
    const auto& terminal = blockTable.back();
    if (terminal.offset + terminal.length > copySize ||
        terminal.checksumOffset + 2 > copySize ||
        terminal.mirrorOffset + 2 > copySize)
        return fail("Gen V export terminal checksum geometry is invalid");
    const uint16_t terminalCrc =
        Utils::crc16ccitt(partition.data()+terminal.offset,terminal.length);
    Crypto::write16(partition,terminal.checksumOffset,terminalCrc);
    Crypto::write16(partition,terminal.mirrorOffset,terminalCrc);
    markChecksum(terminal.checksumOffset);
    markChecksum(terminal.mirrorOffset);

    for (size_t i = 0; i < original.size(); ++i) {
        if (output.bytes[i] != original[i] && !allowed[i])
            return fail("Gen V export modified bytes outside verified PK5 and CRC ranges");
    }

    // Explicit selection matters when the other valid SAV copy is older or
    // different. Automatic parse is intentionally NOT used in that case.
    const auto selection = source.selectedBackupPartition()
        ? SaveCopySelection::Backup : SaveCopySelection::Primary;
    const auto checked = Gen5ReadOnlySave::parse(output.bytes, source.exactGameId(),
                                                 nullptr, selection);
    if (!checked || checked->family() != source.family() ||
        checked->selectedCopyOffset() != base ||
        checked->partyCount() != source.partyCount())
        return fail("Gen V export candidate failed exact-game full-SAV revalidation");

    for (const auto& change : pending) {
        const bool party = change.location.region == Region::Party;
        const auto pk = party ? checked->partyPokemon(change.location.slot)
            : checked->boxPokemon(change.location.box, change.location.slot);
        if (!pk || !pk->valid() ||
            (change.kind!=Gen5StagedPokemonWorkspace::ChangeKind::RemoveBox &&
             pk->empty()) ||
            (change.kind==Gen5StagedPokemonWorkspace::ChangeKind::RemoveBox &&
             !pk->empty()) ||
            pk->originalEncryptedBytes().size() != change.after.size() ||
            !std::equal(pk->originalEncryptedBytes().begin(),
                        pk->originalEncryptedBytes().end(), change.after.begin()))
            return fail("Gen V export candidate did not preserve exact staged PK5 bytes");
    }
    if (stagedBag) {
        const auto exported = decodeBag5(*checked);
        const auto expected = decodeBag5(source.family(), stagedBag->staged());
        bool same = exported && expected;
        for (size_t p = 0; same && p < BagPocket5Count; ++p) {
            same = (*exported)[p].size() == (*expected)[p].size();
            for (size_t i = 0; same && i < (*exported)[p].size(); ++i)
                same = (*exported)[p][i].itemId == (*expected)[p][i].itemId &&
                       (*exported)[p][i].count == (*expected)[p][i].count;
        }
        if (!same)
            return fail("Gen V export candidate did not preserve the exact staged bag");
    }

    output.changedRecords = pending.size();
    output.bagChanged = stagedBag != nullptr;
    if (error) error->clear();
    return output;
}


// A source may change after its Save Instance was opened (emulator progress,
// save-state restore, external replacement). Re-read its exact raw bytes and
// check physical/provider identity separately before this function is called.
// Never export an old in-memory draft over an unseen newer source.
// This checks the WHOLE 0x80000 image, including the unselected backup copy.
[[nodiscard]] inline std::optional<Gen5StagedSaveCandidate>
buildStagedSaveCandidateForObservedSource(
    const Gen5StagedPokemonWorkspace& workspace,
    std::span<const uint8_t> observedBytes,
    std::string* error = nullptr) {
    const auto fail = [&](const char* why) -> std::optional<Gen5StagedSaveCandidate> {
        if (error) *error = why;
        return std::nullopt;
    };
    const auto baseline = workspace.sourceSave().sourceBytes();
    if (observedBytes.size() != LayoutInfo::FullSaveSize ||
        baseline.size() != observedBytes.size() ||
        !std::equal(observedBytes.begin(),observedBytes.end(),baseline.begin()))
        return fail("Gen V source changed since staging; reopen and revalidate the Save Instance");

    const auto selection = workspace.sourceSave().selectedBackupPartition()
        ? SaveCopySelection::Backup : SaveCopySelection::Primary;
    const auto checked = Gen5ReadOnlySave::parse(
        observedBytes,workspace.sourceSave().exactGameId(),nullptr,selection);
    if (!checked || checked->family() != workspace.sourceSave().family() ||
        checked->selectedCopyOffset() != workspace.sourceSave().selectedCopyOffset())
        return fail("Gen V observed source failed exact-title/copy revalidation");

    return buildStagedSaveCandidate(workspace,error);
}

} // namespace PokeVault::Integration::Gen5

#endif
