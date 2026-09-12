#ifndef UI_TRAINER_VIEW_SCREEN_WRAPPER_H
#define UI_TRAINER_VIEW_SCREEN_WRAPPER_H

// Keep every dependency that can declare a generic update()/draw() out of the narrow macro window
// below. The accepted TrainerViewScreen class body is included unchanged from TrainerViewScreenBase.h;
// only its two virtual entry declarations are expanded with private implementation companions so GSC
// can add a tiny read-only overlay without rewriting the large accepted screen implementation.
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <switch.h>
#include "Globals.h"
#include "Safety/WritePolicy.h"
#include "UI/ActionSheetModel.h"
#include "Safety/SourceMutationPolicy.h"
#include "UI/NavigationRepeat.h"
#include "UI/UIScreen.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Bank.h"
#include "Panels/PartyPokemonPanel.h"
#include "Panels/ItemsPanel.h"
#include "Dialogs/ItemEditDialog.h"
#include "Dialogs/SaveConfirmDialog.h"
#include "Dialogs/StatEditDialog.h"
#include "Dialogs/PickerDialog.h"

#define update(...) updateLegacyBase(__VA_ARGS__); void update(__VA_ARGS__)
#define draw(...) drawLegacyBase(__VA_ARGS__); void draw(__VA_ARGS__)
#include "UI/TrainerViewScreenBase.h"
#undef draw
#undef update

#endif
