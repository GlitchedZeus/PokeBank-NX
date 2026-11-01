#include "test_backup_session_trace.h"
// PokeBank NX application QA: Gen I-V acceptance matrix, Generation III rows (Thread 4, QA-owned).
//   ruby, sapphire, emerald, firered, leafgreen
//
// Every cell runs the production wiring on a SYNTHETIC save written to a temp directory:
//   discovery      Legacy::discoverConfiguredLegacySaves (the UIManager / SaveSelectScreen call)
//                  over a temp RetroArch battery-save root
//   linking        FRLGSourceBrowser cards -> LegacySourceBindings::claimInstanceAndSave ->
//                  reload from disk -> buildFRLGSourceCardsForProfile -> resolveFRLGSaveInstance
//   read model     Gen3::ReadOnlySave (strict parse held by the discovered source) and
//                  Legacy::FRLGReadOnlyTrainer (the TrainerViewScreen bridge)
//   editing        FRLGReadOnlyTrainer's staged Pokemon / Bag workspaces driven through the shared
//                  Gen III session model (UI/Gen3SharedPokemonSession.h)
//   export         Gen3::publishVerifiedGen3Export (Pokemon + Bag merged)
//   Home refresh   legacy re-discovery after the watched file's size/mtime changes
//                  (SaveSelectScreen::refreshLegacyWatch -> discoverConfiguredLegacySaves)
//
// Synthetic inputs:
//   FR/LG   build-host/frlg_fixture.sav (qasynth::gen3::fixture(), honours POKEBANK_QA_GEN3_FIXTURE),
//           the accepted pinned fixture written by test_pksm_gen3_adapter --write-fixture.
//   R/S/E   an in-memory builder copied from tests/test_rse_gen3_adapter.cpp (makePokemon /
//           makeLogicalSlot / writeSlot / makeFixture), the accepted RSE adapter fixture.
// Both get one Pokedex seen+caught flag so the Pokedex cell is not vacuous (offsets from production
// PKSMGen3NativeAdapter.cpp ReadOnlySave::dexProgress: logical sector 0 + 0x28 caught, + 0x5C seen).
#include "test_qa_matrix.h"
#include "test_qa_matrix_home.h"

#include "fuzz_synthetic_saves.h"
#include "test_qa_support.h"

#include "Encryption/Encryption3FRLG.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Integration/Gen3/Gen3ExportTransaction.h"
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "Pokemon/Experience.h"
#include "Source/SaveInstance.h"
#include "Source/SaveProviderIdentity.h"
#include "test_qa_matrix_round2.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "UI/ClassicInventoryUIModel.h"
#include "UI/SharedStatReroll.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "Utils/Gen3Text.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;
namespace G3 = PokeVault::Integration::Gen3;
namespace D = PokeVault::Integration::Gen3::Detail;
namespace L = PokeVault::Legacy;
namespace Src = PokeVault::Source;
namespace Common = PokeVault::Integration::Common;
namespace SessionModel = PokeBank::UIModel::Gen3SharedEditor;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
using qacontract::Bytes;

// ---------------------------------------------------------------------------------------------
// R/S/E synthetic builder, copied from tests/test_rse_gen3_adapter.cpp (makePokemon,
// makeLogicalSlot, writeSlot, makeFixture), the accepted strict RSE adapter fixture. Sector
// constants come from the production header Integration/Gen3/Gen3SaveValidation.h (Detail::).
// Only change: one Pokedex seen/caught flag (Bulbasaur) at the production dexProgress offsets.
// ---------------------------------------------------------------------------------------------
namespace rse {

enum class Family { RS, Emerald };

void write16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
void writeName(std::span<uint8_t> bytes, size_t offset, size_t length, std::u16string_view name) {
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), length, Utils::GEN3_TERMINATOR);
    for (size_t index = 0; index < std::min(length, name.size()); ++index)
        bytes[offset + index] = Utils::charToGen3(name[index], 2);
}
uint32_t packIvs(const std::array<uint8_t, 6>& ivs) {
    uint32_t packed = 0;
    for (size_t index = 0; index < ivs.size(); ++index)
        packed |= static_cast<uint32_t>(ivs[index] & 0x1F) << (index * 5);
    return packed;
}
Bytes makePokemon(uint16_t species, uint32_t pid, bool party) {
    Bytes canonical(party ? 100 : 80, 0);
    write32(canonical, 0x00, pid);
    write16(canonical, 0x04, 54321);
    write16(canonical, 0x06, 12345);
    writeName(canonical, 0x08, 10, u"RSEMON");
    canonical[0x12] = 2;
    canonical[0x13] = 2;
    writeName(canonical, 0x14, 7, u"WILL");
    write16(canonical, 0x20, species);
    write16(canonical, 0x22, 13);
    write32(canonical, 0x24, 123456);
    const uint16_t moves[4] = {33, 45, 73, 22};
    const uint8_t pp[4] = {35, 40, 10, 10};
    for (size_t index = 0; index < 4; ++index) {
        write16(canonical, 0x2C + index * 2, moves[index]);
        canonical[0x34 + index] = pp[index];
    }
    const uint8_t evs[6] = {10, 20, 30, 40, 50, 60};
    std::copy(std::begin(evs), std::end(evs), canonical.begin() + 0x38);
    write32(canonical, 0x48, packIvs({31, 30, 29, 28, 27, 26}));
    write16(canonical, 0x1C, Encryption::checksum3FRLG(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(canonical.data()), canonical.size())));
    std::unique_ptr<std::byte[]> encrypted(Encryption::encryptArray3FRLG(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(canonical.data()), canonical.size())));
    Bytes result(canonical.size());
    std::memcpy(result.data(), encrypted.get(), result.size());
    return result;
}
using LogicalSectors = std::array<std::array<uint8_t, D::kSectorSize>, D::kSectorCount>;
LogicalSectors makeLogicalSlot(Family family, uint16_t species, uint32_t pid) {
    LogicalSectors logical{};
    writeName(logical[0], 0x00, 8, u"WILL");
    logical[0][0x08] = 1;
    write16(logical[0], 0x0A, 54321);
    write16(logical[0], 0x0C, 12345);
    // QA addition: species 1 caught + seen (PKSMGen3NativeAdapter.cpp dexProgress offsets).
    logical[0][0x28] |= 0x01;
    logical[0][0x5C] |= 0x01;

    uint32_t securityKey = 0;
    if (family == Family::Emerald) {
        securityKey = 0xA1B2C3D4;
        write32(logical[0], 0xAC, securityKey);
        logical[0][0xEE0] = 0x42;
    } else {
        write32(logical[0], 0xAC, 0);
    }
    write32(logical[1], 0x490, 500000u ^ securityKey);
    const auto writeInventoryEntry = [&](size_t offset, uint16_t itemId, uint16_t count, bool keyed) {
        write16(logical[1], offset, itemId);
        const uint16_t key16 = keyed ? static_cast<uint16_t>(securityKey) : 0;
        write16(logical[1], offset + 2, static_cast<uint16_t>(count ^ key16));
    };
    if (family == Family::RS) {
        writeInventoryEntry(0x0560, 13, 3, false);  // Items: Potion x3
        writeInventoryEntry(0x05B0, 259, 1, false); // Key Items
        writeInventoryEntry(0x0600, 4, 7, false);   // Poke Balls
        writeInventoryEntry(0x0640, 289, 2, false); // TM/HM
        writeInventoryEntry(0x0740, 133, 5, false); // Berries
        writeInventoryEntry(0x0498, 13, 9, false);  // PC Items
    } else {
        writeInventoryEntry(0x0560, 13, 3, true);
        writeInventoryEntry(0x05D8, 259, 1, true);
        writeInventoryEntry(0x0650, 4, 7, true);
        writeInventoryEntry(0x0690, 289, 2, true);
        writeInventoryEntry(0x0790, 133, 5, true);
        writeInventoryEntry(0x0498, 13, 9, false);
    }
    logical[1][0x234] = 1;
    const auto party = makePokemon(species, pid, true);
    std::copy(party.begin(), party.end(), logical[1].begin() + 0x238);
    const auto boxed = makePokemon(species, pid, false);
    std::copy(boxed.begin(), boxed.end(), logical[5].begin() + 4);
    // Box 2 slot 20, crossing the section 5/6 boundary, as in the source fixture.
    const auto splitBoxed = makePokemon(2, pid + 24, false);
    constexpr size_t splitLogical = 4 + 49 * 80;
    const size_t firstPart = D::kSectorDataSize - splitLogical;
    std::copy_n(splitBoxed.begin(), firstPart, logical[5].begin() + splitLogical);
    std::copy(splitBoxed.begin() + static_cast<std::ptrdiff_t>(firstPart), splitBoxed.end(),
              logical[6].begin());
    return logical;
}
void writeSlot(Bytes& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family,
               uint16_t species, uint32_t pid) {
    auto logical = makeLogicalSlot(family, species, pid);
    for (size_t physical = 0; physical < D::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % D::kSectorCount);
        const size_t offset = D::kSlotBases[slot] + physical * D::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, D::sectorChecksum(
            std::span<const uint8_t>(save.data() + offset, D::kChunkLengths[id])));
        write32(save, offset + 0xFF8, D::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}
