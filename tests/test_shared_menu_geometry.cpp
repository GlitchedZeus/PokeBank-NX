// SharedMenuGeometry.h + SharedFieldState.h: every Gen I-V menu kind and field state.
//   * each menu type, at every content size, stays on screen and never covers the footer;
//   * height is content-driven (min(rows, visible) * rowHeight + chrome);
//   * the focus rectangle of every visible cell IS its touch rectangle (hitCell(centre) == cell);
//   * real native names (species, Gen III / Gen IV-V items, Gen III-V met locations) fit their
//     label column, measured with the bundled font's real advances;
//   * the field-state contract (gold == locked, outline always visible, states distinct).

#include "Names/ItemNames.h"
#include "Names/LocationDisplayNames.h"
#include "Names/SpeciesNames.h"
#include "UI/SharedFieldState.h"
#include "UI/SharedMenuGeometry.h"

#include <algorithm>
#include <cstdlib>
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
        "LaunchConfirm", "SaveDiscard"};
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
        const auto l = G::layout(G::MenuType::ItemPicker, 600);
        const auto label = G::labelRect(l, G::cellRect(l, 0), G::textWidth("x999", G::Text::Body));
        int widest = 0;
        for (uint16_t id = 1; id <= 638; ++id) {
            const std::string name = Names::getItemName(id);
            if (!name.empty()) require(G::fitsWidth(name, l.rule.label, label.w), "Gen IV/V item fits: " + name);
            widest = std::max(widest, G::textWidth(name, l.rule.label));
        }
        require(label.w - widest <= 260, "item picker has no wide empty band");
        for (uint16_t id = 1; id <= 376; ++id) {
            const std::string name = Names::getItemNameG3(id);
            if (!name.empty()) require(G::fitsWidth(name, l.rule.label, label.w), "Gen III item fits: " + name);
        }
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
        require(label.w - widest <= 260, "location picker has no wide empty band");
        // PKHeX keeps BW2 id 3 as the game's text-variable escapes; id 0 is the dash sentinel.
        require(Names::getMetLocationDisplayName(22, 3) == "Unknown location", "no raw text-variable escapes");
        require(Names::getMetLocationDisplayName(20, 0) == "None" &&
                Names::getMetLocationDisplayName(12, 0) == "None", "no location reads None");
        require(Names::getMetLocationDisplayName(20, 4) == "Nuvema Town", "real Gen V names unchanged");
    }
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
    fieldStates();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "Shared menu geometry + field state (12 menu types, real Gen I-V names): PASS\n";
    return EXIT_SUCCESS;
}
