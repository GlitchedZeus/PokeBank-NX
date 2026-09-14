#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeBank::UIModel::PokemonEditorFoundation {

enum class Panel : uint8_t { Identity, Values, Moves };
enum class Direction : uint8_t { Up, Down, Left, Right };
enum class SurfaceOwner : uint8_t { FoundationWorkspace, Cleanup3 };

enum class IdentityField : uint8_t {
    Species,
    Nickname,
    LevelExperience,
    OriginalTrainer,
    TrainerId,
};

enum class ValueColumn : uint8_t { DV, StatExperience, CalculatedStat };

enum class ValueRow : uint8_t {
    HP,
    Attack,
    Defense,
    Speed,
    Special,
    Shiny,
    Level,
};

enum class MoveField : uint8_t { Move1, Move2, Move3, Move4 };

struct Focus {
    Panel panel = Panel::Identity;
    uint8_t row = 0;
    uint8_t column = 0;

    constexpr bool operator==(const Focus& other) const noexcept {
        return panel == other.panel && row == other.row && column == other.column;
    }
    constexpr bool operator!=(const Focus& other) const noexcept { return !(*this == other); }
};

constexpr uint8_t identityRowCount() noexcept { return 5; }
constexpr uint8_t valueRowCount() noexcept { return 7; }
constexpr uint8_t moveRowCount() noexcept { return 4; }
constexpr uint8_t valueStatRowCount() noexcept { return 5; }

constexpr SurfaceOwner surfaceOwnerFor(bool foundationMainMode, bool pickerActive) noexcept {
    return (!foundationMainMode || pickerActive) ? SurfaceOwner::Cleanup3 : SurfaceOwner::FoundationWorkspace;
}

constexpr Focus normalize(Focus focus) noexcept {
    switch (focus.panel) {
        case Panel::Identity:
            focus.row = static_cast<uint8_t>(focus.row % identityRowCount());
            focus.column = 0;
            break;
        case Panel::Values:
            focus.row = static_cast<uint8_t>(focus.row % valueRowCount());
            if (focus.row >= valueStatRowCount()) focus.column = 0;
            else focus.column = static_cast<uint8_t>(focus.column % 3);
            break;
        case Panel::Moves:
            focus.row = static_cast<uint8_t>(focus.row % moveRowCount());
            focus.column = 0;
            break;
    }
    return focus;
}

constexpr Focus moveFocus(Focus focus, Direction direction) noexcept {
    focus = normalize(focus);
    if (direction == Direction::Up || direction == Direction::Down) {
        const int delta = direction == Direction::Up ? -1 : 1;
        uint8_t rows = identityRowCount();
        if (focus.panel == Panel::Values) rows = valueRowCount();
        if (focus.panel == Panel::Moves) rows = moveRowCount();
        focus.row = static_cast<uint8_t>((static_cast<int>(focus.row) + delta + rows) % rows);
        if (focus.panel == Panel::Values && focus.row >= valueStatRowCount()) focus.column = 0;
        return normalize(focus);
    }

    if (direction == Direction::Right) {
        if (focus.panel == Panel::Identity) {
            return normalize({Panel::Values, static_cast<uint8_t>(focus.row < valueStatRowCount() ? focus.row : valueStatRowCount() - 1), 0});
        }
        if (focus.panel == Panel::Values) {
            if (focus.row < valueStatRowCount() && focus.column < 2) {
                ++focus.column;
                return focus;
            }
            return normalize({Panel::Moves, static_cast<uint8_t>(focus.row < moveRowCount() ? focus.row : moveRowCount() - 1), 0});
        }
        return focus;
    }

    if (focus.panel == Panel::Moves) {
        return normalize({Panel::Values, static_cast<uint8_t>(focus.row), 2});
    }
    if (focus.panel == Panel::Values) {
        if (focus.row < valueStatRowCount() && focus.column > 0) {
            --focus.column;
            return focus;
        }
        return normalize({Panel::Identity, static_cast<uint8_t>(focus.row < identityRowCount() ? focus.row : identityRowCount() - 1), 0});
    }
    return focus;
}

constexpr Focus nextPanel(Focus focus) noexcept {
    focus = normalize(focus);
    if (focus.panel == Panel::Identity)
        return normalize({Panel::Values, static_cast<uint8_t>(focus.row < valueRowCount() ? focus.row : valueRowCount() - 1), 0});
    if (focus.panel == Panel::Values)
        return normalize({Panel::Moves, static_cast<uint8_t>(focus.row < moveRowCount() ? focus.row : moveRowCount() - 1), 0});
    return normalize({Panel::Identity, static_cast<uint8_t>(focus.row < identityRowCount() ? focus.row : identityRowCount() - 1), 0});
}

