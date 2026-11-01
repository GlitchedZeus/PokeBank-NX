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
//    cannot judge gift / static / trade / event histories). Nothing is reset; a combination that
//    no longer fits the new species is flagged instead (UI/HistoryConsistency.h).
//
// A Create reset is never silent: the species-apply point posts resetStatus(), naming what was
// reset (owner clarification 2: new drafts reset properly, and visibly).
//
// Gen I has neither field. This header decides; each generation applies the decision to its own
// draft with its own setters, so byte layout, checksums and commit paths are untouched.

#include <cstdint>
#include <string>

namespace PokeBank::UIModel::SpeciesChangeHistoryPolicy {

enum class Mode : uint8_t { Create, Edit };

struct Decision {
    bool resetPokerus = false;
    bool resetMetLocation = false;  // Met Location and Met Level back to "not chosen"
};

// Round 11 follow-up: the status names what was reset and says what to do; it no longer says
// "legitimate", which read as a legality claim the encounter tables cannot make.
inline constexpr const char* kChooseEncounterStatus = "Species changed: Met Location needs a new choice";

// The visible Create status after a species change: what was reset, or "" when nothing was.
// `metLocationRequired` is false for Crystal caught data, which stays optional: it was cleared, and
// no new choice is demanded.
[[nodiscard]] inline std::string resetStatus(bool pokerusCleared, bool metLocationReset,
                                             bool metLocationRequired = true) {
    if (!pokerusCleared && !metLocationReset) return {};
    std::string text = "Species changed: ";
    if (pokerusCleared) text += "Pokérus cleared";
    if (pokerusCleared && metLocationReset) text += " and ";
    if (metLocationReset) text += metLocationRequired ? "Met Location needs a new choice" : "Met Location cleared";
    return text;
}

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
