// Native record validation for Gen V exports. Deterministic randomized
// sequences of staged PK5, box Clone/Move/Remove, Bag and Trainer operations
// across all four titles and both copies. For every export candidate:
//   - the candidate strict-reparses with the same exact title and copy,
//   - every retail block CRC (and its mirror) verifies in the selected copy,
//   - the unselected copy is byte-identical,
//   - every changed byte lies in an independently computed whitelist
//     (PK5 record slots, Bag pocket slots, cited Trainer ranges, CRC words),
//   - the source image is never modified.
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace T = PokeVault::Integration::Gen5::TrainerLayout5;

namespace {

struct Rng {
    uint32_t s;
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    uint32_t below(uint32_t n) { return next() % n; }
};

std::span<const L::SaveBlock> blocksFor(G::SaveFamily f) {
    return f == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                          : std::span<const L::SaveBlock>(L::Black2White2);
}
size_t copySizeFor(G::SaveFamily f) {
    return f == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
}

void fillCopy(std::vector<uint8_t>& sav, size_t base, G::SaveFamily family, uint8_t version) {
    const size_t player = base + 0x19400;
    C::write16(sav, player + 0x04, 'N');
    C::write16(sav, player + 0x06, 0xFFFF);
    sav[player + 0x1F] = version;
    C::write16(sav, base + G::trainerBlock5(family, T::MiscBlockIndex).offset, 500);
    sav[base + L::PartyOffset + 4] = 2;
    for (size_t i = 0; i < 2; ++i) {
        std::vector<uint8_t> pk(C::PartySize, 0);
        C::write16(pk, 0, static_cast<uint16_t>(0x1000 + i));
        C::write16(pk, 8, static_cast<uint16_t>(i ? 495 : 25));
        pk[0x8C] = 10;
        // Retail-consistent EXP and party stat cache (full HP): the game never
        // stores a level/EXP/stat mismatch, and replay certifies against them.
        const auto info = G::personal5(i ? 495 : 25, 0, family == G::SaveFamily::Black2White2);
        assert(info);
        const uint32_t exp = G::experienceForLevel5(info.info->growthRate, 10);
        C::write16(pk, 0x10, static_cast<uint16_t>(exp));
        C::write16(pk, 0x12, static_cast<uint16_t>(exp >> 16));
        C::write16(pk, 0x8E, 1);
        C::write16(pk, 0x90, 1);
        assert(G::regeneratePartyStats5(pk, pk, family == G::SaveFamily::Black2White2));
        const auto e = C::encryptCandidate(pk);
        std::copy(e.begin(), e.end(),
                  sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8 + i * C::PartySize));
    }
    for (size_t slot = 0; slot < 4; ++slot) {
        std::vector<uint8_t> pk(C::StoredSize, 0);
        C::write16(pk, 0, static_cast<uint16_t>(0x2000 + slot));
        C::write16(pk, 8, static_cast<uint16_t>(133 + slot));
        const auto e = C::encryptCandidate(pk);
        std::copy(e.begin(), e.end(),
                  sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset + slot * 2 * C::StoredSize));
    }
    const size_t items = base + G::itemsBlockOffset(family);
    C::write16(sav, items, 4);
    C::write16(sav, items + 2, 5);
    const size_t medicine = items + G::bagPocketLayout5(family, 3).offset;
    C::write16(sav, medicine, 17);
    C::write16(sav, medicine + 2, 2);
    auto copy = std::span<uint8_t>(sav).subspan(base, copySizeFor(family));
    for (const auto& b : blocksFor(family)) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}

bool whitelisted(size_t rel, G::SaveFamily family) {
    for (const auto& b : blocksFor(family))
        if (rel == b.checksumOffset || rel == b.checksumOffset + 1 || rel == b.mirrorOffset ||
            rel == b.mirrorOffset + 1)
            return true;
    if (rel >= L::PartyOffset + 8 && rel < L::PartyOffset + 8 + 6 * C::PartySize) return true;
    for (size_t box = 0; box < 24; ++box) {
        const size_t at = L::BoxOffset + box * L::BoxStride;
        if (rel >= at && rel < at + 30 * C::StoredSize) return true;
    }
    const size_t items = G::itemsBlockOffset(family);
    for (size_t p = 0; p < G::BagPocket5Count; ++p) {
        const auto spec = G::bagPocketLayout5(family, p);
        if (rel >= items + spec.offset && rel < items + spec.offset + spec.slots * 4) return true;
    }
    const size_t player = G::trainerBlock5(family, T::PlayerBlockIndex).offset;
    const size_t misc = G::trainerBlock5(family, T::MiscBlockIndex).offset;
    return (rel >= player + T::NameOffset && rel < player + T::NameOffset + T::NameBytes) ||
           (rel >= player + T::HoursOffset && rel < player + T::HoursOffset + 4) ||
           (rel >= misc + T::MoneyOffset && rel < misc + T::MoneyOffset + 4);
}

