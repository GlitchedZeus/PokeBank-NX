// PokeBank NX application QA: Gen I / Gen II rows of the Gen I-V acceptance matrix (Thread 4,
// QA-owned). Versions: red, blue, yellow, gold, silver, crystal.
//
// Production entry points driven here, exactly as the app wires them:
//   discovery / identity  Legacy::discoverConfiguredLegacySaves (SaveSelectScreen's runtime catalog:
//                         RetroArch RBY + GSC scanners) -> Legacy::buildFRLGSourceCards(ForProfile)
//   linking / relaunch    Legacy::LegacySourceBindings::claimInstanceAndSave, a fresh
//                         LegacySourceBindings::load() and a fresh discovery
//   provider              Source::SaveProviderIdentity (providerDisplayName / cardProviderLabel /
//                         homeCardSourceText)
//   parse                 Gen1::parse / Gen2::parse (strict, exact SourceGame)
//   bag                   Gen1::decodeInventory + Gen1::StagedInventoryEditor,
//                         Gen2::StagedEditor::inventoryEntries / stageInventoryQuantity
//   editing               Gen1::StagedPokemonEditor, Gen2::StagedEditor, UI models
//                         Gen1PokemonEditorUIContract / UX3 contract, Gen2PokemonSession,
//                         SharedPokemonEditorContract (the Gen II workspace navigation)
//   export                Gen1::publishVerifiedGen1Export (as Gen1PokemonEditorOverlayUX
//                         exportStagedCopy) and
//                         Gen2::publishVerifiedStagedEditorExport (as TrainerViewScreenGSCOverlay)
//
// Every save is SYNTHETIC and built in memory. Layout sources:
//   Gen I   tests/test_rby_gen1_adapter.cpp makeFixture (INTL layout, PKSM-Core aa22d7a4 / PKHeX
//           e15d2467 SAV1/PokeList1); Bag/PC lists as in tests/test_qa_export_contracts.cpp
//           gen1Source (tests/test_rby_staged_inventory.cpp INTL); Pokedex flag offsets from
//           src/Integration/Gen1/Gen1ReadOnlySave.cpp dexProgress.
//   Gen II  tests/fixtures/gsc_pokemon_fixture.h fixture(GS / C) (PKHeX SAV2); international mirror
//           tables from src/Integration/Gen2/Gen2StagedEditor.cpp synchronizeInternationalMirrors
//           (as tests/test_qa_export_contracts.cpp gen2Path); Pokedex flag offsets from
//           src/Integration/Gen2/Gen2ReadOnlySave.cpp dexProgress.
#include "fixtures/gsc_pokemon_fixture.h"
#include "test_qa_matrix.h"
#include "test_qa_matrix_home.h"
#include "test_qa_support.h"

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen1/Gen1ExportTransaction.h"
#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "Names/MoveNames.h"
#include "Pokemon/Experience.h"
#include "Names/SpeciesNames.h"
#include "Source/SaveProviderIdentity.h"
#include "UI/ExactSaveCapabilities.h"
#include "UI/Gen1PokemonEditorUIContract.h"
#include "UI/Gen1PokemonEditorUX3Contract.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/Gen2StagedAccess.h"
#include "UI/MovePickerPresentation.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <fcntl.h>
#include <sys/stat.h>

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <set>
#include <sstream>

