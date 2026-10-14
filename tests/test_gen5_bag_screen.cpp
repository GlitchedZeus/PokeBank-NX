// Gen V Bag in the shared Items/Inventory surface: host UI-model contract.
// Same A/X/Y/L/R rules as the Gen III/IV inventory contract; Key Items and
// TMs/HMs are view-only; an unrecognized bag is unavailable; Review lists Bag
// changes; exports carry Bag edits alone and combined with PK5 edits.
#include "Integration/Gen5/Gen5BagScreen.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "UI/InventoryUIContract.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
using Screen = G::Gen5BagScreen;
using Surface = Screen::Surface;

namespace {

struct Stack { size_t pocket; uint16_t id; uint16_t count; };

std::span<const L::SaveBlock> blocksFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                               : std::span<const L::SaveBlock>(L::Black2White2);
}
size_t copySizeFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
}

// Checksum-valid synthetic SAV5 (selected primary copy) with one party PK5
// and the given bag stacks. Synthetic only; no retail data.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version, const std::vector<Stack>& stacks) {
    const size_t copySize = copySizeFor(family);
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    sav[0x19400 + 0x1F] = version;
    sav[L::PartyOffset + 4] = 1;
    std::vector<uint8_t> party(C::PartySize, 0);
    party[0] = 0x23; party[1] = 0x01;
    C::write16(party, 8, 25);
    party[0x8C] = 30;
    const auto encrypted = C::encryptCandidate(party);
    std::copy(encrypted.begin(), encrypted.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    std::vector<size_t> next(G::BagPocket5Count, 0);
    const size_t items = G::itemsBlockOffset(family);
    for (const auto& s : stacks) {
        const auto spec = G::bagPocketLayout5(family, s.pocket);
        const size_t at = items + spec.offset + next[s.pocket]++ * 4;
        C::write16(sav, at, s.id);
        C::write16(sav, at + 2, s.count);
    }
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
    for (const auto& b : blocksFor(family)) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

bool hasGlyph(const std::vector<Screen::FooterEntry>& footer, std::string_view glyph) {
    return std::any_of(footer.begin(), footer.end(),
                       [&](const auto& e) { return glyph == e.glyph; });
}

const auto okRefresh = [](std::string&) { return true; };

void unavailableBag() {
    // Potion stored in the general Items pocket: the bag fails closed while the
    // Trainer/Party/Boxes presentation stays available.
    const auto input = fixture(G::SaveFamily::BlackWhite, 21, {{0, 17, 1}});
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, "black_nds", &error, G::SaveCopySelection::Primary);
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    Screen bag;
    assert(!Screen::available(ws));
    const auto can = bag.availability(ws);
    assert(!can.editAmount && !can.addItem && !can.removeItem);
    const auto footer = bag.footer(ws);
    assert(!hasGlyph(footer, "A") && !hasGlyph(footer, "X") && !hasGlyph(footer, "Y"));
    assert(hasGlyph(footer, "L/R") && hasGlyph(footer, "B"));
    assert(bag.status(ws).find("unavailable") != std::string::npos);
    assert(!bag.beginQuantity(ws, error) && !error.empty());
    assert(!bag.openAdd(ws, error) && bag.surface() == Surface::Browse);
    assert(!bag.requestRemove(ws, error) && bag.surface() == Surface::Browse);
    assert(!ws.hasChanges());

    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    assert(trainer && trainer->items.empty() && trainer->getPartySize() == 1);
}

