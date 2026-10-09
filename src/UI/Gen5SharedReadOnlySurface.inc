// Generation V's guarded entry to the EXISTING Trainer/Party/Boxes renderer.
// The shared PK5 draft editor is not production-wired yet. Until then this
// layer owns EVERY Gen V controller frame: inherited Storage/Bank, Items,
// source-save dialogs, creation and generic mutation handlers are unreachable.
// Touch remains inert in this isolated lane until its separate safety review.
#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Pokemon/Pokemon5ReadOnlyView.h"

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
    auto before=trainer.stagedPokemon();
    std::string error;
    if(!trainer.sharedScreen().keep(trainer.stagedPokemon(),error)) {
        screen.postStatus(error.empty()?"Gen V Keep was rejected":error,360);
        return;
    }
    if(!trainer.refreshStagedPokemonPresentation(error)) {
        trainer.stagedPokemon()=std::move(before);
        std::string restored;
        (void)trainer.refreshStagedPokemonPresentation(restored);
        screen.postStatus("Staged PK5 presentation failed; Keep rolled back: "+error,360);
        return;
    }
    screen.postStatus("PK5 draft kept in app memory only; emulator save unchanged",300);
    screen.controllerNavigation.reset();
}
} // namespace

bool handleInput(TrainerViewScreen& screen,u64 down,u64 held,
                 int stickX,int stickY,const TouchInput& touch) {
    (void)touch;
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
            if(keys&HidNpadButton_A)keepDraft(screen);
            else if(keys&HidNpadButton_Y) {
                state.discardDraft();
                screen.postStatus("Local Gen V draft discarded; staged edits preserved",220);
            } else if(keys&HidNpadButton_B)state.back();
            return true;
        case Surface::ConfirmDraft:
            // A may Keep, but never destroys a staged draft or external source.
            if(keys&HidNpadButton_A)keepDraft(screen);
            else if(keys&HidNpadButton_Y)state.discardDraft();
            else if(keys&HidNpadButton_B)state.continueEditing();
            return true;
        case Surface::Review:
            if(keys&HidNpadButton_B)state.back();
            else if(keys&HidNpadButton_Y)state.requestDiscardAll();
            return true;
        case Surface::ConfirmDiscardAll:
            if(keys&HidNpadButton_B)state.back();
            else if(keys&HidNpadButton_Y) {
                auto before=staged;
                if(state.confirmDiscardAll(staged)) {
                    std::string error;
                    if(!trainer.refreshStagedPokemonPresentation(error)) {
                        staged=std::move(before);
                        std::string restored;
                        (void)trainer.refreshStagedPokemonPresentation(restored);
                        screen.postStatus("Discard refresh rejected; previous stage restored: "+error,360);
                    } else
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
            "Move "+std::to_string(i+1)+": #"+std::to_string(p->move(i))+
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
        fb.drawText(x+38,yy+9,
            PokeBank::UIModel::SharedPokemonEditor::actionLabel(choices[i]),
            static_cast<int>(i)==state.actionRow()?Colors::Text:Colors::TextSecondary,
            TextStyle::Body);
    }
    fb.drawText(x+26,y+h-30,
        "View / staged 4-field Edit only; no source SAV5 writes",
        Colors::Accent,TextStyle::Caption);
    drawNavBar(fb,{{"D-pad/Stick","Choose"},{"A","Select"},{"B","Cancel"}});
}
void drawReview(TrainerViewScreen& screen,PKSEFramebuffer& fb,
                const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace& ws) {
    dimShared(screen,fb);
    constexpr int x=150,y=70,w=980;
    fb.drawFilledRoundedRect(x,y,w,500,18,Colors::Panel);
    fb.drawText(x+24,y+18,"Review Pending Changes - Generation V",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+24,y+58,"App-memory PK5 edits only; original emulator source is immutable",
                Colors::Accent,TextStyle::Caption);
    const auto pending=ws.pendingReview();
    fb.drawText(x+24,y+112,std::to_string(pending.size())+
                " validated record(s) changed",Colors::Text,TextStyle::Body);
    for(size_t i=0;i<pending.size() && i<7;++i) {
        const auto& loc=pending[i].location;
        const std::string label=loc.region==Region::Party
            ? "Party slot "+std::to_string(loc.slot+1)
            : "Box "+std::to_string(loc.box+1)+" / Slot "+std::to_string(loc.slot+1);
        fb.drawText(x+32,y+157+static_cast<int>(i)*42,label,
                    Colors::TextSecondary,TextStyle::Body);
    }
    if(pending.size()>7)
        fb.drawText(x+32,y+458,"Additional staged slots: "+
                    std::to_string(pending.size()-7),Colors::TextDim,TextStyle::Caption);
    drawNavBar(fb,{{"B","Back"},{"Y","Request Discard All"}});
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
        const std::string value=std::to_string(ScreenState::fieldValue(*current,row));
        fb.drawText(x+26,yy+11,row.label,Colors::TextSecondary,TextStyle::Body);
        fb.drawText(x+355,yy+11,value,Colors::Text,TextStyle::Body);
    }
    fb.drawText(x+22,y+h-33,"Nature / Friendship / IVs / EVs ONLY",
                Colors::Accent,TextStyle::Caption);
    drawNavBar(fb,{{"Up/Down","Field"},{"Left/Right","Adjust"},
                   {"A","Keep Staged"},{"Y","Discard Draft"},{"B","Back"}});
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
        case Surface::Review:drawReview(screen,fb,staged);return true;
        case Surface::Edit:drawEdit(screen,fb,state);return true;
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
        case Surface::ConfirmDiscardAll:
            drawDecision(screen,fb,"Discard All Staged Changes?",
                "Y permanently clears app-memory edits; SAV5 stays unchanged.",
                {{"Y","Confirm Discard"},{"B","Cancel"}});
            return true;
        case Surface::ConfirmExit:
            drawDecision(screen,fb,"Staged Changes Are Not Saved",
                "Exit requires discarding app-memory PK5 changes. Review or continue.",
                {{"Y","Discard & Exit"},{"X","Review"},{"B","Continue"}});
            return true;
        case Surface::Browse:break;
    }
    screen.drawGSCOverlay(fb);
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
