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
    if (original.size() != LayoutInfo::FullSaveSize || pending.empty())
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

        // Re-verify the entire encrypted before/after transaction. CRC-valid
        // but unrelated mutations (species, PID, moves, OT etc.) are rejected.
        std::string reviewError;
        if (!verifyStagedReview(change, &reviewError))
            return fail("Gen V export refused unverified staged PK5 field changes");
        const auto sourceRecord = party ? source.partyPokemon(change.location.slot)
            : source.boxPokemon(change.location.box, change.location.slot);
        if (!sourceRecord || !sourceRecord->valid() || sourceRecord->empty())
            return fail("Gen V export source slot is absent or invalid");
        const auto bytes = sourceRecord->originalEncryptedBytes();
        if (bytes.size() != change.before.size() ||
            !std::equal(bytes.begin(), bytes.end(), change.before.begin()))
            return fail("Gen V export source slot differs from the staged baseline");

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
        if (!pk || !pk->valid() || pk->empty() ||
            pk->originalEncryptedBytes().size() != change.after.size() ||
            !std::equal(pk->originalEncryptedBytes().begin(),
                        pk->originalEncryptedBytes().end(), change.after.begin()))
            return fail("Gen V export candidate did not preserve exact staged PK5 bytes");
    }

    output.changedRecords = pending.size();
    if (error) error->clear();
    return output;
}

} // namespace PokeVault::Integration::Gen5

#endif
