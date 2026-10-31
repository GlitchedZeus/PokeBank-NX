#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;

namespace {

struct Stack { size_t pocket; uint16_t id; uint16_t count; };

std::span<const L::SaveBlock> blocksFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                               : std::span<const L::SaveBlock>(L::Black2White2);
}
size_t copySizeFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
}
void stamp(std::span<uint8_t> copy, std::span<const L::SaveBlock> blocks) {
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}

// A checksum-valid synthetic SAV5 with one party Pokemon and a populated bag in
// the selected copy. The other copy keeps distinct sentinel bytes so any write
// outside the selected copy is detectable.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version, bool backup,
                             const std::vector<Stack>& stacks) {
    const size_t copySize = copySizeFor(family);
    const size_t base = backup ? copySize : 0;
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    sav[base + 0x19400 + 0x1F] = version;
    sav[base + L::PartyOffset + 4] = 1;
    std::vector<uint8_t> party(C::PartySize, 0);
    party[0] = 0x23; party[1] = 0x01;
    C::write16(party, 8, 25);
    party[0x8C] = 30;
    const auto encrypted = C::encryptCandidate(party);
    std::copy(encrypted.begin(), encrypted.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8));
    std::vector<size_t> next(G::BagPocket5Count, 0);
    const size_t items = base + G::itemsBlockOffset(family);
    for (const auto& s : stacks) {
        const auto spec = G::bagPocketLayout5(family, s.pocket);
        const size_t at = items + spec.offset + next[s.pocket]++ * 4;
        C::write16(sav, at, s.id);
        C::write16(sav, at + 2, s.count);
    }
    stamp(std::span<uint8_t>(sav).subspan(base, copySize), blocksFor(family));
    // Unselected copy: arbitrary (invalid) bytes that must survive untouched.
    const size_t other = backup ? 0 : copySize;
    for (size_t i = 0; i < 64; ++i) sav[other + 0x18400 + i] = static_cast<uint8_t>(0xA0 + i);
    return sav;
}

