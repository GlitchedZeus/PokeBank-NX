// Execute the actual shared Party coordinator. Native format/export tests run
// separately; the peers here observe which generation session owns each input.
#include "UI/SharedPokemonEditorContract.h"
#include "UI/NarrowPokemonActionMenu.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <iostream>
constexpr uint64_t HidNpadButton_A=1,HidNpadButton_B=2,HidNpadButton_Up=4,
    HidNpadButton_Down=8,HidNpadButton_Left=16,HidNpadButton_Right=32;
namespace Names {const char* getSpeciesName(uint16_t){return "Pikachu";}}
namespace UI {
struct Color {Color(int,int,int,int){}};
class PKSEFramebuffer {
public:int dims=0;int getWidth()const{return 1280;}int getHeight()const{return 720;}
    void drawFilledRect(int,int,int,int,Color){++dims;}
};
struct TouchInput {
    int completedTarget=-1;
    // 7D: a completed tap (start and release points) for SharedPopupDismiss.
    bool tapped=false;int sx=0,sy=0,ex=0,ey=0;
    bool justTapped()const{return tapped;}
    int startX()const{return sx;}int startY()const{return sy;}int x()const{return ex;}int y()const{return ey;}
};
struct Navigation {
    int resets=0;void reset(){++resets;}
    uint64_t apply(uint64_t down,uint64_t,int,int,uint64_t,uint64_t,uint64_t,uint64_t){return down;}
};
struct Pokemon {uint16_t speciesID()const{return 25;}};
struct Trainer {std::vector<std::unique_ptr<Pokemon>> party;};
class TrainerViewScreen {
public:std::string sourceGameId;Trainer trainer;int selectedPartyIndex=0;Navigation controllerNavigation;
    std::vector<int> touchButtons;int backgrounds=0;
    int touchedButtonId(const TouchInput& touch)const{return touch.completedTarget;}
    void drawLegacyBase(PKSEFramebuffer&){++backgrounds;}
};
struct MockEditor {
    bool available=true;
    std::optional<int> partyPokemon(size_t slot,std::string& error){error.clear();return slot==0?std::optional<int>{25}:std::nullopt;}
} native;
namespace Gen1PokemonEditor {
enum class UX2Mode {Closed,Actions,View,Edit,Provenance};enum class UX2Picker {None};
struct State {bool party=false;int box=0,slot=0,row=0;UX2Mode mode=UX2Mode::Closed;UX2Picker picker=UX2Picker::None;} state;
State& ux2StateFor(const TrainerViewScreen&){return state;}
MockEditor* realEditor(TrainerViewScreen&){return native.available?&native:nullptr;}
void ux2ResetTouchMotion(State&){}void ux2Close(State& s){s.mode=UX2Mode::Closed;}
}
struct Legacy {bool pokemonParty=false,active=false,pokemonActions=false,review=false,pokemonFields=false;
    int pokemonSlot=0,pokemonBox=0,pokemonRow=0;} legacy;
Legacy& editorOverlayState(const TrainerViewScreen&){return legacy;}
MockEditor* stagedEditor(TrainerViewScreen&){return native.available?&native:nullptr;}
namespace Gen2PokemonEditor {
enum class Mode {None,Edit,View};
struct State {Mode mode=Mode::None;bool party=false;} state;
struct Shared {bool provenanceActive=false;} shared;
State& stateFor(const TrainerViewScreen&){return state;}
Shared& sharedSurfaceStateFor(const TrainerViewScreen&){return shared;}
bool beginEdit(TrainerViewScreen&,int,int slot,bool party){
    if(slot!=0)return false;
    state.party=party;state.mode=Mode::Edit;return true;
}
void dismissActionSheetForWorkspace(TrainerViewScreen&){legacy.active=legacy.pokemonActions=false;}
}
}
namespace PokeBank::UIModel::Gen3SharedEditor {enum class Mode {View,Edit};}
namespace UI::Gen3SharedEditorSurface {
struct State {bool party=false,actions=false,legality=false;int box=0,slot=0,actionRow=0;
    std::optional<PokeBank::UIModel::Gen3SharedEditor::Mode> mode;} state;
State& stateFor(const TrainerViewScreen&){return state;}
MockEditor* editor(TrainerViewScreen&){return native.available?&native:nullptr;}
void beginActions(TrainerViewScreen&){state.actions=true;}
bool beginViewOrEdit(TrainerViewScreen&,PokeBank::UIModel::Gen3SharedEditor::Mode mode){
    state.actions=false;state.mode=mode;return true;
}
}
namespace UI {
PokeBank::UIModel::NarrowPokemonMenu::Model drawn;
void drawNarrowPokemonActions(PKSEFramebuffer&,std::vector<int>& buttons,
    const PokeBank::UIModel::NarrowPokemonMenu::Model& model,int,const std::string&,const std::string&,int base) {
    drawn=model;buttons.clear();for(int i=0;i<model.count;++i)buttons.push_back(base+model.rows[i].nativeIndex);
}
namespace SharedEditorChrome {void drawPickerFooter(PKSEFramebuffer&,std::pair<bool,bool>) {}}
}
#include "../src/UI/ClassicPartyActions.inc"

