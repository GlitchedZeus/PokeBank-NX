// Round 11 H31 / H23 / H32 (owner worksheet): Clone and Move through the REAL TrainerViewScreen::update
// / draw (tests/host-ui harness) for Gen I, II, III, IV and V.
//
// Per generation, with real controller presses:
//   Clone (H31): Actions -> Clone closes the sheet and returns to the box carrying a ghost copy (footer
//     "Place Clone", nothing staged, no number keypad). A on an occupied slot is refused with a clear
//     status, nothing is overwritten and the carry stays; Y is refused with the A / B hint; B cancels
//     with nothing staged and no Actions menu. A second Clone placed with A on an empty slot stages
//     exactly that copy (Gen I / II: the chosen box's next free slot), the source is unchanged, the
//     carry ends and no Actions menu reopens.
//   Move (H23): Y picks up (Gen V: Actions -> Move), Y again is refused with the hint, B cancels back
//     to the exact staged state, and A places on an empty slot.
//   Occupied destinations (H32): Clone and Move onto an occupied slot never overwrite it.
// Round 12 (owner hardware results for 066e1db9):
//   R12-001 (H03 / H16): Gen V Boxes use the Gen I-IV keys - a Y tap picks up, a Y hold multi-selects,
//     + opens Review (Party keeps Y), and the footer says so.
//   R12-002 (H03 / H04): pickup, place, multi-move, a placed clone and cancel post no toast; the
//     capacity refusal, "There is no Pokemon in this slot to move" and occupied refusals still do.
//   R12-003 (H04): over a packed Gen I / II box the cursor IS the landing slot (no second indicator);
//     L/R changes box, the clone lands in the viewed box, a full box is refused, B restores.
// Every save is synthetic, built in code (QA builders, gsc fixture, HostUiFixtures): no real save, ROM
// or Pokemon file is read.
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5PokemonFile.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "PokemonFile/NativePokemonFile.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/ClassicCarryRules.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace G3 = PokeVault::Integration::Gen3;
namespace G5 = PokeVault::Integration::Gen5;
namespace Carry = PokeBank::UIModel::ClassicCarry;
using View = UI::TrainerViewScreen::ViewMode;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

