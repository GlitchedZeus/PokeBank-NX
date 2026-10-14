// Staged PK6 Edit / Clone / Remove on an app-owned SAV6 copy, byte whitelist,
// strict reparse and verified working-copy export (VerifiedStagedExport).
#include "Integration/Gen6/Gen6StagedSession.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace Gen6 = PokeVault::Integration::Gen6;
namespace Layout = Gen6::Layout;
namespace PK6 = Gen6::PK6;
using Gen6::Field;
using Gen6::Gen6Save;
using Gen6::Gen6StagedSession;
using Gen6::SlotRef;

namespace {

Gen6StagedSession session(const std::vector<uint8_t>& bytes) {
    auto save = Gen6Save::parse(bytes);
    if (!save) std::abort();
    return Gen6StagedSession(std::move(*save));
}

// Every differing byte must lie in one of the given [offset, offset+len) ranges.
bool diffWithin(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b,
                const std::vector<std::pair<size_t, size_t>>& ranges, size_t* changed = nullptr) {
    size_t n = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == b[i]) continue;
        ++n;
        bool ok = false;
        for (const auto& [o, l] : ranges) ok |= i >= o && i < o + l;
        if (!ok) {
            std::fprintf(stderr, "  unexpected change at 0x%zx\n", i);
            return false;
        }
    }
    if (changed) *changed = n;
    return true;
}

std::pair<size_t, size_t> checksumRange(Layout::Family f, size_t block) {
    return {Layout::checksumOffset(f, block), 2};
}

void testBoxEditWhitelist() {
    for (const auto family : {Layout::Family::XY, Layout::Family::ORAS}) {
        const uint8_t version = family == Layout::Family::XY ? Layout::VersionY : Layout::VersionAlphaSapphire;
        const auto original = Gen6Fixture::makeSave({family, version, 2, true});
        const auto pristine = original;
        auto s = session(original);
        std::string error;
        const auto box = SlotRef::boxed(0, 1);
        GEN6_CHECK(s.stageValue(box, Field::IV, 31, 2, &error));
        GEN6_CHECK(s.stageValue(box, Field::EV, 252, 3, &error));
        GEN6_CHECK(s.stageValue(box, Field::Nature, 24, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::HeldItem, family == Layout::Family::XY ? 717 : 775, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::Ball, 25, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::TrainerId, 1, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::SecretId, 2, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::Pokerus, 0x21, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::FatefulEncounter, 1, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::OriginalTrainerGender, 1, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::Friendship, 255, 0, &error));
        GEN6_CHECK(s.stageText(box, Field::Nickname, "Sparky", &error));
        GEN6_CHECK(s.stageText(box, Field::OriginalTrainer, "Kalos Kid", &error));
        GEN6_CHECK(s.changeCount() == 1);

        const auto edited = s.buildWorkingCopy(&error);
        GEN6_CHECK(edited.has_value());
        if (!edited) { std::fprintf(stderr, "  %s\n", error.c_str()); continue; }
        // Source never modified (both the caller's buffer and the session's copy).
        GEN6_CHECK(original == pristine);
        GEN6_CHECK(std::equal(pristine.begin(), pristine.end(), s.source().bytes().begin()));
        // Only the one record and the box block checksum changed.
        size_t changed = 0;
        GEN6_CHECK(diffWithin(pristine, *edited,
                              {{Gen6Save::boxRecordOffset(family, 0, 1), PK6::StoredSize},
                               checksumRange(family, Layout::boxBlock(family))}, &changed));
        GEN6_CHECK(changed > 0);
        // Strict reparse and field values.
        const auto re = Gen6Save::parse(*edited, s.source().exactGameId(), &error);
        GEN6_CHECK(re.has_value());
        if (!re) continue;
        const auto mon = re->box(0, 1).pokemon;
        GEN6_CHECK(mon && mon->iv(2) == 31 && mon->ev(3) == 252 && mon->nature() == 24);
        GEN6_CHECK(mon && mon->ball() == 25 && mon->tid() == 1 && mon->sid() == 2 && mon->pokerus() == 0x21);
        GEN6_CHECK(mon && mon->fatefulEncounter() && mon->originalTrainerGender() == 1);
        GEN6_CHECK(mon && mon->originalTrainerFriendship() == 255);
        GEN6_CHECK(mon && mon->nickname() == u"Sparky" && mon->isNicknamed());
        GEN6_CHECK(mon && mon->originalTrainerName() == u"Kalos Kid");
        // Everything else in that record is untouched.
        const auto before = *Gen6Save::parse(pristine)->box(0, 1).pokemon;
        GEN6_CHECK(mon && mon->species() == before.species() && mon->pid() == before.pid() &&
                   mon->experience() == before.experience() && mon->move(0) == before.move(0) &&
                   mon->form() == before.form() && mon->metLocation() == before.metLocation());
        // Other records are byte-identical.
        GEN6_CHECK(std::equal(re->box(0, 0).encrypted.begin(), re->box(0, 0).encrypted.end(),
                              Gen6Save::parse(pristine)->box(0, 0).encrypted.begin()));
        GEN6_CHECK(Gen6::verifyWorkingCopy(s.source(), *edited, s.changes(), &error));
    }
}

