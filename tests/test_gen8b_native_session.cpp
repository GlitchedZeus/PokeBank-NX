// Brilliant Diamond / Shining Pearl (SAV8BS / PB8) adapter of the shared Modern::NativeSession
// contract, on synthetic SaveData images (tests/fixtures/gen8b; layout cited from PKHeX SAV8BS):
//   * strict open: every revision size with its Gem8Version word, whole-file MD5, ROM code, party;
//   * record view, box names, the shared PK8 provider table, BD/SP held items and balls;
//   * staged edits, box Clone/Move/Remove, party storage refusal, battle-team protection;
//   * draft bracket; Export Copy round trip (only staged records + MD5 change; source untouched);
//   * the independent verifier refuses foreign bytes, non-whitelisted PB8 bytes, team-member
//     removal and a stale MD5.

#include "Integration/Gen8b/Gen8bNativeSession.h"
#include "fixtures/gen8b/Gen8bSyntheticSave.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G8b = PokeVault::Integration::Gen8b;
namespace Modern = PokeVault::Integration::Modern;
namespace L = PokeVault::Integration::Gen8b::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;
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
    char pattern[] = "/tmp/gen8b-native-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
G8b::Gen8bNativeSession& native(Modern::OpenedSession& o) { return static_cast<G8b::Gen8bNativeSession&>(*o.session); }

void testOpen() {
    for (size_t r = 0; r < L::Revisions.size(); ++r) {
        Gen8bFixture::Options o;
        o.revision = r;
        auto opened = G8b::Gen8bNativeSession::open(Gen8bFixture::makeSave(o), "brilliant_diamond_switch");
        check(opened.ready(), std::string("revision ") + L::Revisions[r].label + " opens: " + opened.diagnostic);
    }
    const auto file = Gen8bFixture::makeSave();
    check(!G8b::Gen8bNativeSession::open(file, "shining_pearl_switch").ready(), "BD save refused as SP");
    Gen8bFixture::Options sp;
    sp.romCode = 1;
    check(G8b::Gen8bNativeSession::open(Gen8bFixture::makeSave(sp), "shining_pearl_switch").ready(), "SP opens");
    {
        auto bad = file;
        bad[0x6000] ^= 1;
        auto o = G8b::Gen8bNativeSession::open(bad, "brilliant_diamond_switch");
        check(!o.ready() && o.diagnostic.find("MD5") != std::string::npos, "MD5 mismatch refused");
    }
    {
        auto bad = file;
        PK8::write32(bad, 0, 0x2C);  // 1.1 word in a 1.0-sized file
        G8b::Gen8bSave::refreshHash(bad);
        auto o = G8b::Gen8bNativeSession::open(bad, "");
        check(!o.ready() && o.diagnostic.find("revision") != std::string::npos, "revision word / size mismatch refused");
    }
    {
        Gen8bFixture::Options o;
        o.partyCount = 0;
        check(!G8b::Gen8bNativeSession::open(Gen8bFixture::makeSave(o), "").ready(), "empty party refused");
    }
    std::vector<uint8_t> junk(0x100000, 0);
    check(!G8b::Gen8bNativeSession::open(junk, "").ready(), "unknown size refused");
}

