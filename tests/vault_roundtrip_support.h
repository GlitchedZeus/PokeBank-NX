#pragma once
// Master Vault v1 test support for the per-generation staged-editor tests
// (tests/test_pokemon_file_gen12/gen3/gen4/gen5.cpp): the Copy to Vault -> app restart -> Withdraw
// from Vault path exactly as UI/PokemonFileMenus.inc runs it, on the real file system (mkdtemp) with
// the real DurableFile. The caller then stages the withdrawn copy through the generation's own
// import, so every occupied / capacity refusal is the editor's own.
#include "PokemonFile/NativePokemonFile.h"
#include "Vault/VaultStore.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace VaultTest {

inline std::string freshRoot() {
    char tmpl[] = "/tmp/pokebank-vault-gen-XXXXXX";
    const char* dir = ::mkdtemp(tmpl);
    assert(dir);
    return std::string(dir) + "/vault";
}

// Deposits `exported` (what Export to file / copyToVault hands the Vault), closes the Store, opens a
// NEW Store on the same folder (a restart), checks the record is byte-identical, refuses withdrawal
// into every other generation, and returns the withdrawn copy. The Vault still holds the record.
inline PokeBank::PokemonFile::Decoded depositReopenWithdraw(int generation, std::span<const uint8_t> exported) {
    namespace V = PokeBank::Vault;
    const std::string root = freshRoot();
    std::string error;
    V::VaultId id;
    {
        V::Store vault;
        assert(vault.open(root, nullptr, error));
        V::DepositRequest request;
        request.generation = generation;
        request.raw.assign(exported.begin(), exported.end());
        request.summary.speciesName = "Test";
        request.provenance.sourceGame = "Generation " + std::to_string(generation);
        const auto deposited = vault.deposit(request, error);
        if (!deposited) std::cerr << "Vault deposit refused: " << error << "\n";
        assert(deposited);
        id = *deposited;
    }
    V::Store reopened;
    V::OpenReport report;
    assert(reopened.open(root, &report, error));
    assert(report.rolledForward == 0 && report.rolledBack == 0 && report.quarantined == 0);
    assert(reopened.entries().size() == 1 && reopened.entries()[0].id == id);
    const auto record = reopened.read(id, error);
    assert(record && record->raw == std::vector<uint8_t>(exported.begin(), exported.end()));
    for (int other = 1; other <= 5; ++other)
        if (other != generation) assert(!reopened.withdrawCopy(id, other, error));
    auto copy = reopened.withdrawCopy(id, generation, error);
    if (!copy) std::cerr << "Vault withdraw refused: " << error << "\n";
    assert(copy && copy->generation == generation && !copy->wasEncrypted);
    assert(copy->plain == record->raw);
    // Withdraw is a copy: the record is still there, unchanged.
    const auto again = reopened.read(id, error);
    assert(again && again->raw == record->raw && reopened.entries().size() == 1);
    return *copy;
}

}  // namespace VaultTest
