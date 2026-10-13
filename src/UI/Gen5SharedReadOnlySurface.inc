// Generation V's guarded adapter for the EXISTING shared Trainer/Party/Boxes
// renderer. View, verified PK5 staged Edit and Review are in app memory only.
// This layer owns EVERY Gen V controller frame: inherited Storage/Bank, Items,
// source-save dialogs, Create and generic mutation handlers are unreachable.
// Touch targets are isolated to verified read-only and native-field staged
// actions; native bank, source-writing and unsupported mutation routes remain barred.
#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Utils/PokeBankPaths.h"
#include "Utils/FileUtilities.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Pokemon/Pokemon5ReadOnlyView.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Names/LocationNames.h"
#include "Utils/Keyboard.h"

namespace UI::Gen5SharedReadOnlySurface {
namespace {
bool isGen5Source(const TrainerViewScreen& screen) noexcept {
    // Deliberately check the exact source ID, not the Trainer's claimed game
    // group: even a broken adapter cannot fall back to generic mutators.
    return PokeVault::Integration::Gen5EditorProvider::isGen5NdsId(screen.sourceGameId);
}
using ScreenState=PokeVault::Integration::Gen5::Gen5SharedScreenState;
using Surface=ScreenState::Surface;
using Slot=ScreenState::Slot;
using Region=PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace::Region;
// Screen-local ids never overlap inherited Trainer/Storage touch actions.
// Every tap is release-confirmed by touchedButtonId(), not touch-down.
constexpr int kGen5ActionTouchBase=77000;
constexpr int kGen5FieldTouchBase=77100;
constexpr int kGen5DecreaseTouchBase=77200;
constexpr int kGen5IncreaseTouchBase=77300;
constexpr int kGen5PartyTouchBase=77400;
constexpr int kGen5MovePickerTouchBase=77500;
constexpr int kGen5LocationTouchBase=78000;
constexpr int kGen5IdentityTouchBase=78500;
constexpr int kGen5SpeciesTouchBase=78600;
constexpr int kGen5NatureTouchBase=79500;
constexpr int kGen5CloneTouchBase=80000;

PokeVault::Legacy::Gen5ReadOnlyTrainer& nativeTrainer(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::Gen5ReadOnlyTrainer&>(screen.trainer);
}
Slot selectedNativeSlot(const TrainerViewScreen& screen) {
    return screen.selectedMode==TrainerViewScreen::ViewMode::Party
        ? Slot{Region::Party,0,static_cast<size_t>(std::max(0,screen.selectedPartyIndex))}
        : Slot{Region::Box,static_cast<size_t>(std::max(0,screen.selectedBoxIndex)),
               static_cast<size_t>(std::max(0,screen.selectedItemIndex))};
}
void beginActions(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    std::string error;
    if(!trainer.sharedScreen().openActions(trainer.stagedPokemon(),
                                          selectedNativeSlot(screen),error))
        screen.postStatus(error,300);
    screen.controllerNavigation.reset();
}
void keepDraft(TrainerViewScreen& screen) {
    auto& trainer=nativeTrainer(screen);
    std::string error;
    const bool kept=trainer.sharedScreen().keepWithPresentation(
        trainer.stagedPokemon(),
        [&](std::string& refreshError) {
            return trainer.refreshStagedPokemonPresentation(refreshError);
        },error);
    if(!kept) {
        screen.postStatus(error.empty()?"Gen V Keep was rejected":error,360);
        return; // Dirty draft, cursor and prior staged bytes are still intact.
    }
    screen.postStatus("PK5 draft kept in app memory only; emulator save unchanged",300);
    screen.controllerNavigation.reset();
}
} // namespace

bool handleInput(TrainerViewScreen& screen,u64 down,u64 held,
                 int stickX,int stickY,const TouchInput& touch) {
    if(!isGen5Source(screen))return false;

    // Refuse all inherited controls on a mismatched Trainer type/family.
    const auto family=screen.trainer.getGameGroup();
    if(family!=Enums::GameVersion::BW && family!=Enums::GameVersion::B2W2) {
        screen.postStatus("Generation V source presentation mismatch",180);
        if(down&HidNpadButton_B)screen.goBack=true;
        return true;
    }
    auto& trainer=nativeTrainer(screen);
    if(trainer.sourceGameId()!=screen.sourceGameId) {
        screen.postStatus("Generation V exact title identity mismatch",180);
        if(down&HidNpadButton_B)screen.goBack=true;
        return true;
    }
    auto& staged=trainer.stagedPokemon();
    auto& state=trainer.sharedScreen();
    if(screen.storageStatusFrames>0)--screen.storageStatusFrames;
    // Consume only Gen V-owned touch hitboxes published by the last frame.
    // Never forward a tap to inherited bank/source/create handlers.
    const int tap=screen.touchedButtonId(touch);
    if(state.surface()==Surface::Actions) {
        const auto actions=state.availableActions(staged);
        if(tap>=kGen5ActionTouchBase &&
           tap<kGen5ActionTouchBase+static_cast<int>(actions.count)) {
            const int row=tap-kGen5ActionTouchBase;
            state.moveAction(row-state.actionRow(),staged);
            down|=HidNpadButton_A;
        }
    } else if(state.surface()==Surface::Edit) {
        constexpr int fieldCount=static_cast<int>(ScreenState::kEditableRows.size());
        if(tap>=kGen5FieldTouchBase && tap<kGen5FieldTouchBase+fieldCount) {
            const int chosen=tap-kGen5FieldTouchBase;
            const bool alreadyFocused=chosen==state.fieldRow();
            state.moveField(chosen-state.fieldRow());
            if(alreadyFocused) {
                const auto field=ScreenState::kEditableRows[static_cast<size_t>(chosen)].field;
                if(field==ScreenState::Field::Nature ||
                   field==ScreenState::Field::Move ||
                   field==ScreenState::Field::MetLocation ||
                   field==ScreenState::Field::TrainerId ||
                   field==ScreenState::Field::SecretId ||
                   field==ScreenState::Field::Level ||
                   field==ScreenState::Field::Experience ||
                   field==ScreenState::Field::Species ||
                   field==ScreenState::Field::Nickname ||
                   field==ScreenState::Field::OriginalTrainer ||
                   field==ScreenState::Field::Ability ||
                   field==ScreenState::Field::Form ||
                   field==ScreenState::Field::Gender)
                    down|=HidNpadButton_A;
            }
        }
        else if(tap>=kGen5DecreaseTouchBase &&
                tap<kGen5DecreaseTouchBase+fieldCount) {
            state.moveField(tap-kGen5DecreaseTouchBase-state.fieldRow());
            down|=HidNpadButton_Left;
        } else if(tap>=kGen5IncreaseTouchBase &&
                  tap<kGen5IncreaseTouchBase+fieldCount) {
            state.moveField(tap-kGen5IncreaseTouchBase-state.fieldRow());
            down|=HidNpadButton_Right;
        }
    } else if(state.surface()==Surface::MovePicker &&
              tap>=kGen5MovePickerTouchBase &&
              tap<kGen5MovePickerTouchBase+static_cast<int>(
                  PokeVault::Integration::Gen5::MaxNativeMove5)+1) {
        state.movePickerCursor(tap-kGen5MovePickerTouchBase-
                               static_cast<int>(state.movePickerSelection()));
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::LocationPicker &&
              tap>=kGen5LocationTouchBase &&
              tap<kGen5LocationTouchBase+static_cast<int>(
                    PokeVault::Integration::Gen5::NativeMetLocations5.size())) {
        state.moveLocationCursor(tap-kGen5LocationTouchBase-
                                 static_cast<int>(state.locationPickerIndex()));
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::IdentityPicker &&
              tap>=kGen5IdentityTouchBase &&
              tap<kGen5IdentityTouchBase+static_cast<int>(state.identityChoiceCount())) {
        state.moveIdentityCursor(tap-kGen5IdentityTouchBase-
                                 static_cast<int>(state.identitySelection()));
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::SpeciesPicker &&
              tap>=kGen5SpeciesTouchBase &&
              tap<kGen5SpeciesTouchBase+static_cast<int>(state.speciesChoiceCount())) {
        state.moveSpeciesCursor(tap-kGen5SpeciesTouchBase-
                                static_cast<int>(state.speciesChoiceIndex()));
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::NaturePicker &&
              tap>=kGen5NatureTouchBase && tap<kGen5NatureTouchBase+25) {
        state.moveNatureCursor(tap-kGen5NatureTouchBase-
                               static_cast<int>(state.natureSelection()));
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::CloneDestination &&
              tap>=kGen5CloneTouchBase && tap<kGen5CloneTouchBase+720) {
        state.moveCloneCursor(tap-kGen5CloneTouchBase-
                              static_cast<int>(state.cloneDestinationIndex()));
        down|=HidNpadButton_A;
    } else if(state.surface()==Surface::Browse &&
              screen.selectedMode==TrainerViewScreen::ViewMode::Party &&
              tap>=kGen5PartyTouchBase &&
              tap<kGen5PartyTouchBase+static_cast<int>(screen.trainer.getPartySize())) {
        screen.selectedPartyIndex=tap-kGen5PartyTouchBase;
        down|=HidNpadButton_A;
    }
    const u64 keys=screen.controllerNavigation.apply(
        down,held,stickX,stickY,
        HidNpadButton_Up,HidNpadButton_Down,
        HidNpadButton_Left,HidNpadButton_Right);

    switch(state.surface()) {
        case Surface::Actions:
            if(keys&HidNpadButton_Up)state.moveAction(-1,staged);
            if(keys&HidNpadButton_Down)state.moveAction(1,staged);
            if(keys&HidNpadButton_B)state.back();
            if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.activateSelected(staged,error))
                    screen.postStatus(error,300);
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::CloneDestination:
            if(keys&HidNpadButton_Up)state.moveCloneCursor(-1);
            if(keys&HidNpadButton_Down)state.moveCloneCursor(1);
            if(keys&HidNpadButton_Left)state.moveCloneCursor(-6);
            if(keys&HidNpadButton_Right)state.moveCloneCursor(6);
            if(keys&HidNpadButton_ZL)state.moveCloneCursor(-30);
            if(keys&HidNpadButton_ZR)state.moveCloneCursor(30);
            if(keys&HidNpadButton_B)state.cancelBoxAction();
            else if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.confirmClone(staged,[&](std::string& why) {
                    return trainer.refreshStagedPokemonPresentation(why);
                },error))
                    screen.postStatus(error,320);
                else screen.postStatus(
                    "Gen V cloned to empty staged box slot; source SAV5 unchanged",260);
            }
            return true;
        case Surface::ConfirmRemoveBox:
            if(keys&HidNpadButton_B)state.cancelBoxAction();
            else if(keys&HidNpadButton_Y) {
                std::string error;
                if(!state.confirmRemoveBox(staged,[&](std::string& why) {
                    return trainer.refreshStagedPokemonPresentation(why);
                },error))
                    screen.postStatus(error,320);
                else screen.postStatus(
                    "Gen V box slot removed in app memory only; source SAV5 unchanged",260);
            }
            return true;
        case Surface::View:
            if(keys&HidNpadButton_B) {
                state.back();
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::Edit:
            if(keys&HidNpadButton_Up)state.moveField(-1);
            if(keys&HidNpadButton_Down)state.moveField(1);
            if(keys&(HidNpadButton_Left|HidNpadButton_Right|
                     HidNpadButton_ZL|HidNpadButton_ZR)) {
                const int delta=(keys&HidNpadButton_ZL)?-10:
                    (keys&HidNpadButton_ZR)?10:
                    (keys&HidNpadButton_Left)?-1:1;
                std::string error;
                if(!state.adjustField(delta,error))
                    screen.postStatus(error,180);
            }
            if(keys&HidNpadButton_A) {
                if(ScreenState::kEditableRows[static_cast<size_t>(state.fieldRow())].field==
                       ScreenState::Field::Move) {
                    std::string error;
                    if(!state.openMovePicker(error))screen.postStatus(error,300);
                } else {
                    const auto& row=ScreenState::kEditableRows[
                        static_cast<size_t>(state.fieldRow())];
                    if(row.field==ScreenState::Field::MetLocation) {
                        std::string error;
                        if(!state.openLocationPicker(error))screen.postStatus(error,300);
                    } else if(row.field==ScreenState::Field::Ability ||
                              row.field==ScreenState::Field::Form ||
                              row.field==ScreenState::Field::Gender) {
                        std::string error;
                        if(!state.openIdentityPicker(error))screen.postStatus(error,300);
                    } else if(row.field==ScreenState::Field::Nature) {
                        std::string error;
                        if(!state.openNaturePicker(error))screen.postStatus(error,300);
                    } else if(row.field==ScreenState::Field::Species) {
                        std::string error;
                        if(!state.openSpeciesPicker(error))screen.postStatus(error,300);
                    } else if(row.field==ScreenState::Field::Nickname ||
                              row.field==ScreenState::Field::OriginalTrainer) {
                        const auto current=state.draft().current();
                        if(current) {
                            const bool nickname=row.field==ScreenState::Field::Nickname;
                            const auto original=PokeVault::Integration::Gen5::asciiName5(
                                current->decodedBytes(),nickname);
                            const auto input=Utils::promptText(
                                nickname?"Generation V Nickname":"Generation V OT Name",
                                "ASCII only - no unsupported glyph conversion",
                                original?*original:std::string{},
                                nickname?10:7);
                            if(input.accepted) {
                                std::string error;
                                if(!state.stageNameValue(input.text,error))
                                    screen.postStatus(error,360);
                                else screen.postStatus(
                                    "Native PK5 text staged; original SAV5 unchanged",260);
                            }
                            screen.controllerNavigation.reset();
                        }
                    } else if(row.field==ScreenState::Field::TrainerId ||
                              row.field==ScreenState::Field::SecretId ||
                              row.field==ScreenState::Field::Level ||
                              row.field==ScreenState::Field::Experience ||
                              row.field==ScreenState::Field::Species) {
                        const auto current=state.draft().current();
                        if(current) {
                            const auto old=ScreenState::fieldValue(*current,row,state.black2White2());
                            const bool expField=row.field==ScreenState::Field::Experience;
                            const auto info=PokeVault::Integration::Gen5::personal5(
                                current->species(),current->form(),state.black2White2());
                            if((expField || row.field==ScreenState::Field::Level) && !info) {
                                screen.postStatus("Missing BW/B2W2 experience growth data",300);
                                return true;
                            }
                            const int maximum=expField?static_cast<int>(
                                PokeVault::Integration::Gen5::experienceForLevel5(
                                    info.info->growthRate,100)):
                                row.field==ScreenState::Field::Species?649:
                                row.field==ScreenState::Field::Level?100:65535;
                            const char* label=row.field==ScreenState::Field::TrainerId
                                ? "Generation V Trainer ID":
                                row.field==ScreenState::Field::SecretId
                                ? "Generation V Secret ID":
                                row.field==ScreenState::Field::Level
                                ? "Generation V Level":
                                row.field==ScreenState::Field::Species
                                ? "Generation V Species Dex ID (native-compatible only)":
                                  "Generation V Experience";
                            const auto result=Utils::promptNumber(
                                label,static_cast<int>(old),
                                (row.field==ScreenState::Field::Level ||
                                 row.field==ScreenState::Field::Species)?1:0,maximum);
                            if(result.accepted) {
                                std::string error;
                                if(!state.stageFieldValue(
                                      static_cast<uint32_t>(result.value),error))
                                    screen.postStatus(error,300);
                                else screen.postStatus(
                                    "Gen V value changed in local draft; source unchanged",260);
                            }
                            screen.controllerNavigation.reset();
                        }
                    } else keepDraft(screen);
                }
            } else if(keys&HidNpadButton_X)keepDraft(screen);
            else if(keys&HidNpadButton_Y) {
                state.discardDraft();
                screen.postStatus("Local Gen V draft discarded; staged edits preserved",220);
            } else if(keys&HidNpadButton_B)state.back();
            return true;
        case Surface::MovePicker:
            if(keys&HidNpadButton_Up)state.movePickerCursor(-1);
            if(keys&HidNpadButton_Down)state.movePickerCursor(1);
            if(keys&HidNpadButton_Left)state.movePickerCursor(-8);
            if(keys&HidNpadButton_Right)state.movePickerCursor(8);
            if(keys&HidNpadButton_ZL)state.movePickerCursor(-40);
            if(keys&HidNpadButton_ZR)state.movePickerCursor(40);
            if(keys&HidNpadButton_B)state.cancelMovePicker();
            else if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.acceptMovePicker(error))screen.postStatus(error,300);
            }
            return true;
        case Surface::LocationPicker:
            if(keys&HidNpadButton_Up)state.moveLocationCursor(-1);
            if(keys&HidNpadButton_Down)state.moveLocationCursor(1);
            if(keys&HidNpadButton_Left)state.moveLocationCursor(-8);
            if(keys&HidNpadButton_Right)state.moveLocationCursor(8);
            if(keys&HidNpadButton_ZL)state.moveLocationCursor(-40);
            if(keys&HidNpadButton_ZR)state.moveLocationCursor(40);
            if(keys&HidNpadButton_B)state.cancelLocationPicker();
            else if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.acceptLocationPicker(error))screen.postStatus(error,300);
            }
            return true;
        case Surface::NaturePicker:
            if(keys&HidNpadButton_Up)state.moveNatureCursor(-1);
            if(keys&HidNpadButton_Down)state.moveNatureCursor(1);
            if(keys&HidNpadButton_Left)state.moveNatureCursor(-5);
            if(keys&HidNpadButton_Right)state.moveNatureCursor(5);
            if(keys&HidNpadButton_ZL)state.moveNatureCursor(-10);
            if(keys&HidNpadButton_ZR)state.moveNatureCursor(10);
            if(keys&HidNpadButton_B)state.cancelNaturePicker();
            else if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.acceptNaturePicker(error))screen.postStatus(error,300);
            }
            return true;
        case Surface::SpeciesPicker:
            if(keys&HidNpadButton_Up)state.moveSpeciesCursor(-1);
            if(keys&HidNpadButton_Down)state.moveSpeciesCursor(1);
            if(keys&HidNpadButton_Left)state.moveSpeciesCursor(-8);
            if(keys&HidNpadButton_Right)state.moveSpeciesCursor(8);
            if(keys&HidNpadButton_ZL)state.moveSpeciesCursor(-40);
            if(keys&HidNpadButton_ZR)state.moveSpeciesCursor(40);
            if(keys&HidNpadButton_B)state.cancelSpeciesPicker();
            else if(keys&HidNpadButton_X) {
                const auto result=Utils::promptText(
                    "Generation V Species Search","Filter national Dex names",
                    std::string(state.speciesSearch()),32);
                if(result.accepted)
                    state.filterSpecies(result.text,[](uint16_t id) {
                        return Names::getSpeciesName(id);
                    });
                screen.controllerNavigation.reset();
            } else if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.acceptSpeciesPicker(error))screen.postStatus(error,300);
            }
            return true;
        case Surface::IdentityPicker:
            if(keys&HidNpadButton_Up)state.moveIdentityCursor(-1);
            if(keys&HidNpadButton_Down)state.moveIdentityCursor(1);
            if(keys&HidNpadButton_Left)state.moveIdentityCursor(-8);
            if(keys&HidNpadButton_Right)state.moveIdentityCursor(8);
            if(keys&HidNpadButton_B)state.cancelIdentityPicker();
            else if(keys&HidNpadButton_A) {
                std::string error;
                if(!state.acceptIdentityPicker(error))screen.postStatus(error,300);
            }
            return true;
        case Surface::ConfirmDraft:
            // A may Keep, but never destroys a staged draft or external source.
            if(keys&HidNpadButton_A)keepDraft(screen);
            else if(keys&HidNpadButton_Y)state.discardDraft();
            else if(keys&HidNpadButton_B)state.continueEditing();
            return true;
        case Surface::Review:
            if(keys&HidNpadButton_B)state.back();
            else if(keys&HidNpadButton_X) {
                if(!state.requestExport(staged))
                    screen.postStatus("Gen V needs verified staged changes to export",300);
            }
            else if(keys&HidNpadButton_Y)state.requestDiscardAll();
            else if(keys&(HidNpadButton_Left|HidNpadButton_Up|
                         HidNpadButton_L))state.moveReviewPage(-1,staged);
            else if(keys&(HidNpadButton_Right|HidNpadButton_Down|
                         HidNpadButton_R))state.moveReviewPage(1,staged);
            return true;
        case Surface::ConfirmExport:
            if(keys&HidNpadButton_B) {
                state.back();
            } else if(keys&HidNpadButton_Y) {
                // Re-read the exact physical source in read-only mode before
                // serializing an app-owned backup and edited working copy.
                // Candidate construction separately rejects ANY baseline
                // byte change, including the unselected retail save copy.
                std::vector<uint8_t> observed;
                std::string error,container;
                if(!PokeVault::Integration::Gen5::readNormalizedSourceReadOnly(
                       screen.backupDir,observed,error,&container)) {
                    screen.postStatus("Gen V export blocked: "+error,400);
                    return true;
                }
                PokeVault::Integration::Gen5::WorkingCopyExportRequest request;
                request.profileIdentity=PokeBank::Paths::accountUidHex(screen.userUid);
                // AccountUid{0,0} is the existing offline "Pokemon Saves"
                // source profile. Use that same explicit identity instead
                // of failing an otherwise validated external save export.
                if(request.profileIdentity.empty())
                    request.profileIdentity=std::string(32,'0');
                request.timestamp=Utils::getTimestamp();
                request.sourcePath=screen.backupDir;
                request.observedSourceBytes=observed;
#ifdef POKEVAULT_GIT_COMMIT
                request.applicationSha=POKEVAULT_GIT_COMMIT;
#endif
                const auto result=
                    PokeVault::Integration::Gen5::publishGen5WorkingCopy(staged,request);
                if(!result.success) {
                    screen.postStatus("Gen V export blocked: "+result.error,480);
                    return true;
                }
                state.finishExport();
                screen.postStatus("Verified app-owned Gen V copy: "+result.directory+
                                  " (original SAV5 unchanged)",480);
                screen.controllerNavigation.reset();
            }
            return true;
        case Surface::ConfirmDiscardAll:
            if(keys&HidNpadButton_B)state.back();
            else if(keys&HidNpadButton_Y) {
                std::string error;
                if(!state.discardAllWithPresentation(staged,
                    [&](std::string& refreshError) {
                        return trainer.refreshStagedPokemonPresentation(refreshError);
                    },error)) {
                    screen.postStatus(error.empty()
                        ?"Discard rejected; previous staged PK5 records preserved":error,360);
                } else {
                    screen.postStatus("All app-memory PK5 changes discarded; SAV5 unchanged",260);
                }
            }
            return true;
        case Surface::ConfirmExit:
            if(keys&HidNpadButton_B)state.back();
            else if(keys&HidNpadButton_X)state.reviewFromExit(staged);
            else if(keys&HidNpadButton_Y) {
                if(state.confirmDiscardAndExit(staged))screen.goBack=true;
            }
            return true;
        case Surface::Browse:break;
    }

    // The same underlying Trainer/Party/Boxes renderer is reused.
    // All inherited generic storage/bank/source mutation routes stay barred.
    screen.detailViewActive=true;
    screen.details.active=false;
    if(keys&HidNpadButton_B) {
        if(state.requestExit(staged))screen.goBack=true;
        return true;
    }
    if(keys&HidNpadButton_Y) {
        screen.selectedMode=TrainerViewScreen::ViewMode::Trainer;
        return true;
    }
    if(keys&HidNpadButton_X) {
        screen.selectedMode=screen.selectedMode==TrainerViewScreen::ViewMode::Party
            ? TrainerViewScreen::ViewMode::Boxes:TrainerViewScreen::ViewMode::Party;
        return true;
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Trainer)return true;
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Party) {
        const int count=static_cast<int>(screen.trainer.getPartySize());
        if(count>0) {
            if(keys&(HidNpadButton_Down|HidNpadButton_Right))
                screen.selectedPartyIndex=(screen.selectedPartyIndex+1)%count;
            if(keys&(HidNpadButton_Up|HidNpadButton_Left))
                screen.selectedPartyIndex=(screen.selectedPartyIndex+count-1)%count;
            if(keys&HidNpadButton_A)beginActions(screen);
        }
        return true;
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes) {
        constexpr int columns=6;
        const int capacity=static_cast<int>(screen.trainer.getSlotsPerBox());
        const int count=static_cast<int>(screen.trainer.getBoxCount());
        if(capacity<=0 || count<=0)return true;
        screen.selectedBoxIndex=std::clamp(screen.selectedBoxIndex,0,count-1);
        screen.selectedItemIndex=std::clamp(screen.selectedItemIndex,0,capacity-1);
        if(keys&HidNpadButton_L)
            screen.selectedBoxIndex=(screen.selectedBoxIndex+count-1)%count;
        if(keys&HidNpadButton_R)
            screen.selectedBoxIndex=(screen.selectedBoxIndex+1)%count;
        if(keys&HidNpadButton_Up)
            screen.selectedItemIndex=(screen.selectedItemIndex+capacity-columns)%capacity;
        if(keys&HidNpadButton_Down)
            screen.selectedItemIndex=(screen.selectedItemIndex+columns)%capacity;
        if(keys&HidNpadButton_Left)
            screen.selectedItemIndex=(screen.selectedItemIndex+capacity-1)%capacity;
        if(keys&HidNpadButton_Right)
            screen.selectedItemIndex=(screen.selectedItemIndex+1)%capacity;
        if(keys&HidNpadButton_A)beginActions(screen);
    }
    return true;
}

