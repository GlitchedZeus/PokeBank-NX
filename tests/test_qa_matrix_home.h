// PokeBank NX application QA: Home party-strip facts for the Gen I-V acceptance matrix, DERIVED from
// the production adapters (include/Integration/Common/HomePartyPreview.h fromGen1/2/3, the ones
// SaveSelectScreen.cpp feeds into the Home strip) instead of hard-coded strings, so the matrix
// cannot drift from the code. Each probe hands the adapter a record that HAS the property (a level,
// an alternate form, a shiny PID/DV set, a gendered species) and reports "kept" only if the adapter
// output carries it; a property the format does not define is "n/a". `fact(key, value)` records it.
#pragma once

#include "Integration/Common/HomePartyPreview.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "test_qa_matrix.h"

#include <string>

namespace qamatrix::home {

namespace Preview = PokeVault::Integration::HomePartyPreview;
using Slot = PokeVault::Source::HomeCardPartySlot;

inline const char* kept(bool ok) { return ok ? "kept" : "dropped"; }

// Gen I: the record defines species and level only (Gen1ReadOnlySave.h PokemonRecord): no shiny,
// gender or forms exist in the format.
template <class Fact>
void recordGen1(Fact&& fact) {
    PokeVault::Integration::Gen1::PokemonRecord p{};
    p.species = 25;
    p.level = 42;
    const Slot s = Preview::fromGen1(p);
    fact("home.party.level", kept(s.species == 25 && s.level == 42));
    fact("home.party.form", "n/a");
    fact("home.party.shiny", "n/a");
    fact("home.party.gender", s.gender == Slot::GenderUnknown ? "n/a" : "kept");
}

// Gen II: Unown letter and shiny come from the record (DV-derived in Gen2ReadOnlySave), gender from
// the Attack DV against the Gen II ratio (Gen2PersonalData genderFromAttackDV).
template <class Fact>
void recordGen2(Fact&& fact) {
    namespace G2 = PokeVault::Integration::Gen2;
    G2::PokemonRecord unown{};
    unown.species = 201;
    unown.level = 42;
    unown.form = 5;
    unown.shiny = true;
    const Slot u = Preview::fromGen2(unown);
    G2::PokemonRecord pikachu{};
    pikachu.species = 25;
    pikachu.level = 42;
    pikachu.dvs = {15, 15, 15, 15, 15};
    const auto expectedGender = G2::genderFromAttackDV(25, pikachu.dvs[1]);
    const Slot g = Preview::fromGen2(pikachu);
    fact("home.party.level", kept(u.level == 42 && g.level == 42));
    fact("home.party.form", kept(u.form == 5));
    fact("home.party.shiny", u.shiny ? "shown" : "hidden");
    fact("home.party.gender",
           kept(expectedGender != G2::PokemonGender::Unknown && g.gender == static_cast<uint8_t>(expectedGender)));
}

// Gen III: party level from the 100-byte party extension (PK3 0x54; boxed 80-byte records have
// none), Unown letter from the PID (Gen3PidSearch::unownForm), shiny from PID/TID/SID, gender from
// the PID against the Gen III ratio (PokemonRecord::gender()).
template <class Fact>
void recordGen3(Fact&& fact) {
    namespace G3 = PokeVault::Integration::Gen3;
    constexpr uint16_t tid = 12345, sid = 54321;
    // A shiny PID whose Unown letter is not A: hi = tid ^ sid ^ lo makes (tid^sid^hi^lo) == 0.
    uint32_t pid = 0;
    for (uint16_t lo = 1; lo != 0; ++lo) {
        const uint16_t hi = static_cast<uint16_t>(tid ^ sid ^ lo);
        pid = (static_cast<uint32_t>(hi) << 16) | lo;
        if (::Conversion::Gen3PidSearch::unownForm(pid) != 0) break;
    }
    G3::PokemonRecord unown;
    unown.species = 201;
    unown.pid = pid;
    unown.tid = tid;
    unown.sid = sid;
    unown.originalBytes.assign(Preview::kGen3PartySize, 0);
    unown.originalBytes[Preview::kGen3PartyLevelOffset] = 42;
    const Slot u = Preview::fromGen3(unown);
    const uint8_t letter = ::Conversion::Gen3PidSearch::unownForm(pid);
    G3::PokemonRecord pikachu = unown;
    pikachu.species = 25;
    const Slot g = Preview::fromGen3(pikachu);
    const auto expectedGender = pikachu.gender();
    fact("home.party.level", kept(u.level == 42));
    fact("home.party.form", kept(letter != 0 && u.form == letter));
    fact("home.party.shiny", u.shiny ? "shown" : "hidden");
    fact("home.party.gender",
           kept(expectedGender != G3::Gen3Gender::Unknown && g.gender == static_cast<uint8_t>(expectedGender)));
}

} // namespace qamatrix::home
