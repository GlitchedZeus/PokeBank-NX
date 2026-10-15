// QA fuzz target: Generation III SAV3 loader (the native adapter the Switch app compiles) for all
// five exact games, PK3 party/box enumeration, Bag pouches and both staged editors.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"

namespace G3 = PokeVault::Integration::Gen3;

namespace {

void runOne(std::span<const uint8_t> data) {
    for (const auto game : {G3::SourceGame::FireRedGBA, G3::SourceGame::LeafGreenGBA, G3::SourceGame::RubyGBA,
                            G3::SourceGame::SapphireGBA, G3::SourceGame::EmeraldGBA}) {
        const auto parsed = G3::parse(data, game);
        std::string validatorError;
        QA_CHECK(PokeVault::Integration::Common::gen3StrictValidator(game)(data, validatorError) ==
                 static_cast<bool>(parsed));
        if (!parsed) {
            QA_CHECK(parsed.error != G3::SaveError::None);
            continue;
        }
        qafuzz::noteAccepted();
        const auto& save = *parsed.save;
        QA_CHECK(std::equal(save.sourceBytes().begin(), save.sourceBytes().end(), data.begin(), data.end()));
        QA_CHECK(save.metadata().activeSlot < 2);
        QA_CHECK(save.metadata().partyCount <= 6);
        const auto party = save.party();
        const auto boxes = save.boxes();
        QA_CHECK(party.size() <= 6);
        QA_CHECK(boxes.size() <= size_t{14} * 30);
        for (const auto& mon : boxes) QA_CHECK(mon.location.box < 14 && mon.location.slot < 30);
        for (const auto& pouch : save.inventory()) QA_CHECK(pouch.items.size() <= pouch.capacity);
        (void)save.dexProgress();
        (void)save.lastEnumerationError();
        const auto again = G3::parse(save.sourceBytes(), game);
        QA_CHECK(again && again.save->metadata().activeSlot == save.metadata().activeSlot &&
                 again.save->metadata().saveCounter == save.metadata().saveCounter &&
                 again.save->party().size() == party.size());

        std::string error;
        if (auto inventory = G3::StagedInventoryEditor::create(data, game, error)) {
            QA_CHECK(!inventory->hasPendingChanges() && inventory->stagedBytes() == inventory->originalBytes());
            const auto bytes = inventory->finalizedBytes(error);
            QA_CHECK(bytes.empty() || std::equal(bytes.begin(), bytes.end(), data.begin(), data.end()));
        }
        error.clear();
        if (auto pokemon = G3::StagedPokemonEditor::create(data, game, error)) {
            QA_CHECK(!pokemon->hasPendingChanges() && pokemon->stagedBytes() == pokemon->originalBytes());
            for (size_t box = 0; box < pokemon->boxCount(); ++box)
                for (size_t slot = 0; slot < pokemon->boxCapacity(); ++slot)
                    (void)pokemon->boxedPokemon(box, slot, error);
            const auto bytes = pokemon->finalizedBytes(error);
            QA_CHECK(bytes.empty() || std::equal(bytes.begin(), bytes.end(), data.begin(), data.end()));
        }
    }
}

std::vector<qafuzz::Bytes> seeds() {
    std::vector<qafuzz::Bytes> out;
    auto fixture = qasynth::gen3::fixture();
    if (fixture.empty()) {
        // Never silently degrade to rejection-only fuzzing.
        std::fprintf(stderr, "gen3-sav: synthetic FR/LG fixture missing; build it with "
                             "make -f Makefile.host build-host/frlg_fixture.sav or set POKEBANK_QA_GEN3_FIXTURE\n");
        std::exit(2);
    }
    out.push_back(fixture);
    out.push_back(qasynth::gen3::skeleton());
    return out;
}

std::vector<qafuzz::SwapRegion> swaps() {
    namespace D = PokeVault::Integration::Gen3::Detail;
    std::vector<qafuzz::SwapRegion> out{{D::kSlotBases[0], D::kSlotBases[1], D::kSectorCount * D::kSectorSize}};
    // Swap individual sector footers (id/checksum/signature/counter) between and within slots.
    for (size_t s = 0; s + 1 < D::kSectorCount; ++s) {
        out.push_back({s * D::kSectorSize + 0xFF4, D::kSlotBases[1] + s * D::kSectorSize + 0xFF4, 12});
        out.push_back({s * D::kSectorSize + 0xFF4, (s + 1) * D::kSectorSize + 0xFF4, 2});
    }
    return out;
}

std::vector<size_t> counts() {
    namespace D = PokeVault::Integration::Gen3::Detail;
    std::vector<size_t> out;
    // Sector ids and the first bytes of every sector (party counts live in a sector body).
    for (size_t slot : D::kSlotBases)
        for (size_t s = 0; s < D::kSectorCount; ++s) {
            out.push_back(slot + s * D::kSectorSize + 0xFF4);
            out.push_back(slot + s * D::kSectorSize + 0x234);
            out.push_back(slot + s * D::kSectorSize + 0x34);
        }
    return out;
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen3-sav", runOne, seeds, qasynth::gen3::forge, counts(), swaps(),
        1, // the zero-payload sector skeleton is expected to be rejected after sector validation
    };
    return target;
}
