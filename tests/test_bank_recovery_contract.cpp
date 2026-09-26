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
    const auto src = read("src/Trainer/Bank.cpp");
    const auto begin = src.find("auto abandonFile");
    const auto end = src.find("if (fileSize < HEADER_SIZE", begin);
    assert(begin != std::string::npos);
    assert(end != std::string::npos && end > begin);
    const auto body = src.substr(begin, end - begin);

    assert(body.find("uniqueUnreadablePath") != std::string::npos);
    assert(body.find("std::remove(aside.c_str())") == std::string::npos);
    assert(body.find("std::rename(path.c_str(), aside.c_str())") != std::string::npos);

    std::cout << "Bank recovery evidence preservation contract: PASS\n";
}
