// Read-only Gen VI box names and Pokedex progress (synthetic fixtures).
#include "Integration/Gen6/Gen6Readouts.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

namespace Gen6 = PokeVault::Integration::Gen6;
namespace R = Gen6::Readouts;
namespace Layout = Gen6::Layout;
namespace PK6 = Gen6::PK6;

namespace {

void setBit(std::vector<uint8_t>& save, size_t at, uint16_t species) {
    const size_t bit = species - 1u;
    save[at + (bit >> 3)] |= static_cast<uint8_t>(1u << (bit & 7));
}

std::vector<uint8_t> build(Layout::Family f) {
    auto bytes = Gen6Fixture::makeSave({f, f == Layout::Family::XY ? Layout::VersionY : Layout::VersionAlphaSapphire, 1, false});
    const auto box = Layout::blocks(f)[R::BoxLayoutBlock];
    const auto dex = Layout::blocks(f)[R::ZukanBlock];
    std::fill_n(bytes.begin() + box.offset, box.length, uint8_t{0});
    std::fill_n(bytes.begin() + dex.offset, dex.length, uint8_t{0});
    const std::u16string first = u"Team";
    const std::u16string full = u"ABCDEFGHIJKLMNOPQ";  // 17 units fill the 0x22-byte slot
    for (size_t i = 0; i < first.size(); ++i) PK6::write16(bytes, box.offset + 2 * i, first[i]);
    for (size_t i = 0; i < full.size(); ++i) PK6::write16(bytes, box.offset + R::BoxNameBytes * 30 + 2 * i, full[i]);
    bytes[box.offset + R::UnlockedOffset] = 16;
    bytes[box.offset + R::CurrentBoxOffset] = 3;
    setBit(bytes, dex.offset + R::ZukanCaught, 1);
    setBit(bytes, dex.offset + R::ZukanCaught, 721);
    setBit(bytes, dex.offset + R::ZukanSeen, 1);                           // region 0
    setBit(bytes, dex.offset + R::ZukanSeen + 3 * R::ZukanRegion, 721);    // region 3
    setBit(bytes, dex.offset + R::ZukanSeen + 2 * R::ZukanRegion, 25);     // seen only
    if (f == Layout::Family::XY) setBit(bytes, dex.offset + R::ZukanForeignXY, 649);
    Gen6Fixture::resignAll(bytes, f);
    return bytes;
}

void testFamily(Layout::Family f) {
    const auto bytes = build(f);
    const auto save = Gen6::Gen6Save::parse(bytes);
    GEN6_CHECK(save.has_value());
    if (!save) return;
    GEN6_CHECK(R::boxName(*save, 0) == u"Team");
    GEN6_CHECK(R::boxName(*save, 1) == u"");
    GEN6_CHECK(R::boxName(*save, 30) == u"ABCDEFGHIJKLMNOPQ");
    GEN6_CHECK(!R::boxName(*save, 31));
    GEN6_CHECK(R::unlockedBoxes(*save) == 16 && R::currentBox(*save) == 3);
    const auto p = R::dexProgress(*save);
    GEN6_CHECK(p.caught == 2 && p.seen == 3 && p.total == 721);
    GEN6_CHECK(p.foreignObtained == (f == Layout::Family::XY ? 1 : 0));
    GEN6_CHECK(R::dexSeen(*save, 25) && !R::dexCaught(*save, 25));
    GEN6_CHECK(!R::dexSeen(*save, 0) && !R::dexSeen(*save, 722) && !R::dexCaught(*save, 722));
    // Reading never changes the parsed copy.
    GEN6_CHECK(std::equal(bytes.begin(), bytes.end(), save->bytes().begin()));
}

} // namespace

int main() {
    // Cited layout sanity: BoxLayout6 0x41E/0x43D/0x43E, and Zukan block geometry.
    GEN6_CHECK(31 * R::BoxNameBytes == 0x41E);
    GEN6_CHECK(R::ZukanSeen + 4 * R::ZukanRegion == 0x1E8);
    for (const auto f : {Layout::Family::XY, Layout::Family::ORAS}) {
        GEN6_CHECK(Layout::blocks(f)[R::BoxLayoutBlock].offset == 0x4400);
        GEN6_CHECK(Layout::blocks(f)[R::BoxLayoutBlock].length == 0x440);
        GEN6_CHECK(Layout::blocks(f)[R::ZukanBlock].offset == 0x15000);
        testFamily(f);
    }
    GEN6_CHECK(R::ZukanForeignXY + 0x54 <= Layout::XYBlocks[R::ZukanBlock].length);
    return Gen6Test::finish("test_gen6_readouts");
}
