#pragma once
// L4 Stage 1: original Gen I/II static, gift, trade and distribution histories.
// Candidate-local only. Matching never proves a real distribution or receipt;
// a mismatch NEVER establishes globally Invalid. All source rows and all
// evolution/transfer witnesses must be independently supplied and reviewed.
#include <array>
#include <cstdint>

namespace Legality::Gen12GiftHistoryEvidence {

enum class Game : uint8_t { Unknown, Red, Blue, Yellow, Gold, Silver, Crystal };
enum class Kind : uint8_t { Unknown, Static, Gift, NPCTrade, StadiumGift, PCNYGift };
enum class History : uint8_t { Unknown, Unchanged, IndependentlyVerifiedChange };
enum class Crossing : uint8_t { Unknown, NoCrossingVerified, SnapshotVerified };
enum class LevelRule : uint8_t { Unknown, Fixed, OfferedTradeLevel };
enum class Disposition : uint8_t { Unknown, CandidateContradicted, ConditionalCandidate };
enum Field : uint32_t {
    Source = 1u << 0, Species = 1u << 1, Level = 1u << 2,
    TrainerId = 1u << 3, TrainerName = 1u << 4, Nickname = 1u << 5,
    Moves = 1u << 6, Capsule = 1u << 7, Route = 1u << 8
};
template <class T> struct Datum {
    bool known = false;
    T value{};
};
using Name = std::array<uint8_t, 11>; // raw native game-font bytes, never host UTF-8
using Moves4 = std::array<uint16_t, 4>;

struct GiftRow {
    Game game = Game::Unknown;
    Kind kind = Kind::Unknown;
    // Attest original per-version mechanics and precise row independently.
    // They are inputs, not claims automatically verified by this helper.
    bool originalMechanicIndependentlyVerified = false;
    bool originalRowIndependentlyVerified = false;
    bool originalDistributionAttested = false; // only for event families
    bool originalReceiptKnown = false; // not inferable from an extracted PKM
    Datum<uint16_t> species;
    LevelRule levelRule = LevelRule::Unknown;
    Datum<uint8_t> level;
    Datum<uint16_t> trainerId;
    Datum<Name> trainerName;
    Datum<Name> nickname;
    Datum<Moves4> moves;
};
struct Observation {
    Game game = Game::Unknown;
    Datum<uint16_t> species;
    Datum<uint8_t> level;
    Datum<uint16_t> trainerId;
    Datum<Name> trainerName;
    Datum<Name> nickname;
    Datum<Moves4> moves;
};
struct RouteWitness {
    // For a changed species, independent complete evolution-path evidence
    // must provide the endpoint. No guessed species family equivalences.
    History speciesHistory = History::Unknown;
    Datum<uint16_t> independentlyVerifiedEvolutionEndpoint;
    History trainerNameHistory = History::Unknown;
    History nicknameHistory = History::Unknown;
    History moveHistory = History::Unknown;
    Datum<Moves4> independentlyVerifiedFinalMoves;
    Datum<uint8_t> offeredTradeLevel; // original NPC trade input, not a fixed gift level
    Crossing timeCapsule = Crossing::Unknown;
    // Exact party snapshot at the documented crossing, not current moves.
    Datum<uint16_t> crossingSpecies;
    Datum<Moves4> crossingMoves;
};
struct Result {
    Disposition disposition = Disposition::Unknown;
    uint32_t contradicted = 0;
    uint32_t missing = 0;
    // Always false: matching fields / receipts cannot authenticate a Pokemon.
    bool authenticated = false;
};

constexpr bool isGen1(Game g) noexcept {
    return g == Game::Red || g == Game::Blue || g == Game::Yellow;
}
constexpr bool isGen2(Game g) noexcept {
    return g == Game::Gold || g == Game::Silver || g == Game::Crystal;
}
constexpr bool supportedKind(Game game, Kind kind) noexcept {
    if (!isGen1(game) && !isGen2(game)) return false;
    if (kind == Kind::Static || kind == Kind::Gift || kind == Kind::NPCTrade ||
        kind == Kind::StadiumGift) return true;
    return isGen2(game) && kind == Kind::PCNYGift;
}
constexpr bool requiresDistributionProof(Kind kind) noexcept {
    return kind == Kind::StadiumGift || kind == Kind::PCNYGift;
}
constexpr bool legalSpeciesAtCrossing(uint16_t species) noexcept {
    return species >= 1 && species <= 151;
}
constexpr bool legalMovesAtCrossing(const Moves4& moves) noexcept {
    for (const auto move : moves)
        if (move > 165) return false; // 0 is an empty slot.
    return true;
}
constexpr bool validNativeSpecies(Game game, uint16_t species) noexcept {
    return species >= 1 && species <= (isGen1(game) ? 151u : 251u);
}
constexpr bool validLevel(uint8_t level) noexcept {
    return level >= 1 && level <= 100;
}

constexpr Result evaluate(const GiftRow& source, const Observation& observed,
                          const RouteWitness& route) noexcept {
    Result out{};
    const auto unknown = [&](Field field) { out.missing |= field; };
    const auto reject = [&](Field field) { out.contradicted |= field; };
    if (!supportedKind(source.game, source.kind) ||
        !source.originalMechanicIndependentlyVerified ||
        !source.originalRowIndependentlyVerified ||
        !source.species.known || !validNativeSpecies(source.game, source.species.value) ||
        (source.level.known && !validLevel(source.level.value)) ||
        (source.kind == Kind::NPCTrade ? source.levelRule != LevelRule::OfferedTradeLevel
                                       : source.levelRule == LevelRule::OfferedTradeLevel) ||
        (requiresDistributionProof(source.kind) &&
         (!source.originalDistributionAttested || !source.originalReceiptKnown))) {
        unknown(Source);
    }
    if (!supportedKind(source.game, source.kind) ||
        (!isGen1(observed.game) && !isGen2(observed.game))) {
        unknown(Route);
    }
    // Corrupt/unknown native observations cannot be validated by this layer.
    if (!source.species.known || !observed.species.known ||
        !validNativeSpecies(source.game, source.species.value) ||
        !validNativeSpecies(observed.game, observed.species.value)) {
        unknown(Species);
    } else if (source.species.value != observed.species.value) {
        if (route.speciesHistory == History::Unchanged ||
            (route.speciesHistory == History::IndependentlyVerifiedChange &&
             route.independentlyVerifiedEvolutionEndpoint.known &&
             route.independentlyVerifiedEvolutionEndpoint.value != observed.species.value))
            reject(Species);
        else if (route.speciesHistory != History::IndependentlyVerifiedChange ||
                 !route.independentlyVerifiedEvolutionEndpoint.known)
            unknown(Species);
    } else if (route.speciesHistory == History::IndependentlyVerifiedChange &&
               (!route.independentlyVerifiedEvolutionEndpoint.known ||
                route.independentlyVerifiedEvolutionEndpoint.value != observed.species.value)) {
        unknown(Species);
    }
    Datum<uint8_t> originLevel{};
    if (source.levelRule == LevelRule::Fixed && source.kind != Kind::NPCTrade)
        originLevel = source.level;
    else if (source.levelRule == LevelRule::OfferedTradeLevel &&
             source.kind == Kind::NPCTrade)
        originLevel = route.offeredTradeLevel;
    if (!originLevel.known || !observed.level.known ||
        !validLevel(originLevel.value) || !validLevel(observed.level.value))
        unknown(Level);
    else if (observed.level.value < originLevel.value)
        reject(Level); // original retail levels do not normally decrease
    if (!source.trainerId.known || !observed.trainerId.known)
        unknown(TrainerId);
    else if (source.trainerId.value != observed.trainerId.value)
        reject(TrainerId); // trade / evolution do not rewrite OT trainer ID
    if (!source.trainerName.known || !observed.trainerName.known)
        unknown(TrainerName);
    else if (source.trainerName.value != observed.trainerName.value) {
        if (route.trainerNameHistory == History::Unchanged) reject(TrainerName);
        else unknown(TrainerName); // locale, trade, glitches need investigation
    }
    if (!source.nickname.known || !observed.nickname.known)
        unknown(Nickname);
    else if (source.nickname.value != observed.nickname.value) {
        if (route.nicknameHistory == History::Unchanged) reject(Nickname);
        else unknown(Nickname); // nickname / evolution history mutable
    }
    if (!source.moves.known || !observed.moves.known)
        unknown(Moves);
    else if (route.moveHistory == History::Unchanged) {
        if (source.moves.value != observed.moves.value) reject(Moves);
    } else if (route.moveHistory == History::IndependentlyVerifiedChange) {
        if (!route.independentlyVerifiedFinalMoves.known) unknown(Moves);
        else if (route.independentlyVerifiedFinalMoves.value != observed.moves.value)
            reject(Moves);
    } else unknown(Moves); // individual learnability is not full history

    if (isGen1(source.game) != isGen1(observed.game)) {
        if (route.timeCapsule != Crossing::SnapshotVerified) unknown(Capsule);
        else if (!route.crossingSpecies.known || !route.crossingMoves.known)
            unknown(Capsule);
        else if (!legalSpeciesAtCrossing(route.crossingSpecies.value) ||
                 !legalMovesAtCrossing(route.crossingMoves.value)) reject(Capsule);
    } else if (route.timeCapsule == Crossing::SnapshotVerified) {
        // Even with same-generation endpoints, a documented round trip must
        // satisfy Time Capsule constraints at the crossing itself.
        if (!route.crossingSpecies.known || !route.crossingMoves.known)
            unknown(Capsule);
        else if (!legalSpeciesAtCrossing(route.crossingSpecies.value) ||
                 !legalMovesAtCrossing(route.crossingMoves.value)) reject(Capsule);
    }
    // A missing or untrusted source must NEVER produce a negative assertion.
    if (out.missing & (Source | Route)) {
        out.contradicted = 0;
        out.disposition = Disposition::Unknown;
    } else if (out.contradicted) {
        out.disposition = Disposition::CandidateContradicted;
    } else if (out.missing) {
        out.disposition = Disposition::Unknown;
    } else {
        out.disposition = Disposition::ConditionalCandidate;
    }
    return out;
}
static_assert(legalSpeciesAtCrossing(151));
static_assert(!legalSpeciesAtCrossing(152));
static_assert(legalMovesAtCrossing({1, 2, 165, 0}));
static_assert(!legalMovesAtCrossing({1, 2, 166, 0}));
static_assert(supportedKind(Game::Crystal, Kind::PCNYGift));
static_assert(!supportedKind(Game::Red, Kind::PCNYGift));
} // namespace Legality::Gen12GiftHistoryEvidence