void testViewEditsStorage() {
    auto opened = G8b::Gen8bNativeSession::open(Gen8bFixture::makeSave(), "brilliant_diamond_switch");
    auto& s = *opened.session;
    std::string error;
    check(s.capabilities().title == "Brilliant Diamond" && s.familyLabel() == "Generation VIII" && s.boxCount() == 40 &&
              s.slotsPerBox() == 30 && s.partyCount() == 2,
          "identity and 40 x 30 geometry");
    check(s.boxName(0) == "Underground" && s.boxName(1) == "Box 2", "box names from BoxLayout8b");
    const auto p = s.view(SlotRef::party(0));
    check(p && p->species == 387 && p->level == 5 && p->nickname == "SYNTH", "party record decodes");
    bool dynamax = false;
    for (const auto& f : p->nativeFacts) dynamax = dynamax || f.first.find("Dynamax") != std::string::npos;
    check(!dynamax, "no Dynamax facts outside Sword/Shield");
    check(s.fieldState(Field::Pokerus, Modern::Region::Box) == State::Editable && s.fieldState(Field::IV, Modern::Region::Box) == State::ReadOnly,
          "the shared PK8 provider table");
    check(s.ballChoices().size() == 26 && s.heldItemChoices().size() > 50, "BD/SP balls and holdable items");
    check(s.trainerName() == "Lucas" && s.trainerGender() == 0 && s.trainerId() == 31337, "trainer");
    const auto at = SlotRef::boxed(0, 0);
    check(s.stageText(at, Field::Nickname, "Piplup", error) && s.view(at)->nickname == "Piplup", "nickname: " + error);
    check(s.stageValue(at, Field::Pokerus, 0x21, 0, error) && s.stageValue(at, Field::Ball, 26, 0, error) &&
              s.stageValue(SlotRef::party(1), Field::Friendship, 199, 0, error),
          "Pokerus, ball and party friendship");
    check(!s.stageValue(at, Field::Ball, 27, 0, error), "ball outside the BD/SP list refused");
    // Storage.
    check(!s.cloneSupport(SlotRef::party(0)).available && !s.removeSupport(SlotRef::party(0)).available, "party storage refused");
    check(!s.removeSupport(SlotRef::boxed(0, 1)).available && s.cloneSupport(SlotRef::boxed(0, 1)).available,
          "team member: clone yes, remove no");
    check(s.move(SlotRef::boxed(39, 29), SlotRef::boxed(5, 5), error) && s.view(SlotRef::boxed(5, 5))->species == 483 &&
              s.view(SlotRef::boxed(39, 29))->empty,
          "box move: " + error);
    check(s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(2, 2), error) && s.view(SlotRef::boxed(2, 2))->species == 396, "clone");
    check(!s.createSupport(SlotRef::boxed(9, 9)).available, "create blocked");
    Gen8bFixture::Options locked;
    locked.lockTeam0 = true;
    auto lockedSave = G8b::Gen8bNativeSession::open(Gen8bFixture::makeSave(locked), "brilliant_diamond_switch");
    check(lockedSave.ready() && !lockedSave.session->stageValue(SlotRef::boxed(0, 1), Field::HeldItem, 1, 0, error) &&
              error.find("locked") != std::string::npos,
          "locked-team member edit refused");
    Gen8bFixture::Options broken;
    broken.invalidTeam = true;
    auto blind = G8b::Gen8bNativeSession::open(Gen8bFixture::makeSave(broken), "brilliant_diamond_switch");
    check(blind.ready() && !blind.session->stageValue(SlotRef::boxed(0, 0), Field::Pokerus, 1, 0, error) &&
              blind.session->stageValue(SlotRef::party(0), Field::Pokerus, 1, 0, error),
          "unreadable team data: box edits refused, party edits allowed");
}

