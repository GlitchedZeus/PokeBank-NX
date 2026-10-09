// Generation V's guarded entry to the EXISTING Trainer/Party/Boxes renderer.
// The shared PK5 draft editor is not production-wired yet. Until then this
// layer owns EVERY Gen V controller frame: inherited Storage/Bank, Items,
// source-save dialogs, creation and generic mutation handlers are unreachable.
// Touch remains inert in this isolated lane until its separate safety review.
#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"

namespace UI::Gen5SharedReadOnlySurface {
namespace {
bool isGen5Source(const TrainerViewScreen& screen) noexcept {
    // Deliberately check the exact source ID, not the Trainer's claimed game
    // group: even a broken adapter cannot fall back to generic mutators.
    return PokeVault::Integration::Gen5EditorProvider::isGen5NdsId(screen.sourceGameId);
}
void inspectSelected(TrainerViewScreen& screen) {
    auto& details=screen.details;
    Pokemon::Pokemon* selected=nullptr;
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Party) {
        const int n=screen.selectedPartyIndex;
        if(n>=0 && n<static_cast<int>(screen.trainer.party.size()))
            selected=screen.trainer.party[static_cast<size_t>(n)].get();
        details.source=TrainerViewScreen::EditSource::Party;
        details.partyIndex=n;
    } else if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes) {
        const int box=screen.selectedBoxIndex;
        const int slot=screen.selectedItemIndex;
        if(box>=0 && box<static_cast<int>(screen.trainer.boxes.size()) &&
           slot>=0 && slot<static_cast<int>(screen.trainer.getSlotsPerBox()))
            selected=screen.trainer.boxes[static_cast<size_t>(box)][static_cast<size_t>(slot)].get();
        details.source=TrainerViewScreen::EditSource::Box;
    }
    if(!selected || selected->speciesID()==0) {
        screen.postStatus("No validated PK5 in this slot.",180);
        return;
    }
    // Passive inspection stays on the existing shared screen, but its
    // Gen V summary must NOT invoke the older generic legality evaluator:
    // unsupported encounter histories cannot be classified here.
    // No editor action, snapshot, picker or save confirmation is opened.
    details.active=true;
    details.readOnly=true;
    details.editing=false;
    details.leftScroll=0;
    details.category=0;
    details.selectedField=0;
    details.selectedStat=0;
    details.legalityOverlay=false;
    details.ribbonOverlay=false;
    details.discardConfirmActive=false;
    details.editSnapshot.clear();
    screen.controllerNavigation.reset();
}
} // namespace

bool handleInput(TrainerViewScreen& screen,u64 down,u64 held,
                 int stickX,int stickY,const TouchInput& touch) {
    (void)touch;
    if(!isGen5Source(screen))return false;

    // The native PK5 Trainer must match its advertised BW/B2W2 family.
    // A mismatched bridge is not allowed to escape into base UI handlers.
    const auto family=screen.trainer.getGameGroup();
    if(family!=Enums::GameVersion::BW && family!=Enums::GameVersion::B2W2) {
        screen.postStatus("Generation V source presentation mismatch",180);
        if(down&HidNpadButton_B)screen.goBack=true;
        return true;
    }
    if(screen.storageStatusFrames>0)--screen.storageStatusFrames;
    const u64 keys=screen.controllerNavigation.apply(
        down,held,stickX,stickY,
        HidNpadButton_Up,HidNpadButton_Down,
        HidNpadButton_Left,HidNpadButton_Right);
    if(screen.details.active) {
        if(keys&HidNpadButton_B) {
            screen.closeDetailsModal();
            screen.controllerNavigation.reset();
        }
        return true;
    }
    screen.detailViewActive=true;
    if(keys&HidNpadButton_B) {
        screen.goBack=true;
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
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Trainer) {
        // No trainer renaming, money editing, source-save or bank commands.
        return true;
    }
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Party) {
        const int count=static_cast<int>(screen.trainer.getPartySize());
        if(count>0) {
            if(keys&(HidNpadButton_Down|HidNpadButton_Right))
                screen.selectedPartyIndex=(screen.selectedPartyIndex+1)%count;
            if(keys&(HidNpadButton_Up|HidNpadButton_Left))
                screen.selectedPartyIndex=(screen.selectedPartyIndex+count-1)%count;
            if(keys&HidNpadButton_A)inspectSelected(screen);
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
        if(keys&HidNpadButton_A)inspectSelected(screen);
    }
    // Every other input is deliberately swallowed. No generic Edit/Storage,
    // trainer mutations, X-to-Add, Y-to-Move, Save or Inject reaches the base.
    return true;
}

// A narrow passive PK5 panel within the shared Trainer screen. It displays
// only verified record fields; unlike the generic details modal it NEVER
// calls Legality::analyze or synthesizes unknown history/stat calculations.
void drawVerifiedSummary(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    const Pokemon::Pokemon* p=screen.detailsTargetPokemon();
    if(!p)return;
    const int width=fb.getWidth(),height=fb.getHeight();
    fb.drawFilledRect(0,0,width,height-kNavBarH,Color(0,0,0,165));
    const int x=180,y=65,w=width-360,h=height-kNavBarH-125;
    fb.drawFilledRoundedRect(x,y,w,h,18,Colors::Panel);
    fb.drawRoundedRect(x,y,w,h,18,Colors::Border,1);
    fb.drawText(x+26,y+22,"Generation V - Verified PK5 Summary",
                Colors::Text,TextStyle::Heading);
    fb.drawText(x+26,y+60,"Passive View - source SAV5 unchanged - legality unresolved",
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

bool draw(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    if(!isGen5Source(screen))return false;
    if(screen.details.active) {
        // Render the exact shared Party/Boxes frame without running the
        // generic modal (which always analyzes legality, even for View).
        screen.details.active=false;
        screen.drawGSCOverlay(fb);
        screen.details.active=true;
        drawVerifiedSummary(screen,fb);
        drawNavBar(fb,{{"B","Back"},{"","Read-only PK5"}});
        return true;
    }
    screen.drawGSCOverlay(fb);
    if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes)
        drawNavBar(fb,{{"D-pad/Stick","Navigate"},{"L/R","Box"},
                       {"A","View"},{"X","Party"},{"Y","Trainer"},{"B","Exit"}});
    else if(screen.selectedMode==TrainerViewScreen::ViewMode::Trainer)
        drawNavBar(fb,{{"X","Party"},{"B","Exit"},{"","Gen V / Read-only"}});
    else
        drawNavBar(fb,{{"D-pad/Stick","Navigate"},{"A","View"},
                       {"X","Boxes"},{"Y","Trainer"},{"B","Exit"}});
    return true;
}
} // namespace UI::Gen5SharedReadOnlySurface
