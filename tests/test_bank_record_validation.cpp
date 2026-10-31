#include "Trainer/BankRecordValidation.h"

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
    using Trainer::BankRecordValidation::accept;

    assert(!accept(0, 0x1234, 0x1234));
    assert(!accept(25, 0x1234, 0x5678));
    assert(accept(25, 0xBEEF, 0xBEEF));

    const std::string src = read("src/Trainer/Bank.cpp");
    const auto loadBegin = src.find("void Bank::load()");
    const auto migrateBegin = src.find("void Bank::migrateLegacyBanks()");
    const auto verifyBegin = src.find("size_t Bank::verifyImage", migrateBegin);
    assert(loadBegin != std::string::npos);
    assert(migrateBegin != std::string::npos && migrateBegin > loadBegin);
    assert(verifyBegin != std::string::npos && verifyBegin > migrateBegin);

    const std::string loadBody = src.substr(loadBegin, migrateBegin - loadBegin);
    const std::string migrateBody = src.substr(migrateBegin, verifyBegin - migrateBegin);
    assert(loadBody.find("BankRecordValidation::accept(") != std::string::npos);
    assert(migrateBody.find("BankRecordValidation::accept(") != std::string::npos);
    assert(migrateBody.find("rejectedCorrupt") != std::string::npos);

    std::cout << "Bank record acceptance parity: PASS\n";
}
