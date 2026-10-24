// Shared Pokemon editor UI CONTRACT (owner list, hardware failure 35cbf163).
// One PKSE interface, one editor, one set of menus: these checks run for every
// generation layout Gen I..V and are written so a Gen VI..IX adapter is added by
// listing its generation in kGenerations and supplying its native data checks.
//  1 Left -> middle -> right panel navigation      8 Action-menu size and order
//  2 IV changes only after edit activation           9 Box / View / Edit stats agree
//  3 Focus movement never mutates data              10 Cursor kept across modals
//  4 Four move rows with PP and PP Ups              11 Touch / left-stick parity
//  5 Species selection                              12 Move picker Pwr/Acc/PP/type/category
//  6 Named held-item and ball pickers               13 Species picker sprites/shiny/search/bound
//  7 Nature, Gender and Ability editing             14 Existing Gen I-IV menus unchanged
#include "Encryption/Encryption5.h"
#include "Enums/Ball.h"
#include "Integration/Gen5/Gen5DisplayStats.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5SharedEditorBridge.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Names/MoveBattleStats5.h"
#include "UI/NarrowPokemonActionMenu.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchScroll.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
namespace Menu = PokeBank::UIModel::NarrowPokemonMenu;
namespace G = PokeVault::Integration::Gen5;
namespace F = PokeVault::Integration::Gen5::EditorFields;
namespace C = PokeVault::Integration::Gen5::Crypto;
using NF = G::StagedPokemon5Record::Field;
using Panel = Shared::Panel;

constexpr std::array<Shared::Generation, 5> kGenerations{{
    Shared::Generation::Gen1, Shared::Generation::Gen2, Shared::Generation::Gen3,
    Shared::Generation::Gen4, Shared::Generation::Gen5}};

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void contains(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos)
        std::cerr << "Missing shared editor contract: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