namespace qamatrix {
namespace {

namespace fs = std::filesystem;
namespace G1 = PokeVault::Integration::Gen1;
namespace G2 = PokeVault::Integration::Gen2;
namespace Common = PokeVault::Integration::Common;
namespace Legacy = PokeVault::Legacy;
namespace Src = PokeVault::Source;
namespace SharedUI = PokeBank::UIModel::SharedPokemonEditor;
namespace Gen1UI = PokeBank::UIModel::Gen1Editor;
namespace Gen1UX3 = PokeBank::UIModel::Gen1Editor::UX3;
namespace Gen2UI = PokeBank::UIModel::Gen2PokemonEditor;
namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
using qacontract::Bytes;

constexpr const char* kProfile = "qa-profile-a";
constexpr const char* kOtherProfile = "qa-profile-b";
constexpr const char* kTimestamp = "20261015_120000";

// Records every cell exactly once; anything not reached is a Fail naming the step that stopped it.
class Row {
public:
    Row(Matrix& m, std::string version) : m_(m), v_(std::move(version)) {}
    void pass(std::string_view cell, std::string note = {}) { put(cell, Status::Pass, std::move(note)); }
    void fail(std::string_view cell, std::string note) { put(cell, Status::Fail, std::move(note)); }
    void check(std::string_view cell, bool ok, std::string failNote, std::string passNote = {}) {
        if (ok) pass(cell, std::move(passNote));
        else fail(cell, std::move(failNote));
    }
    void hw(std::string_view cell, std::string reason) { put(cell, Status::HardwareOnly, std::move(reason)); }
    void na(std::string_view cell, std::string reason) { put(cell, Status::Unsupported, std::move(reason)); }
    void fact(const std::string& key, std::string value) { m_.fact(v_, key, std::move(value)); }
    bool done(std::string_view cell) const { return done_.count(std::string(cell)) != 0; }
    void finish(const std::string& why) {
        for (auto cell : kCells)
            if (!done(cell)) put(cell, Status::Fail, "not reached: " + why);
    }
    const std::string& version() const { return v_; }

private:
    void put(std::string_view cell, Status s, std::string note) {
        if (!done_.insert(std::string(cell)).second) return;  // first verdict wins
        m_.set(v_, cell, s, std::move(note));
    }
    Matrix& m_;
    std::string v_;
    std::set<std::string> done_;
};

// Collects every failed expectation of one cell so the note names all of them.
struct Checks {
    std::vector<std::string> failures;
    void expect(bool ok, const std::string& what) {
        if (!ok) failures.push_back(what);
    }
    bool ok() const { return failures.empty(); }
    std::string text() const {
        std::string out;
        for (const auto& f : failures) out += (out.empty() ? "" : "; ") + f;
        return out;
    }
};

void setMtime(const fs::path& path, int64_t seconds) {
    struct timespec times[2] = {{seconds, 0}, {seconds, 0}};
    ::utimensat(AT_FDCWD, path.c_str(), times, 0);
}

// ------------------------------------------------------------------------------------------------
// Discovery environment: a RetroArch config whose savefile_directory is the synthetic save root,
// exactly the configured-root path SaveSelectScreen uses (discoverConfiguredLegacySaves).
// ------------------------------------------------------------------------------------------------
struct Env {
    fs::path root, saves, cfg, bindings;
};

// Directory names carry no release word: RetroArch discovery reads release hints from the whole
// path (RetroArchRBYDiscovery.cpp identityHint), so only the file name may name a game.
Env makeEnv() {
    static int counter = 0;
    Env e;
    e.root = qacontract::scratchRoot("g12-" + std::to_string(++counter));
    e.saves = e.root / "saves";
    e.cfg = e.root / "retroarch.cfg";
    e.bindings = e.root / "bindings" / "legacy_bindings.tsv";
    fs::create_directories(e.saves);
    fs::create_directories(e.bindings.parent_path());
    std::ofstream(e.cfg) << "savefile_directory = \"" << e.saves.string() << "\"\n";
    return e;
}

Legacy::FRLGDiscoveryResult discover(const Env& e) {
    return Legacy::discoverConfiguredLegacySaves({}, e.cfg.string(), (e.root / "no-fallback").string(),
                                                 (e.root / "no-mgba.ini").string(),
                                                 (e.root / "no-tico").string());
}

const Legacy::FRLGSource* sourceAt(const Legacy::FRLGDiscoveryResult& r, const fs::path& path) {
    for (const auto& s : r.sources)
        if (s.path == path.string()) return &s;
    return nullptr;
}

const Legacy::FRLGSourceCard* cardFor(const std::vector<Legacy::FRLGSourceCard>& cards,
                                      std::string_view gameId) {
    for (const auto& c : cards)
        if (c.gameId == gameId) return &c;
    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// Gen I synthetic save (international R/B/Y, 0x8000). Constants copied from
// tests/test_rby_gen1_adapter.cpp FixtureLayout INTL and makeFixture.
// ------------------------------------------------------------------------------------------------
namespace g1 {
constexpr size_t kMoney = 0x25F3, kTid = 0x2605, kCurrentBoxIndex = 0x284C, kParty = 0x2F2C,
                 kCurrentBox = 0x30C0, kChecksum = 0x3523, kMainLength = 0x0F8B, kStarter = 0x29C3,
                 kFriendship = 0x271C, kBoxSize = 0x462, kBankBoxesSize = 0x1A4C;
constexpr size_t kBoxCapacity = 20, kBoxCount = 12, kStr = 11;
// Bag (count, id/qty pairs, 0xFF) and PC item list: tests/test_qa_export_contracts.cpp gen1Source.
constexpr size_t kBag = 0x25C9, kBagBytes = 1 + 20 * 2 + 1, kPc = 0x27E6;
// Pokedex owned/seen flags: src/Integration/Gen1/Gen1ReadOnlySave.cpp dexProgress (international).
constexpr size_t kDexCaught = 0x25A3, kDexSeen = 0x25B6;

uint8_t diff8(std::span<const uint8_t> s) {
    uint8_t v = 0xFF;
    for (uint8_t b : s) v = static_cast<uint8_t>(v - b);
    return v;
}
void be16(Bytes& d, size_t o, uint16_t v) { d[o] = static_cast<uint8_t>(v >> 8); d[o + 1] = static_cast<uint8_t>(v); }
void be24(Bytes& d, size_t o, uint32_t v) {
    d[o] = static_cast<uint8_t>(v >> 16); d[o + 1] = static_cast<uint8_t>(v >> 8); d[o + 2] = static_cast<uint8_t>(v);
}
std::vector<uint8_t> enc(const std::string& s) {
    std::vector<uint8_t> out(kStr, 0x50);
    for (size_t i = 0; i < s.size() && i < kStr; ++i) {
        const char c = s[i];
        if (c >= 'A' && c <= 'Z') out[i] = static_cast<uint8_t>(0x80 + (c - 'A'));
        else if (c >= '0' && c <= '9') out[i] = static_cast<uint8_t>(0xF6 + (c - '0'));
        else out[i] = 0x7F;
    }
    return out;
}
size_t boxStart(size_t box) {
    const size_t half = kBoxCount / 2;
    return box < half ? 0x4000 + box * kBoxSize : 0x6000 + (box - half) * kBoxSize;
}
void emptyList(Bytes& d, size_t offset, size_t capacity) {
    d[offset] = 0;
    std::fill(d.begin() + static_cast<std::ptrdiff_t>(offset + 1),
              d.begin() + static_cast<std::ptrdiff_t>(offset + capacity + 2), 0);
    d[offset + 1] = 0xFF;
}
void fillBody(Bytes& d, size_t o, size_t bodySize, uint8_t rawSpecies, uint8_t level, uint32_t exp) {
    std::fill(d.begin() + static_cast<std::ptrdiff_t>(o), d.begin() + static_cast<std::ptrdiff_t>(o + bodySize), 0);
    d[o] = rawSpecies;
    be16(d, o + 1, 45);
    d[o + 3] = level;
    d[o + 5] = 0x14; d[o + 6] = 0x14;
    d[o + 7] = 45;
    d[o + 8] = 33; d[o + 9] = 45;
    be16(d, o + 0x0C, 0x1234);
    be24(d, o + 0x0E, exp);
    be16(d, o + 0x11, 1234); be16(d, o + 0x13, 2345); be16(d, o + 0x15, 3456);
    be16(d, o + 0x17, 4567); be16(d, o + 0x19, 5678);
    be16(d, o + 0x1B, 0xA987);
    d[o + 0x1D] = 0xC0 | 20; d[o + 0x1E] = 15;
    if (bodySize == 44) {
        d[o + 0x21] = level;
        be16(d, o + 0x22, 52); be16(d, o + 0x24, 31); be16(d, o + 0x26, 28);
        be16(d, o + 0x28, 35); be16(d, o + 0x2A, 30);
    }
}
void entry(Bytes& d, size_t offset, size_t capacity, size_t bodySize, size_t index, uint8_t species,
           uint8_t level, const std::string& ot, const std::string& nick) {
    const size_t bodyStart = offset + capacity + 2;
    const size_t otStart = bodyStart + capacity * bodySize;
    const size_t nickStart = otStart + capacity * kStr;
    d[offset] = static_cast<uint8_t>(std::max<size_t>(d[offset], index + 1));
    d[offset + 1 + index] = species;
    d[offset + 1 + d[offset]] = 0xFF;
    // Production requires EXP to match the level on the species growth curve
    // (Gen1StagedPokemonEditor.cpp validPokemon), so the fixture uses the exact curve value.
    const uint32_t exp = Pokemon::getExpForLevel(
        level, G1::StagedPokemonEditor::growthRate(G1::gen1InternalToNational(species)));
    fillBody(d, bodyStart + index * bodySize, bodySize, species, level, exp);
    const auto o = enc(ot), n = enc(nick);
    std::copy(o.begin(), o.end(), d.begin() + static_cast<std::ptrdiff_t>(otStart + index * kStr));
    std::copy(n.begin(), n.end(), d.begin() + static_cast<std::ptrdiff_t>(nickStart + index * kStr));
}
void checksums(Bytes& d) {
    const size_t half = kBoxCount / 2;
    for (int bank = 0; bank < 2; ++bank) {
        const size_t start = bank == 0 ? 0x4000 : 0x6000;
        const size_t sumOfs = start + kBankBoxesSize;
        for (size_t i = 0; i < half; ++i)
            d[sumOfs + 1 + i] = diff8(std::span<const uint8_t>(d).subspan(start + i * kBoxSize, kBoxSize));
        d[sumOfs] = diff8(std::span<const uint8_t>(d).subspan(start, kBankBoxesSize));
    }
    d[kChecksum] = diff8(std::span<const uint8_t>(d).subspan(0x2598, kMainLength));
}

// Party: Pikachu (raw 0x54 -> #25, level partyLevel) and Rhydon (raw 0x01 -> #112, L20).
// Current box (index 2): Bulbasaur (raw 0x99 -> #1). Box 0: Charmander (raw 0xB0 -> #4).
// Yellow evidence (starter Pikachu + friendship) exactly as makeFixture writes it.
Bytes save(bool yellow, uint8_t partyLevel = 15) {
    Bytes d(0x8000, 0);
    const auto name = enc("WILL");
    std::copy(name.begin(), name.end(), d.begin() + 0x2598);
    d[kMoney] = 0x12; d[kMoney + 1] = 0x34; d[kMoney + 2] = 0x56;
    be16(d, kTid, 0x1234);
    d[kStarter] = yellow ? 0x54 : 0x99;
    d[kFriendship] = yellow ? 90 : 0;
    d[kCurrentBoxIndex] = 0x80 | 2;
    emptyList(d, kParty, 6);
    entry(d, kParty, 6, 44, 0, 0x54, partyLevel, "RED", "SPARKY");
    entry(d, kParty, 6, 44, 1, 0x01, 20, "RED", "RHYDON");
    emptyList(d, kCurrentBox, kBoxCapacity);
    entry(d, kCurrentBox, kBoxCapacity, 33, 0, 0x99, 8, "RED", "BULBA");
    for (size_t box = 0; box < kBoxCount; ++box) emptyList(d, boxStart(box), kBoxCapacity);
    entry(d, boxStart(0), kBoxCapacity, 33, 0, 0xB0, 12, "RED", "CHAR");
    d[0x2ED5] = 0xFF;
    std::fill_n(d.begin() + kBag, kBagBytes, 0);
    d[kBag] = 1; d[kBag + 1] = 20; d[kBag + 2] = 3; d[kBag + 3] = 0xFF;  // item 20 x3
    d[kPc] = 0; d[kPc + 1] = 0xFF;
    // Seen: #1 and #25; caught: #25 (bit (species-1)&7 of byte (species-1)>>3).
    d[kDexSeen + 0] |= 0x01; d[kDexSeen + 3] |= 0x01; d[kDexCaught + 3] |= 0x01;
    checksums(d);
    return d;
}
} // namespace g1

// ------------------------------------------------------------------------------------------------
// Gen II synthetic save: tests/fixtures/gsc_pokemon_fixture.h fixture(GS / C), made internally
// consistent (secondary copy + secondary checksum) with the production mirror tables.
// ------------------------------------------------------------------------------------------------
namespace g2 {
// Pokedex flags: src/Integration/Gen2/Gen2ReadOnlySave.cpp dexProgress (international).
constexpr size_t kDexCaughtGS = 0x2A4C, kDexSeenGS = 0x2A6C, kDexCaughtC = 0x2A27, kDexSeenC = 0x2A47;

void mirror(Bytes& d, const ::L& l) {
    if (l.family == G2::VersionFamily::Crystal) {
        std::copy(d.begin() + 0x2009, d.begin() + 0x2B83, d.begin() + 0x1209);
    } else {
        struct M { size_t begin, end, destination; };
        constexpr M table[] = {{0x2009, 0x222F, 0x15C7}, {0x222F, 0x23D9, 0x3D69}, {0x23D9, 0x2856, 0x0C6B},
                               {0x2856, 0x288A, 0x7E39}, {0x288A, 0x2D69, 0x10E8}};
        for (const auto& m : table)
            std::copy(d.begin() + static_cast<std::ptrdiff_t>(m.begin), d.begin() + static_cast<std::ptrdiff_t>(m.end),
                      d.begin() + static_cast<std::ptrdiff_t>(m.destination));
    }
    d[l.sum2] = d[l.sum];
    d[l.sum2 + 1] = d[l.sum + 1];
}

// Party: Pikachu (#25) at partyLevel (EXP = level^3, Pikachu medium-fast, as the fixture's 3375 = 15^3).
// Boxes 0 and 1: one Pikachu each ("PIKA", "PIKA2"). Bag: Potion x2, Poke Ball x5.
Bytes save(const ::L& l, uint8_t partyLevel = 15) {
    Bytes d = ::fixture(l);
    const size_t body = l.party + 1 + (6 + 1);  // gsc_pokemon_fixture.h partyList body offset
    d[body + 31] = partyLevel;
    ::be24(d, body + 8, static_cast<uint32_t>(partyLevel) * partyLevel * partyLevel);
    const bool crystal = l.family == G2::VersionFamily::Crystal;
    const size_t seen = crystal ? kDexSeenC : kDexSeenGS, caught = crystal ? kDexCaughtC : kDexCaughtGS;
    d[seen + 0] |= 0x01; d[seen + 3] |= 0x01; d[caught + 3] |= 0x01;
    ::checksum(d, l);
    mirror(d, l);
    return d;
}
} // namespace g2

// ------------------------------------------------------------------------------------------------
// Shared steps for one version (discovery / linking / provider / identity / home / relaunch).
// ------------------------------------------------------------------------------------------------
struct Spec {
    std::string version;
    int generation = 1;
    G1::SourceGame g1 = G1::SourceGame::Red;
    G2::SourceGame g2 = G2::SourceGame::Gold;
    std::string gameId;
    std::string fileName;      // file name the app's discovery sees
    std::string wrongHintName; // a different release's file name for the same bytes
    bool structuralIdentity = false;  // Crystal: identified from the save's own structure
    std::function<Bytes(uint8_t partyLevel)> build;
};

std::string platformExpected(int generation) { return generation == 1 ? "Game Boy" : "GBC"; }

// Discovery + linking + provider + identity. Returns the claimed instance (or nothing).
std::optional<Src::SaveInstance> runSourceCells(Row& row, const Spec& spec, const Env& env,
                                                const fs::path& source) {
    // discovery
    auto result = discover(env);
    const auto* found = sourceAt(result, source);
    const auto cards = Legacy::buildFRLGSourceCards(result);
    const auto* card = cardFor(cards, spec.gameId);
    {
        Checks c;
        c.expect(found != nullptr, "discoverConfiguredLegacySaves did not list " + source.filename().string());
        if (found) {
            c.expect(found->ready(), "source not Ready: " + found->detail);
            c.expect(found->gameId == spec.gameId, "gameId " + found->gameId);
            c.expect(spec.generation == 1 ? found->isGen1() : found->isGen2(), "wrong typed save pointer");
        }
        c.expect(card != nullptr && card->instances.size() == 1, "no single-instance card for " + spec.gameId);
        if (card) c.expect(card->platformLabel == platformExpected(spec.generation), "platform label " + card->platformLabel);
        if (card && found)
            c.expect(Legacy::resolveFRLGSaveInstance(result, *card, 0) == found, "card instance does not resolve to the source");
        row.check("discovery", c.ok(), c.text(),
                  "discoverConfiguredLegacySaves (RetroArch configured root) -> Ready " + spec.gameId +
                      " card, platform " + platformExpected(spec.generation));
    }
    if (!found || !card || card->instances.empty()) return std::nullopt;
    const auto instance = card->instances.front();

    // provider
    {
        Checks c;
        c.expect(instance.providerLabel == "RetroArch", "providerLabel '" + instance.providerLabel + "'");
        c.expect(Src::identifyProvider(instance.providerLabel) == Src::ProviderIdentity::RetroArch, "identifyProvider");
        c.expect(Src::cardProviderDisplay(Src::cardProviderLabel(card->instances)) == "RetroArch", "card provider display");
        c.expect(Src::homeCardSourceText("", instance.providerLabel) == "RetroArch", "homeCardSourceText");
        c.expect(Src::saveInstanceSourceLine(instance).rfind("RetroArch / ", 0) == 0, "saveInstanceSourceLine");
        row.check("provider", c.ok(), c.text(), "discovery-recorded provider 'RetroArch' -> display 'RetroArch'");
    }

    // identity
    {
        Checks c;
        c.expect(instance.gameId == spec.gameId, "instance gameId " + instance.gameId);
        const auto meta = spec.generation == 1 ? found->gen1Save->metadata().sourceGameId
                                               : found->gen2Save->metadata().sourceGameId;
        c.expect(meta == spec.gameId, "parsed sourceGameId " + meta);
        const auto caps = Foundation::capabilitiesForSourceId(spec.gameId);
        c.expect(caps.has_value(), "ExactSaveCapabilities has no entry");
        // The same bytes under a different release's file name (and under a neutral name).
        const Env other = makeEnv();
        const Bytes bytes = qacontract::slurp(source);
        qacontract::writeSource(other.saves / spec.wrongHintName, bytes);
        qacontract::writeSource(other.saves / "slot1.srm", bytes);
        const auto r2 = discover(other);
        const auto* hinted = sourceAt(r2, other.saves / spec.wrongHintName);
        const auto* neutral = sourceAt(r2, other.saves / "slot1.srm");
        std::string how;
        if (spec.structuralIdentity) {
            // Crystal: a neutral name is identified from the save's own structure.
            c.expect(neutral && neutral->ready() && neutral->gameId == spec.gameId,
                     "neutral file name not identified structurally");
            c.expect(!hinted || !hinted->ready() || hinted->gameId == spec.gameId,
                     "contradicting file-name hint produced another release");
            how = "neutral file name -> " + spec.gameId + " from save structure";
            row.fact("identity.source", "save-internal");
        } else if (spec.generation == 1 && spec.g1 != G1::SourceGame::Yellow) {
            // Red vs Blue is not stored in the save: the file name decides; no name -> ambiguous.
            c.expect(hinted && hinted->ready() && hinted->gameId != spec.gameId,
                     "same bytes under '" + spec.wrongHintName + "' not identified as the other release");
            c.expect(neutral && !neutral->ready() && neutral->status == Legacy::LegacySourceStatus::AmbiguousIdentity,
                     std::string("neutral name was not AmbiguousIdentity: ") + (neutral ? neutral->detail + " gameId=" + neutral->gameId : "not listed"));
            how = "same bytes as '" + spec.wrongHintName + "' -> " + (hinted ? hinted->gameId : "?") +
                  "; neutral name -> AmbiguousIdentity (R/B not stored in the save)";
            row.fact("identity.source", "filename");
        } else {
            // Yellow / Gold / Silver: the hint is required, and the save's own evidence vetoes a
            // contradicting hint.
            c.expect(hinted && !hinted->ready(), "contradicting hint '" + spec.wrongHintName + "' was accepted");
            c.expect(neutral && !neutral->ready() && neutral->status == Legacy::LegacySourceStatus::AmbiguousIdentity,
                     std::string("neutral name was not AmbiguousIdentity: ") + (neutral ? neutral->detail + " gameId=" + neutral->gameId : "not listed"));
            how = "contradicting hint '" + spec.wrongHintName + "' rejected; neutral name -> AmbiguousIdentity";
            row.fact("identity.source", "filename");
        }
        fs::remove_all(other.root);
        row.check("identity", c.ok(), c.text(), how);
    }

    // linking
    {
        Checks c;
        Legacy::LegacySourceBindings bindings(env.bindings.string());
        c.expect(bindings.load(), "bindings load (empty)");
        c.expect(bindings.claimInstanceAndSave(instance, kProfile), "claimInstanceAndSave: " + bindings.lastError());
        auto r = discover(env);
        const auto mine = Legacy::buildFRLGSourceCardsForProfile(r, bindings, kProfile);
        const auto theirs = Legacy::buildFRLGSourceCardsForProfile(r, bindings, kOtherProfile);
        const auto* myCard = cardFor(mine, spec.gameId);
        c.expect(myCard && myCard->instances.size() == 1, "claimed instance not visible to its profile");
        if (myCard && !myCard->instances.empty()) {
            c.expect(myCard->instances.front().path() == source.string(), "claimed instance resolves to another file");
            c.expect(myCard->instances.front().claimedProfile == kProfile, "claimedProfile not recorded");
            const auto* resolved = Legacy::resolveFRLGSaveInstance(r, *myCard, 0);
            c.expect(resolved && resolved->path == source.string(), "resolveFRLGSaveInstance");
        }
        c.expect(cardFor(theirs, spec.gameId) == nullptr, "claimed instance visible to another profile");
        Legacy::LegacySourceBindings reloaded(env.bindings.string());
        c.expect(reloaded.load(), "bindings reload");
        c.expect(reloaded.assignedProfile(instance.sourceIdentity) == kProfile, "persisted assignment lost");
        row.check("linking", c.ok(), c.text(),
                  "claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists");
    }
    return instance;
}

// Home refresh (legacy re-discovery) + relaunch (fresh bindings + fresh discovery).
void runHomeCells(Row& row, const Spec& spec, const Env& env, const fs::path& source,
                  const Src::SaveInstance& shown) {
    // The emulator rewrites the save: a new party level and a new mtime.
    const Bytes rewritten = spec.build(42);
    qacontract::spit(source, rewritten);
    setMtime(source, 1100000000);
    {
        Checks c;
        Legacy::LegacySourceBindings bindings(env.bindings.string());
        c.expect(bindings.load(), "bindings load");
        auto r = discover(env);
        const auto cards = Legacy::buildFRLGSourceCardsForProfile(r, bindings, kProfile);
        const auto* card = cardFor(cards, spec.gameId);
        c.expect(card && card->instances.size() == 1, "card missing after rewrite");
        if (card && !card->instances.empty()) {
            const auto& fresh = card->instances.front();
            c.expect(fresh.sourceIdentity == shown.sourceIdentity, "source identity changed on rewrite");
            c.expect(fresh.contentFingerprint != shown.contentFingerprint, "fingerprint did not change");
            c.expect(fresh.modifiedTime != shown.modifiedTime, "modified time not refreshed");
            c.expect(!Src::sameValidatedSnapshot(shown, fresh), "sameValidatedSnapshot still true");
            const auto* s = Legacy::resolveFRLGSaveInstance(r, *card, 0);
            c.expect(s != nullptr, "fresh instance does not resolve");
            if (s) {
                // The Home party strip reads species + level from this parsed save (SaveSelectScreen.cpp
                // RetroArchFRLG branch: addParty(i, species, level, 0, false)).
                const uint16_t species = spec.generation == 1 ? s->gen1Save->party().at(0).species
                                                              : s->gen2Save->party().at(0).species;
                const uint8_t level = spec.generation == 1 ? s->gen1Save->party().at(0).level
                                                           : s->gen2Save->party().at(0).level;
                c.expect(species == 25 && level == 42, "party[0] after rewrite: #" + std::to_string(species) +
                                                           " L" + std::to_string(level));
            }
        }
        row.check("home-refresh", c.ok(), c.text(),
                  "rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the "
                  "stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp");
    }
    row.fact("home.refresh", "legacy-rediscovery");
    // Derived from the production Home adapters (HomePartyPreview::fromGen1/2), never hard-coded.
    const auto fact = [&](const std::string& key, const std::string& value) { row.fact(key, value); };
    if (spec.generation == 1) home::recordGen1(fact);
    else home::recordGen2(fact);

    {
        Checks c;
        Legacy::LegacySourceBindings fresh(env.bindings.string());
        c.expect(fresh.load(), "fresh process bindings load");
        auto r = discover(env);
        const auto cards = Legacy::buildFRLGSourceCardsForProfile(r, fresh, kProfile);
        const auto* card = cardFor(cards, spec.gameId);
        c.expect(card && card->instances.size() == 1, "fresh process does not see the assigned save");
        c.expect(cardFor(Legacy::buildFRLGSourceCardsForProfile(r, fresh, kOtherProfile), spec.gameId) == nullptr,
                 "fresh process shows the save to another profile");
        if (card && !card->instances.empty()) {
            const auto& inst = card->instances.front();
            c.expect(inst.contentFingerprint == Common::sha256Hex(rewritten), "fresh process fingerprint is not the current file");
            c.expect(inst.path() == source.string(), "fresh process resolves another file");
        }
        row.check("relaunch", c.ok(), c.text(),
                  "new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint");
    }
}

std::string partySummary(size_t count) { return "party " + std::to_string(count); }

// ------------------------------------------------------------------------------------------------
// Gen I
// ------------------------------------------------------------------------------------------------
void runGen1Version(Matrix& m, const Spec& spec) {
    Row row(m, spec.version);
    std::string stage = "setup";
    try {
        const Env env = makeEnv();
        const fs::path source = env.saves / spec.fileName;
        const Bytes original = spec.build(15);
        qacontract::writeSource(source, original);
        const auto sourceBefore = qacontract::fileState(source);

        stage = "discovery";
        const auto instance = runSourceCells(row, spec, env, source);

        stage = "parse";
        auto parsed = G1::parse(original, spec.g1);
        if (!parsed) throw std::runtime_error("strict parse rejected the synthetic save: " + parsed.detail);
        const auto& save = *parsed.save;

        row.check("trainer-name", save.trainer().name == "WILL" && save.trainer().trainerId == 0x1234,
                  "trainer '" + save.trainer().name + "'", "WILL / TID 4660");
        row.na("trainer-gender",
               "Gen I save format stores no player gender (Gen1::TrainerRecord has no gender field; "
               "SaveSelectScreen.cpp loadLegacySources fixes gender 0: 'Red is fixed by the game')");
        row.fact("trainer.gender", "not-stored");
        {
            Checks c;
            const auto& p = save.party();
            c.expect(p.size() == 2, partySummary(p.size()));
            if (p.size() == 2) {
                c.expect(p[0].species == 25 && p[0].level == 15 && p[0].nickname == "SPARKY", "party[0]");
                c.expect(p[1].species == 112 && p[1].level == 20 && p[1].nickname == "RHYDON", "party[1]");
            }
            c.expect(Foundation::partyEditEnabled() == false && Gen1UI::partyEditingEnabled() == false,
                     "party editing advertised");
            row.check("party", c.ok(), c.text(), "2 members (#25 L15 SPARKY, #112 L20 RHYDON); read-only by contract");
        }
        {
            Checks c;
            c.expect(save.boxes().size() == g1::kBoxCount, "box count " + std::to_string(save.boxes().size()));
            if (save.boxes().size() == g1::kBoxCount) {
                const auto& b0 = save.boxes()[0].slots;
                c.expect(!b0.empty() && b0[0] && b0[0]->species == 4 && b0[0]->nickname == "CHAR", "box 0 slot 0");
                const auto& b2 = save.boxes()[2].slots;
                c.expect(!b2.empty() && b2[0] && b2[0]->species == 1, "current box 2 slot 0");
            }
            c.expect(save.metadata().currentBox == 2 && save.metadata().boxesInitialized, "current box metadata");
            row.check("boxes", c.ok(), c.text(), "12 boxes; box 0 #4 CHAR; current box 2 #1");
        }
        {
            const auto dex = save.dexProgress();
            row.check("pokedex", dex.seen == 2 && dex.caught == 1 && dex.total == 151,
                      "dex " + std::to_string(dex.seen) + "/" + std::to_string(dex.caught) + "/" + std::to_string(dex.total),
                      "seen 2 / caught 1 / total 151");
        }

        stage = "bag";
        {
            Checks c;
            const auto inv = G1::decodeInventory(save.sourceBytes(), save.metadata().region);
            c.expect(inv.available, "decodeInventory unavailable: " + inv.error);
            c.expect(inv.bag.size() == 1 && inv.bag[0].itemId == 20 && inv.bag[0].quantity == 3, "bag contents");
            std::string error;
            auto bag = G1::StagedInventoryEditor::create(save, error);
            c.expect(bag != nullptr, "StagedInventoryEditor::create: " + error);
            if (bag) {
                c.expect(bag->stageSetQuantity(PokeVault::Inventory::ClassicPocket::Medicines, 20, 25, error),
                         "stageSetQuantity: " + error);
                const auto out = bag->finalize(error);
                c.expect(!out.empty(), "finalize: " + error);
                if (!out.empty()) {
                    auto re = G1::parse(out, spec.g1);
                    c.expect(static_cast<bool>(re), "reparse: " + re.detail);
                    const auto inv2 = G1::decodeInventory(out, save.metadata().region);
                    c.expect(inv2.bag.size() == 1 && inv2.bag[0].quantity == 25, "staged quantity not round-tripped");
                }
            }
            row.check("bag", c.ok(), c.text(), "Bag item 20 x3 decoded; staged x25 round-trips through strict reparse");
            row.fact("bag.read", "yes");
        }

        stage = "editor";
        std::string error;
        auto editor = G1::StagedPokemonEditor::create(save, error);
        if (!editor) throw std::runtime_error("StagedPokemonEditor::create: " + error);
        const Bytes staged0(editor->stagedBytes().begin(), editor->stagedBytes().end());

        // navigation: the Gen I editor's focus is a row index over the UX3 workspace contract; only
        // StageAdd (or an explicit Apply/Keep) mutates.
        {
            Checks c;
            constexpr auto rows = Gen1UX3::workspaceRowCount();
            constexpr auto starts = Gen1UX3::sectionStarts();
            c.expect(rows == 14 && starts.back() < rows, "workspace rows/sections");
            for (auto ev : {Gen1UI::AddDraftEvent::Navigate, Gen1UI::AddDraftEvent::EditField,
                            Gen1UI::AddDraftEvent::PreviewSpecies, Gen1UI::AddDraftEvent::JumpSection,
                            Gen1UI::AddDraftEvent::OpenLogicalSubEditor, Gen1UI::AddDraftEvent::CancelSubEditor,
                            Gen1UI::AddDraftEvent::Cancel})
                c.expect(!Gen1UI::addDraftDecision(ev).mutateStagedSave, "a non-commit draft event mutates");
            c.expect(Gen1UI::addDraftDecision(Gen1UI::AddDraftEvent::StageAdd).mutateStagedSave, "StageAdd does not commit");
            c.expect(!Gen1UX3::speciesPickerBrowseMutatesStagedSave() && !Gen1UX3::movePickerBrowseMutatesStagedSave() &&
                         !Gen1UX3::shinyPreviewMutatesStagedSave() && Gen1UX3::viewIsReadOnly(),
                     "browse/preview contract mutates");
            // Reading every slot the editor would focus never changes the staged copy.
            for (size_t box = 0; box < g1::kBoxCount; ++box)
                for (size_t slot = 0; slot < g1::kBoxCapacity; ++slot) (void)editor->boxedPokemon(box, slot, error);
            c.expect(Bytes(editor->stagedBytes().begin(), editor->stagedBytes().end()) == staged0 &&
                         !editor->hasPendingChanges(),
                     "browsing changed the staged copy");
            row.check("navigation", c.ok(), c.text(),
                      "UX3 14-row workspace + addDraftDecision: only StageAdd commits; browsing every slot leaves "
                      "the staged copy byte-identical (row arithmetic itself is UI code in "
                      "Gen1PokemonEditorOverlayUXCleanup3.inc)");
            row.fact("nav.mutates", "no");
        }

        // creation: the UI starts every Add draft from BoxPokemonCreate{} (Gen1PokemonEditorOverlayUXCleanup2.inc).
        {
            Checks c;
            auto e = G1::StagedPokemonEditor::create(save, error);
            // Plain index (kNoSlot = refused): GCC -O2 reports a false maybe-uninitialized on optional.
            constexpr size_t kNoSlot = ~size_t{0};
            size_t slot = kNoSlot;
            if (e) {
                if (const auto appended = e->appendSlot(0, error)) slot = *appended;
            }
            c.expect(slot == 1, "appendSlot(box 0)");
            if (slot != kNoSlot) {
                c.expect(!e->stageAdd(0, slot + 1, G1::BoxPokemonCreate{}, error), "non-contiguous destination accepted");
                G1::BoxPokemonCreate bad{};
                bad.species = 0;
                c.expect(!e->stageAdd(0, slot, bad, error), "species 0 accepted");
                bad = G1::BoxPokemonCreate{};
                bad.level = 0;
                c.expect(!e->stageAdd(0, slot, bad, error), "level 0 accepted");
                const G1::BoxPokemonCreate defaults{};
                c.expect(e->stageAdd(0, slot, defaults, error), "stageAdd(defaults): " + error);
                const auto out = e->finalizedBytes(error);
                auto re = G1::parse(out, spec.g1);
                c.expect(static_cast<bool>(re), "reparse after add: " + re.detail);
                if (re) {
                    const auto& p = re.save->boxes()[0].slots.at(slot);
                    c.expect(p && p->species == 25 && p->level == 5, "created record");
                    if (p) {
                        row.fact("create.default.species", std::to_string(p->species));
                        row.fact("create.default.level", std::to_string(p->level));
                    }
                }
            }
            row.check("creation", c.ok(), c.text(),
                      "BoxPokemonCreate{} -> #25 L5 at the first empty slot; species 0, level 0 and a gap rejected");
        }

        // fields: every BoxPokemonEdit field stages, reads back, and survives finalize + strict reparse.
        {
            Checks c;
            auto e = G1::StagedPokemonEditor::create(save, error);
            auto stage1 = [&](const char* what, const G1::BoxPokemonEdit& ed,
                              const std::function<bool(const G1::PokemonRecord&)>& ok) {
                if (!e->stageEdit(0, 0, ed, error)) {
                    c.expect(false, std::string(what) + ": " + error);
                    return;
                }
                const auto p = e->boxedPokemon(0, 0, error);
                c.expect(p && ok(*p), std::string(what) + " did not read back");
            };
            G1::BoxPokemonEdit ed;
            ed = {}; ed.species = 5;
            stage1("species", ed, [](const auto& p) { return p.species == 5; });
            ed = {}; ed.nickname = "FLAME";
            stage1("nickname", ed, [](const auto& p) { return p.nickname == "FLAME"; });
            ed = {}; ed.otName = "ASH";
            stage1("otName", ed, [](const auto& p) { return p.originalTrainer == "ASH"; });
            ed = {}; ed.level = 20;
            stage1("level", ed, [](const auto& p) { return p.level == 20; });
            const uint32_t exp = e->boxedPokemon(0, 0, error)->experience + 100;
            ed = {}; ed.experience = exp;
            stage1("experience", ed, [&](const auto& p) { return p.experience == exp; });
            ed = {}; ed.trainerId = 777;
            stage1("trainerId", ed, [](const auto& p) { return p.trainerId == 777; });
            const std::array<uint8_t, 4> moves{52, 10, 0, 0};  // Ember, Scratch
            const std::array<uint8_t, 4> ups{1, 0, 0, 0};
            const std::array<uint8_t, 4> pp{G1::StagedPokemonEditor::moveMaxPP(52, 1),
                                            G1::StagedPokemonEditor::moveMaxPP(10, 0), 0, 0};
            ed = {}; ed.moves = moves; ed.ppUps = ups; ed.pp = pp;
            stage1("moves/pp/ppUps", ed, [&](const auto& p) { return p.moves == moves && p.pp == pp && p.ppUps == ups; });
            ed = {}; ed.dvs = std::array<uint8_t, 4>{1, 2, 3, 4};
            stage1("dvs", ed, [](const auto& p) { return p.dvs[1] == 1 && p.dvs[2] == 2 && p.dvs[3] == 3 && p.dvs[4] == 4; });
            ed = {}; ed.statExperience = std::array<uint16_t, 5>{10, 20, 30, 40, 50};
            stage1("statExperience", ed, [](const auto& p) {
                return p.statExperience == std::array<uint16_t, 5>{10, 20, 30, 40, 50};
            });
            const auto expected = e->boxedPokemon(0, 0, error);
            const auto out = e->finalizedBytes(error);
            auto re = G1::parse(out, spec.g1);
            c.expect(static_cast<bool>(re), "reparse: " + re.detail);
            if (re && expected) {
                const auto& got = re.save->boxes()[0].slots.at(0);
                c.expect(got && got->species == expected->species && got->nickname == expected->nickname &&
                             got->originalTrainer == expected->originalTrainer && got->level == expected->level &&
                             got->experience == expected->experience && got->trainerId == expected->trainerId &&
                             got->moves == expected->moves && got->pp == expected->pp && got->ppUps == expected->ppUps &&
                             got->dvs == expected->dvs && got->statExperience == expected->statExperience,
                         "finalized record differs from the staged record");
            }
            c.expect(!Gen1UI::hasHeldItemField() && !Gen1UI::hasNatureField() && !Gen1UI::hasAbilityField(),
                     "Gen I contract advertises a modern field");
            row.check("fields", c.ok(), c.text(),
                      "species, nickname, OT, level, EXP, TID, moves+PP+PP Ups, DVs, Stat Exp stage, read back and "
                      "survive finalize + strict reparse (no held item/nature/ability in Gen I)");
        }

        // pickers
        {
            Checks c;
            for (uint16_t s = 1; s <= 151; ++s) {
                const char* name = Names::getSpeciesName(s);
                if (!name || !*name) { c.expect(false, "no name for #" + std::to_string(s)); break; }
            }
            c.expect(Gen1UI::speciesPickerRow(25, Names::getSpeciesName(25)) == "025 - Pikachu", "species row text");
            const auto types = G1::StagedPokemonEditor::personalTypes(25);
            c.expect(types[0] == types[1], "Pikachu personal types");
            const auto moves = G1::MoveCompatibility::compatibleMoves(spec.g1, 25);
            c.expect(!moves.empty() && moves.front() == 0 && std::find(moves.begin(), moves.end(), 84) != moves.end(),
                     "Thundershock not offered for Pikachu");
            c.expect(std::all_of(moves.begin(), moves.end(), [](uint8_t mv) { return mv <= 165; }),
                     "move outside Gen I range offered");
            c.expect(!G1::MoveCompatibility::canLearnMove(spec.g1, 25, 166), "move 166 accepted");
            c.expect(PokeBank::UIModel::MovePickerPresentation::numberedName(84).rfind("084 - ", 0) == 0, "move row text");
            row.check("pickers", c.ok(), c.text(),
                      "151 named species; Pikachu move list (" + std::to_string(moves.size()) +
                          " moves) includes Thundershock, all within 1..165");
        }

        // actions: every entry the Gen I action menu offers is backed by the staged editor.
        {
            Checks c;
            auto e = G1::StagedPokemonEditor::create(save, error);
            const auto occupied = Gen1UI::actionsForSlot(true, false);
            const auto empty = Gen1UI::actionsForSlot(false, false);
            c.expect(occupied.count == 6 && occupied[0] == Gen1UI::Action::View && occupied[1] == Gen1UI::Action::Edit &&
                         occupied[2] == Gen1UI::Action::Clone && occupied[3] == Gen1UI::Action::Remove,
                     "occupied-slot actions");
            c.expect(empty.count == 2 && empty[0] == Gen1UI::Action::AddPokemon, "empty-slot actions");
            const auto dest = e->appendSlot(0, error);
            c.expect(dest && e->stageClone(0, 0, 0, *dest, error), "Clone: " + error);
            const auto pending = Gen1UI::actionsForSlot(true, e->hasPendingChanges());
            c.expect(pending.count == 7 && pending[5] == Gen1UI::Action::ReviewPendingChanges, "Review not offered with pending changes");
            c.expect(e->stageRemove(0, 0, error), "Remove: " + error);
            const auto out = e->finalizedBytes(error);
            c.expect(static_cast<bool>(G1::parse(out, spec.g1)), "clone+remove result failed strict reparse");
            row.check("actions", c.ok(), c.text(),
                      "View/Edit/Clone/Remove/Legality/Cancel (+Review when pending); Clone and Remove execute and reparse");
        }

        // working-copy + review on one nickname edit
        {
            Checks c;
            c.expect(editor->stageEdit(0, 0, [] { G1::BoxPokemonEdit ed; ed.nickname = "QA"; return ed; }(), error),
                     "stage nickname: " + error);
            c.expect(Bytes(editor->originalBytes().begin(), editor->originalBytes().end()) == original,
                     "originalBytes no longer equal the source");
            c.expect(Bytes(editor->stagedBytes().begin(), editor->stagedBytes().end()) != original,
                     "staged copy does not carry the edit");
            c.expect(qacontract::fileState(source) == sourceBefore, "source file changed while staging");
            row.check("working-copy", c.ok(), c.text(), "original bytes untouched; staged copy carries the edit; source file unchanged");

            Checks r;
            const auto& changes = editor->pendingChanges();
            const bool listed = std::any_of(changes.begin(), changes.end(), [](const auto& ch) {
                return ch.afterValue.find("QA") != std::string::npos && ch.beforeValue.find("CHAR") != std::string::npos;
            });
            r.expect(listed, "nickname change CHAR -> QA not listed (" + std::to_string(changes.size()) + " change(s))");
            auto probe = G1::StagedPokemonEditor::create(save, error);
            probe->stageEdit(0, 0, [] { G1::BoxPokemonEdit ed; ed.nickname = "QA"; return ed; }(), error);
            r.expect(probe->revertPokemon(0, 0, error) && !probe->hasPendingChanges(), "revert did not clear the review list");
            row.check("review", r.ok(), r.text(), "pendingChanges lists 'CHAR' -> 'QA'; revert clears it");
        }

        // export: Gen1PokemonEditorOverlayUX exportStagedCopy wiring (Gen1::publishVerifiedGen1Export
        // with the trainer's staged inventory workspace), here with a Bag edit staged as well.
        stage = "export";
        {
            Checks c;
            const fs::path root = env.root / "exports";
            std::shared_ptr<G1::StagedInventoryEditor> bag = G1::StagedInventoryEditor::create(save, error);
            c.expect(bag && bag->stageSetQuantity(PokeVault::Inventory::ClassicPocket::Medicines, 20, 9, error),
                     "bag staging for export: " + error);
            G1::Gen1ExportRequest request;
            request.rootDirectory = root.string();
            request.directoryStem = spec.gameId + "_" + kTimestamp;
            request.sourcePath = source.string();
            request.timestamp = kTimestamp;
            const auto lines = G1::gen1ExportChangeLines(*editor, bag.get());
            c.expect(lines.size() >= 2, "gen1ExportChangeLines lists " + std::to_string(lines.size()) + " line(s)");
            const auto first = G1::publishVerifiedGen1Export(*editor, request, bag.get());
            c.expect(first.success, "export failed: " + first.error);
            if (first.success) {
                const fs::path dir(first.directory);
                c.expect(dir.parent_path() == root, "export left the export root");
                c.expect(qacontract::slurp(dir / "original_backup.srm") == original, "backup differs from the source");
                const auto out = qacontract::slurp(dir / "edited.srm");
                auto re = G1::parse(out, spec.g1);
                c.expect(static_cast<bool>(re), "edited.srm strict reparse: " + re.detail);
                if (re) {
                    c.expect(re.save->boxes()[0].slots.at(0) && re.save->boxes()[0].slots[0]->nickname == "QA",
                             "Pokemon edit missing from edited.srm");
                    const auto inv = G1::decodeInventory(out, re.save->metadata().region);
                    c.expect(inv.bag.size() == 1 && inv.bag[0].quantity == 9, "Bag edit missing from edited.srm");
                }
                // Box 0 (bank 1 at 0x4000, 0x462 bytes) + bank 1 checksums, the Bag list + main checksum.
                size_t offender = 0, changed = 0;
                const bool white = qacontract::onlyWhitelistedChanges(
                    original, out, {{g1::boxStart(0), g1::boxStart(0) + g1::kBoxSize},
                                    {0x4000 + g1::kBankBoxesSize, 0x4000 + g1::kBankBoxesSize + 1 + 6},
                                    {g1::kBag, g1::kBag + g1::kBagBytes}, {g1::kChecksum, g1::kChecksum + 1}},
                    &offender, &changed);
                c.expect(white && changed > 0, "byte " + qacontract::hex(offender) +
                                                   " changed outside box 0, bank checksums, Bag and main checksum");
                const auto second = G1::publishVerifiedGen1Export(*editor, request, bag.get());
                c.expect(second.success && second.directory != first.directory, "second export reused the first directory");
            }
            c.expect(qacontract::fileState(source) == sourceBefore, "source changed by export");
            row.check("export", c.ok(), c.text(),
                      "Gen1::publishVerifiedGen1Export (Pokemon + Bag): backup == source, edited.srm strictly "
                      "reparses with both edits, only box 0/bank checksum/Bag/main checksum bytes differ, second "
                      "export -> new directory");
            row.fact("export.output", "new-file-only");
            row.fact("export.reparse", "strict");
        }

        row.check("immutability", qacontract::fileState(source) == sourceBefore,
                  "source bytes/size/mtime changed during discovery, staging or export",
                  "source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export");

        stage = "home";
        if (instance) runHomeCells(row, spec, env, source, *instance);
        fs::remove_all(env.root);
        stage = "done";
    } catch (const std::exception& e) {
        row.finish("exception in " + stage + ": " + e.what());
        return;
    }
    row.finish("stopped after " + stage);
}

// ------------------------------------------------------------------------------------------------
// Gen II
// ------------------------------------------------------------------------------------------------
void runGen2Version(Matrix& m, const Spec& spec) {
    Row row(m, spec.version);
    std::string stage = "setup";
    const bool crystal = spec.g2 == G2::SourceGame::Crystal;
    const ::L& layout = crystal ? ::C : ::GS;
    try {
        const Env env = makeEnv();
        const fs::path source = env.saves / spec.fileName;
        const Bytes original = spec.build(15);
        qacontract::writeSource(source, original);
        const auto sourceBefore = qacontract::fileState(source);

        stage = "discovery";
        const auto instance = runSourceCells(row, spec, env, source);

        stage = "parse";
        auto parsed = G2::parse(original, spec.g2);
        if (!parsed) throw std::runtime_error("strict parse rejected the synthetic save: " + parsed.detail);
        const auto& save = *parsed.save;

        row.check("trainer-name", save.trainer().name == "ASH" && save.trainer().trainerId == 0x1234,
                  "trainer '" + save.trainer().name + "'", "ASH / TID 4660");
        if (crystal) {
            row.check("trainer-gender", save.trainer().gender.has_value() && *save.trainer().gender == 1,
                      "Crystal player gender not decoded as stored (1 = female)",
                      "Crystal stores the player gender (0x3E3D) -> female");
            row.fact("trainer.gender", "stored");
        } else if (save.trainer().gender.has_value()) {
            row.fail("trainer-gender", "Gold/Silver save reported a stored player gender");
        } else {
            row.na("trainer-gender",
                   "Gold/Silver saves store no player gender (Gen2::TrainerRecord::gender is empty; "
                   "SaveSelectScreen.cpp shows the fixed male player character)");
            row.fact("trainer.gender", "not-stored");
        }
        {
            Checks c;
            const auto& p = save.party();
            c.expect(p.size() == 1, partySummary(p.size()));
            if (!p.empty()) c.expect(p[0].species == 25 && p[0].level == 15 && p[0].nickname == "PARTY", "party[0]");
            c.expect(!Foundation::partyEditEnabled(), "party editing advertised");
            row.check("party", c.ok(), c.text(), "1 member (#25 L15 PARTY); read-only by contract");
        }
        {
            Checks c;
            c.expect(save.boxes().size() == 14, "box count " + std::to_string(save.boxes().size()));
            if (save.boxes().size() == 14) {
                const auto& b0 = save.boxes()[0].slots;
                const auto& b1 = save.boxes()[1].slots;
                c.expect(!b0.empty() && b0[0] && b0[0]->species == 25 && b0[0]->nickname == "PIKA", "box 0 slot 0");
                c.expect(!b1.empty() && b1[0] && b1[0]->species == 25 && b1[0]->nickname == "PIKA2", "box 1 slot 0");
                c.expect(save.boxes()[0].name == "A", "box 0 name '" + save.boxes()[0].name + "'");
            }
            row.check("boxes", c.ok(), c.text(), "14 boxes; boxes 0/1 hold #25 PIKA / PIKA2");
        }
        {
            const auto dex = save.dexProgress();
            row.check("pokedex", dex.seen == 2 && dex.caught == 1 && dex.total == 251,
                      "dex " + std::to_string(dex.seen) + "/" + std::to_string(dex.caught) + "/" + std::to_string(dex.total),
                      "seen 2 / caught 1 / total 251");
        }

        std::string error;
        auto editor = G2::StagedEditor::create(save, error);
        if (!editor) throw std::runtime_error("StagedEditor::create: " + error);
        const Bytes staged0(editor->stagedBytes().begin(), editor->stagedBytes().end());
        {
            const auto caps = PokeBank::UIModel::Gen2StagedAccess::capabilities(true, true);
            if (caps.canWriteOriginalSource || !caps.canEditStagedCopy)
                throw std::runtime_error("Gen2StagedAccess capabilities grant source writes");
        }

        stage = "bag";
        {
            Checks c;
            auto items = editor->inventoryEntries(G2::InventoryPocket::Items, error);
            auto balls = editor->inventoryEntries(G2::InventoryPocket::Balls, error);
            c.expect(items.size() == 1 && items[0].itemId == G2::kPotionItemId && items[0].quantity == 2, "Items pocket");
            c.expect(balls.size() == 1 && balls[0].itemId == G2::kPokeBallItemId && balls[0].quantity == 5, "Balls pocket");
            auto e = G2::StagedEditor::create(save, error);
            c.expect(e->stageInventoryQuantity(G2::InventoryPocket::Items, G2::kPotionItemId, 7, error), "stage: " + error);
            const auto out = e->finalizedBytes(error);
            auto re = G2::parse(out, spec.g2);
            c.expect(static_cast<bool>(re), "reparse: " + re.detail);
            if (re) {
                const auto inv = G2::decodeInventory(re.save->sourceBytes().first(0x8000), re.save->metadata().region,
                                                     re.save->metadata().family);
                c.expect(inv.available && inv.items.size() == 1 && inv.items[0].quantity == 7, "staged quantity not round-tripped");
            }
            row.check("bag", c.ok(), c.text(), "Potion x2 / Poke Ball x5 read; staged Potion x7 round-trips through strict reparse");
            row.fact("bag.read", "yes");
        }

        stage = "editor";
        // navigation: the Gen II workspace (Gen2UnifiedPokemonWorkspace.inc) moves focus only through
        // SharedPokemonEditor::switchPanel / moveVertical / moveRowColumn.
        {
            Checks c;
            SharedUI::Focus focus{};
            const auto gen = SharedUI::Generation::Gen2;
            for (int step = 0; step < 400; ++step) {
                switch (step % 7) {
                    case 0: focus = SharedUI::switchPanel(gen, focus, 1, crystal); break;
                    case 1: focus = SharedUI::moveVertical(gen, focus, 1, crystal); break;
                    case 2: focus = SharedUI::moveRowColumn(gen, focus, 1, crystal); break;
                    case 3: focus = SharedUI::moveVertical(gen, focus, -1, crystal); break;
                    case 4: focus = SharedUI::moveRowColumn(gen, focus, -1, crystal); break;
                    case 5: focus = SharedUI::switchPanel(gen, focus, -1, crystal); break;
                    default: focus = SharedUI::passiveViewMoveVertical(gen, focus, 1, crystal); break;
                }
                if (focus.row >= SharedUI::rowsFor(gen, focus.panel, crystal)) {
                    c.expect(false, "focus left the panel at step " + std::to_string(step));
                    break;
                }
                if (focus.panel == SharedUI::Panel::Values &&
                    !SharedUI::valueCellFocusable(gen, focus.row, focus.column, crystal)) {
                    c.expect(false, "focus on a non-focusable value cell at step " + std::to_string(step));
                    break;
                }
            }
            c.expect(SharedUI::rowsFor(gen, SharedUI::Panel::Details, crystal) == (crystal ? 13 : 9), "DETAILS rows");
            c.expect(!SharedUI::editBrowsingMutatesSource() && !SharedUI::createBrowsingMutatesStagedSave(),
                     "contract says browsing mutates");
            for (size_t box = 0; box < 14; ++box)
                for (size_t slot = 0; slot < 20; ++slot) (void)editor->boxedPokemon(box, slot, error);
            c.expect(Bytes(editor->stagedBytes().begin(), editor->stagedBytes().end()) == staged0 &&
                         !editor->hasPendingChanges(),
                     "browsing changed the staged copy");
            row.check("navigation", c.ok(), c.text(),
                      std::string("400 shared-contract focus moves stay in range (") + (crystal ? "Crystal 13" : "G/S 9") +
                          " DETAILS rows); browsing leaves the staged copy byte-identical");
            row.fact("nav.mutates", "no");
        }

        // creation through the UI-owned Gen2PokemonSession, started from the same record the UI's
        // defaultCreateRecord builds (src/UI/Gen2PokemonEditorFoundation.inc: Pikachu, fallbackLevel).
        {
            Checks c;
            auto e = G2::StagedEditor::create(save, error);
            G2::PokemonRecord rec{};
            rec.species = 25;
            rec.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::fallbackLevel;
            rec.nickname = Names::getSpeciesName(25);
            rec.originalTrainer = e->trainerName();
            rec.trainerId = e->trainerId();
            rec.friendship = 70;
            rec.dvs = {G2::StagedEditor::derivedHPDV({8, 8, 8, 8}), 8, 8, 8, 8};
            rec.moves = {84, 0, 0, 0};
            rec.pp = {G2::StagedEditor::gen2MoveBasePP(84), 0, 0, 0};
            Gen2UI::Session session;
            session.begin(rec, Gen2UI::SessionMode::Create);
            c.expect(session.setLevel(rec.level), "setLevel(default)");
            c.expect(!session.setLevel(0) && !session.setLevel(101), "level outside 1..100 accepted");
            c.expect(!session.setMove(0, 252), "move 252 accepted");
            size_t slot = 0;
            c.expect(session.add(*e, 2, slot, error), "Session::add: " + error);
            G2::BoxPokemonCreate none{};
            size_t ignored = 0;
            c.expect(!e->stageAddBoxPokemon(2, none, ignored, error), "species 0 create accepted");
            const auto out = e->finalizedBytes(error);
            auto re = G2::parse(out, spec.g2);
            c.expect(static_cast<bool>(re), "reparse: " + re.detail);
            if (re) {
                const auto& p = re.save->boxes()[2].slots.at(slot);
                c.expect(p && p->species == 25 && p->level == rec.level, "created record");
            }
            row.fact("create.default.species", "25");
            row.fact("create.default.level", std::to_string(rec.level));
            row.check("creation", c.ok(), c.text(),
                      "Gen2PokemonSession Create (UI default record #25 L" + std::to_string(rec.level) +
                          ") -> stageAddBoxPokemon -> strict reparse; species 0, level 0/101 and move 252 rejected");
        }

        // fields: every BoxPokemonEdit field (+ shiny, trainer name, money) on box 1 slot 0.
        {
            Checks c;
            auto e = G2::StagedEditor::create(save, error);
            auto stage1 = [&](const char* what, const G2::BoxPokemonEdit& ed,
                              const std::function<bool(const G2::PokemonRecord&)>& ok) {
                if (!e->stageBoxPokemonEdit(1, 0, ed, error)) {
                    c.expect(false, std::string(what) + ": " + error);
                    return;
                }
                const auto p = e->boxedPokemon(1, 0, error);
                c.expect(p && ok(*p), std::string(what) + " did not read back");
            };
            G2::BoxPokemonEdit ed;
            ed = {}; ed.species = 26;
            stage1("species", ed, [](const auto& p) { return p.species == 26; });
            ed = {}; ed.nickname = "ZAP";
            stage1("nickname", ed, [](const auto& p) { return p.nickname == "ZAP"; });
            ed = {}; ed.level = 30;
            stage1("level", ed, [](const auto& p) { return p.level == 30; });
            const uint32_t exp = e->boxedPokemon(1, 0, error)->experience + 100;
            ed = {}; ed.experience = exp;
            stage1("experience", ed, [&](const auto& p) { return p.experience == exp; });
            ed = {}; ed.heldItem = G2::kPotionItemId;
            stage1("heldItem", ed, [](const auto& p) { return p.heldItem == G2::kPotionItemId; });
            const std::array<uint8_t, 4> moves{84, 98, 0, 0};  // Thundershock, Quick Attack
            const std::array<uint8_t, 4> ups{1, 0, 0, 0};
            const std::array<uint8_t, 4> pp{G2::StagedEditor::gen2MoveMaxPP(84, 1),
                                            G2::StagedEditor::gen2MoveMaxPP(98, 0), 0, 0};
            ed = {}; ed.moves = moves; ed.ppUps = ups; ed.pp = pp;
            stage1("moves/pp/ppUps", ed, [&](const auto& p) { return p.moves == moves && p.pp == pp && p.ppUps == ups; });
            ed = {}; ed.dvs = std::array<uint8_t, 4>{10, 11, 12, 13};
            stage1("dvs", ed, [](const auto& p) { return p.dvs[1] == 10 && p.dvs[2] == 11 && p.dvs[3] == 12 && p.dvs[4] == 13; });
            ed = {}; ed.statExperience = std::array<uint16_t, 5>{100, 200, 300, 400, 500};
            stage1("statExperience", ed, [](const auto& p) {
                return p.statExperience == std::array<uint16_t, 5>{100, 200, 300, 400, 500};
            });
            ed = {}; ed.otName = "BOB";
            stage1("otName", ed, [](const auto& p) { return p.originalTrainer == "BOB"; });
            ed = {}; ed.trainerId = 4321;
            stage1("trainerId", ed, [](const auto& p) { return p.trainerId == 4321; });
            ed = {}; ed.friendship = 99;
            stage1("friendship", ed, [](const auto& p) { return p.friendship == 99; });
            ed = {}; ed.pokerus = 0x13;
            stage1("pokerus", ed, [](const auto& p) { return p.pokerus == 0x13; });
            if (crystal) {
                ed = {}; ed.caughtData = 0x0A05;
                stage1("caughtData", ed, [](const auto& p) { return p.caughtData == 0x0A05; });
            }
            c.expect(e->stageBoxPokemonShiny(1, 0, true, error), "shiny: " + error);
            const auto shiny = e->boxedPokemon(1, 0, error);
            c.expect(shiny && shiny->shiny, "shiny did not read back");
            c.expect(e->stageTrainerName("BOB", error) && e->trainerName() == "BOB", "trainer name: " + error);
            c.expect(e->stageMoney(54321, error) && e->money() == 54321, "money: " + error);
            const auto expected = e->boxedPokemon(1, 0, error);
            const auto out = e->finalizedBytes(error);
            auto re = G2::parse(out, spec.g2);
            c.expect(static_cast<bool>(re), "reparse: " + re.detail);
            if (re && expected) {
                const auto& got = re.save->boxes()[1].slots.at(0);
                c.expect(got && Gen2UI::sameEditableRecord(*got, *expected) && got->shiny,
                         "finalized record differs from the staged record");
                c.expect(re.save->trainer().name == "BOB" && re.save->trainer().money == 54321, "trainer name/money");
            }
            row.check("fields", c.ok(), c.text(),
                      std::string("species, nickname, level, EXP, held item, moves+PP+PP Ups, DVs, Stat Exp, OT, TID, "
                                  "friendship, Pokerus") + (crystal ? ", caught data" : "") +
                          ", shiny, trainer name, money stage, read back and survive finalize + strict reparse");
        }

        // pickers
        {
            Checks c;
            for (uint16_t s = 1; s <= 251; ++s)
                if (!G2::personalRecord(s) || !Names::getSpeciesName(s)) {
                    c.expect(false, "no personal data/name for #" + std::to_string(s));
                    break;
                }
            c.expect(G2::personalRecord(252) == nullptr, "#252 offered");
            const auto moves = G2::MoveCompatibility::compatibleMoves(spec.g2, 25);
            c.expect(std::find(moves.begin(), moves.end(), 84) != moves.end(), "Thundershock not offered for Pikachu");
            c.expect(std::all_of(moves.begin(), moves.end(), [](uint8_t mv) { return mv <= G2::kMaxGen2Move; }),
                     "move outside 1..251 offered");
            c.expect(PokeBank::UIModel::MovePickerPresentation::numberedName(84).rfind("084 - ", 0) == 0, "move row text");
            row.check("pickers", c.ok(), c.text(),
                      "251 species with personal data; Pikachu move list (" + std::to_string(moves.size()) +
                          " moves) includes Thundershock, all within 1..251");
        }

        // actions: ClassicReleaseActionFix.inc builds SharedPokemonEditor::actionsForSlot with
        // releaseActionCapabilities() {clone, remove, legality}; each offered action must be backed.
        {
            Checks c;
            SharedUI::ActionCapabilities caps;
            caps.canClone = true;
            caps.canRemove = true;
            caps.hasLegalityProvenance = true;
            auto e = G2::StagedEditor::create(save, error);
            const auto occupied = SharedUI::actionsForSlot(true, caps, false);
            const auto empty = SharedUI::actionsForSlot(false, caps, false);
            c.expect(occupied.count == 6 && occupied[2] == SharedUI::Action::Clone && occupied[3] == SharedUI::Action::Remove,
                     "occupied-slot actions");
            c.expect(empty[0] == SharedUI::Action::Add, "empty-slot actions");
            size_t dest = 0;
            c.expect(e->stageCloneBoxPokemon(1, 0, 3, dest, error), "Clone: " + error);
            const auto pending = SharedUI::actionsForSlot(true, caps, e->hasPendingChanges());
            c.expect(pending[5] == SharedUI::Action::Review, "Review not offered with pending changes");
            c.expect(e->stageReleaseBoxPokemon(1, 0, error), "Release: " + error);
            const auto out = e->finalizedBytes(error);
            c.expect(static_cast<bool>(G2::parse(out, spec.g2)), "clone+release result failed strict reparse: " + error);
            row.check("actions", c.ok(), c.text(),
                      "View/Edit/Clone/Release/Legality/Cancel (+Review when pending); Clone and Release execute and reparse");
        }

        // working-copy + review on one nickname edit (box 1: outside the mirrored main block)
        {
            Checks c;
            G2::BoxPokemonEdit ed;
            ed.nickname = "QA";
            c.expect(editor->stageBoxPokemonEdit(1, 0, ed, error), "stage nickname: " + error);
            c.expect(Bytes(editor->originalBytes().begin(), editor->originalBytes().end()) == original,
                     "originalBytes no longer equal the source");
            c.expect(Bytes(editor->stagedBytes().begin(), editor->stagedBytes().end()) != original,
                     "staged copy does not carry the edit");
            c.expect(qacontract::fileState(source) == sourceBefore, "source file changed while staging");
            row.check("working-copy", c.ok(), c.text(), "original bytes untouched; staged copy carries the edit; source file unchanged");

            Checks r;
            const auto& changes = editor->pendingChanges();
            const bool listed = std::any_of(changes.begin(), changes.end(), [](const auto& ch) {
                return ch.afterValue.find("QA") != std::string::npos && ch.beforeValue.find("PIKA2") != std::string::npos;
            });
            r.expect(listed, "nickname change PIKA2 -> QA not listed (" + std::to_string(changes.size()) + " change(s))");
            auto probe = G2::StagedEditor::create(save, error);
            probe->stageBoxPokemonEdit(1, 0, ed, error);
            probe->discard();
            r.expect(!probe->hasPendingChanges(), "discard did not clear the review list");
            row.check("review", r.ok(), r.text(), "pendingChanges lists 'PIKA2' -> 'QA'; discard clears it");
        }

        // export: TrainerViewScreenGSCOverlay wiring (publishVerifiedStagedEditorExport).
        stage = "export";
        {
            Checks c;
            const fs::path root = env.root / "exports";
            G2::StagedEditorExportRequest request;
            request.rootDirectory = root.string();
            request.directoryStem = spec.gameId + "_" + kTimestamp;
            request.sourcePath = source.string();
            request.gameId = spec.gameId;
            request.timestamp = kTimestamp;
            const auto first = G2::publishVerifiedStagedEditorExport(*editor, request);
            c.expect(first.success, "export failed: " + first.error);
            if (first.success) {
                const fs::path dir(first.directory);
                c.expect(dir.parent_path() == root, "export left the export root");
                c.expect(qacontract::slurp(dir / "original_backup.srm") == original, "backup differs from the source");
                const auto out = qacontract::slurp(dir / "edited.srm");
                auto re = G2::parse(out, spec.g2);
                c.expect(static_cast<bool>(re), "edited.srm strict reparse: " + re.detail);
                if (re) c.expect(re.save->boxes()[1].slots.at(0) && re.save->boxes()[1].slots[0]->nickname == "QA",
                                 "edit missing from edited.srm");
                // Box 1 lives in the 0x4000 bank (not in a checksummed/mirrored block); both checksum
                // words are allowed (gsc_pokemon_fixture.h layout).
                size_t offender = 0, changed = 0;
                const bool white = qacontract::onlyWhitelistedChanges(
                    original, out, {{::boxStart(1), ::boxStart(1) + ::stride()},
                                    {layout.sum, layout.sum + 2}, {layout.sum2, layout.sum2 + 2}},
                    &offender, &changed);
                c.expect(white && changed > 0, "byte " + qacontract::hex(offender) + " changed outside box 1 + checksums");
                const auto second = G2::publishVerifiedStagedEditorExport(*editor, request);
                c.expect(second.success && second.directory != first.directory, "second export reused the first directory");
            }
            c.expect(qacontract::fileState(source) == sourceBefore, "source changed by export");
            row.check("export", c.ok(), c.text(),
                      "publishVerifiedStagedEditorExport: backup == source, edited.srm strictly reparses with the "
                      "edit, only box 1 bytes differ, second export -> new directory");
            row.fact("export.output", "new-file-only");
            row.fact("export.reparse", "strict");
        }

        row.check("immutability", qacontract::fileState(source) == sourceBefore,
                  "source bytes/size/mtime changed during discovery, staging or export",
                  "source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export");

        stage = "home";
        if (instance) runHomeCells(row, spec, env, source, *instance);
        fs::remove_all(env.root);
        stage = "done";
    } catch (const std::exception& e) {
        row.finish("exception in " + stage + ": " + e.what());
        return;
    }
    row.finish("stopped after " + stage);
}

} // namespace

void runGen12(Matrix& m) {
    auto gen1 = [](std::string version, G1::SourceGame game, std::string file, std::string wrong) {
        Spec s;
        s.version = std::move(version);
        s.generation = 1;
        s.g1 = game;
        s.gameId = G1::sourceGameId(game);
        s.fileName = std::move(file);
        s.wrongHintName = std::move(wrong);
        const bool yellow = game == G1::SourceGame::Yellow;
        s.build = [yellow](uint8_t level) { return g1::save(yellow, level); };
        return s;
    };
    auto gen2 = [](std::string version, G2::SourceGame game, std::string file, std::string wrong, bool structural) {
        Spec s;
        s.version = std::move(version);
        s.generation = 2;
        s.g2 = game;
        s.gameId = G2::sourceGameId(game);
        s.fileName = std::move(file);
        s.wrongHintName = std::move(wrong);
        s.structuralIdentity = structural;
        const bool crystal = game == G2::SourceGame::Crystal;
        s.build = [crystal](uint8_t level) { return g2::save(crystal ? ::C : ::GS, level); };
        return s;
    };
    runGen1Version(m, gen1("red", G1::SourceGame::Red, "Pokemon Red.srm", "Pokemon Blue.srm"));
    runGen1Version(m, gen1("blue", G1::SourceGame::Blue, "Pokemon Blue.srm", "Pokemon Red.srm"));
    runGen1Version(m, gen1("yellow", G1::SourceGame::Yellow, "Pokemon Yellow.srm", "Pokemon Red.srm"));
    runGen2Version(m, gen2("gold", G2::SourceGame::Gold, "Pokemon Gold.srm", "Pokemon Crystal.srm", false));
    runGen2Version(m, gen2("silver", G2::SourceGame::Silver, "Pokemon Silver.srm", "Pokemon Crystal.srm", false));
    // Crystal is linked under a neutral name: discovery identifies it from the save structure.
    runGen2Version(m, gen2("crystal", G2::SourceGame::Crystal, "slot3.srm", "Pokemon Gold.srm", true));
}

} // namespace qamatrix
