// Sword/Shield (Generation VIII) adapter of the shared Modern::NativeSession contract.
//
// Proves on synthetic SwishCrypto `main` images (tests/fixtures/gen8, no real save committed):
//   * strict open, exact-game assignment, identity and geometry;
//   * the provider field table (what the shared editor may offer) and the record view;
//   * staged Edit (text + numeric) with format bounds, handler-friendship refusal, Undo;
//   * Clone / Move / Remove box-to-box, party storage refusal, battle-team protection
//     (team-referenced slots never removed/moved, locked team members never edited, unreadable
//     team blocks refuse every box change);
//   * the shared draft bracket (begin / dirty / discard / keep, draft-owned slot refusals);
//   * Bag/Pokedex honestly unavailable with reasons;
//   * Export Copy: a separate verified directory whose edited copy strictly reopens with the
//     changes, the backup equals the pristine source, and the source bytes never change;
//   * the independent verifier refuses tampered images (foreign block byte, non-whitelisted PK8
//     byte, team-referenced removal, non-canonical container).

#include "Integration/Gen8/Gen8NativeSession.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G8 = PokeVault::Integration::Gen8;
namespace Modern = PokeVault::Integration::Modern;
namespace L = PokeVault::Integration::Gen8::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;
namespace Swish = PokeVault::Integration::Gen8::Swish;
using Modern::SlotRef;
using Field = Modern::Field;
using State = Modern::FieldState;

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
    char pattern[] = "/tmp/gen8-native-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
G8::Gen8NativeSession& native(Modern::OpenedSession& opened) {
    return static_cast<G8::Gen8NativeSession&>(*opened.session);
}
bool hasFact(const std::vector<std::pair<std::string, std::string>>& facts, const std::string& label,
             const std::string& value = {}) {
    for (const auto& f : facts)
        if (f.first == label && (value.empty() || f.second == value)) return true;
    return false;
}

// Box 1 slot 1 holds a record whose current handler is the handling trainer.
std::vector<uint8_t> saveWithHandledRecord() {
    auto blocks = Gen8Fixture::makeBlocks();
    auto plain = Gen8Fixture::plainMon(25, 1);
    plain[0xC4] = 1;
    PK8::write16(plain, 0x06, PK8::checksum(plain));
    Gen8Fixture::put(Gen8Fixture::findBlock(blocks, L::KeyBox)->data, 0, PK8::encrypt(plain));
    return Swish::encrypt(blocks);
}

void testOpenAndIdentity() {
    const auto file = Gen8Fixture::makeSave();
    check(Swish::encrypt(G8::Gen8Save::parse(file)->blocks()) == file,
          "prerequisite: SwishCrypto re-encryption of an unchanged block list is byte-identical");
    auto opened = G8::Gen8NativeSession::open(file, "sword_switch");
    check(opened.ready(), "Sword opens: " + opened.diagnostic);
    auto wrong = G8::Gen8NativeSession::open(file, "shield_switch");
    check(!wrong.ready() && !wrong.diagnostic.empty(), "Sword save assigned to Shield is refused with a reason");
    std::vector<uint8_t> junk(file.size(), 0x5A);
    check(!G8::Gen8NativeSession::open(junk, "sword_switch").ready(), "a corrupt container is refused");
    Gen8Fixture::Options shield;
    shield.version = L::VersionShield;
    check(G8::Gen8NativeSession::open(Gen8Fixture::makeSave(shield), "shield_switch").ready(), "Shield opens");
    if (!opened.ready()) return;
    auto& s = *opened.session;
    check(s.exactGameId() == "sword_switch" && s.capabilities().title == "Sword" && s.familyLabel() == "Generation VIII",
          "identity comes from the registry row");
    check(s.boxCount() == 32 && s.slotsPerBox() == 30 && s.partyCount() == 2, "SWSH storage geometry");
    check(s.boxName(4) == "Box 5", "box names are generic until a BoxLayout8 decoder is verified");
}

