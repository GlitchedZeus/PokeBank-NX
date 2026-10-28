#include "UI/ClassicPartyActions.h"
#include "UI/ClassicGenericUiGuard.h"
#include "UI/SharedPopupDismiss.h"
#include "UI/PokemonFileMenus.h"

// One Party sheet. The existing generation surfaces own the View / Edit /
// Origin sessions; their explicit Party target selects the native backend.
namespace UI::ClassicPartyActions {
namespace {
int generation(const TrainerViewScreen& screen) {
    return PokeBank::UIModel::ClassicGenericUi::classicGeneration(screen.sourceGameId);
}
bool active(const TrainerViewScreen& screen) {
    switch(generation(screen)) {
        case 1: {const auto& s=Gen1PokemonEditor::ux2StateFor(screen);
            return s.party && s.mode==Gen1PokemonEditor::UX2Mode::Actions;}
        case 2: {const auto& s=editorOverlayState(screen);
            return s.pokemonParty && s.active && s.pokemonActions && !s.review;}
        case 3: {const auto& s=Gen3SharedEditorSurface::stateFor(screen);
            return s.party && s.actions;}
        default:return false;
    }
}
int& row(TrainerViewScreen& screen) {
    if(generation(screen)==1)return Gen1PokemonEditor::ux2StateFor(screen).row;
    if(generation(screen)==2)return editorOverlayState(screen).pokemonRow;
    return Gen3SharedEditorSurface::stateFor(screen).actionRow;
}
auto actions() {
    namespace Shared=PokeBank::UIModel::SharedPokemonEditor;
    // The sheet only opens with a staged editor (open()), so Feature A's Export to file joins it.
    return Shared::withAction(Shared::actionsForSlot(true,{false,false,true},false),Shared::Action::ExportFile);
}
int partySlot(TrainerViewScreen& screen) {
    switch(generation(screen)) {
        case 1:return Gen1PokemonEditor::ux2StateFor(screen).slot;
        case 2:return editorOverlayState(screen).pokemonSlot;
        case 3:return Gen3SharedEditorSurface::stateFor(screen).slot;
        default:return -1;
    }
}
void close(TrainerViewScreen& screen) {
    switch(generation(screen)) {
        case 1:Gen1PokemonEditor::ux2Close(Gen1PokemonEditor::ux2StateFor(screen));break;
        case 2:{auto& s=editorOverlayState(screen);s.active=s.pokemonActions=false;break;}
        case 3:Gen3SharedEditorSurface::stateFor(screen).actions=false;break;
    }
    screen.controllerNavigation.reset();
}
}
bool open(TrainerViewScreen& screen,int slot) {
    if(slot<0 || slot>=static_cast<int>(screen.trainer.party.size()))return false;
    std::string error;
    switch(generation(screen)) {
        case 1: {
            auto* editor=Gen1PokemonEditor::realEditor(screen);
            if(!editor || !editor->partyPokemon(static_cast<size_t>(slot),error))return false;
            auto& s=Gen1PokemonEditor::ux2StateFor(screen);
            Gen1PokemonEditor::ux2ResetTouchMotion(s);
            s.party=true;s.box=0;s.slot=slot;s.row=0;
            s.mode=Gen1PokemonEditor::UX2Mode::Actions;
            s.picker=Gen1PokemonEditor::UX2Picker::None;
            break;
        }
        case 2: {
            auto* editor=stagedEditor(screen);
            if(!editor || !editor->partyPokemon(static_cast<size_t>(slot),error))return false;
            auto& s=editorOverlayState(screen);
            s.active=true;s.review=false;s.pokemonActions=true;s.pokemonFields=false;
            s.pokemonParty=true;s.pokemonSlot=slot;s.pokemonBox=0;s.pokemonRow=0;
            break;
        }
        case 3: {
            auto* editor=Gen3SharedEditorSurface::editor(screen);
            if(!editor || !editor->partyPokemon(static_cast<size_t>(slot),error))return false;
            Gen3SharedEditorSurface::beginActions(screen);
            auto& s=Gen3SharedEditorSurface::stateFor(screen);
            s.party=true;s.box=0;s.slot=slot;
            break;
        }
        default:return false;
    }
    screen.selectedPartyIndex=slot;
    screen.controllerNavigation.reset();
    return true;
}
bool handle(TrainerViewScreen& screen,uint64_t down,uint64_t held,int stickX,int stickY,const TouchInput& touch) {
    if(!active(screen))return false;
    // 7D: a completed tap outside the actions sheet closes it like B; the return consumes it.
    if(SharedPopupDismiss::tappedOutside(touch)){close(screen);return true;}
    auto& selected=row(screen);
    const auto list=actions();
    const uint64_t nav=screen.controllerNavigation.apply(down,held,stickX,stickY,
        HidNpadButton_Up,HidNpadButton_Down,HidNpadButton_Left,HidNpadButton_Right);
    if(nav&HidNpadButton_Up)selected=(selected+static_cast<int>(list.count)-1)%static_cast<int>(list.count);
    if(nav&HidNpadButton_Down)selected=(selected+1)%static_cast<int>(list.count);
    const int tapped=screen.touchedButtonId(touch);
    if(tapped>=89000 && tapped<89000+static_cast<int>(list.count)) {selected=tapped-89000;down|=HidNpadButton_A;}
    selected=std::clamp(selected,0,static_cast<int>(list.count)-1);
    if(down&HidNpadButton_B) {close(screen);return true;}
    if(!(down&HidNpadButton_A))return true;
    using Action=PokeBank::UIModel::SharedPokemonEditor::Action;
    const auto action=list[static_cast<size_t>(selected)];
    if(action==Action::Close){close(screen);return true;}
    if(action==Action::ExportFile) {
        const int gen=generation(screen),slot=partySlot(screen);
        close(screen);
        PokemonFileMenus::exportPokemon(screen,gen,true,0,slot);
        return true;
    }
    switch(generation(screen)) {
        case 1: {
            auto& s=Gen1PokemonEditor::ux2StateFor(screen);
            s.mode=action==Action::View?Gen1PokemonEditor::UX2Mode::View:
                   action==Action::Edit?Gen1PokemonEditor::UX2Mode::Edit:Gen1PokemonEditor::UX2Mode::Provenance;
            s.row=0;break;
        }
        case 2: {
            auto& legacy=editorOverlayState(screen);
            if(action==Action::LegalityProvenance) {
                Gen2PokemonEditor::sharedSurfaceStateFor(screen).provenanceActive=true;
                legacy.pokemonActions=false;
            } else if(Gen2PokemonEditor::beginEdit(screen,0,legacy.pokemonSlot,true)) {
                auto& session=Gen2PokemonEditor::stateFor(screen);
                if(action==Action::View)session.mode=Gen2PokemonEditor::Mode::View;
                Gen2PokemonEditor::dismissActionSheetForWorkspace(screen);
            }
            break;
        }
        case 3: {
            auto& s=Gen3SharedEditorSurface::stateFor(screen);
            if(action==Action::LegalityProvenance){s.actions=false;s.legality=true;}
            else Gen3SharedEditorSurface::beginViewOrEdit(screen,action==Action::View?
                PokeBank::UIModel::Gen3SharedEditor::Mode::View:PokeBank::UIModel::Gen3SharedEditor::Mode::Edit);
            break;
        }
    }
    screen.controllerNavigation.reset();
    return true;
}
bool draw(TrainerViewScreen& screen,PKSEFramebuffer& fb) {
    if(!active(screen))return false;
    screen.drawLegacyBase(fb);
    fb.drawFilledRect(0,0,fb.getWidth(),fb.getHeight()-48,Color(0,0,0,105));
    const auto model=PokeBank::UIModel::NarrowPokemonMenu::classic(actions());
    const auto* pokemon=screen.selectedPartyIndex>=0 && screen.selectedPartyIndex<static_cast<int>(screen.trainer.party.size())?
        screen.trainer.party[static_cast<size_t>(screen.selectedPartyIndex)].get():nullptr;
    drawNarrowPokemonActions(fb,screen.touchButtons,model,row(screen),
        pokemon?Names::getSpeciesName(pokemon->speciesID()):"Party Pokémon",
        "PARTY / SOURCE SAVE IMMUTABLE",89000);
    {   // 7D: the sheet's frame, so a tap outside it closes it
        const auto g=PokeBank::UIModel::NarrowPokemonMenu::geometry(model.count,fb.getWidth(),fb.getHeight(),
            SharedPopupDismiss::kNavBarHeight);
        SharedPopupDismiss::publish(g.x,g.y,g.width,g.height);
    }
    SharedEditorChrome::drawPickerFooter(fb,{true,false});
    return true;
}
} // namespace UI::ClassicPartyActions
