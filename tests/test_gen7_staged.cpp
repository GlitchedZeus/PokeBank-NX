// Gen VII staged PK7 Edit / Clone / Remove up to (excluding) the MemeCrypto
// re-sign: unsigned candidate, byte whitelist and tamper rejection.
#include "Integration/Gen7/Gen7StagedSession.h"

#include "fixtures/gen6/Gen6TestSupport.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

namespace Gen7 = PokeVault::Integration::Gen7;
namespace Layout = Gen7::Layout;
namespace PK7 = Gen7::PK7;
using Gen7::Field;
using Gen7::Gen7Save;
using Gen7::Gen7StagedSession;
using Gen7::SlotRef;

// Vendored memecrypto rsa.c leaks mpz temporaries (see docs/gen7/README.md).
extern "C" const char* __lsan_default_suppressions() {
    return "leak:rsa_encrypt\nleak:rsa_decrypt\n";
}

namespace {

Gen7StagedSession session(const std::vector<uint8_t>& bytes) {
    std::string error;
    auto save = Gen7Save::parse(bytes, {}, &error);
    if (!save) {
        std::fprintf(stderr, "fixture parse failed: %s\n", error.c_str());
        std::abort();
    }
    return Gen7StagedSession(std::move(*save));
}

void testCandidate() {
    for (const auto f : {Layout::Family::SunMoon, Layout::Family::UltraSunUltraMoon}) {
        const uint8_t v = f == Layout::Family::SunMoon ? Layout::VersionMoon : Layout::VersionUltraSun;
        const auto original = Gen7Fixture::makeSave({f, v, 2, true, true});
        const auto pristine = original;
        auto s = session(original);
        std::string error;
        const auto box = SlotRef::boxed(31, 29);
        GEN6_CHECK(s.stageValue(box, Field::IV, 31, 4, &error));
        GEN6_CHECK(s.stageValue(box, Field::EV, 252, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::Ball, 26, 0, &error));                 // Beast Ball
        GEN6_CHECK(!s.stageValue(box, Field::Ball, 27, 0, &error));
        GEN6_CHECK(s.stageValue(box, Field::HeldItem, f == Layout::Family::SunMoon ? 920 : 959, 0, &error));
        GEN6_CHECK(!s.stageValue(box, Field::HeldItem, f == Layout::Family::SunMoon ? 921 : 960, 0, &error));
        GEN6_CHECK(s.stageText(box, Field::Nickname, "Rowlet", &error));
        GEN6_CHECK(s.stageText(SlotRef::party(0), Field::OriginalTrainer, "Kahuna", &error));
        GEN6_CHECK(!s.stageValue(SlotRef::party(0), Field::IV, 1, 0, &error));      // party stat input
        GEN6_CHECK(!s.stageValue(box, Field::Species, 1, 0, &error));
        GEN6_CHECK(!s.stageValue(box, Field::Level, 5, 0, &error));
        GEN6_CHECK(s.cloneToBox(SlotRef::party(1), 0, 0, &error));
        GEN6_CHECK(s.stageValue(SlotRef::boxed(0, 0), Field::IV, 0, 0, &error));     // edit a staged clone
        GEN6_CHECK(!s.cloneToBox(SlotRef::party(1), 31, 29, &error));               // occupied/staged target
        GEN6_CHECK(s.removeFromBox(31, 29, &error) == false);                       // already staged
        const auto candidate = s.buildUnsignedCandidate(&error);
        GEN6_CHECK(candidate.has_value());
        if (!candidate) { std::fprintf(stderr, "  %s\n", error.c_str()); continue; }
        GEN6_CHECK(candidate->signaturePending);
        GEN6_CHECK(original == pristine);
        GEN6_CHECK(std::equal(pristine.begin(), pristine.end(), s.source().bytes().begin()));
        // Only staged records and the Party/Box block CRCs changed; the signature region is untouched.
        for (size_t i = 0; i < pristine.size(); ++i) {
            if (pristine[i] == candidate->bytes[i]) continue;
            const auto in = [&](size_t a, size_t n) { return i >= a && i < a + n; };
            GEN6_CHECK(in(Gen7Save::boxRecordOffset(f, 31, 29), Layout::BoxRecordSize) ||
                       in(Gen7Save::boxRecordOffset(f, 0, 0), Layout::BoxRecordSize) ||
                       in(Gen7Save::partyRecordOffset(f, 0), Layout::PartyRecordSize) ||
                       in(Layout::checksumOffset(f, Layout::PartyBlock), 2) ||
                       in(Layout::checksumOffset(f, Layout::BoxBlock), 2));
        }
        // The candidate is CRC-valid but its old signature no longer matches: it is NOT
        // a valid SAV7 until re-signed, so the strict (signature-required) parse refuses it.
        GEN6_CHECK(!Gen7Save::parse(candidate->bytes, {}, &error));
        const auto relaxed = Gen7Save::parse(candidate->bytes, {}, &error, Gen7::SignaturePolicy::Ignore);
        GEN6_CHECK(relaxed && !relaxed->signatureVerified());
        const auto mon = relaxed->box(31, 29).pokemon;
        GEN6_CHECK(mon && mon->iv(4) == 31 && mon->ev(0) == 252 && mon->ball() == 26 && mon->nickname() == u"Rowlet");
        GEN6_CHECK(relaxed->box(0, 0).occupied() && relaxed->box(0, 0).pokemon->iv(0) == 0);
        GEN6_CHECK(relaxed->party(0).pokemon->originalTrainerName() == u"Kahuna");
        GEN6_CHECK(Gen7::verifyUnsignedCandidate(s.source(), candidate->bytes, s.changes(), &error));
        // Test-only: the fixture's PKHeX-style signer makes the image signature-valid again,
        // which shows everything before the signing step is consistent. Not an export path.
        auto signedCopy = candidate->bytes;
        Gen7Fixture::signMeme(signedCopy, f);
        GEN6_CHECK(Gen7Save::parse(signedCopy, s.source().exactGameId(), &error).has_value());
    }
    GEN6_CHECK(!Gen7StagedSession::exportUnsupportedReason().empty());
}

void testRemoveAndTamper() {
    const auto original = Gen7Fixture::makeSave();
    auto s = session(original);
    std::string error;
    GEN6_CHECK(s.removeFromBox(31, 29, &error));
    GEN6_CHECK(!s.removeFromBox(5, 5, &error));                         // empty slot
    GEN6_CHECK(!s.stageValue(SlotRef::boxed(31, 29), Field::IV, 1, 0, &error));
    const auto c = s.buildUnsignedCandidate(&error);
    GEN6_CHECK(c.has_value());
    if (!c) return;
    const auto relaxed = Gen7Save::parse(c->bytes, {}, &error, Gen7::SignaturePolicy::Ignore);
    GEN6_CHECK(relaxed && relaxed->box(31, 29).state == Gen7::SlotState::Empty);
    const auto f = Layout::Family::SunMoon;
    // A byte in another block, re-CRC'd, is refused.
    auto other = c->bytes;
    other[Layout::blocks(f)[Layout::MyStatusBlock].offset + 0x40] ^= 1;
    Gen7Fixture::resignBlocks(other, f);
    GEN6_CHECK(!Gen7::verifyUnsignedCandidate(s.source(), other, s.changes(), &error));
    // Touching the signature region is refused.
    auto sig = c->bytes;
    sig[Layout::signatureOffset(f)] ^= 1;
    GEN6_CHECK(!Gen7::verifyUnsignedCandidate(s.source(), sig, s.changes(), &error));
    // A non-editable PK7 byte inside an edited record, re-CRC'd, is refused.
    auto e = session(original);
    GEN6_CHECK(e.stageValue(SlotRef::boxed(31, 29), Field::Pokerus, 0x11, 0, &error));
    const auto good = e.buildUnsignedCandidate(&error);
    GEN6_CHECK(good.has_value());
    auto changes = e.changes();
    PK7::write16(changes[0].stagedPlain, 0x08, 1);  // species
    PK7::write16(changes[0].stagedPlain, 0x06, PK7::checksum(changes[0].stagedPlain));
    auto forged = good->bytes;
    Gen6Fixture::put(forged, Gen7Save::boxRecordOffset(f, 31, 29), PK7::encrypt(changes[0].stagedPlain));
    Gen7Fixture::resignBlocks(forged, f);
    GEN6_CHECK(!Gen7::verifyUnsignedCandidate(e.source(), forged, changes, &error));
    // Nothing staged -> no candidate.
    auto none = session(original);
    GEN6_CHECK(!none.buildUnsignedCandidate(&error));
}

} // namespace

int main() {
    testCandidate();
    testRemoveAndTamper();
    return Gen6Test::finish("test_gen7_staged");
}
