// Gen VII through the shared Modern::NativeSession contract: the adapter must present and stage
// exactly what Gen7StagedSession verifies, refuse what it cannot verify (with a reason), keep the
// source bytes immutable and, because SAV7 re-signing is unproven, refuse Export Copy without
// writing a single file. Check-for-check mirror of tests/test_gen6_native_session.cpp plus the
// Gen VII-specific behaviours (blocked export, seven pockets, USUM Roto Powers, hyper training).
#include "Integration/Gen7/Gen7NativeSession.h"
#include "Integration/Gen7/Gen7Bag.h"
#include "Pokemon/ModernRecordView.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace G7 = PokeVault::Integration::Gen7;
namespace Modern = PokeVault::Integration::Modern;
namespace Capability = PokeVault::Integration::Capability;
namespace Layout = PokeVault::Integration::Gen7::Layout;
namespace PK7 = G7::PK7;
using Modern::SlotRef;
using Field = Modern::Field;
using State = Modern::FieldState;
using G7::BagPocket7;

// Vendored FlagBrew/memecrypto@bd3b16ee rsa.c never mpz_clear()s its temporaries (docs/gen7/README.md).
extern "C" const char* __lsan_default_suppressions() {
    return "leak:rsa_encrypt\nleak:rsa_decrypt\n";
}

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
std::string makeTempDir() {
    char pattern[] = "/tmp/gen7-native-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
// Entries other than "." and ".."; SIZE_MAX when the directory cannot be opened.
size_t directoryEntries(const std::string& dir) {
    DIR* d = ::opendir(dir.c_str());
    if (!d) return static_cast<size_t>(-1);
    size_t n = 0;
    while (const dirent* e = ::readdir(d)) {
        const std::string name = e->d_name;
        if (name != "." && name != "..") ++n;
    }
    ::closedir(d);
    return n;
}
constexpr size_t P(BagPocket7 p) { return static_cast<size_t>(p); }

// Fixture: party of 2 (Rowlet 722, Dartrix 723), Box 1 slots 1-3 (species 100, 101 and an Alolan
// Exeggutor 103 form 1), Box 32 slot 30 (Tapu Koko 785), every other box slot a verified blank, and a
// native bag: Items {Master Ball x3, Poke Ball x10}, Medicine {Potion x2}, one Key Item, one Z-Crystal
// (Normalium Z 807) and, for USUM only, one Roto Power (949).
std::vector<uint8_t> saveWithBag(Layout::Family f, uint8_t version) {
    auto save = Gen7Fixture::makeSave({f, version, 2, false, false});
    for (size_t s = 0; s < 3; ++s) {
        Gen6Fixture::MonSpec m;
        m.species = s == 2 ? 103 : static_cast<uint16_t>(100 + s);
        m.form = s == 2 ? 1 : 0;
        m.ec = 0xA0000000u + static_cast<uint32_t>(s) * 0x01010101u;
        m.pid = 0x0B000000u + static_cast<uint32_t>(s) * 0x00FF00FFu;
        m.version = version;
        Gen6Fixture::put(save, G7::Gen7Save::boxRecordOffset(f, 0, s), Gen6Fixture::encryptedMon(m, false));
    }
    const auto block = Layout::blocks(f)[G7::ItemsBlock7];
    std::fill_n(save.begin() + block.offset, block.length, uint8_t{0});
    const auto put = [&](size_t pocket, size_t slot, uint32_t value) {
        PK7::write32(save, block.offset + G7::bagPocketLayout7(f, pocket).offset + slot * 4, value);
    };
    put(P(BagPocket7::Items), 0, 1u | (3u << 10));
    put(P(BagPocket7::Items), 1, 4u | (10u << 10));
    put(P(BagPocket7::Medicine), 0, 17u | (2u << 10));
    put(P(BagPocket7::KeyItems), 0, 465u | (1u << 10));
    put(P(BagPocket7::ZCrystals), 0, 807u | (1u << 10));
    if (f == Layout::Family::UltraSunUltraMoon) put(P(BagPocket7::BattleItems), 0, 949u | (1u << 10));
    Gen7Fixture::resignBlocks(save, f);
    Gen7Fixture::signMeme(save, f);
    return save;
}

bool hasFact(const std::vector<Modern::TrainerFact>& facts, const std::string& label, const std::string& value) {
    for (const auto& f : facts) if (f.label == label && f.value == value) return true;
    return false;
}
bool hasNativeFact(const Modern::RecordView& v, const std::string& label) {
    for (const auto& f : v.nativeFacts) if (f.first == label) return true;
    return false;
}

// Battle Box fixture (PKSE 1.2.2 audit: Gen VII had no team guard). Box 1 slots 1-3 hold species
// 100-102; Box 32 slot 30 holds Tapu Koko; the team table comes from `teams` (PKHeX BoxLayout7 layout).
std::vector<uint8_t> saveWithTeams(Layout::Family f, uint8_t version, const Gen7Fixture::TeamSpec& teams,
                                   bool sunMoonDemo = false) {
    auto save = Gen7Fixture::makeSave({f, version, 2, false, false}, teams);
    for (size_t s = 0; s < 3; ++s) {
        Gen6Fixture::MonSpec m;
        m.species = static_cast<uint16_t>(100 + s);
        m.ec = 0xB0000000u + static_cast<uint32_t>(s) * 0x01010101u;
        m.pid = 0x0C000000u + static_cast<uint32_t>(s) * 0x00FF00FFu;
        m.version = version;
        Gen6Fixture::put(save, G7::Gen7Save::boxRecordOffset(f, 0, s), Gen6Fixture::encryptedMon(m, false));
    }
    if (sunMoonDemo) {   // PKHeX SAV7.ReloadBattleTeams: BoxLayout [0, 0x4C4) all zero = demo, no teams
        const size_t layout = Layout::blocks(f)[13].offset;
        std::fill_n(save.begin() + static_cast<std::ptrdiff_t>(layout), 0x4C4 + 72, uint8_t{0});
    }
    Gen7Fixture::resignBlocks(save, f);
    Gen7Fixture::signMeme(save, f);
    return save;
}

void battleBoxGuard() {
    using G7::Gen7NativeSession;
    std::string error;
    // Team 1 (unlocked): Box 1 slot 1 and an EMPTY Box 1 slot 6; team 2 (locked): Box 1 slot 2.
    Gen7Fixture::TeamSpec teams;
    teams.members = {{0, 0, 0, false}, {0, 0, 5, false}, {1, 0, 1, true}};
    const auto bytes = saveWithTeams(Layout::Family::SunMoon, Layout::VersionSun, teams);
    auto opened = Gen7NativeSession::open(bytes, "sun_3ds");
    check(opened.ready(), "Battle Box fixture opens: " + opened.diagnostic);
    if (!opened.ready()) return;
    auto& s = *opened.session;
    const auto inTeam = SlotRef::boxed(0, 0), locked = SlotRef::boxed(0, 1), free = SlotRef::boxed(0, 2);
    const auto teamBlank = SlotRef::boxed(0, 5);
    check(!s.moveSupport(inTeam).available && std::string(s.moveSupport(inTeam).reason).find("Battle Box") != std::string::npos,
          "a Battle Box member is never moved");
    check(!s.removeSupport(inTeam).available, "a Battle Box member is never released");
    check(!s.remove(inTeam, error) && error.find("Battle Box") != std::string::npos, "remove() itself refuses: " + error);
    const auto dest = s.firstEmptyBoxSlot();
    check(dest && *dest != teamBlank, "the first free slot is never a team-referenced blank");
    check(!s.emptyDestination(teamBlank), "a team-referenced empty slot is not a clone/move destination");
    check(dest && !s.move(inTeam, *dest, error), "move() itself refuses a team member");
    check(s.stageText(inTeam, Field::Nickname, "TEAM", error), "an UNLOCKED team member stays editable: " + error);
    check(!s.stageText(locked, Field::Nickname, "LOCK", error) && error.find("locked") != std::string::npos,
          "a LOCKED team member is never edited: " + error);
    check(!s.beginDraft(locked, error) && error.find("locked") != std::string::npos, "no editor draft on a locked member");
    check(!s.moveSupport(locked).available && !s.removeSupport(locked).available, "a locked member is never moved/released");
    check(s.moveSupport(free).available && s.removeSupport(free).available, "a slot outside every team moves/releases normally");
    check(dest && s.move(free, *dest, error), "a free box Pokemon moves: " + error);
    check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), bytes.begin(), bytes.end()), "Battle Box: source immutable");

    // A pointer outside 32 x 30 makes the table unreadable: every box storage change is refused.
    Gen7Fixture::TeamSpec bad;
    bad.rawPointer = true;
    bad.rawValue = static_cast<uint16_t>((40u << 8) | 3u);
    auto broken = Gen7NativeSession::open(saveWithTeams(Layout::Family::SunMoon, Layout::VersionSun, bad), "sun_3ds");
    check(broken.ready(), "a save with a malformed team pointer still opens read-only");
    if (broken.ready()) {
        auto& b = *broken.session;
        check(!b.moveSupport(free).available && std::string(b.moveSupport(free).reason).find("could not be read") != std::string::npos,
              "unreadable teams refuse every box Move");
        check(!b.removeSupport(free).available && !b.firstEmptyBoxSlot(), "unreadable teams refuse Release and every destination");
        check(!b.stageText(free, Field::Nickname, "NOPE", error), "unreadable teams refuse box edits");
        check(b.stageText(SlotRef::party(0), Field::Nickname, "PARTY", error), "party edits are unaffected: " + error);
    }

    // Ultra Sun: same table in the USUM BoxLayout block.
    Gen7Fixture::TeamSpec usumTeams;
    usumTeams.members = {{3, 31, 29, true}};
    auto us = Gen7NativeSession::open(saveWithTeams(Layout::Family::UltraSunUltraMoon, Layout::VersionUltraSun, usumTeams),
                                      "ultrasun_3ds");
    check(us.ready(), "USUM Battle Box fixture opens: " + us.diagnostic);
    if (us.ready()) {
        check(!us.session->moveSupport(SlotRef::boxed(31, 29)).available, "USUM locked team member never moved");
        check(!us.session->stageText(SlotRef::boxed(31, 29), Field::Nickname, "KOKO", error), "USUM locked member never edited");
        check(us.session->moveSupport(SlotRef::boxed(0, 0)).available, "USUM non-member moves");
    }

    // Sun/Moon demo save (PKHeX: BoxLayout bytes before 0x4C4 all zero): zero pointers are NOT box 1 slot 1.
    Gen7Fixture::TeamSpec zeros;
    zeros.rawPointer = true;
    zeros.rawValue = 0;
    auto demo = Gen7NativeSession::open(saveWithTeams(Layout::Family::SunMoon, Layout::VersionSun, zeros, true), "sun_3ds");
    check(demo.ready() && demo.session->moveSupport(SlotRef::boxed(0, 0)).available,
          "a Sun/Moon demo save has no Battle Box teams (PKHeX ReloadBattleTeams)");
}

