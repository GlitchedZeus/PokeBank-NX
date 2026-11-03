// Master Vault v1: the ONE shared Pokemon Actions vocabulary gains "Copy to Vault" (occupied slot,
// after Export to file) and "Withdraw from Vault" (empty slot, after Import from file) ONLY when the
// build flag is on (Vault/VaultFeature.h) and only where the staged editor has the file capability.
// Built twice: the default build (flag OFF: no candidate shows the rows) and a Vault test build
// (-DPOKEBANK_ENABLE_VAULT=1). Gen I uses its own native set; Gen II, III and V the shared one;
// Gen IV its own list with the shared labels (pinned by tests/test_vault_wiring_contract.py).
#include "UI/NarrowPokemonActionMenu.h"
#include "UI/Gen1PokemonEditorUIContract.h"
#include "UI/SharedPokemonEditorContract.h"
#include "Vault/VaultFeature.h"

#include <cassert>
#include <cstring>
#include <iostream>

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Gen1 = PokeBank::UIModel::Gen1Editor;
namespace Menu = PokeBank::UIModel::NarrowPokemonMenu;

namespace {
template <class Set, class A>
int indexOf(const Set& set, A action) {
    for (std::size_t i = 0; i < set.count; ++i)
        if (set[i] == action) return static_cast<int>(i);
    return -1;
}
bool hasLabel(const Menu::Model& m, const char* label) {
    for (int i = 0; i < m.count; ++i)
        if (std::strcmp(m.rows[static_cast<std::size_t>(i)].label, label) == 0) return true;
    return false;
}
Shared::ActionCapabilities fileCaps(bool vault) {
    Shared::ActionCapabilities c = Shared::fileCapabilities(true);
    c.hasLegalityProvenance = true;
    c.canRemove = true;
    c.canMove = true;
    c.vault = vault;
    return c;
}
}  // namespace

int main() {
    // The default capability follows the build flag.
    assert(Shared::ActionCapabilities{}.vault == PokeBank::Vault::kEnabled);
#if POKEBANK_ENABLE_VAULT
    assert(PokeBank::Vault::kEnabled);
#else
    assert(!PokeBank::Vault::kEnabled);
    // Default build: no Gen I-V sheet lists a Vault row, whatever the slot offers.
    for (const bool occupied : {true, false}) {
        auto caps = Shared::fileCapabilities(true);
        caps.canMove = caps.canRemove = caps.hasLegalityProvenance = true;
        const auto set = Shared::actionsForSlot(occupied, caps, true);
        assert(indexOf(set, Shared::Action::CopyToVault) < 0 && indexOf(set, Shared::Action::WithdrawFromVault) < 0);
        const auto g1 = Gen1::actionsForSlot(occupied, true, true, true);
        assert(indexOf(g1, Gen1::Action::CopyToVault) < 0 && indexOf(g1, Gen1::Action::WithdrawFromVault) < 0);
        assert(!hasLabel(Menu::classic(set), Menu::kCopyToVaultLabel));
        assert(!hasLabel(Menu::classic(set), Menu::kWithdrawFromVaultLabel));
    }
#endif

    // Vault on: one row each, in the shared order, keeping native indices aligned.
    {
        const auto occupied = Shared::actionsForSlot(true, fileCaps(true), true);
        const int exp = indexOf(occupied, Shared::Action::ExportFile);
        const int vault = indexOf(occupied, Shared::Action::CopyToVault);
        assert(exp >= 0 && vault == exp + 1 && occupied[static_cast<std::size_t>(vault + 1)] == Shared::Action::Review);
        assert(indexOf(occupied, Shared::Action::WithdrawFromVault) < 0);
        const auto model = Menu::classic(occupied);
        assert(model.count == static_cast<int>(occupied.count) - 0 && model.count == 10);
        assert(std::strcmp(model.rows[static_cast<std::size_t>(model.visualIndex(vault))].label, "Copy to Vault") == 0);

        const auto empty = Shared::actionsForSlot(false, fileCaps(true), false);
        const int imp = indexOf(empty, Shared::Action::ImportFile);
        assert(imp >= 0 && indexOf(empty, Shared::Action::WithdrawFromVault) == imp + 1);
        assert(indexOf(empty, Shared::Action::CopyToVault) < 0);
        assert(hasLabel(Menu::classic(empty), "Withdraw from Vault"));
        assert(Shared::surfaceForAction(Shared::Action::CopyToVault) == Shared::Surface::Actions);
        assert(std::strcmp(Shared::actionLabel(Shared::Action::WithdrawFromVault), "Withdraw from Vault") == 0);
    }
    // No staged editor (read-only / Japanese Gen I-II sessions): no file capability, so no Vault rows
    // even with the flag on.
    {
        Shared::ActionCapabilities readOnly;
        readOnly.vault = true;
        for (const bool occupied : {true, false}) {
            const auto set = Shared::actionsForSlot(occupied, readOnly, false);
            assert(indexOf(set, Shared::Action::CopyToVault) < 0 && indexOf(set, Shared::Action::WithdrawFromVault) < 0);
            const auto g1 = Gen1::actionsForSlot(occupied, false, false, /*fileActions=*/false, /*vault=*/true);
            assert(indexOf(g1, Gen1::Action::CopyToVault) < 0 && indexOf(g1, Gen1::Action::WithdrawFromVault) < 0);
        }
    }
    // The Party sheet (Gen I-III) is View / Edit / Origin / Export: Copy to Vault joins after Export.
    {
        Shared::ActionCapabilities party{false, false, true, false, true};
        party.vault = true;
        const auto set = Shared::actionsForSlot(true, party, false);
        assert(indexOf(set, Shared::Action::CopyToVault) == indexOf(set, Shared::Action::ExportFile) + 1);
    }
    // Gen I native set.
    {
        const auto g1 = Gen1::actionsForSlot(true, true, true, true, true);
        assert(g1.count == 10 && indexOf(g1, Gen1::Action::CopyToVault) == indexOf(g1, Gen1::Action::ExportFile) + 1);
        assert(hasLabel(Menu::gen1(g1), Menu::kCopyToVaultLabel));
        const auto e1 = Gen1::actionsForSlot(false, false, false, true, true);
        assert(indexOf(e1, Gen1::Action::WithdrawFromVault) == indexOf(e1, Gen1::Action::ImportFile) + 1);
    }
    std::cout << "test_vault_actions_contract (" << (PokeBank::Vault::kEnabled ? "Vault build" : "default build")
              << "): PASS\n";
    return 0;
}
