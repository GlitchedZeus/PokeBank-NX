#ifndef UI_TRAINER_VIEW_SCREEN_H
#define UI_TRAINER_VIEW_SCREEN_H

#include <string>
#include <vector>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/PKSEFramebuffer.h"
#include "Panels/TrainerInfoPanel.h"
#include "Panels/ModeSelectorPanel.h"
#include "Panels/PartyPokemonPanel.h"
#include "Panels/ItemsPanel.h"
#include "Dialogs/EditDialog.h"
#include "Dialogs/SaveConfirmDialog.h"
#include "Dialogs/StatEditDialog.h"
#include "Modals/PokemonDetailsModal.h"

// Forward declaration
namespace Trainer {
    class Trainer;
}

namespace UI {
    class TrainerViewScreen : public UIScreen {
    public:
        enum class ViewMode {
            Party,
            Boxes,
            Items
        };

        TrainerViewScreen(Trainer::Trainer& trainer, const std::string& titleName, const std::string& backupDir, u64 titleId);
        void update(const PadState& pad) override;
        void draw(PKSEFramebuffer& fb) override;
        bool shouldExit() const override { return goBack; }
        bool hasRequestedExit() const { return exitRequested; }

        // Public state - accessible by UI components (Panels, Dialogs, Modals)
        Trainer::Trainer& trainer;
        std::string titleName;
        std::string backupDir;
        u64 titleId;
        int scrollOffset;
        bool goBack;
        bool exitRequested;  // True when user presses + to close app

        ViewMode selectedMode;
        int currentPage;
        int totalPages;
        int selectedCategory;  // For Items mode: selected PouchType
        int selectedBoxIndex;  // For Boxes mode: selected box (0-31)
        int selectedPartyIndex;  // For Party mode: selected party Pokemon (0-5)
        bool detailViewActive;  // True when detail panel is active (for Items/Boxes)
        int selectedItemIndex;  // Selected item/pokemon index in detail view (item for Items, slot for Boxes)

        // Item editing state
        bool editDialogActive;  // True when editing an item's amount
        int editDialogValue;    // Current value being edited
        int editDialogOriginalValue;  // Original value before editing

        // Save confirmation state
        bool saveConfirmActive;  // True when showing save confirmation dialog
        bool hasUnsavedChanges;  // True when data has been modified

        // Stat editing state (for Pokemon EV/IV editing)
        bool statEditDialogActive;  // True when editing Pokemon stats
        int statEditSelectedStat;  // Which stat is being edited (0-5: HP, ATK, DEF, SPE, SPA, SPD)
        Dialogs::StatEditMode statEditMode;  // IV or EV editing mode
        int statEditValue;  // Current value being edited (points to either IV or EV)
        int statEditOriginalIV;  // Original IV value
        int statEditOriginalEV;  // Original EV value
        int statEditCurrentIV;  // Current edited IV value (preserved when switching modes)
        int statEditCurrentEV;  // Current edited EV value (preserved when switching modes)
        bool statEditOriginalShiny;  // Original shiny status
        bool statEditCurrentShiny;  // Current edited shiny status

        // Pokemon details modal state
        bool pokemonDetailsActive;  // True when showing pokemon details modal
        bool pokemonDetailsIsParty;  // True if editing party Pokemon, false for box Pokemon
        int pokemonDetailsPartyIndex;  // Index in party (0-5) when editing party Pokemon
        int pokemonDetailsCategory;  // Selected category: 0=Main, 1=Met, 2=Stats, 3=Moves, 4=Cosmetic, 5=OT/Misc
        int pokemonDetailsSelectedStat;  // For Stats category: which stat is selected for editing
        int pokemonDetailsSelectedField;  // For Main category: which field is selected (0=PID, 1=Species, 2=Gender, 3=Shiny, etc.)
        bool pokemonDetailsEditing;  // True when editing a stat value or main field
        int pokemonDetailsEditValue;  // Current edit value for stat
    };
}

#endif
