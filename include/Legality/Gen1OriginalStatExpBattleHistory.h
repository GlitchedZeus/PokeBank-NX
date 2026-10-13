#pragma once
// L2-owned original Gen I, NOT breeding or modern EV legality.
// Pinned pret/pokered af519899719f0754965776faac0e836a3b906e6d:
// engine/battle/experience.asm, GainExperience/.gainStatExpLoop.
// Only a witnessed *single recipient without Exp. All* can be replayed
// using exact original personal base stats; party splits are Unknown.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <array>
#include <cstdint>
namespace Legality::Gen1OriginalStatExpBattleHistory {
namespace G=PokeVault::Integration::Gen1;
enum class Status : std::uint8_t {
    UnknownSource, MalformedAttestedRecord, UnknownOriginalTimeline,
    IncompleteEventCoverage, ContradictedAttestedEvent, 
    ConditionalOriginalBattleHistory
};
struct Battle {
    std::uint64_t originalEventId=0;
    G::SourceGame originalGame=G::SourceGame::Red;
    std::uint16_t defeatedNationalSpecies=0;
    bool originalDefeatIndependentlyAttested=false;
    bool singleEligibleRecipientIndependentlyAttested=false;
    bool expAllAbsentIndependentlyAttested=false;
    bool sourceSpeciesIdentityIndependentlyAttested=false;
    bool originalOrderingIndependentlyAttested=false;
    std::array<std::uint16_t,5> originalAfterStatExp{};
    bool originalAfterStatExpIndependentlyAttested=false;
};
struct History {
    bool strictPK1IndependentlyVerified=false;
    bool originalInitialSnapshotIndependentlyAttested=false;
    bool allOriginalStatExpEventsIndependentlyEnumerated=false;
    G::SourceGame sourceGame=G::SourceGame::Red;
    std::array<std::uint16_t,5> originalBeforeStatExp{};
    std::array<Battle,16> events{};
    std::uint8_t count=0;
};
struct Result {
    Status status=Status::UnknownSource;
    std::array<std::uint16_t,5> replayed{};
    std::uint8_t eventsReplayed=0;
    std::uint16_t firstContradictionEvent=0;
};
// Source-attested RETAIL SINGLE-RECIPIENT gain; never infer the original
// battle list or division from the final five Stat Experience words.
Result evaluate(const G::PokemonRecord& record, const History& history) noexcept;
} // namespace Legality::Gen1OriginalStatExpBattleHistory
