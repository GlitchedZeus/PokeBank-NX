// Read-only SM/USUM box names and Pokedex progress (synthetic, signed fixtures).
#include "Integration/Gen7/Gen7Readouts.h"

#include "fixtures/gen6/Gen6TestSupport.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

namespace Gen7 = PokeVault::Integration::Gen7;
namespace R = Gen7::Readouts;
namespace Layout = Gen7::Layout;
namespace PK7 = Gen7::PK7;

extern "C" const char* __lsan_default_suppressions() {
    return "leak:rsa_encrypt\nleak:rsa_decrypt\n";
}

namespace {

void setBit(std::vector<uint8_t>& save, size_t at, uint16_t species) {
    const size_t bit = species - 1u;
    save[at + (bit >> 3)] |= static_cast<uint8_t>(1u << (bit & 7));
}

void testFamily(Layout::Family f) {
    auto bytes = Gen7Fixture::makeSave({f, f == Layout::Family::SunMoon ? Layout::VersionSun : Layout::VersionUltraMoon,
                                        1, false, false});
    const auto box = Layout::blocks(f)[R::BoxLayoutBlock];
    const auto dex = Layout::blocks(f)[R::ZukanBlock];
    std::fill_n(bytes.begin() + box.offset, box.length, uint8_t{0});
    std::fill_n(bytes.begin() + dex.offset, dex.length, uint8_t{0});
    const std::u16string name = u"Alola";
    for (size_t i = 0; i < name.size(); ++i) PK7::write16(bytes, box.offset + R::BoxNameBytes * 31 + 2 * i, name[i]);
    bytes[box.offset + R::UnlockedOffset] = 32;
    bytes[box.offset + R::CurrentBoxOffset] = 31;
    const uint16_t last = R::maxSpecies(f);
    setBit(bytes, dex.offset + R::ZukanCaught, 722);
    setBit(bytes, dex.offset + R::ZukanCaught, last);
    setBit(bytes, dex.offset + R::ZukanSeen + R::ZukanRegion, 722);
    setBit(bytes, dex.offset + R::ZukanSeen + 3 * R::ZukanRegion, last);
    setBit(bytes, dex.offset + R::ZukanSeen, 25);
    if (f == Layout::Family::SunMoon) setBit(bytes, dex.offset + R::ZukanCaught, 807);  // beyond SM range: ignored
    Gen7Fixture::resignBlocks(bytes, f);
    Gen7Fixture::signMeme(bytes, f);
    std::string error;
    const auto save = Gen7::Gen7Save::parse(bytes, {}, &error);
    GEN6_CHECK(save.has_value());
    if (!save) { std::fprintf(stderr, "  %s\n", error.c_str()); return; }
    GEN6_CHECK(R::boxName(*save, 31) == u"Alola" && R::boxName(*save, 0) == u"" && !R::boxName(*save, 32));
    GEN6_CHECK(R::unlockedBoxes(*save) == 32 && R::currentBox(*save) == 31);
    const auto p = R::dexProgress(*save);
    GEN6_CHECK(p.total == last && p.caught == 2 && p.seen == 3);
    GEN6_CHECK(R::dexSeen(*save, 25) && !R::dexCaught(*save, 25) && !R::dexSeen(*save, last + 1));
}

} // namespace

int main() {
    GEN6_CHECK(R::ZukanCaught == 4 + 4 + 0x80);
    GEN6_CHECK(R::ZukanSeen == R::ZukanCaught + 0x68);
    GEN6_CHECK(32 * R::BoxNameBytes <= 0x5C0);  // names end before PCBackgrounds
    for (const auto f : {Layout::Family::SunMoon, Layout::Family::UltraSunUltraMoon}) {
        GEN6_CHECK(R::ZukanSeen + 4 * R::ZukanRegion <= Layout::blocks(f)[R::ZukanBlock].length);
        GEN6_CHECK(R::CurrentBoxOffset < Layout::blocks(f)[R::BoxLayoutBlock].length);
        testFamily(f);
    }
    return Gen6Test::finish("test_gen7_readouts");
}
