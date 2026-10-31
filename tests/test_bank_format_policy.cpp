#include "Trainer/BankFormatPolicy.h"
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
    namespace P = Trainer::BankFormatPolicy;
    static_assert(P::currentBoxCount == 100);

    assert(P::classifyBoxCount(1) == P::Disposition::Supported);
    assert(P::classifyBoxCount(P::currentBoxCount) == P::Disposition::Supported);
    assert(P::classifyBoxCount(P::currentBoxCount + 1) == P::Disposition::MigrationRequired);
    assert(P::classifyBoxCount(4096) == P::Disposition::MigrationRequired);
    assert(P::classifyBoxCount(0) == P::Disposition::Invalid);
    assert(P::classifyBoxCount(4097) == P::Disposition::Invalid);

    const auto bank = read("src/Trainer/Bank.cpp");
    assert(bank.find("BankFormatPolicy::classifyBoxCount(fileBoxes)") != std::string::npos);
    assert(bank.find("writeBlocked = true") != std::string::npos);
    assert(bank.find("if (writeBlocked)") != std::string::npos);
    assert(bank.find("fileBoxes = static_cast<uint32_t>(BANK_BOX_COUNT)") == std::string::npos);

    std::cout << "Bank format fail-closed policy: PASS\n";
}