void testFieldTableAndView() {
    auto opened = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(), "sword_switch");
    auto& s = *opened.session;
    for (const auto f : {Field::Nickname, Field::OriginalTrainer, Field::OriginalTrainerGender, Field::HeldItem,
                         Field::Friendship, Field::Pokerus, Field::TrainerId, Field::SecretId, Field::Ball,
                         Field::FatefulEncounter})
        check(s.fieldState(f, Modern::Region::Box) == State::Editable && s.fieldState(f, Modern::Region::Party) == State::Editable,
              "plain stored PK8 field is Editable");
    for (const auto f : {Field::IV, Field::EV, Field::Nature, Field::StatNature, Field::Level, Field::Species, Field::Move,
                         Field::Ability, Field::MetLocation})
        check(s.fieldState(f, Modern::Region::Box) == State::ReadOnly, "battle-stat input / identity field is ReadOnly");
    check(s.fieldState(Field::CalculatedStats, Modern::Region::Box) == State::Derived, "cached stats are Derived in boxes too");
    check(s.fieldState(Field::AV, Modern::Region::Box) == State::Hidden && s.fieldState(Field::DV, Modern::Region::Party) == State::Hidden,
          "non-PK8 fields are Hidden");
    std::string error;
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::IV, 31, 0, error) && !error.empty(),
          "a ReadOnly field cannot be staged even if a caller tries");

    const auto p = s.view(SlotRef::party(0));
    check(p && p->valid && !p->empty && p->species == 810 && p->nickname == "SYNTH" && p->otName == "TESTER",
          "party record decodes");
    check(p && p->level == 5 && p->stats[0] == 20 && p->stats[5] == 25 && p->currentHp == 20, "cached stats/level shown");
    check(p && p->ball == 4 && p->friendship == 70 && p->originGame == 44 && p->language == 2, "stored facts shown");
    check(p && hasFact(p->nativeFacts, "Dynamax Level") && hasFact(p->nativeFacts, "Can Gigantamax", "No"),
          "Dynamax/Gigantamax facts are native read-only rows");
    const auto team = s.view(SlotRef::boxed(0, 1));
    check(team && hasFact(team->nativeFacts, "Battle team", "Team 1"), "team membership is shown");
    const auto blank = s.view(SlotRef::boxed(3, 3));
    check(blank && blank->valid && blank->empty, "blank box slot");
    check(s.originalOccupied(SlotRef::boxed(31, 29)) && !s.originalOccupied(SlotRef::boxed(31, 28)), "occupancy");
    check(s.emptyDestination(SlotRef::boxed(0, 2)) && !s.emptyDestination(SlotRef::party(5)),
          "only verified blank box slots are destinations");
    check(s.firstEmptyBoxSlot() == SlotRef::boxed(0, 2), "first empty box slot");
    const auto items = s.heldItemChoices();
    check(items.size() > 100 && items.front() == 0 &&
          std::all_of(items.begin(), items.end(), [](uint16_t id) { return id <= 1607; }),
          "held-item choices come from SWSH item presence, bounded by maxItem 1607");
    const auto balls = s.ballChoices();
    check(balls.size() == 26 && balls.front() == 1 && balls.back() == 26, "SWSH ball list is Master..Beast");
}

