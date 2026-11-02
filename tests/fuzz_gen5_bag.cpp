// QA fuzz target: the native Gen V Bag (Items block 25) decoder and the staged Bag editor.
//
// Input layout: [Items block bytes][operation tape]. The block is decoded directly for both
// families, then embedded into a synthetic, CRC-stamped BW or B2W2 save (family chosen by the tape)
// so the staged editor runs exactly as in the app. Each tape operation (set quantity / add / remove)
// must either be refused with the staged bytes unchanged, or succeed by changing exactly one
// 4-byte slot while the block still decodes as a recognized native bag. Source bytes never change.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"

namespace G5 = PokeVault::Integration::Gen5;

namespace {

void runOne(std::span<const uint8_t> data) {
    for (const auto family : {G5::SaveFamily::BlackWhite, G5::SaveFamily::Black2White2}) {
        const auto direct = G5::decodeBag5(family, data);
        if (data.size() != G5::itemsBlockLength(family)) QA_CHECK(!direct);
        if (direct) {
            qafuzz::noteAccepted();
            for (size_t p = 0; p < G5::BagPocket5Count; ++p) {
                QA_CHECK(direct->at(p).size() <= G5::bagPocketLayout5(family, p).slots);
                for (const auto& item : direct->at(p)) {
                    QA_CHECK(G5::gen5BagItemInPocket(family, p, item.itemId));
                    QA_CHECK(item.count >= 1 && item.count <= G5::bagPocketLayout5(family, p).maxCount);
                }
            }
        }
    }

    const size_t bwLen = G5::itemsBlockLength(G5::SaveFamily::BlackWhite);
    const size_t b2Len = G5::itemsBlockLength(G5::SaveFamily::Black2White2);
    const size_t blockLen = std::min(bwLen, b2Len);
    if (data.size() < blockLen + 1) return;
    const auto family = (data[blockLen] & 1) ? G5::SaveFamily::Black2White2 : G5::SaveFamily::BlackWhite;
    const size_t len = G5::itemsBlockLength(family);
    if (data.size() < len + 1) return;
    auto sav = qasynth::gen5::save(family, family == G5::SaveFamily::BlackWhite ? 21 : 23, 1);
    std::copy_n(data.begin(), len, sav.begin() + static_cast<std::ptrdiff_t>(G5::itemsBlockOffset(family)));
    qasynth::gen5::stamp(std::span<uint8_t>(sav).subspan(0, qasynth::gen5::copySize(family)),
                         qasynth::gen5::blocks(family));
    std::string error;
    const auto save = G5::Gen5ReadOnlySave::parse(sav, {}, &error, G5::SaveCopySelection::Primary);
    QA_CHECK(save.has_value());  // only the Items block changed and every CRC was re-stamped
    auto staged = G5::Gen5StagedBag::create(*save, &error);
    QA_CHECK(static_cast<bool>(staged) == static_cast<bool>(G5::decodeBag5(family, data.subspan(0, len))));
    if (!staged) return;
    qafuzz::noteAccepted();
    const qafuzz::Bytes source(save->sourceBytes().begin(), save->sourceBytes().end());

    std::span<const uint8_t> tape = data.subspan(len + 1);
    while (tape.size() >= 5) {
        const uint8_t op = tape[0] % 3;
        const size_t pocket = tape[1] % (G5::BagPocket5Count + 1);   // includes one out-of-range pocket
        const uint16_t value = static_cast<uint16_t>(tape[2] | (tape[3] << 8));
        const uint16_t qty = static_cast<uint16_t>(tape[4] * 5);
        tape = tape.subspan(5);
        const qafuzz::Bytes before(staged->staged().begin(), staged->staged().end());
        bool ok = false;
        if (op == 0) ok = staged->setQuantity(pocket, value % 300, qty, &error);
        else if (op == 1) ok = staged->add(pocket, value % 700, qty, &error);
        else ok = staged->remove(pocket, value % 300, &error);
        const auto after = staged->staged();
        size_t changed = 0, first = after.size(), last = 0;
        for (size_t i = 0; i < after.size(); ++i)
            if (after[i] != before[i]) {
                ++changed;
                first = std::min(first, i);
                last = i;
            }
        if (!ok) {
            QA_CHECK(changed == 0 && !error.empty());
        } else {
            QA_CHECK(changed == 0 || first / 4 == last / 4);  // one native 4-byte slot
            QA_CHECK(G5::decodeBag5(family, after).has_value());
        }
        QA_CHECK(std::equal(staged->original().begin(), staged->original().end(),
                            source.begin() + static_cast<std::ptrdiff_t>(G5::itemsBlockOffset(family))));
    }
    QA_CHECK(std::equal(save->sourceBytes().begin(), save->sourceBytes().end(), source.begin(), source.end()));
}

std::vector<qafuzz::Bytes> seeds() {
    std::vector<qafuzz::Bytes> out;
    for (const auto family : {G5::SaveFamily::BlackWhite, G5::SaveFamily::Black2White2}) {
        const auto sav = qasynth::gen5::save(family, family == G5::SaveFamily::BlackWhite ? 21 : 23, 1);
        const size_t at = G5::itemsBlockOffset(family);
        const size_t len = G5::itemsBlockLength(family);
        qafuzz::Bytes block(sav.begin() + static_cast<std::ptrdiff_t>(at),
                            sav.begin() + static_cast<std::ptrdiff_t>(at + len));
        out.push_back(block);
        // Same block plus a tape: family selector, then set/add/remove operations.
        auto withTape = block;
        withTape.push_back(family == G5::SaveFamily::BlackWhite ? 0 : 1);
        const uint8_t tape[] = {0, 0, 0, 0, 20,   1, 3, 1, 0, 2,   2, 0, 1, 0, 0,   1, 2, 0x50, 0, 1,
                                0, 1, 0, 0, 0,   2, 1, 0, 0, 0,   1, 0, 0xFF, 0x02, 200};
        withTape.insert(withTape.end(), std::begin(tape), std::end(tape));
        out.push_back(withTape);
    }
    return out;
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen5-bag", runOne, seeds, nullptr, {0x002, 0x4DA, 0x626, 0x7DA, 0x89A}, {{0x000, 0x7D8, 0x40}},
    };
    return target;
}