constexpr bool panelIsInteractive(Panel) noexcept { return true; }
constexpr bool valueCellEditable(ValueRow row, ValueColumn column) noexcept {
    if (row == ValueRow::Shiny || row == ValueRow::Level) return column == ValueColumn::DV;
    if (column == ValueColumn::CalculatedStat) return false;
    if (row == ValueRow::HP && column == ValueColumn::DV) return false;
    return true;
}
constexpr bool calculatedStatsAreReadOnly() noexcept { return true; }
constexpr bool hpDVIsDerived() noexcept { return true; }
constexpr bool moveRowsAreIndividuallyFocusable() noexcept { return true; }
constexpr bool radarIsFocusable() noexcept { return false; }

constexpr uint8_t nextRandomDV(uint32_t& state) noexcept {
    state = state * 1664525u + 1013904223u;
    return static_cast<uint8_t>((state >> 28U) & 0x0FU);
}

constexpr std::array<uint8_t,4> randomDVs(uint32_t& state) noexcept {
    return {nextRandomDV(state), nextRandomDV(state), nextRandomDV(state), nextRandomDV(state)};
}

enum class Generation : uint8_t { Gen1 = 1, Gen2 = 2, Gen3 = 3, Gen4 = 4, Gen5 = 5, Gen6 = 6, Gen7 = 7, Gen8 = 8, Gen9 = 9 };

struct Capabilities {
    bool supportsAbility = false;
    bool supportsHeldItem = false;
    bool supportsNature = false;
    bool supportsFriendship = false;
    bool supportsEgg = false;
    bool supportsMetLevel = false;
    bool supportsRibbons = false;
    bool supportsMarks = false;
    bool usesDVs = false;
    bool usesIVs = false;
    bool usesStatExp = false;
    bool usesEVs = false;
    bool hasSplitSpecial = false;
    bool supportsGender = false;
    bool supportsShiny = true;
};

constexpr Capabilities capabilitiesForGeneration(Generation generation) noexcept {
    switch (generation) {
        case Generation::Gen1:
            return {/*ability*/false, /*held*/false, /*nature*/false, /*friendship*/false,
                    /*egg*/false, /*met*/false, /*ribbons*/false, /*marks*/false,
                    /*dvs*/true, /*ivs*/false, /*statExp*/true, /*evs*/false,
                    /*splitSpecial*/false, /*gender*/false, /*shiny*/true};
        case Generation::Gen2:
            return {false, true, false, true, true, true, false, false,
                    true, false, true, false, false, true, true};
        case Generation::Gen3:
            return {true, true, true, true, true, true, true, false,
                    false, true, false, true, true, true, true};
        case Generation::Gen4:
        case Generation::Gen5:
        case Generation::Gen6:
        case Generation::Gen7:
        case Generation::Gen8:
            return {true, true, true, true, true, true, true, false,
                    false, true, false, true, true, true, true};
        case Generation::Gen9:
            return {true, true, true, true, true, true, true, true,
                    false, true, false, true, true, true, true};
    }
    return {};
}

constexpr bool supplementalPanelShowsHeldItem(Generation generation) noexcept {
    return capabilitiesForGeneration(generation).supportsHeldItem;
}

constexpr bool supplementalPanelShowsRibbons(Generation generation) noexcept {
    return capabilitiesForGeneration(generation).supportsRibbons;
}

constexpr std::array<const char*, 5> gen1StatLabels() noexcept {
    return {"HP", "Attack", "Defense", "Speed", "Special"};
}

constexpr bool gen1HasFakeModernFields() noexcept {
    constexpr auto c = capabilitiesForGeneration(Generation::Gen1);
    return c.supportsAbility || c.supportsHeldItem || c.supportsNature || c.supportsFriendship ||
           c.supportsEgg || c.supportsMetLevel || c.supportsRibbons || c.supportsMarks ||
           c.usesIVs || c.usesEVs || c.hasSplitSpecial || c.supportsGender;
}

constexpr bool previewNavigationMutatesSource() noexcept { return false; }
constexpr bool viewAllowsMutation() noexcept { return false; }
constexpr bool liveRetroArchWriteEnabled() noexcept { return false; }
constexpr bool liveInstalledGameWriteEnabled() noexcept { return false; }
constexpr bool liveOtherEmulatorWriteEnabled() noexcept { return false; }
constexpr bool partyEditEnabled() noexcept { return false; }

} // namespace PokeBank::UIModel::PokemonEditorFoundation
