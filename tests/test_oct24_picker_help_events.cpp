#include "UI/SharedHeldItemSearch.h"
#include "UI/ItemPickerSearch.h"
#include "UI/ClassicHelpPresentation.h"
#include "UI/SharedMenuGeometry.h"
#include "Names/LocationDisplayNames.h"
#include <cassert>
#include <iostream>

namespace Search=UI::SharedHeldItemSearch;
namespace Keys=UI::SharedSearchInput;
namespace Grid=PokeBank::UIModel::SharedHeldItemPicker;

int main() {
    const std::vector<unsigned> native{0,4,5,13,14,221};
    auto shown=native;
    const auto name=[](unsigned item)->std::string {
        switch(item) {
            case 0:return "None";case 4:return "Poké Ball";case 5:return "Safari Ball";
            case 13:return "Potion";case 14:return "Hyper Potion";default:return "King's Rock";
        }
    };
    Search::State search;int selected=1,updates=0;
    auto sort=UI::ItemPickerSort::Mode::Native;
    const auto refilter=[&](const std::string& query) {
        ++updates;
        const int row=UI::ItemPickerSearch::apply(shown,native,sort,query,selected,name,Keys::searchKey);
        if(row>=0)selected=row;
        return row>=0;
    };
    const auto handle=[&](const Keys::Keys& keys={},const Keys::Touch& touch={},bool toggle=false) {
        return Search::handle(search,toggle,keys,touch,100,50,refilter);
    };
    assert(handle({}, {}, true) && search.open());
    assert(Grid::columnsForSearch(search.open())==2 && Grid::pageSizeForSearch(search.open())==20);
    const auto keyboard=Search::keyboardRect(100,50);
    assert(100+Grid::gridInsetX+Grid::gridWidth(true)<keyboard.x);
    assert(keyboard.y>=50+Grid::gridInsetY && keyboard.y+keyboard.h<50+Grid::modalHeight);
    // Navigate with the SAME directional/A input handled by the actual picker.
    const auto type=[&](int key) {
        int safety=50;
        while(Keys::keyRow(search.input.cursor)!=Keys::keyRow(key) && --safety>0)
            assert(handle({.down=true}));
        while(search.input.cursor!=key && --safety>0)assert(handle({.right=true}));
        assert(safety>0);assert(handle({.a=true}));
    };
    type('P'-'A');type('O'-'A');type('K'-'A');type('E'-'A');
    assert(search.input.query=="POKE" && updates==4 && shown==std::vector<unsigned>{4});
    assert(handle({.b=true}) && !search.open()); // B closes keys, retains live filter.
    assert(shown==std::vector<unsigned>{4});
    assert(!handle({.b=true})); // the next B belongs to the real chooser's Back action.
    assert(!handle({.a=true})); // A can now choose the actual native item, unchanged ID 4.
    assert(handle({}, {}, true));type(Keys::Clear);assert(shown==native);
    type('Z'-'A');assert(search.noMatches);const auto previous=shown;
    assert(handle({.b=true}) && handle({.a=true})); // never commit a stale result after no match.
    assert(shown==previous);
    assert(handle({}, {}, true));type(Keys::Backspace);assert(!search.noMatches && shown==native);
    // A touch key activates only on release of the SAME key; a drag never types.
    const auto p=Keys::keyRect(keyboard,'P'-'A');const int x=p.x+5,y=p.y+5;
    assert(handle({}, {true,true,false,x,y,x,y}));assert(search.input.query.empty());
    assert(handle({}, {false,false,true,x,y,x,y}));assert(search.input.query=="P");
    assert(handle({}, {true,true,false,x,y,x,y}));
    assert(handle({}, {false,false,true,x+100,y,x,y}));assert(search.input.query=="P");
    type(Keys::Clear);type('P'-'A');type('O'-'A');type('T'-'A');
    assert((shown==std::vector<unsigned>{13,14}));
    sort=UI::ItemPickerSort::next(sort);assert(refilter(search.input.query));
    assert((shown==std::vector<unsigned>{14,13})); // sort plus substring filtering; native IDs retained.
    assert(native==std::vector<unsigned>({0,4,5,13,14,221}));

    namespace Help=PokeBank::UIModel::ClassicHelp;
    for(int gen=1;gen<=5;++gen)for(auto page:{Help::Page::Hub,Help::Page::Pokemon,
        Help::Page::Party,Help::Page::Items,Help::Page::Trainer}) {
        const auto lines=Help::lines(page,gen);assert(lines.size()>=6 && lines.size()<=10);
        for(const auto& line:lines)assert(!line.empty() && line!="Tools.");
        assert(lines.front().find("Original save unchanged")!=std::string::npos);
    }
    namespace Menu=PokeBank::UIModel::SharedMenuGeometry;
    assert(Menu::rule(Menu::MenuType::LocationPicker).width==620);
    for(auto origin:{20,21,22,23})for(unsigned year:{2010,2011,2013,2020}) {
        const uint16_t location=40074+year-2010;
        assert(Names::metLocationEventYear(origin,location)==year);
        assert(Names::getMetLocationDisplayName(origin,location)=="Pokémon Event "+std::to_string(year));
        const auto evidence=Names::metLocationEventEvidence(origin,location);
        assert(evidence.find("unverified")!=std::string::npos);
    }
    assert(!Names::metLocationEventYear(12,40074));assert(!Names::metLocationEventYear(22,3061));
    assert(Names::metLocationEventYear(12,3061)==2006);
    assert(Names::metLocationEventEvidence(22,1).empty());
    assert(!Names::metLocationEventYear(22,40085));
    std::cout<<"October 24 docked Held Item controller/touch/live search/sort/no-match, compact menus, Help and honest event years PASS\n";
}