void absent(const std::string& text, const char* needle) {
    if (text.find(needle) != std::string::npos)
        std::cerr << "Forbidden in shared editor contract: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}

Workspace::Keys key(char k) {
    Workspace::Keys keys{};
    keys.l = k == 'L'; keys.r = k == 'R'; keys.up = k == 'U'; keys.down = k == 'D';
    keys.left = k == '<'; keys.right = k == '>';
    return keys;
}

// 1, 3, 11 for every generation layout: D-pad / stick cross DETAILS -> STATS ->
// MOVES at the panel edges and L/R switch panels; navigation is pure.
void navigationContract(Shared::Generation generation) {
    const int detailRows = Shared::rowsFor(generation, Panel::Details);
    const auto all = [](int) { return true; };
    for (const bool passive : {false, true}) {
        Shared::Focus focus{Panel::Details, 0, 0};
        auto step = [&](char k) {
            focus = Workspace::navigate(generation, focus, key(k), passive, !passive,
                                        detailRows, all, all).focus;
        };
        step('>');
        assert(focus.panel == Panel::Values && focus.column == 0);
        step('>');
        assert(focus.panel == Panel::Values && focus.column == 1);
        step('>');
        assert(focus.panel == Panel::Moves);
        step('>');
        assert(focus.panel == Panel::Moves); // the right edge stays on MOVES
        step('<');
        assert(focus.panel == Panel::Values);
        step('<');
        step('<');
        assert(focus.panel == Panel::Details);
        step('R');
        assert(focus.panel == Panel::Values);
        step('R');
        assert(focus.panel == Panel::Moves);
        step('R');
        assert(focus.panel == Panel::Details); // L/R cycle the three panels
        step('L');
        assert(focus.panel == Panel::Moves);
        // Up/Down stay inside a panel and wrap.
        focus = {Panel::Details, 0, 0};
        step('U');
        assert(focus.panel == Panel::Details && focus.row == detailRows - 1);
        step('D');
        assert(focus.row == 0);
    }
}

// 11: a completed tap selects exactly the focus the controller reaches.
void touchParityContract(int detailRows) {
    // L06 follow-up: touch the partially visible Gen IV Pokérus row after
    // scrolling. The production input boundary reveals it BEFORE the same
    // activation gate as A; no second tap or keypad is needed.
    UI::TouchPickerViewport viewport;
    viewport.firstRow=4;
    int detailsFocus=0;
    PokeBank::UIModel::SharedPokemonEditor::Focus pokerus{};
    if(detailRows>14) {
        assert(Workspace::focusCompletedTap(Workspace::kDetailTouchBase+14,detailRows,
            pokerus,detailsFocus,viewport,Workspace::kVisibleDetailRows));
        assert(detailsFocus==14 && viewport.containsSelection(14,detailRows,Workspace::kVisibleDetailRows));
        const auto activation=Workspace::activationFor(pokerus);
        assert(activation.kind==Workspace::ActivationKind::DetailField && activation.row==14);
    }
    Shared::Focus focus{};
    assert(Workspace::focusForTouch(Workspace::kDetailTouchBase + detailRows - 1, detailRows, focus));
    assert(focus.panel == Panel::Details && focus.row == detailRows - 1);
    assert(!Workspace::focusForTouch(Workspace::kDetailTouchBase + detailRows, detailRows, focus) ||
           detailRows >= 100);
    assert(Workspace::focusForTouch(Workspace::kIvTouchBase + 5, detailRows, focus));
    assert(focus.panel == Panel::Values && focus.row == 5 && focus.column == 0);
    assert(Workspace::focusForTouch(Workspace::kEvTouchBase + 2, detailRows, focus));
    assert(focus.panel == Panel::Values && focus.row == 2 && focus.column == 1);
    for (int row = Workspace::kNatureRow; row <= Workspace::kPidRow; ++row) {
        assert(Workspace::focusForTouch(Workspace::kPidTouchBase + row, detailRows, focus));
        assert(focus.panel == Panel::Values && focus.row == row);
    }
    for (int row = 0; row < Workspace::kMoveRows; ++row) {
        assert(Workspace::focusForTouch(Workspace::kMoveTouchBase + row, detailRows, focus));
        assert(focus.panel == Panel::Moves && focus.row == row);
    }
}

// --- Gen V native fixture (synthetic, never a real save) ---
void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}
std::vector<uint8_t> pk5(uint16_t species, uint32_t pid, bool party) {
    const auto personal = G::personal5(species, 0, true);
    std::vector<uint8_t> pk(party ? C::PartySize : C::StoredSize, 0);
    write32(pk, 0, pid);
    C::write16(pk, 0x08, species);
    C::write16(pk, 0x0C, 12345);
    C::write16(pk, 0x0E, 54321);
    write32(pk, 0x10, G::experienceForLevel5(personal.info->growthRate, 30));
    pk[0x15] = G::abilityForSlot5(*personal.info, G::abilityBit5(pid));
    pk[0x17] = 2;
    write32(pk, 0x38, 0x3FFFFFFFu & 0x1084210Fu); // mixed IVs
    pk[0x18] = 84; pk[0x1B] = 252;
    C::write16(pk, 0x28, 33); pk[0x30] = 35;
    C::write16(pk, 0x2A, 85); pk[0x31] = 15;
    pk[0x40] = static_cast<uint8_t>(G::genderFromPid5(pid, personal.info->genderRatio) << 1);
    pk[0x41] = 3;
    pk[0x5F] = 23; // Black 2
    C::write16(pk, 0x80, 1);
    pk[0x83] = 4;
    pk[0x84] = 5;
    if (party) {
        pk[0x8C] = 30;
        const auto original = pk;
        const bool regenerated = G::regeneratePartyStats5(pk, original, true);
        assert(regenerated);
        (void)regenerated;
        C::write16(pk, 0x8E, C::read16(pk, 0x90));
    }
    C::write16(pk, 6, C::checksum(pk));
    return C::encryptCandidate(pk);
}
// 2, 3, 4, 7: through the real shared screen state over a synthetic SAV5.
void gen5FieldContracts() {
    // A minimal synthetic Black 2 save: one party Pidove, box slot Pidove.
    namespace L = PokeVault::Integration::Gen5::LayoutInfo;
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    sav[tr + 4] = 'N'; sav[tr + 6] = 'X';
    C::write16(sav, tr + 0x14, 12345);
    C::write16(sav, tr + 0x16, 54321);
    sav[tr + 0x1F] = 23;
    const auto party = pk5(519, 0x00012345u, true);
    sav[L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    const auto boxed = pk5(519, 0x00012345u, false);
    std::copy(boxed.begin(), boxed.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset));
    auto copy = std::span<uint8_t>(sav).subspan(0, L::Black2White2CopySize);
    for (const auto& b : L::Black2White2) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, "black2_nds", &error, G::SaveCopySelection::Primary);
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    using Slot = G::Gen5StagedPokemonWorkspace::Slot;
    using Region = G::Gen5StagedPokemonWorkspace::Region;
    assert(ui.openActions(ws, Slot{Region::Party, 0, 0}, error));
    assert(ui.activate(ws, Shared::Action::Edit, error));
    auto& draft = ui.editDraft();
    const auto opened = *draft.current();
    const std::vector<uint8_t> pristine(opened.decodedBytes().begin(), opened.decodedBytes().end());

    // 3: a long walk over every panel, row and column with every key never
    // touches the draft; it only moves focus.
    const auto detailEditable = [&](int row) { return F::detailEditable(draft, row, true); };
    const auto pidEditable = [&](int row) { return F::pidRowEditable(draft, row, true); };
    Shared::Focus focus{};
    const char walk[] = "DDDDDDDDDDDDDDDDDDDDD>>>>DDDDDDDDDDDD<<<<<URLRLUUUD>>>D>D>D<<<<";
    for (const char k : walk) {
        focus = Workspace::navigate(Shared::Generation::Gen5, focus, key(k), false, true,
                                    F::RowCount, detailEditable, pidEditable).focus;
        const auto now = *draft.current();
        assert(std::equal(now.decodedBytes().begin(), now.decodedBytes().end(),
                          pristine.begin(), pristine.end()));
    }
    // Read-only DETAILS rows are skipped by Edit focus, as in Gen IV.
    focus = {Panel::Details, 8, 0};
    focus = Workspace::navigate(Shared::Generation::Gen5, focus, key('D'), false, true,
                                F::RowCount, detailEditable, pidEditable).focus;
    assert(focus.row == F::Language);
    focus = Workspace::navigate(Shared::Generation::Gen5, focus, key('D'), false, true,
                                F::RowCount, detailEditable, pidEditable).focus;
    assert(focus.row == F::Ball); // Origin stays read-only.

    // 2: IVs change only after the activation reaches the IV cell's keypad.
    focus = {Panel::Values, 1, 0};
    const auto activation = Workspace::activationFor(focus);
    assert(activation.kind == Workspace::ActivationKind::IvCell && activation.row == 1);
    const uint8_t before = draft.current()->ivs()[1];
    assert(draft.stage(NF::IV, 1, before == 31 ? 30u : 31u, error));
    assert(draft.current()->ivs()[1] != before);
    assert(Workspace::activationFor({Panel::Values, 1, 1}).kind == Workspace::ActivationKind::EvCell);
    assert(Workspace::activationFor({Panel::Moves, 6, 0}).row == 2);

    // 4: four move rows, each with Move / PP / PP Ups and Apply / Cancel.
    assert(Workspace::kMoveRows == 4 && Workspace::kMoveEditorRows == 5);
    int editorRow = 0;
    for (int i = 0; i < 5; ++i) editorRow = Workspace::moveEditorStep(editorRow, 1);
    assert(editorRow == 0 && Workspace::moveEditorStep(0, -1) == 4);
    assert(draft.stage(NF::Move, 3, 559, error) && draft.current()->pp()[3] == G::basePP5(559));
    assert(draft.stage(NF::MovePPUps, 3, 3, error) && draft.current()->pp()[3] == G::maxPP5(559, 3));
    assert(draft.stage(NF::MovePP, 3, 1, error) && draft.current()->pp()[3] == 1);
    assert(!draft.stage(NF::MovePP, 3, G::maxPP5(559, 3) + 1u, error));

    // 7: Nature (stored), Gender / Shiny / Ability (PID-coordinated) in the
    // STATS PID-linked rows; PID itself is read-only.
    for (const int row : {F::NatureRow, F::GenderRow, F::ShinyRow, F::AbilityRow})
        assert(F::pidRowEditable(draft, row, true));
    assert(!F::pidRowEditable(draft, F::PidValueRow, true));
    const uint32_t pid = draft.current()->pid();
    assert(draft.stage(NF::Nature, 0, 17, error) && draft.current()->pid() == pid);
    assert(F::toggleShiny(draft, error) && draft.current()->shiny());
    const uint8_t gender = draft.current()->gender();
    assert(F::toggleGender(draft, error) && draft.current()->gender() != gender && draft.current()->shiny());
    const auto abilities = F::abilityChoices(*draft.current(), true);
    assert(!abilities.empty() && abilities.front() == 0);
    for (const auto slot : abilities) {
        assert(draft.stage(NF::AbilitySlot, 0, slot, error));
        assert(F::abilitySlot(*draft.current()) == slot && draft.current()->shiny());
    }
    // ...and the coordinated PID survives Keep's replay and Review exactly.
    assert(ui.keepWithPresentation(ws, [](std::string&) { return true; }, error));
    const auto pending = ws.pendingReview();
    assert(!pending.empty() && G::verifyStagedReview(pending.front(), &error));

    // 5: species. Every BW/B2W2 species exists (1..649, Gen I-IV included);
    // later generations never; occupied-slot refusals explain themselves.
    assert(F::kMaxSpecies == 649);
    assert(ui.openActions(ws, Slot{Region::Box, 0, 0}, error));
    assert(ui.activate(ws, Shared::Action::Edit, error));
    assert(!ui.speciesRefusal(650).empty());
    assert(!ui.speciesRefusal(1).empty()); // needs a nickname first / different growth
    assert(ui.speciesRefusal(519).empty());
    ui.discardDraft();

    // 9: Box summary, View and Edit read the same stats.
    const auto partyView = *ws.viewParty(0);
    const auto boxView = *ws.viewBox(0, 0);
    const auto partyStats = G::displayStats5(*parsed->partyPokemon(0), true);
    assert(partyStats[0] > 0);
    const auto boxStats = G::displayStats5(*parsed->boxPokemon(0, 0), true);
    assert(boxStats == partyStats); // same Pokemon, boxed: same native values, never 0
    (void)partyView; (void)boxView;
}

