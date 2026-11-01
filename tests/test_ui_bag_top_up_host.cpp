// Top Up / Add Missing (owner Oct 28, Feature B) through the REAL TrainerViewScreen update / draw
// (tests/host-ui harness) for a Gen I (Red), Gen III (FireRed), Gen IV (Platinum) and Gen V (Black) Bag:
//   Bag footer names ZR -> ZR opens SETUP (Mode / Scope / Amount / Preview / Cancel, one footer) ->
//   A on Amount asks the numpad -> A on Preview draws the PREVIEW whose change rows ("Potion 3 → 50",
//   "+ <item> ×50") lie inside the modal above the footer -> B goes back with NOTHING staged ->
//   A on Preview, A again stages it -> the staged Bag holds the new counts -> + opens Review, which
//   lists the Bag change; Discard restores the source.
// A Japanese Gen I save (read-only while Integration/Common/JapaneseEditingGate.h is closed) never
// offers it: no footer entry, ZR opens nothing, nothing is staged.
// Round 11 (owner H04 / H34): SETUP opens on the Scope row at "< All Bag pockets >" on a Bag tab; the
// owner's H04 steps then pick "This pocket" (Left), which the flow below follows. scopeFlow previews
// All Bag pockets and, in Gen I-III, PC Items (Gen IV / V never offer a PC scope), staging nothing.
// Synthetic saves only (HostUiFixtures / fuzz_synthetic_saves.h).
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Common/JapaneseEditingGate.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Utils/Gen12KanaText.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace MG = PokeBank::UIModel::SharedMenuGeometry;

namespace {

int failures = 0;
int checks = 0;
bool verbose = false;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

struct Driver {
    explicit Driver(UI::TrainerViewScreen& s) : screen(s) {}
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
        if (verbose) {
            std::cerr << "---- frame\n";
            for (const auto& t : HostUi::texts) std::cerr << "  " << t << '\n';
        }
    }
    void press(u64 button) { frame(button); frame(); }
    bool drawn(const std::string& needle) const {
        return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                           [&](const std::string& t) { return t.find(needle) != std::string::npos; });
    }
};

// The footer the player sees: the visible text on the footer band (a covered footer is painted
// over by the modal's own bar, HostUi layout recorder), and no two visible runs overlap.
bool footerShows(const std::string& label) {
    return std::any_of(HostUi::visibleText.begin(), HostUi::visibleText.end(), [&](const HostUi::TextRect& t) {
        return t.y >= MG::kScreenH - MG::kFooterH && t.text == label;
    });
}
bool noOverlaps() {
    const auto& t = HostUi::visibleText;
    for (std::size_t i = 0; i < t.size(); ++i)
        for (std::size_t j = i + 1; j < t.size(); ++j) {
            const int ox = std::min(t[i].x + t[i].w, t[j].x + t[j].w) - std::max(t[i].x, t[j].x);
            const int oy = std::min(t[i].y + t[i].h, t[j].y + t[j].h) - std::max(t[i].y, t[j].y);
            if (ox > 0 && oy > 0) {
                std::cerr << "  overlap: \"" << t[i].text << "\" / \"" << t[j].text << "\"\n";
                return false;
            }
        }
    return true;
}

// Round 11 QA: Gen I's Bag is ONE 20-slot list shared by every category (pret pokered
// wNumBagItems), so its whole-Bag scope says so instead of implying per-pocket room.
std::string allBagLabel(const std::string& game) {
    return game == "Red" ? "Whole Bag (20 slots shared)" : "All Bag pockets";
}

bool isChangeRow(const std::string& t) {
    return t.rfind("+ ", 0) == 0 || t.find("\xE2\x86\x92") != std::string::npos;
}

