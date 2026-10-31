#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string section(const std::string& src, const std::string& begin, const std::string& end) {
    const auto a = src.find(begin);
    const auto b = src.find(end, a == std::string::npos ? 0 : a + begin.size());
    assert(a != std::string::npos && b != std::string::npos && b > a);
    return src.substr(a, b - a);
}
}

int main() {
    for (const char* path : {
             "src/Trainer/Trainer8SWSH.cpp",
             "src/Trainer/Trainer8LA.cpp",
             "src/Trainer/Trainer9SV.cpp",
             "src/Trainer/Trainer9LZA.cpp",
         }) {
        const std::string src = read(path);
        const std::string party = section(src, "::parsePartyBlock(", "::parseMoneyBlock(");
        const std::string boxes = section(src, "::parseBoxBlock(", "::parseBoxLayoutBlock(");

        assert(party.find("speciesID() != 0") != std::string::npos);
        assert(boxes.find("speciesID() != 0") != std::string::npos);
        assert(party.find("bool isEmptySlot") == std::string::npos);
        assert(boxes.find("bool isEmptySlot") == std::string::npos);
    }

    // Direct Boxes-mode grab/swap remains species-aware as defense in depth.
    const std::string ui = read("src/UI/TrainerViewScreenBase.inc");
    assert(ui.find("cursorPokemon && cursorPokemon->speciesID() != 0") != std::string::npos);

    const std::string saveSelectHeader = read("include/UI/SaveSelectScreen.h");
    const std::string saveSelectSource = read("src/UI/SaveSelectScreen.cpp");
    assert(saveSelectHeader.find("OpenFailure") != std::string::npos);
    assert(saveSelectSource.find("overlay = Overlay::OpenFailure;") != std::string::npos);
    assert(saveSelectSource.find("\"SAVE NOT OPENED\"") != std::string::npos);
    assert(saveSelectSource.find("Technical details were written to diagnostics.") != std::string::npos);
    assert(saveSelectSource.find("HidNpadButton_A | HidNpadButton_B") != std::string::npos);

    std::cout << "Modern trainer encrypted-blank occupancy contract: PASS\n";
    return 0;
}