void testPartyEditRules() {
    const auto original = Gen6Fixture::makeSave();
    auto s = session(original);
    std::string error;
    const auto p = SlotRef::party(1);
    // Non-stat edits leave the cached party block untouched (stat edits: test_gen6_stats).
    GEN6_CHECK(s.stageValue(p, Field::Friendship, 200, 0, &error));
    GEN6_CHECK(s.stageText(p, Field::Nickname, "Bud", &error));
    const auto edited = s.buildWorkingCopy(&error);
    GEN6_CHECK(edited.has_value());
    if (!edited) return;
    GEN6_CHECK(diffWithin(original, *edited,
                          {{Gen6Save::partyRecordOffset(1), PK6::PartySize},
                           checksumRange(Layout::Family::XY, Layout::PartyBlock)}));
    const auto re = Gen6Save::parse(*edited);
    GEN6_CHECK(re && re->partyCount() == 2);
    const auto mon = re->party(1).pokemon;
    GEN6_CHECK(mon && mon->partyForm() && mon->originalTrainerFriendship() == 200 && mon->nickname() == u"Bud");
    // Cached party stats preserved byte-for-byte (no stat input changed).
    const auto old = Gen6Save::parse(original)->party(1).pokemon;
    GEN6_CHECK(mon && old && std::equal(mon->plain().begin() + PK6::StoredSize, mon->plain().end(),
                                        old->plain().begin() + PK6::StoredSize));
}