// The one flow, the same for every generation. `staged` reports whether the editor holds changes;
// `held` the staged count of the probe stack; Review is checked by the drawn item name.
void flow(const std::string& game, UI::TrainerViewScreen& screen, const std::function<bool()>& staged,
          const std::function<uint16_t()> held, uint16_t heldBefore, const std::string& probeName) {
    Driver d(screen);
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Items;
    screen.selectedItemIndex = 0;
    d.frame();
    d.frame();
    check(footerShows("Top Up"), game + ": the Bag footer offers ZR Top Up");
    check(std::all_of(HostUi::visibleText.begin(), HostUi::visibleText.end(),
                      [](const HostUi::TextRect& t) { return t.x + t.w <= MG::kScreenW; }),
          game + ": the Bag footer stays on screen");
    check(noOverlaps(), game + ": the Bag footer with ZR Top Up never overlaps");
    check(!staged(), game + ": nothing staged before");

    d.press(HidNpadButton_ZR);
    check(d.drawn("Top Up / Add Missing") && d.drawn("Preview changes") && d.drawn("Scope") && d.drawn("Amount"),
          game + ": ZR opens SETUP");
    check(d.drawn("< " + allBagLabel(game) + " >"), game + ": SETUP starts on Scope = " + allBagLabel(game) + " (a Bag tab)");
    if (game == "Red") check(!d.drawn("All Bag pockets"), "Red: no per-pocket wording for the shared 20-slot Bag");
    // H04 steps: Scope -> This pocket.
    d.press(HidNpadButton_Left);
    check(d.drawn("< This pocket ("), game + ": Left picks This pocket");
    check(HostUi::footerBars >= 1 && footerShows("Close") && footerShows("OK") && !footerShows("Add Item"),
          game + ": SETUP draws its own footer over the Bag's");
    check(noOverlaps(), game + ": SETUP text never overlaps");
    check(d.drawn("Key Items and HMs are never changed"), game + ": SETUP says story items are excluded");
    // Amount: A opens the numpad; 50 is the target (each item is then capped natively).
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_Down);
    HostUi::numbers.push_back({true, 50});
    const int prompts = HostUi::promptCount;
    d.press(HidNpadButton_A);
    check(HostUi::promptCount == prompts + 1, game + ": A on Amount opens the numpad");
    check(d.drawn("50 per item"), game + ": the Amount row shows 50");
    check(!staged(), game + ": SETUP stages nothing");

    // Preview: every change row drawn inside the modal, above the footer.
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(d.drawn("Preview: Top Up / Add Missing"), game + ": A on Preview changes opens PREVIEW");
    check(d.drawn(probeName + " " + std::to_string(heldBefore) + " \xE2\x86\x92 50"),
          game + ": PREVIEW names the top-up of the held stack exactly");
    const auto frame = MG::layout(MG::MenuType::BulkPreview, 200).frame;   // full height: the tallest it gets
    int rows = 0;
    for (const auto& t : HostUi::visibleText) {
        if (!isChangeRow(t.text)) continue;
        ++rows;
        check(t.x >= frame.x && t.x + t.w <= frame.right() && t.y >= frame.y && t.y + t.h <= MG::kScreenH - MG::kFooterH,
              game + ": preview row \"" + t.text + "\" fits its modal");
    }
    check(rows > 1, game + ": PREVIEW draws its change rows (" + std::to_string(rows) + ")");
    check(footerShows("Stage All") && footerShows("Back") && !footerShows("Add Item"),
          game + ": PREVIEW draws its own confirm footer (B Back, A Stage All)");
    check(noOverlaps(), game + ": PREVIEW text never overlaps");
    check(!staged(), game + ": PREVIEW stages nothing");
    // Scrolling never stages either.
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_R);
    check(!staged(), game + ": scrolling stages nothing");

    // B: back to SETUP, nothing staged, the count unchanged.
    d.press(HidNpadButton_B);
    check(!d.drawn("Preview: Top Up / Add Missing") && d.drawn("Preview changes"), game + ": B returns to SETUP");
    check(!staged() && held() == heldBefore, game + ": B stages nothing");

    // Again; A confirms: staged, the held stack now 50, the modal closed.
    d.press(HidNpadButton_A);
    check(d.drawn("Preview: Top Up / Add Missing"), game + ": PREVIEW again");
    d.press(HidNpadButton_A);
    check(!d.drawn("Preview: Top Up / Add Missing") && !d.drawn("Preview changes"), game + ": A closes the flow");
    check(staged(), game + ": A stages the plan");
    check(held() == 50, game + ": the held stack reads 50 (" + std::to_string(held()) + ")");
    check(d.drawn("Staged:") || d.drawn("topped up"), game + ": a status names the counts");

    // + opens Review, which lists the Bag change.
    d.press(HidNpadButton_Plus);
    d.frame();
    const auto visible = [](const std::string& needle) {
        return std::any_of(HostUi::visibleText.begin(), HostUi::visibleText.end(),
                           [&](const HostUi::TextRect& t) { return t.text.find(needle) != std::string::npos; });
    };
    check(visible("Review Pending Changes") && visible(probeName) &&
              visible(std::to_string(heldBefore) + " -> 50"),
          game + ": Review lists the staged Bag change (" + probeName + " " + std::to_string(heldBefore) + " -> 50)");
}

