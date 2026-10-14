// SAV7 (Sun/Moon, Ultra Sun/Ultra Moon) strict read-only parser, MemeCrypto
// signature verification, provider and sources. Block tables and CRC16Invert
// are differentially checked against PKSM-Core (vendor/PKSM-Core @ aa22d7a4).
#include "Integration/Gen7/Gen7ExactFormatEditorProvider.h"
#include "Integration/Gen7/Gen7Save.h"
#include "Integration/Gen7/Gen7SaveSources.h"

#include "fixtures/gen6/Gen6TestSupport.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

#include "sav/SavSUMO.hpp"
#include "sav/SavUSUM.hpp"
#include "utils/crypto.hpp"

#include <random>
#include <string>

namespace Gen7 = PokeVault::Integration::Gen7;
namespace Layout = Gen7::Layout;
namespace PK7 = Gen7::PK7;
namespace Provider = PokeVault::Integration::Gen7EditorProvider;
using Gen7::Gen7Save;

// Vendored FlagBrew/memecrypto@bd3b16ee rsa.c never mpz_clear()s its temporaries, so
// every RSA operation leaks. Scoped to exactly those two vendor functions; any other
// leak (including in PokeBank code) still fails this sanitizer binary.
extern "C" const char* __lsan_default_suppressions() {
    return "leak:rsa_encrypt\nleak:rsa_decrypt\n";
}

