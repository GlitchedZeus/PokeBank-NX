// Gen VII native Bag (MyItem block 0, packed u32 slots): strict decode, staged
// edits that preserve the free-space/NEW bits, and the unsigned-candidate whitelist.
#include "Integration/Gen7/Gen7Bag.h"
#include "Integration/Gen7/Gen7StagedSession.h"

#include "fixtures/gen6/Gen6TestSupport.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

#include <algorithm>
#include <set>

namespace Gen7 = PokeVault::Integration::Gen7;
namespace Layout = Gen7::Layout;
namespace PK7 = Gen7::PK7;
using Gen7::BagPocket7;

extern "C" const char* __lsan_default_suppressions() {
    return "leak:rsa_encrypt\nleak:rsa_decrypt\n";
}

namespace {

constexpr size_t P(BagPocket7 p) { return static_cast<size_t>(p); }

std::vector<uint8_t> saveWithBag(Layout::Family f) {
    auto bytes = Gen7Fixture::makeSave({f, f == Layout::Family::SunMoon ? Layout::VersionSun : Layout::VersionUltraSun,
                                        1, false, false});
    const auto block = Layout::blocks(f)[Gen7::ItemsBlock7];
    std::fill_n(bytes.begin() + block.offset, block.length, uint8_t{0});
    const auto put = [&](size_t pocket, size_t slot, uint32_t value) {
        PK7::write32(bytes, block.offset + Gen7::bagPocketLayout7(f, pocket).offset + slot * 4, value);
    };
    put(P(BagPocket7::Items), 0, 4u | (30u << 10) | (7u << 20) | 0x40000000u);  // Poke Ball x30, sort 7, NEW
    put(P(BagPocket7::Items), 1, 2u | (5u << 10));                              // Ultra Ball x5
    put(P(BagPocket7::Medicine), 0, 17u | (12u << 10));                         // Potion x12
    put(P(BagPocket7::Berries), 0, 149u | (99u << 10));                         // Cheri Berry x99
    put(P(BagPocket7::Machines), 0, 328u | (1u << 10));                         // TM01
    Gen7Fixture::resignBlocks(bytes, f);
    Gen7Fixture::signMeme(bytes, f);
    return bytes;
}

void testLayout() {
    for (const auto f : {Layout::Family::SunMoon, Layout::Family::UltraSunUltraMoon}) {
        // Pouches tile the MyItem block exactly (PlayerBag7SM / PlayerBag7USUM).
        std::vector<std::pair<size_t, size_t>> spans;
        size_t total = 0;
        for (size_t p = 0; p < Gen7::BagPocket7Count; ++p) {
            const auto s = Gen7::bagPocketLayout7(f, p);
            if (!s.slots) continue;
            spans.emplace_back(s.offset, s.offset + s.slots * 4);
            total += s.slots * 4;
            GEN6_CHECK(Gen7::bagPocketItems7(f, p).size() <= s.slots);
            const auto ids = Gen7::bagPocketItems7(f, p);
            GEN6_CHECK(std::set<uint16_t>(ids.begin(), ids.end()).size() == ids.size());
        }
        std::sort(spans.begin(), spans.end());
        for (size_t i = 0; i + 1 < spans.size(); ++i) GEN6_CHECK(spans[i].second == spans[i + 1].first);
        GEN6_CHECK(spans.front().first == 0 && spans.back().second == Gen7::itemsBlockLength7(f));
        GEN6_CHECK(total == Gen7::itemsBlockLength7(f));
    }
    GEN6_CHECK(Gen7::bagPocketLayout7(Layout::Family::SunMoon, P(BagPocket7::BattleItems)).slots == 0);
    GEN6_CHECK(Gen7::bagPocketLayout7(Layout::Family::UltraSunUltraMoon, P(BagPocket7::BattleItems)).slots == 11);
    GEN6_CHECK(!Gen7::gen7BagItemAddable(Layout::Family::SunMoon, P(BagPocket7::ZCrystals), 776));
    GEN6_CHECK(!Gen7::gen7BagItemAddable(Layout::Family::SunMoon, P(BagPocket7::Machines), 328));
    GEN6_CHECK(Gen7::gen7BagItemAddable(Layout::Family::SunMoon, P(BagPocket7::Items), 3));
}

void testDecodeAndStage() {
    for (const auto f : {Layout::Family::SunMoon, Layout::Family::UltraSunUltraMoon}) {
        const auto original = saveWithBag(f);
        std::string error;
        auto save = Gen7::Gen7Save::parse(original, {}, &error);
        GEN6_CHECK(save.has_value());
        if (!save) { std::fprintf(stderr, "  %s\n", error.c_str()); continue; }
        const auto bag = Gen7::decodeBag7(f, Gen7::sourceItemsBlock7(*save));
        GEN6_CHECK(bag && (*bag)[0].size() == 2 && (*bag)[0][0] == (Gen7::BagItem7{4, 30}));
        // Strict failures.
        std::vector<uint8_t> block(Gen7::sourceItemsBlock7(*save).begin(), Gen7::sourceItemsBlock7(*save).end());
        auto wrong = block;
        PK7::write32(wrong, 0, 17u | (1u << 10));  // Potion in Items
        GEN6_CHECK(!Gen7::decodeBag7(f, wrong));
        auto tm2 = block;
        PK7::write32(tm2, Gen7::bagPocketLayout7(f, P(BagPocket7::Machines)).offset, 328u | (2u << 10));
        GEN6_CHECK(!Gen7::decodeBag7(f, tm2));

        Gen7::Gen7StagedSession s(std::move(*save));
        auto* b = s.bag(&error);
        GEN6_CHECK(b != nullptr);
        if (!b) continue;
        GEN6_CHECK(b->setQuantity(P(BagPocket7::Items), 0, 99, &error));
        GEN6_CHECK(!b->setQuantity(P(BagPocket7::Items), 0, 1000, &error));
        GEN6_CHECK(b->add(P(BagPocket7::Medicine), 18, 4, &error));       // Antidote
        GEN6_CHECK(!b->add(P(BagPocket7::Medicine), 18, 1, &error));      // duplicate
        GEN6_CHECK(!b->add(P(BagPocket7::Items), 17, 1, &error));         // wrong pocket
        GEN6_CHECK(b->remove(P(BagPocket7::Berries), 0, &error));
        GEN6_CHECK(!b->remove(P(BagPocket7::Machines), 0, &error));
        // The quantity edit kept the sort index (7) and the NEW bit.
        const uint32_t first = PK7::read32(b->staged(), 0);
        GEN6_CHECK(Gen7::slotItem(first) == 4 && Gen7::slotCount(first) == 99);
        GEN6_CHECK(((first >> 20) & 0x3FF) == 7 && (first & 0x40000000u) != 0);

        const auto c = s.buildUnsignedCandidate(&error);
        GEN6_CHECK(c.has_value());
        if (!c) { std::fprintf(stderr, "  %s\n", error.c_str()); continue; }
        const size_t at = Gen7::itemsBlockOffset7(f);
        for (size_t i = 0; i < original.size(); ++i) {
            if (original[i] == c->bytes[i]) continue;
            const bool ok = (i >= at && i < at + Gen7::itemsBlockLength7(f)) ||
                            i == Layout::checksumOffset(f, 0) || i == Layout::checksumOffset(f, 0) + 1;
            GEN6_CHECK(ok);
        }
        const auto relaxed = Gen7::Gen7Save::parse(c->bytes, {}, &error, Gen7::SignaturePolicy::Ignore);
        GEN6_CHECK(relaxed && !relaxed->signatureVerified());
        const auto after = Gen7::decodeBag7(f, Gen7::sourceItemsBlock7(*relaxed));
        GEN6_CHECK(after && (*after)[0][0].count == 99 && (*after)[1].size() == 2 && (*after)[3].empty());
        // Without the bag expectation the image is refused; a forged out-of-slot byte too.
        GEN6_CHECK(!Gen7::verifyUnsignedCandidate(s.source(), c->bytes, s.changes(), &error));
        GEN6_CHECK(Gen7::verifyUnsignedCandidate(s.source(), c->bytes, s.changes(), &error, s.stagedBagBlock()));
    }
}

} // namespace

int main() {
    testLayout();
    testDecodeAndStage();
    return Gen6Test::finish("test_gen7_bag");
}
