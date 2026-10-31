// Gen VI native Bag (MyItem block 1): strict decode, staged edits and the
// export byte whitelist.
#include "Integration/Gen6/Gen6Bag.h"
#include "Integration/Gen6/Gen6StagedSession.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

#include <set>

namespace Gen6 = PokeVault::Integration::Gen6;
namespace Layout = Gen6::Layout;
namespace PK6 = Gen6::PK6;
using Gen6::BagPocket6;
using Gen6::Gen6Save;
using Gen6::Gen6StagedSession;

namespace {

constexpr size_t P(BagPocket6 p) { return static_cast<size_t>(p); }

std::vector<uint8_t> saveWithBag(Layout::Family f) {
    auto bytes = Gen6Fixture::makeSave({f, f == Layout::Family::XY ? Layout::VersionX : Layout::VersionOmegaRuby, 2, false});
    const size_t med = f == Layout::Family::XY ? 0x968 : 0x970;
    const size_t berry = f == Layout::Family::XY ? 0xA68 : 0xA70;
    Gen6Fixture::writeBag(bytes, f, {
        {0x000, 4, 30},      // Poke Ball
        {0x000, 2, 5},       // Ultra Ball
        {0x640, 466, 1},     // Key: in both families' key lists
        {0x7C0, 328, 1},     // TM01
        {med, 17, 12},       // Potion
        {berry, 149, 99},    // Cheri Berry
    });
    Gen6Fixture::resignAll(bytes, f);
    return bytes;
}

void testLayout() {
    for (const auto f : {Layout::Family::XY, Layout::Family::ORAS}) {
        const size_t len = Gen6::itemsBlockLength6(f);
        size_t prevEnd = 0;
        for (size_t p = 0; p < Gen6::BagPocket6Count; ++p) {
            const auto spec = Gen6::bagPocketLayout6(f, p);
            GEN6_CHECK(spec.slots > 0 && spec.offset >= prevEnd && spec.offset + spec.slots * 4 <= len);
            prevEnd = spec.offset + spec.slots * 4;
            // No duplicate IDs within a pocket list.
            const auto ids = Gen6::bagPocketItems6(f, p);
            GEN6_CHECK(std::set<uint16_t>(ids.begin(), ids.end()).size() == ids.size());
        }
        // PKSM-Core aa22d7a4 SavXY/SavORAS absolute pouch offsets.
        const size_t base = Gen6::itemsBlockOffset6(f);
        GEN6_CHECK(base == 0x400);
        GEN6_CHECK(base + Gen6::bagPocketLayout6(f, P(BagPocket6::KeyItems)).offset == 0xA40);
        GEN6_CHECK(base + Gen6::bagPocketLayout6(f, P(BagPocket6::Machines)).offset == 0xBC0);
        GEN6_CHECK(base + Gen6::bagPocketLayout6(f, P(BagPocket6::Medicine)).offset ==
                   (f == Layout::Family::XY ? 0xD68u : 0xD70u));
        GEN6_CHECK(base + Gen6::bagPocketLayout6(f, P(BagPocket6::Berries)).offset ==
                   (f == Layout::Family::XY ? 0xE68u : 0xE70u));
    }
    // ORAS adds HM06/HM07 (425, 737) and moves the flutes to Medicine.
    GEN6_CHECK(!Gen6::gen6BagItemInPocket(Layout::Family::XY, P(BagPocket6::Machines), 737));
    GEN6_CHECK(Gen6::gen6BagItemInPocket(Layout::Family::ORAS, P(BagPocket6::Machines), 737));
    GEN6_CHECK(Gen6::gen6BagItemInPocket(Layout::Family::XY, P(BagPocket6::Items), 65));
    GEN6_CHECK(Gen6::gen6BagItemInPocket(Layout::Family::ORAS, P(BagPocket6::Medicine), 65));
    GEN6_CHECK(!Gen6::gen6BagItemInPocket(Layout::Family::ORAS, P(BagPocket6::Items), 65));
    // Story pockets and unreleased items are never addable.
    GEN6_CHECK(!Gen6::gen6BagItemAddable(Layout::Family::XY, P(BagPocket6::KeyItems), 466));
    GEN6_CHECK(!Gen6::gen6BagItemAddable(Layout::Family::XY, P(BagPocket6::Machines), 328));
    GEN6_CHECK(!Gen6::gen6BagItemAddable(Layout::Family::XY, P(BagPocket6::Items), 576));  // Dream Ball
    GEN6_CHECK(Gen6::gen6BagItemAddable(Layout::Family::XY, P(BagPocket6::Items), 3));
}

void testDecode() {
    for (const auto f : {Layout::Family::XY, Layout::Family::ORAS}) {
        const auto bytes = saveWithBag(f);
        const auto save = Gen6Save::parse(bytes);
        GEN6_CHECK(save.has_value());
        if (!save) continue;
        const auto bag = Gen6::decodeBag6(f, Gen6::sourceItemsBlock6(*save));
        GEN6_CHECK(bag.has_value());
        if (!bag) continue;
        GEN6_CHECK((*bag)[0].size() == 2 && (*bag)[0][0] == (Gen6::BagItem6{4, 30}));
        GEN6_CHECK((*bag)[1].size() == 1 && (*bag)[2].size() == 1 && (*bag)[3].size() == 1 && (*bag)[4].size() == 1);
        // Strict failures.
        auto block = std::vector<uint8_t>(Gen6::sourceItemsBlock6(*save).begin(), Gen6::sourceItemsBlock6(*save).end());
        auto wrongPocket = block;
        PK6::write16(wrongPocket, 0x000, 17);  // Potion in the Items pouch
        GEN6_CHECK(!Gen6::decodeBag6(f, wrongPocket));
        auto tooMany = block;
        PK6::write16(tooMany, 0x002, 1000);
        GEN6_CHECK(!Gen6::decodeBag6(f, tooMany));
        auto keyTwo = block;
        PK6::write16(keyTwo, 0x642, 2);
        GEN6_CHECK(!Gen6::decodeBag6(f, keyTwo));
        block.pop_back();
        GEN6_CHECK(!Gen6::decodeBag6(f, block));
    }
    // The pattern-filled fixture bag is not a recognized bag: no Bag session.
    auto plain = Gen6StagedSession(*Gen6Save::parse(Gen6Fixture::makeSave()));
    std::string error;
    GEN6_CHECK(plain.bag(&error) == nullptr && !error.empty());
}

void testStagedBagAndExport() {
    const auto original = saveWithBag(Layout::Family::ORAS);
    auto s = Gen6StagedSession(*Gen6Save::parse(original));
    std::string error;
    auto* bag = s.bag(&error);
    GEN6_CHECK(bag != nullptr);
    if (!bag) return;
    GEN6_CHECK(bag->setQuantity(P(BagPocket6::Items), 0, 99, &error));
    GEN6_CHECK(!bag->setQuantity(P(BagPocket6::Items), 0, 1000, &error));
    GEN6_CHECK(!bag->setQuantity(P(BagPocket6::Items), 9, 1, &error));
    GEN6_CHECK(bag->add(P(BagPocket6::Medicine), 66, 3, &error));        // ORAS flute in Medicine
    GEN6_CHECK(!bag->add(P(BagPocket6::Medicine), 66, 1, &error));       // duplicate
    GEN6_CHECK(!bag->add(P(BagPocket6::Items), 66, 1, &error));          // not an ORAS Items member
    GEN6_CHECK(!bag->add(P(BagPocket6::KeyItems), 718, 1, &error));
    GEN6_CHECK(bag->remove(P(BagPocket6::Berries), 0, &error));
    GEN6_CHECK(!bag->remove(P(BagPocket6::Machines), 0, &error));
    GEN6_CHECK(s.hasChanges() && s.bagDirty());
    GEN6_CHECK(s.stageValue(Gen6::SlotRef::boxed(0, 0), Gen6::Field::HeldItem, 3, 0, &error));

    const auto edited = s.buildWorkingCopy(&error);
    GEN6_CHECK(edited.has_value());
    if (!edited) { std::fprintf(stderr, "  %s\n", error.c_str()); return; }
    const auto f = Layout::Family::ORAS;
    const size_t bagAt = Gen6::itemsBlockOffset6(f);
    for (size_t i = 0; i < edited->size(); ++i) {
        if ((*edited)[i] == original[i]) continue;
        const bool inBag = i >= bagAt && i < bagAt + Gen6::itemsBlockLength6(f);
        const bool inRec = i >= Gen6Save::boxRecordOffset(f, 0, 0) && i < Gen6Save::boxRecordOffset(f, 0, 0) + PK6::StoredSize;
        const bool inChk = i == Layout::checksumOffset(f, 1) || i == Layout::checksumOffset(f, 1) + 1 ||
                           i == Layout::checksumOffset(f, Layout::boxBlock(f)) ||
                           i == Layout::checksumOffset(f, Layout::boxBlock(f)) + 1;
        GEN6_CHECK(inBag || inRec || inChk);
    }
    const auto re = Gen6Save::parse(*edited, "omegaruby_3ds");
    GEN6_CHECK(re.has_value());
    const auto bagAfter = Gen6::decodeBag6(f, Gen6::sourceItemsBlock6(*re));
    GEN6_CHECK(bagAfter && (*bagAfter)[0][0].count == 99 && (*bagAfter)[3].size() == 2 && (*bagAfter)[4].empty());
    const auto lines = s.changeLines();
    size_t bagLines = 0;
    for (const auto& l : lines) if (l.rfind("Bag ", 0) == 0) ++bagLines;
    GEN6_CHECK(bagLines == 3);

    // Without the staged bag expectation, the same image is refused.
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), *edited, s.changes(), &error));
    // A byte outside every pouch slot array inside block 1 is refused even when re-signed.
    auto forgedBlock = std::vector<uint8_t>(bag->staged().begin(), bag->staged().end());
    forgedBlock.back() ^= 1;  // tail padding after the Berries pouch
    auto forged = *edited;
    std::copy(forgedBlock.begin(), forgedBlock.end(), forged.begin() + static_cast<std::ptrdiff_t>(bagAt));
    Gen6Fixture::resignAll(forged, f);
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), forged, s.changes(), &error, forgedBlock));

    // Discarding the bag leaves only the Pokemon change.
    s.bag()->discard();
    GEN6_CHECK(!s.bagDirty() && s.hasChanges());
    const auto noBag = s.buildWorkingCopy(&error);
    GEN6_CHECK(noBag && std::equal(noBag->begin() + static_cast<std::ptrdiff_t>(bagAt),
                                   noBag->begin() + static_cast<std::ptrdiff_t>(bagAt + Gen6::itemsBlockLength6(f)),
                                   original.begin() + static_cast<std::ptrdiff_t>(bagAt)));
}

} // namespace

int main() {
    testLayout();
    testDecode();
    testStagedBagAndExport();
    return Gen6Test::finish("test_gen6_bag");
}
