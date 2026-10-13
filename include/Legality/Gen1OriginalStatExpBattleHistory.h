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
enum class EventKind : std::uint8_t { Battle, HPUp, Protein, Iron, Carbos, Calcium };
enum class Status : std::uint8_t {
    UnknownSource, MalformedAttestedRecord, UnknownOriginalTimeline,
    IncompleteEventCoverage, ContradictedAttestedEvent, 
    ConditionalOriginalBattleHistory
};
struct Battle {
    EventKind kind=EventKind::Battle;
    std::uint64_t originalEventId=0;
    std::uint64_t originalMonotonicEventSequence=0;
    G::SourceGame originalGame=G::SourceGame::Red;
    std::uint16_t defeatedNationalSpecies=0;
    bool originalDefeatIndependentlyAttested=false;
    bool singleEligibleRecipientIndependentlyAttested=false;
    // Original wPartyGainExpFlags count. When source-attested, divided
    // enemy base stats use floor(base / recipient count) in retail RBY.
    std::uint8_t originalEligibleRecipientCount=0;
    bool originalRecipientCountIndependentlyAttested=false;
    // A successful retail HP Up, Protein, Iron, Carbos, Calcium use
    // increments ONLY the high byte of the corresponding Stat Exp by 10.
    bool originalVitaminItemIndependentlyAttested=false;
    bool originalVitaminConsumedIndependentlyAttested=false;
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
// Source-attested retail 1..6 eligible-recipient battle gains and original
// successful vitamins. Never infer event identities/counts/Exp All state
// from the five final Stat Experience values.
Result evaluate(const G::PokemonRecord& record, const History& history) noexcept;
} // namespace Legality::Gen1OriginalStatExpBattleHistory