int main() {
    using namespace UI;
    for(const auto* game:{"red_gb","gold_gbc","firered_gba"}) {
        TrainerViewScreen screen;screen.sourceGameId=game;screen.trainer.party.push_back(std::make_unique<Pokemon>());
        const int gen=PokeBank::UIModel::ClassicGenericUi::classicGeneration(game);
        const auto reset=[&]{Gen1PokemonEditor::state={};Gen2PokemonEditor::state={};Gen2PokemonEditor::shared={};
            legacy={};Gen3SharedEditorSurface::state={};native.available=true;};
        const auto mode=[&]{return gen==1?static_cast<int>(Gen1PokemonEditor::state.mode):
            gen==2?static_cast<int>(Gen2PokemonEditor::state.mode):
            Gen3SharedEditorSurface::state.mode?static_cast<int>(*Gen3SharedEditorSurface::state.mode):-1;};
        for(int action=0;action<4;++action)for(bool touch:{false,true}) {
            reset();assert(ClassicPartyActions::open(screen,0));
            PKSEFramebuffer fb;screen.touchButtons={-999};assert(ClassicPartyActions::draw(screen,fb));
            assert(fb.dims==1 && drawn.count==4 && screen.touchButtons.size()==4);
            assert(std::string(drawn.rows[0].label)=="View" && std::string(drawn.rows[1].label)=="Edit");
            assert(std::string(drawn.rows[2].label)=="Origin & Legality" && std::string(drawn.rows[3].label)=="Cancel");
            if(!touch)for(int i=0;i<action;++i)assert(ClassicPartyActions::handle(screen,HidNpadButton_Down,0,0,0,{}));
            assert(ClassicPartyActions::handle(screen,touch?0:HidNpadButton_A,0,0,0,{touch?89000+action:-1}));
            assert(!ClassicPartyActions::active(screen));
            if(action<2) {
                if(gen==1)assert(mode()==static_cast<int>(action?Gen1PokemonEditor::UX2Mode::Edit:Gen1PokemonEditor::UX2Mode::View));
                if(gen==2)assert(Gen2PokemonEditor::state.party && mode()==static_cast<int>(action?Gen2PokemonEditor::Mode::Edit:Gen2PokemonEditor::Mode::View));
                if(gen==3)assert(Gen3SharedEditorSurface::state.party && mode()==static_cast<int>(action?PokeBank::UIModel::Gen3SharedEditor::Mode::Edit:PokeBank::UIModel::Gen3SharedEditor::Mode::View));
            }
            if(action==2)assert(gen==1?Gen1PokemonEditor::state.mode==Gen1PokemonEditor::UX2Mode::Provenance:
                gen==2?Gen2PokemonEditor::shared.provenanceActive:Gen3SharedEditorSurface::state.legality);
        }
        reset();assert(!ClassicPartyActions::open(screen,1));
        native.available=false;assert(!ClassicPartyActions::open(screen,0));
        reset();assert(ClassicPartyActions::open(screen,0));
        // B owns a simultaneous A/cancel frame and cannot enter View/Edit.
        assert(ClassicPartyActions::handle(screen,HidNpadButton_A|HidNpadButton_B,0,0,0,{}));
        assert(!ClassicPartyActions::active(screen));
        assert(!ClassicPartyActions::handle(screen,HidNpadButton_A,0,0,0,{}));
        // 7D: a completed tap outside the drawn sheet closes it exactly like B and enters no
        // session; a tap inside the sheet (not on a row) does not close it.
        reset();assert(ClassicPartyActions::open(screen,0));
        assert(ClassicPartyActions::handle(screen,HidNpadButton_B,0,0,0,{}));
        const int afterB=mode();
        reset();assert(ClassicPartyActions::open(screen,0));
        {PKSEFramebuffer fb;assert(ClassicPartyActions::draw(screen,fb));}
        const auto g=PokeBank::UIModel::NarrowPokemonMenu::geometry(4,1280,720,56);
        assert(ClassicPartyActions::handle(screen,0,0,0,0,{-1,true,g.x+8,g.y+8,g.x+9,g.y+9}));
        assert(ClassicPartyActions::active(screen));
        assert(ClassicPartyActions::handle(screen,0,0,0,0,{-1,true,20,40,22,41}));
        assert(!ClassicPartyActions::active(screen) && mode()==afterB);
    }
    std::cout<<"Actual Gen I-III Party coordinator: four shared rows, controller/touch parity, native sessions, cancel priority PASS\n";
}