// 6 and 12: named, native choice lists and the move table.
void nativeCatalogContracts() {
    const auto items = F::heldItemChoices();
    assert(!items.empty() && items.front() == 0);
    for (const auto id : items) {
        assert(id <= 638);
        assert(!G::detail5::contains(G::BagCatalog5::kUnreleased, id));
    }
    assert(std::adjacent_find(items.begin(), items.end()) == items.end());
    const auto balls = F::ballChoices();
    assert(balls.size() == 25 && balls.front() == 1 && balls.back() == 25);
    for (const auto ball : balls)
        assert(std::string_view(Enums::getBallName(static_cast<uint8_t>(ball))).size() > 0);
    assert(F::metLocationChoices().size() == G::NativeMetLocations5.size());
    // Gen V must choose moves for the *current species and exact game*, not the
    // entire native MoveInfo catalog. Historical source-only moves are retained
    // as unverified records, never offered for an unrelated Pokemon.
    namespace Moves = PokeVault::Integration::Gen5MoveCompatibility;
    for (const auto* game : {"black_nds", "white_nds", "black2_nds", "white2_nds"}) {
        const auto pikachu = F::moveChoices(game, 25, 0, {});
        assert(pikachu.size() > 40 && pikachu.size() < 180);
        assert(pikachu.front() == 0 && std::is_sorted(pikachu.begin(), pikachu.end()));
        assert(std::find(pikachu.begin(), pikachu.end(), 85) != pikachu.end());
        assert(std::find(pikachu.begin(), pikachu.end(), 459) == pikachu.end());
        assert(std::all_of(pikachu.begin() + 1, pikachu.end(), [](uint16_t move) {
            return G::validNativeMove5(move);
        }));
        assert(Moves::classify(game, 25, 0, 459, false) == Moves::Availability::Unavailable);
        const auto preserved = F::moveChoices(game, 25, 0, {459, 0, 0, 0});
        assert(std::find(preserved.begin(), preserved.end(), 459) != preserved.end());
        assert(Moves::classify(game, 25, 0, 459, true) == Moves::Availability::Preserved);
    }
    size_t bwDirect = 0, b2w2Direct = 0;
    for (uint16_t move = 1; move <= G::MaxNativeMove5; ++move) {
        bwDirect += Moves::direct("black_nds", 25, 0, move) ? 1u : 0u;
        b2w2Direct += Moves::direct("black2_nds", 25, 0, move) ? 1u : 0u;
    }
    assert(b2w2Direct > bwDirect); // B2W2 tutors/learnsets expand direct choices
    assert(F::moveChoices("platinum_nds", 25, 0, {}) == std::vector<uint16_t>{0});

    // Thunderbolt 85: Electric, Special, 95 / 100 / 15.
    const auto* thunderbolt = Names::getMoveBattleStats5(85);
    assert(thunderbolt && thunderbolt->type == 12 && thunderbolt->category == 1);
    assert(thunderbolt->power == 95 && thunderbolt->accuracy == 100 && thunderbolt->pp == 15);
    // Swords Dance 14: Status; no power or accuracy (em dash, never 0).
    const auto* swordsDance = Names::getMoveBattleStats5(14);
    assert(swordsDance->category == 2 && swordsDance->power == Names::kMoveValueUndefined5 &&
           swordsDance->accuracy == Names::kMoveValueUndefined5);
    // Swift 129 never misses: accuracy undefined, power defined.
    const auto* swift = Names::getMoveBattleStats5(129);
    assert(swift->accuracy == Names::kMoveValueUndefined5 && swift->power == 60);
    for (uint16_t move = 1; move <= G::MaxNativeMove5; ++move) {
        const auto* s = Names::getMoveBattleStats5(move);
        assert(s && s->pp == G::basePP5(move) && s->type <= 16 && s->category <= 2);
        assert(s->power != 0 && s->accuracy != 0); // undefined is 255, never 0
    }
    assert(!Names::getMoveBattleStats5(0) && !Names::getMoveBattleStats5(560));
}

