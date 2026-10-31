// SAV6 strict parser: XY vs ORAS identity, BEEF footer, per-block CRC16-CCITT,
// party/box structure and diagnostics. The block tables and checksum algorithm
// are differentially checked against PKSM-Core (vendor/PKSM-Core @ aa22d7a4).
#include "Integration/Gen6/Gen6Save.h"
#include "Utils/CRC16.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

#include "sav/SavORAS.hpp"
#include "sav/SavXY.hpp"
#include "utils/crypto.hpp"

#include <random>
#include <string>

namespace Gen6 = PokeVault::Integration::Gen6;
namespace Layout = Gen6::Layout;
namespace PK6 = Gen6::PK6;
using Gen6::Gen6Save;

namespace {

struct PksmXY : pksm::SavXY {
    using pksm::SavXY::chkofs;
    using pksm::SavXY::chklen;
};
struct PksmORAS : pksm::SavORAS {
    using pksm::SavORAS::chkofs;
    using pksm::SavORAS::chklen;
};

void testTablesMatchPksm() {
    GEN6_CHECK(Layout::XYBlocks.size() == std::size(PksmXY::chkofs));
    GEN6_CHECK(Layout::ORASBlocks.size() == std::size(PksmORAS::chkofs));
    for (size_t i = 0; i < Layout::XYBlocks.size(); ++i) {
        GEN6_CHECK(Layout::XYBlocks[i].offset == PksmXY::chkofs[i]);
        GEN6_CHECK(Layout::XYBlocks[i].length == PksmXY::chklen[i]);
    }
    for (size_t i = 0; i < Layout::ORASBlocks.size(); ++i) {
        GEN6_CHECK(Layout::ORASBlocks[i].offset == PksmORAS::chkofs[i]);
        GEN6_CHECK(Layout::ORASBlocks[i].length == PksmORAS::chklen[i]);
    }
    // PKSM resign(): csoff 0x6541A / 0x75E1A, stride 8 -- same as PKHeX BlockInfo3DS.
    GEN6_CHECK(Layout::checksumOffset(Layout::Family::XY, 0) == 0x6541A);
    GEN6_CHECK(Layout::checksumOffset(Layout::Family::ORAS, 0) == 0x75E1A);
    GEN6_CHECK(Layout::checksumOffset(Layout::Family::XY, 54) == 0x6541A + 54 * 8);
    // Blocks are ordered, disjoint and end before the metadata footer.
    for (const auto f : {Layout::Family::XY, Layout::Family::ORAS}) {
        const auto t = Layout::blocks(f);
        for (size_t i = 0; i + 1 < t.size(); ++i) GEN6_CHECK(t[i].offset + t[i].length <= t[i + 1].offset);
        GEN6_CHECK(t.back().offset + t.back().length <= Layout::metadataOffset(f));
        // Party and box storage are wholly owned by the cited blocks.
        GEN6_CHECK(Gen6Save::owningBlock(f, Layout::PartyOffset, 6 * PK6::PartySize + 1) == Layout::PartyBlock);
        GEN6_CHECK(Gen6Save::owningBlock(f, Layout::boxOffset(f), 31 * 30 * PK6::StoredSize) ==
                   Layout::boxBlock(f));
        GEN6_CHECK(t[Layout::boxBlock(f)].length == 31 * 30 * PK6::StoredSize);
        GEN6_CHECK(t[Layout::MyStatusBlock].offset == Layout::MyStatusOffset);
    }
}

void testCrcMatchesPksm() {
    std::mt19937 rng(66);
    for (size_t n : {size_t{0}, size_t{1}, size_t{7}, size_t{0x2C8}, size_t{0x1000}}) {
        std::vector<uint8_t> data(n);
        for (auto& b : data) b = static_cast<uint8_t>(rng());
        GEN6_CHECK(Utils::crc16ccitt(data.data(), data.size()) == pksm::crypto::ccitt16(data));
    }
}

void testValidParse() {
    struct Case { Layout::Family family; uint8_t version; const char* id; };
    for (const Case c : {Case{Layout::Family::XY, Layout::VersionX, "x_3ds"},
                         Case{Layout::Family::XY, Layout::VersionY, "y_3ds"},
                         Case{Layout::Family::ORAS, Layout::VersionOmegaRuby, "omegaruby_3ds"},
                         Case{Layout::Family::ORAS, Layout::VersionAlphaSapphire, "alphasapphire_3ds"}}) {
        const auto bytes = Gen6Fixture::makeSave({c.family, c.version, 2, true});
        std::string error;
        const auto save = Gen6Save::parse(bytes, {}, &error);
        GEN6_CHECK(save.has_value());
        if (!save) { std::fprintf(stderr, "  parse error: %s\n", error.c_str()); continue; }
        GEN6_CHECK(save->family() == c.family);
        GEN6_CHECK(save->exactGameId() == c.id);
        GEN6_CHECK(save->partyCount() == 2);
        GEN6_CHECK(save->diagnostics().occupiedBoxRecords == 3);
        GEN6_CHECK(save->diagnostics().emptyBoxRecords == 31 * 30 - 3);
        GEN6_CHECK(save->diagnostics().invalidBoxRecords == 0);
        GEN6_CHECK(save->diagnostics().metadataMismatches == 0);
        GEN6_CHECK(Gen6Save::parse(bytes, c.id).has_value());
        GEN6_CHECK(!Gen6Save::parse(bytes, c.family == Layout::Family::XY ? "omegaruby_3ds" : "x_3ds", &error));
        const auto t = save->trainer();
        GEN6_CHECK(t.rawName == u"TESTER" && t.tid == 12345 && t.sid == 54321 && t.version == c.version);
        GEN6_CHECK(t.playedHours == 42 && t.playedMinutes == 7 && t.playedSeconds == 9);
        const auto p0 = save->party(0);
        GEN6_CHECK(p0.occupied() && p0.pokemon && p0.pokemon->partyForm() && p0.pokemon->species() == 1);
        GEN6_CHECK(save->party(2).state == Gen6::SlotState::Empty);
        GEN6_CHECK(save->box(0, 2).pokemon->species() == 676);
        GEN6_CHECK(save->box(0, 3).blank());
        GEN6_CHECK(save->box(30, 29).blank());
        GEN6_CHECK(save->box(31, 0).state == Gen6::SlotState::Invalid);  // out of range -> no record
        // The parse holds its own copy: the bytes are identical, not aliased.
        GEN6_CHECK(save->bytes().data() != bytes.data());
        GEN6_CHECK(std::equal(bytes.begin(), bytes.end(), save->bytes().begin()));
    }
}

void testStrictRejections() {
    std::string error;
    const auto good = Gen6Fixture::makeSave();

    // Unknown sizes and the ORAS demo.
    for (size_t size : {size_t{0}, Layout::XYSize - 1, Layout::XYSize + 1, Layout::ORASSize - 0x200,
                        size_t{0x80000}, size_t{0x6CC00}}) {
        std::vector<uint8_t> bytes(size, 0);
        GEN6_CHECK(!Gen6Save::parse(bytes, {}, &error));
    }
    GEN6_CHECK(!Gen6Save::parse(std::vector<uint8_t>(Layout::ORASDemoSize, 0), {}, &error));
    GEN6_CHECK(error.find("unsupported") != std::string::npos);

    // XY-sized data claiming an ORAS version (and vice versa) is rejected.
    GEN6_CHECK(!Gen6Save::parse(Gen6Fixture::makeSave({Layout::Family::XY, Layout::VersionOmegaRuby, 2, true}), {}, &error));
    GEN6_CHECK(!Gen6Save::parse(Gen6Fixture::makeSave({Layout::Family::ORAS, Layout::VersionX, 2, true}), {}, &error));
    GEN6_CHECK(!Gen6Save::parse(Gen6Fixture::makeSave({Layout::Family::XY, 30, 2, true}), {}, &error));

    // Missing BEEF footer.
    auto noBeef = good;
    noBeef[Layout::metadataOffset(Layout::Family::XY) + Layout::MagicWithinMetadata] ^= 1;
    GEN6_CHECK(!Gen6Save::parse(noBeef, {}, &error));

    // Any single block with a stale checksum rejects the whole save.
    for (size_t id = 0; id < Layout::XYBlocks.size(); ++id) {
        auto bad = good;
        bad[Layout::XYBlocks[id].offset + Layout::XYBlocks[id].length / 2] ^= 0x40;
        GEN6_CHECK(!Gen6Save::parse(bad, {}, &error));
        GEN6_CHECK(Gen6Save::validBlockCount(bad, Layout::Family::XY) == Layout::XYBlocks.size() - 1);
    }
    // Unsigned saves (block checksums never written) fail.
    GEN6_CHECK(!Gen6Save::parse(Gen6Fixture::makeSave({Layout::Family::ORAS, Layout::VersionOmegaRuby, 2, false}), {}, &error));

    // Party count above six.
    auto many = good;
    many[Gen6Save::partyCountOffset()] = 7;
    Gen6Fixture::resignAll(many, Layout::Family::XY);
    GEN6_CHECK(!Gen6Save::parse(many, {}, &error));

    // A declared party slot that is empty or corrupt.
    auto declaredEmpty = good;
    declaredEmpty[Gen6Save::partyCountOffset()] = 3;
    Gen6Fixture::resignAll(declaredEmpty, Layout::Family::XY);
    GEN6_CHECK(!Gen6Save::parse(declaredEmpty, {}, &error));
    auto corruptParty = good;
    corruptParty[Gen6Save::partyRecordOffset(1) + 0x30] ^= 0x10;
    Gen6Fixture::resignAll(corruptParty, Layout::Family::XY);
    GEN6_CHECK(!Gen6Save::parse(corruptParty, {}, &error));
    // A boxed-size record in a party slot is not a party record.
    auto boxedInParty = good;
    auto boxedPlain = Gen6Fixture::plainMon(Gen6Fixture::MonSpec{}, false);
    boxedPlain.resize(PK6::PartySize, 0);  // stats zero but checksum still valid
    Gen6Fixture::put(boxedInParty, Gen6Save::partyRecordOffset(0), PK6::encrypt(boxedPlain));
    Gen6Fixture::resignAll(boxedInParty, Layout::Family::XY);
    GEN6_CHECK(Gen6Save::parse(boxedInParty, {}, &error).has_value());  // 260-byte form is valid PK6

    // A corrupt box slot is quarantined, not fatal, and counted.
    auto corruptBox = good;
    corruptBox[Gen6Save::boxRecordOffset(Layout::Family::XY, 5, 5) + 0x20] ^= 0x01;
    Gen6Fixture::resignAll(corruptBox, Layout::Family::XY);
    const auto q = Gen6Save::parse(corruptBox, {}, &error);
    GEN6_CHECK(q && q->diagnostics().invalidBoxRecords == 1);
    GEN6_CHECK(q && q->box(5, 5).state == Gen6::SlotState::Invalid);

    // Metadata length/id disagreement is reported, not fatal.
    auto meta = good;
    PK6::write32(meta, Layout::metadataOffset(Layout::Family::XY) + Layout::EntriesWithinMetadata, 0x1234);
    const auto m = Gen6Save::parse(meta, {}, &error);
    GEN6_CHECK(m && m->diagnostics().metadataMismatches == 1);
}

} // namespace

int main() {
    testTablesMatchPksm();
    testCrcMatchesPksm();
    testValidParse();
    testStrictRejections();
    return Gen6Test::finish("test_gen6_save");
}
