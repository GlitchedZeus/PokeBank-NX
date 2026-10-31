#pragma once

// Shared species-change policy for a draft's history fields, Generation II-V (Round 11, owner H25).
//
// The owner's case: a Create Pikachu given Trophy Garden and Pokerus, then changed to Arbok, kept
// both, because every editor rewrote only the identity fields and each generation decided the
// rest on its own. One rule now, applied at every species-apply point:
//
//  - Create (a new Pokemon nobody has caught yet): its history is draft-authored, so Pokerus is
//    reset to none, and a Met Location the user chose is kept only while it is still a valid
//    encounter for the new species or one of its ancestors (a Pichu/Pikachu location stays for
//    Raichu). Otherwise Met Location / Met Level go back to the draft's "not chosen" values and
//    the user is told to choose a new encounter.
//  - Edit (an existing Pokemon): its Met Location and Pokerus are its own recorded history, which
//    the editor never clears behind the user's back (the encounter tables are wild-only and
//    cannot judge gift / static / trade / event histories). Nothing is reset.
//
// Gen I has neither field. This header decides; each generation applies the decision to its own
// draft with its own setters, so byte layout, checksums and commit paths are untouched.

#include <cstdint>

namespace PokeBank::UIModel::SpeciesChangeHistoryPolicy {

enum class Mode : uint8_t { Create, Edit };

struct Decision {
    bool resetPokerus = false;
    bool resetMetLocation = false;  // Met Location and Met Level back to "not chosen"
};

inline constexpr const char* kChooseEncounterStatus = "Species changed; choose a new legitimate encounter";

// metLocationChosen: the draft holds a Met Location the user picked (not the unset placeholder).
// stillValid(): true when that location is an encounter of the new species or its ancestry; it is
// only consulted for a Create draft with a chosen location.
template <class StillValid>
[[nodiscard]] Decision reconcile(Mode mode, bool metLocationChosen, StillValid&& stillValid) {
    Decision decision;
    if (mode != Mode::Create) return decision;
    decision.resetPokerus = true;
    decision.resetMetLocation = metLocationChosen && !stillValid();
    return decision;
}

} // namespace PokeBank::UIModel::SpeciesChangeHistoryPolicy