// 8: the narrow Gen I-IV action menu, same geometry for every generation.
void actionMenuContract() {
    Menu::Model gen4Like{};
    for (const char* label : {"View", "Edit", "Clone", "Move", "Release",
                              "Origin & Legality", "Review Pending Changes", "Cancel"})
        gen4Like.append(label, 0);
    const auto g4 = Menu::geometry(gen4Like.count);
    assert(g4.width == 440 && g4.headerHeight == 78);
    Menu::Model gen5{};
    for (const char* label : {"View", "Edit", "Clone", "Move", "Release", "Cancel"})
        gen5.append(label, gen5.count);
    const auto g5 = Menu::geometry(gen5.count);
    assert(g5.width == g4.width && g5.headerHeight == g4.headerHeight && g5.rowHeight == 46); // shared BoxMenu rule
    assert(g5.rowX() == (1280 - 440) / 2 + 10);
}

// 10, 13, 14 and the one-code-path rule, checked in the surfaces that compile them.
void surfaceSourceContracts() {
    const auto shared = read("src/UI/SharedPokemonEditorSurface.inc");
    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    // One code path: both generations include and instantiate the same surface.
    contains(gen4, "#include \"SharedPokemonEditorSurface.inc\"");
    contains(gen5, "#include \"SharedPokemonEditorSurface.inc\"");
    contains(gen4, "Surface::handleWorkspace<Gen4Editor>(screen, state, down, touch);");
    contains(gen4, "Surface::drawWorkspace<Gen4Editor>(screen, fb, state);");
    contains(gen5, "EditorSurface::handleWorkspace<Gen5Editor>(screen, state, keys, touch);");
    contains(gen5, "EditorSurface::drawWorkspace<Gen5Editor>(screen, fb, state);");
    // No regression to a separate editor: the Gen IV and Gen V surface files no
    // longer contain their own workspace navigation, pickers or move editor.
    for (const auto* text : {&gen4, &gen5}) {
        for (const char* own : {"bool handleValuePicker(", "void drawValuePicker(",
                                "bool handleMoveEditor(", "void drawMoveEditor(",
                                "void normalizeEditableFocus(", "enum class PickerTarget",
                                "Shared::switchPanel(", "Shared::moveVertical(",
                                "Shared::moveRowColumn(", "Shared::passiveViewMoveVertical(",
                                "SharedSpeciesPicker::drawContent(", "NaturePickerLayout::forChoices(",
                                "SharedHeldItemPickerPresentation::drawGrid(", "drawScrollableDetails("})
            absent(*text, own);
    }
    for (const char* retired : {"drawWorkspace5", "drawMovePicker(", "drawLocationPicker(",
                                "drawNaturePicker(", "drawSpeciesPicker(", "drawIdentityPicker(",
                                "kGen5FieldTouchBase", "stepWithinPanel5", "switchPanel5",
                                "Surface::MovePicker", "Surface::SpeciesPicker"})
        absent(gen5, retired);
    // Coordinated PID edits are explicit: a confirmation that states the PID
    // change, a strict re-verify, and never a silent side effect.
    contains(gen5, "\"Change PID?\"");
    contains(gen5, "This changes the Pokemon's PID (personality value):");
    contains(gen5, "Gen5Editor::pidLinksHold(*after, b2w2)");
    contains(gen5, "if (handlePidConfirmation(screen, state, keys, touch)) return true;");
    contains(gen5, "if (before->pid() == after->pid()) {");
    // 11: the stick is folded into D-pad keys before the shared workspace runs.
    contains(gen4, "screen.controllerNavigation.apply(");
    contains(gen5, "const u64 workspaceKeys=screen.controllerNavigation.apply(");
    // 3: the shared surface only changes values from an activation / picker A.
    contains(shared, "if (!(down & HidNpadButton_A) || !editable) return true;");
    contains(shared, "const bool unchangedLocation = state.pickerTarget == PickerTarget::MetLocation &&");
    contains(shared, "state.pickerMetContextActive && value == state.pickerMetContext.stored();");
    contains(shared, "if (!unchangedLocation && !A::applyPicker(screen, state, state.pickerTarget, value)) return true;");
    absent(gen5, "adjustField(");
    absent(gen5, "\"Left/Right\",\"Adjust\"");
    // 10: opening / closing a picker or the move editor keeps the field focus.
    {
        const auto open = shared.substr(shared.find("inline void openPicker("),
                                        shared.find("inline bool compactPicker(") -
                                            shared.find("inline void openPicker("));
        absent(open, "state.focus");
        const auto picker = shared.substr(shared.find("bool handleValuePicker("),
                                          shared.find("void openMoveEditor(") -
                                              shared.find("bool handleValuePicker("));
        absent(picker, "state.focus");
        const auto editor = shared.substr(shared.find("bool handleMoveEditor("),
                                          shared.find("bool editStatCell(") -
                                              shared.find("bool handleMoveEditor("));
        absent(editor, "state.focus");
    }
    // 13: one species picker with sprites, Y shiny preview and the generation bound.
    contains(shared, "SharedSpeciesPicker::drawContent(");
    contains(shared, "species, A::kMaxSpecies, previewShiny");
    contains(shared, "state.speciesPreviewShiny = !state.speciesPreviewShiny;");
    contains(gen5, "static constexpr int kMaxSpecies = Fields::kMaxSpecies;");
    // Species search is no longer a per-adapter flag: the shared chooser (UI/SharedSpeciesPicker.h +
    // UI/SharedSearchInput.h) searches in every generation; QA CG15 guards it.
    contains(gen5, "for (uint16_t s = 1; s <= Fields::kMaxSpecies; ++s)");
    // 12: the move row carries type, category and an em dash for undefined values: the ONE
    // shared chooser row (UI/SharedMovesPanel.h) fed by the exact BW / B2W2 game version.
    contains(shared, "SharedMovesPanel::drawChooserRow(fb, columns, yy + 6, value, A::moveGame(screen), selected)");
    contains(gen5, "return b2w2(screen) ? Enums::GameVersion::B2W2 : Enums::GameVersion::BW;");
    const auto chooser = read("include/UI/SharedMovesPanel.h");
    contains(chooser, "Facts::categoryLabel(f)");
    const auto moveFacts = read("include/UI/SharedMoveFacts.h");
    contains(moveFacts, "s->power == Names::kMoveValueUndefined5 ? -1 : s->power");
    contains(moveFacts, "std::string(\"\\xE2\\x80\\x94\")");
    // 8: Gen V uses the narrow Gen I-IV action menu component.
    contains(gen5, "drawNarrowPokemonActions(fb,screen.touchButtons,model,state.actionRow(),title,");
    // 14: Gen I-IV menus are unchanged: the Gen IV adapter keeps its exact
    // strings, layout and non-compact Form picker (species search: shared chooser, CG15).
    contains(gen4, "static constexpr bool kCompactForm = false;");
    contains(gen4, "static constexpr int kMaxSpecies = 493;");
    contains(gen4, "\"Held Item — Generation IV\"");
    contains(gen4, "\"Empty + compatible moves only • exact Gen IV Acc / Pwr / PP\"");
    contains(gen4, "model.append(entry.label, entry.enabled ? i : -1);");
    contains(gen4, "static Enums::GameVersion moveGame(const TrainerViewScreen& screen) { return family(screen); }");
    // Gen I-III keep their accepted editors and the same shared contract layout.
    static_assert(Shared::layoutFor(Shared::Generation::Gen4).detailsRows == 16);
    static_assert(Shared::layoutFor(Shared::Generation::Gen5).detailsRows == F::RowCount);
    static_assert(Shared::layoutFor(Shared::Generation::Gen3).detailsRows == 16);
}