void exercise(G::SaveFamily family, std::string_view game, uint8_t version) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const uint16_t key = b2w2 ? 627 : 437;
    const std::vector<Stack> stacks = {
        {0, 4, 30}, {0, 213, 1}, {1, key, 1}, {2, 328, 1}, {3, 17, 12}, {3, 26, 3}, {4, 149, 5},
    };
    const auto input = fixture(family, version, stacks);
    const auto pristine = input;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, game, &error, G::SaveCopySelection::Primary);
    assert(parsed && error.empty());

    // The shared Trainer presents the five native pockets in the Items panel.
    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, std::string(game), error);
    assert(trainer && trainer->items.size() == G::BagPocket5Count);
    assert(trainer->items[0].size() == 2 && trainer->items[0][0].itemId == 4);
    assert(trainer->items[1].size() == 1 && trainer->items[1][0].itemId == key);
    assert(std::string_view(G::bagPocketName5(2)) == "TMs & HMs");
    auto& ws = trainer->stagedPokemon();
    auto& bag = trainer->bagScreen();
    auto& shared = trainer->sharedScreen();
    const auto refresh = [&](std::string& why) { return trainer->refreshStagedPokemonPresentation(why); };

    // Controller/touch contract mirrors the shared inventory contract.
    using PokeBank::UIModel::InventoryAction;
    using PokeBank::UIModel::InventoryInput;
    using PokeBank::UIModel::inventoryAction;
    static_assert(inventoryAction(InventoryInput::A) == InventoryAction::EditAmount);
    static_assert(inventoryAction(InventoryInput::X) == InventoryAction::AddItem);
    static_assert(inventoryAction(InventoryInput::Y) == InventoryAction::RemoveItem);
    assert(Screen::available(ws));
    auto can = bag.availability(ws);
    assert(can.editAmount && can.addItem && can.removeItem);
    auto footer = bag.footer(ws);
    assert(hasGlyph(footer, "A") && hasGlyph(footer, "X") && hasGlyph(footer, "Y") && !hasGlyph(footer, "+"));

    // Pocket navigation wraps across all five native pockets (L/R and arrows).
    bag.changePocket(-1);
    assert(bag.pocket() == 4 && bag.row() == 0);
    bag.changePocket(1);
    assert(bag.pocket() == 0);

    // Key Items and TMs/HMs: view only, never added or removed.
    for (size_t pocket : {size_t{1}, size_t{2}}) {
        while (bag.pocket() != pocket) bag.changePocket(1);
        can = bag.availability(ws);
        assert(!can.editAmount && !can.addItem && !can.removeItem);
        footer = bag.footer(ws);
        assert(!hasGlyph(footer, "A") && !hasGlyph(footer, "X") && !hasGlyph(footer, "Y"));
        assert(bag.status(ws).find("view only") != std::string::npos);
        assert(!bag.openAdd(ws, error) && error.find("cannot be added") != std::string::npos);
        assert(!bag.requestRemove(ws, error) && error.find("cannot be removed") != std::string::npos);
        assert(!bag.beginQuantity(ws, error));
        assert(bag.surface() == Surface::Browse && !ws.hasChanges());
    }
    while (bag.pocket() != 0) bag.changePocket(1);

    // Touch: first tap focuses a row, a tap on the focused row acts as A.
    assert(!bag.tapRow(1, ws) && bag.row() == 1);
    assert(bag.tapRow(1, ws));
    assert(!bag.tapRow(9, ws) && bag.row() == 1);
    bag.moveRow(-5, ws);
    assert(bag.row() == 0);
    bag.moveRow(50, ws);
    assert(bag.row() == 1);
    bag.moveRow(-1, ws);

    // A = quantity keypad. A stale request (cursor moved) is refused.
    auto request = bag.beginQuantity(ws, error);
    assert(request && request->itemId == 4 && request->current == 30 && request->maximum == 999);
    bag.moveRow(1, ws);
    assert(!bag.applyQuantity(ws, *request, 50, okRefresh, error) && !ws.hasChanges());
    bag.moveRow(-1, ws);
    assert(!bag.applyQuantity(ws, *request, 0, okRefresh, error));
    assert(!bag.applyQuantity(ws, *request, 1000, okRefresh, error));
    // A refused presentation refresh rolls the staged bag back exactly.
    assert(!bag.applyQuantity(ws, *request, 77, [](std::string& why) { why = "refresh refused"; return false; }, error));
    assert(!ws.hasChanges() && trainer->items[0][0].count == 30);
    assert(bag.applyQuantity(ws, *request, 99, refresh, error) && error.empty());
    assert(ws.bagDirty() && trainer->items[0][0].count == 99);
    assert(trainer->hasStagedChanges());

    // X = Add picker in Medicine: existing stacks are excluded; touch picks.
    while (bag.pocket() != 3) bag.changePocket(1);
    assert(bag.openAdd(ws, error) && bag.surface() == Surface::AddPicker);
    const auto& choices = bag.addChoices();
    assert(std::find(choices.begin(), choices.end(), uint16_t{17}) == choices.end());
    assert(std::find(choices.begin(), choices.end(), uint16_t{26}) == choices.end());
    const auto fullRestore = std::find(choices.begin(), choices.end(), uint16_t{591});
    assert(fullRestore != choices.end());
    const size_t index = static_cast<size_t>(fullRestore - choices.begin());
    assert(!bag.tapAddChoice(index) && bag.chosenAddItem() == 591);
    assert(bag.tapAddChoice(index));
    assert(bag.addMaximum(ws) == 999);
    assert(!bag.confirmAdd(ws, 23, 1, okRefresh, error));        // not the chosen row
    assert(!bag.confirmAdd(ws, 591, 0, okRefresh, error));
    assert(!bag.confirmAdd(ws, 591, 1000, okRefresh, error));
    assert(bag.surface() == Surface::AddPicker);
    bag.cancel();                                                  // B is inert
    assert(bag.surface() == Surface::Browse && trainer->items[3].size() == 2);
    assert(bag.openAdd(ws, error));
    while (bag.chosenAddItem() != 591) bag.moveAddCursor(1);
    assert(bag.confirmAdd(ws, 591, 4, refresh, error) && error.empty());
    assert(bag.surface() == Surface::Browse && bag.row() == 2);
    assert(trainer->items[3].size() == 3 && trainer->items[3][2].itemId == 591 && trainer->items[3][2].count == 4);

    // Y = Remove with confirmation. B cancels; a second Y stages the removal.
    bag.moveRow(-2, ws);
    assert(bag.requestRemove(ws, error) && bag.surface() == Surface::ConfirmRemove);
    assert(bag.pendingRemove() && bag.pendingRemove()->itemId == 17);
    assert(hasGlyph(bag.footer(ws), "Y") && hasGlyph(bag.footer(ws), "B"));
    bag.cancel();
    assert(trainer->items[3].size() == 3);
    assert(bag.requestRemove(ws, error));
    assert(bag.confirmRemove(ws, refresh, error) && error.empty());
    assert(bag.surface() == Surface::Browse && trainer->items[3].size() == 2);
    assert(trainer->items[3][0].itemId == 26);

    // Review lists every Bag change, with + available from the Items panel.
    assert(hasGlyph(bag.footer(ws), "+"));
    const auto changes = ws.bagReviewChanges();
    assert(changes.size() == 3);
    auto find = [&](uint16_t id) {
        return *std::find_if(changes.begin(), changes.end(), [&](const auto& c) { return c.itemId == id; });
    };
    assert(find(4).pocket == 0 && find(4).before == 30 && find(4).after == 99);
    assert(find(17).before == 12 && find(17).after == 0);
    assert(find(591).before == 0 && find(591).after == 4);
    assert(G::Gen5SharedScreenState::reviewRowCount(ws) == 3);
    assert(shared.openReviewFromBrowse(ws) && shared.surface() == G::Gen5SharedScreenState::Surface::Review);
    (void)shared.back();
    assert(shared.surface() == G::Gen5SharedScreenState::Surface::Browse);

    // Bag-only export: only the Items block and its CRCs differ; reparse holds.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && out->bagChanged && out->changedRecords == 0);
    const auto& itemsBlock = blocksFor(family)[G::ItemsBlockIndex];
    const auto& terminal = blocksFor(family).back();
    for (size_t i = 0; i < input.size(); ++i) {
        if (out->bytes[i] == input[i]) continue;
        const bool inItems = i >= itemsBlock.offset && i < itemsBlock.offset + itemsBlock.length;
        const bool crc = i == itemsBlock.checksumOffset || i == itemsBlock.checksumOffset + 1 ||
                         i == itemsBlock.mirrorOffset || i == itemsBlock.mirrorOffset + 1 ||
                         i == terminal.checksumOffset || i == terminal.checksumOffset + 1 ||
                         i == terminal.mirrorOffset || i == terminal.mirrorOffset + 1;
        assert(inItems || crc);
    }
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(reopened);
    const auto exported = G::decodeBag5(*reopened);
    assert(exported && (*exported)[0][0].count == 99 && (*exported)[3].size() == 2);

    // Bag + PK5 edit export together; Review counts both.
    using Field = G::StagedPokemon5Record::Field;
    assert(ws.stageParty(0, Field::Friendship, 0, 200, &error));
    assert(G::Gen5SharedScreenState::reviewRowCount(ws) == 4);
    const auto both = G::buildStagedSaveCandidate(ws, &error);
    assert(both && both->bagChanged && both->changedRecords == 1);
    const auto both2 = G::Gen5ReadOnlySave::parse(both->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(both2 && (*G::decodeBag5(*both2))[3][1].itemId == 591);
    assert(both2->partyPokemon(0) && both2->partyPokemon(0)->friendship() == 200);

    // The source bytes were never touched by any of the above.
    assert(input == pristine);

    // Discard All restores the source bag in the shared panel.
    ws.discardAll();
    std::string why;
    assert(trainer->refreshStagedPokemonPresentation(why));
    assert(!ws.hasChanges() && trainer->items[0][0].count == 30 && trainer->items[3].size() == 2);
    assert(!shared.openReviewFromBrowse(ws));
    bag.reset();
    assert(bag.pocket() == 0 && bag.row() == 0 && bag.surface() == Surface::Browse);
}

void fullPocketRefusesAdd() {
    // Every native Berry slot used: Add is refused before any picker opens.
    std::vector<Stack> stacks;
    for (size_t i = 0; i < G::BagCatalog5::kBerry.size(); ++i)
        stacks.push_back({4, G::BagCatalog5::kBerry[i], 1});
    const auto input = fixture(G::SaveFamily::BlackWhite, 20, stacks);
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, "white_nds", &error, G::SaveCopySelection::Primary);
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    Screen bag;
    bag.changePocket(-1);
    assert(bag.pocket() == 4);
    assert(!bag.openAdd(ws, error) && bag.surface() == Surface::Browse && !error.empty());
}

} // namespace

int main() {
    unavailableBag();
    exercise(G::SaveFamily::BlackWhite, "white_nds", 20);
    exercise(G::SaveFamily::BlackWhite, "black_nds", 21);
    exercise(G::SaveFamily::Black2White2, "white2_nds", 22);
    exercise(G::SaveFamily::Black2White2, "black2_nds", 23);
    fullPocketRefusesAdd();
    std::cout << "Gen V Bag shared Items surface: pockets, A/X/Y parity, view-only story pockets, "
                 "Review and export PASS\n";
}
