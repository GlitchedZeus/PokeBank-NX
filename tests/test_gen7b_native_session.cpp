// Let's Go Pikachu / Eevee (SAV7b / PB7) adapter of the shared Modern::NativeSession contract.
//
// On synthetic `main` images (tests/fixtures/gen7b; layout cited from PKSM-Core SavLGPE/PB7):
//   * strict open: block CRCs, version, gapless storage, party/partner pointer integrity;
//   * party slots resolve through pointers into storage (one record, two views);
//   * the provider table (Friendship/AVs read-only because they feed the cached CP);
//   * staged Edit within bounds (Let's Go balls only), Clone appended at the single next free slot,
//     Release with compaction and party/partner pointer remap, party/partner protection, Move blocked;
//   * Undo of the last operation and the shared draft bracket;
//   * Export Copy round trip: only the PokeListHeader/storage blocks and their CRCs change, the
//     source never changes, the 1 MiB file tail is preserved;
//   * the independent verifier refuses foreign-block changes, non-whitelisted PB7 bytes, a release
//     of a party member and an image that does not match the reviewed operations.

#include "Integration/Gen7b/Gen7bNativeSession.h"
#include "fixtures/gen7b/Gen7bSyntheticSave.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G7b = PokeVault::Integration::Gen7b;
namespace Modern = PokeVault::Integration::Modern;
namespace L = PokeVault::Integration::Gen7b::Layout;
namespace PK6 = PokeVault::Integration::Gen6::PK6;
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
    char pattern[] = "/tmp/gen7b-native-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
G7b::Gen7bNativeSession& native(Modern::OpenedSession& o) { return static_cast<G7b::Gen7bNativeSession&>(*o.session); }
bool fact(const Modern::RecordView& v, const std::string& label, const std::string& value) {
    for (const auto& f : v.nativeFacts) if (f.first == label && f.second == value) return true;
    return false;
}
uint32_t ecAt(const Modern::NativeSession& s, SlotRef slot) {
    const auto v = s.view(slot);
    return v ? v->encryptionConstant : 0;
}

void testOpen() {
    const auto file = Gen7bFixture::makeSave();
    auto opened = G7b::Gen7bNativeSession::open(file, "letsgo_pikachu_switch");
    check(opened.ready(), "Pikachu opens: " + opened.diagnostic);
    check(!G7b::Gen7bNativeSession::open(file, "letsgo_eevee_switch").ready(), "Pikachu save refused as Eevee");
    Gen7bFixture::Options eevee;
    eevee.version = L::VersionEevee;
    eevee.fullFile = true;
    check(G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(eevee), "letsgo_eevee_switch").ready(), "1 MiB Eevee file opens");
    {
        auto bad = file;
        bad[L::recordOffset(2) + 0x40] ^= 1;  // storage byte without CRC refresh
        auto o = G7b::Gen7bNativeSession::open(bad, "letsgo_pikachu_switch");
        check(!o.ready() && o.diagnostic.find("checksum") != std::string::npos, "storage CRC mismatch refused");
    }
    {
        auto gap = file;
        Gen6Fixture::put(gap, L::recordOffset(20), Gen6Fixture::encryptedMon(Gen7bFixture::spec(20, L::VersionPikachu), true));
        Gen7bFixture::refreshCrcs(gap);
        auto o = G7b::Gen7bNativeSession::open(gap, "letsgo_pikachu_switch");
        check(!o.ready() && o.diagnostic.find("gapless") != std::string::npos, "a record after the stored count is refused");
    }
    {
        Gen7bFixture::Options o;
        o.party = {0, 9, 1001, 1001, 1001, 1001};  // 9 >= count 6
        auto r = G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(o), "letsgo_pikachu_switch");
        check(!r.ready() && r.diagnostic.find("party pointer") != std::string::npos, "party pointer outside the list refused");
        o.party = {0, 1001, 1, 1001, 1001, 1001};
        check(!G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(o), "letsgo_pikachu_switch").ready(), "non-compact party refused");
        o.party = {0, 0, 1001, 1001, 1001, 1001};
        check(!G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(o), "letsgo_pikachu_switch").ready(), "repeated party pointer refused");
    }
    {
        Gen7bFixture::Options o;
        o.version = 30;
        check(!G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(o), "").ready(), "non-Let's Go version refused");
    }
    std::vector<uint8_t> wrongSize(0x80000, 0);
    check(!G7b::Gen7bNativeSession::open(wrongSize, "letsgo_pikachu_switch").ready(), "wrong size refused");
    if (!opened.ready()) return;
    auto& s = *opened.session;
    check(s.capabilities().title == "Let's Go Pikachu" && s.familyLabel() == "Generation VII" && s.boxCount() == 40 &&
              s.slotsPerBox() == 25 && s.partyCount() == 2,
          "identity and 40 x 25 geometry");
}