void testDraftAndExport() {
    const auto pristine = Gen8bFixture::makeSave();
    auto original = pristine;
    auto opened = G8b::Gen8bNativeSession::open(original, "brilliant_diamond_switch");
    auto& s = *opened.session;
    auto& n = native(opened);
    std::string error;
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::Ball, 2, 0, error) && s.draftDirty(),
          "draft edit");
    s.discardDraft();
    check(!s.hasChanges(), "discard draft");
    check(s.stageText(SlotRef::boxed(0, 0), Field::OriginalTrainer, "Dawn", error) && s.remove(SlotRef::boxed(39, 29), error),
          "stage OT edit and a removal");
    const std::string root = makeTempDir();
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.sourcePath = "manual:/switch/saves/bdsp/SaveData.bin";
    req.timestamp = "20261031_220000";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export: " + result.error);
    if (result.success) {
        const auto edited = slurp(result.directory + "/edited.bin");
        check(slurp(result.directory + "/original_backup.bin") == pristine, "backup pristine");
        size_t outside = 0;
        for (size_t i = 0; i < edited.size(); ++i) {
            const bool rec0 = i >= L::boxOffset(0, 0) && i < L::boxOffset(0, 0) + L::RecordSize;
            const bool rec1 = i >= L::boxOffset(39, 29) && i < L::boxOffset(39, 29) + L::RecordSize;
            const bool hash = i >= L::HashOffset && i < L::HashOffset + L::HashBytes;
            if (!rec0 && !rec1 && !hash && edited[i] != pristine[i]) ++outside;
        }
        check(outside == 0 && G8b::Gen8bSave::hashValid(edited), "only the staged records and the MD5 changed; MD5 valid");
        auto reopened = G8b::Gen8bNativeSession::open(edited, "brilliant_diamond_switch");
        check(reopened.ready() && reopened.session->view(SlotRef::boxed(0, 0))->otName == "Dawn" &&
                  reopened.session->view(SlotRef::boxed(39, 29))->empty,
              "edited copy strictly reopens with the changes");
    }
    check(original == pristine, "source unchanged");
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    const auto built = n.staged().buildWorkingCopy(&error);
    check(built.has_value(), "working copy builds: " + error);
    if (!built) return;
    const auto changes = n.staged().changes();
    check(G8b::verifyWorkingCopy(n.staged().source(), *built, changes, &error), "verifier accepts");
    {
        auto bad = *built;
        bad[0x6000] ^= 1;
        G8b::Gen8bSave::refreshHash(bad);
        check(!G8b::verifyWorkingCopy(n.staged().source(), bad, changes, &error), "foreign byte refused even with a fresh MD5");
    }
    {
        auto bad = *built;
        bad[L::boxOffset(0, 0) + 0x20] ^= 1;  // stale MD5
        check(!G8b::verifyWorkingCopy(n.staged().source(), bad, changes, &error), "stale MD5 refused");
    }
    {
        auto bad = *built;
        auto plain = PK8::decrypt(std::span<const uint8_t>(bad).subspan(L::boxOffset(0, 0), L::RecordSize));
        PK8::write16(plain, 0x08, 25);
        PK8::write16(plain, 0x06, PK8::checksum(plain));
        const auto enc = PK8::encrypt(plain);
        std::copy(enc.begin(), enc.end(), bad.begin() + static_cast<std::ptrdiff_t>(L::boxOffset(0, 0)));
        G8b::Gen8bSave::refreshHash(bad);
        auto forged = changes;
        for (auto& c : forged)
            if (c.slot.box == 0 && c.slot.slot == 0) c.stagedPlain = plain;
        check(!G8b::verifyWorkingCopy(n.staged().source(), bad, forged, &error) && error.find("non-whitelisted") != std::string::npos,
              "species change refused even if 'reviewed'");
    }
    {
        std::vector<uint8_t> bad(pristine);
        Gen8Fixture::put(bad, L::boxOffset(0, 1), Gen8Fixture::blankMon());
        G8b::Gen8bSave::refreshHash(bad);
        G8b::StagedChange c;
        c.slot = G8b::SlotRef::boxed(0, 1);
        c.kind = G8b::ChangeKind::Remove;
        const auto member = n.staged().source().box(0, 1);
        c.originalPlain.assign(member.pokemon->plain().begin(), member.pokemon->plain().end());
        c.stagedPlain.assign(PK8::PartySize, 0);
        check(!G8b::verifyWorkingCopy(n.staged().source(), bad, {c}, &error), "forged team-member removal refused");
    }
}

} // namespace

int main() {
    testOpen();
    testViewEditsStorage();
    testDraftAndExport();
    std::cout << "test_gen8b_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
