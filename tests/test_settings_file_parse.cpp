// Settings hardening (PKSE f4fd79a / c671aa4 review; gap matrix B-14; owner Oct 28 Phase 2):
// settings.cfg lines are parsed whole and bounded. The old loader read 127-byte chunks with fgets,
// so a longer line was split and its tail parsed as a NEW line: a long value (e.g. a future folder
// path) could inject another setting ("...debugLogging=1") or be silently truncated.
#include "Utils/SettingsFile.h"
#include "Utils/PokeBankPaths.h"

#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>

using Utils::SettingsFile::parse;

int main() {
    // Ordinary file, CRLF and LF, blank and comment-like lines ignored.
    auto e = parse("theme=dark\r\nmoveWarn=1\n\n# note\nnoequals\n=novalue\ndebugLogging=0");
    assert(e.size() == 3);
    assert(e[0].key == "theme" && e[0].value == "dark");
    assert(e[1].key == "moveWarn" && e[1].value == "1");
    assert(e[2].key == "debugLogging" && e[2].value == "0");

    // A long, legitimate value (beyond the old 127-byte chunk) is kept whole.
    const std::string longPath = "sdmc:/" + std::string(600, 'a') + "/saves";
    e = parse("favoriteFolder=" + longPath + "\n");
    assert(e.size() == 1 && e[0].value == longPath);

    // The old split could turn the tail of a long line into a setting. Never again.
    // Reproduced on the old loop: with the tail at byte 127 it read a second line "debugLogging=1"
    // and switched debug logging on.
    for (std::size_t pad : {112u, 140u}) {
        const std::string spill = "favoriteFolder=" + std::string(pad, 'x') + "debugLogging=1\n";
        e = parse(spill);
        assert(e.size() == 1 && e[0].key == "favoriteFolder" && e[0].value.size() == pad + 14);
    }

    // Repeated keys are preserved in order (favourite folders).
    e = parse("favoriteFolder=sdmc:/a\nfavoriteFolder=sdmc:/b\n");
    assert(e.size() == 2 && e[0].value == "sdmc:/a" && e[1].value == "sdmc:/b");

    // Excessive line or value: the whole line is dropped, neighbours survive.
    const std::string huge = "favoriteFolder=" + std::string(Utils::SettingsFile::kMaxValue + 1, 'y');
    e = parse("theme=light\n" + huge + "\nmoveWarn=0\n");
    assert(e.size() == 2 && e[0].key == "theme" && e[1].key == "moveWarn");
    const std::string hugeLine(Utils::SettingsFile::kMaxLine + 10, 'z');
    e = parse(hugeLine + "debugLogging=1\ntheme=light\n");
    assert(e.size() == 1 && e[0].key == "theme");

    // Keys are plain identifiers; control bytes in a value refuse the line.
    e = parse("bad key=1\ntheme=da\x01rk\nmoveWarn=1\n");
    assert(e.size() == 1 && e[0].key == "moveWarn");

    // A truncated last line (no newline) still parses; an empty file yields nothing.
    assert(parse("theme=dark").size() == 1 && parse("").empty());
    // The production loader uses this parser and never fixed-size fgets chunks again.
    std::ifstream in("src/Utils/Settings.cpp");
    const std::string src{std::istreambuf_iterator<char>(in), {}};
    assert(src.find("SettingsFile::parse(text)") != std::string::npos);
    assert(src.find("fgets(") == std::string::npos);

    // B-15 (PKSE c671aa4): an empty path is never an existing directory.
    assert(!PokeBank::Paths::directoryExists(""));
    assert(PokeBank::Paths::directoryExists("."));
    std::cout << "settings.cfg bounded whole-line parsing: PASS\n";
    return 0;
}