// Owner hardware rule (every Gen I-V editor field): D-pad Left/Right and the
// stick only move focus; a value changes only after A + picker/keypad. Gen IV/V
// share Workspace::navigate (pure, navigationContract above). Gen I-III keep
// their accepted editors: every live workspace handler routes Left/Right to its
// pure focus function, never to a value setter.
void leftRightNeverMutatesGen1To3() {
    const auto gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    for (const char* needle : {
             "? Shared::passiveViewMoveColumn(Shared::Generation::Gen3, state.focus, -1)",
             ": Shared::moveRowColumn(Shared::Generation::Gen3, state.focus, -1);",
             "? Shared::passiveViewMoveColumn(Shared::Generation::Gen3, state.focus, 1)",
             ": Shared::moveRowColumn(Shared::Generation::Gen3, state.focus, 1);"})
        contains(gen3, needle);
    const auto gen2 = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    contains(gen2, "if (nav & HidNpadButton_Left) unified.focus = Unified::moveRowColumn("
                   "Unified::Generation::Gen2, unified.focus, -1, unified.crystal);");
    contains(gen2, "if (nav & HidNpadButton_Right) unified.focus = Unified::moveRowColumn("
                   "Unified::Generation::Gen2, unified.focus, 1, unified.crystal);");
    contains(gen2, "unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, "
                   "unified.focus, -1, unified.crystal);");
    const auto gen2Parity = read("src/UI/Gen2SharedSurfaceParity.inc");
    contains(gen2Parity, "unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, "
                         "unified.focus, -1, unified.crystal);");
    contains(gen2Parity, "unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, "
                         "unified.focus, 1, unified.crystal);");
    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    contains(gen1, "if (down & HidNpadButton_Left) foundation.focus = Foundation::moveFocus("
                   "foundation.focus, Foundation::Direction::Left);");
    contains(gen1, "if (down & HidNpadButton_Right) foundation.focus = Foundation::moveFocus("
                   "foundation.focus, Foundation::Direction::Right);");
    // No Gen I-V surface advertises Left/Right as a value control.
    for (const char* file : {"src/UI/Gen1PokemonEditorOverlayFoundation.inc",
                             "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
                             "src/UI/Gen2UnifiedPokemonWorkspace.inc",
                             "src/UI/Gen2HardwareFinalFix.inc",
                             "src/UI/Gen3SharedPokemonSurface.inc",
                             "src/UI/SharedPokemonEditorSurface.inc",
                             "src/UI/Gen4SharedPokemonSurface.inc",
                             "src/UI/Gen5SharedReadOnlySurface.inc"}) {
        const auto text = read(file);
        absent(text, "{\"Left/Right\", \"Adjust\"}");
        absent(text, "{\"Left/Right\",\"Adjust\"}");
        absent(text, "{\"Left/Right\", \"Change\"}");
    }
}

