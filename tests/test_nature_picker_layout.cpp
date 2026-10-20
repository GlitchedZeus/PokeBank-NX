#include "UI/NaturePickerLayout.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>

namespace {
// The shared editor workspace lives in src/UI/SharedPokemonEditorSurface.inc,
// which a generation surface includes textually. Expand that one include so a
// surface's contract covers the shared code it compiles.
std::string expandSurfaceIncludes(const std::string& text) {
    std::string out;
    std::size_t pos = 0;
    const std::string marker = "#include \"";
    while (true) {
        const auto at = text.find(marker, pos);
        if (at == std::string::npos) { out += text.substr(pos); break; }
        const auto close = text.find('"', at + marker.size());
        const std::string name = text.substr(at + marker.size(), close - at - marker.size());
        out += text.substr(pos, close + 1 - pos);
        pos = close + 1;
        if (name == "SharedPokemonEditorSurface.inc") {
            std::ifstream nested("src/UI/" + name, std::ios::binary);
            if (nested) out += "\n" + expandSurfaceIncludes(
                {std::istreambuf_iterator<char>(nested), std::istreambuf_iterator<char>()});
        }
    }
    return out;
}
}
static std::string read(const char* path) {
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return expandSurfaceIncludes({std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()});
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
        // NEW-08: a scrolling list reserves a gutter so the scrollbar never crowds the rows.
        assert(source.find("const int rowW = w - 72;")!=std::string::npos);
        assert(source.find("static_assert(72 - 22 >= 32 + SharedScrollbar::kTouchSlop")!=std::string::npos);
        assert(source.find("drawAndPublish(fb, state.pickerScrollbar, x + w - 32, viewportTop - 4,")!=std::string::npos);
        assert(source.find("SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy - 4, rowW, naturePicker ? natureLayout.highlightHeight : 38), {selected});")!=std::string::npos); // R2-BUG-012; NEW-08 rowW
    }
    return 0;
}
