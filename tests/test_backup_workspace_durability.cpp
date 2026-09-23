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

std::string functionBody(const std::string& source, const std::string& name) {
    const auto begin = source.find("bool " + name + "(");
    assert(begin != std::string::npos);
    const auto next = source.find("\n    bool saveTrainerInfo", begin + 10);
    return source.substr(begin, next == std::string::npos ? std::string::npos : next - begin);
}

void requireDurable(const std::string& source, const char* name, const char* validator) {
    const auto body = functionBody(source, name);
    assert(body.find("persistWorkspaceFile") != std::string::npos);
    assert(body.find(validator) != std::string::npos);
    assert(body.find("fopen(savePath, \"wb\")") == std::string::npos);
    assert(body.find("fwrite(") == std::string::npos);
}
}

int main() {
    const auto source = read("src/Save/GetSaveFileContents.cpp");

    assert(source.find("#include \"Utils/DurableFile.h\"") != std::string::npos);
    assert(source.find("bool validateSCWorkspace") != std::string::npos);
    assert(source.find("bool validatePLAWorkspace") != std::string::npos);
    assert(source.find("bool validateLGPEWorkspace") != std::string::npos);
    assert(source.find("bool validateFRLGWorkspace") != std::string::npos);

    requireDurable(source, "saveTrainerInfoLetsGo", "validateLGPEWorkspace");
    requireDurable(source, "saveTrainerInfoSwSh", "validateSCWorkspace");
    requireDurable(source, "saveTrainerInfoLA", "validatePLAWorkspace");
    requireDurable(source, "saveTrainerInfoLZA", "validateSCWorkspace");
    requireDurable(source, "saveTrainerInfoSV", "validateSCWorkspace");
    requireDurable(source, "saveTrainerInfoFRLG", "validateFRLGWorkspace");

    const auto bdsp = functionBody(source, "saveTrainerInfoBDSP");
    assert(bdsp.find("MULTI-FILE JOURNAL REQUIRED") != std::string::npos);
    assert(bdsp.find("return false") != std::string::npos);
    assert(bdsp.find("fopen(") == std::string::npos);
    assert(bdsp.find("DurableFile::replace") == std::string::npos);

    assert(source.find("fopen(savePath, \"wb\")") == std::string::npos);

    std::cout << "Mutable backup workspace durability contract: PASS\n";
}
