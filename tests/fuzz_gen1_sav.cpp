// QA fuzz target: Generation I SAV1 loader, PK1 records, Bag decode and staged editors.
// Entry points are the ones the app uses: Gen1::parse (all three exact games, also the strict
// export validator), decodeInventory, Pokemon1ReadOnly, StagedInventoryEditor and
// StagedPokemonEditor create/finalize.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Pokemon/Pokemon1ReadOnly.h"

namespace G1 = PokeVault::Integration::Gen1;

namespace {

void runOne(std::span<const uint8_t> data) {
    for (const auto game : {G1::SourceGame::Red, G1::SourceGame::Blue, G1::SourceGame::Yellow}) {
        const auto parsed = G1::parse(data, game);
        // The strict export validator must agree exactly with the loader.
        std::string validatorError;
        QA_CHECK(PokeVault::Integration::Common::gen1StrictValidator(game)(data, validatorError) ==
                 static_cast<bool>(parsed));
        if (!parsed) {
            QA_CHECK(parsed.error != G1::SaveError::None);
            continue;
        }
        qafuzz::noteAccepted();
        const auto& save = *parsed.save;
        // Round trip: accepted bytes are retained verbatim and re-parse identically.
        QA_CHECK(std::equal(save.sourceBytes().begin(), save.sourceBytes().end(), data.begin(), data.end()));
        const auto again = G1::parse(save.sourceBytes(), game);
        QA_CHECK(again && again.save->party().size() == save.party().size() &&
                 again.save->boxes().size() == save.boxes().size() &&
                 again.save->trainer().trainerId == save.trainer().trainerId);
        QA_CHECK(save.party().size() <= 6);
        for (const auto& box : save.boxes()) QA_CHECK(box.slots.size() <= 20);
        // PK1 presentation wrapper over every decoded record.
        auto wrap = [](const G1::PokemonRecord& record) {
            const Pokemon::Pokemon1ReadOnly mon(record);
            (void)mon.species();
            (void)mon.nickname();
            QA_CHECK(mon.speciesID() == record.species && mon.level() == record.level);
        };
        for (const auto& record : save.party()) wrap(record);
        for (const auto& box : save.boxes())
            for (const auto& slot : box.slots)
                if (slot) wrap(*slot);
        (void)save.dexProgress();
        (void)G1::decodeInventory(save.sourceBytes(), save.metadata().region);

        std::string error;
        if (auto inventory = G1::StagedInventoryEditor::create(save, error)) {
            QA_CHECK(!inventory->hasPendingChanges());
            const auto bytes = inventory->finalize(error);
            QA_CHECK(bytes.empty() || std::equal(bytes.begin(), bytes.end(), data.begin(), data.end()));
        }
        error.clear();
        if (auto pokemon = G1::StagedPokemonEditor::create(save, error)) {
            QA_CHECK(!pokemon->hasPendingChanges());
            for (size_t box = 0; box < save.boxes().size(); ++box)
                for (size_t slot = 0; slot < 20; ++slot) (void)pokemon->boxedPokemon(box, slot, error);
            const auto bytes = pokemon->finalizedBytes(error);
            QA_CHECK(bytes.empty() || std::equal(bytes.begin(), bytes.end(), data.begin(), data.end()));
        }
    }
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen1-sav",
        runOne,
        [] {
            return std::vector<qafuzz::Bytes>{qasynth::gen1::save(), qasynth::gen1::save(0x999999, 0),
                                              qasynth::gen1::save(0x000000)};
        },
        qasynth::gen1::forge,
        qasynth::gen1::kCounts,
        {{0x4000, 0x6000, 0x2000}, {0x2598, 0x4000, 0xF8B}},
    };
    return target;
}