void testViewAndFields() {
    auto opened = G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(), "letsgo_pikachu_switch");
    auto& s = *opened.session;
    const auto partner = s.view(SlotRef::party(0));
    check(partner && partner->species == 25 && partner->level == 12 && fact(*partner, "Partner Pokemon", "Yes") &&
              fact(*partner, "In party", "Yes"),
          "party slot 1 resolves to the partner record");
    check(ecAt(s, SlotRef::party(1)) == ecAt(s, SlotRef::boxed(0, 1)) && ecAt(s, SlotRef::party(1)) != 0,
          "a party member is the same storage record in Boxes");
    const auto boxed = s.view(SlotRef::boxed(0, 3));
    check(boxed && !boxed->empty && fact(*boxed, "Partner Pokemon", "No") && fact(*boxed, "Storage index", "4 of 6"),
          "box record with storage facts");
    bool hasAv = false, hasCp = false;
    for (const auto& f : boxed->nativeFacts) {
        hasAv = hasAv || f.first == "Awakening Values";
        hasCp = hasCp || f.first == "Combat Power";
    }
    check(hasAv && hasCp, "Awakening Values and CP are native read-only facts");
    check(s.view(SlotRef::boxed(0, 6))->empty && !s.originalOccupied(SlotRef::boxed(0, 6)), "index 6 is blank");
    check(s.emptyDestination(SlotRef::boxed(0, 6)) && !s.emptyDestination(SlotRef::boxed(0, 7)) &&
              s.firstEmptyBoxSlot() == SlotRef::boxed(0, 6),
          "only the next free index is a destination");
    for (const auto f : {Field::Nickname, Field::OriginalTrainer, Field::OriginalTrainerGender, Field::TrainerId,
                         Field::SecretId, Field::Ball, Field::FatefulEncounter})
        check(s.fieldState(f, Modern::Region::Box) == State::Editable, "plain PB7 field is Editable");
    check(s.fieldState(Field::Friendship, Modern::Region::Box) == State::ReadOnly && s.fieldState(Field::AV, Modern::Region::Party) == State::ReadOnly,
          "Friendship and AVs are read-only (they feed the cached CP)");
    check(s.fieldState(Field::Pokerus, Modern::Region::Box) == State::Hidden, "Pokerus does not exist in Let's Go");
    check(s.ballChoices() == std::vector<uint16_t>{1, 2, 3, 4, 12} && s.heldItemChoices().empty(), "Let's Go balls, no held items");
    check(s.trainerName() == "Red" && s.trainerId() == 24680 && s.secretId() == 13579, "trainer");
    check(!s.bagAvailable() && !s.pokedex().available(), "Bag and Pokedex unavailable with reasons");
}

