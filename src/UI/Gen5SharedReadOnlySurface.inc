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
            if(alreadyFocused && ScreenState::kEditableRows[static_cast<size_t>(chosen)].field==
                                    ScreenState::Field::Move)
                down|=HidNpadButton_A;
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
                } else keepDraft(screen);
            } else if(keys&HidNpadButton_Y) {
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
    std::string name;
    switch(c.field) {
        case Field::Nature:name="Nature";break;
        case Field::Friendship:name="Friendship";break;
        case Field::IV:name=std::string("IV ")+reviewedStatName(c.stat);break;
        case Field::EV:name=std::string("EV ")+reviewedStatName(c.stat);break;
        case Field::HeldItem:name="Held Item";break;
        case Field::Ball:name="Ball";break;
        case Field::MetLevel:name="Met Level";break;
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
    fb.drawText(x+24,y+h-31,"No Create, Clone, source writes or inferred legality",
                Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"L/R","Review Pages"},{"X","Export Working Copy"},
                   {"Y","Discard All..."},{"B","Back"}});
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
        if(row.field!=ScreenState::Field::Move) {
            screen.touchButtons.push_back(
                {kGen5DecreaseTouchBase+static_cast<int>(index),x+308,yy+3,36,40});
            screen.touchButtons.push_back(
                {kGen5IncreaseTouchBase+static_cast<int>(index),x+418,yy+3,36,40});
        }
        const std::string value=std::to_string(ScreenState::fieldValue(*current,row));
        fb.drawText(x+26,yy+11,row.label,Colors::TextSecondary,TextStyle::Body);
        if(row.field==ScreenState::Field::Move) {
            fb.drawText(x+315,yy+11,"A Pick",Colors::Accent,TextStyle::Body);
            fb.drawText(x+398,yy+11,value,Colors::Text,TextStyle::Body);
        } else {
            fb.drawText(x+315,yy+11,"-",Colors::Accent,TextStyle::Body);
            fb.drawText(x+355,yy+11,value,Colors::Text,TextStyle::Body);
            fb.drawText(x+429,yy+11,"+",Colors::Accent,TextStyle::Body);
        }
    }
    fb.drawText(x+22,y+h-33,
                "Moves & PP: A picks moves / D-pad edits other fields",
                Colors::Accent,TextStyle::Caption);
    drawNavBar(fb,{{"Up/Down","Field"},{"Left/Right","Adjust"},
                   {"A","Pick Move / Keep"},{"Y","Discard Draft"},{"B","Back"}});
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
        case Surface::Review:drawReview(screen,fb,state,staged);return true;
        case Surface::Edit:drawEdit(screen,fb,state);return true;
        case Surface::MovePicker:drawMovePicker(screen,fb,state);return true;
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