// Hyper Training (PKSE 1.2.2 port; rules from PKHeX 6501f0ab IHyperTrain + HyperTrainingVerifier). Fixture
// IVs in storage order HP 31, Atk 16, Def 15, Spe 31, SpA 2, SpD 30; flag bits HP0 Atk1 Def2 SpA3 SpD4 Spe5.
void hyperTraining() {
    using G7::Gen7NativeSession;
    std::string error;
    const auto bytes = saveWithBag(Layout::Family::SunMoon, Layout::VersionSun);
    auto opened = Gen7NativeSession::open(bytes, "sun_3ds");
    check(opened.ready(), "HT fixture opens");
    if (!opened.ready()) return;
    auto& s = *opened.session;
    const auto box = SlotRef::boxed(0, 0), party = SlotRef::party(0);
    check(s.fieldState(Field::HyperTraining, Modern::Region::Box) == State::Editable &&
              s.fieldState(Field::HyperTraining, Modern::Region::Party) == State::Editable,
          "Hyper Training is editable for SM/USUM box and party records");
    check(s.view(box)->hyperTrainFlags == 0, "fixture starts untrained");
    check(!s.stageValue(box, Field::HyperTraining, 0x02, 0, error) && error.find("level 100") != std::string::npos,
          "below level 100 Hyper Training is refused: " + error);
    check(s.stageValue(box, Field::Level, 100, 0, error), "stage level 100: " + error);
    const auto before = s.view(box)->stats;
    check(s.stageValue(box, Field::HyperTraining, 0x02, 0, error), "train Atk at level 100: " + error);
    auto v = s.view(box);
    check(v && v->hyperTrainFlags == 0x02, "the Atk flag (bit 1) is stored");
    check(v && v->stats[1] > before[1] && v->stats[0] == before[0] && v->stats[2] == before[2],
          "only the trained stat rises (computed with IV 31)");
    check(v && v->ivs[1] == 16, "the stored IV itself is unchanged");
    check(!s.stageValue(box, Field::HyperTraining, 0x03, 0, error) && error.find("perfect HP") != std::string::npos,
          "a perfect HP IV is never hyper trained: " + error);
    check(!s.stageValue(box, Field::HyperTraining, 0x22, 0, error) && error.find("perfect Spe") != std::string::npos,
          "bit 5 is Speed (storage stat 3) and its 31 IV is refused: " + error);
    check(!s.stageValue(box, Field::HyperTraining, 0x42, 0, error), "flags beyond six bits are refused");
    bool reviewed = false;
    for (const auto& r : s.reviewRows()) reviewed |= r.value.find("Hyper Training -> Atk") != std::string::npos ||
                                                     r.label.find("Hyper Training -> Atk") != std::string::npos;
    check(reviewed, "Review lists the Hyper Training change");
    const auto trainedAtk = s.view(box)->stats[1];
    check(s.stageValue(box, Field::IV, 31, 1, error) && s.view(box)->hyperTrainFlags == 0,
          "raising a trained IV to 31 clears its flag: " + error);
    check(s.view(box)->stats[1] == trainedAtk, "IV 31 untrained equals the trained stat");
    check(s.stageValue(box, Field::HyperTraining, 0x08, 0, error) && s.view(box)->hyperTrainFlags == 0x08,
          "train SpA (bit 3, IV 2): " + error);
    check(s.stageValue(box, Field::HyperTraining, 0, 0, error) && s.view(box)->hyperTrainFlags == 0,
          "clearing is always allowed: " + error);

    // Party: cached stats are recomputed hyper-training aware.
    check(s.stageValue(party, Field::Level, 100, 0, error), "party level 100: " + error);
    const auto partyBefore = s.view(party)->stats;
    check(s.stageValue(party, Field::HyperTraining, 0x04, 0, error), "train party Def: " + error);
    check(s.view(party)->stats[2] > partyBefore[2] && s.view(party)->stats[1] == partyBefore[1],
          "party Def stat recomputed with IV 31, others unchanged");
    check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), bytes.begin(), bytes.end()), "HT: source immutable");
    s.discardAll();
    check(s.view(box)->hyperTrainFlags == 0 && !s.hasChanges(), "Discard All restores the flags");
}
} // namespace