void testEditsCloneRelease() {
    auto opened = G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(), "letsgo_pikachu_switch");
    auto& s = *opened.session;
    std::string error;
    check(s.stageText(SlotRef::party(1), Field::Nickname, "Sparky", error) && s.view(SlotRef::boxed(0, 1))->nickname == "Sparky",
          "editing a party slot edits the shared storage record");
    check(s.stageValue(SlotRef::boxed(0, 3), Field::Ball, 12, 0, error) && s.view(SlotRef::boxed(0, 3))->ball == 12, "Premier Ball");
    check(!s.stageValue(SlotRef::boxed(0, 3), Field::Ball, 5, 0, error), "a non-Let's Go ball is refused");
    check(s.stageValue(SlotRef::boxed(0, 3), Field::TrainerId, 111, 0, error) && s.stageText(SlotRef::boxed(0, 3), Field::OriginalTrainer, "Blue", error) &&
              s.stageValue(SlotRef::boxed(0, 3), Field::OriginalTrainerGender, 1, 0, error) &&
              s.stageValue(SlotRef::boxed(0, 3), Field::FatefulEncounter, 1, 0, error),
          "TID, OT, OT gender, fateful");
    check(!s.stageValue(SlotRef::boxed(0, 3), Field::Friendship, 200, 0, error), "friendship edit refused");
    check(!s.stageValue(SlotRef::boxed(0, 9), Field::Ball, 1, 0, error), "edit of a blank refused");

    // Clone appends at the single next free index.
    check(!s.clone(SlotRef::boxed(0, 4), SlotRef::boxed(3, 3), error) && error.find("Box 1 / Slot 7") != std::string::npos,
          "a clone elsewhere is refused and names the next free slot");
    check(s.clone(SlotRef::boxed(0, 4), SlotRef::boxed(0, 6), error) && ecAt(s, SlotRef::boxed(0, 6)) == ecAt(s, SlotRef::boxed(0, 4)) &&
              s.firstEmptyBoxSlot() == SlotRef::boxed(0, 7),
          "clone appended at index 7");
    // Release protections.
    check(!s.removeSupport(SlotRef::boxed(0, 0)).available &&
              std::string(s.removeSupport(SlotRef::boxed(0, 0)).reason).find("partner") != std::string::npos,
          "the partner is never released");
    check(!s.removeSupport(SlotRef::boxed(0, 1)).available && !s.removeSupport(SlotRef::party(1)).available, "party members are not released");
    check(!s.moveSupport(SlotRef::boxed(0, 2)).available && !s.move(SlotRef::boxed(0, 2), SlotRef::boxed(0, 7), error),
          "Move is blocked with the gapless-storage reason");
    const uint32_t third = ecAt(s, SlotRef::boxed(0, 3));
    check(s.remove(SlotRef::boxed(0, 2), error) && ecAt(s, SlotRef::boxed(0, 2)) == third && s.view(SlotRef::boxed(0, 6))->empty &&
              s.firstEmptyBoxSlot() == SlotRef::boxed(0, 6),
          "release compacts: later records move up one slot");
    check(s.view(SlotRef::boxed(0, 2))->ball == 12, "the edited record moved with its staged edits");
    check(s.reviewRows().size() == 8, "review lists every operation (6 edits, clone, release)");
    check(!s.undo(SlotRef::boxed(0, 4)) && s.undo(SlotRef::boxed(0, 2)) && !s.view(SlotRef::boxed(0, 6))->empty &&
              s.firstEmptyBoxSlot() == SlotRef::boxed(0, 7) && ecAt(s, SlotRef::boxed(0, 3)) == third,
          "undo only reverts the most recent operation at its slot (the release)");
    s.discardAll();
    check(!s.hasChanges() && s.view(SlotRef::boxed(0, 1))->nickname == "SYNTH", "discard all");

    // Compaction remaps party and partner pointers.
    Gen7bFixture::Options o;
    o.party = {0, 4, 1001, 1001, 1001, 1001};
    o.starter = 3;
    auto remapped = G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(o), "letsgo_pikachu_switch");
    auto& r = *remapped.session;
    const uint32_t member = ecAt(r, SlotRef::party(1)), partnerEc = ecAt(r, SlotRef::boxed(0, 3));
    check(r.remove(SlotRef::boxed(0, 1), error), "release before the party member and partner: " + error);
    const auto& h = native(remapped).staged().current().header;
    check(h.party[1] == 3 && h.starter == 2 && h.count == 5 && ecAt(r, SlotRef::party(1)) == member && ecAt(r, SlotRef::boxed(0, 2)) == partnerEc,
          "party pointer 4 -> 3 and partner 3 -> 2 keep naming the same Pokemon");
}

void testDraft() {
    auto opened = G7b::Gen7bNativeSession::open(Gen7bFixture::makeSave(), "letsgo_pikachu_switch");
    auto& s = *opened.session;
    std::string error;
    check(s.beginDraft(SlotRef::boxed(0, 3), error) && !s.draftDirty(), "draft begins clean");
    check(s.stageValue(SlotRef::boxed(0, 3), Field::Ball, 2, 0, error) && s.draftDirty(), "draft edit");
    check(!s.clone(SlotRef::boxed(0, 3), SlotRef::boxed(0, 6), error) && !s.undo(SlotRef::boxed(0, 3)), "draft blocks storage ops");
    s.discardDraft();
    check(!s.hasChanges() && s.view(SlotRef::boxed(0, 3))->ball == 4, "discard draft restores");
    check(s.beginDraft(SlotRef::boxed(0, 3), error) && s.stageText(SlotRef::boxed(0, 3), Field::Nickname, "Keep", error), "second draft");
    s.keepDraft();
    check(s.hasChanges() && s.view(SlotRef::boxed(0, 3))->nickname == "Keep", "keep draft");
}

