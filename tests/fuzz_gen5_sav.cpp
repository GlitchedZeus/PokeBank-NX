// QA fuzz target: Generation V SAV5 loader (BW / B2W2), the primary/backup copy selector, Save
// Instance identification (probeNormalizedBattery), PK5 party/box records, the native Bag and the
// staged export candidate.
//
// Copy-selector contract checked on every input:
//  * Automatic never picks a copy that Primary/Backup would refuse;
//  * when both copies validate but differ, Automatic refuses instead of guessing;
//  * every selected copy round-trips (re-parse with the explicit selection is identical).
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5Trainer.h"

namespace G5 = PokeVault::Integration::Gen5;

namespace {

void exercise(const G5::Gen5ReadOnlySave& save, std::span<const uint8_t> data) {
    QA_CHECK(std::equal(save.sourceBytes().begin(), save.sourceBytes().end(), data.begin(), data.end()));
    QA_CHECK(G5::isExactGen5Id(save.exactGameId()));
    QA_CHECK(save.partyCount() <= G5::LayoutInfo::PartySlots);
    (void)save.trainer();
    (void)save.dexProgress();
    for (size_t slot = 0; slot <= G5::LayoutInfo::PartySlots; ++slot) {
        const auto mon = save.partyPokemon(slot);
        if (slot == G5::LayoutInfo::PartySlots) QA_CHECK(!mon);
        else if (slot < save.partyCount()) QA_CHECK(mon && mon->valid() && !mon->empty());
        if (mon) (void)mon->shiny();
    }
    for (size_t box = 0; box <= G5::LayoutInfo::BoxCount; box += 1)
        for (size_t slot = 0; slot <= G5::LayoutInfo::BoxSlots; ++slot) {
            const auto mon = save.boxPokemon(box, slot);
            if (box == G5::LayoutInfo::BoxCount || slot == G5::LayoutInfo::BoxSlots) QA_CHECK(!mon);
            if (mon) (void)mon->ivs();
        }

    const auto bag = G5::decodeBag5(save);
    auto staged = G5::Gen5StagedBag::create(save);
    QA_CHECK(static_cast<bool>(bag) == static_cast<bool>(staged));

    G5::Gen5StagedPokemonWorkspace workspace(save);
    std::string error;
    QA_CHECK(!workspace.hasChanges());
    QA_CHECK(!G5::buildStagedSaveCandidate(workspace, &error) && !error.empty());

    // A real staged edit on a fuzzed-but-accepted save must either be refused or produce a
    // candidate that re-parses strictly as the same game and copy, differs from the source only
    // inside the selected copy, and leaves the source bytes untouched.
    // Native Trainer view/staging (Gen5Trainer.h): decode must agree with create.
    QA_CHECK(static_cast<bool>(G5::decodeTrainer5(save)) ==
             static_cast<bool>(G5::Gen5StagedTrainer::create(save)));
    if (auto* trainer = workspace.stagedTrainer(&error)) {
        const auto before = trainer->current();
        if (trainer->setMoney(before.money == 1 ? 2 : 1, &error)) {
            QA_CHECK(trainer->dirty());
            const auto candidate = G5::buildStagedSaveCandidateForObservedSource(workspace, data, &error);
            if (candidate) {
                QA_CHECK(candidate->trainerChanged && candidate->selectedCopyOffset == save.selectedCopyOffset());
                std::string reparseError;
                const auto selection = save.selectedBackupPartition() ? G5::SaveCopySelection::Backup
                                                                      : G5::SaveCopySelection::Primary;
                QA_CHECK(G5::Gen5ReadOnlySave::parse(candidate->bytes, save.exactGameId(), &reparseError, selection));
            }
        }
        workspace.discardAll();
    }
    if (auto* bagEditor = workspace.stagedBag(&error)) {
        const auto current = bagEditor->current();
        bool edited = false;
        for (size_t pocket = 0; pocket < G5::BagPocket5Count && !edited; ++pocket) {
            if (pocket == static_cast<size_t>(G5::BagPocket5::KeyItems) ||
                pocket == static_cast<size_t>(G5::BagPocket5::Machines) || current[pocket].empty())
                continue;
            const uint16_t next = current[pocket][0].count == 1 ? 2 : 1;
            edited = bagEditor->setQuantity(pocket, 0, next, &error);
        }
        if (edited) {
            QA_CHECK(workspace.hasChanges());
            const auto candidate = G5::buildStagedSaveCandidateForObservedSource(workspace, data, &error);
            if (candidate) {
                QA_CHECK(candidate->bytes.size() == data.size() && candidate->bagChanged);
                QA_CHECK(candidate->selectedCopyOffset == save.selectedCopyOffset());
                const auto selection = save.selectedBackupPartition() ? G5::SaveCopySelection::Backup
                                                                      : G5::SaveCopySelection::Primary;
                std::string reparseError;
                const auto reparsed =
                    G5::Gen5ReadOnlySave::parse(candidate->bytes, save.exactGameId(), &reparseError, selection);
                QA_CHECK(reparsed && reparsed->selectedCopyOffset() == save.selectedCopyOffset());
                const size_t copy = save.family() == G5::SaveFamily::BlackWhite ? G5::LayoutInfo::BlackWhiteCopySize
                                                                                : G5::LayoutInfo::Black2White2CopySize;
                for (size_t i = 0; i < data.size(); ++i)
                    if (candidate->bytes[i] != data[i])
                        QA_CHECK(i >= save.selectedCopyOffset() && i < save.selectedCopyOffset() + copy);
            }
        }
    }
}

void runOne(std::span<const uint8_t> data) {
    std::string primaryError, backupError, autoError;
    const auto primary = G5::Gen5ReadOnlySave::parse(data, {}, &primaryError, G5::SaveCopySelection::Primary);
    const auto backup = G5::Gen5ReadOnlySave::parse(data, {}, &backupError, G5::SaveCopySelection::Backup);
    const auto automatic = G5::Gen5ReadOnlySave::parse(data, {}, &autoError, G5::SaveCopySelection::Automatic);
    if (!primary) QA_CHECK(!primaryError.empty());
    if (!backup) QA_CHECK(!backupError.empty());
    if (!automatic) QA_CHECK(!autoError.empty());
    if (automatic) {
        // Automatic may only ever choose a copy that is independently valid on request.
        if (automatic->selectedBackupPartition()) QA_CHECK(backup && !primary);
        else QA_CHECK(static_cast<bool>(primary));
    }
    if (primary && backup) {
        const auto p = primary->sourceBytes().subspan(primary->selectedCopyOffset(),
                                                      backup->selectedCopyOffset());
        const auto b = backup->sourceBytes().subspan(backup->selectedCopyOffset(), backup->selectedCopyOffset());
        const bool identical = std::equal(p.begin(), p.end(), b.begin(), b.end());
        QA_CHECK(static_cast<bool>(automatic) == identical);
    }
    for (const auto* parsed : {&primary, &backup, &automatic}) {
        if (!*parsed) continue;
        qafuzz::noteAccepted();
        exercise(**parsed, data);
        // Assigning the validated exact game must accept; any other exact title must refuse.
        for (const std::string_view id : {"black_nds", "white_nds", "black2_nds", "white2_nds"}) {
            std::string error;
            const auto selection = (*parsed)->selectedBackupPartition() ? G5::SaveCopySelection::Backup
                                                                        : G5::SaveCopySelection::Primary;
            const auto assigned = G5::Gen5ReadOnlySave::parse(data, id, &error, selection);
            QA_CHECK(static_cast<bool>(assigned) == (id == (*parsed)->exactGameId()));
        }
    }

    // Save Instance identification: pure, in-memory, never ready without exact provenance.
    for (const std::string_view assigned : {"black_nds", "white2_nds", "emerald_gba", ""}) {
        G5::SourceContext context;
        context.assignedExactGame = assigned;
        context.providerId = "drastic";
        context.providerLabel = "DraStic";
        context.sourcePath = "sdmc:/qa/synthetic.sav";
        context.physicalIdentity = "qa-physical";
        const auto probe = G5::probeNormalizedBattery(data, context);
        QA_CHECK(probe.instance.readOnly());
        if (probe.ready()) {
            QA_CHECK(probe.save && probe.save->exactGameId() == assigned);
            QA_CHECK(probe.instance.partyCount == probe.save->partyCount());
        } else {
            QA_CHECK(!probe.instance.diagnostic.empty());
        }
        if (!G5::isExactGen5Id(assigned)) QA_CHECK(!probe.ready());
    }
}

std::vector<qafuzz::Bytes> seeds() {
    using qasynth::gen5::Family;
    using qasynth::gen5::save;
    return {save(Family::BlackWhite, 21, 1), save(Family::BlackWhite, 20, 2), save(Family::BlackWhite, 21, 3),
            save(Family::BlackWhite, 20, 4), save(Family::Black2White2, 23, 1), save(Family::Black2White2, 22, 2),
            save(Family::Black2White2, 23, 3), save(Family::Black2White2, 22, 4)};
}

std::vector<qafuzz::SwapRegion> swaps() {
    namespace L = PokeVault::Integration::Gen5::LayoutInfo;
    return {
        {0, L::BlackWhiteCopySize, L::BlackWhiteCopySize},                       // swap BW copies
        {0, L::Black2White2CopySize, L::Black2White2CopySize},                   // swap B2W2 copies
        {0x23F00, L::BlackWhiteCopySize + 0x23F00, 0x100},                       // swap BW checksum footers
        {0x25F00, L::Black2White2CopySize + 0x25F00, 0x100},                     // swap B2W2 checksum footers
        {L::PartyOffset, L::BlackWhiteCopySize + L::PartyOffset, 0x534},          // swap party blocks
    };
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen5-sav", runOne, seeds, qasynth::gen5::forge, qasynth::gen5::countOffsets(), swaps(),
    };
    return target;
}
