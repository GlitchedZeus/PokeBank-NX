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

    // N05/A09: same-directory DurableFile recovery generations are evidence, not
    // authoritative FRLG save candidates. Their size is also 128 KiB, so the scanner
    // must exclude them explicitly.
    assert(source.find("name.find(\".tmp.\")") != std::string::npos);
    assert(source.find("name.find(\".previous.\")") != std::string::npos);
    assert(source.find("name.find(\".failed.\")") != std::string::npos);

    const auto bdsp = functionBody(source, "saveTrainerInfoBDSP");
    assert(bdsp.find("MULTI-FILE JOURNAL REQUIRED") != std::string::npos);
    assert(bdsp.find("return false") != std::string::npos);
    assert(bdsp.find("fopen(") == std::string::npos);
    assert(bdsp.find("DurableFile::replace") == std::string::npos);

    assert(source.find("fopen(savePath, \"wb\")") == std::string::npos);

    // AUDIT-015: ordinary backup creation must not expose a partial final directory.
    const auto files = read("src/Utils/FileUtilities.cpp");
    assert(files.find("bool copyDirectoryTransactional(") != std::string::npos);
    assert(files.find(".incomplete.") != std::string::npos);
    assert(files.find(".failed.") != std::string::npos);
    assert(files.find(".previous.") != std::string::npos);
    assert(files.find("rename(incomplete.c_str(), final.c_str())") != std::string::npos);
    assert(files.find("verifyCopiedFile(destFilePath, data, size)") != std::string::npos);
    assert(files.find("fflush(out)") != std::string::npos);
    assert(files.find("fclose(out) != 0") != std::string::npos);
    const auto txBegin = files.find("bool copyDirectoryTransactional(");
    const auto backupBegin = files.find("std::string backupSaveData(", txBegin);
    assert(txBegin != std::string::npos && backupBegin != std::string::npos);
    const auto txBody = files.substr(txBegin, backupBegin - txBegin);
    const auto copy = txBody.find("copyDirectory(srcPath, incomplete.c_str())");
    const auto promote = txBody.find("rename(incomplete.c_str(), final.c_str())");
    assert(copy != std::string::npos && promote != std::string::npos && copy < promote);

    std::cout << "Mutable backup workspace durability contract: PASS\n";
}
