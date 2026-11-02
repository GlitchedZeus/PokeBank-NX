// QA fuzz target: Generation IV SAV4 loader (DP / Pt / HGSS, with and without an assigned exact
// game), partition selection, PK4 party/box records, the native Bag decoder and the staged editor.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"

namespace G4 = PokeVault::Integration::Gen4;

namespace {

const char* gameFor(G4::Layout layout) {
    switch (layout) {
        case G4::Layout::DiamondPearl: return "diamond_nds";
        case G4::Layout::Platinum: return "platinum_nds";
        case G4::Layout::HeartGoldSoulSilver: return "heartgold_nds";
    }
    return "";
}

void exercise(const G4::Gen4ReadOnlySave& save, std::span<const uint8_t> data) {
    QA_CHECK(std::equal(save.sourceBytes().begin(), save.sourceBytes().end(), data.begin(), data.end()));
    QA_CHECK(save.partyCount() <= 6 && save.party().size() == save.partyCount());
    QA_CHECK(save.generalSelection().partition < 2 && save.storageSelection().partition < 2);
    for (const auto& mon : save.nativePartySlots()) {
        (void)mon.valid();
        (void)mon.nickname();
        (void)mon.battleStats();
    }
    for (size_t box = 0; box <= 18; ++box) {
        for (size_t slot = 0; slot <= 30; ++slot) {
            const auto& mon = save.box(box, slot);
            if (box == 18 || slot == 30) QA_CHECK(!mon.valid());
            if (mon.valid() && !mon.empty()) {
                (void)mon.nickname();
                (void)mon.originalTrainerName();
                (void)mon.personal();
            }
        }
        (void)save.hgssBoxPadding(box);
    }
    for (const auto& name : save.boxNames()) QA_CHECK(name.size() <= 64);
    (void)save.trainer();
    (void)save.dexProgress();
    (void)G4::decodeReadOnlyBag(save);
}

void runOne(std::span<const uint8_t> data) {
    for (const auto layout : {G4::Layout::DiamondPearl, G4::Layout::Platinum, G4::Layout::HeartGoldSoulSilver}) {
        for (const std::string_view assigned : {std::string_view{}, std::string_view{gameFor(layout)}}) {
            std::string error;
            const auto save = G4::Gen4ReadOnlySave::parse(data, layout, assigned, &error);
            if (!save) {
                QA_CHECK(!error.empty());
                continue;
            }
            qafuzz::noteAccepted();
            exercise(*save, data);
            std::string againError;
            const auto again = G4::Gen4ReadOnlySave::parse(save->sourceBytes(), layout, assigned, &againError);
            QA_CHECK(again && again->partyCount() == save->partyCount() &&
                     again->generalSelection().offset == save->generalSelection().offset &&
                     again->storageSelection().offset == save->storageSelection().offset &&
                     again->diagnostics().occupiedBoxRecords == save->diagnostics().occupiedBoxRecords);
            if (assigned.empty()) continue;
            if (auto editor = G4::Gen4StagedPokemonEditor::create(data, layout, assigned, &error)) {
                QA_CHECK(!editor->hasChanges());
                QA_CHECK(std::equal(editor->originalBytes().begin(), editor->originalBytes().end(),
                                    data.begin(), data.end()));
                const auto bytes = editor->finalizedBytes(&error);
                QA_CHECK(bytes.empty() || std::equal(bytes.begin(), bytes.end(), data.begin(), data.end()));
            }
        }
    }
}

std::vector<qafuzz::Bytes> seeds() {
    using qasynth::gen4::Kind;
    return {qasynth::gen4::save(Kind::DiamondPearl), qasynth::gen4::save(Kind::Platinum),
            qasynth::gen4::save(Kind::HeartGoldSoulSilver),
            qasynth::gen4::save(Kind::Platinum, 7, 1, 0),       // newer general in partition 1
            qasynth::gen4::save(Kind::HeartGoldSoulSilver, 7, 0, 1)};
}

std::vector<qafuzz::SwapRegion> swaps() {
    using qasynth::gen4::Kind;
    std::vector<qafuzz::SwapRegion> out{{0, qasynth::gen4::kPartition, qasynth::gen4::kPartition}};
    for (Kind kind : {Kind::DiamondPearl, Kind::Platinum, Kind::HeartGoldSoulSilver}) {
        const auto sp = qasynth::gen4::spec(kind);
        const size_t g = sp.generalSize - 0x14, st = sp.storageStart + sp.storageSize - 0x14;
        out.push_back({g, qasynth::gen4::kPartition + g, 0x14});     // swapped general footers
        out.push_back({st, qasynth::gen4::kPartition + st, 0x14});   // swapped storage footers
    }
    return out;
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen4-sav", runOne, seeds, qasynth::gen4::forge, qasynth::gen4::countOffsets(), swaps(),
    };
    return target;
}
