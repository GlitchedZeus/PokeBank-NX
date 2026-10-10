#include "UI/NaturePickerLayout.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>

static std::string read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int main() {
    namespace Nature=PokeBank::UIModel::NaturePickerLayout;
    constexpr auto full=Nature::forChoices(25);
    static_assert(full.width==480 && full.height==390);
    static_assert(full.visibleRows==7 && full.rowStep==38);
    static_assert(full.highlightHeight==34);
    static_assert(full.height<560 && full.width<680);
    constexpr auto shortList=Nature::forChoices(2);
    static_assert(shortList.visibleRows==2 && shortList.height==200);
    static_assert(Nature::forChoices(0).visibleRows==1);
    // A seven-row window has enough vertical clearance for title, page
    // counter, readable selection text and its final focus rectangle.
    static_assert(90+(full.visibleRows-1)*full.rowStep+full.highlightHeight < full.height);
    for(const char* path:{"src/UI/Gen3SharedPokemonSurface.inc",
                          "src/UI/Gen4SharedPokemonSurface.inc"}) {
        const auto source=read(path);
        assert(source.find("#include \"UI/NaturePickerLayout.h\"")!=std::string::npos);
        assert(source.find("naturePicker=state.pickerTarget==PickerTarget::Nature")!=std::string::npos);
        assert(source.find("natureLayout.visibleRows")!=std::string::npos);
        assert(source.find("naturePicker ? natureLayout.width")!=std::string::npos);
        assert(source.find("naturePicker ? natureLayout.height")!=std::string::npos);
        assert(source.find("naturePicker ? natureLayout.rowStep : 42")!=std::string::npos);
        assert(source.find("naturePicker ? natureLayout.highlightHeight : 38")!=std::string::npos);
        assert(source.find("if (selected) fb.drawSelectionHighlight")!=std::string::npos);
    }
    return 0;
}