int main() {
    const auto original = saveWithBag(Layout::Family::SunMoon, Layout::VersionSun);
    const auto pristine = original;
    const auto* sunRow = Capability::find("sun_3ds");
    check(sunRow != nullptr, "registry has the Sun row");
    if (!sunRow) return EXIT_FAILURE;

    // ---- open: exact-title assignment is authoritative; the MemeCrypto signature is required ----
    check(!G7::Gen7NativeSession::open(original, "moon_3ds").ready(), "a Sun save refuses a Moon assignment");
    check(!G7::Gen7NativeSession::open(original, "moon_3ds").diagnostic.empty(), "mismatch names why");
    check(!G7::Gen7NativeSession::open(original, "ultrasun_3ds").ready(), "a Sun save refuses an Ultra Sun assignment");
    {
        const auto unsignedSave = Gen7Fixture::makeSave({Layout::Family::SunMoon, Layout::VersionSun, 2, true, false});
        const auto refused = G7::Gen7NativeSession::open(unsignedSave, "sun_3ds");
        check(!refused.ready() && refused.diagnostic.find("signature") != std::string::npos,
              "a CRC-valid but unsigned save is refused with the signature diagnostic: " + refused.diagnostic);
    }
    auto opened = G7::Gen7NativeSession::open(original, "sun_3ds");
    check(opened.ready(), "a valid Sun save opens: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;
    auto& s = *opened.session;
    auto* g7 = dynamic_cast<G7::Gen7NativeSession*>(opened.session.get());
    check(g7 != nullptr, "the opened session is the Gen VII adapter");
    if (!g7) return EXIT_FAILURE;
    check(g7->gameGroup() == Enums::GameVersion::SM, "Sun/Moon game group");
    check(g7->source().signatureVerified(), "the source signature was verified on open");
    check(s.exactGameId() == "sun_3ds", "exact id");
    check(&s.capabilities() == sunRow && s.capabilities().generation == 7, "registry row attached");
    check(s.boxCount() == 32 && s.slotsPerBox() == 30 && s.partyCount() == 2, "native geometry 32x30, party 2");
    check(s.familyLabel() == "Generation VII", "family label");
    check(!s.boxName(0).empty() && !s.boxName(31).empty(), "every box has a presentable name (native or Box N fallback)");

    // ---- record views ----
    const auto party0 = s.view(SlotRef::party(0));
    check(party0 && party0->valid && !party0->empty && party0->partyRecord, "party 0 is a valid party record");
    check(party0 && party0->species == 722 && party0->level == 12, "party 0 species/level from the native record");
    check(party0 && party0->currentHp == 20 && party0->stats[0] == 21 && party0->stats[5] == 26, "party cached stats");
    check(party0 && party0->nickname == "SYNTH" && party0->otName == "TESTER", "UTF-16 names decoded");
    check(party0 && party0->tid == 12345 && party0->sid == 54321 && party0->heldItem == 1, "ids and held item");
    check(party0 && party0->ivs[0] == 31 && party0->evs[0] == 4 && party0->moves[0] == 33 && party0->pp[0] == 35, "IV/EV/move/PP");
    check(party0 && party0->metLocation == 6 && party0->ball == 4 && party0->metLevel == 5 && party0->language == 2, "met data");
    check(party0 && party0->metDate.year == 2014 && party0->metDate.month == 1 && party0->metDate.day == 2, "met date");
    check(party0 && party0->originGame == Layout::VersionSun, "origin version");
    check(party0 && !party0->nativeFacts.empty(), "native facts carried for the shared editor");
    check(party0 && hasNativeFact(*party0, "Hyper training") && hasNativeFact(*party0, "Current handler"),
          "Gen VII native facts include Hyper Training (PK7 0xDE) and the handler");
    const auto box2 = s.view(SlotRef::boxed(0, 2));
    check(box2 && box2->valid && !box2->empty && !box2->partyRecord, "box 0/2 is a stored record");
    check(box2 && box2->species == 103 && box2->form == 1, "Alolan Exeggutor form 1 kept");
    check(box2 && box2->level >= 1 && box2->stats[0] > 0 && box2->baseStats[0] > 0, "boxed stats computed from USUM personal data");
    const auto tapu = s.view(SlotRef::boxed(31, 29));
    check(tapu && tapu->valid && tapu->species == 785 && !tapu->partyRecord && tapu->level >= 1, "Box 32 slot 30 Tapu Koko");
    const auto blank = s.view(SlotRef::boxed(5, 5));
    check(blank && blank->valid && blank->empty, "blank slot is valid and empty");
    check(!s.view(SlotRef::boxed(32, 0)).has_value() && !s.view(SlotRef::party(6)).has_value(), "non-canonical slots have no view");
    check(s.originalOccupied(SlotRef::boxed(0, 0)) && !s.originalOccupied(SlotRef::boxed(5, 5)), "originalOccupied");
    check(s.emptyDestination(SlotRef::boxed(5, 5)) && !s.emptyDestination(SlotRef::party(5)), "emptyDestination is box-only");
    check(!s.emptyDestination(SlotRef::boxed(31, 29)), "an occupied slot is never an empty destination");
    check(s.firstEmptyBoxSlot() == SlotRef::boxed(0, 3), "first blank box slot is Box 1 slot 4");

    // ModernRecordView presents the same data to the shared panels.
    {
        Pokemon::ModernRecordView pk(*party0, Enums::GameVersion::SM);
        check(pk.speciesID() == 722 && pk.level() == 12 && pk.statHPMax() == 21 && pk.statHPCurrent() == 20, "presentation Pokemon mirrors the view");
        check(pk.ivHP() == 31 && pk.move(0) == 33 && pk.getGameGroup() == Enums::GameVersion::SM, "presentation IV/move/group");
        check(Utils::utf16ToUtf8(pk.nickname()) == "SYNTH", "presentation nickname");
    }

    // ---- field model ----
    check(s.fieldState(Field::Nickname, Modern::Region::Box) == State::Editable, "Nickname editable");
    check(s.fieldState(Field::Species, Modern::Region::Box) == State::ReadOnly, "Species read-only (no verified species writer)");
    check(s.fieldState(Field::Move, Modern::Region::Box) == State::ReadOnly, "Moves read-only (no pinned SM learnset)");
    check(s.fieldState(Field::DV, Modern::Region::Box) == State::Hidden, "DV hidden");
    check(s.fieldState(Field::AV, Modern::Region::Box) == State::Hidden, "Awakening Values hidden (Let's Go is another family)");
    check(s.fieldState(Field::CalculatedStats, Modern::Region::Party) == State::Derived &&
              s.fieldState(Field::CalculatedStats, Modern::Region::Box) == State::Hidden,
          "calculated stats derived on party, hidden on box");
    check(s.fieldMaximum(Field::HeldItem) == 920 && s.fieldMaximum(Field::EV) == 252 && s.fieldMaximum(Field::Ball) == 26, "SM limits");
    {
        const auto items = s.heldItemChoices();
        check(items.size() > 100 && items.front() == 0, "held item choices come from the SM catalog");
        check(std::all_of(items.begin(), items.end(), [](uint16_t id) { return id <= 920; }), "every held item choice is within the SM range");
    }
    check(s.ballChoices().size() == 26 && s.ballChoices().back() == 26, "26 balls up to Beast Ball");
    check(s.maxPP(33, 0) == 0, "no PP table: unknown");

    // ---- staging ----
    std::string error;
    check(s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 5, 0, error), "stage held item: " + error);
    check(s.view(SlotRef::boxed(0, 0))->heldItem == 5, "staged view shows the new held item");
    check(s.stagedAt(SlotRef::boxed(0, 0)) && !s.stagedAt(SlotRef::boxed(0, 1)), "stagedAt");
    check(s.hasChanges() && s.changeCount() == 1, "one staged change");
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::Species, 150, 0, error) && !error.empty(), "species refused with reason");
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 921, 0, error), "held item beyond SM range refused");
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::Ball, 27, 0, error), "ball beyond Beast Ball refused");
    check(s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "NEW", error), "stage nickname: " + error);
    check(s.view(SlotRef::boxed(0, 0))->nickname == "NEW", "nickname staged");
    check(!s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "Pok\xC3\xA9", error), "non-ASCII nickname refused");
    check(!s.stageValue(SlotRef::boxed(5, 5), Field::HeldItem, 5, 0, error) && !error.empty(), "editing a blank slot refused");
    check(s.stageValue(SlotRef::party(0), Field::IV, 7, 0, error), "party IV stages: " + error);
    check(s.view(SlotRef::party(0))->ivs[0] == 7 && s.view(SlotRef::party(0))->stats[0] != 21, "party stats recomputed after IV edit");
    const auto rows = s.reviewRows();
    check(rows.size() >= 3, "review rows list every staged field");
    bool sawHeld = false;
    for (const auto& r : rows) if (r.label == "Box 1 / Slot 1" && r.value.find("held item") != std::string::npos) sawHeld = true;
    check(sawHeld, "review names the held item change at its slot");

    // ---- clone / move / remove / create ----
    check(s.cloneSupport(SlotRef::boxed(0, 1)).available, "clone available for an unmodified occupied box record");
    check(!s.cloneSupport(SlotRef::boxed(0, 0)).available, "clone refused for a staged source");
    check(s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(0, 3), error), "clone to Box 1 slot 4: " + error);
    check(s.view(SlotRef::boxed(0, 3))->species == 101, "clone landed");
    check(!s.clone(SlotRef::boxed(0, 2), SlotRef::boxed(0, 3), error), "clone onto a staged destination refused");
    check(!s.clone(SlotRef::boxed(0, 2), SlotRef::party(5), error) && !error.empty(), "clone into the party refused");
    check(s.cloneSupport(SlotRef::party(1)).available && s.clone(SlotRef::party(1), SlotRef::boxed(0, 4), error),
          "party record clones into a box as its stored form: " + error);
    check(s.view(SlotRef::boxed(0, 4))->species == 723 && !s.view(SlotRef::boxed(0, 4))->partyRecord, "party clone is a boxed record");
    check(!s.moveSupport(SlotRef::party(1)).available && !s.moveSupport(SlotRef::party(1)).reason.empty(), "party Move blocked with reason");
    check(s.moveSupport(SlotRef::boxed(0, 1)).available, "box Move available");
    check(s.move(SlotRef::boxed(0, 1), SlotRef::boxed(1, 0), error), "move Box 1/2 -> Box 2/1: " + error);
    check(s.view(SlotRef::boxed(1, 0))->species == 101 && s.view(SlotRef::boxed(0, 1))->empty, "move = clone + remove");
    check(!s.move(SlotRef::party(0), SlotRef::boxed(1, 1), error) && !error.empty(), "party Move refused with the reason");
    check(!s.removeSupport(SlotRef::party(0)).available, "party Remove blocked");
    check(!s.remove(SlotRef::party(0), error) && !error.empty(), "party Remove refused with reason");
    check(s.removeSupport(SlotRef::boxed(0, 2)).available, "box Remove available");
    check(s.remove(SlotRef::boxed(0, 2), error), "remove Box 1/3: " + error);
    check(s.view(SlotRef::boxed(0, 2))->empty, "removed slot reads empty");
    check(!s.removeSupport(SlotRef::boxed(0, 2)).available, "a slot staged for removal cannot be removed again");
    check(!s.createSupport(SlotRef::boxed(5, 5)).available && !s.createSupport(SlotRef::boxed(5, 5)).reason.empty(),
          "Create blocked with the named prerequisite");
    check(s.createSupport(SlotRef::boxed(5, 5)).reason == sunRow->status(Capability::Workflow::Create).reason,
          "Create reason is the registry's Create reason");
    check(s.undo(SlotRef::boxed(0, 2)) && !s.view(SlotRef::boxed(0, 2))->empty, "undo restores the removed record");
    check(!s.undo(SlotRef::boxed(0, 2)), "undo of an unstaged slot reports nothing to undo");

    // ---- bag ----
    check(s.bagAvailable(), "bag decodes");
    auto bag = s.bag();
    check(bag.size() == 6, "Sun/Moon shows six pockets (no Roto Powers)");
    if (bag.size() == 6) {
        check(bag[0].name == "Items" && bag[1].name == "Medicine" && bag[2].name == "TMs" && bag[3].name == "Berries" &&
                  bag[4].name == "Key Items" && bag[5].name == "Z-Crystals",
              "native pocket order");
        check(bag[0].items.size() == 2 && bag[0].items[1].itemId == 4 && bag[0].items[1].count == 10, "two Items stacks decoded");
        check(bag[0].editable && bag[1].editable && bag[3].editable, "Items, Medicine and Berries editable");
        check(!bag[2].editable && !bag[4].editable && !bag[5].editable && !bag[5].reason.empty(), "TMs, Key Items and Z-Crystals read-only with reason");
        check(bag[5].items.size() == 1 && bag[5].items[0].itemId == 807 && bag[5].maxQuantity == 1, "Z-Crystal stack shown read-only");
        check(bag[0].maxQuantity == 999, "Items pocket maximum");
    }
    check(s.bagSetQuantity(0, 1, 42, error), "set quantity: " + error);
    check(s.bag()[0].items[1].count == 42, "quantity staged");
    check(!s.bagSetQuantity(0, 1, 1000, error), "quantity beyond the pocket maximum refused");
    check(!s.bagAdd(4, 466, 1, error) && !error.empty(), "key item add refused");
    check(!s.bagAdd(5, 808, 1, error) && !error.empty(), "Z-Crystal add refused");
    check(!s.bagAdd(2, 329, 1, error) && !error.empty(), "TM add refused");
    check(!s.bagRemove(5, 0, error) && !error.empty(), "Z-Crystal remove refused");
    check(!s.bagAdd(6, 949, 1, error), "Roto Powers pocket is absent in Sun/Moon");
    check(s.bagAdd(0, 2, 7, error), "add Ultra Ball: " + error);
    check(s.bag()[0].items.size() == 3, "stack added");
    check(s.bagAdd(3, 149, 5, error) && s.bag()[3].items.size() == 1, "add Cheri Berry: " + error);
    check(!s.bagAddChoices(0).empty() && !s.bagAddChoices(3).empty(), "add choices for addable pockets");
    check(s.bagAddChoices(2).empty() && s.bagAddChoices(4).empty() && s.bagAddChoices(5).empty() && s.bagAddChoices(6).empty(),
          "no add choices for story pockets or an absent pocket");
    check(s.changeCount() == 7, "bag counts as one review change next to the six record changes");
    bool sawBag = false;
    for (const auto& r : s.reviewRows()) if (r.label.rfind("Bag / Items", 0) == 0) sawBag = true;
    check(sawBag, "review lists bag changes");
    s.discardBag();
    check(s.bag()[0].items.size() == 2 && s.bag()[0].items[1].count == 10 && s.bag()[3].items.empty(), "bag discard restores the source bag");

    // ---- trainer / dex ----
    check(s.trainerName() == "ALOLA" && s.trainerGender() == 0, "trainer name and gender");
    check(s.trainerId() == 1111 && s.secretId() == 2222, "MyStatus TID/SID");
    const auto facts = s.trainerFacts();
    check(hasFact(facts, "Trainer ID", "1111") && hasFact(facts, "Secret ID", "2222"), "trainer facts include the TID/SID");
    check(hasFact(facts, "Game", "Sun") && hasFact(facts, "Play Time", "300h 1m 2s"), "trainer facts name the game and play time");
    check(s.pokedex().total == 802 && s.pokedex().available(), "Alola dex total (SM)");

    // ---- export: Blocked for Gen VII; nothing is written ----
    check(s.hasChanges(), "changes are staged before the export check");
    const auto support = s.exportSupport();
    check(!support.available && !support.reason.empty(), "export unavailable even with staged changes, with reason");
    check(support.reason.find("SAV7") != std::string::npos, "export reason names SAV7");
    check(support.reason == sunRow->status(Capability::Workflow::Export).reason, "export reason is the registry's Export reason");
    check(sunRow->status(Capability::Workflow::Export).support == Capability::Support::Blocked, "registry marks Gen VII export Blocked");
    const std::string root = makeTempDir();
    check(!root.empty() && directoryEntries(root) == 0, "temp export root starts empty");
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.sourcePath = "sdmc:/3ds/Checkpoint/saves/0x01648 Sun/b1/main";
    req.timestamp = "20261031_120000";
    req.applicationSha = "test";
    const auto result = s.exportWorkingCopy(req);
    check(!result.success && result.error == std::string(support.reason), "export fails with the registry reason");
    check(result.directory.empty() && result.changedBytes == 0 && result.editedSha256.empty(), "failed export reports no output");
    check(directoryEntries(root) == 0, "export wrote nothing into the export root");
    check(::rmdir(root.c_str()) == 0, "the empty export root can be removed (no hidden files)");

    // Test-only verification of what the adapter staged: the lane's unsigned candidate must build and
    // pass the independent whitelist verifier, and relaxed reparse must show every staged change.
    {
        std::string why;
        const auto candidate = g7->staged().buildUnsignedCandidate(&why);
        check(candidate.has_value() && candidate->signaturePending, "unsigned candidate builds from the adapter's staged entries: " + why);
        if (candidate) {
            check(G7::verifyUnsignedCandidate(g7->source(), candidate->bytes, g7->staged().changes(), &why, g7->staged().stagedBagBlock()),
                  "independent verifier accepts the staged candidate: " + why);
            check(!G7::Gen7Save::parse(candidate->bytes, "sun_3ds", &why).has_value(), "the candidate is NOT a valid signed SAV7");
            const auto relaxed = G7::Gen7Save::parse(candidate->bytes, "sun_3ds", &why, G7::SignaturePolicy::Ignore);
            check(relaxed.has_value() && !relaxed->signatureVerified(), "relaxed reparse (signature ignored) succeeds: " + why);
            if (relaxed) {
                const auto edited = relaxed->box(0, 0).pokemon;
                check(edited && edited->heldItem() == 5 && edited->nickname() == u"NEW", "edits present in the candidate");
                check(relaxed->box(1, 0).occupied() && relaxed->box(1, 0).pokemon->species() == 101 &&
                          relaxed->box(0, 1).state == G7::SlotState::Empty,
                      "move present in the candidate");
                check(relaxed->party(0).pokemon->iv(0) == 7, "party edit present in the candidate");
            }
        }
    }
    check(original == pristine, "the source bytes never changed");
    check(s.sourceBytes().size() == pristine.size() && std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin()),
          "adapter holds the immutable source");

    s.discardAll();
    check(!s.hasChanges() && s.changeCount() == 0 && s.view(SlotRef::boxed(0, 0))->heldItem == 1 && s.view(SlotRef::boxed(0, 1))->species == 101,
          "discard all restores the source view");

    // ---- draft bracket (shared View/Edit workspace) ----
    check(!s.beginDraft(SlotRef::boxed(5, 5), error) && !error.empty(), "draft on an empty slot refused");
    check(!s.beginDraft(SlotRef::boxed(32, 0), error) && !error.empty(), "draft on a non-canonical slot refused");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.draftActive() && s.draftSlot() == SlotRef::boxed(0, 0) && !s.draftDirty(),
          "draft begins clean");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 9, 0, error) && s.draftDirty() && s.view(SlotRef::boxed(0, 0))->heldItem == 9,
          "edit inside the draft is visible and dirty");
    s.discardDraft();
    check(!s.draftActive() && !s.stagedAt(SlotRef::boxed(0, 0)) && s.view(SlotRef::boxed(0, 0))->heldItem == 1 && !s.hasChanges(),
          "discard draft restores the pre-draft state exactly");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 200, 0, error), "second draft");
    s.keepDraft();
    check(!s.draftActive() && s.stagedAt(SlotRef::boxed(0, 0)) && s.view(SlotRef::boxed(0, 0))->friendship == 200, "keep draft retains the edit");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && !s.draftDirty(), "a draft over a kept edit starts clean");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 150, 0, error) && s.draftDirty(), "further edit dirties it");
    s.discardDraft();
    check(s.view(SlotRef::boxed(0, 0))->friendship == 200 && s.stagedAt(SlotRef::boxed(0, 0)), "discard returns to the KEPT edit, not the source");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::Pokerus, 0x11, 0, error) &&
              s.beginDraft(SlotRef::boxed(0, 3), error) == false,
          "a new draft on an empty slot is refused while another is active");
    check(s.beginDraft(SlotRef::boxed(31, 29), error) && s.draftSlot() == SlotRef::boxed(31, 29), "a new draft on another record replaces the stale bracket");
    check(s.view(SlotRef::boxed(0, 0))->pokerus == 0 && s.view(SlotRef::boxed(0, 0))->friendship == 200, "the stale bracket was discarded, not kept");
    s.discardDraft();
    s.discardAll();
    // Review findings W5-01/W5-02 (same contract as Gen VI): Discard All closes an open draft, and
    // slot operations touching the open draft are refused.
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 9, 0, error), "draft before Discard All");
    s.discardAll();
    s.discardDraft();
    check(!s.draftActive() && !s.hasChanges(), "discardDraft after Discard All re-stages nothing");
    check(s.beginDraft(SlotRef::boxed(0, 1), error), "draft on Box 1/2");
    check(!s.move(SlotRef::boxed(0, 1), SlotRef::boxed(1, 0), error) && error.find("open editor draft") != std::string::npos &&
              !s.remove(SlotRef::boxed(0, 1), error) && !s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(1, 0), error) &&
              !s.undo(SlotRef::boxed(0, 1)),
          "Move/Remove/Clone/undo of the drafted slot refused");
    s.discardDraft();
    check(!s.hasChanges(), "nothing half-applied");
    check(!s.exportSupport().available && !s.exportSupport().reason.empty(), "export unavailable without changes, with reason");
    check(original == pristine, "the source bytes still never changed");

    // ---- Moon: a different exact id in the same family; a bag the strict decoder rejects ----
    {
        const auto moon = Gen7Fixture::makeSave({Layout::Family::SunMoon, Layout::VersionMoon, 1, true, true});
        check(!G7::Gen7NativeSession::open(moon, "sun_3ds").ready(), "a Moon save refuses a Sun assignment");
        auto m = G7::Gen7NativeSession::open(moon, "moon_3ds");
        check(m.ready(), "a Moon save opens: " + m.diagnostic);
        if (m.ready()) {
            check(m.session->exactGameId() == "moon_3ds" && m.session->partyCount() == 1, "Moon identity and party count");
            check(hasFact(m.session->trainerFacts(), "Game", "Moon"), "Moon trainer fact");
            check(!m.session->bagAvailable() && m.session->bag().empty(), "an unrecognized native bag is unavailable, never guessed");
            check(!m.session->bagSetQuantity(0, 0, 1, error) && !error.empty(), "bag edits refused with reason when the bag is unavailable");
            check(!m.session->exportSupport().available, "Moon export blocked too");
        }
    }

    // ---- Ultra Sun: USUM geometry, limits, dex and the seventh (Roto Powers) pocket ----
    {
        const auto usum = saveWithBag(Layout::Family::UltraSunUltraMoon, Layout::VersionUltraSun);
        check(!G7::Gen7NativeSession::open(usum, "sun_3ds").ready(), "an Ultra Sun save refuses a Sun assignment");
        check(!G7::Gen7NativeSession::open(usum, "ultramoon_3ds").ready(), "an Ultra Sun save refuses an Ultra Moon assignment");
        auto u = G7::Gen7NativeSession::open(usum, "ultrasun_3ds");
        check(u.ready(), "an Ultra Sun save opens: " + u.diagnostic);
        if (u.ready()) {
            auto& us = *u.session;
            auto* ug7 = dynamic_cast<G7::Gen7NativeSession*>(u.session.get());
            check(ug7 && ug7->gameGroup() == Enums::GameVersion::USUM, "USUM game group");
            check(us.capabilities().id == "ultrasun_3ds" && us.capabilities().family == "USUM", "USUM registry row");
            check(us.boxCount() == 32 && us.slotsPerBox() == 30 && us.familyLabel() == "Generation VII", "USUM geometry");
            check(us.fieldMaximum(Field::HeldItem) == 959, "USUM held item limit");
            check(us.pokedex().total == 807, "Alola dex total (USUM)");
            check(us.view(SlotRef::boxed(31, 29))->species == 785 && us.view(SlotRef::party(0))->species == 722, "USUM records read");
            check(us.stageValue(SlotRef::boxed(31, 29), Field::HeldItem, 959, 0, error), "USUM top held item stages: " + error);
            check(!us.stageValue(SlotRef::boxed(31, 29), Field::HeldItem, 960, 0, error), "beyond the USUM item range refused");
            const auto ub = us.bag();
            check(ub.size() == 7, "Ultra Sun shows seven pockets");
            if (ub.size() == 7) {
                check(ub[6].name == "Roto Powers" && ub[6].editable && ub[6].items.size() == 1 && ub[6].items[0].itemId == 949,
                      "Roto Powers pocket present, editable, decoded");
                check(ub[5].name == "Z-Crystals" && !ub[5].editable, "Z-Crystals still read-only in USUM");
            }
            check(!us.bagAddChoices(6).empty() && us.bagAdd(6, 950, 2, error) && us.bag()[6].items.size() == 2, "Roto Power add: " + error);
            check(!us.bagAdd(6, 1, 1, error), "a non-Roto item cannot enter the Roto Powers pocket");
            check(!us.exportSupport().available && us.exportSupport().reason.find("SAV7") != std::string::npos, "USUM export blocked with the SAV7 reason");
            const std::string uroot = makeTempDir();
            Modern::ExportRequest ureq;
            ureq.rootDirectory = uroot;
            ureq.timestamp = "20261031_120001";
            check(!us.exportWorkingCopy(ureq).success && directoryEntries(uroot) == 0, "USUM export writes nothing");
            (void)::rmdir(uroot.c_str());
            check(std::equal(us.sourceBytes().begin(), us.sourceBytes().end(), usum.begin(), usum.end()), "USUM source immutable");
        }
    }

    battleBoxGuard();
    hyperTraining();

    std::cout << "test_gen7_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
