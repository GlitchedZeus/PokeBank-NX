#pragma once
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedSearchInput.h"
#include "UI/ItemPickerArtwork.h"
#if __has_include(<switch.h>)
#include "UI/ScreenChrome.h"
#endif

namespace UI::SharedHeldItemSearch {
namespace Grid=PokeBank::UIModel::SharedHeldItemPicker;
struct State {
    SharedSearchInput::State input;
    bool noMatches=false;
    bool open() const noexcept {return input.open;}
};
inline SharedSearchInput::Rect keyboardRect(int x,int y) {
    return {x+540,y+320,Grid::modalWidth-562,176};
}
// Reuse the Species chooser's keyboard and folded text, docked BESIDE
// results. It owns A/navigation while open; typing never selects an item.
template<class Refilter>
bool handle(State& state,bool toggle,const SharedSearchInput::Keys& keys,
    const SharedSearchInput::Touch& touch,int x,int y,Refilter&& refilter,bool extraKeys=false) {
    if(toggle){state.input.toggleGrid();return true;}
    if(!state.open())return state.noMatches && keys.a;
    const auto revision=state.input.revision;
    const auto touched=SharedSearchInput::handleTouch(state.input,touch,keyboardRect(x,y));
    const auto keyed=touched==SharedSearchInput::Event::None?
        SharedSearchInput::handleKeys(state.input,keys):SharedSearchInput::Event::None;
    if(state.input.revision!=revision)state.noMatches=!refilter(state.input.query);
    return touched!=SharedSearchInput::Event::None || keyed!=SharedSearchInput::Event::None || extraKeys;
}
inline void draw(PKSEFramebuffer& fb,int x,int y,const State& state,
    const std::string& name,const std::string& sprite) {
    if(state.noMatches)
        fb.drawText(x+28,y+110,"No items match \""+state.input.query+"\"",Colors::TextDim,TextStyle::Caption);
    if(!state.open())return;
    const int right=x+540;
    fb.drawText(right,y+94,"Search: "+state.input.query,Colors::Accent,TextStyle::Body);
    fb.drawText(right,y+122,"Type here; results remain visible on the left",Colors::TextDim,TextStyle::Caption);
    if(!sprite.empty())ItemPickerArtwork::draw(fb,right+300,y+158,96,sprite);
    fb.drawText(right,y+278,name,Colors::Text,TextStyle::Caption);
    SharedSearchInput::drawGrid(fb,state.input,keyboardRect(x,y));
}
#if __has_include(<switch.h>)
inline void drawFooter(PKSEFramebuffer& fb,const State& state) {
    if(state.open())drawNavBar(fb,{{"D-pad/Stick","Choose Key"},{"A","Type"},{"B","Results"},{"-","Close Search"}});
    else drawNavBar(fb,{{"-","Edit Search"},{"B","Back"}});
}
#endif
}
