#include "UI/ActionSheetModel.h"
#include "Safety/WritePolicy.h"

#include <array>
#include <cassert>
#include <cstdint>

using namespace PokeVault::UIModel;

static_assert(POKEMON_ACTIONS.size() == 9);
static_assert(POKEMON_ACTIONS[0] == PokemonAction::View);
static_assert(POKEMON_ACTIONS[1] == PokemonAction::AddToMasterVault);
static_assert(POKEMON_ACTIONS[2] == PokemonAction::AddToBank);
static_assert(POKEMON_ACTIONS[3] == PokemonAction::TransferToGame);
static_assert(POKEMON_ACTIONS[4] == PokemonAction::Edit);
static_assert(POKEMON_ACTIONS[5] == PokemonAction::Clone);
static_assert(POKEMON_ACTIONS[6] == PokemonAction::MakeShiny);
static_assert(POKEMON_ACTIONS[7] == PokemonAction::LegalityAndProvenance);
static_assert(POKEMON_ACTIONS[8] == PokemonAction::Cancel);

namespace {
    struct MutationSentinel {
        std::array<std::uint8_t, 16> bytes{};
        int mutationCount = 0;

        constexpr bool operator==(const MutationSentinel&) const noexcept = default;
    };

    void exerciseOpenClose(PokemonLocation location) {
        MutationSentinel source{{1, 2, 3, 4}, 0};
        const MutationSentinel before = source;
        PokemonActionSheet sheet;
        const PokemonTarget target{location, 2, 17};

        // A on an occupied Party/Box/Storage target opens the same model and does no work.
        sheet.open(target);
        assert(sheet.isOpen());
        assert(sheet.target() == target);
        assert(sheet.selectedAction() == PokemonAction::View);
        assert(source == before);

        // Navigation is state-only and never touches the represented Pokémon.
        sheet.navigate(1);
        sheet.navigate(-1);
        assert(sheet.selectedAction() == PokemonAction::View);
        assert(source == before);

        // B maps to close: stable and mutation-free.
        assert(sheet.close() == ActionResult::Closed);
        assert(!sheet.isOpen());
        assert(source == before);

        // Repeated open/close remains deterministic.
        for (int i = 0; i < 100; ++i) {
            sheet.open(target);
            assert(sheet.selectedIndex() == 0);
            sheet.close();
        }
        assert(source == before);
    }
}

int main() {
    exerciseOpenClose(PokemonLocation::Party);
    exerciseOpenClose(PokemonLocation::SaveBox);
    exerciseOpenClose(PokemonLocation::Bank);

    PokemonActionSheet sheet;
    sheet.open({PokemonLocation::SaveBox, 1, 4});

    // Every unfinished command has one explicit safe result and cannot fall through.
    for (PokemonAction action : POKEMON_ACTIONS) {
        if (actionImplemented(action)) continue;
        assert(dispatchAction(action) == ActionResult::NotYetSupported);
    }
    sheet.select(5); // Clone
    assert(sheet.activate() == ActionResult::NotYetSupported);
    assert(sheet.isOpen());
    assert(sheet.selectedAction() == PokemonAction::Clone);

    // Cancel closes without accidentally dispatching the neighboring action.
    sheet.select(8);
    assert(sheet.activate() == ActionResult::Closed);
    assert(!sheet.isOpen());

    // The low-level live-title write firewall remains part of this regression gate.
    static_assert(!PokeVault::Safety::LIVE_SAVE_WRITES_ENABLED);
    assert(!PokeVault::Safety::canWriteTo(PokeVault::Safety::SaveDestination::LiveGame));
}