// A narrow passive PK5 panel within the shared Trainer screen. It displays
// only verified record fields; unlike the generic details modal it NEVER
// calls Legality::analyze or synthesizes unknown history/stat calculations.
void drawVerifiedSummary(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                         const Pokemon::Pokemon* p,bool editDraft=false) {
    if(!p)return;
    const int width=fb.getWidth(),height=fb.getHeight();
    fb.drawFilledRect(0,0,width,height-kNavBarH,Color(0,0,0,165));
    const int x=180,y=65,w=width-360,h=height-kNavBarH-125;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+26,y+22,"Generation V - Verified PK5 Summary",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+26,y+60,editDraft ? "App-memory draft ONLY - not written to the emulator SAV5"
                          : "Passive View - source SAV5 unchanged - legality unresolved",
                Colors::Accent,TextStyle::Caption);
    const std::string species=std::string(p->species());
    fb.drawText(x+30,y+106,species+"  #"+std::to_string(p->speciesID()),
                Colors::Text,TextStyle::Heading);
    const std::string level=p->level()!=0
        ? "Level " + std::to_string(p->level())
        : "Box level: not recorded natively";
    fb.drawText(x+30,y+152,level+
                (p->isShiny(p->id32(),species)?"  /  Shiny":""),
                Colors::TextSecondary,TextStyle::Body);
    fb.drawText(x+30,y+196,
        "Nature: "+std::string(Trainer::getNatureName(p->nature()))+
        "  |  Friendship: "+std::to_string(p->friendship()),
        Colors::Text,TextStyle::Body);
    fb.drawText(x+30,y+235,
        "Ability ID: "+std::to_string(p->ability())+
        "  |  Held item ID: "+std::to_string(p->heldItem()),
        Colors::Text,TextStyle::Body);
    fb.drawText(x+30,y+281,
        "IV  HP "+std::to_string(p->ivHP())+" / Atk "+std::to_string(p->ivATK())+
        " / Def "+std::to_string(p->ivDEF())+" / Spe "+std::to_string(p->ivSPE())+
        " / SpA "+std::to_string(p->ivSPA())+" / SpD "+std::to_string(p->ivSPD()),
        Colors::Text,TextStyle::Body);
    fb.drawText(x+30,y+319,
        "EV  HP "+std::to_string(p->evHP())+" / Atk "+std::to_string(p->evATK())+
        " / Def "+std::to_string(p->evDEF())+" / Spe "+std::to_string(p->evSPE())+
        " / SpA "+std::to_string(p->evSPA())+" / SpD "+std::to_string(p->evSPD()),
        Colors::Text,TextStyle::Body);
    for(int i=0;i<4;++i) {
        fb.drawText(x+30+(i%2)*435,y+375+(i/2)*43,
            "Move "+std::to_string(i+1)+": "+
            std::string(Names::getMoveName(p->move(i)))+
            "  PP "+std::to_string(p->movePP(i)),
            Colors::TextSecondary,TextStyle::Body);
    }
    fb.drawText(x+30,y+478,
        "TID "+std::to_string(p->tid16())+
        " / SID "+std::to_string(p->sid16())+
        "  |  Met location "+std::to_string(p->metLocation())+
        "  |  Ball "+std::to_string(p->ball()),
        Colors::TextSecondary,TextStyle::Caption);
}