namespace {

struct PksmSM : pksm::SavSUMO {
    using pksm::SavSUMO::chkofs;
    using pksm::SavSUMO::chklen;
};
struct PksmUSUM : pksm::SavUSUM {
    using pksm::SavUSUM::chkofs;
    using pksm::SavUSUM::chklen;
};

void testTablesMatchPksm() {
    GEN6_CHECK(Layout::SMBlocks.size() == std::size(PksmSM::chkofs));
    GEN6_CHECK(Layout::USUMBlocks.size() == std::size(PksmUSUM::chkofs));
    for (size_t i = 0; i < Layout::SMBlocks.size(); ++i)
        GEN6_CHECK(Layout::SMBlocks[i].offset == PksmSM::chkofs[i] && Layout::SMBlocks[i].length == PksmSM::chklen[i]);
    for (size_t i = 0; i < Layout::USUMBlocks.size(); ++i)
        GEN6_CHECK(Layout::USUMBlocks[i].offset == PksmUSUM::chkofs[i] &&
                   Layout::USUMBlocks[i].length == PksmUSUM::chklen[i]);
    // PKSM resign(): csoff and memecrypto offsets.
    GEN6_CHECK(Layout::checksumOffset(Layout::Family::SunMoon, 0) == 0x6BC1A);
    GEN6_CHECK(Layout::checksumOffset(Layout::Family::UltraSunUltraMoon, 0) == 0x6CA1A);
    GEN6_CHECK(Layout::signatureOffset(Layout::Family::SunMoon) == 0x6BB00);
    GEN6_CHECK(Layout::signatureOffset(Layout::Family::UltraSunUltraMoon) == 0x6C100);
    GEN6_CHECK(Layout::metadataOffset(Layout::Family::SunMoon) == 0x6BC00);
    GEN6_CHECK(Layout::metadataOffset(Layout::Family::UltraSunUltraMoon) == 0x6CA00);
    for (const auto f : {Layout::Family::SunMoon, Layout::Family::UltraSunUltraMoon}) {
        const auto t = Layout::blocks(f);
        for (size_t i = 0; i + 1 < t.size(); ++i) GEN6_CHECK(t[i].offset + t[i].length <= t[i + 1].offset);
        GEN6_CHECK(t.back().offset + t.back().length <= Layout::metadataOffset(f));
        GEN6_CHECK(t[Layout::BoxBlock].length == 32 * 30 * Layout::BoxRecordSize);
        GEN6_CHECK(t[Layout::PartyBlock].length == 6 * Layout::PartyRecordSize + 4);
        // Every checksum entry lies inside the signed table.
        GEN6_CHECK(Layout::checksumOffset(f, t.size() - 1) + 2 <= Layout::metadataOffset(f) + Layout::signedTableLength(f));
    }
}

void testCrcMatchesPksm() {
    std::mt19937 rng(77);
    for (size_t n : {size_t{0}, size_t{3}, size_t{0x7C}, size_t{0x1000}}) {
        std::vector<uint8_t> data(n);
        for (auto& b : data) b = static_cast<uint8_t>(rng());
        GEN6_CHECK(Utils::crc16Invert(data.data(), data.size()) == pksm::crypto::crc16(data));
    }
}

void testValidParse() {
    struct Case { Layout::Family f; uint8_t v; const char* id; };
    for (const Case c : {Case{Layout::Family::SunMoon, Layout::VersionSun, "sun_3ds"},
                         Case{Layout::Family::SunMoon, Layout::VersionMoon, "moon_3ds"},
                         Case{Layout::Family::UltraSunUltraMoon, Layout::VersionUltraSun, "ultrasun_3ds"},
                         Case{Layout::Family::UltraSunUltraMoon, Layout::VersionUltraMoon, "ultramoon_3ds"}}) {
        const auto bytes = Gen7Fixture::makeSave({c.f, c.v, 2, true, true});
        std::string error;
        const auto save = Gen7Save::parse(bytes, {}, &error);
        GEN6_CHECK(save.has_value());
        if (!save) { std::fprintf(stderr, "  %s\n", error.c_str()); continue; }
        GEN6_CHECK(save->exactGameId() == c.id && save->family() == c.f);
        GEN6_CHECK(save->signatureVerified());
        GEN6_CHECK(save->diagnostics().signatureTail == Gen7::SignatureTail::Zero);
        GEN6_CHECK(save->partyCount() == 2 && save->party(1).pokemon->species() == 723);
        GEN6_CHECK(save->box(31, 29).occupied() && save->box(31, 29).pokemon->species() == 785);
        GEN6_CHECK(save->diagnostics().occupiedBoxRecords == 1 && save->diagnostics().invalidBoxRecords == 0);
        GEN6_CHECK(save->diagnostics().metadataMismatches == 0);
        const auto t = save->trainer();
        GEN6_CHECK(t.rawName == u"ALOLA" && t.tid == 1111 && t.sid == 2222 && t.version == c.v);
        GEN6_CHECK(t.playedHours == 300 && t.playedMinutes == 1 && t.playedSeconds == 2 && t.language == 2);
        GEN6_CHECK(Gen7Save::parse(bytes, c.id).has_value());
        GEN6_CHECK(!Gen7Save::parse(bytes, c.f == Layout::Family::SunMoon ? "ultrasun_3ds" : "sun_3ds"));
        // The block-36 checksum is over the cleared signature region: a raw CRC of the
        // signed block differs, yet the save validates (PKHeX ClearMemeCrypto semantics).
        const auto& b36 = Layout::blocks(c.f)[Layout::MemeCryptoBlock];
        GEN6_CHECK(Utils::crc16Invert(bytes.data() + b36.offset, b36.length) !=
                   PK7::read16(bytes, Layout::checksumOffset(c.f, Layout::MemeCryptoBlock)));
    }
}

void testRejections() {
    std::string error;
    const auto good = Gen7Fixture::makeSave();
    for (size_t size : {size_t{0}, Layout::SMSize - 1, Layout::USUMSize + 1, size_t{0x65600}, size_t{0x100000}})
        GEN6_CHECK(!Gen7Save::parse(std::vector<uint8_t>(size, 0), {}, &error));
    // Wrong version for the family.
    GEN6_CHECK(!Gen7Save::parse(Gen7Fixture::makeSave({Layout::Family::SunMoon, Layout::VersionUltraSun, 2, true, true}), {}, &error));
    GEN6_CHECK(!Gen7Save::parse(Gen7Fixture::makeSave({Layout::Family::UltraSunUltraMoon, Layout::VersionMoon, 2, true, true}), {}, &error));
    // Unsigned (zero signature) saves fail by default; Ignore is diagnostic only.
    const auto unsignedSave = Gen7Fixture::makeSave({Layout::Family::SunMoon, Layout::VersionSun, 2, true, false});
    GEN6_CHECK(!Gen7Save::parse(unsignedSave, {}, &error));
    GEN6_CHECK(error.find("MemeCrypto") != std::string::npos);
    const auto diag = Gen7Save::parse(unsignedSave, {}, &error, Gen7::SignaturePolicy::Ignore);
    GEN6_CHECK(diag && !diag->signatureVerified());
    // Any block edit + CRC resign, without re-signing, breaks the signature.
    auto tampered = good;
    tampered[Layout::blocks(Layout::Family::SunMoon)[Layout::MyStatusBlock].offset + 0x40] ^= 1;
    Gen7Fixture::resignBlocks(tampered, Layout::Family::SunMoon);
    GEN6_CHECK(!Gen7Save::parse(tampered, {}, &error));
    // ... and re-signing makes it valid again (the fixture pipeline round-trips).
    Gen7Fixture::signMeme(tampered, Layout::Family::SunMoon);
    GEN6_CHECK(Gen7Save::parse(tampered, {}, &error).has_value());
    // Stale block CRC.
    for (const size_t id : {size_t{0}, Layout::PartyBlock, Layout::BoxBlock, size_t{35}, Layout::MemeCryptoBlock}) {
        auto bad = good;
        const auto& b = Layout::blocks(Layout::Family::SunMoon)[id];
        bad[b.offset + (id == Layout::MemeCryptoBlock ? 0x10 : b.length / 2)] ^= 0x20;
        GEN6_CHECK(!Gen7Save::parse(bad, {}, &error));
    }
    // A flipped signature byte fails verification.
    auto sig = good;
    sig[Layout::signatureOffset(Layout::Family::SunMoon) + 0x50] ^= 1;
    GEN6_CHECK(!Gen7Save::parse(sig, {}, &error));
    // No BEEF.
    auto beef = good;
    beef[Layout::metadataOffset(Layout::Family::SunMoon) + Layout::MagicWithinMetadata] ^= 1;
    GEN6_CHECK(!Gen7Save::parse(beef, {}, &error));
    // Party count > 6 (resigned + signed so only the structural rule fails).
    auto party = good;
    party[Gen7Save::partyCountOffset(Layout::Family::SunMoon)] = 7;
    Gen7Fixture::resignBlocks(party, Layout::Family::SunMoon);
    Gen7Fixture::signMeme(party, Layout::Family::SunMoon);
    GEN6_CHECK(!Gen7Save::parse(party, {}, &error));
    // Corrupt box record is quarantined.
    auto box = good;
    box[Gen7Save::boxRecordOffset(Layout::Family::SunMoon, 0, 0) + 0x30] ^= 1;
    Gen7Fixture::resignBlocks(box, Layout::Family::SunMoon);
    Gen7Fixture::signMeme(box, Layout::Family::SunMoon);
    const auto q = Gen7Save::parse(box, {}, &error);
    GEN6_CHECK(q && q->diagnostics().invalidBoxRecords == 1);
}

void testProviderAndSources() {
    using Field = Provider::Field;
    using State = Provider::State;
    GEN6_CHECK(!Provider::StagedEditingSupported);
    GEN6_CHECK(!Provider::descriptorForSource("x_3ds"));
    const auto d = Provider::descriptorForSource("moon_3ds");
    GEN6_CHECK(d.has_value());
    for (int i = 0; i <= static_cast<int>(Field::FatefulEncounter); ++i) {
        const auto f = static_cast<Field>(i);
        GEN6_CHECK(Provider::fieldState(f) != State::Editable);
        GEN6_CHECK(d && !d->fieldIsEditorTarget(f));
    }
    GEN6_CHECK(Provider::fieldState(Field::Shiny) == State::Derived);
    GEN6_CHECK(Provider::fieldState(Field::AV) == State::Hidden);
    GEN6_CHECK(Provider::fieldState(Field::IV) == State::ReadOnly);
    GEN6_CHECK(d && !d->source.saveOperations.supports(PokeVault::SaveEdit::Capability::PokemonEditing));

    namespace S = Gen7::Sources;
    GEN6_CHECK(S::checkpointFolderPrefix("sun_3ds") == "0x01648 ");
    GEN6_CHECK(S::checkpointFolderPrefix("moon_3ds") == "0x0175E ");
    GEN6_CHECK(S::checkpointFolderPrefix("ultrasun_3ds") == "0x01B50 ");
    GEN6_CHECK(S::checkpointFolderPrefix("ultramoon_3ds") == "0x01B51 ");
    GEN6_CHECK(S::gameIdFromCheckpointFolder("0x01B51 Pokemon Ultra Moon") == "ultramoon_3ds");
    GEN6_CHECK(S::gameIdFromCheckpointFolder("0x0055D Pokemon X").empty());
    GEN6_CHECK(S::emulatorSaveFile("sdmc:/switch/dekopon/", "sun_3ds") ==
               "sdmc:/switch/dekopon/sdmc/Nintendo 3DS/00000000000000000000000000000000/"
               "00000000000000000000000000000000/title/00040000/00164800/data/00000001/main");

    const auto bytes = Gen7Fixture::makeSave({Layout::Family::UltraSunUltraMoon, Layout::VersionUltraMoon, 1, true, true});
    S::SourceContext ctx{S::ProviderId::DekoponEmulator, "ultramoon_3ds", "ultramoon_3ds", "sdmc:/x/main", "id", {}};
    auto p = S::probeSave(bytes, ctx);
    GEN6_CHECK(p.ready() && p.instance.readOnly() && p.instance.generation == 7 && p.instance.trainerName == "ALOLA");
    GEN6_CHECK(p.instance.diagnostic.find("staged export unsupported") != std::string::npos);
    ctx.assignedExactGame = "ultrasun_3ds";
    p = S::probeSave(bytes, ctx);
    GEN6_CHECK(p.instance.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch);
    ctx.assignedExactGame = "ultramoon_3ds";
    ctx.provider = S::ProviderId::AzaharEmulator;
    GEN6_CHECK(S::probeSave(bytes, ctx).instance.validation == PokeVault::Source::ValidationStatus::Unsupported);
    ctx.provider = S::ProviderId::ManualLink;
    auto bad = bytes;
    bad[10] ^= 1;
    GEN6_CHECK(S::probeSave(bad, ctx).instance.validation == PokeVault::Source::ValidationStatus::Invalid);
}

} // namespace

int main() {
    testTablesMatchPksm();
    testCrcMatchesPksm();
    testValidParse();
    testRejections();
    testProviderAndSources();
    return Gen6Test::finish("test_gen7_save");
}