Bytes makeFixture(Family family) {
    Bytes save(D::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, 1, 0x23456789);
    writeSlot(save, 1, 9, 5, family, 1, 0x12345679);
    return save;
}

} // namespace rse

// FR/LG: the accepted fixture plus one Pokedex flag (same production dexProgress offsets),
// written into logical sector 0 of both rotating slots, then sector checksums re-forged with
// qasynth::gen3::forge (tests/fuzz_synthetic_saves.h).
Bytes frlgSave() {
    auto bytes = qasynth::gen3::fixture();
    if (bytes.size() != D::kSaveSize) return {};
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const auto v = D::validateSlot(bytes, slot);
        if (!v.valid) continue;
        bytes[v.logicalSectorOffsets[0] + 0x28] |= 0x01;
        bytes[v.logicalSectorOffsets[0] + 0x5C] |= 0x01;
    }
    qasynth::gen3::forge(bytes);
    return bytes;
}

// Export byte whitelist: copied from tests/test_qa_export_contracts.cpp gen3Whitelist(). Only the
// active rotating slot's sector payload and checksum fields may change; ids, signatures, counters
// and the whole inactive slot must not.
std::vector<qacontract::Range> gen3Whitelist(uint8_t activeSlot) {
    std::vector<qacontract::Range> out;
    for (size_t s = 0; s < D::kSectorCount; ++s) {
        const size_t off = D::kSlotBases[activeSlot] + s * D::kSectorSize;
        out.push_back({off, off + 0xFF4});
        out.push_back({off + 0xFF6, off + 0xFF8});
    }
    return out;
}

struct Spec {
    const char* version;
    const char* gameId;
    G3::SourceGame game;
    const char* fileName;      // RetroArch battery-save name carrying the release hint
    const char* wrongFamilyName;
    bool frlg;
    bool emerald;
    uint8_t origin;            // PK3 origin game, as in tests/test_gen3_staged_pokemon_editor.cpp origin()
    uint16_t potionCount;      // Items pouch Potion count in the synthetic input
};

// Records each in-scope cell exactly once; anything not reached becomes Fail with the reason.
class Row {
public:
    Row(qamatrix::Matrix& m, std::string version) : m_(m), v_(std::move(version)) {}
    void check(std::string_view cell, const std::vector<std::string>& errors, std::string passNote) {
        if (!mark(cell)) return;
        if (errors.empty()) {
            m_.pass(v_, cell, std::move(passNote));
            return;
        }
        std::string joined;
        for (const auto& e : errors) joined += (joined.empty() ? "" : "; ") + e;
        m_.fail(v_, cell, joined);
    }
    void fact(const std::string& key, std::string value) { m_.fact(v_, key, std::move(value)); }
    void finish(const std::string& reason) {
        for (auto cell : qamatrix::kCells)
            if (qamatrix::cellApplies(cell, v_) && !done_.count(std::string(cell)))
                m_.fail(v_, cell, "not reached: " + reason);
    }

private:
    bool mark(std::string_view cell) { return done_.insert(std::string(cell)).second; }
    qamatrix::Matrix& m_;
    std::string v_;
    std::set<std::string> done_;
};

struct Errors {
    std::vector<std::string> list;
    void expect(bool ok, std::string what) {
        if (!ok) list.push_back(std::move(what));
    }
};

const L::FRLGSource* findSource(const L::FRLGDiscoveryResult& result, const fs::path& path) {
    for (const auto& s : result.sources)
        if (s.path == path.string()) return &s;
    return nullptr;
}

const L::FRLGSourceCard* findCard(const std::vector<L::FRLGSourceCard>& cards, std::string_view id) {
    for (const auto& c : cards)
        if (c.gameId == id) return &c;
    return nullptr;
}

std::string str(size_t v) { return std::to_string(v); }