void testValueValidation() {
    const auto xy = Gen6Fixture::makeSave();
    auto s = session(xy);
    std::string error;
    const auto b = SlotRef::boxed(0, 0);
    GEN6_CHECK(!s.stageValue(b, Field::HeldItem, 718, 0, &error));   // XY max 717
    GEN6_CHECK(!s.stageValue(b, Field::IV, 32, 0, &error));
    GEN6_CHECK(!s.stageValue(b, Field::IV, 1, 6, &error));
    GEN6_CHECK(!s.stageValue(b, Field::EV, 253, 0, &error));
    GEN6_CHECK(s.stageValue(b, Field::EV, 252, 0, &error));
    GEN6_CHECK(s.stageValue(b, Field::EV, 252, 1, &error));
    GEN6_CHECK(!s.stageValue(b, Field::EV, 7, 2, &error));           // 252+252+7 > 510
    GEN6_CHECK(s.stageValue(b, Field::EV, 6, 2, &error));            // exactly 510
    GEN6_CHECK(!s.stageValue(b, Field::Nature, 25, 0, &error));
    GEN6_CHECK(!s.stageValue(b, Field::Ball, 0, 0, &error));
    GEN6_CHECK(!s.stageValue(b, Field::Ball, 26, 0, &error));
    GEN6_CHECK(!s.stageValue(b, Field::TrainerId, 0x10000, 0, &error));
    GEN6_CHECK(!s.stageValue(b, Field::FatefulEncounter, 2, 0, &error));
    // ReadOnly / Derived / Hidden fields are never staged.
    for (const auto f : {Field::Species, Field::Form, Field::Gender, Field::Shiny, Field::CalculatedStats,
                         Field::Experience, Field::Ability, Field::PersonalityId, Field::Move,
                         Field::MovePP, Field::MetLocation, Field::Language, Field::DV, Field::AV})
        GEN6_CHECK(!s.stageValue(b, f, 1, 0, &error));
    // Text rules.
    GEN6_CHECK(!s.stageText(b, Field::Nickname, "", &error));
    GEN6_CHECK(!s.stageText(b, Field::Nickname, "   ", &error));
    GEN6_CHECK(!s.stageText(b, Field::Nickname, "ThirteenChars", &error));
    GEN6_CHECK(!s.stageText(b, Field::Nickname, "Caf\xC3\xA9", &error));
    GEN6_CHECK(!s.stageText(b, Field::HeldItem, "x", &error));
    // Empty / invalid / out-of-range targets.
    GEN6_CHECK(!s.stageValue(SlotRef::boxed(0, 5), Field::IV, 1, 0, &error));
    GEN6_CHECK(!s.stageValue(SlotRef::party(4), Field::Friendship, 1, 0, &error));
    GEN6_CHECK(!s.stageValue(SlotRef::boxed(31, 0), Field::IV, 1, 0, &error));
    GEN6_CHECK(!s.stageValue(SlotRef{Gen6::RecordRegion::Party, 1, 0}, Field::Friendship, 1, 0, &error));

    // ORAS accepts its larger item range.
    auto oras = session(Gen6Fixture::makeSave({Layout::Family::ORAS, Layout::VersionOmegaRuby, 2, true}));
    GEN6_CHECK(oras.stageValue(b, Field::HeldItem, 775, 0, &error));
    GEN6_CHECK(!oras.stageValue(b, Field::HeldItem, 776, 0, &error));

    // Handler friendship and egg nicknames are explicitly unsupported.
    auto bytes = Gen6Fixture::makeSave();
    Gen6Fixture::MonSpec traded;
    traded.species = 300;
    traded.handler = 1;
    Gen6Fixture::put(bytes, Gen6Save::boxRecordOffset(Layout::Family::XY, 2, 0), Gen6Fixture::encryptedMon(traded, false));
    Gen6Fixture::MonSpec egg;
    egg.species = 301;
    egg.egg = true;
    Gen6Fixture::put(bytes, Gen6Save::boxRecordOffset(Layout::Family::XY, 2, 1), Gen6Fixture::encryptedMon(egg, false));
    Gen6Fixture::resignAll(bytes, Layout::Family::XY);
    auto t = session(bytes);
    GEN6_CHECK(!t.stageValue(SlotRef::boxed(2, 0), Field::Friendship, 1, 0, &error));
    GEN6_CHECK(!t.stageText(SlotRef::boxed(2, 1), Field::Nickname, "Eggy", &error));
    GEN6_CHECK(t.stageText(SlotRef::boxed(2, 1), Field::OriginalTrainer, "Eggy", &error));

    // Reverting an edit removes the pending change.
    auto r = session(xy);
    const auto old = r.current(b)->iv(0);
    GEN6_CHECK(r.stageValue(b, Field::IV, old == 0 ? 1 : 0, 0, &error) && r.hasChanges());
    GEN6_CHECK(r.stageValue(b, Field::IV, old, 0, &error) && !r.hasChanges());
    GEN6_CHECK(!r.buildWorkingCopy(&error));  // nothing staged -> no working copy
    // Create is explicitly unsupported.
    GEN6_CHECK(Gen6StagedSession::createCapability().support == Gen6::CreateSupport::Unsupported);
}

