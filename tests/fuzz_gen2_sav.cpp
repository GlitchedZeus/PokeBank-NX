// QA fuzz target: Generation II SAV2 loader (Gold/Silver/Crystal, with and without RTC footers),
// PK2 records, Bag decode and the staged editor (Trainer, Bag and Box Pokemon).
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Pokemon/Pokemon2ReadOnly.h"

namespace G2 = PokeVault::Integration::Gen2;

namespace {

void runOne(std::span<const uint8_t> data) {
    for (const auto game : {G2::SourceGame::Gold, G2::SourceGame::Silver, G2::SourceGame::Crystal}) {
        const auto parsed = G2::parse(data, game);
        std::string validatorError;
        QA_CHECK(PokeVault::Integration::Common::gen2StrictValidator(game)(data, validatorError) ==
                 static_cast<bool>(parsed));
        if (!parsed) {
            QA_CHECK(parsed.error != G2::SaveError::None);
            continue;
        }
        qafuzz::noteAccepted();
        const auto& save = *parsed.save;
        const auto& meta = save.metadata();
        QA_CHECK(std::equal(save.sourceBytes().begin(), save.sourceBytes().end(), data.begin(), data.end()));
        QA_CHECK(meta.payloadSize <= data.size() && meta.payloadSize + meta.rtcFooterSize + meta.paddingSize == data.size());
        const auto again = G2::parse(save.sourceBytes(), game);
        QA_CHECK(again && again.save->party().size() == save.party().size() &&
                 again.save->boxes().size() == save.boxes().size() &&
                 again.save->trainer().money == save.trainer().money);
        QA_CHECK(save.party().size() <= 6);
        for (const auto& box : save.boxes()) QA_CHECK(box.slots.size() <= meta.boxCapacity);
        // PK2 presentation wrapper over every decoded record.
        auto wrap = [](const G2::PokemonRecord& record) {
            const Pokemon::Pokemon2ReadOnly mon(record);
            (void)mon.species();
            (void)mon.nickname();
            QA_CHECK(mon.speciesID() == record.species && mon.heldItem() == record.heldItem);
        };
        for (const auto& record : save.party()) wrap(record);
        for (const auto& box : save.boxes())
            for (const auto& slot : box.slots)
                if (slot) wrap(*slot);
        (void)save.dexProgress();
        (void)G2::decodeInventory(save.payloadBytes(), meta.region, meta.family);

        std::string error;
        if (auto editor = G2::StagedEditor::create(save, error)) {
            QA_CHECK(!editor->hasPendingChanges());
            QA_CHECK(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(),
                                data.begin(), data.end()));
            const auto bytes = editor->finalizedBytes(error);
            QA_CHECK(bytes.empty() || std::equal(bytes.begin(), bytes.end(), data.begin(), data.end()));
        }
    }
}

std::vector<qafuzz::Bytes> seeds() {
    using namespace qasynth::gen2;
    auto gs = save(IGS);
    auto crystal = save(IC);
    // Accepted RTC footer variants are appended by the loader's own size table; probe the
    // common 0x2C-byte footer shape too (rejected cleanly if not in the table).
    auto gsFooter = gs;
    gsFooter.resize(gs.size() + 0x2C, 0);
    return {gs, crystal, save(IGS, 0x99), gsFooter};
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen2-sav",
        runOne,
        seeds,
        qasynth::gen2::forge,
        qasynth::gen2::kCounts,
        {{0x4000, 0x6000, 0x2000}, {0x2009, 0x1209, 0xB60}},
        1, // the RTC-footer probe seed may legitimately be rejected
    };
    return target;
}
