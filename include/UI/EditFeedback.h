#pragma once

// The one Gen I-V rule for status toasts after an in-place editor edit (owner Round 11 H22:
// "when i switch unowns i get a pop in middle bottom about stat changes and i dont need it ...
// i also get the pop up for rerolling ... can we fix this for all gens").
//
// A ROUTINE edit changes a value the player is looking at: the Unown letter, a DV / IV / EV
// reroll, the Shiny toggle. When it succeeds the changed value, sprite and calculated stats ARE
// the feedback, so it posts no bottom-centre toast (the same class as the earlier Gen V Language
// fix, CG79). What still posts, in every generation:
//   * Unchanged - the edit could not change anything (a reroll with no other DVs that keep the
//                 shiny state / gender / letter, a cancelled question): without a line the
//                 press would look broken (owner Round 6 B07);
//   * Warning   - it worked but moved something the player did not ask for (a Gen II shiny
//                 rewrite that changes gender for the species ratio, or the Unown letter);
//   * Refused   - errors and refusals.
// Every other edit (Kind::Field) keeps its generation's own message: commits (Keep / Review /
// Export / Discard), derived-field hints and PID notices are not routine edits.
//
// Round 12 R12-002 (owner H03: "when a pokemon is picked up we have a little boc pop up telling the
// controls we dont need that we have the footer for that ... the only pop we need is \"there is no
// pokemon in this slot to move\""; H04: clone "has a pop up menu thing on the botom about cloned
// pop dont need it"): Kind::Carry is the Gen I-V box carry (ClassicCarryRules). Picking up, placing,
// a multi-move, a placed clone, a cancel and a key the carry swallows are routine: the moved sprite,
// the carry footer and the grid ARE the feedback. Refusals (no Pokemon there, no room for the group,
// occupied / protected / source changed) and failures still post. settled() also retires a refusal
// still on screen when the carry ends, so "That slot is occupied" never lingers over a placed Pokemon.
//
// Display only: nothing here stages, encrypts or writes.

#include <string>

namespace PokeBank::UIModel::EditFeedback {

enum class Kind : unsigned char { Field, Reroll, UnownLetter, Shiny, Carry };
enum class Outcome : unsigned char { Applied, Unchanged, Warning, Refused };

[[nodiscard]] constexpr bool routine(Kind kind) noexcept { return kind != Kind::Field; }

// Whether this outcome of this kind of edit posts a status toast.
[[nodiscard]] constexpr bool postsStatus(Kind kind, Outcome outcome) noexcept {
    return !(outcome == Outcome::Applied && routine(kind));
}

// Post `text` for `frames` only when the rule allows it; true when it was posted. `Screen` is
// anything with postStatus(const std::string&, int) (TrainerViewScreen, or a host-test fake).
template <class Screen>
bool report(Screen& screen, Kind kind, Outcome outcome, const std::string& text, int frames) {
    if (!postsStatus(kind, outcome)) return false;
    screen.postStatus(text, frames);
    return true;
}

// A routine edit that succeeded: the documented call site for "nothing to say". Posts nothing.
template <class Screen>
constexpr void applied(Screen&, Kind kind) noexcept {
    static_cast<void>(kind);
}

// A routine step that ENDS an interaction (a carry placed or cancelled): posts nothing and clears
// whatever transient line the interaction left (a refusal the player has now answered).
template <class Screen>
void settled(Screen& screen, Kind kind) {
    if (routine(kind)) screen.postStatus(std::string(), 0);
}

static_assert(!postsStatus(Kind::Reroll, Outcome::Applied) && !postsStatus(Kind::UnownLetter, Outcome::Applied) &&
              !postsStatus(Kind::Shiny, Outcome::Applied) && !postsStatus(Kind::Carry, Outcome::Applied));
static_assert(postsStatus(Kind::Reroll, Outcome::Unchanged) && postsStatus(Kind::Shiny, Outcome::Warning) &&
              postsStatus(Kind::UnownLetter, Outcome::Refused) && postsStatus(Kind::Field, Outcome::Applied) &&
              postsStatus(Kind::Carry, Outcome::Refused));

} // namespace PokeBank::UIModel::EditFeedback