void testCloneAndRemove() {
    const auto original = Gen6Fixture::makeSave();
    auto s = session(original);
    std::string error;
    // Party -> blank box: boxed 232-byte form of the donor.
    GEN6_CHECK(s.cloneToBox(SlotRef::party(0), 3, 4, &error));
    GEN6_CHECK(s.cloneToBox(SlotRef::boxed(0, 0), 3, 5, &error));
    GEN6_CHECK(!s.cloneToBox(SlotRef::boxed(0, 1), 0, 0, &error));   // destination occupied
    GEN6_CHECK(!s.cloneToBox(SlotRef::boxed(0, 0), 3, 4, &error));   // destination already staged
    GEN6_CHECK(!s.cloneToBox(SlotRef::boxed(0, 9), 3, 6, &error));   // empty donor
    GEN6_CHECK(!s.cloneToBox(SlotRef::boxed(0, 2), 3, 7, &error));   // Furfrou form 2 into a box
    GEN6_CHECK(!s.cloneToBox(SlotRef::boxed(0, 0), 31, 0, &error));
    // A staged clone may then be edited inside the boxed edit mask.
    GEN6_CHECK(s.stageValue(SlotRef::boxed(3, 5), Field::IV, 0, 0, &error));
    // Remove: boxed only, occupied only.
    GEN6_CHECK(s.removeFromBox(0, 1, &error));
    GEN6_CHECK(!s.removeFromBox(0, 1, &error));
    GEN6_CHECK(!s.removeFromBox(0, 8, &error));
    GEN6_CHECK(!s.stageValue(SlotRef::boxed(0, 1), Field::IV, 1, 0, &error));  // removed slot

    const auto edited = s.buildWorkingCopy(&error);
    GEN6_CHECK(edited.has_value());
    if (!edited) { std::fprintf(stderr, "  %s\n", error.c_str()); return; }
    const auto f = Layout::Family::XY;
    GEN6_CHECK(diffWithin(original, *edited,
                          {{Gen6Save::boxRecordOffset(f, 3, 4), PK6::StoredSize},
                           {Gen6Save::boxRecordOffset(f, 3, 5), PK6::StoredSize},
                           {Gen6Save::boxRecordOffset(f, 0, 1), PK6::StoredSize},
                           checksumRange(f, Layout::boxBlock(f))}));
    const auto re = Gen6Save::parse(*edited);
    GEN6_CHECK(re.has_value());
    if (!re) return;
    const auto src = Gen6Save::parse(original);
    const auto c1 = re->box(3, 4).pokemon;
    GEN6_CHECK(c1 && !c1->partyForm() && c1->species() == src->party(0).pokemon->species());
    GEN6_CHECK(c1 && std::equal(c1->plain().begin(), c1->plain().end(), src->party(0).pokemon->plain().begin()));
    GEN6_CHECK(re->box(3, 5).pokemon->iv(0) == 0);
    GEN6_CHECK(re->box(0, 1).blank());
    GEN6_CHECK(re->partyCount() == 2);
    GEN6_CHECK(re->diagnostics().occupiedBoxRecords == 3 + 2 - 1);
    // Undo restores nothing-staged for that slot.
    GEN6_CHECK(s.undo(SlotRef::boxed(0, 1)) && s.changeCount() == 2);
    GEN6_CHECK(s.changeLines().size() >= 2);
}

void testVerifierRejectsTampering() {
    const auto original = Gen6Fixture::makeSave();
    auto s = session(original);
    std::string error;
    GEN6_CHECK(s.stageValue(SlotRef::boxed(0, 0), Field::IV, 3, 0, &error));
    const auto good = *s.buildWorkingCopy(&error);
    const auto changes = s.changes();
    const auto f = Layout::Family::XY;
    GEN6_CHECK(Gen6::verifyWorkingCopy(s.source(), good, changes, &error));

    // 1) A byte in another block, even when that block is re-signed.
    auto other = good;
    other[Layout::MyStatusOffset + 0x30] ^= 1;
    Gen6Fixture::resignAll(other, f);
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), other, changes, &error));
    // 2) A different box record in the same block, re-signed.
    auto neighbour = good;
    auto plain = Gen6Fixture::plainMon(Gen6Fixture::MonSpec{}, false);
    Gen6Fixture::put(neighbour, Gen6Save::boxRecordOffset(f, 9, 9), PK6::encrypt(plain));
    Gen6Fixture::resignAll(neighbour, f);
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), neighbour, changes, &error));
    // 3) The staged record itself, changed at a non-editable field (species), re-signed.
    auto species = good;
    auto rec = *Gen6Save::parse(good)->box(0, 0).pokemon;
    std::vector<uint8_t> p(rec.plain().begin(), rec.plain().end());
    PK6::write16(p, 0x08, 151);
    PK6::write16(p, 0x06, PK6::checksum(p));
    Gen6Fixture::put(species, Gen6Save::boxRecordOffset(f, 0, 0), PK6::encrypt(p));
    Gen6Fixture::resignAll(species, f);
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), species, changes, &error));
    // 4) A stale checksum (not re-signed) fails the strict reparse.
    auto stale = good;
    stale[Gen6Save::boxRecordOffset(f, 0, 0) + 0x10] ^= 1;
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), stale, changes, &error));
    // 5) Size changes are refused.
    auto shorter = good;
    shorter.pop_back();
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), shorter, changes, &error));
    // 6) An unchanged image does not satisfy a staged change.
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), original, changes, &error));
    // 7) A change that claims a party-region field outside the party mask.
    auto partySession = session(original);
    GEN6_CHECK(partySession.stageValue(SlotRef::party(0), Field::Friendship, 1, 0, &error));
    auto forged = partySession.changes();
    PK6::write32(forged[0].stagedPlain, 0x74, PK6::read32(forged[0].stagedPlain, 0x74) ^ 1);  // IV
    PK6::write16(forged[0].stagedPlain, 0x06, PK6::checksum(forged[0].stagedPlain));
    auto forgedBytes = original;
    Gen6Fixture::put(forgedBytes, Gen6Save::partyRecordOffset(0), PK6::encrypt(forged[0].stagedPlain));
    Gen6Fixture::resignAll(forgedBytes, f);
    GEN6_CHECK(!Gen6::verifyWorkingCopy(partySession.source(), forgedBytes, forged, &error));
}

