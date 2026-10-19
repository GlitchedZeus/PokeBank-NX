// PokeBank NX application QA: Gen I-V acceptance matrix, Generation IV rows (Thread 4, QA-owned).
//
// Diamond, Pearl, Platinum, HeartGold and SoulSilver, each driven through the production Gen IV
// entry points on a SYNTHETIC save written to a temporary directory:
//   discoverSources / inspectSourceFile / toSaveInstance      (Gen4SourceDiscovery.h)
//   LegacySourceBindings + openAssignedSource                  (Gen4AssignedSource.h)
//   Gen4ReadOnlySave / decodeReadOnlyBag / Gen4ReadOnlyTrainer (the Trainer/Party/Box/Bag read model)
//   Gen4StagedPokemonEditor + Gen4SharedEditor::Session        (staged Edit / Create / Keep)
//   SharedPokemonWorkspace::navigate, Gen4BoxMoveNavigation, Gen4BoxTouchActions
//   describeStagedSlotChanges / publishVerifiedGen4Export      (review + verified export)
//   HomeSnapshotCache + assignedSnapshotRequest + homeCardSnapshot (the SaveSelectScreen wiring)
//
// Save layout: qasynth::gen4::save() in tests/fuzz_synthetic_saves.h (offsets as in
// tests/test_gen4_staged_pokemon_editor.cpp, PKHeX SAV4 DP/Pt/HGSS). Additional bytes written here:
//   * party form byte 0x40 (form << 3): tests/test_home_card_gen4_snapshot.cpp partyEntity()
//     (PKHeX PK4 0x40); party level 0x8C and stats 0x8E..0x9A: qasynth::gen4::party().
//   * Pokedex block offsets 0x12DC (DP) / 0x1328 (Pt) / 0x12B8 (HGSS) and the Zukan4 caught/seen
//     bit arrays at +4 / +0x44: src/Integration/Gen4/Gen4ReadOnlySave.cpp specFor() and dexProgress().
//   * bag pockets: production Gen4::bagLayout() in include/Integration/Gen4/Gen4ReadOnlyInventory.h.
// No real save, ROM or key is read.
#include "test_qa_matrix.h"

