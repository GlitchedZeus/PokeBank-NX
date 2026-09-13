#ifndef UI_TRAINER_VIEW_SCREEN_WRAPPER_H
#define UI_TRAINER_VIEW_SCREEN_WRAPPER_H

// Keep every dependency that can declare a generic update()/draw() out of the narrow macro window
// below. The accepted TrainerViewScreen class body is included unchanged from TrainerViewScreenBase.h;
// its two public frame entry declarations are expanded with implementation companions. The legacy
// base stays byte-for-byte isolated, while the accepted GSC overlay can also be compiled under a
// private name and composed behind the Generation I staged editor wrapper.
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

#define update(...) updateLegacyBase(__VA_ARGS__); void updateGSCOverlay(__VA_ARGS__); void update(__VA_ARGS__)
#define draw(...) drawLegacyBase(__VA_ARGS__); void drawGSCOverlay(__VA_ARGS__); void draw(__VA_ARGS__)
#include "UI/TrainerViewScreenBase.h"
#undef draw
#undef update

#endif