void testEditsAndBounds() {
    auto opened = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(), "sword_switch");
    auto& s = *opened.session;
    std::string error;
    const auto at = SlotRef::boxed(0, 0);
    check(s.stageText(at, Field::Nickname, "Sparky", error) && s.view(at)->nickname == "Sparky" && s.view(at)->nicknamed,
          "nickname staged and flag set: " + error);
    check(!s.stageText(at, Field::Nickname, "ThisNameIsTooLong", error), "13+ character name refused");
    check(!s.stageText(at, Field::Nickname, "Caf\xC3\xA9", error), "non-ASCII name refused (no invented glyph table)");
    check(!s.stageText(at, Field::Nickname, "   ", error), "blank name refused");
    check(s.stageText(at, Field::OriginalTrainer, "Leon", error) && s.view(at)->otName == "Leon", "OT name staged");
    check(s.stageValue(at, Field::HeldItem, 1, 0, error) && s.view(at)->heldItem == 1, "held item staged");
    check(!s.stageValue(at, Field::HeldItem, 1608, 0, error), "held item beyond 1607 refused");
    check(!s.stageValue(at, Field::Ball, 0, 0, error) && !s.stageValue(at, Field::Ball, 27, 0, error), "ball bounds");
    check(s.stageValue(at, Field::Ball, 26, 0, error) && s.view(at)->ball == 26, "Beast Ball accepted");
    check(s.stageValue(at, Field::Friendship, 255, 0, error) && s.view(at)->friendship == 255, "OT friendship");
    check(s.stageValue(at, Field::Pokerus, 0x11, 0, error) && s.view(at)->pokerus == 0x11, "Pokerus byte");
    check(s.stageValue(at, Field::FatefulEncounter, 1, 0, error) && s.view(at)->fatefulEncounter, "fateful bit");
    check(!s.stageValue(at, Field::FatefulEncounter, 2, 0, error), "fateful is one bit");
    check(s.stageValue(at, Field::OriginalTrainerGender, 1, 0, error) && s.view(at)->otGender == 1, "OT gender bit");
    check(s.stageValue(at, Field::TrainerId, 4242, 0, error) && s.view(at)->tid == 4242, "TID16 staged");
    check(s.stageValue(SlotRef::party(1), Field::HeldItem, 4, 0, error) && s.view(SlotRef::party(1))->heldItem == 4,
          "party records are editable");
    check(s.hasChanges() && s.changeCount() == 2 && s.stagedAt(at), "two staged slots");
    const auto rows = s.reviewRows();
    check(rows.size() >= 10 && rows.front().label == "Box 1 / Slot 1", "Review lists every staged field change");
    check(s.undo(SlotRef::party(1)) && !s.stagedAt(SlotRef::party(1)) && s.view(SlotRef::party(1))->heldItem == 0, "Undo");
    check(!s.stageValue(SlotRef::boxed(3, 3), Field::HeldItem, 1, 0, error), "edit of an empty slot refused");

    auto handled = G8::Gen8NativeSession::open(saveWithHandledRecord(), "sword_switch");
    check(handled.ready() && !handled.session->stageValue(SlotRef::boxed(0, 0), Field::Friendship, 1, 0, error) &&
          error.find("handling-trainer") != std::string::npos,
          "handling-trainer friendship is refused, not silently written to the OT byte");
}