std::string makeTempDir() {
    char pattern[] = "/tmp/gen6-export-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
size_t countEntries(const std::string& dir) {
    size_t n = 0;
    if (DIR* d = ::opendir(dir.c_str())) {
        while (const dirent* e = ::readdir(d)) if (std::string(e->d_name) != "." && std::string(e->d_name) != "..") ++n;
        ::closedir(d);
    }
    return n;
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void testVerifiedExport() {
    const auto original = Gen6Fixture::makeSave({Layout::Family::ORAS, Layout::VersionOmegaRuby, 2, true});
    const auto pristine = original;
    auto s = session(original);
    std::string error;
    GEN6_CHECK(s.stageValue(SlotRef::boxed(0, 0), Field::EV, 100, 5, &error));
    GEN6_CHECK(s.cloneToBox(SlotRef::party(1), 10, 10, &error));
    const std::string root = makeTempDir();
    GEN6_CHECK(!root.empty());

    // Fault injection at every stage leaves no partial export behind.
    for (const auto point : {PokeVault::Integration::Common::ExportFailurePoint::AfterBackupWrite,
                             PokeVault::Integration::Common::ExportFailurePoint::AfterEditedWrite,
                             PokeVault::Integration::Common::ExportFailurePoint::AfterReadbackVerify,
                             PokeVault::Integration::Common::ExportFailurePoint::AfterManifestWrite}) {
        Gen6::Gen6ExportRequest req{root, "sdmc:/3ds/Checkpoint/saves/0x011C4 OR/b1/main", "20261014_120000", "abc"};
        req.failurePoint = point;
        const auto failed = s.exportWorkingCopy(req);
        GEN6_CHECK(!failed.success);
        GEN6_CHECK(countEntries(root) == 0);
    }

    const auto result = s.exportWorkingCopy(
        {root, "sdmc:/3ds/Checkpoint/saves/0x011C4 OR/b1/main", "20261014_120000", "abc"});
    GEN6_CHECK(result.success);
    if (!result.success) { std::fprintf(stderr, "  export error: %s\n", result.error.c_str()); return; }
    GEN6_CHECK(original == pristine);
    const auto backup = slurp(result.directory + "/original_backup.sav");
    const auto edited = slurp(result.directory + "/edited.sav");
    GEN6_CHECK(backup == pristine);
    GEN6_CHECK(edited.size() == Layout::ORASSize && edited != pristine);
    const auto re = Gen6Save::parse(edited, "omegaruby_3ds", &error);
    GEN6_CHECK(re && re->box(0, 0).pokemon->ev(5) == 100 && re->box(10, 10).occupied());
    GEN6_CHECK(Gen6::verifyWorkingCopy(s.source(), edited, s.changes(), &error));
    const auto manifest = slurp(result.directory + "/EDIT_MANIFEST.txt");
    const std::string text(manifest.begin(), manifest.end());
    GEN6_CHECK(text.find("GAME_ID=omegaruby_3ds") != std::string::npos);
    GEN6_CHECK(text.find("Generation VI") != std::string::npos);
    GEN6_CHECK(text.find("LIVE_EMULATOR_WRITE=DISABLED") != std::string::npos);
    GEN6_CHECK(text.find("Box 11 slot 11: clone of Party slot 2") != std::string::npos);
    // A second export never overwrites the first.
    const auto second = s.exportWorkingCopy({root, "manual", "20261014_120000", "abc"});
    GEN6_CHECK(second.success && second.directory != result.directory);
    GEN6_CHECK(slurp(result.directory + "/edited.sav") == edited);
    [[maybe_unused]] const int removed = std::system(("rm -rf '" + root + "'").c_str());
}

} // namespace

int main() {
    testBoxEditWhitelist();
    testPartyEditRules();
    testValueValidation();
    testCloneAndRemove();
    testVerifierRejectsTampering();
    testVerifiedExport();
    return Gen6Test::finish("test_gen6_staged");
}