void testExportAndVerifier() {
    Gen7bFixture::Options o;
    o.fullFile = true;
    const auto pristine = Gen7bFixture::makeSave(o);
    auto original = pristine;
    auto opened = G7b::Gen7bNativeSession::open(original, "letsgo_pikachu_switch");
    auto& n = native(opened);
    auto& s = *opened.session;
    std::string error;
    check(s.stageText(SlotRef::boxed(0, 3), Field::Nickname, "NEW", error) && s.clone(SlotRef::boxed(0, 5), SlotRef::boxed(0, 6), error) &&
              s.remove(SlotRef::boxed(0, 2), error),
          "stage edit, clone, release: " + error);
    const std::string root = makeTempDir();
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.sourcePath = "manual:/switch/saves/lgpe/main";
    req.timestamp = "20261031_210000";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export succeeds: " + result.error);
    if (result.success) {
        const auto edited = slurp(result.directory + "/edited.bin");
        check(slurp(result.directory + "/original_backup.bin") == pristine, "backup is pristine");
        check(edited.size() == L::FullSize, "1 MiB size kept");
        size_t outside = 0;
        for (size_t i = 0; i < edited.size(); ++i) {
            const bool header = i >= L::Header && i < L::Header + L::HeaderBlockSize;
            const bool storage = i >= L::Storage && i < L::Storage + L::Slots * L::RecordSize;
            const bool crc = i == L::checksumOffset(8) || i == L::checksumOffset(8) + 1 || i == L::checksumOffset(9) || i == L::checksumOffset(9) + 1;
            if (!header && !storage && !crc && edited[i] != pristine[i]) ++outside;
        }
        check(outside == 0 && edited[0x10] == 0x5A, "only the header/storage blocks and their CRCs changed");
        auto reopened = G7b::Gen7bNativeSession::open(edited, "letsgo_pikachu_switch");
        check(reopened.ready(), "edited copy strictly reopens: " + reopened.diagnostic);
        if (reopened.ready()) {
            auto& r = *reopened.session;
            check(r.view(SlotRef::boxed(0, 2))->nickname == "NEW" && native(reopened).staged().current().header.count == 6 &&
                      ecAt(r, SlotRef::boxed(0, 5)) == ecAt(r, SlotRef::boxed(0, 4)),
                  "edit, clone and compaction survive the round trip");
        }
    }
    check(original == pristine, "source bytes unchanged");
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    const auto built = n.staged().buildWorkingCopy(&error);
    check(built.has_value(), "working copy builds: " + error);
    if (!built) return;
    const auto ops = n.staged().ops();
    check(G7b::verifyWorkingCopy(n.staged().source(), *built, ops, &error), "verifier accepts");
    {
        auto bad = *built;
        bad[0x10] ^= 1;
        Gen7bFixture::refreshCrcs(bad);
        check(!G7b::verifyWorkingCopy(n.staged().source(), bad, ops, &error) && error.find("outside storage") != std::string::npos,
              "a foreign block change is refused even with valid CRCs");
    }
    {
        auto forged = ops;
        PK6::write16(forged.front().plain, 0x08, 26);  // species
        check(!G7b::verifyWorkingCopy(n.staged().source(), *built, forged, &error) && error.find("non-whitelisted") != std::string::npos,
              "a species change is refused even if 'reviewed'");
    }
    {
        std::vector<G7b::Op> partyRelease{{G7b::OpKind::Remove, 1, {}, "forged"}};
        check(!G7b::verifyWorkingCopy(n.staged().source(), *built, partyRelease, &error), "a forged party-member release is refused");
    }
    check(!G7b::verifyWorkingCopy(n.staged().source(), *built, {}, &error) && error.find("differs") != std::string::npos,
          "an image that does not match the reviewed operations is refused");
    {
        auto bad = *built;
        bad.pop_back();
        check(!G7b::verifyWorkingCopy(n.staged().source(), bad, ops, &error), "size change refused");
    }
}

} // namespace

int main() {
    testOpen();
    testViewAndFields();
    testEditsCloneRelease();
    testDraft();
    testExportAndVerifier();
    std::cout << "test_gen7b_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