void testStorageAndTeams() {
    auto opened = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(), "sword_switch");
    auto& s = *opened.session;
    std::string error;
    // Party storage changes are refused with reasons.
    check(!s.cloneSupport(SlotRef::party(0)).available && !s.removeSupport(SlotRef::party(0)).available &&
          !s.moveSupport(SlotRef::party(0)).available, "party Clone/Move/Remove are unavailable with reasons");
    check(!s.clone(SlotRef::party(0), SlotRef::boxed(0, 2), error), "party clone refused");
    // Team-referenced record (box 1 slot 2, team 1 unlocked).
    const auto teamSlot = SlotRef::boxed(0, 1);
    const auto rm = s.removeSupport(teamSlot);
    check(!rm.available && std::string(rm.reason).find("battle team") != std::string::npos, "team member cannot be removed");
    check(!s.moveSupport(teamSlot).available && !s.remove(teamSlot, error), "team member cannot be moved or removed");
    check(s.cloneSupport(teamSlot).available, "a team member may be cloned (the copy is team-free)");
    check(s.stageValue(teamSlot, Field::HeldItem, 1, 0, error), "unlocked team member is editable");
    s.discardAll();

    // Clone / Move / Remove box-to-box.
    check(s.clone(SlotRef::boxed(0, 0), SlotRef::boxed(1, 0), error) && s.view(SlotRef::boxed(1, 0))->species == 25,
          "clone into a blank box slot: " + error);
    check(!s.clone(SlotRef::boxed(0, 0), SlotRef::boxed(1, 0), error), "clone onto a staged slot refused");
    check(s.move(SlotRef::boxed(31, 29), SlotRef::boxed(1, 1), error) && s.view(SlotRef::boxed(1, 1))->species == 888 &&
          s.view(SlotRef::boxed(31, 29))->empty, "move box 32/30 -> box 2/2: " + error);
    check(s.undo(SlotRef::boxed(1, 1)) && s.undo(SlotRef::boxed(31, 29)), "undo both halves of the move");
    check(s.removeSupport(SlotRef::boxed(31, 29)).available && s.remove(SlotRef::boxed(31, 29), error) &&
          s.view(SlotRef::boxed(31, 29))->empty, "remove a team-free box record");
    check(!s.createSupport(SlotRef::boxed(5, 5)).available && !s.createSupport(SlotRef::boxed(5, 5)).reason.empty(),
          "Create is unavailable with the named prerequisite");

    // A locked team: members are never edited.
    Gen8Fixture::Options locked;
    locked.lockTeam0 = true;
    auto lockedSave = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(locked), "sword_switch");
    check(lockedSave.ready() && !lockedSave.session->stageValue(teamSlot, Field::HeldItem, 1, 0, error) &&
          error.find("locked") != std::string::npos, "locked team member edit refused");
    check(lockedSave.session->stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 1, 0, error), "non-members stay editable");

    // Unreadable team blocks: every box change refused, party edits still allowed.
    Gen8Fixture::Options noTeams;
    noTeams.includeTeams = false;
    auto blind = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(noTeams), "sword_switch");
    check(blind.ready(), "a save without team blocks still opens read-safe");
    if (blind.ready()) {
        auto& b = *blind.session;
        check(!b.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 1, 0, error) && error.find("battle-team") != std::string::npos,
              "box edit refused without team blocks");
        check(!b.firstEmptyBoxSlot() && !b.cloneSupport(SlotRef::boxed(0, 0)).available && !b.removeSupport(SlotRef::boxed(0, 0)).available,
              "box storage refused without team blocks");
        check(b.stageValue(SlotRef::party(0), Field::HeldItem, 1, 0, error), "party edit still allowed");
    }
}

void testDraftBracket() {
    auto opened = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(), "sword_switch");
    auto& s = *opened.session;
    std::string error;
    const auto at = SlotRef::boxed(0, 0);
    check(!s.beginDraft(SlotRef::boxed(5, 5), error), "draft on an empty slot refused");
    check(s.beginDraft(at, error) && s.draftActive() && !s.draftDirty(), "draft begins clean");
    check(s.stageValue(at, Field::HeldItem, 9, 0, error) && s.draftDirty() && s.view(at)->heldItem == 9, "draft edit visible");
    check(!s.clone(at, SlotRef::boxed(2, 0), error) && !s.remove(at, error) && !s.undo(at), "draft-owned slot refuses storage ops");
    s.discardDraft();
    check(!s.draftActive() && !s.stagedAt(at) && s.view(at)->heldItem == 0, "discard draft restores the slot");
    check(s.beginDraft(at, error) && s.stageText(at, Field::Nickname, "Keep", error), "second draft");
    s.keepDraft();
    check(!s.draftActive() && s.stagedAt(at) && s.view(at)->nickname == "Keep", "keep draft retains the edit");
    check(s.beginDraft(at, error) && s.stageValue(at, Field::Ball, 2, 0, error), "third draft over a staged slot");
    s.discardDraft();
    check(s.view(at)->nickname == "Keep" && s.view(at)->ball == 4, "discard restores to the kept staged state");
    check(s.beginDraft(at, error), "open draft");
    s.discardAll();
    check(!s.draftActive() && !s.hasChanges(), "Discard All ends the draft and clears the stage");
}