void dimShared(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-kNavBarH,Color(0,0,0,140));
}
void drawActions(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                 const ScreenState& state,
                 const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& workspace) {
    dimShared(screen,fb);
    const auto choices=state.availableActions(workspace);
    const auto geometry=PokeBank::UIModel::SharedPokemonEditor::actionMenuGeometry();
    const int w=560,h=geometry.rowStart+static_cast<int>(choices.count)*geometry.rowStep+64;
    const int x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+26,y+18,"Pokemon Actions - Generation V",
                Colors::Text,TextStyle::Heading);
    const auto& slot=state.target();
    fb.drawText(x+26,y+53,slot.region==Region::Party
        ? "Party slot "+std::to_string(slot.slot+1)
        : "Box "+std::to_string(slot.box+1)+" / Slot "+std::to_string(slot.slot+1),
        Colors::TextDim,TextStyle::Caption);
    for(size_t i=0;i<choices.count;++i) {
        const int yy=y+geometry.rowStart+static_cast<int>(i)*geometry.rowStep;
        if(static_cast<int>(i)==state.actionRow())
            fb.drawSelectionHighlight(x+22,yy,w-44,geometry.rowHeight);
        screen.touchButtons.push_back(
            {kGen5ActionTouchBase+static_cast<int>(i),
             x+22,yy,w-44,geometry.rowHeight});
        fb.drawText(x+38,yy+9,
            PokeBank::UIModel::SharedPokemonEditor::actionLabel(choices[i]),
            static_cast<int>(i)==state.actionRow()?Colors::Text:Colors::TextSecondary,
            TextStyle::Body);
    }
    fb.drawText(x+26,y+h-30,
        "View / verified PK5 edit only; source SAV5 remains untouched",
        Colors::Accent,TextStyle::Caption);
    drawNavBar(fb,{{"D-pad/Stick","Choose"},{"A","Select"},{"B","Cancel"}});
}
const char* reviewedStatName(size_t stat) noexcept {
    constexpr const char* names[]={"HP","Atk","Def","Spe","SpA","SpD"};
    return stat<6?names[stat]:"?";
}
std::string reviewedFieldLabel(const PokeVault::Integration::Gen5::VerifiedFieldChange& c) {
    using Field=PokeVault::Integration::Gen5::StagedPokemon5Record::Field;
    if(c.field==Field::NicknameText)return "Nickname changed";
    if(c.field==Field::OriginalTrainerText)return "OT Name changed";
    std::string name;
    switch(c.field) {
        case Field::Nature:name="Nature";break;
        case Field::Level:name="Level";break;
        case Field::Experience:name="Experience";break;
        case Field::Species:name="Species";break;
        case Field::CloneBox:name="Clone to Box";break;
        case Field::RemoveBox:name="Remove from Box";break;
        case Field::NicknameText:name="Nickname";break;
        case Field::OriginalTrainerText:name="OT Name";break;
        case Field::Friendship:name="Friendship";break;
        case Field::IV:name=std::string("IV ")+reviewedStatName(c.stat);break;
        case Field::EV:name=std::string("EV ")+reviewedStatName(c.stat);break;
        case Field::HeldItem:name="Held Item";break;
        case Field::Ball:name="Ball";break;
        case Field::MetLevel:name="Met Level";break;
        case Field::TrainerId:name="Trainer ID";break;
        case Field::SecretId:name="Secret ID";break;
        case Field::OriginalTrainerGender:name="OT Gender";break;
        case Field::Pokerus:name="Pokerus";break;
        case Field::MetLocation:name="Met Location";break;
        case Field::FatefulEncounter:name="Fateful";break;
        case Field::Form:name="Form";break;
        case Field::AbilitySlot:name="Ability Slot";break;
        case Field::Gender:name="Gender";break;
        case Field::Move:name="Move "+std::to_string(c.stat+1);break;
        case Field::MovePPUps:name="PP Ups "+std::to_string(c.stat+1);break;
        case Field::MovePP:name="PP "+std::to_string(c.stat+1);break;
        default:name="Unsupported";break;
    }
    return name+": "+std::to_string(c.before)+" -> "+std::to_string(c.after);
}
void drawReview(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                const ScreenState& state,
                const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws) {
    dimShared(screen,fb);
    constexpr int x=150,y=59,w=980,h=534;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+24,y+18,"Review Pending Changes - Generation V",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+57,
        "Verified app-memory PK5 fields only; original emulator SAV5 unchanged",
        Colors::Accent,TextStyle::Caption);
    const auto pending=ws.pendingReview();
    constexpr size_t pageSize=ScreenState::kReviewPageSize;
    const size_t pages=std::max(size_t{1},(pending.size()+pageSize-1)/pageSize);
    const size_t page=std::min(state.reviewPage(),pages-1);
    fb.drawText(x+24,y+94,std::to_string(pending.size())+
        " staged Pokemon  /  Page "+std::to_string(page+1)+
        " of "+std::to_string(pages),Colors::Text,TextStyle::Body);
    for(size_t row=0;row<pageSize;++row) {
        const size_t i=page*pageSize+row;
        if(i>=pending.size())break;
        const auto& change=pending[i];
        const auto& loc=change.location;
        const int yy=y+133+static_cast<int>(row)*91;
        const std::string label=loc.region==Region::Party
            ? "Party slot "+std::to_string(loc.slot+1)
            : "Box "+std::to_string(loc.box+1)+" / Slot "+std::to_string(loc.slot+1);
        fb.drawFilledRoundedRect(x+18,yy,w-36,83,8,Colors::PanelAlt);
        fb.drawText(x+32,yy+8,label,Colors::Text,TextStyle::Body);
        std::string error;
        const auto verified=PokeVault::Integration::Gen5::verifyStagedReview(change,&error);
        if(!verified) {
            fb.drawText(x+32,yy+42,
                "Unverified staged difference - no legality conclusion",
                Colors::Warning,TextStyle::Caption);
            continue;
        }
        std::string line;
        for(size_t field=0;field<verified->fields.size() && field<3;++field) {
            if(!line.empty())line+="  |  ";
            line+=reviewedFieldLabel(verified->fields[field]);
        }
        if(verified->fields.size()>3)
            line+="  |  +"+std::to_string(verified->fields.size()-3)+" more";
        fb.drawText(x+32,yy+43,line,Colors::TextSecondary,TextStyle::Caption);
    }
    fb.drawText(x+24,y+h-31,"Box clone/remove staged only; no source writes or inferred legality",
                Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"L/R","Review Pages"},{"X","Export Working Copy"},
                   {"Y","Discard All..."},{"B","Back"}});
}
void drawCloneDestination(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                          const ScreenState& state,
                          const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws) {
    dimShared(screen,fb);
    constexpr int w=820,h=555;
    const int x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,16,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,16,Colors::Border,1);
    fb.drawText(x+24,y+18,"Clone Pokemon - Choose Empty Box Slot",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+56,
        "Original PK5 is unchanged; only a validated empty box can be selected",
        Colors::Accent,TextStyle::Caption);
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        size_t{720},9,state.cloneDestinationIndex());
    for(size_t i=0;i<visible.count;++i) {
        const size_t index=visible.first+i;
        const size_t box=index/30,slot=index%30;
        const bool allowed=ws.emptyCloneDestination(box,slot);
        const int yy=y+100+static_cast<int>(i)*44;
        if(index==state.cloneDestinationIndex())
            fb.drawSelectionHighlight(x+12,yy,w-24,40);
        if(allowed)
            screen.touchButtons.push_back({kGen5CloneTouchBase+static_cast<int>(index),
                x+14,yy,w-28,40});
        fb.drawText(x+28,yy+9,
            "Box "+std::to_string(box+1)+" / Slot "+std::to_string(slot+1),
            allowed?Colors::Text:Colors::TextDim,TextStyle::Body);
        fb.drawText(x+w-180,yy+11,allowed?"Empty":"Unavailable",
            allowed?Colors::Accent:Colors::TextDim,TextStyle::Caption);
    }
    fb.drawText(x+24,y+h-40,
        "A clones to empty slot in memory only; B cancels without any change",
        Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"D-pad/Stick","Browse"},{"ZL/ZR","Box"},
                   {"A","Stage Clone"},{"B","Cancel"}});
}
void drawDecision(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                  std::string title,std::string message,
                  const std::vector<ControllerHint>& keys) {
    dimShared(screen,fb);
    const int w=870,h=230,x=(fb.getWidth()-w)/2;
    const int y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+26,y+24,title,Colors::Text,TextStyle::Heading);
    fb.drawText(x+26,y+93,message,Colors::TextSecondary,TextStyle::Body);
    fb.drawText(x+26,y+142,"Source SAV5 is never modified.",
                Colors::Accent,TextStyle::Caption);
    if(!keys.empty())drawNavBar(fb,keys);
}
void drawEdit(TrainerViewScreen& screen,PKSEFramebuffer& fb,
              const ScreenState& state) {
    const auto current=state.draft().current();
    if(!current)return;
    Pokemon::Pokemon5ReadOnlyView view(*current,screen.trainer.getGameGroup());
    screen.drawGSCOverlay(fb);
    drawVerifiedSummary(screen,fb,&view,true);
    const int x=735,y=90,w=485,h=465;
    fb.drawFilledRoundedRect(x,y,w,h,14,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,14,Colors::Accent,1);
    fb.drawText(x+22,y+12,"Staged PK5 Fields",Colors::Text,TextStyle::Heading);
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        ScreenState::kEditableRows.size(),7,static_cast<size_t>(state.fieldRow()));
    for(size_t i=0;i<visible.count;++i) {
        const size_t index=visible.first+i;
        const auto& row=ScreenState::kEditableRows[index];
        const int yy=y+65+static_cast<int>(i)*52;
        if(static_cast<int>(index)==state.fieldRow())
            fb.drawSelectionHighlight(x+12,yy,w-24,46);
        // Touch selects the row; only the explicit +/- hitboxes can change
        // an in-memory PK5 value. No implicit Keep, Discard or source write.
        screen.touchButtons.push_back(
            {kGen5FieldTouchBase+static_cast<int>(index),x+14,yy,279,46});
        if(row.field!=ScreenState::Field::Nature &&
           row.field!=ScreenState::Field::Move &&
           row.field!=ScreenState::Field::MetLocation &&
           row.field!=ScreenState::Field::TrainerId &&
           row.field!=ScreenState::Field::SecretId &&
           row.field!=ScreenState::Field::Level &&
           row.field!=ScreenState::Field::Experience &&
           row.field!=ScreenState::Field::Species &&
           row.field!=ScreenState::Field::Nickname &&
           row.field!=ScreenState::Field::OriginalTrainer &&
           row.field!=ScreenState::Field::Ability &&
           row.field!=ScreenState::Field::Form &&
           row.field!=ScreenState::Field::Gender) {
            screen.touchButtons.push_back(
                {kGen5DecreaseTouchBase+static_cast<int>(index),x+308,yy+3,36,40});
            screen.touchButtons.push_back(
                {kGen5IncreaseTouchBase+static_cast<int>(index),x+418,yy+3,36,40});
        }
        std::string value=std::to_string(ScreenState::fieldValue(*current,row,state.black2White2()));
        if(row.field==ScreenState::Field::Nature)
            value=std::string(ScreenState::natureName(current->nature()));
        if(row.field==ScreenState::Field::Pokerus)
            value=current->pokerusMode()==0?"None":
                  current->pokerusMode()==1?"Cured":"Infected";
        else if(row.field==ScreenState::Field::OriginalTrainerGender)
            value=current->originalTrainerGender()==0?"Male":"Female";
        else if(row.field==ScreenState::Field::FatefulEncounter)
            value=current->fatefulEncounter()?"Yes":"No";
        else if(row.field==ScreenState::Field::Species)
            value=Names::getSpeciesName(current->species());
        else if(row.field==ScreenState::Field::Nickname ||
                row.field==ScreenState::Field::OriginalTrainer) {
            const auto name=PokeVault::Integration::Gen5::asciiName5(
                current->decodedBytes(),row.field==ScreenState::Field::Nickname);
            value=name?*name:"(native glyphs)";
        }
        fb.drawText(x+26,yy+11,row.label,Colors::TextSecondary,TextStyle::Body);
        if(row.field==ScreenState::Field::Nature ||
           row.field==ScreenState::Field::Move ||
           row.field==ScreenState::Field::MetLocation ||
           row.field==ScreenState::Field::TrainerId ||
           row.field==ScreenState::Field::SecretId ||
           row.field==ScreenState::Field::Level ||
           row.field==ScreenState::Field::Experience ||
           row.field==ScreenState::Field::Species ||
           row.field==ScreenState::Field::Nickname ||
           row.field==ScreenState::Field::OriginalTrainer ||
           row.field==ScreenState::Field::Ability ||
           row.field==ScreenState::Field::Form ||
           row.field==ScreenState::Field::Gender) {
            fb.drawText(x+315,yy+11,
                row.field==ScreenState::Field::TrainerId ||
                row.field==ScreenState::Field::SecretId ||
                row.field==ScreenState::Field::Level ||
                row.field==ScreenState::Field::Experience ||
                row.field==ScreenState::Field::Species ||
                row.field==ScreenState::Field::Nickname ||
                row.field==ScreenState::Field::OriginalTrainer?"A Edit":"A Pick",
                Colors::Accent,TextStyle::Body);
            fb.drawText(x+398,yy+11,value,Colors::Text,TextStyle::Body);
        } else {
            fb.drawText(x+315,yy+11,"-",Colors::Accent,TextStyle::Body);
            fb.drawText(x+355,yy+11,value,Colors::Text,TextStyle::Body);
            fb.drawText(x+429,yy+11,"+",Colors::Accent,TextStyle::Body);
        }
    }
    fb.drawText(x+22,y+h-33,
                "Gen V identity + encounter edits remain staged and reversible",
                Colors::Accent,TextStyle::Caption);
    drawNavBar(fb,{{"Up/Down","Field"},{"Left/Right","Adjust"},
                   {"A","Pick / Edit"},{"X","Keep Staged"},{"Y","Discard Draft"},{"B","Back"}});
}
// Guarded BW/B2W2 move picker inside the existing shared Trainer shell.
// A checked selection resets only this move's native PP and PP Ups; a
// picker's appearance never asserts that the species can learn the move.
void drawMovePicker(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                    const ScreenState& state) {
    dimShared(screen,fb);
    constexpr int w=840,h=560;
    const int x=(fb.getWidth()-w)/2;
    const int y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+22,y+16,"Generation V Move Picker",Colors::Text,TextStyle::Heading);
    fb.drawText(x+22,y+56,
        "BW / B2W2 moves 000-559; species learnability is not yet evaluated",
        Colors::Accent,TextStyle::Caption);
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        static_cast<size_t>(PokeVault::Integration::Gen5::MaxNativeMove5)+1,
        9,static_cast<size_t>(state.movePickerSelection()));
    for(size_t i=0;i<visible.count;++i) {
        const auto id=static_cast<uint16_t>(visible.first+i);
        const int yy=y+100+static_cast<int>(i)*43;
        if(id==state.movePickerSelection())
            fb.drawSelectionHighlight(x+14,yy,w-28,39);
        screen.touchButtons.push_back(
            {kGen5MovePickerTouchBase+static_cast<int>(id),x+15,yy,w-30,39});
        const auto name=id==0?"Empty move slot":Names::getMoveName(id);
        fb.drawText(x+28,yy+7,
            std::to_string(id)+" - "+std::string(name),
            Colors::Text,TextStyle::Body);
        fb.drawText(x+w-150,yy+12,
            "PP "+std::to_string(PokeVault::Integration::Gen5::basePP5(id)),
            Colors::TextSecondary,TextStyle::Caption);
    }
    fb.drawText(x+22,y+h-45,
        "Choosing a move resets only its own PP / PP Ups",
        Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"D-pad/Stick","Browse"},{"ZL/ZR","Jump 40"},
                   {"A","Choose Move"},{"B","Cancel"}});
}
// Native BW/B2W2 location picker. The list itself is format-valid input
// evidence, NOT proof that the specific Pokemon can originate at that site.
void drawLocationPicker(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                        const ScreenState& state) {
    dimShared(screen,fb);
    constexpr int w=860,h=560;
    const int x=(fb.getWidth()-w)/2;
    const int y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+22,y+14,"Generation V Encounter Location",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+22,y+56,
        "276 native location IDs; species encounter legality is not checked",
        Colors::Accent,TextStyle::Caption);
    const auto current=state.draft().current();
    const uint8_t origin=current?current->originVersion():20;
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        PokeVault::Integration::Gen5::NativeMetLocations5.size(),9,
        state.locationPickerIndex());
    for(size_t i=0;i<visible.count;++i) {
        const size_t index=visible.first+i;
        const uint16_t id=PokeVault::Integration::Gen5::NativeMetLocations5[index];
        const int yy=y+100+static_cast<int>(i)*43;
        if(index==state.locationPickerIndex())
            fb.drawSelectionHighlight(x+14,yy,w-28,39);
        screen.touchButtons.push_back(
            {kGen5LocationTouchBase+static_cast<int>(index),x+16,yy,w-32,39});
        const char* label=Names::getMetLocationName(origin,id);
        const std::string name=label && *label?label:"(source location ID)";
        fb.drawText(x+28,yy+7,std::to_string(id)+" - "+name,
                    Colors::Text,TextStyle::Body);
    }
    fb.drawText(x+22,y+h-46,
        "A selects a known Gen V ID; B preserves the original location",
        Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"D-pad/Stick","Browse"},{"ZL/ZR","Jump 40"},
                   {"A","Choose Location"},{"B","Cancel"}});
}
// Compact native Nature picker (25 Gen III+ indices), read-only until A.
void drawNaturePicker(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                      const ScreenState& state) {
    dimShared(screen,fb);
    constexpr int w=680,h=525;
    const int x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,16,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,16,Colors::Border,1);
    fb.drawText(x+24,y+16,"Generation V Nature",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+54,
        "Native 25-nature stat modifiers; changes stay in PK5 draft",
        Colors::Accent,TextStyle::Caption);
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        ScreenState::kNatureNames.size(),8,state.natureSelection());
    constexpr const char* stat[5]={"Atk","Def","Spe","SpA","SpD"};
    for(size_t i=0;i<visible.count;++i) {
        const uint32_t id=static_cast<uint32_t>(visible.first+i);
        const int yy=y+96+static_cast<int>(i)*44;
        if(id==state.natureSelection())
            fb.drawSelectionHighlight(x+12,yy,w-24,40);
        screen.touchButtons.push_back({
            kGen5NatureTouchBase+static_cast<int>(id),x+14,yy,w-28,40});
        const unsigned up=id/5,down=id%5;
        const std::string effect=up==down?"Neutral":
            std::string("+")+stat[up]+" / -"+stat[down];
        fb.drawText(x+28,yy+8,
            std::to_string(id)+" - "+std::string(ScreenState::natureName(id)),
            Colors::Text,TextStyle::Body);
        fb.drawText(x+w-204,yy+10,effect,
                    Colors::TextSecondary,TextStyle::Caption);
    }
    fb.drawText(x+24,y+h-39,
        "A stages a Nature change; B preserves the active PK5 draft",
        Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"D-pad/Stick","Browse"},{"A","Choose"},{"B","Cancel"}});
}
// Native Gen V named species browser. No legality inference and no
// mutation until an explicit confirmed pick reaches the guarded PK5 draft.
void drawSpeciesPicker(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                       const ScreenState& state) {
    dimShared(screen,fb);
    constexpr int w=870,h=570;
    const int x=(fb.getWidth()-w)/2;
    const int y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+24,y+15,"Gen V Species - National Dex",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+58,
        "Exact BW/B2W2 identity compatibility; move/encounter legality not checked",
        Colors::Accent,TextStyle::Caption);
    fb.drawText(x+24,y+81,
        "Search: "+(state.speciesSearch().empty()?std::string("All 649"):
                    std::string(state.speciesSearch()))+
        "   |   "+std::to_string(state.speciesChoiceCount())+" matches",
        Colors::TextSecondary,TextStyle::Caption);
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        state.speciesChoiceCount(),9,state.speciesChoiceIndex());
    for(size_t i=0;i<visible.count;++i) {
        const size_t index=visible.first+i;
        const uint16_t id=state.speciesChoiceId(index);
        const bool allowed=state.speciesChoiceAllowed(id);
        const int yy=y+112+static_cast<int>(i)*42;
        if(index==state.speciesChoiceIndex())
            fb.drawSelectionHighlight(x+14,yy,w-28,38);
        if(allowed)
            screen.touchButtons.push_back({
                kGen5SpeciesTouchBase+static_cast<int>(index),
                x+16,yy,w-32,38});
        fb.drawText(x+28,yy+8,
            std::to_string(id)+" - "+Names::getSpeciesName(id),
            allowed?Colors::Text:Colors::TextDim,TextStyle::Body);
        fb.drawText(x+w-160,yy+11,
            allowed?"Compatible":"Unavailable",
            allowed?Colors::Accent:Colors::TextDim,TextStyle::Caption);
    }
    if(state.speciesChoiceCount()==0)
        fb.drawText(x+26,y+180,"No matching Generation V species",
                    Colors::TextSecondary,TextStyle::Body);
    fb.drawText(x+24,y+h-61,
        "Rename first if unnicknamed; species changes never silently rewrite names",
        Colors::TextSecondary,TextStyle::Caption);
    fb.drawText(x+24,y+h-39,
        "Only compatible original base forms, growth groups, gender and abilities",
        Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"Up/Down","Browse"},{"ZL/ZR","Jump 40"},
                   {"X","Search"},{"A","Choose"},{"B","Cancel"}});
}
// Exact BW/B2W2 personal table; no encounter legality or original SAV write.
void drawIdentityPicker(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                        const ScreenState& state) {
    dimShared(screen,fb);
    constexpr int w=840,h=520;
    const int x=(fb.getWidth()-w)/2,y=(fb.getHeight()-kNavBarH-h)/2;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    const auto& field=ScreenState::kEditableRows[static_cast<size_t>(state.fieldRow())];
    const auto current=state.draft().current();
    if(!current)return;
    const auto personal=PokeVault::Integration::Gen5::personal5(
        current->species(),current->form(),state.black2White2());
    if(!personal)return;
    const auto f=field.field;
    const char* title=f==ScreenState::Field::Ability?"Native Ability":
                      f==ScreenState::Field::Form?"Native Form":"Native Gender";
    fb.drawText(x+24,y+18,std::string("Gen V - ")+title,
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+56,
        "Exact BW/B2W2 personal data; legality and encounters separate",
        Colors::Accent,TextStyle::Caption);
    const auto visible=PokeBank::UIModel::SharedPokemonEditor::scrollWindow(
        state.identityChoiceCount(),8,static_cast<size_t>(state.identitySelection()));
    for(size_t i=0;i<visible.count;++i) {
        const uint32_t value=static_cast<uint32_t>(visible.first+i);
        const int yy=y+100+static_cast<int>(i)*43;
        const bool available=state.identityChoiceAllowed(value);
        if(value==state.identitySelection())
            fb.drawSelectionHighlight(x+14,yy,w-28,39);
        if(available)screen.touchButtons.push_back({
            kGen5IdentityTouchBase+static_cast<int>(value),
            x+16,yy,w-32,39});
        std::string label;
        if(f==ScreenState::Field::Ability)
            label=std::string(value==2?"Hidden ability":"Ability slot ")+
                  (value==2?"":std::to_string(value+1))+
                  " / ID "+std::to_string(
                      PokeVault::Integration::Gen5::abilityForSlot5(*personal.info,value));
        else if(f==ScreenState::Field::Form)
            label="Form "+std::to_string(value);
        else label=value==0?"Male":value==1?"Female":"Genderless";
        if(!available)label+=" (unavailable)";
        fb.drawText(x+30,yy+8,label,
            available?Colors::Text:Colors::TextDim,TextStyle::Body);
    }
    fb.drawText(x+24,y+h-45,
        "A stages in memory; B preserves the current PK5 draft",
        Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"Up/Down","Choose"},{"A","Stage Choice"},{"B","Cancel"}});
}
bool draw(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    if(!isGen5Source(screen))return false;
    auto& trainer=nativeTrainer(screen);
    auto& state=trainer.sharedScreen();
    const auto& staged=trainer.stagedPokemon();
    // The inherited modal unconditionally invokes an unsupported Gen V
    // legality analyzer; the shared Gen V adapter never opens that modal.
    screen.details.active=false;
    switch(state.surface()) {
        case Surface::Actions:drawActions(screen,fb,state,staged);return true;
        case Surface::CloneDestination:
            drawCloneDestination(screen,fb,state,staged);return true;
        case Surface::Review:drawReview(screen,fb,state,staged);return true;
        case Surface::Edit:drawEdit(screen,fb,state);return true;
        case Surface::MovePicker:drawMovePicker(screen,fb,state);return true;
        case Surface::LocationPicker:drawLocationPicker(screen,fb,state);return true;
        case Surface::IdentityPicker:drawIdentityPicker(screen,fb,state);return true;
        case Surface::SpeciesPicker:drawSpeciesPicker(screen,fb,state);return true;
        case Surface::NaturePicker:drawNaturePicker(screen,fb,state);return true;
        case Surface::View: {
            const auto current=state.draft().current();
            screen.drawGSCOverlay(fb);
            if(current) {
                Pokemon::Pokemon5ReadOnlyView view(*current,screen.trainer.getGameGroup());
                drawVerifiedSummary(screen,fb,&view);
            }
            drawNavBar(fb,{{"B","Back"},{"","Verified PK5 View"}});
            return true;
        }
        case Surface::ConfirmRemoveBox:
            drawDecision(screen,fb,"Remove Boxed Pokemon From Staged Copy?",
                "Requires Y confirmation; source SAV5 and backups stay intact.",
                {{"Y","Stage Remove"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmDraft:
            drawDecision(screen,fb,"Unsaved PK5 Draft",
                "Choose Keep Staged, Discard Draft, or Continue Editing.",
                {{"A","Keep Staged"},{"Y","Discard Draft"},{"B","Keep Editing"}});
            return true;
        case Surface::ConfirmExport:
            drawDecision(screen,fb,"Export Verified Generation V Working Copy?",
                "Creates an untouched backup + edited SAV5 under PokeBank NX/exports/gen5.",
                {{"Y","Export Copy"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmDiscardAll:
            drawDecision(screen,fb,"Discard All Staged Changes?",
                "Y permanently clears app-memory edits; SAV5 stays unchanged.",
                {{"Y","Confirm Discard"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmExit:
            drawDecision(screen,fb,"Staged Changes Remain In Memory",
                "Exit discards in-memory edits. Exported working copies remain safe.",
                {{"Y","Discard & Exit"},{"X","Review"},{"B","Continue"}});
            return true;
        case Surface::Browse:break;
    }
    screen.drawGSCOverlay(fb);
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Party) {
        // Match the established two-column Party panel hit geometry, but
        // route through Gen V's own guarded Actions instead of the base sheet.
        constexpr int x=LEFT_PANEL_X,y=CONTENT_PANEL_Y;
        constexpr int gutter=16,slotGap=12;
        const int width=fb.getWidth()-x;
        const int top=y+58;
        const int colW=(width-3*gutter)/2;
        const int slotH=(CONTENT_PANEL_HEIGHT-(top-y)-2*slotGap-gutter)/3;
        const int size=static_cast<int>(screen.trainer.getPartySize());
        for(int i=0;i<std::min(size,6);++i) {
            const int column=i>=3?1:0;
            const int row=i>=3?i-3:i;
            screen.touchButtons.push_back({
                kGen5PartyTouchBase+i,
                x+gutter+column*(colW+gutter),
                top+row*(slotH+slotGap),colW,slotH});
        }
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes)
        drawNavBar(fb,{{"D-pad/Stick","Navigate"},{"L/R","Box"},
                       {"A","Actions"},{"X","Party"},{"Y","Trainer"},{"B","Exit"}});
    else if(screen.selectedMode==TrainerViewScreen::ViewMode::Trainer)
        drawNavBar(fb,{{"X","Party"},{"B","Exit"},{"","Gen V / Read-only Trainer"}});
    else
        drawNavBar(fb,{{"D-pad/Stick","Navigate"},{"A","Actions"},
                       {"X","Boxes"},{"Y","Trainer"},{"B","Exit"}});
    return true;
}
} // namespace UI::Gen5SharedReadOnlySurface
