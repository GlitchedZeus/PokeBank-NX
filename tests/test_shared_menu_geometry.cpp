// SharedMenuGeometry.h + SharedFieldState.h: every Gen I-V menu kind and field state.
//   * each menu type, at every content size, stays on screen and never covers the footer;
//   * height is content-driven (min(rows, visible) * rowHeight + chrome);
//   * the focus rectangle of every visible cell IS its touch rectangle (hitCell(centre) == cell);
//   * real native names (species, Gen III / Gen IV-V items, Gen III-V met locations) fit their
//     label column, measured with the bundled font's real advances;
//   * the field-state contract (gold == locked, outline always visible, states distinct).

#include "Enums/Ball.h"
#include "Names/ItemNames.h"
#include "Names/LocationDisplayNames.h"
#include "Names/SpeciesNames.h"
#include "UI/SharedFieldState.h"
#include "UI/SharedMenuGeometry.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>

namespace {

namespace G = PokeBank::UIModel::SharedMenuGeometry;
namespace F = PokeBank::UIModel::SharedFieldState;

int failures = 0;
void require(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

const char* typeName(G::MenuType t) {
    constexpr const char* names[] = {"ItemPicker", "KeyItemWarning", "RemoveConfirm", "HeldItemPicker",
        "BallPicker", "SpeciesPicker", "MovePicker", "LocationPicker", "BoxMenu", "BackupMenu",
        "LaunchConfirm", "SaveDiscard", "ChoicePicker"};
    return names[static_cast<int>(t)];
}

void geometryEveryType() {
    for (int t = 0; t < static_cast<int>(G::MenuType::Count); ++t) {
        const auto type = static_cast<G::MenuType>(t);
        const std::string name = typeName(type);
        int previousH = 0;
        for (int n = 0; n <= 120; ++n) {
            const auto l = G::layout(type, n);
            require(G::wellFormed(l), name + " well-formed at " + std::to_string(n));
            require(!l.frame.overlaps(G::kFooterBand), name + " never covers the footer");
            require(l.frame.y >= G::kMargin && l.frame.bottom() <= G::kScreenH - G::kFooterH - G::kMargin,
                    name + " keeps its margins");
            require(l.frame.h >= previousH, name + " grows only with content");
            previousH = l.frame.h;
            if (l.rule.kind == G::Kind::Confirm) continue;
            const int rows = (n + l.rule.columns - 1) / l.rule.columns;
            const int expected = std::min(std::max(rows, 1), l.rule.maxVisibleRows);
            require(l.visibleRows == expected, name + " content-driven row count");
            require(l.frame.h == l.rule.headerH + l.visibleRows * l.rule.rowHeight + l.rule.padBottom,
                    name + " height = rows * rowHeight + chrome");
            for (int i = 0; i < l.visibleRows * l.rule.columns; ++i) {
                const auto cell = G::cellRect(l, i);
                require(cell.w > 0 && cell.h > 0 && cell.inside(l.list), name + " cell inside viewport");
                require(G::hitCell(l, cell.x + cell.w / 2, cell.y + cell.h / 2) == i &&
                        G::hitCell(l, cell.x, cell.y) == i &&
                        G::hitCell(l, cell.right() - 1, cell.bottom() - 1) == i,
                        name + " focus rect == touch rect");
                for (int j = 0; j < i; ++j)
                    require(!G::cellRect(l, j).overlaps(cell), name + " cells never overlap");
                const auto label = G::labelRect(l, cell);
                require(label.w > 0 && label.right() <= cell.right(), name + " label inside cell");
                if (l.rule.spriteSize > 0) {
                    const auto sprite = G::spriteRect(l, cell);
                    require(sprite.inside(cell), name + " sprite inside cell");
                    require(label.x - sprite.right() == l.rule.spriteGap, name + " fixed sprite-to-label gap");
                    require(l.rule.spriteGap <= 10, name + " no wide sprite gap");
                }
            }
            require(G::hitCell(l, l.frame.x - 1, l.frame.y - 1) == -1, name + " outside the frame hits nothing");
            const int last = std::max(0, n - 1);
            const int first = G::firstVisibleRow(l, last);
            require(last / l.rule.columns >= first && last / l.rule.columns < first + l.visibleRows,
                    name + " the selection stays visible");
        }
        const auto confirm = G::layout(type, 0);
        if (confirm.rule.kind == G::Kind::Confirm) {
            for (int count = 1; count <= 3; ++count)
                for (int i = 0; i < count; ++i) {
                    const auto b = G::buttonRect(confirm, i, count);
                    require(b.inside(confirm.frame) && b.h >= 56, name + " glyph buttons are full touch targets");
                    if (i > 0) require(!G::buttonRect(confirm, i - 1, count).overlaps(b), name + " buttons apart");
                }
            require(G::bodyLineTop(confirm, confirm.rule.bodyLines) <= confirm.buttons.y,
                    name + " message lines end above the buttons");
        }
    }
    require(G::layout(G::MenuType::BoxMenu, 3).frame.h < G::layout(G::MenuType::BoxMenu, 6).frame.h,
            "a short menu is drawn short");
    require(G::layout(G::MenuType::HeldItemPicker, 400).cellWidth == 249, "Held Item reference grid");
}

void textMetrics() {
    // Exact advances (Nunito default instance) at the 20 px Body size: "Pikachu" =
    // P616 i212 k468 a514 c455 h553 u545 = 3363 units -> 67.26 px -> 68.
    require(G::textWidth("Pikachu", G::Text::Body) == 68, "Nunito advances");
    require(G::textWidth("", G::Text::Body) == 0, "empty text");
    require(G::textWidth("\xC3\xA9", G::Text::Body) >= G::textWidth("e", G::Text::Body), "non-ASCII is never under-counted");
    require(G::fitsWidth("Pikachu", G::Text::Body, 68) && !G::fitsWidth("Pikachu", G::Text::Body, 67), "fitsWidth bound");
}

// Every real native label fits its column: no clipped species, item or location name.
void namesFit() {
    {
        const auto l = G::layout(G::MenuType::SpeciesPicker, 649);
        const auto label = G::labelRect(l, G::cellRect(l, 0), G::textWidth("#649", G::Text::Caption));
        for (uint16_t s = 1; s <= 649; ++s) {
            const std::string name = Names::getSpeciesName(s);
            require(G::fitsWidth(name, l.rule.label, label.w), "species fits: " + name);
        }
    }
    {
        // R2-BUG-006: the item grid. A name that does not fit a 4-column cell (beside the "x999"
        // held column) must fit the grid's 3-column cell: SharedItemGridPicker::columnsFor drops
        // the whole grid to 3 columns rather than clip it. Most names fit 4 columns.
        const auto l = G::layout(G::MenuType::ItemPicker, 600);
        const auto label4 = G::labelRect(l, G::cellRect(l, 0), G::textWidth("x999", G::Text::Caption));
        const int cell3 = l.list.w / 3 - (l.cellWidth - G::cellRect(l, 0).w);
        const int label3 = label4.w + (cell3 - G::cellRect(l, 0).w);
        int beyond4 = 0, total = 0;
        const auto check = [&](const std::string& name, const char* what) {
            if (name.empty()) return;
            ++total;
            if (G::fitsWidth(name, l.rule.label, label4.w)) return;
            ++beyond4;
            require(G::fitsWidth(name, l.rule.label, label3), std::string(what) + " fits the 3-column grid: " + name);
        };
        for (uint16_t id = 1; id <= 638; ++id) check(Names::getItemName(id), "Gen IV/V item");
        for (uint16_t id = 1; id <= 376; ++id) check(Names::getItemNameG3(id), "Gen III item");
        require(beyond4 * 20 <= total, "at most 5% of item names need the 3-column grid");
    }
    {
        const auto l = G::layout(G::MenuType::HeldItemPicker, 400);
        const auto label = G::labelRect(l, G::cellRect(l, 0));
        for (uint16_t id = 1; id <= 376; ++id) {
            const std::string name = Names::getItemNameG3(id);
            if (!name.empty()) require(G::fitsWidth(name, l.rule.label, label.w), "Held Item fits: " + name);
        }
    }
    {
        // Name left, method / level right: the name column leaves room for "Lv. 100  Surf".
        const auto l = G::layout(G::MenuType::LocationPicker, 200);
        const auto label = G::labelRect(l, G::cellRect(l, 0), G::textWidth("Rock Smash  Lv. 20-40", G::Text::Body));
        int widest = 0;
        for (uint8_t origin : {uint8_t{1}, uint8_t{3}, uint8_t{4}, uint8_t{10}, uint8_t{12}, uint8_t{7},
                               uint8_t{20}, uint8_t{22}})
            for (uint16_t id = 0; id < 700; ++id) {
                const std::string name = Names::getMetLocationDisplayName(origin, id);
                if (name != "Unknown location") {
                    require(G::fitsWidth(name, l.rule.label, label.w), "location fits: " + name);
                    widest = std::max(widest, G::textWidth(name, l.rule.label));
                }
            }
        // The 900 px location window is the accepted L03/L04 layout (method / level on the right).
        // PKHeX keeps BW2 id 3 as the game's text-variable escapes; id 0 is the dash sentinel.
        require(Names::getMetLocationDisplayName(22, 3) == "Unknown location", "no raw text-variable escapes");
        require(Names::getMetLocationDisplayName(20, 0) == "None" &&
                Names::getMetLocationDisplayName(12, 0) == "None", "no location reads None");
        require(Names::getMetLocationDisplayName(20, 4) == "Nuvema Town", "real Gen V names unchanged");
    }
}

// Phase 2: every editor value list (Ball, Language, Ability, Form, Gender, Pokerus, Met Location)
// keeps the accepted 42 px rows / 38 px cells / 86 px header; Ball sprites lead their names.
void editorValueLists() {
    for (auto type : {G::MenuType::BallPicker, G::MenuType::ChoicePicker, G::MenuType::LocationPicker})
        for (int n = 1; n <= 30; ++n) {
            const auto l = G::layout(type, n);
            const int visible = std::min(8, n);
            // NEW-10: Met Location drops the unused band under its last row (14 px, matching Move).
            const int pad = type == G::MenuType::LocationPicker ? 86 + 14 : 125;
            require(l.frame.h == pad + visible * 42, "value list keeps the accepted height");
            require(l.frame.bottom() - (l.frame.y + 86 + (visible - 1) * 42 + 38) >= 16, "last row clear of the frame edge");
            for (int i = 0; i < visible; ++i) {
                const auto c = G::cellRect(l, i);
                require(c.x == l.frame.x + 22 && c.w == l.frame.w - 44 && c.y == l.frame.y + 86 + i * 42 &&
                        c.h == 38, "value list cells keep the accepted rows");
            }
        }
    require(G::layout(G::MenuType::BallPicker, 12).frame.w == G::layout(G::MenuType::ChoicePicker, 12).frame.w,
            "Ball and the other value lists are the same size");
    const auto balls = G::layout(G::MenuType::BallPicker, 26);
    const auto cell = G::cellRect(balls, 0);
    const auto label = G::labelRect(balls, cell);
    require(label.x - G::spriteRect(balls, cell).right() == 8, "ball sprite leads its name at an 8 px gap");
    for (uint8_t id = 1; id <= 26; ++id) {
        const std::string name = Enums::getBallName(id);
        if (!name.empty()) require(G::fitsWidth(name, G::Text::Body, label.w), "ball name fits: " + name);
    }
}

// QA CG13: every MenuType has a real consumer; the Move chooser and the save / discard dialog
// read their geometry from the shared rules and keep their accepted pixels.
void consumers() {
    const auto move = G::layout(G::MenuType::MovePicker, 1000);
    require(move.frame.w == 760 && move.frame.h == 520 && move.visibleRows == 10 &&
            move.rule.headerH == 86 && move.rule.rowHeight == 42 &&
            move.rule.rowHeight - move.rule.rowGap == 38, "MovePicker rule = the accepted chooser");
    const auto save = G::rule(G::MenuType::SaveDiscard);
    G::Layout row;
    row.buttons = {100 + save.padX, 400, save.width - 2 * save.padX, 56};
    const auto b0 = G::buttonRect(row, 0, 2), b1 = G::buttonRect(row, 1, 2);
    require(save.width == 560 && b0.x == 124 && b0.w == 248 && b1.x == 100 + 560 - 24 - 248 && b1.w == 248,
            "SaveDiscard rule = the accepted 560 px dialog and its two buttons");
    std::ifstream movePresentation("include/UI/MovePickerPresentation.h"), saveDialog("src/UI/Dialogs/SaveConfirmDialog.cpp"),
        gen3("src/UI/Gen3SharedPokemonSurface.inc");
    const std::string mp{std::istreambuf_iterator<char>(movePresentation), {}};
    const std::string sd{std::istreambuf_iterator<char>(saveDialog), {}};
    const std::string g3{std::istreambuf_iterator<char>(gen3), {}};
    require(mp.find("Geo::layout(Geo::MenuType::MovePicker, 1000)") != std::string::npos,
            "Move chooser derives from MenuType::MovePicker");
    require(sd.find("Geo::rule(Geo::MenuType::SaveDiscard)") != std::string::npos &&
            sd.find("constexpr int w = kSaveDiscard.width;") != std::string::npos,
            "save / discard dialog lays out via MenuType::SaveDiscard");
    require(g3.find("Derived (read-only)") == std::string::npos, "CG16: no \"(read-only)\" PID text in Gen III");
    // BackupMenu records the accepted Save Backups screen exactly (2R routes Save Backups and the
    // emulator Backups list through the rule); its pixels are pinned by QA's touch contracts.
    std::ifstream backupScreen("src/UI/BackupSelectionScreen.cpp");
    const std::string bs{std::istreambuf_iterator<char>(backupScreen), {}};
    const auto backup = G::rule(G::MenuType::BackupMenu);
    const auto has = [&](const std::string& needle) { return bs.find(needle) != std::string::npos; };
    require(has("constexpr int CARD_W = " + std::to_string(backup.width) + ";") &&
            has("constexpr int LIST_ROW_H = " + std::to_string(backup.rowHeight) + ";") &&
            has("constexpr int LIST_MAX_VISIBLE = " + std::to_string(backup.maxVisibleRows) + ";") &&
            has("itemY + LIST_ROW_H - " + std::to_string(backup.rowGap)) &&
            has("CARD_X + " + std::to_string(backup.padX)) &&
            has("CARD_Y + " + std::to_string(backup.headerH)),
            "BackupMenu rule == BackupSelectionScreen's accepted constants");
}

void fieldStates() {
    using S = F::State;
    using T = F::Token;
    for (bool focused : {false, true}) {
        const auto e = F::style(S::Editable, focused);
        const auto r = F::style(S::ReadOnly, focused);
        const auto d = F::style(S::Disabled, focused);
        require(e.activates && !r.activates && !d.activates, "only Editable activates");
        require(r.value == T::Gold && e.value != T::Gold && d.value != T::Gold, "gold marks ReadOnly only");
        require(e.focusOutline == focused && r.focusOutline == focused && d.focusOutline == focused,
                "focus outline drawn in every state");
        require(r.value == T::Gold, "a focused locked value stays gold");
        require(e.value != r.value && r.value != d.value && e.value != d.value, "three distinct states");
    }
    using A = PokeBank::UIModel::SharedPokemonEditor::FieldAccess;
    require(F::fromAccess(A::Editable) == S::Editable && F::fromAccess(A::Derived) == S::ReadOnly &&
            F::fromAccess(A::ReadOnly) == S::ReadOnly && F::fromAccess(A::Hidden) == S::Disabled,
            "editor contract access -> field state");
    require(std::string(F::reason(S::Editable)).empty() && !std::string(F::reason(S::ReadOnly)).empty() &&
            !std::string(F::reason(S::Disabled)).empty(), "A on a locked field explains itself");
}

} // namespace

int main() {
    geometryEveryType();
    textMetrics();
    namesFit();
    editorValueLists();
    consumers();
    fieldStates();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "Shared menu geometry + field state (12 menu types, real Gen I-V names): PASS\n";
    return EXIT_SUCCESS;
}
