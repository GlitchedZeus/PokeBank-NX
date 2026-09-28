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
}

int main() {
    const auto header = read("include/Utils/Settings.h");
    const auto source = read("src/Utils/Settings.cpp");

    assert(header.find("bool saveSettings();") != std::string::npos);

    const auto begin = source.find("bool saveSettings()");
    assert(begin != std::string::npos);
    const auto end = source.find("\n    }\n}", begin);
    assert(end != std::string::npos && end > begin);
    const auto body = source.substr(begin, end - begin);

    assert(body.find("DurableFile::replace") != std::string::npos);
    assert(body.find("settings reread differs from requested configuration") != std::string::npos);
    assert(body.find("injectToGame=0\\n") != std::string::npos);
    assert(body.find("return false") != std::string::npos);
    assert(body.find("fopen(settingsPath().c_str(), \"w\")") == std::string::npos);
    assert(body.find("fprintf(") == std::string::npos);
    assert(body.find("fclose(") == std::string::npos);

    std::cout << "Settings durable persistence contract: PASS\n";
}