void testBagTrainerDex() {
    auto opened = G8::Gen8NativeSession::open(Gen8Fixture::makeSave(), "sword_switch");
    auto& s = *opened.session;
    std::string error;
    check(!s.bagAvailable() && s.bag().empty() && !s.bagAdd(0, 1, 1, error) && error.find("MyItem") != std::string::npos,
          "Bag is unavailable with the named prerequisite");
    check(!s.pokedex().available(), "Pokedex counts are unavailable (no verified Zukan8 decoder)");
    check(s.trainerName() == "Hop" && s.trainerGender() == 1 && s.trainerId() == 12345 && s.secretId() == 54321,
          "trainer identity");
    const uint32_t id32 = (54321u << 16) | 12345u;
    bool tid7 = false, sid7 = false;
    for (const auto& f : s.trainerFacts()) {
        if (f.label == "Trainer ID" && f.value == std::to_string(id32 % 1000000u)) tid7 = true;
        if (f.label == "Secret ID" && f.value == std::to_string(id32 / 1000000u)) sid7 = true;
    }
    check(tid7 && sid7, "Gen VII+ display TID/SID are derived from ID32");
}

void testExportRoundTrip() {
    const auto pristine = Gen8Fixture::makeSave();
    auto original = pristine;
    auto opened = G8::Gen8NativeSession::open(original, "sword_switch");
    auto& s = *opened.session;
    std::string error;
    check(!s.exportSupport().available, "export needs a staged change");
    check(s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "NEW", error) &&
          s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 5, 0, error) &&
          s.stageValue(SlotRef::party(0), Field::Friendship, 200, 0, error) &&
          s.move(SlotRef::boxed(31, 29), SlotRef::boxed(1, 0), error), "stage edits and a move: " + error);
    check(s.exportSupport().available, "export available");
    const std::string root = makeTempDir();
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.sourcePath = "manual:/switch/saves/sword/main";
    req.timestamp = "20261031_120000";
    req.applicationSha = "test";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export succeeds: " + result.error);
    if (result.success) {
        const auto edited = slurp(result.directory + "/edited.bin");
        const auto backup = slurp(result.directory + "/original_backup.bin");
        check(backup == pristine, "backup is the pristine source");
        check(edited.size() == pristine.size() && edited != pristine, "edited copy differs, same size");
        const auto manifest = slurp(result.directory + "/EDIT_MANIFEST.txt");
        const std::string text(manifest.begin(), manifest.end());
        check(text.find("Generation VIII") != std::string::npos && text.find("nickname -> NEW") != std::string::npos,
              "manifest names the generation and the reviewed changes");
        auto reopened = G8::Gen8NativeSession::open(edited, "sword_switch");
        check(reopened.ready(), "exported copy strictly reopens: " + reopened.diagnostic);
        if (reopened.ready()) {
            auto& r = *reopened.session;
            check(r.view(SlotRef::boxed(0, 0))->nickname == "NEW" && r.view(SlotRef::boxed(0, 0))->heldItem == 5,
                  "box edits survive");
            check(r.view(SlotRef::party(0))->friendship == 200, "party edit survives");
            check(r.view(SlotRef::boxed(1, 0))->species == 888 && r.view(SlotRef::boxed(31, 29))->empty, "move survives");
            check(r.view(SlotRef::boxed(0, 1))->species == 133 && !r.hasChanges(), "untouched records unchanged");
            const auto& a = native(opened).source().blocks();
            const auto& b = native(reopened).source().blocks();
            size_t differing = 0;
            for (size_t i = 0; i < a.size() && i < b.size(); ++i) if (a[i].data != b[i].data) ++differing;
            check(a.size() == b.size() && differing == 2, "only the Party and Box blocks differ");
        }
    }
    check(original == pristine, "the source bytes never changed");
    check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin(), pristine.end()),
          "adapter holds the immutable source");
    if (!root.empty()) {
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }
}

