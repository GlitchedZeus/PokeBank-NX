#ifndef POKEBANK_UI_ACTION_SHEET_MODEL_H
#define POKEBANK_UI_ACTION_SHEET_MODEL_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace PokeVault::UIModel {
    enum class PokemonLocation : uint8_t {
        Party,
        SaveBox,
        Bank,
    };

    struct PokemonTarget {
        PokemonLocation location = PokemonLocation::SaveBox;
        int box = 0;
        int slot = 0;

        constexpr bool operator==(const PokemonTarget&) const noexcept = default;
    };

    enum class PokemonAction : uint8_t {
        View,
        AddToMasterVault,
        AddToBank,
        TransferToGame,
        Edit,
        Clone,
        MakeShiny,
        LegalityAndProvenance,
        Cancel,
    };

    inline constexpr std::array<PokemonAction, 9> POKEMON_ACTIONS{
        PokemonAction::View,
        PokemonAction::AddToMasterVault,
        PokemonAction::AddToBank,
        PokemonAction::TransferToGame,
        PokemonAction::Edit,
        PokemonAction::Clone,
        PokemonAction::MakeShiny,
        PokemonAction::LegalityAndProvenance,
        PokemonAction::Cancel,
    };

    constexpr std::string_view actionLabel(PokemonAction action) noexcept {
        switch (action) {
            case PokemonAction::View:                  return "View Pokémon";
            case PokemonAction::AddToMasterVault:      return "Add to Master Vault";
            case PokemonAction::AddToBank:             return "Add to Bank…";
            case PokemonAction::TransferToGame:        return "Transfer to Game…";
            case PokemonAction::Edit:                  return "Edit";
            case PokemonAction::Clone:                 return "Clone";
            case PokemonAction::MakeShiny:             return "Make Shiny";
            case PokemonAction::LegalityAndProvenance: return "Legality & Provenance";
            case PokemonAction::Cancel:                return "Cancel";
        }
        return "Unknown action";
    }

    enum class ActionResult : uint8_t {
        None,
        OpenView,
        OpenEditor,
        NotYetSupported,
        Closed,
    };

    constexpr bool actionImplemented(PokemonAction action, bool allowEdit = true) noexcept {
        return action == PokemonAction::View ||
               (action == PokemonAction::Edit && allowEdit) ||
               action == PokemonAction::Cancel;
    }

    constexpr ActionResult dispatchAction(PokemonAction action, bool allowEdit = true) noexcept {
        switch (action) {
            case PokemonAction::View:   return ActionResult::OpenView;
            case PokemonAction::Edit:   return allowEdit ? ActionResult::OpenEditor : ActionResult::NotYetSupported;
            case PokemonAction::Cancel: return ActionResult::Closed;

            case PokemonAction::AddToMasterVault:
            case PokemonAction::AddToBank:
            case PokemonAction::TransferToGame:
            case PokemonAction::Clone:
            case PokemonAction::MakeShiny:
            case PokemonAction::LegalityAndProvenance:
                return ActionResult::NotYetSupported;
        }
        return ActionResult::NotYetSupported;
    }

    class PokemonActionSheet {
    public:
        void open(PokemonTarget target) noexcept {
            target_ = target;
            selected_ = 0;
            open_ = true;
        }

        ActionResult close() noexcept {
            open_ = false;
            selected_ = 0;
            return ActionResult::Closed;
        }

        bool isOpen() const noexcept { return open_; }
        int selectedIndex() const noexcept { return selected_; }
        const PokemonTarget& target() const noexcept { return target_; }

        PokemonAction selectedAction() const noexcept {
            return POKEMON_ACTIONS[static_cast<std::size_t>(selected_)];
        }

        void select(int index) noexcept {
            constexpr int count = static_cast<int>(POKEMON_ACTIONS.size());
            if (index >= 0 && index < count) selected_ = index;
        }

        void navigate(int direction) noexcept {
            constexpr int count = static_cast<int>(POKEMON_ACTIONS.size());
            if (!open_ || direction == 0) return;
            const int step = direction < 0 ? -1 : 1;
            selected_ = (selected_ + step + count) % count;
        }

        ActionResult activate(bool allowEdit = true) noexcept {
            if (!open_) return ActionResult::None;
            const ActionResult result = dispatchAction(selectedAction(), allowEdit);
            if (result != ActionResult::NotYetSupported) close();
            return result;
        }

    private:
        bool open_ = false;
        int selected_ = 0;
        PokemonTarget target_{};
    };
}

#endif