void exercise(G::SaveFamily family, std::string_view game, uint8_t version, bool backup) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const uint16_t key = b2w2 ? 627 : 437;
    const std::vector<Stack> stacks = {
        {0, 4, 30}, {0, 213, 1}, {1, key, 1}, {2, 328, 1},
        {3, 17, 12}, {3, 26, 3}, {4, 149, 5},
    };
    const auto input = fixture(family, version, backup, stacks);
    std::string error;
    const auto sel = backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, game, &error, sel);
    assert(parsed && error.empty());

    // Strict read-only decode of the selected copy.
    const auto bag = G::decodeBag5(*parsed);
    assert(bag);
    assert((*bag)[0].size() == 2 && (*bag)[0][0].itemId == 4 && (*bag)[0][0].count == 30);
    assert((*bag)[1].size() == 1 && (*bag)[1][0].itemId == key);
    assert((*bag)[2].size() == 1 && (*bag)[2][0].itemId == 328);
    assert((*bag)[3].size() == 2 && (*bag)[3][1].itemId == 26 && (*bag)[3][1].count == 3);
    assert((*bag)[4].size() == 1 && (*bag)[4][0].itemId == 149);

    // Catalog: family-specific key items, Gen V Machines include TM93-95.
    assert(G::gen5BagItemInPocket(family, 2, 618));
    assert(G::gen5BagItemInPocket(family, 1, 627) == b2w2);
    assert(!G::gen5BagItemInPocket(family, 0, 17));       // Potion is Medicine
    assert(!G::gen5BagItemAddable(family, 0, 5));         // Safari Ball unreleased
    assert(G::gen5BagItemAddable(family, 1, key));        // warning belongs to the UI
    assert(G::gen5BagItemAddable(family, 2, 328));
    assert(G::gen5BagItemAddable(family, 3, 591));
    assert(!G::gen5BagChoices(family, 0).empty() && !G::gen5BagChoices(family, 1).empty());

    G::Gen5StagedPokemonWorkspace ws(*parsed);
    auto* staged = ws.stagedBag(&error);
    assert(staged && !staged->dirty() && !ws.hasChanges());

    // Rejected edits leave the staged block unchanged.
    assert(!staged->setQuantity(0, 0, 0, &error));
    assert(!staged->setQuantity(0, 0, 1000, &error));
    assert(!staged->setQuantity(1, 0, 2, &error));        // Key Item max 1
    assert(!staged->setQuantity(0, 9, 5, &error));        // missing row
    assert(!staged->setQuantity(7, 0, 5, &error));        // missing pocket
    assert(!staged->add(1, key, 1, &error));              // unique duplicate
    assert(!staged->add(2, 328, 1, &error));
    assert(!staged->add(1, 447, 2, &error));
    assert(!staged->add(2, 618, 2, &error));
    assert(!staged->add(0, 4, 1, &error));                // duplicate stack
    assert(!staged->add(0, 17, 1, &error));               // wrong pocket
    assert(!staged->add(0, 5, 1, &error));                // unreleased
    assert(!staged->add(3, 17, 1000, &error));
    assert(!staged->dirty());

    // Accepted edits, including unique Key Item and reusable-machine ownership.
    assert(staged->remove(1, 0, &error));
    assert(staged->add(1, 447, 1, &error));
    assert(staged->remove(2, 0, &error));
    assert(staged->add(2, 618, 1, &error));
    assert(staged->setQuantity(0, 0, 99, &error) && error.empty());
    assert(staged->remove(3, 0, &error));
    assert(staged->add(4, 150, 7, &error));
    assert(staged->add(3, 591, 2, &error));
    assert(staged->dirty() && ws.hasChanges() && ws.bagDirty());
    const auto current = staged->current();
    assert(current[0][0].count == 99);
    assert(current[3].size() == 2 && current[3][0].itemId == 26 && current[3][1].itemId == 591);
    assert(current[4].size() == 2 && current[4][1].itemId == 150 && current[4][1].count == 7);
    // The untouched source workspace baseline is unchanged.
    assert((*G::decodeBag5(*parsed))[0][0].count == 30);

    // Bag-only export candidate.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && error.empty() && out->bagChanged && out->changedRecords == 0);
    const size_t copySize = copySizeFor(family);
    const size_t base = backup ? copySize : 0;
    const auto& itemsBlock = blocksFor(family)[G::ItemsBlockIndex];
    const auto& terminal = blocksFor(family).back();
    for (size_t i = 0; i < input.size(); ++i) {
        if (out->bytes[i] == input[i]) continue;
        const size_t rel = i - base;
        const bool inCopy = i >= base && i < base + copySize;
        const bool inItems = inCopy && rel >= itemsBlock.offset &&
                             rel < itemsBlock.offset + itemsBlock.length;
        const bool crc = inCopy && (rel == itemsBlock.checksumOffset || rel == itemsBlock.checksumOffset + 1 ||
                                    rel == itemsBlock.mirrorOffset || rel == itemsBlock.mirrorOffset + 1 ||
                                    rel == terminal.checksumOffset || rel == terminal.checksumOffset + 1 ||
                                    rel == terminal.mirrorOffset || rel == terminal.mirrorOffset + 1);
        assert(inItems || crc);
    }
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, sel);
    assert(reopened);
    const auto exportedBag = G::decodeBag5(*reopened);
    assert(exportedBag && (*exportedBag)[0][0].count == 99 && (*exportedBag)[4].size() == 2);
    assert((*exportedBag)[1].size() == 1 && (*exportedBag)[1][0].itemId == 447 && (*exportedBag)[1][0].count == 1);
    assert((*exportedBag)[2].size() == 1 && (*exportedBag)[2][0].itemId == 618 && (*exportedBag)[2][0].count == 1);
    assert(reopened->partyCount() == 1);
    // A stale observed source is refused.
    auto stale = input;
    stale[base + G::itemsBlockOffset(family) + 2] ^= 1;
    assert(!G::buildStagedSaveCandidateForObservedSource(ws, stale, &error));

    // Bag + PK5 edits export together.
    using Field = G::StagedPokemon5Record::Field;
    assert(ws.stageParty(0, Field::Friendship, 0, 200, &error));
    const auto both = G::buildStagedSaveCandidate(ws, &error);
    assert(both && both->bagChanged && both->changedRecords == 1);
    const auto both2 = G::Gen5ReadOnlySave::parse(both->bytes, game, &error, sel);
    assert(both2 && (*G::decodeBag5(*both2))[3][1].itemId == 591);

    // Full verified working-copy export with the BAG_CHANGED manifest line.
    const std::string root = "build-host/gen5_bag_export_" + std::string(game) + (backup ? "_b" : "_p");
    ::mkdir("build-host", 0700);
    ::mkdir(root.c_str(), 0700);
    G::WorkingCopyExportRequest req;
    req.profileIdentity = "0123456789abcdef0123456789abcdef";
    req.timestamp = "20261014_120000";
    req.sourcePath = "sdmc:/switch/drastic/user/backup/test.dsv";
    req.applicationSha = "host-synthetic-test";
    req.observedSourceBytes = input;
    req.rootDirectory = root;
    const auto exported = G::publishGen5WorkingCopy(ws, req);
    assert(exported.success && exported.error.empty());
    FILE* manifest = std::fopen((exported.directory + "/EXPORT_MANIFEST.txt").c_str(), "rb");
    assert(manifest);
    char text[4096]{};
    const size_t n = std::fread(text, 1, sizeof(text) - 1, manifest);
    assert(std::fclose(manifest) == 0 && n > 0);
    assert(std::string_view(text, n).find("BAG_CHANGED=YES") != std::string_view::npos);
    for (const char* f : {"/original_backup.sav", "/edited.sav", "/EXPORT_MANIFEST.txt"})
        assert(std::remove((exported.directory + f).c_str()) == 0);
    assert(::rmdir(exported.directory.c_str()) == 0);
    assert(::rmdir((root + "/account-" + req.profileIdentity).c_str()) == 0);
    assert(::rmdir(root.c_str()) == 0);

    // Discard All restores the source bag; no export without changes.
    ws.discardAll();
    assert(!ws.hasChanges() && !ws.bagDirty());
    assert(!G::buildStagedSaveCandidate(ws, &error));
}