// Round 11 (H04 / H34): the other scopes on the same screen (the flow above left it on This pocket):
// All Bag pockets previews every Bag pocket (a full list says how many slots it has: `fullNote`);
// Gen I-III then offer PC Items, Gen IV / V go back to This pocket. B everywhere: nothing staged.
void scopeFlow(const std::string& game, UI::TrainerViewScreen& screen, const std::function<bool()>& staged,
               bool pc, const std::string& fullNote) {
    Driver d(screen);
    d.frame();
    if (d.drawn("Review Pending Changes")) d.press(HidNpadButton_B);   // the flow ended in Review
    check(!d.drawn("Review Pending Changes"), game + ": back on the Bag");
    d.press(HidNpadButton_ZR);
    check(d.drawn("< This pocket ("), game + ": the picked scope is kept for the session");
    d.press(HidNpadButton_Right);
    check(d.drawn("< " + allBagLabel(game) + " >"), game + ": Right picks " + allBagLabel(game));
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(d.drawn("Preview: Top Up / Add Missing") && d.drawn(allBagLabel(game) + ", up to"), game + ": whole-Bag preview");
    check(fullNote.empty() || d.drawn(fullNote), game + ": the preview names the full list (" + fullNote + ")");
    check(!staged(), game + ": the All Bag preview stages nothing");
    d.press(HidNpadButton_B);
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Up);
    d.press(HidNpadButton_Right);
    if (pc) {
        check(d.drawn("< PC Items >"), game + ": Right then offers PC Items");
        for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Down);
        d.press(HidNpadButton_A);
        check(d.drawn("Preview: Top Up / Add Missing") && d.drawn("PC Items, up to"), game + ": PC Items preview");
        d.press(HidNpadButton_B);
        for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Up);
        d.press(HidNpadButton_Right);
        check(d.drawn("< Bag + PC Items >"), game + ": then Bag + PC Items");
    } else {
        check(d.drawn("< This pocket (") && !d.drawn("PC Items"), game + ": no PC scope without PC item storage");
    }
    check(!staged(), game + ": changing scope stages nothing");
    d.press(HidNpadButton_B);
    check(!d.drawn("Preview changes") && !staged(), game + ": B closes SETUP with nothing staged");
}