size_t run(G::SaveFamily family, std::string_view game, uint8_t version, bool backup, uint32_t seed) {
    const size_t copySize = copySizeFor(family);
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    fillCopy(sav, 0, family, version);
    fillCopy(sav, copySize, family, version);
    sav[(backup ? 0 : copySize) + 0x19400 + 0x08] = 'Q'; // make the copies differ
    // Restamp the copy we just perturbed so both copies are valid.
    {
        auto copy = std::span<uint8_t>(sav).subspan(backup ? 0 : copySize, copySize);
        for (const auto& b : blocksFor(family)) {
            const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
            C::write16(copy, b.checksumOffset, crc);
            C::write16(copy, b.mirrorOffset, crc);
        }
    }
    const auto pristine = sav;
    const auto sel = backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, sel);
    assert(parsed && parsed->selectedBackupPartition() == backup);
    const size_t base = parsed->selectedCopyOffset();
    using Field = G::StagedPokemon5Record::Field;
    using Slot = G::Gen5StagedPokemonWorkspace::Slot;
    using Region = G::Gen5StagedPokemonWorkspace::Region;

    Rng rng{seed};
    size_t exports = 0;
    for (int round = 0; round < 12; ++round) {
        G::Gen5StagedPokemonWorkspace ws(*parsed);
        const int ops = 1 + static_cast<int>(rng.below(6));
        for (int op = 0; op < ops; ++op) {
            switch (rng.below(10)) {
                case 0: (void)ws.stageParty(rng.below(2), Field::Friendship, 0, rng.below(256), &error); break;
                case 1: (void)ws.stageParty(rng.below(2), Field::IV, rng.below(6), rng.below(32), &error); break;
                case 2: (void)ws.stageParty(rng.below(2), Field::Level, 0, 1 + rng.below(100), &error); break;
                case 3: (void)ws.stageBox(0, 2 * rng.below(4), Field::EV, rng.below(6), rng.below(80), &error); break;
                case 4: (void)ws.cloneToEmptyBox(Slot{Region::Box, 0, 2 * rng.below(4)}, 1 + rng.below(3),
                                                 rng.below(30), &error); break;
                case 5: (void)ws.moveToEmptyBox(Slot{Region::Box, 0, 2 * rng.below(4)}, 4 + rng.below(3),
                                                rng.below(30), &error); break;
                case 6: (void)ws.removeBox(0, 2 * rng.below(4), &error); break;
                case 7: {
                    auto* bag = ws.stagedBag(&error);
                    assert(bag);
                    const uint32_t which = rng.below(3);
                    if (which == 0) (void)bag->setQuantity(0, 0, static_cast<uint16_t>(1 + rng.below(999)), &error);
                    else if (which == 1) (void)bag->add(3, 18 + static_cast<uint16_t>(rng.below(8)),
                                                        static_cast<uint16_t>(1 + rng.below(50)), &error);
                    else (void)bag->remove(3, 0, &error);
                    break;
                }
                case 8: {
                    auto* t = ws.stagedTrainer(&error);
                    assert(t);
                    (void)t->setMoney(rng.below(10000000), &error);
                    (void)t->setPlayTime(rng.below(1000), rng.below(60), rng.below(60), &error);
                    break;
                }
                default: {
                    auto* t = ws.stagedTrainer(&error);
                    assert(t);
                    const char* names[] = {"Ab", "Hilbert", "N", "Rosa 2"};
                    (void)t->setName(names[rng.below(4)], &error);
                    break;
                }
            }
        }
        if (!ws.hasChanges()) {
            assert(!G::buildStagedSaveCandidate(ws, &error));
            continue;
        }
        const auto out = G::buildStagedSaveCandidate(ws, &error);
        assert(out && out->selectedCopyOffset == base);
        ++exports;
        const auto again = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, sel);
        assert(again && again->selectedCopyOffset() == base && again->partyCount() == parsed->partyCount());
        const auto copy = std::span<const uint8_t>(out->bytes).subspan(base, copySize);
        for (const auto& b : blocksFor(family)) {
            const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
            assert(C::read16(copy, b.checksumOffset) == crc && C::read16(copy, b.mirrorOffset) == crc);
        }
        for (size_t i = 0; i < sav.size(); ++i) {
            if (out->bytes[i] == sav[i]) continue;
            assert(i >= base && i < base + copySize);
            assert(whitelisted(i - base, family));
        }
        // Every staged record reads back exactly and validly.
        for (const auto& change : ws.pendingReview()) {
            const auto pk = change.location.region == Region::Party
                ? again->partyPokemon(change.location.slot)
                : again->boxPokemon(change.location.box, change.location.slot);
            assert(pk && pk->valid());
            assert(std::equal(pk->originalEncryptedBytes().begin(), pk->originalEncryptedBytes().end(),
                              change.after.begin()));
        }
        if (ws.bagDirty()) assert(G::decodeBag5(*again));
        if (ws.trainerDirty()) assert(G::decodeTrainer5(*again)->money == ws.stagedTrainerIfOpened()->current().money);
        // Observed-source gate: identical bytes accepted, any change refused.
        assert(G::buildStagedSaveCandidateForObservedSource(ws, sav, &error));
        auto touched = sav;
        touched[rng.below(static_cast<uint32_t>(touched.size()))] ^= 0x40;
        assert(!G::buildStagedSaveCandidateForObservedSource(ws, touched, &error));
    }
    assert(sav == pristine);
    return exports;
}

} // namespace

int main() {
    size_t exports = 0;
    uint32_t seed = 0x5EED5u;
    for (const bool backup : {false, true}) {
        for (int i = 0; i < 6; ++i) {
            exports += run(G::SaveFamily::BlackWhite, "white_nds", 20, backup, seed += 7919);
            exports += run(G::SaveFamily::BlackWhite, "black_nds", 21, backup, seed += 7919);
            exports += run(G::SaveFamily::Black2White2, "white2_nds", 22, backup, seed += 7919);
            exports += run(G::SaveFamily::Black2White2, "black2_nds", 23, backup, seed += 7919);
        }
    }
    assert(exports > 300);
    std::cout << "Gen V export validation: " << exports
              << " randomized exports strict-reparse, all CRCs verify, whitelist-only diffs PASS\n";
}