void unrecognizedBagFailsClosed() {
    // Potion (Medicine) stored in the Items pocket: the bag is unavailable, but
    // the save itself (trainer/party/boxes) still opens.
    const auto input = fixture(G::SaveFamily::BlackWhite, 21, false, {{0, 17, 1}});
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, "black_nds", &error,
                                                   G::SaveCopySelection::Primary);
    assert(parsed && parsed->partyCount() == 1);
    assert(!G::decodeBag5(*parsed));
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    assert(!ws.stagedBag(&error) && !error.empty());
    // Over-maximum quantity also fails closed.
    const auto big = fixture(G::SaveFamily::BlackWhite, 21, false, {{0, 4, 1000}});
    const auto parsedBig = G::Gen5ReadOnlySave::parse(big, "black_nds", &error,
                                                      G::SaveCopySelection::Primary);
    assert(parsedBig && !G::decodeBag5(*parsedBig));
    // Wrong block length is rejected outright.
    std::vector<uint8_t> shortBlock(10, 0);
    assert(!G::decodeBag5(G::SaveFamily::BlackWhite, shortBlock));
}

} // namespace

// Phase 3 #6 / Q09 boundary: Gen V pockets have exactly as many native slots as legal items
// (PKHeX pouch lists; bagPocketLayout5). Medicine at capacity - 1 accepts the last legal item;
// at capacity every add is refused (duplicates) and refusal changes nothing; stack max 999.
void medicineCapacityBoundary(G::SaveFamily family, std::string_view game, uint8_t version) {
    const auto& medicine = G::BagCatalog5::kMedicine;
    std::vector<Stack> stacks;
    for (size_t i = 0; i + 1 < medicine.size(); ++i) stacks.push_back({3, medicine[i], 1});
    const auto input = fixture(family, version, false, stacks);
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, game, &error, G::SaveCopySelection::Primary);
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    auto* staged = ws.stagedBag(&error);
    assert(staged);
    assert(G::bagPocketLayout5(family, 3).slots == medicine.size());
    assert(staged->add(3, medicine.back(), 1, &error));            // capacity - 1 -> capacity
    assert(staged->current()[3].size() == medicine.size());
    const std::vector<uint8_t> full(staged->staged().begin(), staged->staged().end());
    for (const uint16_t id : {medicine.front(), medicine.back()})
        assert(!staged->add(3, id, 1, &error));                    // at capacity: refused
    assert(std::equal(full.begin(), full.end(), staged->staged().begin(), staged->staged().end()));
    assert(staged->setQuantity(3, 0, 999, &error));                // native stack maximum
    assert(!staged->setQuantity(3, 0, 1000, &error));
    assert(staged->current()[3][0].count == 999);
}

int main() {
    medicineCapacityBoundary(G::SaveFamily::BlackWhite, "white_nds", 20);
    medicineCapacityBoundary(G::SaveFamily::Black2White2, "white2_nds", 22);
    exercise(G::SaveFamily::BlackWhite, "white_nds", 20, false);
    exercise(G::SaveFamily::BlackWhite, "black_nds", 21, true);
    exercise(G::SaveFamily::Black2White2, "white2_nds", 22, false);
    exercise(G::SaveFamily::Black2White2, "black2_nds", 23, true);
    unrecognizedBagFailsClosed();
    std::cout << "Gen V native Bag: BW/B2W2 decode, staged quantity/add/remove, "
                 "export candidate and verified working copy PASS\n";
}