#include "fuzz_synthetic_saves.h"
#include "test_qa_support.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Integration/Gen4/Gen4ExactFormatEditorProvider.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4GameCardPreview.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/LegacySourceBindings.h"
#include "Names/Gen4HeldItemCatalog.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Source/HomeCardSnapshot.h"
#include "Source/SaveProviderIdentity.h"
#include "UI/Gen4BoxMoveNavigation.h"
#include "UI/Gen4BoxTouchActions.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "UI/Gen4BagQuantity.h"
#include "UI/SharedStatReroll.h"
#include "test_qa_matrix_round2.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "Utils/StringHelpers.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qamatrix {
namespace {

namespace fs = std::filesystem;
namespace G4 = PokeVault::Integration::Gen4;
namespace S = PokeVault::Source;
namespace Legacy = PokeVault::Legacy;
namespace WS = PokeBank::UIModel::SharedPokemonWorkspace;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Session4 = PokeBank::UIModel::Gen4SharedEditor;
namespace BoxNav = PokeBank::UIModel::Gen4BoxMoveNavigation;
namespace BoxTouch = PokeBank::UIModel::Gen4BoxTouchActions;
namespace Provider4 = PokeVault::Integration::Gen4EditorProvider;
namespace Compat4 = PokeVault::Integration::Gen4MoveCompatibility;
using GV = Enums::GameVersion;
using qasynth::Bytes;
using qasynth::gen4::Kind;

constexpr const char* kProfile = "qa-profile";
constexpr const char* kProvider = "DraStic";

struct VersionSpec {
    const char* version;
    const char* gameId;
    Kind kind;
    G4::Layout layout;
    uint8_t romCode;      // trainer +0x1C; only HGSS reads it (Gen4ReadOnlySave.cpp parse())
    GV exact;             // the exact game production must end up with
    GV fromSave;          // what the save alone proves (Invalid for raw DP)
    const char* family;   // expectedRawFamily (Gen4SourceDiscovery.cpp familyName)
    const char* wrongGame;   // an assignment the save must refuse
    const char* sibling;     // D <-> P: same bytes, other assignment (nullptr otherwise)
    GV siblingExact;
    size_t pokedexOffset;    // Gen4ReadOnlySave.cpp specFor() pokedexOffset
};

const std::vector<VersionSpec>& versions() {
    static const std::vector<VersionSpec> list = {
        {"diamond", "diamond_nds", Kind::DiamondPearl, G4::Layout::DiamondPearl, 7, GV::D, GV::Invalid,
         "DP", "platinum_nds", "pearl_nds", GV::P, 0x12DC},
        {"pearl", "pearl_nds", Kind::DiamondPearl, G4::Layout::DiamondPearl, 7, GV::P, GV::Invalid,
         "DP", "heartgold_nds", "diamond_nds", GV::D, 0x12DC},
        {"platinum", "platinum_nds", Kind::Platinum, G4::Layout::Platinum, 7, GV::Pt, GV::Pt,
         "PT", "diamond_nds", nullptr, GV::Invalid, 0x1328},
        {"heartgold", "heartgold_nds", Kind::HeartGoldSoulSilver, G4::Layout::HeartGoldSoulSilver,
         static_cast<uint8_t>(GV::HG), GV::HG, GV::HG, "HGSS", "soulsilver_nds", nullptr, GV::Invalid, 0x12B8},
        {"soulsilver", "soulsilver_nds", Kind::HeartGoldSoulSilver, G4::Layout::HeartGoldSoulSilver,
         static_cast<uint8_t>(GV::SS), GV::SS, GV::SS, "HGSS", "heartgold_nds", nullptr, GV::Invalid, 0x12B8},
    };
    return list;
}

// Collects every failed expectation of one cell so a Fail note names all of them.
struct Probe {
    std::vector<std::string> bad;
    void expect(bool ok, std::string what) {
        if (!ok) bad.push_back(std::move(what));
    }
    [[nodiscard]] bool ok() const noexcept { return bad.empty(); }
    [[nodiscard]] std::string note() const {
        std::string out;
        for (const auto& b : bad) out += (out.empty() ? "" : "; ") + b;
        return out;
    }
};

// Runs one cell; an exception is a Fail with its message, never a silent skip.
void cell(Matrix& m, const std::string& version, std::string_view name, std::string passNote,
          const std::function<void(Probe&)>& body) {
    Probe p;
    try {
        body(p);
    } catch (const std::exception& e) {
        p.bad.push_back(std::string("exception: ") + e.what());
    } catch (...) {
        p.bad.push_back("unknown exception");
    }
    m.check(version, name, p.ok(), p.note(), std::move(passNote));
}

// ------------------------------------------------------------------------------------------------
// Synthetic save
// ------------------------------------------------------------------------------------------------
constexpr uint16_t kPotion = 17;       // Gen IV item id 17 (Potion), Medicine pocket
constexpr uint16_t kPotionCount = 5;
constexpr uint16_t kBurmy = 412;      // three Gen IV cloak forms (personal formCount 3)
constexpr uint8_t kBurmyForm = 1;
constexpr uint8_t kBurmyLevel = 9;

// Party record exactly as qasynth::gen4::party(), plus the form byte from
// tests/test_home_card_gen4_snapshot.cpp partyEntity() (0x40 = form << 3).
std::vector<std::byte> partyRecord(uint16_t species, uint8_t level, uint8_t form) {
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    const auto decoded = qasynth::gen4::decryptedStored(species);
    std::copy(decoded.begin(), decoded.end(), d.begin());
    d[0x40] = static_cast<std::byte>(form << 3);
    d[0x8C] = static_cast<std::byte>(level);
    qasynth::gen4::wb16(d, 0x8E, 35); qasynth::gen4::wb16(d, 0x90, 35); qasynth::gen4::wb16(d, 0x92, 25);
    qasynth::gen4::wb16(d, 0x94, 20); qasynth::gen4::wb16(d, 0x96, 30); qasynth::gen4::wb16(d, 0x98, 25);
    qasynth::gen4::wb16(d, 0x9A, 20);
    return Encryption::encryptArray4(d);
}

Bytes buildSave(const VersionSpec& v) {
    Bytes s = qasynth::gen4::save(v.kind, v.romCode);
    const auto sp = qasynth::gen4::spec(v.kind);
    const auto burmy = partyRecord(kBurmy, kBurmyLevel, kBurmyForm);
    const auto bag = G4::bagLayout(v.layout)[static_cast<size_t>(G4::BagPocket::Medicine)];
    for (size_t partition = 0; partition < 2; ++partition) {
        const size_t base = partition * qasynth::gen4::kPartition;
        s[base + sp.party - 4] = 2;
        qasynth::gen4::copyBytes(s, base + sp.party + Encryption::SIZE_PARTY4, burmy);
        // Zukan4: caught bits at +4, seen bits at +0x44 (Gen4ReadOnlySave.cpp dexProgress()).
        const size_t caught = base + v.pokedexOffset + 4, seen = caught + 0x40;
        s[caught + (24 >> 3)] |= static_cast<uint8_t>(1u << (24 & 7));                 // #25 caught
        s[seen + (24 >> 3)] |= static_cast<uint8_t>(1u << (24 & 7));                   // #25 seen
        s[seen + ((kBurmy - 1) >> 3)] |= static_cast<uint8_t>(1u << ((kBurmy - 1) & 7)); // #412 seen
        qasynth::w16(s, base + bag.offset, kPotion);
        qasynth::w16(s, base + bag.offset + 2, kPotionCount);
    }
    qasynth::gen4::forge(s);
    return s;
}

void setMtime(const fs::path& path, int64_t seconds) {
    struct timespec times[2] = {{seconds, 0}, {seconds, 0}};
    ::utimensat(AT_FDCWD, path.c_str(), times, 0);
}

// The SaveSelectScreen Home wiring (tests/test_home_card_gen4_snapshot.cpp).
const S::HomeCardSnapshot& refreshHome(S::HomeSnapshotCache& cache, const Legacy::LegacySourceBindings& b,
                                       const char* gameId, S::SnapshotRefresh& outcome) {
    const auto assigned = b.resolveFileForGame(kProfile, gameId);
    return cache.refresh(S::assignedSnapshotRequest(kProfile, gameId, assigned), S::statRegularFile,
                         [&] { return G4::homeCardSnapshot(G4::openAssignedSource(b, kProfile, gameId)); },
                         &outcome);
}

bool hasLine(const std::vector<std::string>& lines, const std::string& text) {
    return std::find(lines.begin(), lines.end(), text) != lines.end();
}

// ------------------------------------------------------------------------------------------------
// Round 2 cells (bag-add, reroll), each on a fresh Gen4ReadOnlyTrainer bridge over the in-memory
// synthetic save, so they never disturb the cells above.
//   bag-add  Gen4BagQuantity::policyRule + BagAdd::decidePick / decideRow with the pocket state and
//            writer cap Gen4SharedPokemonSurface.inc handleItemQuantity builds, staged through the
//            bridge's Gen4StagedPokemonEditor::stageBagAdd (row edits: stageBagQuantity) and
//            refreshStagedPokemonPresentation, exactly as that handler does
//   reroll   the Y handler of SharedPokemonEditorSurface.inc with the Gen IV adapter
//            (Gen4SharedPokemonSurface.inc setIv / setEv on Session::working), then Session::keep
// ------------------------------------------------------------------------------------------------
void runRound2Gen4(Matrix& m, const VersionSpec& v, const Bytes& original) {
    namespace R2 = round2;
    const std::string ver = v.version;
    std::string parseError;
    const auto parsed = G4::Gen4ReadOnlySave::parse(original, v.layout, v.gameId, &parseError);

    cell(m, ver, "bag-add", "Gen4BagQuantity::policyRule + decidePick (pocket slots / held from the bridge): "
             "Antidote StageNew x1 (no numpad) -> stageBagAdd -> refreshed Medicine shows Antidote x1 (strict "
             "reparse); row EditQuantity [1,999]; held Potion -> FocusExisting, nothing staged; Bicycle StageNew x1 "
             "then FixedQuantity, x2 refused", [&](Probe& p) {
        p.expect(parsed.has_value(), "parse: " + parseError);
        if (!parsed) return;
        std::string error;
        auto trainer = Legacy::Gen4ReadOnlyTrainer::create(*parsed, v.gameId, error);
        p.expect(trainer && trainer->stagedPokemon(), "Gen4ReadOnlyTrainer staged workspace: " + error);
        if (!trainer || !trainer->stagedPokemon()) return;
        auto& editor = *trainer->stagedPokemon();
        PokeVault::Inventory::QuantityPolicy::Game game{};
        p.expect(PokeVault::Inventory::QuantityPolicy::gameFromId(v.gameId, game), "gameFromId");
        const auto item = [&](std::string name, G4::BagPocket pocket, uint16_t id) {
            R2::BagItem i;
            i.name = std::move(name);
            i.id = id;
            i.pocket = static_cast<size_t>(pocket);
            i.rule = PokeBank::UIModel::Gen4BagQuantity::policyRule(game, i.pocket, id);
            i.writerCap = G4::gen4BagMaxQuantity(i.pocket, id);
            return i;
        };
        const auto rowOf = [&](const R2::BagItem& i) -> std::optional<size_t> {
            const auto& rows = trainer->items[i.pocket];
            for (size_t r = 0; r < rows.size(); ++r)
                if (rows[r].itemId == i.id) return r;
            return std::nullopt;
        };
        R2::BagBackend b;
        b.held = [&](const R2::BagItem& i) -> uint16_t {
            const auto r = rowOf(i);
            return r ? trainer->items[i.pocket][*r].count : 0;
        };
        b.state = [&](const R2::BagItem& i) {
            R2::BagAdd::PocketState s{};
            s.usedSlots = trainer->items[i.pocket].size();
            s.capacity = G4::bagLayout(editor.layout())[i.pocket].slots;
            s.heldInPocket = b.held(i);
            return s;
        };
        b.stage = [&](const R2::BagItem& i, uint16_t q, std::string& err) {
            if (!G4::gen4BagItemAllowed(editor.layout(), i.pocket, i.id)) { err = "not in the native catalog"; return false; }
            auto backup = editor;
            const auto r = rowOf(i);
            const bool ok = r ? editor.stageBagQuantity(i.pocket, *r, q, &err) : editor.stageBagAdd(i.pocket, i.id, q, &err);
            if (!ok) return false;
            if (!trainer->refreshStagedPokemonPresentation(err)) {
                editor = std::move(backup);
                std::string ignored;
                (void)trainer->refreshStagedPokemonPresentation(ignored);
                return false;
            }
            return true;
        };
        b.bytes = [&] { return editor.stagedBytes(); };
        for (const auto& f : R2::bagAddChecks(b, item("Antidote", G4::BagPocket::Medicine, 18),
                                              item("Potion", G4::BagPocket::Medicine, kPotion),
                                              item("Bicycle", G4::BagPocket::KeyItems, 450)))
            p.expect(false, f);
        std::string err;
        const auto out = editor.finalizedBytes(&err);
        p.expect(!out.empty(), "finalize: " + err);
        if (!out.empty()) {
            const auto re = G4::Gen4ReadOnlySave::parse(out, v.layout, v.gameId, &err);
            const auto bag = re ? G4::decodeReadOnlyBag(*re) : std::nullopt;
            const auto has = [&](G4::BagPocket pocket, uint16_t id) {
                if (!bag) return false;
                for (const auto& s : (*bag)[static_cast<size_t>(pocket)])
                    if (s.itemId == id && s.count == 1) return true;
                return false;
            };
            p.expect(has(G4::BagPocket::Medicine, 18) && has(G4::BagPocket::KeyItems, 450),
                     "reparsed bag lacks Antidote x1 / Bicycle x1: " + err);
        }
        p.expect(editor.originalBytes() == original, "workspace original differs from the source");
    });

    cell(m, ver, "reroll", "Y = SharedStatReroll::reroll via Session::working setEV(0)/setIV/setEV -> keep, x" +
             std::to_string(R2::kRerolls) + " per case: Pikachu shiny/non-shiny x male/female keep shiny, gender "
             "and PID; IVs/EVs land as rolled", [&](Probe& p) {
        p.expect(parsed.has_value(), "parse: " + parseError);
        if (!parsed) return;
        std::string error;
        auto editor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &error);
        p.expect(editor.has_value(), "staged editor: " + error);
        if (!editor) return;
        namespace Reroll = PokeBank::UIModel::SharedStatReroll;
        Reroll::Rng rng(0x5EED4u);
        for (const bool shiny : {false, true})
            for (const uint8_t gender : {uint8_t{0}, uint8_t{1}}) {
                const std::string what = std::string(shiny ? "shiny" : "non-shiny") + (gender ? " female" : " male");
                auto rec = editor->boxedPokemon(0, 0, &error);
                p.expect(rec.has_value(), "box 1/1: " + error);
                if (!rec) return;
                Session4::Session setup;
                p.expect(setup.begin(*rec, parsed->rawFamily(), Session4::Mode::Edit, error), "begin: " + error);
                if (!setup.working) return;
                if (setup.working->shiny() != shiny) p.expect(setup.cycleShiny(), what + ": cycleShiny refused");
                if (setup.working->gender() != gender) p.expect(setup.cycleGender(), what + ": cycleGender refused");
                if (!setup.keep(*editor, 0, 0, error)) {
                    p.expect(false, what + ": Keep start state: " + error);
                    continue;
                }
                auto start = editor->editableBoxPokemon(0, 0, &error);
                p.expect(start && start->shiny() == shiny && start->gender() == gender, what + ": start state not staged");
                if (!start) return;
                for (int i = 0; i < R2::kRerolls; ++i) {
                    rec = editor->boxedPokemon(0, 0, &error);
                    if (!rec) { p.expect(false, "box 1/1: " + error); return; }
                    Session4::Session y;
                    if (!y.begin(*rec, parsed->rawFamily(), Session4::Mode::Edit, error)) {
                        p.expect(false, "begin: " + error);
                        return;
                    }
                    const auto next = Reroll::reroll(rng, {y.working->ivs(), y.working->evs()});
                    for (size_t s = 0; s < 6; ++s) y.working->setEV(s, 0);
                    for (size_t s = 0; s < 6; ++s) y.working->setIV(s, next.ivs[s]);
                    for (size_t s = 0; s < 6; ++s) y.working->setEV(s, next.evs[s]);
                    if (!y.keep(*editor, 0, 0, error)) {
                        p.expect(false, what + ": Keep reroll " + std::to_string(i) + ": " + error);
                        break;
                    }
                    const auto now = editor->editableBoxPokemon(0, 0, &error);
                    if (!now) { p.expect(false, "box 1/1: " + error); return; }
                    p.expect(now->ivs() == next.ivs && now->evs() == next.evs, what + ": rolled IVs/EVs not staged");
                    p.expect(now->shiny() == shiny && now->gender() == gender && now->pid() == start->pid(),
                             what + ": reroll " + std::to_string(i) + " changed shiny/gender/PID");
                }
            }
        p.expect(editor->originalBytes() == original, "workspace original differs from the source");
    });
}