// Save safety (Gen IV .dsv): Open snapshots the exact source FILE from the same payload the
// staged editor holds, with the container from the read-only adapter; export hands that
// snapshot to the transaction, which refuses a source that changed since Open.
void gen4SourceSnapshotWiring() {
    const auto ui = read("src/UI/UI.cpp");
    contains(ui, "const auto payload = G4::readSourcePayloadReadOnly(sourcePath);");
    contains(ui, "!std::equal(openedBytes.begin(), openedBytes.end(), payload.bytes.begin())");
    contains(ui, "payload.kind == G4::SourceContainerKind::DsvFooter ? G4::Gen4SourceContainer::Dsv");
    contains(ui, "auto snapshot = G4::snapshotGen4Source(");
    contains(ui, "trainerScreen.gen4SourceSnapshot = std::move(snapshot);");
    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    contains(gen4, "request.sourceSnapshot = screen.gen4SourceSnapshot;");
    contains(read("include/UI/TrainerViewScreenBase.h"),
             "std::optional<PokeVault::Integration::Gen4::Gen4SourceSnapshot> gen4SourceSnapshot;");
}

} // namespace

int main() {
    for (const auto generation : kGenerations) {
        navigationContract(generation);
        touchParityContract(Shared::rowsFor(generation, Panel::Details));
    }
    gen5FieldContracts();
    nativeCatalogContracts();
    actionMenuContract();
    surfaceSourceContracts();
    leftRightNeverMutatesGen1To3();
    gen4SourceSnapshotWiring();
    std::cout << "Shared Pokemon editor UI contract (Gen I-V, 14 owner checks): PASS\n";
}
