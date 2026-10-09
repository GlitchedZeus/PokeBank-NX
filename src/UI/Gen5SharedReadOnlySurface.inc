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
    // Reuse the shared passive Pokemon-details renderer. No editor action,
    // snapshot, picker, source mutator or save confirmation is ever opened.
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

bool draw(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    if(!isGen5Source(screen))return false;
    screen.drawGSCOverlay(fb); // exact shared screen, not a second renderer
    if(screen.details.active)
        drawNavBar(fb,{{"B","Back"},{"","Gen V / Read-only View"}});
    else if(screen.selectedMode==TrainerViewScreen::ViewMode::Boxes)
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
