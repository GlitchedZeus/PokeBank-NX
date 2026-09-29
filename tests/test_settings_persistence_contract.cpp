#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

static std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

int main() {
    const std::string src = read("src/Utils/Settings.cpp");
    const std::string hdr = read("include/Utils/Settings.h");
    assert(src.find("AtomicTextFile::replace(settingsPath(), text)") != std::string::npos);
    assert(src.find("AtomicTextFile::recoverPreviousIfNeeded(settingsPath())") != std::string::npos);
    assert(src.find("fopen(settingsPath().c_str(), \"w\")") == std::string::npos);
    assert(hdr.find("bool saveSettings();") != std::string::npos);
    std::cout << "Settings persistence contract: PASS\n";
}
