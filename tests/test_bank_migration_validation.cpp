#include "Trainer/BankPokemonValidation.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
struct FakePokemon {
    unsigned species = 0;
    unsigned stored = 0;
    unsigned calculated = 0;

    unsigned speciesID() const noexcept { return species; }
    unsigned checksum() const noexcept { return stored; }
    unsigned calculateChecksum() const noexcept { return calculated; }
};

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    using Trainer::BankPokemonValidation::accepts;

    assert(!accepts<FakePokemon>(nullptr));
    const FakePokemon empty{0, 0x1234, 0x1234};
    assert(!accepts(&empty));
    const FakePokemon corrupt{25, 0x1234, 0x5678};
    assert(!accepts(&corrupt));
    const FakePokemon valid{25, 0xBEEF, 0xBEEF};
    assert(accepts(&valid));

    // Both normal unified loading and one-time legacy migration must use the
    // exact same acceptance predicate so the two paths cannot drift again.
    const auto src = read("src/Trainer/Bank.cpp");
    const auto loadBegin = src.find("void Bank::load()");
    const auto migrateBegin = src.find("void Bank::migrateLegacyBanks()");
    const auto verifyBegin = src.find("size_t Bank::verifyImage", migrateBegin);
    assert(loadBegin != std::string::npos);
    assert(migrateBegin != std::string::npos && migrateBegin > loadBegin);
    assert(verifyBegin != std::string::npos && verifyBegin > migrateBegin);

    const auto loadBody = src.substr(loadBegin, migrateBegin - loadBegin);
    const auto migrateBody = src.substr(migrateBegin, verifyBegin - migrateBegin);
    assert(loadBody.find("BankPokemonValidation::accepts(pk.get())") != std::string::npos);
    assert(migrateBody.find("BankPokemonValidation::accepts(pk.get())") != std::string::npos);
    assert(migrateBody.find("++rejected") != std::string::npos);
    assert(migrateBody.find("corrupt rejected") != std::string::npos);
    assert(migrateBody.find("capacity dropped") != std::string::npos);

    // Migration is import-only: it must not rename/remove the legacy evidence.
    assert(migrateBody.find("std::remove(") == std::string::npos);
    assert(migrateBody.find("std::rename(") == std::string::npos);

    std::cout << "Bank legacy migration validation contract: PASS\n";
}