void testVerifierRefusesTampering() {
    const auto pristine = Gen8Fixture::makeSave();
    auto opened = G8::Gen8NativeSession::open(pristine, "sword_switch");
    auto& n = native(opened);
    std::string error;
    check(n.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 7, 0, error), "stage one edit");
    const auto built = n.staged().buildWorkingCopy(&error);
    check(built.has_value(), "working copy builds and self-verifies: " + error);
    if (!built) return;
    const auto changes = n.staged().changes();
    check(G8::verifyWorkingCopy(n.source(), *built, changes, &error), "independent verifier accepts it");

    // (1) A byte in a foreign block (MyStatus) changed.
    {
        auto blocks = G8::Gen8Save::parse(*built)->blocks();
        Gen8Fixture::findBlock(blocks, L::KeyMyStatus)->data[L::MyStatusTid] ^= 1;
        check(!G8::verifyWorkingCopy(n.source(), Swish::encrypt(blocks), changes, &error), "foreign block change refused");
    }
    // (2) A non-whitelisted PK8 byte (species) inside the staged record.
    {
        auto blocks = G8::Gen8Save::parse(*built)->blocks();
        auto& box = Gen8Fixture::findBlock(blocks, L::KeyBox)->data;
        auto plain = PK8::decrypt(std::span<const uint8_t>(box).subspan(0, L::RecordSize));
        PK8::write16(plain, 0x08, 26);
        PK8::write16(plain, 0x06, PK8::checksum(plain));
        Gen8Fixture::put(box, 0, PK8::encrypt(plain));
        auto forged = changes;
        forged.front().stagedPlain = plain;
        check(!G8::verifyWorkingCopy(n.source(), Swish::encrypt(blocks), forged, &error) &&
              error.find("non-whitelisted") != std::string::npos, "species change refused even if 'reviewed'");
    }
    // (3) A forged removal of a team-referenced record.
    {
        auto blocks = n.source().blocks();
        auto& box = Gen8Fixture::findBlock(blocks, L::KeyBox)->data;
        Gen8Fixture::put(box, 1 * L::RecordSize, Gen8Fixture::blankMon());
        G8::StagedChange c;
        c.slot = G8::SlotRef::boxed(0, 1);
        c.kind = G8::ChangeKind::Remove;
        const auto member = n.source().box(0, 1);
        c.originalPlain.assign(member.pokemon->plain().begin(), member.pokemon->plain().end());
        c.stagedPlain.assign(PK8::PartySize, 0);
        check(!G8::verifyWorkingCopy(n.source(), Swish::encrypt(blocks), {c}, &error), "team-member removal refused");
    }
    // (4) A non-canonical container (hash broken).
    {
        auto bad = *built;
        bad[bad.size() - 1] ^= 0xFF;
        check(!G8::verifyWorkingCopy(n.source(), bad, changes, &error), "broken container hash refused");
    }
    // (5) Size change.
    {
        auto bad = *built;
        bad.push_back(0);
        check(!G8::verifyWorkingCopy(n.source(), bad, changes, &error), "size change refused");
    }
}

} // namespace

int main() {
    testOpenAndIdentity();
    testFieldTableAndView();
    testEditsAndBounds();
    testStorageAndTeams();
    testDraftBracket();
    testBagTrainerDex();
    testExportRoundTrip();
    testVerifierRefusesTampering();
    if (failures) {
        std::cerr << "test_gen8_native_session: " << failures << " of " << checks << " checks FAILED\n";
        return 1;
    }
    std::cout << "test_gen8_native_session: all " << checks << " checks passed\n";
    return 0;
}
