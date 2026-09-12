#include <cstring>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sys/stat.h>

#include "Names/ItemPouches.h"
#include "Names/MoveInfo.h"
#include "Names/MoveNames.h"
#include "Names/MovePresence.h"
#include "Names/LocationNames.h"
#include "Names/FormNames.h"
#include "Enums/Ball.h"
#include "Legality/Legality.h"
#include "Globals.h"
#include "Games/GameIdentity.h"
#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Save/GetSaveFileContents.h"
#include "UI/TrainerViewScreen.h"
#include "UI/TouchInput.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/Panels/PartyPokemonPanel.h"
#include "UI/Panels/BoxPokemonPanel.h"
#include "UI/Panels/ItemsPanel.h"
#include "UI/Panels/StoragePanel.h"
#include "UI/Panels/HomeMenuPanel.h"
#include "UI/Dialogs/ItemEditDialog.h"
#include "UI/Dialogs/SaveConfirmDialog.h"
#include "UI/Dialogs/StatEditDialog.h"
#include "UI/Modals/PokemonDetailsModal.h"
#include "Utils/HelperUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/Logger.h"
#include "Utils/EventLog.h"
#include "Utils/FileUtilities.h"
#include "Utils/Settings.h"
#include "Trainer/Trainer.h"
#include "Trainer/Inventory.h"
#include "Trainer/Inventory9LZA.h"
#include "Trainer/Inventory9SV.h"
#include "Trainer/Inventory8LA.h"
#include "Trainer/Inventory8BDSP.h"
#include "Trainer/Inventory7LGPE.h"
#include "Trainer/Inventory3FRLG.h"
#include "Pokemon/Pokemon.h"
#include "Pokemon/Experience.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/AbilityInfo.h"
#include "Pokemon/FormInfo.h"
#include "Pokemon/LearnsetTable.h"
#include "Conversion/Convert.h"
#include "Utils/StringHelpers.h"

// Compile the accepted screen implementation byte-for-byte, but under implementation-only names for
// its two public frame entry points. TrainerViewScreenGSCOverlay.cpp supplies the real update()/draw()
// wrappers and delegates every non-GSC behavior straight back here.
#define update updateLegacyBase
#define draw drawLegacyBase
#include "TrainerViewScreenBase.inc"
#undef draw
#undef update