// ---------------------------------------------------------------------------------------------
// Round 2 cells (bag-add, reroll), each on a fresh FRLGReadOnlyTrainer bridge over the parsed
// synthetic save, so they never disturb the cells of the main flow.
//   bag-add  PokeBank::UIModel::classicItemQuantityRule + BagAdd::decidePick / decideRow, staged as
//            ClassicInventoryOverlay.cpp stageQuantity does (FRLGReadOnlyTrainer::stagedInventory()
//            ->stageInventoryQuantity); tests/test_bag_add_flow_gen15.cpp
//   reroll   the Gen III surface's Y handler (Gen3SharedPokemonSurface.inc): SharedStatReroll::reroll
//            into the session's working IVs/EVs, then Session::keep on the staged Pokemon workspace;
//            tests/test_reroll_mechanics_gen15.cpp
// ---------------------------------------------------------------------------------------------
void runRound2Gen3(Row& row, const Spec& spec, const G3::ReadOnlySave& save, const Bytes& original) {
    using PokeVault::Inventory::ClassicPocket;
    namespace R2 = qamatrix::round2;
    const auto guarded = [&](std::string_view cell, std::string note, const std::function<void(Errors&)>& body) {
        Errors e;
        try {
            body(e);
        } catch (const std::exception& ex) {
            e.expect(false, std::string("exception: ") + ex.what());
        }
        row.check(cell, e.list, std::move(note));
    };
    const bool frlg = spec.frlg;

    guarded("bag-add",
            std::string("classicItemQuantityRule + decidePick: Antidote StageNew x1 (no numpad) -> "
                        "FRLGReadOnlyTrainer::stagedInventory()->stageInventoryQuantity -> staged Items Antidote x1 "
                        "(strict reparse); row EditQuantity [1,") + (frlg ? "999" : "99") +
                "]; held Potion -> FocusExisting, nothing staged; Old Rod StageNew x1 then FixedQuantity, x2 refused",
            [&](Errors& e) {
        std::string error;
        auto bridge = L::FRLGReadOnlyTrainer::create(save, error);
        e.expect(bridge && bridge->stagedInventory(), "FRLGReadOnlyTrainer staged inventory: " + error);
        if (!bridge || !bridge->stagedInventory()) return;
        auto& inv = *bridge->stagedInventory();
        const auto game = PokeBank::UIModel::classicInventoryGame(spec.gameId);
        e.expect(game.has_value(), std::string("classicInventoryGame(") + spec.gameId + ")");
        if (!game) return;
        // ClassicInventoryOverlay.cpp gen3Pouch: Items / Key Items map one to one.
        const auto pouch = [](size_t pocket) {
            return static_cast<ClassicPocket>(pocket) == ClassicPocket::KeyItems ? G3::InventoryPouch::KeyItems
                                                                                 : G3::InventoryPouch::Items;
        };
        const auto item = [&](std::string name, ClassicPocket pocket, uint16_t id) {
            R2::BagItem i;
            i.name = std::move(name);
            i.id = id;
            i.rule = PokeBank::UIModel::classicItemQuantityRule(*game, pocket, id);
            i.pocket = static_cast<size_t>(pocket);
            return i;
        };
        R2::BagBackend b;
        b.held = [&](const R2::BagItem& i) { return inv.stagedInventoryQuantity(pouch(i.pocket), i.id); };
        b.state = [&b](const R2::BagItem& i) { return R2::BagAdd::PocketState{b.held(i), 0, 0}; };
        b.stage = [&](const R2::BagItem& i, uint16_t q, std::string& err) {
            return inv.stageInventoryQuantity(pouch(i.pocket), i.id, q, err);
        };
        b.bytes = [&inv] { return inv.stagedBytes(); };
        for (const auto& f : R2::bagAddChecks(b, item("Antidote", ClassicPocket::Items, 14),
                                              item("Potion", ClassicPocket::Items, 13),
                                              item("Old Rod", ClassicPocket::KeyItems, 262)))
            e.expect(false, f);
        const auto out = inv.finalizedBytes(error);
        e.expect(!out.empty(), "finalize: " + error);
        if (!out.empty()) {
            auto again = G3::StagedInventoryEditor::create(out, spec.game, error);
            e.expect(again && again->stagedInventoryQuantity(G3::InventoryPouch::Items, 14) == 1 &&
                         again->stagedInventoryQuantity(G3::InventoryPouch::KeyItems, 262) == 1,
                     "reparsed Bag lacks Antidote x1 / Old Rod x1: " + error);
        }
        e.expect(inv.originalBytes() == original, "Bag workspace original differs from the source");
    });

    guarded("reroll",
            "Y = SharedStatReroll::reroll into Session working IVs/EVs -> keep, x" + std::to_string(R2::kRerolls) +
                " per case: Bulbasaur shiny/non-shiny x male/female and Unown shiny/non-shiny keep shiny, gender, "
                "PID and Unown letter; IVs/EVs land as rolled",
            [&](Errors& e) {
        std::string error;
        auto bridge = L::FRLGReadOnlyTrainer::create(save, error);
        e.expect(bridge && bridge->stagedPokemon(), "FRLGReadOnlyTrainer staged Pokemon: " + error);
        if (!bridge || !bridge->stagedPokemon()) return;
        auto& editor = *bridge->stagedPokemon();
        namespace Reroll = PokeBank::UIModel::SharedStatReroll;
        Reroll::Rng rng(0x5EED3u);
        struct Case { uint16_t species; bool shiny; std::optional<uint8_t> gender; };
        const Case cases[] = {
            {0, false, 0}, {0, false, 1}, {0, true, 0}, {0, true, 1}, {201, false, std::nullopt}, {201, true, std::nullopt},
        };
        for (const auto& k : cases) {
            auto rec = editor.boxedPokemon(0, 0, error);
            e.expect(rec.has_value(), "Box 1/1 unreadable: " + error);
            if (!rec) return;
            SessionModel::Session setup;
            setup.begin(*rec, SessionModel::Mode::Edit);
            if (k.species) e.expect(setup.setSpecies(k.species, spec.gameId), "setSpecies " + str(k.species));
            // Round 11 (owner H28): Unown keeps none of the old moves and none is filled in; Keep
            // needs one (CreateRequiredFields), so the start state teaches Hidden Power.
            if (k.species == 201) { setup.working.moves[0] = 237; setup.working.pp[0] = 15; }
            e.expect(setup.setShiny(k.shiny), "setShiny");
            if (k.gender) e.expect(setup.setGender(*k.gender), "setGender " + str(*k.gender));
            if (!setup.keep(editor, 0, 0, error)) {
                e.expect(false, "Keep start state: " + error);
                continue;
            }
            rec = editor.boxedPokemon(0, 0, error);
            if (!rec) { e.expect(false, "Box 1/1 unreadable: " + error); return; }
            const std::string what = "species " + str(rec->species) + (k.shiny ? " shiny" : " non-shiny");
            e.expect(rec->shiny == k.shiny && (!k.gender || rec->gender == *k.gender) &&
                         (!k.species || rec->species == k.species),
                     what + ": start state not staged");
            const auto start = *rec;
            for (int i = 0; i < R2::kRerolls; ++i) {
                SessionModel::Session y;
                y.begin(*rec, SessionModel::Mode::Edit);
                auto& w = y.working;
                const auto next = Reroll::reroll(rng, {w.ivs, w.evs});
                w.ivs = next.ivs;
                w.evs = next.evs;
                if (!y.keep(editor, 0, 0, error)) {
                    e.expect(false, what + " Keep reroll " + str(static_cast<size_t>(i)) + ": " + error);
                    break;
                }
                rec = editor.boxedPokemon(0, 0, error);
                if (!rec) { e.expect(false, "Box 1/1 unreadable: " + error); return; }
                e.expect(rec->ivs == next.ivs && rec->evs == next.evs, what + ": rolled IVs/EVs not staged");
                e.expect(rec->shiny == start.shiny && rec->gender == start.gender && rec->pid == start.pid &&
                             rec->form == start.form && rec->nature == start.nature,
                         what + ": reroll " + str(static_cast<size_t>(i)) + " changed shiny/gender/PID/letter");
            }
        }
        e.expect(editor.originalBytes() == original, "Pokemon workspace original differs from the source");
    });
}