// ------------------------------------------------------------------------------------------------
// One version
// ------------------------------------------------------------------------------------------------
void runVersion(Matrix& m, const VersionSpec& v) {
    const std::string ver = v.version;
    const fs::path root = qacontract::scratchRoot(std::string("matrix-gen4-") + v.version);
    const fs::path providerDir = root / "drastic" / "backup";
    const fs::path source = providerDir / (ver + ".sav");
    const fs::path bindingsPath = root / "legacy_bindings.tsv";
    const Bytes original = buildSave(v);
    qacontract::writeSource(source, original);
    // A non-save next to it: discovery must surface it as invalid, never as a Gen IV source.
    qacontract::spit(providerDir / "notes.sav", Bytes(100, 0x42));
    const auto sourceBefore = qacontract::fileState(source);
    std::vector<std::string> mutationLog;  // steps after which the source differed
    auto sourceUnchanged = [&](const char* step) {
        const bool same = qacontract::fileState(source) == sourceBefore;
        if (!same) mutationLog.push_back(step);
        return same;
    };

    // ---------------------------------------------------------------- discovery
    G4::SourceCandidate candidate;
    cell(m, ver, "discovery", "discoverSources(DraStic root) -> 1 Ready " + std::string(v.family) +
             " candidate + decoy Invalid; toSaveInstance Ready gen 4", [&](Probe& p) {
        const std::vector<G4::DiscoveryRoot> roots = {{providerDir.string(), kProvider, 1}};
        const auto result = G4::discoverSources(roots);
        p.expect(result.filesExamined == 2, "filesExamined " + std::to_string(result.filesExamined) + " != 2");
        size_t ready = 0;
        for (const auto& c : result.candidates) {
            if (c.ready()) {
                ++ready;
                candidate = c;
            } else {
                p.expect(c.status == G4::CandidateStatus::InvalidSave,
                         "decoy status " + std::string(G4::candidateStatusName(c.status)));
            }
        }
        p.expect(ready == 1, "ready candidates " + std::to_string(ready) + " != 1");
        p.expect(candidate.layout == v.layout, "layout differs");
        p.expect(candidate.expectedRawFamily == v.family, "family " + candidate.expectedRawFamily);
        p.expect(candidate.exactGameFromSave == v.fromSave, "exactGameFromSave " +
                 std::to_string(static_cast<int>(candidate.exactGameFromSave)));
        p.expect(candidate.trainerName == "Kylie" && candidate.partyCount == 2, "trainer/party summary");
        p.expect(!candidate.recoveredOlderCopy, "recoveredOlderCopy set");
        p.expect(!candidate.contentFingerprint.empty(), "no content fingerprint");
        const auto direct = G4::inspectSourceFile(source.string(), kProvider, v.gameId);
        p.expect(direct.ready() && direct.sourceIdentity == candidate.sourceIdentity,
                 "inspectSourceFile(assigned) not Ready / identity differs");
        const auto row = G4::toSaveInstance(candidate, v.gameId);
        p.expect(row.validation == S::ValidationStatus::Ready, "SaveInstance not Ready");
        p.expect(row.generation == 4 && row.gameId == v.gameId && row.platformLabel == "Nintendo DS",
                 "SaveInstance generation/gameId/platform");
        p.expect(row.access == S::AccessMode::ReadOnly, "SaveInstance not read-only");
        p.expect(sourceUnchanged("discovery"), "discovery changed the source");
    });

    // ---------------------------------------------------------------- linking
    cell(m, ver, "linking", "assignFileAndSave -> reload in a new LegacySourceBindings -> "
             "resolveFileForGame Ready (same path/identity) -> openAssignedSource Ready", [&](Probe& p) {
        {
            Legacy::LegacySourceBindings b(bindingsPath.string());
            p.expect(b.load(), "initial load failed");
            p.expect(b.assignFileAndSave(candidate.sourceIdentity,
                                         {kProfile, v.gameId, source.string(), kProvider, v.family}),
                     "assignFileAndSave failed: " + b.lastError());
        }
        Legacy::LegacySourceBindings b(bindingsPath.string());
        p.expect(b.load(), "reload failed");
        const auto resolved = b.resolveFileForGame(kProfile, v.gameId);
        p.expect(resolved.status == Legacy::AssignedFileStatus::Ready, "resolve not Ready");
        p.expect(resolved.binding.sourcePath == source.string(), "resolved path " + resolved.binding.sourcePath);
        p.expect(resolved.sourceIdentity == candidate.sourceIdentity, "resolved identity differs");
        p.expect(b.assignedGame(candidate.sourceIdentity) == v.gameId, "assignedGame differs");
        p.expect(b.isVisibleTo(candidate.sourceIdentity, kProfile), "not visible to its profile");
        p.expect(!b.isVisibleTo(candidate.sourceIdentity, "other-profile"), "visible to another profile");
        const auto opened = G4::openAssignedSource(b, kProfile, v.gameId);
        p.expect(opened.status == G4::OpenStatus::Ready && opened.save, "openAssignedSource not Ready: " +
                 opened.diagnostic);
        p.expect(G4::openAssignedSource(b, kProfile, v.wrongGame).status == G4::OpenStatus::Unassigned,
                 "another game resolved to this file");
        p.expect(sourceUnchanged("linking"), "linking changed the source");
    });

    Legacy::LegacySourceBindings bindings(bindingsPath.string());
    (void)bindings.load();
    const auto opened = G4::openAssignedSource(bindings, kProfile, v.gameId);

    // ---------------------------------------------------------------- provider
    cell(m, ver, "provider", "binding sourceType DraStic -> \"Nintendo DS \xE2\x80\xA2 DraStic\"; "
             "Save Instance row \"DraStic / ...\"", [&](Probe& p) {
        const auto resolved = bindings.resolveFileForGame(kProfile, v.gameId);
        p.expect(S::identifyProvider(resolved.binding.sourceType) == S::ProviderIdentity::DraStic,
                 "provider identity of '" + resolved.binding.sourceType + "'");
        const std::string line = S::homeSourceLine("Nintendo DS",
                                                   S::homeCardSourceText("REMEMBERED", resolved.binding.sourceType));
        p.expect(line == "Nintendo DS \xE2\x80\xA2 DraStic", "home line '" + line + "'");
        const auto row = G4::toSaveInstance(candidate, v.gameId);
        p.expect(row.providerLabel == kProvider && row.providerId == "drastic", "row provider " + row.providerLabel);
        p.expect(S::saveInstanceSourceLine(row).rfind("DraStic / " + std::string(v.family), 0) == 0,
                 "row line '" + S::saveInstanceSourceLine(row) + "'");
        p.expect(row.kind == S::SaveInstanceKind::BatterySave, "row kind not battery save");
    });

    // ---------------------------------------------------------------- identity
    const bool assignmentOnly = v.fromSave == GV::Invalid;
    m.fact(ver, "identity.source", assignmentOnly ? "assignment" : "save-internal");
    cell(m, ver, "identity",
         assignmentOnly
             ? std::string("raw DP save cannot prove D vs P (exactGameFromSave Invalid); exact game = assignment: "
                           "same bytes Match as ") + v.gameId + " and " + v.sibling + ", Create origin follows the "
                           "assignment; " + v.wrongGame + " assignment -> AssignmentMismatch"
             : std::string("exact game from the save (") +
                   (v.layout == G4::Layout::Platinum ? "Platinum layout" : "HGSS trainer ROMCode 0x1C") +
                   "); assignment " + v.wrongGame + " -> AssignmentMismatch on inspect/open/SaveInstance",
         [&](Probe& p) {
        std::string err;
        const auto parsed = G4::Gen4ReadOnlySave::parse(original, v.layout, v.gameId, &err);
        p.expect(parsed.has_value(), "parse failed: " + err);
        if (!parsed) return;
        p.expect(parsed->exactGameFromSave() == v.fromSave, "exactGameFromSave differs");
        p.expect(parsed->assignmentStatus() == G4::AssignmentStatus::Match, "own assignment not Match");
        p.expect(parsed->assignedExactGame() == v.exact, "assignedExactGame differs");
        GV assigned = GV::Invalid;
        p.expect(G4::Gen4ReadOnlySave::validateAssignment(v.layout, v.fromSave, v.wrongGame, &assigned) ==
                     G4::AssignmentStatus::Mismatch, std::string("validateAssignment accepted ") + v.wrongGame);
        // Assignment-mismatch path through the real entry points.
        const auto wrongCandidate = G4::inspectSourceFile(source.string(), kProvider, v.wrongGame);
        p.expect(wrongCandidate.status == G4::CandidateStatus::AssignmentMismatch,
                 std::string("inspect with wrong game: ") + G4::candidateStatusName(wrongCandidate.status));
        p.expect(G4::toSaveInstance(candidate, v.wrongGame).validation == S::ValidationStatus::AssignmentMismatch,
                 "SaveInstance for wrong game not AssignmentMismatch");
        {
            Legacy::LegacySourceBindings wrong((root / "wrong_bindings.tsv").string());
            (void)wrong.load();
            p.expect(wrong.assignFileAndSave(candidate.sourceIdentity,
                                             {kProfile, v.wrongGame, source.string(), kProvider, v.family}),
                     "wrong assignment could not be stored");
            const auto bad = G4::openAssignedSource(wrong, kProfile, v.wrongGame);
            p.expect(bad.status == G4::OpenStatus::AssignmentMismatch,
                     "openAssignedSource(wrong game) status " + std::to_string(static_cast<int>(bad.status)));
        }
        // The exact game reaches the staged Create transaction as the PK4 origin version.
        auto editor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &err);
        p.expect(editor.has_value(), "staged editor: " + err);
        if (editor) {
            auto draft = editor->createBoxDraft(0, 1, 1, &err);
            p.expect(draft && editor->stageCreateBoxPokemon(0, 1, *draft, &err), "create: " + err);
            const auto created = editor->boxedPokemon(0, 1, &err);
            p.expect(created && created->originVersion() == static_cast<uint8_t>(v.exact),
                     "created origin " + std::to_string(created ? created->originVersion() : 0));
        }
        if (assignmentOnly) {
            const auto sib = G4::Gen4ReadOnlySave::parse(original, v.layout, v.sibling, &err);
            p.expect(sib && sib->assignmentStatus() == G4::AssignmentStatus::Match &&
                         sib->assignedExactGame() == v.siblingExact,
                     std::string("same bytes not accepted as ") + v.sibling);
            auto sibEditor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.sibling, &err);
            auto sibDraft = sibEditor ? sibEditor->createBoxDraft(0, 1, 1, &err) : std::nullopt;
            p.expect(sibDraft && sibEditor->stageCreateBoxPokemon(0, 1, *sibDraft, &err), "sibling create: " + err);
            const auto sibCreated = sibEditor ? sibEditor->boxedPokemon(0, 1, &err) : std::nullopt;
            p.expect(sibCreated && sibCreated->originVersion() == static_cast<uint8_t>(v.siblingExact),
                     "sibling Create origin does not follow the assignment");
        } else {
            // A save-internal identity ignores the filename: copy under another game's name.
            const fs::path renamed = root / (std::string(v.wrongGame) + ".sav");
            qacontract::spit(renamed, original);
            const auto other = G4::inspectSourceFile(renamed.string(), kProvider);
            p.expect(other.ready() && other.exactGameFromSave == v.fromSave,
                     "identity changed with the filename");
        }
    });

    // ---------------------------------------------------------------- read model (Gen4ReadOnlyTrainer)
    std::string trainerError;
    std::unique_ptr<Legacy::Gen4ReadOnlyTrainer> trainer;
    if (opened.save) trainer = Legacy::Gen4ReadOnlyTrainer::create(*opened.save, v.gameId, trainerError);
    const auto needTrainer = [&](Probe& p) {
        p.expect(opened.status == G4::OpenStatus::Ready && opened.save, "assigned source not Ready");
        p.expect(trainer != nullptr, "Gen4ReadOnlyTrainer::create: " + trainerError);
        return trainer != nullptr && opened.save.has_value();
    };

    cell(m, ver, "trainer-name", "\"Kylie\" via Gen4ReadOnlyTrainer, TID/SID 12150/22558", [&](Probe& p) {
        if (!needTrainer(p)) return;
        p.expect(opened.save->trainer().name == u"Kylie", "parsed name");
        p.expect(trainer->trainerName == "Kylie", "trainer view name '" + trainer->trainerName + "'");
        p.expect(trainer->TID16 == 12150 && trainer->SID16 == 22558, "TID/SID");
    });

    m.fact(ver, "trainer.gender", "stored");
    cell(m, ver, "trainer-gender", "stored (trainer +0x18 bit 0): female decoded", [&](Probe& p) {
        if (!needTrainer(p)) return;
        p.expect(opened.save->trainer().gender == 1, "parsed gender");
        p.expect(trainer->trainerGender == 1, "trainer view gender");
    });

    cell(m, ver, "party", "2 members: Pikachu Lv20, Burmy Sandy Cloak (form 1) Lv9", [&](Probe& p) {
        if (!needTrainer(p)) return;
        const auto party = opened.save->party();
        p.expect(party.size() == 2 && opened.save->diagnostics().invalidPartyRecords == 0, "party size/validity");
        if (party.size() == 2) {
            p.expect(party[0].species() == 25 && party[0].partyLevel() == 20, "slot 1");
            p.expect(party[1].species() == kBurmy && party[1].form() == kBurmyForm &&
                         party[1].partyLevel() == kBurmyLevel, "slot 2");
        }
        p.expect(trainer->getPartySize() == 2 && trainer->party.size() == 2 && trainer->party[0] &&
                     trainer->party[1], "trainer view party");
        if (trainer->party.size() == 2 && trainer->party[1])
            p.expect(trainer->party[1]->speciesID() == kBurmy, "trainer view slot 2 species");
    });

    cell(m, ver, "boxes", "18x30 native boxes; box 1 slot 1 Pikachu, rest empty, no invalid records", [&](Probe& p) {
        if (!needTrainer(p)) return;
        const auto& d = opened.save->diagnostics();
        p.expect(d.occupiedBoxRecords == 1 && d.invalidBoxRecords == 0, "box diagnostics");
        p.expect(opened.save->box(0, 0).species() == 25, "box 1 slot 1");
        p.expect(opened.save->box(0, 1).empty() && opened.save->box(17, 29).empty(), "empty slots");
        p.expect(!opened.save->box(18, 0).valid(), "out-of-range box is not a record");
        p.expect(trainer->getBoxCount() == 18 && trainer->getSlotsPerBox() == 30, "geometry");
        p.expect(trainer->boxes.size() == 18 && trainer->boxes[0][0] && !trainer->boxes[0][1], "trainer view boxes");
        p.expect(trainer->boxNames.size() == 18, "box names");
    });

    cell(m, ver, "pokedex", "seen 2 / caught 1 / total 493", [&](Probe& p) {
        if (!needTrainer(p)) return;
        const auto dex = opened.save->dexProgress();
        p.expect(dex.seen == 2 && dex.caught == 1 && dex.total == 493,
                 "dex " + std::to_string(dex.seen) + "/" + std::to_string(dex.caught) + "/" + std::to_string(dex.total));
    });

    m.fact(ver, "bag.read", "yes");
    cell(m, ver, "bag", "8 native pockets decoded; Medicine = Potion x5 (decodeReadOnlyBag + trainer view)",
         [&](Probe& p) {
        if (!needTrainer(p)) return;
        const auto bag = G4::decodeReadOnlyBag(*opened.save);
        p.expect(bag.has_value(), "bag did not decode");
        if (bag) {
            const auto& medicine = (*bag)[static_cast<size_t>(G4::BagPocket::Medicine)];
            p.expect(medicine.size() == 1 && medicine[0].itemId == kPotion && medicine[0].count == kPotionCount,
                     "medicine pocket");
            size_t total = 0;
            for (const auto& pocket : *bag) total += pocket.size();
            p.expect(total == 1, "unexpected items in other pockets");
        }
        p.expect(trainer->items.size() == G4::BagPocketCount, "trainer view pockets");
        if (trainer->items.size() == G4::BagPocketCount) {
            const auto& medicine = trainer->items[static_cast<size_t>(G4::BagPocket::Medicine)];
            p.expect(medicine.size() == 1 && medicine[0].itemId == kPotion, "trainer view medicine");
        }
    });

    // ---------------------------------------------------------------- navigation
    m.fact(ver, "nav.mutates", "no");
    cell(m, ver, "navigation", "SharedPokemonWorkspace::navigate (Gen4, View + Edit) and Gen4BoxMoveNavigation "
             "move focus only; Edit draft and staged save byte-identical", [&](Probe& p) {
        std::string err;
        auto editor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &err);
        p.expect(editor.has_value(), "staged editor: " + err);
        if (!editor || !opened.save) return;
        Session4::Session session;
        p.expect(session.begin(opened.save->box(0, 0), opened.save->rawFamily(), Session4::Mode::Edit, err),
                 "session begin: " + err);
        if (!session.working) return;
        const auto before = session.working->encryptedBytes();
        // Rows 6 (SID) and 10 (Origin) are the read-only Details rows of the Gen IV surface
        // (detailLabels in src/UI/Gen4SharedPokemonSurface.inc); PID row 10 is not a control.
        const auto detailEditable = [](int row) { return row != 6 && row != 10; };
        const auto pidEditable = [](int row) { return row != WS::kPidRow; };
        constexpr int kRows = 16;
        Shared::Focus focus{Shared::Panel::Details, 5, 0};
        WS::Keys down;
        down.down = true;
        auto r = WS::navigate(Shared::Generation::Gen4, focus, down, false, true, kRows, detailEditable, pidEditable);
        p.expect(r.focus.panel == Shared::Panel::Details && r.focus.row != 6,
                 "Down from Details row 5 landed on read-only row " + std::to_string(r.focus.row));
        std::vector<Shared::Focus> visited;
        const WS::Keys sequence[] = {{false, true, false, false, false, false}, {false, false, false, true, false, false},
                                     {false, false, false, false, false, true}, {false, true, false, false, false, false},
                                     {false, false, true, false, false, false}, {true, false, false, false, false, false},
                                     {false, false, false, false, true, false}};
        for (bool passive : {false, true})
            for (const auto& keys : sequence) {
                focus = WS::navigate(Shared::Generation::Gen4, focus, keys, passive, !passive, kRows,
                                     detailEditable, pidEditable).focus;
                visited.push_back(focus);
            }
        bool moved = false;
        for (const auto& f : visited) moved = moved || f.panel != Shared::Panel::Details;
        p.expect(moved, "focus never left DETAILS");
        p.expect(session.working->encryptedBytes() == before && !session.dirty(), "navigation changed the draft");
        p.expect(!editor->hasChanges(), "navigation changed the staged save");
        BoxNav::Cursor c{0, 5};
        c = BoxNav::navigate(c, BoxNav::Direction::Right);
        p.expect(c.box == 0 && c.slot == 0, "Right wraps within the row");
        c = BoxNav::navigate({17, 29}, BoxNav::Direction::NextBox);
        p.expect(c.box == 0 && c.slot == 29, "NextBox wraps 18 -> 1");
        c = BoxNav::navigate({0, 0}, BoxNav::Direction::Up);
        p.expect(c.slot == 24, "Up wraps to the bottom row");
        p.expect(!editor->hasChanges(), "box navigation changed the staged save");
    });

    // ---------------------------------------------------------------- creation
    m.fact(ver, "create.default.species", "25");
    m.fact(ver, "create.default.level", "5");
    cell(m, ver, "creation", "createBoxDraft(species 25 as Gen4SharedPokemonSurface.inc beginCreate) -> Lv5, "
             "Poke Ball, trainer OT/TID; Session Create -> keepCreate stages; occupied/out-of-range/invalid refused",
         [&](Probe& p) {
        std::string err;
        auto editor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &err);
        p.expect(editor.has_value(), "staged editor: " + err);
        if (!editor) return;
        auto draft = editor->createBoxDraft(0, 1, 25, &err);
        p.expect(draft.has_value(), "draft: " + err);
        if (!draft) return;
        p.expect(draft->species() == 25 && draft->level() == 5 && draft->ball() == 4 && draft->metLevel() == 5,
                 "draft defaults species/level/ball/metLevel " + std::to_string(draft->species()) + "/" +
                     std::to_string(draft->level()));
        p.expect(draft->tid() == 12150 && draft->sid() == 22558, "draft TID/SID");
        p.expect(!draft->isParty(), "draft is a party record");
        Session4::Session session;
        p.expect(session.beginCreate(std::move(*draft), err), "beginCreate: " + err);
        p.expect(session.creating() && session.dirty(), "create session not dirty");
        p.expect(!session.back() && session.confirmExit, "Back on a Create draft did not ask to confirm");
        session.continueEditing();
        p.expect(session.keepCreate(*editor, 0, 1, err), "keepCreate: " + err);
        const auto created = editor->boxedPokemon(0, 1, &err);
        p.expect(created && created->species() == 25 && created->originalTrainerName() == u"Kylie",
                 "created record species/OT");
        const auto reopened = editor->editableBoxPokemon(0, 1, &err);
        p.expect(reopened && reopened->level() == 5, "created level");
        // Validation.
        p.expect(!editor->createBoxDraft(0, 0, 1, &err), "draft accepted for an occupied slot");
        p.expect(!editor->createBoxDraft(18, 0, 1, &err), "draft accepted outside native storage");
        p.expect(!editor->createBoxDraft(0, 2, 0, &err), "draft accepted species 0");
        p.expect(!editor->createBoxDraft(0, 2, 494, &err), "draft accepted species 494 (beyond Gen IV)");
        auto again = editor->createBoxDraft(0, 2, 1, &err);
        p.expect(again && !editor->stageCreateBoxPokemon(0, 0, *again, &err), "Create overwrote an occupied slot");
    });

    // ---------------------------------------------------------------- fields
    // One editor carries every field edit; it is reused by working-copy, review, export and Home.
    std::string fieldsError;
    auto fieldsEditor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &fieldsError);
    cell(m, ver, "fields", "box: species/nickname/level/OT/TID/held item/friendship/language/met level/"
             "met location/ball/Pokerus/IV/EV/move/PP/PP Ups/nature/gender/shiny/ability; party: level/shiny/form (Burmy); "
             "bag quantity: all stage and round-trip through strict reparse", [&](Probe& p) {
        p.expect(fieldsEditor.has_value(), "staged editor: " + fieldsError);
        if (!fieldsEditor) return;
        std::string err;
        auto mon = fieldsEditor->editableBoxPokemon(0, 0, &err);
        p.expect(mon.has_value(), "editable box: " + err);
        if (!mon) return;
        // Eevee (133): variable gender and two Gen IV abilities, so every PID-linked row is editable.
        p.expect(mon->setSpecies(133), "setSpecies");
        p.expect(mon->setNickname(u"QAEEVEE"), "setNickname");
        p.expect(mon->setLevel(30), "setLevel");
        p.expect(mon->setOriginalTrainerName(u"QAOT"), "setOriginalTrainerName");
        p.expect(mon->setTID(4242), "setTID");
        p.expect(mon->setHeldItem(234), "setHeldItem(234 Leftovers)");
        p.expect(mon->setFriendship(123), "setFriendship");
        p.expect(mon->setLanguage(3), "setLanguage(3)");
        p.expect(mon->setMetLevel(7), "setMetLevel");
        // Ball / Met Location are origin metadata: refused on this non-Gen-IV-origin record
        // (origin byte 0x5F = 0) and edited below on a native Create draft instead.
        p.expect(!mon->setBall(1) && !mon->setMetLocation(16), "origin metadata edited on a non-native record");
        p.expect(mon->setPokerusMode(Pokemon::PokerusMode::Cured), "setPokerusMode(Cured)");
        p.expect(mon->setIV(0, 31) && mon->setIV(5, 7), "setIV");
        p.expect(mon->setEV(1, 200) && mon->setEV(3, 52), "setEV");
        p.expect(mon->setMove(1, 98), "setMove(1, Quick Attack)");
        p.expect(mon->setPPUps(1, 3), "setPPUps");
        p.expect(mon->setPP(1, 20), "setPP");
        const uint8_t nature = static_cast<uint8_t>((mon->nature() + 7) % 25);
        p.expect(mon->setNature(nature), "setNature");
        const uint8_t gender = mon->gender() == 0 ? 1 : 0;
        p.expect(mon->setGender(gender), "setGender");
        p.expect(mon->setShiny(true), "setShiny");
        const uint8_t ability = mon->abilitySlot() == 0 ? 1 : 0;
        p.expect(mon->setAbilitySlot(ability), "setAbilitySlot");
        const auto staged = *mon;
        p.expect(fieldsEditor->commitBoxPokemon(0, 0, *mon, &err), "commitBoxPokemon: " + err);
        const auto back = fieldsEditor->editableBoxPokemon(0, 0, &err);
        p.expect(back.has_value(), "reparse box: " + err);
        if (back) {
            p.expect(back->species() == 133 && back->nickname() == u"QAEEVEE", "species/nickname round-trip");
            p.expect(back->level() == 30 && back->experience() == staged.experience(), "level/experience round-trip");
            p.expect(back->tid() == 4242 && back->heldItem() == 234 && back->friendship() == 123,
                     "TID/held/friendship round-trip");
            p.expect(back->language() == 3 && back->metLevel() == 7, "language/met level round-trip");
            p.expect(back->pokerus() == staged.pokerus() && back->pokerus() != 0, "Pokerus round-trip");
            p.expect(back->ivs()[0] == 31 && back->ivs()[5] == 7 && back->evs()[1] == 200 && back->evs()[3] == 52,
                     "IV/EV round-trip");
            p.expect(back->moves()[1] == 98 && back->ppUps()[1] == 3 && back->pp()[1] == staged.pp()[1],
                     "move/PP round-trip");
            p.expect(back->nature() == nature && back->gender() == gender && back->shiny() &&
                         back->abilitySlot() == ability, "nature/gender/shiny/ability round-trip");
            p.expect(back->encryptedBytes() == staged.encryptedBytes(), "record bytes differ after Keep");
        }
        const auto readBack = fieldsEditor->boxedPokemon(0, 0, &err);
        p.expect(readBack && readBack->originalTrainerName() == u"QAOT", "OT name round-trip");
        // Native-origin record (Create draft carries the exact game as origin): Ball + Met Location.
        // Location ids are valid per Names::isGen4NativeMetLocation (DP <=111, Pt <=125, HGSS 126..233).
        const uint16_t location = v.layout == G4::Layout::HeartGoldSoulSilver ? 126 : 16;
        auto native = fieldsEditor->createBoxDraft(0, 3, 1, &err);
        p.expect(native && native->setBall(1) && native->setMetLocation(location), "native ball/met location: " + err);
        if (native) p.expect(fieldsEditor->stageCreateBoxPokemon(0, 3, *native, &err), "stage native: " + err);
        const auto nativeBack = fieldsEditor->editableBoxPokemon(0, 3, &err);
        const auto nativeRead = fieldsEditor->boxedPokemon(0, 3, &err);
        p.expect(nativeBack && nativeBack->ball() == 1, "ball round-trip");
        p.expect(nativeRead && (nativeRead->metLocationDP() == location || nativeRead->metLocationExtended() == location),
                 "met location round-trip");
        // Party: level, shiny, Unown form.
        auto lead = fieldsEditor->editablePartyPokemon(0, &err);
        p.expect(lead && lead->setLevel(33) && lead->setShiny(true), "party lead edit: " + err);
        if (lead) p.expect(fieldsEditor->commitPartyPokemon(0, *lead, &err), "commitPartyPokemon(0): " + err);
        auto burmy = fieldsEditor->editablePartyPokemon(1, &err);
        p.expect(burmy && burmy->setForm(2), "Burmy setForm(2): " + err);
        if (burmy) p.expect(fieldsEditor->commitPartyPokemon(1, *burmy, &err), "commitPartyPokemon(1): " + err);
        const auto p0 = fieldsEditor->partyPokemon(0, &err);
        const auto p1 = fieldsEditor->partyPokemon(1, &err);
        p.expect(p0 && p0->partyLevel() == 33, "party level round-trip");
        p.expect(p1 && p1->form() == 2, "party form round-trip");
        // Bag quantity (Medicine pocket, first populated row).
        p.expect(fieldsEditor->stageBagQuantity(static_cast<size_t>(G4::BagPocket::Medicine), 0, 9, &err),
                 "stageBagQuantity: " + err);
        const auto finalized = fieldsEditor->finalizedBytes(&err);
        const auto reparsed = G4::Gen4ReadOnlySave::parse(finalized, v.layout, v.gameId, &err);
        p.expect(reparsed && reparsed->assignmentStatus() == G4::AssignmentStatus::Match && !reparsed->recovered(),
                 "finalized bytes failed strict reparse: " + err);
        if (reparsed) {
            const auto bag = G4::decodeReadOnlyBag(*reparsed);
            p.expect(bag && (*bag)[static_cast<size_t>(G4::BagPocket::Medicine)].size() == 1 &&
                         (*bag)[static_cast<size_t>(G4::BagPocket::Medicine)][0].count == 9,
                     "bag quantity round-trip");
        }
        p.expect(sourceUnchanged("fields"), "field staging changed the source");
    });

    // ---------------------------------------------------------------- pickers
    cell(m, ver, "pickers", "species names 1..493; Gen4MoveCompatibility::selectableMoves (the move picker "
             "source) per exact game; held-item catalog per family; descriptor move evaluator", [&](Probe& p) {
        for (uint16_t species = 1; species <= 493; ++species) {
            const char* name = Names::getSpeciesName(species);
            if (!name || !*name) {
                p.expect(false, "species " + std::to_string(species) + " has no name");
                break;
            }
        }
        const std::array<uint16_t, 4> existing{84, 0, 0, 0};
        const auto moves = Compat4::selectableMoves(v.gameId, 25, 0, existing);
        p.expect(moves.size() > 20 && moves.front() == 0 && std::is_sorted(moves.begin() + 1, moves.end()),
                 "selectableMoves shape (" + std::to_string(moves.size()) + ")");
        p.expect(std::find(moves.begin(), moves.end(), 85) != moves.end(), "Thunderbolt missing for Pikachu");
        p.expect(std::find(moves.begin(), moves.end(), 468) == moves.end(), "post-Gen IV move offered");
        p.expect(Compat4::classify(v.gameId, 25, 0, 85, false) == Compat4::Availability::Direct, "Thunderbolt not Direct");
        const auto descriptor = Provider4::descriptorForSource(v.gameId, true);
        p.expect(descriptor.has_value(), "no exact-format descriptor");
        if (descriptor) {
            using Exact = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult;
            p.expect(descriptor->moves.evaluate({v.gameId, 25, 0, 85, false}) == Exact::Compatible,
                     "descriptor evaluator Thunderbolt");
            p.expect(descriptor->moves.evaluate({v.gameId, 25, 0, 600, false}) == Exact::Invalid,
                     "descriptor evaluator accepted move 600");
        }
        const auto family = opened.save ? opened.save->rawFamily() : GV::Invalid;
        p.expect(Names::isGen4HeldItemPresent(234, family), "Leftovers not holdable");
        p.expect(Names::isGen4HeldItemPresent(112, family) == (v.layout != G4::Layout::DiamondPearl),
                 "Griseous Orb availability does not follow the family");
        p.expect(!Names::isGen4HeldItemPresent(5, family), "item 5 (Safari Ball) offered as held item");
    });

    // ---------------------------------------------------------------- actions
    cell(m, ver, "actions", "Gen4BoxTouchActions select/open + staged Clone/Move/Release primitives; descriptor "
             "grants BoxPokemon/Editing/Creation, never a source write (menu rows live in the non-host "
             "Gen4SharedPokemonSurface.inc); a net no-op clone/move/release leaves no "
             "review line (QA-F8a)", [&](Probe& p) {
        p.expect(BoxTouch::selectsTappedBoxSlot(0, 30) && BoxTouch::selectsTappedBoxSlot(29, 30) &&
                     !BoxTouch::selectsTappedBoxSlot(30, 30), "tap selection");
        p.expect(BoxTouch::opensSelectedSlotActions(4, 4, 30) &&
                     BoxTouch::opensSelectedSlotActions(BoxTouch::SummaryPanelTouchId, 4, 30) &&
                     !BoxTouch::opensSelectedSlotActions(5, 4, 30), "Actions open rule");
        const auto descriptor = Provider4::descriptorForSource(v.gameId, true);
        p.expect(descriptor.has_value(), "no descriptor");
        if (descriptor) {
            using Cap = PokeVault::SaveEdit::Capability;
            const auto& caps = descriptor->source.saveOperations;
            p.expect(caps.supports(Cap::BoxPokemon) && caps.supports(Cap::PokemonEditing) &&
                         caps.supports(Cap::PokemonCreation), "staged capabilities");
            p.expect(!descriptor->source.canWriteOriginalSource(), "descriptor allows a source write");
        }
        std::string err;
        auto editor = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &err);
        p.expect(editor.has_value(), "staged editor: " + err);
        if (!editor) return;
        p.expect(editor->stageCloneBoxPokemon(0, 0, 0, 2, &err), "clone: " + err);
        p.expect(!editor->stageCloneBoxPokemon(0, 0, 0, 2, &err), "clone overwrote an occupied slot");
        p.expect(editor->stageMoveBoxPokemon(0, 2, 1, 0, &err), "move: " + err);
        const auto afterMove = G4::Gen4ReadOnlySave::parse(editor->stagedBytes(), v.layout, v.gameId, &err);
        p.expect(afterMove && afterMove->box(1, 0).species() == 25 && afterMove->box(0, 2).valid() &&
                     afterMove->box(0, 2).empty(), "move result");
        p.expect(editor->stageReleaseBoxPokemon(1, 0, &err), "release: " + err);
        const auto afterRelease = G4::Gen4ReadOnlySave::parse(editor->stagedBytes(), v.layout, v.gameId, &err);
        p.expect(afterRelease && afterRelease->box(1, 0).valid() && afterRelease->box(1, 0).empty() &&
                     afterRelease->box(0, 0).species() == 25, "release result");
        p.expect(!editor->boxedPokemon(1, 0, &err), "released slot still opens as a Pokemon");
        p.expect(!editor->stageReleaseBoxPokemon(1, 0, &err), "release of an empty slot accepted");
        // A net no-op: no review line (QA-F8a, c7b0f97) and the slot keeps the source's own empty record
        // (QA-F8b, f883f4d0; regression in test_qa_regressions.cpp).
        const auto lines = G4::describeStagedSlotChanges(*editor);
        p.expect(lines.empty(),
                 "unexpected review after clone+move+release: " + (lines.empty() ? std::string() : lines[0]));
        p.expect(sourceUnchanged("actions"), "actions changed the source");
    });

    // ---------------------------------------------------------------- working copy
    cell(m, ver, "working-copy", "originalBytes == source file; staged/finalized bytes carry every edit; "
             "source file byte/mtime identical", [&](Probe& p) {
        if (!fieldsEditor) {
            p.expect(false, "no staged editor");
            return;
        }
        p.expect(fieldsEditor->originalBytes() == qacontract::slurp(source), "original image != source file");
        p.expect(fieldsEditor->hasChanges() && fieldsEditor->stagedBytes() != fieldsEditor->originalBytes(),
                 "staged image carries no edit");
        std::string err;
        const auto finalized = fieldsEditor->finalizedBytes(&err);
        const auto parsed = G4::Gen4ReadOnlySave::parse(finalized, v.layout, v.gameId, &err);
        p.expect(parsed && parsed->box(0, 0).species() == 133 && parsed->party()[0].partyLevel() == 33,
                 "working copy does not carry the edit");
        p.expect(sourceUnchanged("working-copy"), "source changed");
    });

    // ---------------------------------------------------------------- review
    cell(m, ver, "review", "describeStagedSlotChanges lists party slots 1-2 (edited) and Box 1 slot 1 "
             "Pikachu -> Eevee, Box 1 slot 4 (empty) -> Bulbasaur, Bag Medicines Potion 5 -> 9", [&](Probe& p) {
        if (!fieldsEditor) {
            p.expect(false, "no staged editor");
            return;
        }
        const auto lines = G4::describeStagedSlotChanges(*fieldsEditor);
        p.expect(hasLine(lines, "Box 1 slot 1: Pikachu #25 -> Eevee #133"), "box line missing");
        p.expect(hasLine(lines, "Party slot 1: Pikachu #25 (edited)"), "party slot 1 line missing");
        p.expect(hasLine(lines, "Party slot 2: Burmy #412 (edited)"), "party slot 2 line missing");
        p.expect(hasLine(lines, "Box 1 slot 4: (empty) -> Bulbasaur #1"), "created slot line missing");
        p.expect(hasLine(lines, "Bag Medicines: item #17 5 -> 9"), "bag quantity line missing");
        p.expect(lines.size() == 5, "unexpected line count " + std::to_string(lines.size()));
    });

    // ---------------------------------------------------------------- export
    m.fact(ver, "export.output", "new-file-only");
    m.fact(ver, "export.reparse", "strict");
    std::string exportedDir;
    cell(m, ver, "export", "publishVerifiedGen4Export: new directory with original_backup.sav (== source) + "
             "edited.sav; strict reparse Match; changes only inside the selected General + Storage blocks; "
             "unchanged editor refused", [&](Probe& p) {
        if (!fieldsEditor) {
            p.expect(false, "no staged editor");
            return;
        }
        const fs::path exportRoot = root / "exports";
        G4::Gen4ExportRequest request;
        request.rootDirectory = exportRoot.string();
        request.directoryStem = std::string(v.gameId) + "_20261015_120000";
        request.sourcePath = source.string();
        request.timestamp = "20261015_120000";
        const auto result = G4::publishVerifiedGen4Export(*fieldsEditor, request);
        p.expect(result.success && !result.directory.empty(), "export failed: " + result.error);
        if (!result.success) return;
        exportedDir = result.directory;
        const fs::path dir = result.directory;
        p.expect(dir.parent_path() == exportRoot, "export outside the app-owned root");
        const auto backup = qacontract::slurp(dir / "original_backup.sav");
        const auto edited = qacontract::slurp(dir / "edited.sav");
        p.expect(backup == original, "backup != original source bytes");
        std::string err;
        const auto again = G4::Gen4ReadOnlySave::parse(edited, v.layout, v.gameId, &err);
        p.expect(again && again->assignmentStatus() == G4::AssignmentStatus::Match && !again->recovered(),
                 "edited.sav strict reparse: " + err);
        const auto parsed = G4::Gen4ReadOnlySave::parse(original, v.layout, v.gameId, &err);
        if (parsed) {
            const auto sp = qasynth::gen4::spec(v.kind);
            const size_t g = parsed->generalSelection().offset, st = parsed->storageSelection().offset;
            size_t offender = 0, changed = 0;
            p.expect(qacontract::onlyWhitelistedChanges(original, edited, {{g, g + sp.generalSize}, {st, st + sp.storageSize}},
                                                        &offender, &changed),
                     "byte outside the selected blocks changed at " + qacontract::hex(offender));
            p.expect(changed > 0, "no byte changed");
        }
        // S06: corrupt the first byte of the selected General block (its CRC covers the block body);
        // the reparse must stop reporting an exact, unrecovered match.
        if (parsed) {
            const size_t g = parsed->generalSelection().offset;
            const auto s06 = qamatrix::exportCorruptionProbe(edited, g, [&](const auto& bytes) {
                std::string why;
                const auto r = G4::Gen4ReadOnlySave::parse(bytes, v.layout, v.gameId, &why);
                return r && r->assignmentStatus() == G4::AssignmentStatus::Match && !r->recovered() &&
                       r->generalSelection().offset == g;
            });
            p.expect(s06.empty(), s06);
            if (s06.empty()) m.fact(ver, std::string(qamatrix::kExportChecksumFact),
                                    std::string(qamatrix::kExportChecksumDetected));
        }
        p.expect(sourceUnchanged("export"), "export changed the source");
        auto untouched = G4::Gen4StagedPokemonEditor::create(original, v.layout, v.gameId, &err);
        if (untouched) {
            auto request2 = request;
            request2.directoryStem += "_noop";
            const auto noop = G4::publishVerifiedGen4Export(*untouched, request2);
            p.expect(!noop.success && !fs::exists(exportRoot / request2.directoryStem), "unchanged export published");
        }
    });

    // ---------------------------------------------------------------- immutability
    cell(m, ver, "immutability", "source bytes, size and ns mtime identical after discovery, linking, open, "
             "staging, actions, review and export", [&](Probe& p) {
        p.expect(sourceUnchanged("immutability"), "source changed");
        std::string steps;
        for (const auto& s : mutationLog) steps += s + " ";
        p.expect(mutationLog.empty(), "source changed after: " + steps);
        p.expect(G4::readSourcePayloadReadOnly(source.string()).bytes == original, "payload differs");
    });

    // ---------------------------------------------------------------- home refresh
    std::optional<Bytes> emulatorWrite;  // the "new" save the emulator writes
    if (fieldsEditor) {
        std::string err;
        auto bytes = fieldsEditor->finalizedBytes(&err);
        if (!bytes.empty()) emulatorWrite = std::move(bytes);
    }
    m.fact(ver, "home.refresh", "snapshot-cache");
    cell(m, ver, "home-refresh", "HomeSnapshotCache: Parsed then Reused x30 (1 parse); emulator rewrite "
             "(new mtime) -> Parsed with new party; invalid rewrite -> Stale, never Current", [&](Probe& p) {
        S::HomeSnapshotCache cache;
        S::SnapshotRefresh outcome{};
        const auto& first = refreshHome(cache, bindings, v.gameId, outcome);
        p.expect(outcome == S::SnapshotRefresh::Parsed && first.current(), "first refresh not Parsed/Current: " +
                 first.reason);
        p.expect(first.gameId == v.gameId && first.trainerName == "Kylie" && first.trainerGenderKnown &&
                     first.trainerGender == 1, "trainer on the card");
        p.expect(first.dexSeen == 2 && first.dexCaught == 1 && first.dexTotal == 493, "dex on the card");
        p.expect(first.partyCount == 2 && first.party[0].species == 25 && first.party[0].level == 20 &&
                     first.party[1].species == kBurmy, "party on the card");
        const bool formKept = first.party[1].form == kBurmyForm;
        const bool levelKept = first.party[0].level == 20;
        m.fact(ver, "home.party.form", formKept ? "kept" : "dropped");
        m.fact(ver, "home.party.level", levelKept ? "kept" : "dropped");
        for (int i = 0; i < 30; ++i) {
            refreshHome(cache, bindings, v.gameId, outcome);
            if (outcome != S::SnapshotRefresh::Reused) {
                p.expect(false, "unchanged file reparsed");
                break;
            }
        }
        p.expect(cache.parseCount() == 1, "parse count " + std::to_string(cache.parseCount()));
        p.expect(emulatorWrite.has_value(), "no emulator write image");
        if (!emulatorWrite) return;
        qacontract::spit(source, *emulatorWrite);  // same size: detection is by mtime
        setMtime(source, 1500000000);
        const auto& changed = refreshHome(cache, bindings, v.gameId, outcome);
        p.expect(outcome == S::SnapshotRefresh::Parsed && changed.current(), "rewrite not reparsed: " + changed.reason);
        p.expect(changed.party[0].species == 25 && changed.party[0].level == 33, "card party level after rewrite");
        p.expect(changed.party[1].form == 2, "card form after rewrite");
        // The rewritten lead is shiny (fields cell); whether the card shows it is recorded as a fact
        // (Gen4GameCardPreview.h fills the card through HomePartyPreview::fromGen4).
        const auto reopened = G4::openAssignedSource(bindings, kProfile, v.gameId);
        bool leadShiny = false;
        if (reopened.save && !reopened.save->party().empty()) {
            const auto lead = Pokemon::Pokemon4Mutable::fromEncrypted(
                reopened.save->party()[0].originalEncryptedBytes(), reopened.save->rawFamily());
            leadShiny = lead && lead->shiny();
        }
        p.expect(leadShiny, "rewritten lead is not shiny in the save");
        m.fact(ver, "home.party.shiny", changed.party[0].shiny ? "shown" : "hidden");
        const bool genderKept = reopened.save && !reopened.save->party().empty() &&
                                reopened.save->party()[0].gender() <= 2 &&
                                changed.party[0].gender == reopened.save->party()[0].gender();
        m.fact(ver, "home.party.gender", genderKept ? "kept" : "dropped");
        p.expect(S::homePartyStatus(changed) == "Current save party", "party status '" + S::homePartyStatus(changed) + "'");
        const Bytes garbage(qasynth::gen4::kSaveSize, 0x5A);
        qacontract::spit(source, garbage);
        setMtime(source, 1600000000);
        const auto& stale = refreshHome(cache, bindings, v.gameId, outcome);
        p.expect(outcome == S::SnapshotRefresh::Parsed && stale.state == S::SnapshotState::Stale &&
                     stale.party[0].level == 33, "invalid rewrite not Stale with last data");
        p.expect(S::homePartyStatus(stale).rfind("STALE", 0) == 0, "stale wording");
        // The emulator writes a valid save again (the relaunch cell reads this state).
        qacontract::spit(source, *emulatorWrite);
        setMtime(source, 1700000000);
        const auto& recovered = refreshHome(cache, bindings, v.gameId, outcome);
        p.expect(outcome == S::SnapshotRefresh::Parsed && recovered.current(), "valid rewrite after stale not Current");
    });

    // ---------------------------------------------------------------- relaunch
    cell(m, ver, "relaunch", "fresh LegacySourceBindings load + fresh HomeSnapshotCache + re-inspect see the "
             "current file (Lv33 lead, new fingerprint, same source identity)", [&](Probe& p) {
        Legacy::LegacySourceBindings fresh(bindingsPath.string());
        p.expect(fresh.load(), "bindings reload failed");
        const auto resolved = fresh.resolveFileForGame(kProfile, v.gameId);
        p.expect(resolved.status == Legacy::AssignedFileStatus::Ready &&
                     resolved.binding.sourcePath == source.string(), "binding lost across relaunch");
        S::HomeSnapshotCache cache;
        S::SnapshotRefresh outcome{};
        const auto& snap = refreshHome(cache, fresh, v.gameId, outcome);
        p.expect(outcome == S::SnapshotRefresh::Parsed && snap.current(), "fresh cache not Current: " + snap.reason);
        p.expect(snap.party[0].level == 33 && snap.party[1].form == 2, "fresh cache shows old data");
        const auto again = G4::inspectSourceFile(source.string(), kProvider, v.gameId);
        p.expect(again.ready() && again.sourceIdentity == candidate.sourceIdentity, "re-inspect identity");
        p.expect(again.contentFingerprint != candidate.contentFingerprint, "fingerprint did not change");
        const auto reopened = G4::openAssignedSource(fresh, kProfile, v.gameId);
        p.expect(reopened.status == G4::OpenStatus::Ready && reopened.save &&
                     reopened.save->party()[0].partyLevel() == 33, "reopen after relaunch");
    });

    runRound2Gen4(m, v, original);

    std::error_code ec;
    fs::remove_all(root, ec);
}

} // namespace

void runGen4(Matrix& m) {
    for (const auto& v : versions()) runVersion(m, v);
}

} // namespace qamatrix