// Screens stay alive for the whole run: surfaces key per-screen state by a construction serial.
std::vector<std::shared_ptr<void>> keepAlive;
template <class T>
T* keep(std::unique_ptr<T> p) {
    T* raw = p.get();
    keepAlive.push_back(std::shared_ptr<void>(std::move(p)));
    return raw;
}
UI::TrainerViewScreen* open(Trainer::Trainer& trainer, const char* title, const char* id, const char* emulator) {
    return keep(std::make_unique<UI::TrainerViewScreen>(trainer, title, std::string("/host/") + id + ".sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, id, emulator));
}

} // namespace

// A minimal synthetic Japanese Red save (layout PKHeX 8e3eb3d4 SAV1 Japanese offsets, as
// tests/test_gen1_japanese_writer.cpp): player name, Potion x3 in the Bag, empty PC, Party and
// current box, banks uninitialized, main checksum.
std::vector<uint8_t> japaneseRed() {
    std::vector<uint8_t> v(PokeVault::Integration::Gen1::kRawSaveSize, 0);
    constexpr std::size_t Name = 0x2598, Bag = 0x25C4, PcItems = 0x27DC, BoxIndex = 0x2842, Starter = 0x29B9;
    constexpr std::size_t Party = 0x2ED5, CurrentBox = 0x302D, Checksum = 0x3594, MainLength = 0x0FFC;
    std::fill_n(v.begin() + Name, 6, 0x50);
    std::vector<uint8_t> kana;
    if (Utils::Gen12Kana::encode("\xE3\x82\xB5\xE3\x83\x88\xE3\x82\xB7", 5, kana) == Utils::Gen12Kana::Result::Ok)   // サトシ
        std::copy(kana.begin(), kana.end(), v.begin() + Name);
    v[Bag] = 1; v[Bag + 1] = 0x14; v[Bag + 2] = 3; v[Bag + 3] = 0xFF;
    v[PcItems] = 0; v[PcItems + 1] = 0xFF;
    v[Starter] = 0x99;
    v[BoxIndex] = 0;
    v[Party] = 0; v[Party + 1] = 0xFF;
    v[CurrentBox] = 0; v[CurrentBox + 1] = 0xFF;
    uint8_t sum = 0xFF;
    for (std::size_t i = Name; i < Name + MainLength; ++i) sum = static_cast<uint8_t>(sum - v[i]);
    v[Checksum] = sum;
    return v;
}

int main(int argc, char** argv) {
    verbose = argc > 1 && std::string(argv[1]) == "-v";
    std::string error;

    // Gen I: QA synthetic Red with Potion (0x14) x3 in the Bag (the Medicines category).
    {
        namespace G1 = PokeVault::Integration::Gen1;
        auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen1::save());
        auto& d = *bytes;
        d[0x25C9] = 1; d[0x25CA] = 0x14; d[0x25CB] = 3; d[0x25CC] = 0xFF;   // pret wNumBagItems
        d[0x27E6] = 0; d[0x27E7] = 0xFF;
        qasynth::gen1::forge(d);
        keepAlive.push_back(bytes);
        auto parsed = G1::parse(d, G1::SourceGame::Red);
        check(static_cast<bool>(parsed), "Gen I fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error));
            check(trainer && trainer->stagedInventory(), "Gen I staged inventory: " + error);
            if (trainer && trainer->stagedInventory()) {
                auto* inv = trainer->stagedInventory();
                auto* screen = open(*trainer, "Pokemon Red", "red_gb", "mGBA");
                screen->selectedCategory = 1;   // Medicines
                const auto held = [inv]() -> uint16_t {
                    std::string e;
                    for (const auto& x : inv->entries(PokeVault::Inventory::ClassicPocket::Medicines, e))
                        if (x.itemId == 0x14) return x.quantity;
                    return 0;
                };
                flow("Red", *screen, [inv] { return inv->hasPendingChanges(); }, held, 3, "Potion");
                inv->discard();
                check(!inv->hasPendingChanges() && held() == 3, "Red: Discard restores");
                // One Potion stack: 19 of the 20 shared Bag slots are free; the Items category's 19
                // candidates take them and every other category's new items are reported.
                scopeFlow("Red", *screen, [inv] { return inv->hasPendingChanges(); }, true,
                          "Bag (every category shares 20 slots): 19 free");
            }
        }
    }
    // Japanese Gen I: read-only while the gate is closed; Top Up is not offered.
    if (!PokeVault::Integration::Common::kJapaneseClassicEditingEnabled) {
        namespace G1 = PokeVault::Integration::Gen1;
        auto bytes = std::make_shared<std::vector<uint8_t>>(japaneseRed());
        keepAlive.push_back(bytes);
        auto parsed = G1::parse(*bytes, G1::SourceGame::Red);
        check(static_cast<bool>(parsed) && parsed.save->metadata().region == G1::RegionLayout::Japanese,
              "Japanese Gen I fixture parses as Japanese");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Japanese Gen I trainer: " + error);
            if (trainer) {
                auto* screen = open(*trainer, "Pokemon Red", "red_gb", "mGBA");
                Driver d(*screen);
                d.frame();
                screen->detailViewActive = true;
                screen->selectedMode = UI::TrainerViewScreen::ViewMode::Items;
                screen->selectedCategory = 1;
                d.frame();
                d.frame();
                check(!footerShows("Top Up"), "Japanese Red: the Bag footer offers no Top Up");
                const auto* inv = trainer->stagedInventory();
                const auto before = inv ? std::vector<uint8_t>(inv->stagedBytes().begin(), inv->stagedBytes().end())
                                        : std::vector<uint8_t>{};
                d.press(HidNpadButton_ZR);
                check(!d.drawn("Preview changes") && !d.drawn("Top Up / Add Missing:"), "Japanese Red: ZR opens nothing");
                d.press(HidNpadButton_A);   // no modal is open: nothing to confirm
                check(!inv || (!inv->hasPendingChanges() &&
                               std::equal(before.begin(), before.end(), inv->stagedBytes().begin())),
                      "Japanese Red: nothing is staged");
            }
        }
    } else {
        std::cout << "Japanese Gen I gate is open: read-only check NOT TESTED\n";
    }

    // Gen III: FireRed with Potion x3 in the Items pouch (active slot, sector 1, FR/LG item key).
    {
        namespace G3 = PokeVault::Integration::Gen3;
        namespace D3 = PokeVault::Integration::Gen3::Detail;
        auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen3Fixture(0));
        auto& save = *bytes;
        const auto active = D3::validateSlot(save, 1);
        const auto sector = active.logicalSectorOffsets[1];
        const uint16_t key16 = 0xC3D4;   // low half of the fixture's security key (logical sector 0 + 0xF20)
        HostUiFixtures::put16(save, sector + 0x310, 13);
        HostUiFixtures::put16(save, sector + 0x312, static_cast<uint16_t>(3 ^ key16));
        HostUiFixtures::put16(save, sector + 0xFF6,
            D3::sectorChecksum(std::span<const uint8_t>(save).subspan(sector, D3::kChunkLengths[1])));
        keepAlive.push_back(bytes);
        auto parsed = G3::parse(save, G3::SourceGame::FireRedGBA);
        check(static_cast<bool>(parsed), "Gen III fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error));
            check(trainer && trainer->stagedInventory(), "Gen III staged inventory: " + error);
            if (trainer && trainer->stagedInventory()) {
                auto* inv = trainer->stagedInventory();
                auto* frScreen = open(*trainer, "Pokemon FireRed", "firered_gba", "mGBA");
                flow("FireRed", *frScreen,
                     [inv] { return inv->hasPendingChanges(); },
                     [inv] { return inv->stagedInventoryQuantity(G3::InventoryPouch::Items, 13); }, 3, "Potion");
                std::string keyErr;
                check(inv->inventoryEntries(G3::InventoryPouch::KeyItems, keyErr).empty(), "FireRed: no Key Item staged");
                inv->discard();
                check(!inv->hasPendingChanges() && inv->stagedBytes() == inv->originalBytes(), "FireRed: Discard restores");
                scopeFlow("FireRed", *frScreen, [inv] { return inv->hasPendingChanges(); }, true, "(42 slots): 41 free");
            }
        }
    }
    // Gen IV: Platinum with Potion x3 in Medicine and a Bicycle in Key Items (both partitions).
    {
        namespace G4 = PokeVault::Integration::Gen4;
        auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen4::save(qasynth::gen4::Kind::Platinum));
        auto& raw = *bytes;
        const auto layout = G4::bagLayout(G4::Layout::Platinum);
        for (std::size_t base : {std::size_t{0}, qasynth::gen4::kPartition}) {
            qasynth::w16(raw, base + layout[4].offset, 17);
            qasynth::w16(raw, base + layout[4].offset + 2, 3);
            qasynth::w16(raw, base + layout[1].offset, 450);
            qasynth::w16(raw, base + layout[1].offset + 2, 1);
        }
        qasynth::gen4::forge(raw);
        keepAlive.push_back(bytes);
        auto parsed = G4::Gen4ReadOnlySave::parse(raw, G4::Layout::Platinum, "platinum_nds", &error);
        check(parsed.has_value(), "Gen IV fixture parses: " + error);
        if (parsed) {
            auto keptSave = std::make_shared<G4::Gen4ReadOnlySave>(*parsed);
            keepAlive.push_back(keptSave);
            auto* trainer = keep(PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*keptSave, "platinum_nds", error));
            check(trainer && trainer->stagedPokemon(), "Gen IV staged workspace: " + error);
            if (trainer && trainer->stagedPokemon()) {
                auto* screen = open(*trainer, "Pokemon Platinum", "platinum_nds", "DraStic");
                const auto stackOf = [trainer](std::size_t pocket, uint16_t id) -> uint16_t {
                    const auto p = G4::Gen4ReadOnlySave::parse(trainer->stagedPokemon()->stagedBytes(),
                                                               G4::Layout::Platinum, "platinum_nds");
                    const auto bag = p ? G4::decodeReadOnlyBag(*p) : std::nullopt;
                    if (bag) for (const auto& s : (*bag)[pocket]) if (s.itemId == id) return s.count;
                    return 0;
                };
                screen->selectedCategory = 4;   // Medicines
                flow("Platinum", *screen, [trainer] { return trainer->stagedPokemon()->hasChanges(); },
                     [&] { return stackOf(4, 17); }, 3, "Potion");
                check(stackOf(1, 450) == 1, "Platinum: the Bicycle is untouched");
                trainer->stagedPokemon()->discard();
                check(!trainer->stagedPokemon()->hasChanges(), "Platinum: Discard restores");
                scopeFlow("Platinum", *screen, [trainer] { return trainer->stagedPokemon()->hasChanges(); }, false, "");
            }
        }
    }
    // Gen V: Black with Potion x3 in Medicine.
    {
        namespace G5 = PokeVault::Integration::Gen5;
        auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen5Fixture());
        auto& sav = *bytes;
        const std::size_t at = G5::itemsBlockOffset(G5::SaveFamily::BlackWhite) +
                               G5::bagPocketLayout5(G5::SaveFamily::BlackWhite, 3).offset;
        qasynth::w16(sav, at, 17);
        qasynth::w16(sav, at + 2, 3);
        qasynth::gen5::stamp(std::span<uint8_t>(sav).subspan(0, qasynth::gen5::copySize(G5::SaveFamily::BlackWhite)),
                             qasynth::gen5::blocks(G5::SaveFamily::BlackWhite));
        keepAlive.push_back(bytes);
        auto parsed = G5::Gen5ReadOnlySave::parse(sav, "black_nds", &error, G5::SaveCopySelection::Primary);
        check(parsed.has_value(), "Gen V fixture parses: " + error);
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error));
            check(trainer != nullptr, "Gen V trainer: " + error);
            if (trainer) {
                auto* screen = open(*trainer, "Pokemon Black", "black_nds", "DraStic");
                const auto held = [trainer]() -> uint16_t {
                    const auto bag = trainer->stagedPokemon().bagPresentation();
                    if (bag) for (const auto& s : (*bag)[3]) if (s.itemId == 17) return s.count;
                    return 0;
                };
                // Gen V keeps its pocket cursor in Gen5BagScreen: R three times reaches Medicine.
                Driver d(*screen);
                d.frame();
                screen->detailViewActive = true;
                screen->selectedMode = UI::TrainerViewScreen::ViewMode::Items;
                d.frame();
                for (int i = 0; i < 3; ++i) d.press(HidNpadButton_R);
                check(trainer->bagScreen().pocket() == 3, "Black: Medicine pocket selected");
                flow("Black", *screen, [trainer] { return trainer->stagedPokemon().hasChanges(); }, held, 3, "Potion");
                check(!trainer->stagedPokemon().bagReviewChanges().empty(), "Black: Review rows exist for the Bag");
                trainer->stagedPokemon().discardAll();
                std::string why;
                check(trainer->refreshStagedPokemonPresentation(why) && !trainer->stagedPokemon().hasChanges() && held() == 3,
                      "Black: Discard restores");
                scopeFlow("Black", *screen, [trainer] { return trainer->stagedPokemon().hasChanges(); }, false, "");
            }
        }
    }

    if (failures) {
        std::cerr << failures << " of " << checks << " Bag Top Up host UI checks failed\n";
        return 1;
    }
    std::cout << "Top Up / Add Missing through the real Gen I / III / IV / V Bag screens (ZR, preview, B, A, Review; "
                 "Scope: This pocket / All Bag / PC Items only in Gen I-III; Japanese Gen I not offered) ("
              << checks << " checks): PASS\n";
    return 0;
}