void runVersion(qamatrix::Matrix& m, const Spec& spec) {
    Row row(m, spec.version);
    const std::string game = spec.gameId;
    try {
        const Bytes original = spec.frlg ? frlgSave()
                                         : rse::makeFixture(spec.emerald ? rse::Family::Emerald : rse::Family::RS);
        if (original.size() != D::kSaveSize) {
            row.finish("synthetic Gen III input unavailable (build build-host/frlg_fixture.sav or set "
                       "POKEBANK_QA_GEN3_FIXTURE)");
            return;
        }
        const fs::path root = qacontract::scratchRoot("matrix-g3-" + std::to_string(spec.origin));
        // The root name must carry no release word: RetroArchFRLGDiscovery.cpp identityHint() scans
        // the WHOLE path (directories included), not only the file name.
        const fs::path retro = root / "retroarch" / "savefiles";
        const fs::path srcPath = retro / spec.fileName;
        const fs::path bindingsPath = root / "config" / "legacy_bindings.tsv";
        const fs::path exportRoot = root / "exports";
        fs::create_directories(bindingsPath.parent_path());
        qacontract::writeSource(srcPath, original);
        const auto sourceBefore = qacontract::fileState(srcPath);
        std::string backupError;
        const bool backupOk=verifyBackupSessionTrace(original,game,backupError);
        row.check("backup-session",backupOk?std::vector<std::string>{}:std::vector<std::string>{backupError},
            "Historical copy stages, separately exports and Discards without source mutation");
        const auto treeBefore = qacontract::treeState(retro);
        const std::string profile = "qa-profile-a";

        // The exact runtime catalog call (UIManager / SaveSelectScreen), pointed at temp roots:
        // RetroArch config absent -> conventional fallback root; mGBA / Tico not configured.
        const auto discover = [&] {
            return L::discoverConfiguredLegacySaves({}, (root / "absent" / "retroarch.cfg").string(),
                                                    retro.string(),
                                                    (root / "absent" / "config.ini").string(),
                                                    (root / "absent" / "tico").string());
        };

        // ---- discovery ------------------------------------------------------------------------
        auto catalog = discover();
        const L::FRLGSource* src = findSource(catalog, srcPath);
        {
            Errors e;
            e.expect(src != nullptr, "discoverConfiguredLegacySaves did not list " + srcPath.filename().string());
            if (src) {
                e.expect(src->ready() && src->isGen3() && src->status == L::LegacySourceStatus::Ready,
                         "source not Ready/Gen III: " + src->detail);
                e.expect(catalog.activeRootKind == L::FRLGDiscoveryResult::RootKind::ConventionalFallback,
                         "active root kind is not the conventional fallback");
            }
            row.check("discovery", e.list,
                      "discoverConfiguredLegacySaves (conventional RetroArch root) -> Ready Gen III source");
        }
        if (!src || !src->ready() || !src->save) {
            row.finish("discovery failed");
            return;
        }

        // ---- identity -------------------------------------------------------------------------
        {
            Errors e;
            e.expect(src->gameId == game, "gameId " + src->gameId + " != " + game);
            e.expect(src->save->metadata().sourceGameId == game, "parsed metadata id mismatch");
            // Production decides the exact release from the path hint, then strictly verifies the
            // family (RetroArchFRLGDiscovery.cpp identityHint/inspectFile; PKSMGen3Adapter.h parse()).
            const fs::path probe = root / "identity-probe";
            qacontract::writeSource(probe / "Mystery.sav", original);
            qacontract::writeSource(probe / spec.wrongFamilyName, original);
            const std::vector<std::string> probeRoots{probe.string()};
            const auto probed = L::discoverFRLGSaves(probeRoots);
            const auto* noHint = findSource(probed, probe / "Mystery.sav");
            const auto* wrong = findSource(probed, probe / spec.wrongFamilyName);
            e.expect(noHint && noHint->status == L::LegacySourceStatus::AmbiguousIdentity && !noHint->ready(),
                     "valid save without a release hint was not held as AmbiguousIdentity");
            e.expect(wrong && wrong->status == L::LegacySourceStatus::InvalidSave &&
                         wrong->parseError == G3::SaveError::UnsupportedGame,
                     std::string("wrong-family hint ") + spec.wrongFamilyName + " was not rejected");
            row.check("identity", e.list,
                      std::string("exact id from the RetroArch filename hint, family verified in the save; "
                                  "no hint -> AmbiguousIdentity, wrong family -> InvalidSave") +
                          (spec.frlg ? " (FR and LG share one family marker)"
                                     : spec.emerald ? " (Emerald family is save-internal but the exact id still needs the hint)"
                                                    : " (Ruby and Sapphire bytes are identical)"));
            row.fact("identity.source", "filename");
        }

        // ---- provider -------------------------------------------------------------------------
        const auto cards = L::buildFRLGSourceCards(catalog);
        const L::FRLGSourceCard* card = findCard(cards, game);
        {
            Errors e;
            e.expect(src->providerLabel == "RetroArch", "providerLabel '" + src->providerLabel + "'");
            e.expect(Src::identifyProvider(src->providerLabel) == Src::ProviderIdentity::RetroArch,
                     "identifyProvider is not RetroArch");
            e.expect(card && card->instances.size() == 1, "no single-instance card for " + game);
            if (card && !card->instances.empty()) {
                e.expect(card->sourceLabel == "RETROARCH", "card sourceLabel '" + card->sourceLabel + "'");
                e.expect(Src::cardProviderDisplay(Src::cardProviderLabel(card->instances)) == "RetroArch",
                         "card provider display is not RetroArch");
                e.expect(card->instances.front().platformLabel == "Game Boy Advance",
                         "platformLabel '" + card->instances.front().platformLabel + "'");
            }
            row.check("provider", e.list, "RetroArch (SaveProviderIdentity), platform Game Boy Advance");
        }
        if (!card || card->instances.empty()) {
            row.finish("no Save Instance card");
            return;
        }
        const Src::SaveInstance shownInstance = card->instances.front();

        // ---- linking --------------------------------------------------------------------------
        {
            Errors e;
            L::LegacySourceBindings bindings(bindingsPath.string());
            e.expect(bindings.load(), "bindings load: " + bindings.lastError());
            // Accepted contract (tests/test_legacy_frlg_browse.cpp): an unassigned validated save is
            // browseable by every profile until one profile claims it.
            e.expect(findCard(L::buildFRLGSourceCardsForProfile(catalog, bindings, "qa-profile-b"), game),
                     "unassigned source not browseable before the claim");
            e.expect(bindings.claimInstanceAndSave(shownInstance, profile),
                     "claimInstanceAndSave: " + bindings.lastError());
            L::LegacySourceBindings reloaded(bindingsPath.string());
            e.expect(reloaded.load(), "reload: " + reloaded.lastError());
            e.expect(reloaded.assignedProfile(shownInstance.sourceIdentity) == profile,
                     "reloaded binding does not name the profile");
            const auto mine = L::buildFRLGSourceCardsForProfile(catalog, reloaded, profile);
            const auto other = L::buildFRLGSourceCardsForProfile(catalog, reloaded, "qa-profile-b");
            const auto* mineCard = findCard(mine, game);
            e.expect(mineCard && mineCard->instances.size() == 1, "assigned card not visible to the owner");
            e.expect(other.empty(), "claimed source still visible to another profile");
            if (mineCard && mineCard->instances.size() == 1) {
                const auto* resolved = L::resolveFRLGSaveInstance(catalog, *mineCard, 0);
                e.expect(resolved && resolved->path == srcPath.string(),
                         "resolveFRLGSaveInstance did not resolve to the same file");
            }
            row.check("linking", e.list,
                      "unassigned: visible to all profiles; claimInstanceAndSave -> reload from disk -> owner-only card -> "
                      "resolveFRLGSaveInstance = same file");
        }

        const auto& save = *src->save;
        std::string trainerError;
        auto trainer = L::FRLGReadOnlyTrainer::create(save, trainerError);
        runRound2Gen3(row, spec, save, original);

        // ---- trainer-name / trainer-gender ----------------------------------------------------
        {
            Errors e;
            e.expect(save.trainer().name == "WILL", "trainer name '" + save.trainer().name + "'");
            e.expect(shownInstance.trainerName == "WILL", "Save Instance trainer '" + shownInstance.trainerName + "'");
            e.expect(trainer && trainer->trainerName == "WILL",
                     "FRLGReadOnlyTrainer name (" + trainerError + ")");
            row.check("trainer-name", e.list, "WILL (strict parse, Save Instance, trainer bridge)");
        }
        {
            Errors e;
            e.expect(save.trainer().gender == 1, "gender " + str(save.trainer().gender));
            e.expect(trainer && trainer->trainerGender == 1, "trainer bridge gender");
            row.check("trainer-gender", e.list, "stored (block 0 +0x08), female round-trips");
            row.fact("trainer.gender", "stored");
        }

        // ---- party / boxes / pokedex ----------------------------------------------------------
        {
            Errors e;
            const auto party = save.party();
            e.expect(save.lastEnumerationError() == G3::SaveError::None, "party enumeration error");
            e.expect(save.metadata().partyCount == 1 && party.size() == 1, "party count " + str(party.size()));
            if (!party.empty())
                e.expect(party[0].species == 1 && party[0].checksumValid && party[0].byteIdenticalRoundTrip,
                         "party[0] species " + str(party[0].species));
            e.expect(shownInstance.partyCount == 1, "Save Instance party count");
            row.check("party", e.list, "1 member (Bulbasaur), checksum-valid, byte-identical round trip");
        }
        {
            Errors e;
            const auto boxes = save.boxes();
            e.expect(save.lastEnumerationError() == G3::SaveError::None, "box enumeration error");
            e.expect(boxes.size() == 2, "box count " + str(boxes.size()));
            const bool first = std::any_of(boxes.begin(), boxes.end(), [](const auto& p) {
                return p.location.box == 0 && p.location.slot == 0 && p.species == 1; });
            const bool split = std::any_of(boxes.begin(), boxes.end(), [](const auto& p) {
                return p.location.box == 1 && p.location.slot == 19 && p.species == 2; });
            e.expect(first && split, "expected Box 1/1 Bulbasaur and sector-split Box 2/20 Ivysaur");
            e.expect(trainer && trainer->getBoxCount() == 14 && trainer->getSlotsPerBox() == 30,
                     "trainer bridge box geometry");
            e.expect(trainer && trainer->stagedPokemonAvailable(),
                     "staged Pokemon workspace unavailable: " +
                         (trainer ? trainer->stagedPokemonUnavailableReason() : trainerError));
            row.check("boxes", e.list, "14x30; Box 1/1 and sector-split Box 2/20 decoded");
        }
        {
            Errors e;
            const auto dex = save.dexProgress();
            e.expect(dex.seen == 1 && dex.caught == 1 && dex.total == 386,
                     "dex " + str(dex.seen) + "/" + str(dex.caught) + "/" + str(dex.total));
            row.check("pokedex", e.list, "seen 1 / caught 1 / total 386");
        }
        if (!trainer || !trainer->stagedPokemonAvailable() || !trainer->stagedInventoryAvailable()) {
            row.finish("FRLGReadOnlyTrainer staged workspaces unavailable: " + trainerError);
            return;
        }
        auto& editor = *trainer->stagedPokemon();
        auto& bag = *trainer->stagedInventory();

        // ---- bag ------------------------------------------------------------------------------
        {
            Errors e;
            const auto& inv = save.inventory();
            e.expect(inv.size() == 6, "pouch count " + str(inv.size()));
            bool potion = false;
            for (const auto& pouch : inv)
                for (const auto& item : pouch.items)
                    if (pouch.pouch == G3::InventoryPouch::Items && item.itemId == 13 && item.count == spec.potionCount)
                        potion = true;
            e.expect(potion, "Items pouch Potion x" + str(spec.potionCount) + " missing");
            if (!inv.empty()) e.expect(inv[0].countEncrypted == (spec.frlg || spec.emerald),
                                       "Items pouch quantity-key flag");
            std::string err;
            e.expect(bag.stageInventoryQuantity(G3::InventoryPouch::Items, 13, 7, err), "stage Potion x7: " + err);
            e.expect(bag.stagedInventoryQuantity(G3::InventoryPouch::Items, 13) == 7, "staged Potion quantity");
            e.expect(bag.originalBytes() == original, "Bag workspace original differs from the source");
            row.check("bag", e.list, "6 pouches read; Potion staged to 7 in the Bag workspace");
            row.fact("bag.read", "yes");
        }

        // ---- navigation -----------------------------------------------------------------------
        std::string err;
        const auto baseRecord = editor.boxedPokemon(0, 0, err);
        {
            Errors e;
            e.expect(baseRecord.has_value(), "Box 1/1 not readable: " + err);
            if (baseRecord) {
                const auto stagedBefore = editor.stagedBytes();
                for (auto mode : {SessionModel::Mode::View, SessionModel::Mode::Edit}) {
                    SessionModel::Session session;
                    session.begin(*baseRecord, mode);
                    Shared::Focus focus{};
                    const Workspace::Keys sequence[] = {
                        {false, true}, {false, false, false, true}, {false, false, false, false, false, true},
                        {false, true}, {false, false, true}, {true}, {false, false, false, false, true},
                        {true}, {false, true}, {false, true}, {false, false, false, true},
                    };
                    for (const auto& keys : sequence) {
                        const auto result = Workspace::navigate(
                            Shared::Generation::Gen3, focus, keys, mode == SessionModel::Mode::View,
                            session.editable(), 15, [](int) { return true; }, [](int) { return true; });
                        focus = result.focus;
                    }
                    e.expect(!session.dirty() && SessionModel::sameEditableRecord(session.working, *baseRecord),
                             "focus navigation changed the draft");
                }
                e.expect(editor.stagedBytes() == stagedBefore && !editor.hasPendingChanges(),
                         "focus navigation changed the staged workspace");
            }
            row.check("navigation", e.list,
                      "SharedPokemonWorkspace::navigate(Gen3) in View/Edit; draft and workspace unchanged "
                      "(the Gen III surface calls the same Shared:: primitives inline)");
            row.fact("nav.mutates", "no");
        }

        // ---- creation -------------------------------------------------------------------------
        {
            Errors e;
            SessionModel::Session create;
            create.beginCreate(editor.trainer(), editor.originGame());
            e.expect(create.working.species == 25 && create.working.level == 5, "Create defaults changed");
            e.expect(create.working.otName == editor.trainer().name && create.working.tid == editor.trainer().tid16,
                     "Create draft does not carry the trainer OT/TID");
            e.expect(!create.setLevel(0) && !create.setLevel(101) && !create.setSpecies(0) && !create.setSpecies(387),
                     "Create draft accepted an out-of-range level/species");
            const auto stagedBefore = editor.stagedBytes();
            SessionModel::Session occupied;
            occupied.beginCreate(editor.trainer(), editor.originGame());
            e.expect(!occupied.add(editor, 0, 0, err) && editor.stagedBytes() == stagedBefore,
                     "Create into an occupied slot was accepted");
            SessionModel::Session badOt;
            badOt.beginCreate(editor.trainer(), editor.originGame());
            badOt.working.otName = "TOOLONGOT";
            e.expect(!badOt.add(editor, 0, 1, err) && editor.stagedBytes() == stagedBefore,
                     "Create with an 8-character OT was accepted");
            // Round 11 (owner H07 for all gens / H28): a new Pokemon starts with no moves, and Add
            // refuses until a move and (where the game lists encounters) a Met Location are chosen.
            e.expect(create.working.moves == (std::array<uint16_t, 4>{}), "Create draft starts with moves");
            e.expect(!create.add(editor, 0, 1, err) && err.find("at least one move") != std::string::npos &&
                         editor.stagedBytes() == stagedBefore, "Create without a move was accepted: " + err);
            create.working.moves[0] = 84;   // Thunder Shock, as the move picker applies it
            create.working.pp[0] = 30;
            create.noteMetLocationChosen();  // the Met picker's choice (its encounter list is pinned elsewhere)
            err.clear();
            e.expect(create.add(editor, 0, 1, err), "Create add: " + err);
            const auto added = editor.boxedPokemon(0, 1, err);
            e.expect(added && added->species == 25 && added->level == 5 && added->originGame == spec.origin &&
                         added->otName == "WILL" && added->ball == 4 && added->language == 2,
                     "created record does not match the defaults / exact origin game " + str(spec.origin));
            row.check("creation", e.list,
                      "defaults Pikachu Lv5, OT/TID from trainer, origin game " + str(spec.origin) +
                          "; occupied slot / long OT / out-of-range values refused");
            row.fact("create.default.species", "25");
            row.fact("create.default.level", "5");
        }

        // ---- fields ---------------------------------------------------------------------------
        {
            Errors e;
            if (baseRecord) {
                SessionModel::Session edit;
                edit.begin(*baseRecord, SessionModel::Mode::Edit);
                edit.working.nickname = "QAMON";
                e.expect(edit.setLevel(30), "setLevel");
                edit.working.heldItem = 14;
                edit.working.moves = {33, 45, 73, 22};
                edit.working.pp = {30, 30, 10, 10};
                edit.working.ppUps = {1, 0, 0, 0};
                edit.working.ivs = {1, 2, 3, 4, 5, 6};
                edit.working.evs = {10, 20, 30, 40, 50, 60};
                edit.working.otName = "QAOT";
                e.expect(edit.setTid(1111), "setTid");  // the surface's TID path (keeps shininess)
                edit.working.friendship = 123;
                edit.working.pokerus = 0x21;
                edit.working.ball = 2;
                edit.working.metLevel = 5;
                // Round 11 FRLG (deliberately updated from 10 for every game): a changed met location
                // must be native to the record's origin game; FR/LG take a Kanto id (89 Viridian City).
                const uint16_t nativeMet = baseRecord->originGame == 4 || baseRecord->originGame == 5 ? 89 : 10;
                edit.working.metLocation = nativeMet;
                const bool kept = edit.keep(editor, 0, 0, err);
                e.expect(kept, "Keep (plain fields): " + err);
                auto rec = editor.boxedPokemon(0, 0, err);
                if (kept && rec) {
                    e.expect(rec->nickname == "QAMON" && rec->level == 30 && rec->heldItem == 14 &&
                                 rec->moves == edit.working.moves && rec->pp == edit.working.pp &&
                                 rec->ppUps == edit.working.ppUps && rec->ivs == edit.working.ivs &&
                                 rec->evs == edit.working.evs && rec->otName == "QAOT" && rec->tid == 1111 &&
                                 rec->friendship == 123 && rec->pokerus == 0x21 && rec->ball == 2 &&
                                 rec->metLevel == 5 && rec->metLocation == nativeMet,
                             "plain field values did not round-trip");
                    e.expect(rec->shiny == baseRecord->shiny, "TID edit flipped shininess");
                }
                // Experience as the progression source.
                if (rec) {
                    SessionModel::Session exp;
                    exp.begin(*rec, SessionModel::Mode::Edit);
                    const uint32_t exp42 = Pokemon::getExpForLevel(42, Pokemon::getGrowthRate(rec->species));
                    e.expect(exp.setExperience(exp42) && exp.keep(editor, 0, 0, err), "Keep (experience): " + err);
                    rec = editor.boxedPokemon(0, 0, err);
                    e.expect(rec && rec->experience == exp42 && rec->level == 42, "experience did not round-trip");
                }
                // PID-linked fields (Nature / Gender / Shiny / Ability) via the coordinated PID path.
                if (rec) {
                    SessionModel::Session pid;
                    pid.begin(*rec, SessionModel::Mode::Edit);
                    e.expect(pid.setNature(5) && pid.setGender(0) && pid.setShiny(true) && pid.setAbilityNumber(1),
                             "session refused a legal PID-linked value");
                    e.expect(pid.keep(editor, 0, 0, err), "Keep (PID-linked): " + err);
                    const auto linked = editor.boxedPokemon(0, 0, err);
                    e.expect(linked && linked->nature == 5 && linked->gender == 0 && linked->shiny &&
                                 linked->abilityNumber == 1 && linked->tid == 1111 && linked->sid == rec->sid &&
                                 linked->nickname == "QAMON",
                             "PID-linked values did not round-trip or disturbed other fields");
                }
                // Species change through the shared species/level policy.
                if (auto cur = editor.boxedPokemon(0, 0, err)) {
                    SessionModel::Session species;
                    species.begin(*cur, SessionModel::Mode::Edit);
                    const uint16_t newSpecies = spec.frlg ? 16 : 261;  // as in test_gen3_staged_pokemon_editor.cpp
                    e.expect(species.setSpecies(newSpecies, game), "setSpecies");
                    e.expect(species.keep(editor, 0, 0, err), "Keep (species): " + err);
                    const auto changed = editor.boxedPokemon(0, 0, err);
                    e.expect(changed && changed->species == newSpecies && changed->level == species.working.level,
                             "species change did not round-trip");
                }
                // Everything above survives finalize -> strict parse -> a fresh workspace.
                std::string ferr;
                const auto finalized = editor.finalizedBytes(ferr);
                e.expect(!finalized.empty(), "finalize: " + ferr);
                if (!finalized.empty()) {
                    e.expect(static_cast<bool>(G3::parse(finalized, spec.game)), "finalized bytes fail strict parse");
                    auto reopened = G3::StagedPokemonEditor::create(finalized, spec.game, ferr);
                    e.expect(reopened != nullptr, "reopen: " + ferr);
                    if (reopened)
                        for (size_t slot : {size_t{0}, size_t{1}}) {
                            const auto a = editor.boxedPokemon(0, slot, ferr);
                            const auto b = reopened->boxedPokemon(0, slot, ferr);
                            e.expect(a && b && a->encryptedBytes == b->encryptedBytes,
                                     "Box 1/" + str(slot + 1) + " differs after finalize/reopen");
                        }
                }
            } else {
                e.expect(false, "no base record");
            }
            row.check("fields", e.list,
                      "nickname, level, EXP, item, moves/PP/PP Ups, IVs, EVs, OT, TID, friendship, Pokerus, "
                      "ball, met level/location, nature, gender, shiny, ability, species: Keep -> finalize -> "
                      "strict reparse -> reopen");
        }

        // ---- pickers --------------------------------------------------------------------------
        {
            Errors e;
            const auto descriptor = PokeVault::Integration::Gen3EditorProvider::descriptorForSource(game, true);
            e.expect(descriptor.has_value(), "no exact-format descriptor for " + game);
            if (descriptor) {
                namespace Exact = PokeBank::UIModel::ExactFormatEditor;
                const auto eval = [&](uint16_t species, uint16_t move, bool existing = false) {
                    return descriptor->moves.evaluate({game, species, 0, move, existing});
                };
                e.expect(eval(25, 85) == Exact::MoveCompatibilityResult::Compatible, "Pikachu Thunderbolt not native");
                e.expect(eval(25, 0) == Exact::MoveCompatibilityResult::Compatible, "empty move not accepted");
                e.expect(eval(25, 355) == Exact::MoveCompatibilityResult::Invalid, "move 355 accepted");
                e.expect(eval(387, 33) == Exact::MoveCompatibilityResult::Invalid, "species 387 accepted");
                e.expect(descriptor->moves.evaluate({"diamond_nds", 25, 0, 85, false}) ==
                             Exact::MoveCompatibilityResult::Invalid,
                         "move evaluator accepted a foreign game id");
            }
            const auto moves = G3::Learnset::selectableMoves(spec.game, 1, {});
            e.expect(moves.size() > 1 && moves.front() == 0 &&
                         std::find(moves.begin(), moves.end(), uint16_t{22}) != moves.end(),
                     "Bulbasaur move picker list lacks Vine Whip");
            e.expect(std::all_of(moves.begin(), moves.end(), [](uint16_t mv) { return mv < G3::Learnset::kMoveCount; }),
                     "move picker offered an id outside the native table");
            SessionModel::Session s;
            s.beginCreate(editor.trainer(), editor.originGame());
            e.expect(s.setSpecies(386, game) && !s.setSpecies(387, game) && s.setSpecies(1, game),
                     "species picker range is not 1-386");
            e.expect((SessionModel::nativeAbilitySlots(25) == std::vector<uint16_t>{1}) &&
                         (SessionModel::nativeAbilitySlots(280) == std::vector<uint16_t>{1, 2}),
                     "ability picker slots");
            row.check("pickers", e.list,
                      "species 1-386; exact-game move pools (Gen3LearnsetTable) via the exact-format descriptor; "
                      "native ability slots");
        }

        // ---- actions --------------------------------------------------------------------------
        {
            Errors e;
            // The Gen III surface's call: actionsForSlot(occupied, {clone, remove, legality}, pending).
            const auto occupied = Shared::actionsForSlot(true, {true, true, true}, editor.hasPendingChanges());
            const auto empty = Shared::actionsForSlot(false, {true, true, true}, editor.hasPendingChanges());
            const std::vector<Shared::Action> wantOccupied = {
                Shared::Action::View, Shared::Action::Edit, Shared::Action::Clone, Shared::Action::Remove,
                Shared::Action::LegalityProvenance, Shared::Action::Review, Shared::Action::Close};
            const std::vector<Shared::Action> wantEmpty = {Shared::Action::Add, Shared::Action::Review,
                                                           Shared::Action::Close};
            e.expect(std::vector<Shared::Action>(occupied.values.begin(), occupied.values.begin() + occupied.count) ==
                         wantOccupied,
                     "occupied-slot Actions list");
            e.expect(std::vector<Shared::Action>(empty.values.begin(), empty.values.begin() + empty.count) == wantEmpty,
                     "empty-slot Actions list");
            // Clone and Remove execute on the staged workspace with exact sparse slots.
            const auto cloneSource = editor.boxedPokemon(0, 0, err);
            e.expect(editor.stageCloneBoxPokemon(0, 0, 0, 3, err), "Clone: " + err);
            const auto cloned = editor.boxedPokemon(0, 3, err);
            e.expect(cloneSource && cloned && cloned->encryptedBytes == cloneSource->encryptedBytes,
                     "Clone is not byte-identical");
            e.expect(editor.stageReleaseBoxPokemon(0, 3, err) && !editor.boxedPokemon(0, 3, err),
                     "Remove did not empty the slot");
            e.expect(editor.boxedPokemon(0, 0, err).has_value(), "Remove disturbed a neighbour");
            err.clear();
            row.check("actions", e.list,
                      "View/Edit/Clone/Remove/Legality/Review/Cancel (occupied), Add/Review/Cancel (empty); "
                      "Clone and Remove staged sparse");
        }

        // ---- working-copy ---------------------------------------------------------------------
        {
            Errors e;
            e.expect(qacontract::fileState(srcPath) == sourceBefore, "source file changed while staging");
            e.expect(editor.originalBytes() == original && bag.originalBytes() == original,
                     "workspace original is not the source snapshot");
            e.expect(editor.stagedBytes() != original && bag.stagedBytes() != original,
                     "staged copies do not carry the edits");
            e.expect(src->save->sourceBytes().size() == original.size() &&
                         std::equal(original.begin(), original.end(), src->save->sourceBytes().begin()),
                     "the catalog's parsed snapshot changed");
            row.check("working-copy", e.list, "source untouched; Pokemon and Bag staged copies carry the edits");
        }

        // ---- review ---------------------------------------------------------------------------
        {
            Errors e;
            const auto& changes = editor.pendingChanges();
            e.expect(changes.size() >= 4, "only " + str(changes.size()) + " staged Pokemon change line(s)");
            const bool box1 = std::any_of(changes.begin(), changes.end(), [](const auto& c) {
                return c.box == 0 && c.slot == 0 && !c.label.empty(); });
            const bool created = std::any_of(changes.begin(), changes.end(), [](const auto& c) {
                return c.box == 0 && c.slot == 1; });
            e.expect(box1 && created, "review lacks the Box 1/1 edit or the Box 1/2 create");
            const auto& bagChanges = bag.pendingChanges();
            e.expect(bagChanges.size() == 1 && bagChanges[0].itemId == 13 &&
                         bagChanges[0].before == spec.potionCount && bagChanges[0].after == 7,
                     "Bag review line");
            row.check("review", e.list,
                      str(changes.size()) + " Pokemon change lines (edit, create, clone, remove) + 1 Bag line");
        }

        // ---- export ---------------------------------------------------------------------------
        Bytes exported;
        {
            Errors e;
            G3::Gen3ExportRequest request;
            request.rootDirectory = exportRoot.string();
            request.directoryStem = game + "_20261015_120000";
            request.sourcePath = srcPath.string();
            request.timestamp = "20261015_120000";
            const auto result = G3::publishVerifiedGen3Export(editor, request, &bag);
            e.expect(result.success, "publishVerifiedGen3Export: " + result.error);
            if (result.success) {
                const fs::path dir(result.directory);
                e.expect(dir.parent_path() == exportRoot, "export left the app-owned root");
                e.expect(qacontract::slurp(dir / "original_backup.sav") == original, "backup is not the source");
                exported = qacontract::slurp(dir / "edited.sav");
                auto parsed = G3::parse(exported, spec.game);
                e.expect(static_cast<bool>(parsed), "edited.sav fails strict reparse as " + game);
                size_t offender = 0, changed = 0;
                e.expect(qacontract::onlyWhitelistedChanges(original, exported, gen3Whitelist(editor.activeSlot()),
                                                            &offender, &changed),
                         "byte outside the active-slot payload changed at " + qacontract::hex(offender));
                if (parsed) {
                    bool potion = false;
                    for (const auto& pouch : parsed.save->inventory())
                        for (const auto& item : pouch.items)
                            if (pouch.pouch == G3::InventoryPouch::Items && item.itemId == 13 && item.count == 7)
                                potion = true;
                    e.expect(potion, "exported Bag lacks Potion x7");
                }
                auto reopened = G3::StagedPokemonEditor::create(exported, spec.game, err);
                const auto mon = reopened ? reopened->boxedPokemon(0, 0, err) : std::nullopt;
                e.expect(mon && mon->nickname == "QAMON" && mon->shiny, "exported Box 1/1 lacks the edit");
                // S06: corrupt byte 0 (player name) of logical section 0 in the active slot. The sector
                // checksum covers it and the edit check does not read it.
                size_t section0 = exported.size();
                for (size_t sct = 0; sct < D::kSectorCount; ++sct) {
                    const size_t off = D::kSlotBases[editor.activeSlot()] + sct * D::kSectorSize;
                    if (off + 0xFF6 <= exported.size() && (exported[off + 0xFF4] | exported[off + 0xFF5] << 8) == 0)
                        section0 = off;
                }
                const auto s06 = qamatrix::exportCorruptionProbe(exported, section0, [&](const auto& bytes) {
                    auto p = G3::parse(bytes, spec.game);
                    if (!p) return false;
                    bool potion = false;
                    for (const auto& pouch : p.save->inventory())
                        for (const auto& item : pouch.items)
                            potion |= pouch.pouch == G3::InventoryPouch::Items && item.itemId == 13 && item.count == 7;
                    std::string why;
                    auto again = G3::StagedPokemonEditor::create(bytes, spec.game, why);
                    const auto boxed = again ? again->boxedPokemon(0, 0, why) : std::nullopt;
                    return potion && boxed && boxed->nickname == "QAMON";
                });
                e.expect(s06.empty(), s06);
                if (s06.empty()) row.fact(std::string(qamatrix::kExportChecksumFact),
                                          std::string(qamatrix::kExportChecksumDetected));
                std::ifstream manifestIn(dir / "EDIT_MANIFEST.txt");
                const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
                e.expect(manifest.find("GAME_ID=" + game) != std::string::npos, "manifest GAME_ID");
            }
            e.expect(qacontract::treeState(retro) == treeBefore, "export wrote into the emulator save root");
            row.check("export", e.list,
                      "new app-owned directory; edited.sav strict reparse; only active-slot payload/checksums "
                      "changed; Pokemon+Bag merged");
            row.fact("export.output", "new-file-only");
            row.fact("export.reparse", "strict");
        }

        // ---- immutability ---------------------------------------------------------------------
        {
            Errors e;
            e.expect(qacontract::fileState(srcPath) == sourceBefore, "source bytes or mtime changed");
            e.expect(qacontract::treeState(retro) == treeBefore, "emulator save root changed");
            row.check("immutability", e.list,
                      "bytes, size and ns mtime unchanged after discovery, linking, staging, review and export");
        }

        // ---- home-refresh ---------------------------------------------------------------------
        row.fact("home.refresh", "legacy-rediscovery");
        // Derived from the production Home adapter (HomePartyPreview::fromGen3, which
        // SaveSelectScreen.cpp feeds into the Home strip), never hard-coded.
        qamatrix::home::recordGen3([&](const std::string& key, const std::string& value) { row.fact(key, value); });
        {
            Errors e;
            e.expect(!exported.empty(), "no exported save to restore");
            if (!exported.empty()) {
                // The emulator (or the user restoring the export) rewrites the battery save.
                qacontract::spit(srcPath, exported);
                struct timespec times[2] = {{1100000000, 0}, {1100000000, 0}};
                ::utimensat(AT_FDCWD, srcPath.c_str(), times, 0);
                struct stat st{};
                ::stat(srcPath.c_str(), &st);
                // SaveSelectScreen re-discovers only when the watched size/mtime differ.
                e.expect(static_cast<int64_t>(st.st_mtime) != shownInstance.modifiedTime,
                         "watch condition (mtime change) not observable");
                L::LegacySourceBindings bindings(bindingsPath.string());
                e.expect(bindings.load(), "bindings load");
                const auto refreshed = discover();
                const auto homeCards = L::buildFRLGSourceCardsForProfile(refreshed, bindings, profile);
                const auto* homeCard = findCard(homeCards, game);
                e.expect(homeCard && homeCard->instances.size() == 1, "refreshed card not visible");
                if (homeCard && homeCard->instances.size() == 1) {
                    const auto& fresh = homeCard->instances.front();
                    e.expect(fresh.sourceIdentity == shownInstance.sourceIdentity, "source identity not stable");
                    e.expect(!Src::sameValidatedSnapshot(shownInstance, fresh), "stale snapshot reported current");
                    e.expect(fresh.contentFingerprint == Common::sha256Hex(exported), "fingerprint is not the new file");
                    const auto* source = L::resolveFRLGSaveInstance(refreshed, *homeCard, 0);
                    e.expect(source && source->save, "refreshed instance does not resolve");
                    if (source && source->save) {
                        const auto boxes = source->save->boxes();
                        e.expect(std::any_of(boxes.begin(), boxes.end(), [](const auto& p) {
                                     return p.location.box == 0 && p.location.slot == 1 && p.species == 25; }),
                                 "refreshed snapshot does not show the new Box 1/2 Pikachu");
                        e.expect(source->save->party().size() == 1 && source->save->trainer().name == "WILL",
                                 "refreshed party/trainer for the Home card");
                    }
                }
            }
            row.check("home-refresh", e.list,
                      "size/mtime change -> discoverConfiguredLegacySaves -> profile card shows the new snapshot "
                      "(legacy re-discovery; Home party strip passes level 0, form 0, shiny false: "
                      "SaveSelectScreen.cpp RetroArchFRLG preview)");
        }

        // ---- relaunch -------------------------------------------------------------------------
        {
            Errors e;
            const auto current = qacontract::slurp(srcPath);
            L::LegacySourceBindings fresh(bindingsPath.string());
            e.expect(fresh.load(), "fresh bindings load: " + fresh.lastError());
            const auto relaunchCatalog = discover();
            const auto relaunchCards = L::buildFRLGSourceCardsForProfile(relaunchCatalog, fresh, profile);
            const auto* relaunchCard = findCard(relaunchCards, game);
            e.expect(relaunchCard && relaunchCard->instances.size() == 1, "card not visible after relaunch");
            if (relaunchCard && relaunchCard->instances.size() == 1) {
                const auto* source = L::resolveFRLGSaveInstance(relaunchCatalog, *relaunchCard, 0);
                e.expect(source && source->path == srcPath.string() &&
                             source->contentFingerprint == Common::sha256Hex(current),
                         "relaunch did not resolve the current file");
                if (source && source->save) {
                    std::string terr;
                    auto bridge = L::FRLGReadOnlyTrainer::create(*source->save, terr);
                    e.expect(bridge && bridge->stagedPokemonAvailable() &&
                                 bridge->stagedPokemon()->originalBytes() == current &&
                                 !bridge->stagedPokemon()->hasPendingChanges(),
                             "relaunch workspace is not a clean copy of the current file: " + terr);
                }
            }
            row.check("relaunch", e.list,
                      "fresh bindings load + fresh discovery resolve the current file; clean workspace");
        }
        row.finish("unexpected early exit");
        fs::remove_all(root);
    } catch (const std::exception& ex) {
        row.finish(std::string("exception: ") + ex.what());
    }
}

} // namespace

namespace qamatrix {

void runGen3(Matrix& m) {
    const Spec specs[] = {
        {"ruby", "ruby_gba", G3::SourceGame::RubyGBA, "Pokemon Ruby.sav", "Pokemon FireRed.sav", false, false, 2, 3},
        {"sapphire", "sapphire_gba", G3::SourceGame::SapphireGBA, "Pokemon Sapphire.srm", "Pokemon Emerald.sav",
         false, false, 1, 3},
        {"emerald", "emerald_gba", G3::SourceGame::EmeraldGBA, "Pokemon Emerald.sav", "Pokemon Ruby.sav", false,
         true, 3, 3},
        {"firered", "firered_gba", G3::SourceGame::FireRedGBA, "Pokemon FireRed.sav", "Pokemon Ruby.sav", true,
         false, 4, 25},
        {"leafgreen", "leafgreen_gba", G3::SourceGame::LeafGreenGBA, "Pokemon LeafGreen.srm", "Pokemon Emerald.srm",
         true, false, 5, 25},
    };
    for (const auto& spec : specs) runVersion(m, spec);
}

} // namespace qamatrix
