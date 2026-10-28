#include "Inventory/ItemQuantityPolicy.h"
#include "UI/ClassicInventoryUIModel.h"
#include "UI/InventoryHelp.h"
#include "UI/GlyphInstruction.h"
#include "UI/InventoryUIContract.h"
#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <cassert>
#include <iostream>

using PokeVault::Inventory::ClassicPocket;
using namespace PokeBank::UIModel;

int main() {
    const auto yellow = classicInventoryGame("yellow_gb");
    const auto gold = classicInventoryGame("gold_gbc");
    const auto emerald = classicInventoryGame("emerald_gba");
    const auto firered = classicInventoryGame("firered_gba");
    assert(yellow && gold && emerald && firered);

    assert(classicInventoryCategories(*yellow).size() == 6);
    assert(classicInventoryPocketAt(*yellow, 0) == ClassicPocket::Items);
    assert(classicInventoryPocketAt(*yellow, 1) == ClassicPocket::Medicines);
    assert(classicInventoryPocketAt(*yellow, 2) == ClassicPocket::Balls);
    assert(classicInventoryPocketAt(*yellow, 5) == ClassicPocket::PCItems);
    assert(!classicInventoryPocketAt(*yellow, 6));

    assert(classicInventoryCategories(*gold).size() == 5);
    assert(classicInventoryPocketAt(*gold, 0) == ClassicPocket::Items);
    assert(classicInventoryPocketAt(*gold, 1) == ClassicPocket::Balls);
    assert(classicInventoryPocketAt(*gold, 2) == ClassicPocket::KeyItems);
    assert(classicInventoryPocketAt(*gold, 3) == ClassicPocket::TMHM);
    assert(classicInventoryPocketAt(*gold, 4) == ClassicPocket::PCItems);

    assert(classicInventoryCategories(*emerald).size() == 6);
    assert(classicInventoryPocketAt(*emerald, 4) == ClassicPocket::Berries);
    assert(classicInventoryCategories(*firered).size() == 6);
    // Hardware regression: arrow taps must reach every displayed pocket.
    // Gen I and III have six categories, Gen II has five; the inherited
    // RBY decoder's two raw pockets must never be used to wrap these tabs.
    for (const auto game : {*yellow, *gold, *emerald, *firered}) {
        const int count = static_cast<int>(classicInventoryCategories(game).size());
        int selected = 0;
        for (int i = 0; i < count; ++i) {
            assert(selected == i);
            assert(classicInventoryPocketAt(game, selected).has_value());
            selected = classicInventoryShiftCategory(game, selected, +1);
        }
        assert(selected == 0);
        for (int i = 0; i < count; ++i) {
            selected = classicInventoryShiftCategory(game, selected, -1);
            assert(selected == (count - 1 - i));
        }
        assert(selected == 0);
        assert(classicInventoryShiftCategory(game, count - 1, +1) == 0);
        assert(classicInventoryShiftCategory(game, 0, -1) == count - 1);
        assert(classicInventoryShiftCategory(game, count + 99, +1) == 0);
    }

    const auto yellowBalls = classicInventoryAddableItems(*yellow, ClassicPocket::Balls);
    assert(std::find(yellowBalls.begin(), yellowBalls.end(), 4) != yellowBalls.end());
    assert(std::find(yellowBalls.begin(), yellowBalls.end(), 851) == yellowBalls.end());
    const auto goldBalls = classicInventoryAddableItems(*gold, ClassicPocket::Balls);
    assert(std::find(goldBalls.begin(), goldBalls.end(), 851) == goldBalls.end());
    assert(!PokeVault::Inventory::isAddableItem(*emerald, ClassicPocket::Items, 500));
    assert(!PokeVault::Inventory::isAddableItem(*firered, ClassicPocket::TMHM, 500));

    // Generation II keeps exact ids/catalog membership while player-facing picker text is polished.
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::Items, 59) == "Good Rod");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::Items, 16) == "Hyper Potion");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::Balls, 5) == "Poké Ball");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::Items, 36) == "Nugget");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::Items, 13) == "Paralyze Heal");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::Items, 99) == "Black Apricorn");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::TMHM, 201) == "TM10 — Hidden Power");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::TMHM, 244) == "HM02 — Fly");
    assert(PokeVault::Inventory::displayItemName(*gold, ClassicPocket::PCItems, 201) == "TM10 — Hidden Power");

    assert(classicInventoryEmptyState(true, 0));
    assert(!classicInventoryEmptyState(true, 1));
    assert(!classicInventoryEmptyState(false, 0));
    assert(!classicInventoryQuantityEditable(*gold, ClassicPocket::KeyItems, 54));
    assert(!classicInventoryQuantityEditable(*gold, ClassicPocket::TMHM, 247));
    assert(classicInventoryQuantityEditable(*gold, ClassicPocket::Items, 18));
    assert(classicInventoryMaximumQuantity(*gold, ClassicPocket::Items, 18) == 99);
    assert(classicInventoryNeedsStoryWarning(*gold, ClassicPocket::KeyItems, 54));
    assert(!classicInventoryNeedsStoryWarning(*gold, ClassicPocket::Balls, 5));
    // Shared rule (SharedBagAddFlow needsKeyItemWarning): a Key Item in the PC warns too; a
    // regular PC item and an HM (unique, but not story-gated) do not.
    assert(classicInventoryNeedsStoryWarning(*gold, ClassicPocket::PCItems, 7));
    assert(!classicInventoryNeedsStoryWarning(*gold, ClassicPocket::PCItems, 18));
    assert(!classicInventoryNeedsStoryWarning(*gold, ClassicPocket::TMHM, 243));

    assert(classicInventoryAction(ClassicInventoryInput::A) == ClassicInventoryAction::EditAmount);
    assert(classicInventoryAction(ClassicInventoryInput::X) == ClassicInventoryAction::AddItem);
    assert(classicInventoryAction(ClassicInventoryInput::Y) == ClassicInventoryAction::RemoveItem);
    assert(classicInventoryAction(ClassicInventoryInput::L) == ClassicInventoryAction::PreviousCategory);
    assert(classicInventoryAction(ClassicInventoryInput::R) == ClassicInventoryAction::NextCategory);
    assert(classicInventoryAction(ClassicInventoryInput::Plus) == ClassicInventoryAction::Options);
    assert(classicInventoryAction(ClassicInventoryInput::Minus) == ClassicInventoryAction::Help);
    assert(classicInventoryAction(ClassicInventoryInput::B) == ClassicInventoryAction::Back);
    assert(classicInventoryAction(ClassicInventoryInput::X) == inventoryAction(InventoryInput::X));
    assert(classicInventoryAction(ClassicInventoryInput::Y) == inventoryAction(InventoryInput::Y));

    const auto emptyActions = classicInventoryActionAvailability(true, false, false);
    assert(!emptyActions.editAmount);
    assert(emptyActions.addItem);
    assert(!emptyActions.removeItem);
    const auto quantityActions = classicInventoryActionAvailability(true, true, true);
    assert(quantityActions.editAmount);
    assert(quantityActions.addItem);
    assert(quantityActions.removeItem);
    const auto fixedActions = classicInventoryActionAvailability(true, true, false);
    assert(!fixedActions.editAmount);
    assert(fixedActions.addItem);
    assert(fixedActions.removeItem);
    const auto unsupportedActions = classicInventoryActionAvailability(false, false, false);
    assert(!unsupportedActions.editAmount && !unsupportedActions.addItem && !unsupportedActions.removeItem);

    // Phase 2: Remove asks first. Only Y (the same confirm as Release) removes; B cancels and wins
    // a same-frame Y+B; A and no input do nothing.
    using PokeBank::UIModel::ClassicRemoveConfirm;
    static_assert(classicRemoveConfirmStep(true, false) == ClassicRemoveConfirm::Remove);
    static_assert(classicRemoveConfirmStep(false, true) == ClassicRemoveConfirm::Cancel);
    static_assert(classicRemoveConfirmStep(true, true) == ClassicRemoveConfirm::Cancel);
    static_assert(classicRemoveConfirmStep(false, false) == ClassicRemoveConfirm::None);
    // One per-item quantity query: unique items (key items, HMs) have no editor and are fixed at 1;
    // ordinary items edit 1..99. Every addable Gen I/II item answers consistently.
    for (const auto game : {PokeVault::Inventory::ClassicGame::Red, PokeVault::Inventory::ClassicGame::Yellow,
                            PokeVault::Inventory::ClassicGame::Gold, PokeVault::Inventory::ClassicGame::Crystal}) {
        for (const auto pocket : PokeVault::Inventory::supportedPockets(game)) {
            for (const uint16_t item : PokeVault::Inventory::addableItems(game, pocket)) {
                const auto q = classicItemQuantity(game, pocket, item);
                assert(q.editable == classicInventoryQuantityEditable(game, pocket, item));
                if (pocket == PokeVault::Inventory::ClassicPocket::KeyItems)
                    assert(!q.editable && q.minimum == 1 && q.maximum == 1);
                if (q.editable) assert(q.minimum == 1 && q.maximum == 99);
                else assert(q.maximum == 1);
            }
        }
    }
    // Gen III uses the same query, which is QuantityPolicy's native pocket cap: FireRed/LeafGreen
    // bag pockets stack to 999 (PKHeX PlayerBag3FRLG), Ruby/Sapphire/Emerald bag pockets to 99 with
    // Berries and PC Items to 999 (PlayerBag3RS / PlayerBag3E). Key items and HMs stay unique.
    {
        using PokeVault::Inventory::ClassicGame;
        using PokeVault::Inventory::ClassicPocket;
        namespace QP = PokeVault::Inventory::QuantityPolicy;
        bool sawFrlg999 = false, sawRse99 = false, sawRse999 = false;
        for (const auto game : {ClassicGame::Ruby, ClassicGame::Sapphire, ClassicGame::Emerald,
                                ClassicGame::FireRed, ClassicGame::LeafGreen}) {
            for (const auto pocket : PokeVault::Inventory::supportedPockets(game)) {
                for (const uint16_t item : PokeVault::Inventory::addableItems(game, pocket)) {
                    const auto q = classicItemQuantity(game, pocket, item);
                    const auto shared = QP::quantityRule(static_cast<QP::Game>(static_cast<uint8_t>(game)),
                        pocket == ClassicPocket::Items ? QP::Pocket::Items :
                        pocket == ClassicPocket::Medicines ? QP::Pocket::Medicine :
                        pocket == ClassicPocket::Balls ? QP::Pocket::Balls :
                        pocket == ClassicPocket::KeyItems ? QP::Pocket::KeyItems :
                        pocket == ClassicPocket::TMHM ? QP::Pocket::TMHM :
                        pocket == ClassicPocket::Berries ? QP::Pocket::Berries : QP::Pocket::PCItems, item);
                    assert(shared.valid && q.maximum == shared.maximum && q.editable == shared.editable);
                    if (pocket == ClassicPocket::KeyItems) assert(!q.editable && q.maximum == 1);
                    const bool frlg = game == ClassicGame::FireRed || game == ClassicGame::LeafGreen;
                    if (q.editable && frlg) { assert(q.maximum == 999); sawFrlg999 = true; }
                    if (q.editable && !frlg) {
                        const bool big = pocket == ClassicPocket::Berries || pocket == ClassicPocket::PCItems;
                        assert(q.maximum == (big ? 999 : 99));
                        (big ? sawRse999 : sawRse99) = true;
                    }
                }
            }
        }
        assert(sawFrlg999 && sawRse99 && sawRse999);
        assert(classicItemQuantity(ClassicGame::FireRed, ClassicPocket::Items, 0).maximum == 0);
    }
    {
        std::ifstream in("src/UI/ClassicInventoryOverlay.cpp", std::ios::binary);
        const std::string overlay((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        // Y on an item opens the question instead of removing; the question owns input and footer.
        const auto removeAction = overlay.find("if (action == PokeBank::UIModel::ClassicInventoryAction::RemoveItem) {");
        assert(removeAction != std::string::npos);
        const auto removeBody = overlay.substr(removeAction, 400);
        assert(removeBody.find("state.removeConfirmActive = true;") != std::string::npos);
        assert(removeBody.find("queueMutationWithWarning") == std::string::npos);
        assert(overlay.find("classicRemoveConfirmStep((down & HidNpadButton_A) != 0,") != std::string::npos);
        assert(overlay.find("drawNavBar(fb, {{\"Y\", \"Remove\"}, {\"B\", \"Cancel\"}});") == std::string::npos);
        // The add and edit quantity editors ask the one per-item query.
        assert(overlay.find("PokeVault::Inventory::quantityRule(") == std::string::npos);
    }

    {
        // Round 7 (B04): the Gen I-III Bag draws the ONE shared "Inventory Controls" Help
        // (InventoryHelp.h) as the Gen IV / V Bag does. Every row after the banner names real
        // buttons, so drawGlyphHelpLine draws them as badges (R2-BUG-013); + is Review (B03), and
        // the stale "+ Inventory Options" / "X Add remains available" rows are gone.
        namespace GI = PokeBank::UIModel::GlyphInstruction;
        namespace InvHelp = PokeBank::UIModel::InventoryHelp;
        const auto glyphs = [](std::string_view line) {
            std::string out;
            for (const auto& seg : GI::helpLine(line)) {
                if (seg.kind != GI::Kind::Glyph) continue;
                if (!out.empty()) out += ",";
                out += seg.text;
            }
            return out;
        };
        for (int gen = 1; gen <= 5; ++gen) {
            const auto lines = InvHelp::lines(gen);
            // Owner Oct 28 (Feature B): ZR opens Top Up / Add Missing on every Gen I-V Bag.
            const char* expected[] = {"", "D-pad/Stick", "L/R", "A", "X,Y", "ZR", "A,B", "+", "B", "B,-"};
            assert(lines.size() == std::size(expected));
            for (std::size_t i = 0; i < lines.size(); ++i) {
                if (glyphs(lines[i]) != expected[i])
                    std::cerr << "Gen " << gen << " help row " << i << ": got {" << glyphs(lines[i])
                              << "} want {" << expected[i] << "}\n";
                assert(glyphs(lines[i]) == expected[i]);
                assert(lines[i].find("Inventory Options") == std::string::npos);
                assert(lines[i].find("remains available") == std::string::npos);
            }
            assert(lines[7].rfind("+   Review staged changes", 0) == 0);
            assert(lines[5].rfind("ZR   Top Up / Add Missing", 0) == 0);
            assert(lines[2].find(gen <= 3 ? "category" : "pocket") != std::string::npos);
        }
        std::ifstream in("src/UI/ClassicInventoryOverlay.cpp", std::ios::binary);
        const std::string overlay((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        // Owner Oct 28 (Bag Help #2, d9105fae): the Gen I-III Bag Help is the shared content-sized
        // modal (ScreenChrome::drawInfoOverlay, which draws every row through drawGlyphHelpLine) with
        // the InventoryHelp title and rows, no longer a 900 x 610 panel of its own.
        const auto help = overlay.find("if (state.helpActive && !state.pickerActive && !state.discardConfirmActive) {");
        const auto helpEnd = overlay.find("return;", help);
        assert(help != std::string::npos && helpEnd != std::string::npos);
        const auto body = overlay.substr(help, helpEnd - help);
        assert(body.find("drawInfoOverlay(fb, InvHelp::kTitle, InvHelp::lines(generation));") != std::string::npos);
        assert(body.find("\"Inventory Help\"") == std::string::npos);
        assert(std::string(InvHelp::kTitle) == "Inventory Controls");
    }

    std::cout << "classic inventory UI model + polished Gen II display: PASS\n";
}
