#pragma once

// Round 11 (owner H25, safety clarification 2): an EXISTING Pokemon's recorded history (Met
// Location, Met Level, Pokerus, origin game) is preserved when its species is edited. When that
// history no longer fits the newly chosen species, it is FLAGGED in the editor and in Review --
// never deleted, never changed, and Keep is never refused because of a flag.
//
// This header is pure: each generation supplies the facts it can establish from data already in
// the repository (its own exact-game native location list, its catch templates with evolution
// ancestry, the record's levels), and gets zero or more short flags back. A flag says the
// combination is suspect / not verified and that the history was kept as recorded. It is never a
// legality verdict: the catch tables are wild-only and cannot judge gift, static, trade, egg or
// event histories, so a marker location (trade / egg / gift / event / special) is never called
// "not a listed catch", and nothing here ever says "illegal".

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::HistoryConsistency {

// The caution label shown beside the flags (editor data pane, Review note line).
inline constexpr const char* kCautionLabel = "Caution";

struct Facts {
    std::string_view species;          // the record's (new) species display name
    bool metLocationRecorded = false;  // a Met Location is stored (not the "none" / unset value)
    bool originKnown = false;          // the origin game's own native list is known to this generation
    bool nativeToOrigin = true;        // the location is in that list (only read when originKnown)
    bool marker = false;               // trade / egg / gift / event / special entry: not a catch claim
    bool catchDataAvailable = false;   // the repo has catch templates for the origin game
    bool listedCatch = false;          // the location is a catch template of the species or its ancestry
    uint8_t metLevel = 0;              // 0 (or 1 in Crystal: hatched) means not judged
    uint8_t level = 0;                 // the record's current level
    bool levelsKnown = false;          // both levels are the record's real values
};

[[nodiscard]] inline std::string notListedCatch(std::string_view species) {
    return "Met Location is not a listed catch for " + std::string(species) + " (history kept)";
}
inline constexpr const char* kNotNativeToOrigin = "Origin game has no record of this Met Location (history kept)";
inline constexpr const char* kMetLevelAboveLevel = "Met Level is above the current level (history kept)";

// Zero or more flags, in a fixed order. Nothing recorded: nothing to judge.
[[nodiscard]] inline std::vector<std::string> flags(const Facts& facts) {
    std::vector<std::string> out;
    if (facts.metLocationRecorded) {
        if (facts.originKnown && !facts.nativeToOrigin)
            out.emplace_back(kNotNativeToOrigin);   // not even a place of that game: no catch claim
        else if (!facts.marker && facts.catchDataAvailable && !facts.listedCatch)
            out.push_back(notListedCatch(facts.species));
    }
    // A Pokemon never loses levels, so a met level above the current level cannot be the record's
    // own history for this level; it is still kept exactly as recorded.
    if (facts.levelsKnown && facts.metLevel != 0 && facts.level != 0 && facts.metLevel > facts.level)
        out.emplace_back(kMetLevelAboveLevel);
    return out;
}

// One line for a data pane / Review note: the flags joined with "; ". Empty when nothing is flagged.
[[nodiscard]] inline std::string joined(const std::vector<std::string>& list) {
    std::string out;
    for (const auto& flag : list) {
        if (!out.empty()) out += "; ";
        out += flag;
    }
    return out;
}

} // namespace PokeBank::UIModel::HistoryConsistency