struct Driver {
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0, u64 held = 0) {
        PadState pad{down, held | down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void press(u64 button) {
        frame(button);
        frame();
    }
};

bool drawn(const std::string& text) {
    return std::any_of(HostUi::texts.begin(), HostUi::texts.end(), [&](const std::string& t) { return t == text; });
}
// The narrow Pokemon Actions sheet draws its row labels at x = 448 (NarrowPokemonActionMenu).
std::vector<std::string> sheetRows() {
    std::vector<std::string> rows;
    for (const auto& t : HostUi::visibleText)
        if (t.x == 448 && t.style == 1) rows.push_back(t.text);
    return rows;
}
std::string joined(const std::vector<std::string>& rows) {
    std::string out;
    for (const auto& r : rows) out += (out.empty() ? "" : ", ") + r;
    return out;
}
bool chooseAction(Driver& d, const std::string& label) {
    const auto rows = sheetRows();
    const auto it = std::find(rows.begin(), rows.end(), label);
    if (it == rows.end()) return false;
    for (auto i = it - rows.begin(); i > 0; --i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    return true;
}

struct Cell {
    int box = 0, slot = 0;
};
std::string where(const Cell& c) { return "Box " + std::to_string(c.box + 1) + " / Slot " + std::to_string(c.slot + 1); }

struct Case {
    int generation = 0;
    std::string game;
    UI::TrainerViewScreen* screen = nullptr;
    View sourceView = View::Boxes;   // Gen V also clones from the Party
    Cell source;                     // occupied
    Cell occupied;                   // another occupied slot
    Cell emptyCell;                  // the empty cell the clone is dropped on
    Cell landing;                    // where it must land (Gen I / II: the box's next free slot)
    Cell moveFrom, moveTo, moveLanding;
    Cell packedEnd;                  // Gen I / II: the source box's end (where the clone cursor rests)
    std::function<std::vector<uint8_t>()> snapshot;   // staged bytes + pending change count
    std::function<std::optional<uint16_t>(Cell)> speciesAt;
    std::function<std::optional<uint16_t>()> sourceSpecies;
};

void openSheet(Driver& d, View mode, const Cell& c) {
    d.screen.goBack = false;
    d.screen.helpOverlayActive = false;
    d.screen.detailViewActive = true;
    d.screen.selectedMode = mode;
    if (mode == View::Party) d.screen.selectedPartyIndex = c.slot;
    else {
        d.screen.selectedBoxIndex = c.box;
        d.screen.selectedItemIndex = c.slot;
    }
    d.frame();
    d.frame();
    d.press(HidNpadButton_A);
}
// Moves the box cursor (the carry keeps using the accepted box navigation path).
void hover(Driver& d, const Cell& c) {
    d.screen.selectedBoxIndex = c.box;
    d.screen.selectedItemIndex = c.slot;
    d.frame();
}
bool carryingClone() { return drawn("Place Clone"); }
// R12-002: no routine toast is showing (a refusal from earlier has expired or was retired).
bool quiet(const Driver& d) { return d.screen.storageStatusFrames == 0 || d.screen.storageStatus.empty(); }
bool at(const Driver& d, const Cell& c) {
    return d.screen.selectedBoxIndex == c.box && d.screen.selectedItemIndex == c.slot;
}
std::string cursor(const Driver& d) { return where({d.screen.selectedBoxIndex, d.screen.selectedItemIndex}); }
// A held Y: the press, `frames` more frames held (kMultiHoldFrames is 8), keys pressed while held.
void holdY(Driver& d, int frames, std::initializer_list<u64> whileHeld = {}) {
    d.frame(HidNpadButton_Y, HidNpadButton_Y);
    for (int i = 0; i < frames; ++i) d.frame(0, HidNpadButton_Y);
    for (const u64 key : whileHeld) {
        d.frame(key, HidNpadButton_Y);
        d.frame(0, HidNpadButton_Y);
    }
}

void runClone(Case& c) {
    const std::string g = "Gen " + std::to_string(c.generation) + " " + c.game + " Clone: ";
    Driver d{*c.screen, {}, {}};
    d.frame();
    const auto species = c.sourceSpecies();
    check(species.has_value(), g + "the source holds a Pokemon");
    const auto before = c.snapshot();
    const int prompts = HostUi::promptCount;

    // Actions -> Clone: back to the box with the ghost copy, nothing staged, no keypad, no menu.
    openSheet(d, c.sourceView, c.source);
    check(chooseAction(d, "Clone"), g + "the Actions sheet lists Clone (" + joined(sheetRows()) + ")");
    check(HostUi::promptCount == prompts, g + "no number keypad opens");
    check(sheetRows().empty(), g + "the Actions sheet closed (" + joined(sheetRows()) + ")");
    check(carryingClone() && drawn("Box") && drawn("Cancel"), g + "the carry footer reads A Place Clone / L/R Box / B Cancel");
    check(d.screen.selectedMode == View::Boxes, g + "the carry runs on the box grid");
    // R12-002 (owner H04: clone "has a pop up menu thing on the botom about cloned pop dont need it"):
    // the carry footer names the keys, so picking the clone up posts no toast.
    const bool packed = c.generation <= 2;
    check(quiet(d), g + "Actions -> Clone posts no toast (" + d.screen.storageStatus + ")");
    check(c.snapshot() == before, g + "picking the clone up stages nothing");

    const auto occupiedSpecies = c.speciesAt(c.occupied);
    if (packed) {
        // R12-003: a packed box has no gaps, so the cursor cannot rest on an occupied cell: it is the
        // landing slot (the box's end), where the ghost is drawn - one indicator.
        check(at(d, c.packedEnd), g + "the cursor starts on the box end " + where(c.packedEnd) + " (" + cursor(d) + ")");
        hover(d, c.occupied);
        check(at(d, c.packedEnd), g + "an occupied cell snaps back to the landing slot (" + cursor(d) + ")");
    } else {
        // A on an occupied slot: refused, nothing overwritten, the carry stays.
        hover(d, c.occupied);
        d.press(HidNpadButton_A);
        check(d.screen.storageStatus == Carry::dropStatus(Carry::Drop::Occupied, packed) && !quiet(d),
              g + "A on an occupied slot is refused (" + d.screen.storageStatus + ")");
    }
    check(c.snapshot() == before && c.speciesAt(c.occupied) == occupiedSpecies,
          g + "the occupied slot is not overwritten and nothing is staged");
    check(carryingClone() && sheetRows().empty(), g + "the carry stays");
    // Y is consumed and posts nothing new (R12-002: no hint toast).
    const std::string shown = d.screen.storageStatus;
    d.press(HidNpadButton_Y);
    check(d.screen.storageStatus == shown, g + "Y posts no hint (" + d.screen.storageStatus + ")");
    check(c.snapshot() == before && carryingClone(), g + "Y changes nothing");
    // B cancels: nothing staged, no Actions menu, no toast, the cursor back on the source.
    d.press(HidNpadButton_B);
    check(d.screen.storageStatus.empty() && quiet(d), g + "B cancels without a toast (" + d.screen.storageStatus + ")");
    check(c.snapshot() == before && !carryingClone() && sheetRows().empty(),
          g + "after B: nothing staged, no carry, no Actions menu");
    if (c.sourceView == View::Boxes) check(at(d, c.source), g + "B restores the cursor to the source");

    // Clone again and place it with A on an empty slot.
    openSheet(d, c.sourceView, c.source);
    check(chooseAction(d, "Clone"), g + "Clone again");
    hover(d, c.emptyCell);
    d.press(HidNpadButton_A);
    check(d.screen.storageStatus.empty() && quiet(d), g + "A places the clone without a toast (" +
                                                          d.screen.storageStatus + ")");
    check(c.speciesAt(c.landing) == species, g + "the copy is at " + where(c.landing));
    check(c.sourceSpecies() == species, g + "the source is unchanged");
    check(c.snapshot() != before, g + "the clone is one staged change");
    check(!carryingClone() && sheetRows().empty(), g + "no carry and no Actions menu after the clone");
    check(HostUi::promptCount == prompts, g + "still no keypad");
    d.frame();
    check(sheetRows().empty(), g + "the Actions menu stays closed on the next frame");
    std::cout << g << "checked\n";
}

void runMove(Case& c) {
    const std::string g = "Gen " + std::to_string(c.generation) + " " + c.game + " Move: ";
    Driver d{*c.screen, {}, {}};
    d.frame();
    const auto species = c.speciesAt(c.moveFrom);
    check(species.has_value(), g + "the Pokemon to move exists");
    const auto before = c.snapshot();

    // R12-001: a Y tap picks up in every generation, Gen V included (it was Review there).
    const auto pickUp = [&] {
        d.screen.selectedMode = View::Boxes;
        d.screen.detailViewActive = true;
        hover(d, c.moveFrom);
        d.press(HidNpadButton_Y);   // a Y tap: picked up on release
    };
    pickUp();
    check(drawn("Place") && drawn("Cancel"), g + "the carry footer reads A Place / L/R Box / B Cancel");
    // R12-002 (owner H03: "when a pokemon is picked up we have a little boc pop up telling the controls
    // we dont need that we have the footer for that").
    check(quiet(d), g + "the pickup posts no toast (" + d.screen.storageStatus + ")");
    const auto carried = c.snapshot();
    d.press(HidNpadButton_Y);
    check(quiet(d), g + "Y posts no hint (" + d.screen.storageStatus + ")");
    check(c.snapshot() == carried && drawn("Place"), g + "Y changes nothing; the carry stays");
    d.press(HidNpadButton_B);
    check(c.snapshot() == before && !drawn("Place"), g + "B cancels back to the exact staged state");
    check(quiet(d) && at(d, c.moveFrom), g + "B cancels without a toast, cursor on the source");

    pickUp();
    if (c.generation == 3 || c.generation == 5) {
        // Sparse boxes refuse an occupied target and keep the carry (Gen IV swaps losslessly; Gen I / II insert).
        hover(d, c.source);
        const auto occupiedSpecies = c.speciesAt(c.source);
        const auto carrying = c.snapshot();
        d.press(HidNpadButton_A);
        check(drawn("Place") && c.speciesAt(c.source) == occupiedSpecies && c.snapshot() == carrying,
              g + "A on an occupied slot keeps the carry and the occupant (" + d.screen.storageStatus + ")");
        check(!quiet(d), g + "the occupied refusal is still shown (R12-002 keeps refusals)");
    }
    hover(d, c.moveTo);
    // R12-003: over a packed box the carried sprite is where it lands (an insertion point 0..count).
    if (c.generation <= 2) check(at(d, c.moveLanding), g + "the packed cursor is the landing slot (" + cursor(d) + ")");
    d.press(HidNpadButton_A);
    check(!drawn("Place"), g + "A placed the Pokemon (" + d.screen.storageStatus + ")");
    check(d.screen.storageStatus.empty() && quiet(d), g + "placing posts no toast (" + d.screen.storageStatus + ")");
    check(c.speciesAt(c.moveLanding) == species, g + "it is at " + where(c.moveLanding));
    check(sheetRows().empty(), g + "no Actions menu after the move");
    std::cout << g << "checked\n";
}

// ---- R12-003 (owner H04): one focus over a packed Gen I / II box; R12-002 refusals that remain ----
// `fullBox` holds `capacity` Pokemon (filled by the caller); `roomError` is the backend's group refusal.
void runPackedFocus(Case& c, int fullBox, int capacity, const std::string& roomError) {
    const std::string g = "Gen " + std::to_string(c.generation) + " " + c.game + " packed focus: ";
    Driver d{*c.screen, {}, {}};
    d.frame();
    const auto count = [&](int box) {
        int n = 0;
        while (n < capacity && c.speciesAt({box, n})) ++n;
        return n;
    };
    check(count(fullBox) == capacity, g + "Box " + std::to_string(fullBox + 1) + " is full");
    const auto species = c.sourceSpecies();
    const auto before = c.snapshot();
    const int box0 = c.source.box;
    const int end0 = count(box0);

    // Clone: the cursor is the end of the viewed box; the D-pad never moves it off that slot or box.
    openSheet(d, View::Boxes, c.source);
    check(chooseAction(d, "Clone") && carryingClone(), g + "Actions -> Clone carries the copy");
    check(at(d, {box0, end0}), g + "the cursor starts on the box end (" + cursor(d) + ")");
    for (const u64 key : {HidNpadButton_Right, HidNpadButton_Right, HidNpadButton_Right, HidNpadButton_Right,
                          HidNpadButton_Right, HidNpadButton_Right, HidNpadButton_Down, HidNpadButton_Down,
                          HidNpadButton_Left, HidNpadButton_Up}) {
        d.press(key);
        check(at(d, {box0, end0}), g + "the D-pad keeps the one cursor on the landing slot, same box (" + cursor(d) + ")");
    }
    // L / R change the box; the cursor lands on that box's end.
    d.press(HidNpadButton_R);
    check(at(d, {box0 + 1, count(box0 + 1)}), g + "R shows the next box's end (" + cursor(d) + ")");
    d.press(HidNpadButton_L);
    check(at(d, {box0, end0}), g + "L returns to the source box's end (" + cursor(d) + ")");
    // A full box: the cursor rests on its last cell and A is refused with the capacity message.
    for (int b = box0; b < fullBox; ++b) d.press(HidNpadButton_R);
    check(at(d, {fullBox, capacity - 1}), g + "the full box's cursor is its last cell (" + cursor(d) + ")");
    d.press(HidNpadButton_A);
    check(d.screen.storageStatus == Carry::dropStatus(Carry::Drop::BoxFull, true) && !quiet(d),
          g + "a full box is refused (" + d.screen.storageStatus + ")");
    check(c.snapshot() == before && carryingClone(), g + "the refusal stages nothing; the carry stays");
    // B cancels: everything restored, cursor back on the source, no toast.
    d.press(HidNpadButton_B);
    check(c.snapshot() == before && !carryingClone() && at(d, c.source) && d.screen.storageStatus.empty(),
          g + "B restores everything (" + cursor(d) + ", " + d.screen.storageStatus + ")");

    // The copy lands in the box being viewed, never the box after it.
    const int next = box0 + 1, after = box0 + 2;
    const int nextEnd = count(next), afterCount = count(after);
    openSheet(d, View::Boxes, c.source);
    check(chooseAction(d, "Clone"), g + "Clone again");
    d.press(HidNpadButton_R);
    d.press(HidNpadButton_A);
    check(c.speciesAt({next, nextEnd}) == species && count(after) == afterCount && at(d, {next, nextEnd}),
          g + "the copy is at " + where({next, nextEnd}) + ", the box being viewed");
    check(!carryingClone() && quiet(d), g + "placed without a toast (" + d.screen.storageStatus + ")");

    // Move: the cursor stays on the insertion points 0..count of the viewed box.
    const auto beforeMove = c.snapshot();
    hover(d, {next, 0});
    d.press(HidNpadButton_Y);
    check(drawn("Place"), g + "a Y tap picks up");
    for (int i = 0; i < 8; ++i) {
        d.press(HidNpadButton_Right);
        check(d.screen.selectedBoxIndex == next && d.screen.selectedItemIndex <= count(next),
              g + "a packed Move's cursor stays on an insertion point of this box (" + cursor(d) + ")");
    }
    d.press(HidNpadButton_B);
    check(c.snapshot() == beforeMove && quiet(d), g + "B cancels the Move silently");

    // R12-002 (owner H03: "only if the allowed space doesnt allow the amount of pokemon ... that when we
    // should havve the pop up window"): a group that does not fit is refused with the capacity message.
    hover(d, {next, 0});
    holdY(d, 9, {HidNpadButton_Right});
    d.frame();   // release Y: the group is picked up
    check(drawn("Place Group") && quiet(d), g + "a Y hold picks up the group silently (" + d.screen.storageStatus + ")");
    for (int b = next; b < fullBox; ++b) d.press(HidNpadButton_R);
    d.press(HidNpadButton_A);
    check(d.screen.storageStatus == roomError && drawn("Place Group"),
          g + "no room for the group: refused with the capacity message (" + d.screen.storageStatus + ")");
    d.press(HidNpadButton_B);
    check(c.snapshot() == beforeMove, g + "B restores the group");

    // R12-002: the one pickup message the owner keeps.
    int empty = 0;
    while (empty == fullBox || count(empty) != 0) ++empty;
    hover(d, {empty, 0});
    d.press(HidNpadButton_Y);
    check(d.screen.storageStatus == "There is no Pokemon in this slot to move" && !quiet(d) && !drawn("Place"),
          g + "Y on an empty slot says so (" + d.screen.storageStatus + ")");
    std::cout << g << "checked\n";
}

// ---- Gen I / II / IV: synthetic saves through the real strict parsers (openBackupTrainer). ----
struct Opened {
    std::unique_ptr<Trainer::Trainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Opened> keepAlive;   // surfaces key per-screen state by address: never reuse one

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

Opened* openClassic(const qasynth::Bytes& bytes, const std::string& game, PokeVault::Safety::SourceKind kind,
                    const std::string& provider) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    char pattern[] = "/tmp/pokebank-clone-carry-save-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    check(dir != nullptr, game + ": temp dir");
    if (!dir) return nullptr;
    const std::string path = std::string(dir) + "/" + game + ".sav";
    writeFile(path, bytes);
    std::string error;
    S::File file;
    check(S::read(path, dir, {}, file, error), game + ": read synthetic save: " + error);
    auto trainer = PokeVault::Legacy::openBackupTrainer(file, game, error);
    check(trainer != nullptr, game + ": strict parse + trainer: " + error);
    fs::remove_all(dir);
    if (!trainer) return nullptr;
    auto& opened = keepAlive.emplace_back();
    opened.trainer = std::move(trainer);
    opened.screen = std::make_unique<UI::TrainerViewScreen>(*opened.trainer, "Pokemon " + game, path, 0,
                                                           AccountUid{}, kind, game, provider);
    return &opened;
}

template <class Editor>
std::vector<uint8_t> classicSnapshot(const Editor* e) {
    const auto bytes = e->stagedBytes();
    std::vector<uint8_t> out(bytes.begin(), bytes.end());
    out.push_back(static_cast<uint8_t>(e->pendingChanges().size()));
    return out;
}

void gen1() {
    using Kind = PokeVault::Safety::SourceKind;
    auto* opened = openClassic(qasynth::gen1::save(), "red_gb", Kind::RetroArchLegacy, "RetroArch");
    if (!opened) return;
    auto* rby = dynamic_cast<PokeVault::Legacy::RBYReadOnlyTrainer*>(opened->trainer.get());
    check(rby && rby->stagedPokemon(), "Gen 1: staged editor");
    if (!rby || !rby->stagedPokemon()) return;
    auto* e = rby->stagedPokemon();
    std::string error;
    PokeVault::Integration::Gen1::BoxPokemonCreate bulbasaur, charmander;
    bulbasaur.species = 1;
    charmander.species = 4;
    check(e->stageAdd(0, 0, bulbasaur, error) && e->stageAdd(0, 1, charmander, error) &&
              rby->refreshBoxesFromStagedPokemon(error), "Gen 1: two Pokemon in Box 1: " + error);
    Case c;
    c.generation = 1;
    c.game = "red_gb";
    c.screen = opened->screen.get();
    c.source = {0, 0};
    c.occupied = {0, 1};
    c.emptyCell = {1, 5};
    c.landing = {1, 0};                    // packed list: the box's next free slot
    c.packedEnd = {0, 2};
    c.moveFrom = {0, 1}; c.moveTo = {2, 7}; c.moveLanding = {2, 0};
    c.snapshot = [e] { return classicSnapshot(e); };
    c.speciesAt = [e](Cell at) -> std::optional<uint16_t> {
        std::string ignored;
        const auto p = e->boxedPokemon(static_cast<size_t>(at.box), static_cast<size_t>(at.slot), ignored);
        return p ? std::optional<uint16_t>(p->species) : std::nullopt;
    };
    c.sourceSpecies = [c] { return c.speciesAt(c.source); };
    runClone(c);
    runMove(c);

    // R12-003: Box 6 full (20 Pokemon), for the full-box and group-capacity refusals.
    bool filled = true;
    for (size_t slot = 0; slot < 20; ++slot) filled = filled && e->stageAdd(5, slot, bulbasaur, error);
    check(filled && rby->refreshBoxesFromStagedPokemon(error), "Gen 1: Box 6 full: " + error);
    runPackedFocus(c, 5, 20, "This Generation I box does not have enough room for the selected Pokemon");
}

void gen2() {
    using Kind = PokeVault::Safety::SourceKind;
    auto* opened = openClassic(fixture(GS), "gold_gbc", Kind::RetroArchLegacy, "RetroArch");
    if (!opened) return;
    auto* gsc = dynamic_cast<PokeVault::Legacy::GSCReadOnlyTrainer*>(opened->trainer.get());
    check(gsc && gsc->stagedEditor(), "Gen 2: staged editor");
    if (!gsc || !gsc->stagedEditor()) return;
    auto* e = gsc->stagedEditor();
    std::string error;
    PokeVault::Integration::Gen2::BoxPokemonCreate create;
    create.species = 152;
    create.level = 10;
    create.nickname = "LEAF";
    create.otName = "GOLD";
    std::size_t added = 0;
    check(e->stageAddBoxPokemon(0, create, added, error) && added == 1 &&
              gsc->refreshStagedBoxPresentation(error), "Gen 2: a second Pokemon in Box 1: " + error);
    Case c;
    c.generation = 2;
    c.game = "gold_gbc";
    c.screen = opened->screen.get();
    c.source = {0, 0};                     // the fixture's boxed Pikachu
    c.occupied = {0, 1};
    c.emptyCell = {2, 4};
    c.landing = {2, 0};
    c.packedEnd = {0, 2};
    c.moveFrom = {0, 1}; c.moveTo = {3, 6}; c.moveLanding = {3, 0};
    c.snapshot = [e] { return classicSnapshot(e); };
    c.speciesAt = [e](Cell at) -> std::optional<uint16_t> {
        std::string ignored;
        const auto p = e->boxedPokemon(static_cast<size_t>(at.box), static_cast<size_t>(at.slot), ignored);
        return p ? std::optional<uint16_t>(p->species) : std::nullopt;
    };
    c.sourceSpecies = [c] { return c.speciesAt(c.source); };
    runClone(c);
    runMove(c);

    // R12-003: Box 6 full, for the full-box and group-capacity refusals.
    const int capacity = static_cast<int>(e->metadata().boxCapacity);
    bool filled = true;
    for (int i = 0; i < capacity && filled; ++i) {
        std::string ignored;
        if (e->boxedPokemon(5, static_cast<size_t>(i), ignored)) continue;
        filled = e->stageAddBoxPokemon(5, create, added, error);
    }
    check(filled && gsc->refreshStagedBoxPresentation(error), "Gen 2: Box 6 full: " + error);
    runPackedFocus(c, 5, capacity, "This Generation II box does not have enough room for the selected Pokemon");
}

void gen4() {
    using Kind = PokeVault::Safety::SourceKind;
    auto* opened = openClassic(qasynth::gen4::save(qasynth::gen4::Kind::Platinum), "platinum_nds",
                               Kind::ExternalLegacy, "DraStic");
    if (!opened) return;
    auto* g4 = dynamic_cast<PokeVault::Legacy::Gen4ReadOnlyTrainer*>(opened->trainer.get());
    check(g4 && g4->stagedPokemon(), "Gen 4: staged editor");
    if (!g4 || !g4->stagedPokemon()) return;
    auto* e = g4->stagedPokemon();
    std::string error;
    for (const auto& [slot, species] : {std::pair<size_t, uint16_t>{0, 393}, {1, 387}}) {
        if (e->boxedPokemon(0, slot)) continue;
        auto draft = e->createBoxDraft(0, slot, species, &error);
        check(draft && e->stageCreateBoxPokemon(0, slot, *draft, &error), "Gen 4: Box 1 Pokemon: " + error);
    }
    check(g4->refreshStagedPokemonPresentation(error), "Gen 4: presentation: " + error);
    Case c;
    c.generation = 4;
    c.game = "platinum_nds";
    c.screen = opened->screen.get();
    c.source = {0, 0};
    c.occupied = {0, 1};
    c.emptyCell = {1, 4};
    c.landing = {1, 4};
    c.moveFrom = {0, 1}; c.moveTo = {2, 9}; c.moveLanding = {2, 9};
    c.snapshot = [e] {
        auto out = e->stagedBytes();
        return out;
    };
    c.speciesAt = [e](Cell at) -> std::optional<uint16_t> {
        const auto p = e->boxedPokemon(static_cast<size_t>(at.box), static_cast<size_t>(at.slot));
        return p ? std::optional<uint16_t>(p->species()) : std::nullopt;
    };
    c.sourceSpecies = [c] { return c.speciesAt(c.source); };
    runClone(c);
    runMove(c);

    // The Storage view's game-save pane: Clone from there stays in that pane (like a Move from it).
    Driver d{*c.screen, {}, {}};
    d.screen.selectedMode = View::Storage;
    d.screen.detailViewActive = true;
    d.screen.storageFocusPane = 0;
    d.screen.stSaveBox = 0;
    d.screen.stSaveSlot = 0;
    d.frame();
    d.frame();
    d.press(HidNpadButton_A);
    const auto before = c.snapshot();
    check(chooseAction(d, "Clone"), "Gen 4 Storage: the save pane's Actions list Clone (" + joined(sheetRows()) + ")");
    check(carryingClone() && d.screen.selectedMode == View::Storage && c.snapshot() == before,
          "Gen 4 Storage: the clone is carried in the save pane, nothing staged");
    d.screen.stSaveBox = c.landing.box;   // occupied by the clone placed above
    d.screen.stSaveSlot = c.landing.slot;
    d.frame();
    d.press(HidNpadButton_A);
    check(d.screen.storageStatus == Carry::dropStatus(Carry::Drop::Occupied) && c.snapshot() == before,
          "Gen 4 Storage: an occupied save-pane slot is refused (" + d.screen.storageStatus + ")");
    d.screen.stSaveBox = 3;
    d.screen.stSaveSlot = 11;
    d.frame();
    d.press(HidNpadButton_A);
    check(c.speciesAt({3, 11}) == c.speciesAt(c.source) && !carryingClone() && sheetRows().empty(),
          "Gen 4 Storage: A places the clone in Box 4 / Slot 12 (" + d.screen.storageStatus + ")");
}

// ---- Gen III (FireRed): the party Pikachu imported into Box 1 twice. ----
struct Gen3Opened {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::FRLGReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Gen3Opened> gen3Alive;

void gen3() {
    auto& out = gen3Alive.emplace_back();
    out.bytes = HostUiFixtures::gen3Fixture(0);
    auto parsed = G3::parse(out.bytes, G3::SourceGame::FireRedGBA);
    check(static_cast<bool>(parsed), "Gen III fixture parses: " + parsed.detail);
    if (!parsed) return;
    std::string error;
    out.trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
    check(out.trainer != nullptr && out.trainer->stagedPokemon(), "Gen III trainer: " + error);
    if (!out.trainer || !out.trainer->stagedPokemon()) return;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon FireRed", "/host/firered.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA");
    auto* e = out.trainer->stagedPokemon();
    const auto pk3 = e->exportPartyPokemonFile(0, error);
    check(pk3.has_value(), "Gen III: party export: " + error);
    if (!pk3) return;
    const auto decoded = PokeBank::PokemonFile::decode(*pk3, "pika.pk3", error);
    check(decoded && e->stageImportBoxPokemon(0, 0, *decoded, error) && e->stageImportBoxPokemon(0, 1, *decoded, error) &&
              out.trainer->refreshStagedPokemonPresentation(error), "Gen III: two Pokemon in Box 1: " + error);
    Case c;
    c.generation = 3;
    c.game = "firered_gba";
    c.screen = out.screen.get();
    c.source = {0, 0};
    c.occupied = {0, 1};
    c.emptyCell = {0, 7};
    c.landing = {0, 7};
    c.moveFrom = {0, 1}; c.moveTo = {1, 9}; c.moveLanding = {1, 9};
    c.snapshot = [e] { return classicSnapshot(e); };
    c.speciesAt = [e](Cell at) -> std::optional<uint16_t> {
        std::string ignored;
        const auto p = e->boxedPokemon(static_cast<size_t>(at.box), static_cast<size_t>(at.slot), ignored);
        return p ? std::optional<uint16_t>(p->species) : std::nullopt;
    };
    c.sourceSpecies = [c] { return c.speciesAt(c.source); };
    runClone(c);
    runMove(c);
}

// ---- Gen V (Black): a complete PK5 imported into Box 1 twice; the party Pikachu clones too. ----
std::vector<uint8_t> pk5() {
    namespace C = PokeVault::Integration::Gen5::Crypto;
    std::vector<uint8_t> b(C::StoredSize, 0);
    const auto put32 = [&](size_t o, uint32_t v) {
        C::write16(b, o, static_cast<uint16_t>(v));
        C::write16(b, o + 2, static_cast<uint16_t>(v >> 16));
    };
    put32(0, 0x2468ACE1u);
    C::write16(b, 0x08, 25);
    C::write16(b, 0x0C, 12345);
    C::write16(b, 0x0E, 54321);
    put32(0x10, 1000);
    b[0x14] = 70;
    b[0x15] = 9;
    b[0x17] = 2;
    b[0x18] = 4;
    C::write16(b, 0x28, 33);
    b[0x30] = 35;
    put32(0x38, 0x0012345u);
    b[0x41] = 3;
    const char16_t name[] = u"SPARK";
    for (size_t i = 0; i < 5; ++i) C::write16(b, 0x48 + i * 2, name[i]);
    C::write16(b, 0x48 + 10, 0xFFFF);
    b[0x5F] = 20;
    const char16_t ot[] = u"ASH";
    for (size_t i = 0; i < 3; ++i) C::write16(b, 0x68 + i * 2, ot[i]);
    C::write16(b, 0x68 + 6, 0xFFFF);
    b[0x7B] = 11; b[0x7C] = 3; b[0x7D] = 9;
    C::write16(b, 0x80, 4);
    b[0x83] = 4;
    b[0x84] = 5;
    C::write16(b, 0x06, C::checksum(b));
    return b;
}

struct Gen5Opened {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::Gen5ReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Gen5Opened> gen5Alive;

void gen5() {
    auto& out = gen5Alive.emplace_back();
    out.bytes = HostUiFixtures::gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(out.bytes, "black_nds", &error, G5::SaveCopySelection::Primary);
    check(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return;
    out.trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    check(out.trainer != nullptr, "Gen V trainer: " + error);
    if (!out.trainer) return;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon Black", "/host/black.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    auto* trainer = out.trainer.get();
    namespace PF = PokeBank::PokemonFile;
    const auto record = pk5();
    const auto decoded = PF::decode(record, "seed.pk5", error);
    check(decoded && G5::importPokemonFile5(trainer->stagedPokemon(), *decoded, 0, 0, &error) &&
              G5::importPokemonFile5(trainer->stagedPokemon(), *decoded, 0, 1, &error) &&
              trainer->refreshStagedPokemonPresentation(error), "Gen V: two Pokemon in Box 1: " + error);
    Case c;
    c.generation = 5;
    c.game = "black_nds";
    c.screen = out.screen.get();
    c.source = {0, 0};
    c.occupied = {0, 1};
    c.emptyCell = {0, 4};
    c.landing = {0, 4};
    c.moveFrom = {0, 1}; c.moveTo = {1, 9}; c.moveLanding = {1, 9};
    c.snapshot = [trainer] {
        const auto& ws = trainer->stagedPokemon();
        std::vector<uint8_t> snap{static_cast<uint8_t>(ws.changedRecordCount()),
                                  static_cast<uint8_t>(ws.pendingReview().size())};
        for (size_t box = 0; box < 4; ++box)
            for (size_t slot = 0; slot < 30; ++slot)
                if (const auto p = ws.viewBox(box, slot); p && p->valid())
                    snap.insert(snap.end(), p->decodedBytes().begin(), p->decodedBytes().end());
        return snap;
    };
    c.speciesAt = [trainer](Cell at) -> std::optional<uint16_t> {
        const auto p = trainer->stagedPokemon().viewBox(static_cast<size_t>(at.box), static_cast<size_t>(at.slot));
        return p && p->valid() && !p->empty() ? std::optional<uint16_t>(p->species()) : std::nullopt;
    };
    c.sourceSpecies = [c] { return c.speciesAt(c.source); };
    runClone(c);
    runMove(c);

    // Gen V clones from the Party too: the carry switches to the box grid and lands in a box slot.
    Case party = c;
    party.game = "black_nds Party";
    party.sourceView = View::Party;
    party.source = {0, 0};
    party.occupied = {0, 0};             // Box 1 / Slot 2 was moved away above
    party.emptyCell = {0, 5};
    party.landing = {0, 5};
    party.sourceSpecies = [trainer]() -> std::optional<uint16_t> {
        const auto p = trainer->stagedPokemon().viewParty(0);
        return p && p->valid() && !p->empty() ? std::optional<uint16_t>(p->species()) : std::nullopt;
    };
    runClone(party);

    // ---- R12-001 (owner H03: "Y is review in gen 5 and that is wrong that should be move, reivew should
    // be what ever button gens 1-4 use"; H16: "gen 5 still needs the footer and controls for review and
    // move changed"). Two fresh Pokemon in Box 3 for the Y-hold group.
    check(G5::importPokemonFile5(trainer->stagedPokemon(), *decoded, 2, 0, &error) &&
              G5::importPokemonFile5(trainer->stagedPokemon(), *decoded, 2, 1, &error) &&
              trainer->refreshStagedPokemonPresentation(error), "Gen V: two Pokemon in Box 3: " + error);
    using Surface = G5::Gen5SharedScreenState::Surface;
    const std::string g = "Gen 5 black_nds keys: ";
    Driver d{*c.screen, {}, {}};
    d.screen.selectedMode = View::Boxes;
    d.screen.detailViewActive = true;
    hover(d, {2, 0});
    check(drawn("Move / Hold Multi") && drawn("Review") && drawn("Actions") && drawn("Help"),
          g + "the Boxes footer is the Gen I-IV one: A Actions, Y Move / Hold Multi, + Review, - Help");
    // (The + badge is drawn as bars, not text; its key is ReviewEntry::footerHint(Boxes, 5), pinned in
    // test_classic_carry_rules / test_review_entry. Its behaviour is driven here.)
    // + opens Review from Boxes; B returns to the same box and slot.
    d.press(HidNpadButton_Plus);
    check(trainer->sharedScreen().surface() == Surface::Review, g + "+ opens Review from Boxes");
    d.press(HidNpadButton_B);
    check(trainer->sharedScreen().surface() == Surface::Browse && d.screen.selectedMode == View::Boxes && at(d, {2, 0}),
          g + "B returns from Review to the same box slot (" + cursor(d) + ")");
    // Party keeps its Y Review; + is not a second key there.
    d.screen.selectedMode = View::Party;
    d.screen.selectedPartyIndex = 0;
    d.frame();
    d.press(HidNpadButton_Plus);
    check(trainer->sharedScreen().surface() == Surface::Browse, g + "+ does not open Review on Party");
    d.press(HidNpadButton_Y);
    check(trainer->sharedScreen().surface() == Surface::Review, g + "Y still opens Review on Party");
    d.press(HidNpadButton_B);
    check(trainer->sharedScreen().surface() == Surface::Browse && d.screen.selectedMode == View::Party,
          g + "B returns to the Party");

    // A Y hold multi-selects; releasing Y picks the group up; A places it with its shape, silently.
    d.screen.selectedMode = View::Boxes;
    hover(d, {2, 0});
    const auto before = c.snapshot();
    holdY(d, 9, {HidNpadButton_Right});
    check(drawn("Hold / Release Pick Up") && drawn("Resize Selection"), g + "the multi-select footer is shown");
    check(trainer->sharedScreen().surface() == Surface::Browse, g + "a held Y never opens Review");
    d.frame();   // release Y
    check(drawn("Place Group") && quiet(d), g + "releasing Y picks up the group silently (" + d.screen.storageStatus + ")");
    check(c.snapshot() == before, g + "the pickup stages nothing");
    // An occupied destination and a shape running off the box are refused; the carry stays.
    hover(d, {0, 0});
    d.press(HidNpadButton_A);
    check(d.screen.storageStatus == "Multi requires empty destination slots" && drawn("Place Group") && c.snapshot() == before,
          g + "an occupied destination is refused (" + d.screen.storageStatus + ")");
    hover(d, {3, 5});
    d.press(HidNpadButton_A);
    check(d.screen.storageStatus == "Group does not fit at that exact destination" && drawn("Place Group") &&
              c.snapshot() == before,
          g + "a group running off the box is refused (" + d.screen.storageStatus + ")");
    // B restores; then the real drop.
    d.press(HidNpadButton_B);
    check(c.snapshot() == before && !drawn("Place Group") && at(d, {2, 0}) && d.screen.storageStatus.empty(),
          g + "B restores the group");
    holdY(d, 9, {HidNpadButton_Right});
    d.frame();
    hover(d, {3, 6});
    d.press(HidNpadButton_A);
    check(!drawn("Place Group") && d.screen.storageStatus.empty(), g + "A places the group silently (" +
                                                                       d.screen.storageStatus + ")");
    check(c.speciesAt({3, 6}) == 25 && c.speciesAt({3, 7}) == 25 && !c.speciesAt({2, 0}) && !c.speciesAt({2, 1}),
          g + "the group moved to Box 4 / Slots 7-8 with its shape");
    check(c.snapshot() != before, g + "the group move is staged");
    std::cout << g << "checked\n";
}

}  // namespace

int main() {
    char pattern[] = "/tmp/pokebank-clone-carry-XXXXXX";
    const char* made = ::mkdtemp(pattern);
    if (!made) {
        std::cerr << "mkdtemp failed\n";
        return EXIT_FAILURE;
    }
    const std::string root = made;
    const fs::path previous = fs::current_path();
    fs::current_path(root);
    fs::create_directory("sdmc:");

    gen1();
    gen2();
    gen3();
    gen4();
    gen5();

    fs::current_path(previous);
    fs::remove_all(root);
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Clone and Move carry through the real TrainerViewScreen, Gen I-V (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
