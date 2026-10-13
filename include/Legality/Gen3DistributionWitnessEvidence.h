#pragma once
// L4 Stage 2: original Gen III distribution / gift candidate-local evidence.
// Not an event database or provenance attestation. No copyrighted source rows.
// Candidate mismatch NEVER implies global Invalid. Matching never authenticates
// an actual distribution, receipt, or a Pokemon's acquisition history.
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen3DistributionWitnessEvidence {

enum class Family : uint8_t {
    Unknown, Wishmaker, Channel, PCNY, PCJP, MystryMew,
    Wondercard, BerryFix, GameCubeGift, ShadowEncounter, EventIslandEncounter
};
enum class Mechanism : uint8_t {
    Unknown, PokemonGift, InGameEncounter, ServiceOnly, ShadowEncounter
};
enum class OriginalSource : uint8_t {
    Unknown, Ruby, Sapphire, Emerald, FireRed, LeafGreen, Colosseum, XD
};
enum class History : uint8_t {
    Unknown, IndependentlyVerifiedUnchanged, IndependentlyVerifiedFinal
};
enum class Status : uint8_t {
    UnavailableNative, UnknownOriginal, UnknownHistory, ConditionalMatch,
    CandidateMismatch
};
enum class SourceStatus : uint8_t {
    UnknownSource, UntrustedArchive, MissingPayloadDigest, UnknownOriginalTemplate, UnknownRelease,
    MissingReceipt, ConditionalSource, RouteContradicted
};
// Indices are stable for the standalone regression and future coordinator
// integrations. Statuses apply only to the ONE hypothesized original source.
enum class Field : uint8_t {
    Species, PID, TrainerIDs, TrainerGender, Language, Nickname, OTName,
    OriginGame, MetLevel, MetLocation, Ball, Moves, HeldItem,
    Egg, Fateful, RibbonBits, Count
};
inline constexpr std::size_t FIELD_COUNT = static_cast<std::size_t>(Field::Count);
using Nickname = std::array<std::byte, 10>;
using OTName = std::array<std::byte, 7>;
using Moves = std::array<uint16_t, 4>;
struct TrainerIDs { uint16_t tid=0, sid=0; constexpr bool operator==(const TrainerIDs&) const = default; };

template<class T> struct Witness {
    // The original value must be independently extracted from a source,
    // not filled with a plausible inferred/template value.
    bool independentlyKnown=false;
    T original{};
    History history=History::Unknown;
    T independentlyVerifiedFinal{};
};
struct Native {
    bool available=false;
    uint16_t speciesInternal=0;
    uint32_t pid=0;
    TrainerIDs ids{};
    uint8_t trainerGender=0, language=0, originGame=0;
    Nickname nickname{};
    OTName otName{};
    uint8_t metLevel=0, metLocation=0, ball=0;
    Moves moves{};
    uint16_t heldItem=0;
    bool egg=false, fateful=false;
    uint32_t ribbonBits=0;
};
struct OriginalCandidate {
    Family family=Family::Unknown;
    Mechanism mechanism=Mechanism::Unknown;
    OriginalSource originalSource=OriginalSource::Unknown;
    // A service/card/Unlock/Shadow mechanism must not masquerade as
    // a Pokemon gift. Event island encounters are not Wondercard gifts.
    bool exactRouteIndependentlyVerified=false;
    // The archive is original or an independently attested original conversion.
    // Reconstructed template rows cannot establish source authenticity.
    bool originalArchiveIndependentlyAuthenticated=false;
    bool originalPayloadDigestVerified=false;
    std::array<std::byte,32> originalPayloadSha256{};
    bool originalTemplateIndependentlyAttested=false;
    bool historicalReleaseIndependentlyCorroborated=false;
    bool individualHistoricalReceiptKnown=false;
    // Historical metadata transforms are *not* inferred from the present PKM.
    bool nicknameOrOTReencodingPossible=false;
    bool languageConversionPossible=false;
    Witness<uint16_t> speciesInternal;
    Witness<uint32_t> pid;
    Witness<TrainerIDs> ids;
    Witness<uint8_t> trainerGender, language;
    Witness<Nickname> nickname;
    Witness<OTName> otName;
    Witness<uint8_t> originGame, metLevel, metLocation, ball;
    Witness<Moves> moves;
    Witness<uint16_t> heldItem;
    Witness<bool> egg, fateful;
    Witness<uint32_t> ribbonBits;
};
struct Result {
    SourceStatus source=SourceStatus::UnknownSource;
    bool nativeAvailable=false;
    std::array<Status,FIELD_COUNT> fields{};
    uint32_t mismatchMask=0, conditionalMatchMask=0, unknownMask=0;
    // Immutable invariant: this witness never proves an event's authenticity
    // or establishes globally Valid/Invalid for an encountered Pokemon.
    bool authenticated=false;
    bool globallyInvalid=false;
    constexpr Status field(Field f) const noexcept {
        return fields[static_cast<std::size_t>(f)];
    }
    constexpr bool hasCandidateMismatch() const noexcept { return mismatchMask!=0; }
};
constexpr bool compatible(Family family, Mechanism mechanism) noexcept {
    switch(family) {
    case Family::Wishmaker: case Family::Channel: case Family::PCNY:
    case Family::PCJP: case Family::MystryMew:
    case Family::GameCubeGift:
        return mechanism==Mechanism::PokemonGift;
    case Family::Wondercard:
        // A card can deliver a gift or unlock an original in-game encounter;
        // its mere presence cannot establish which without source evidence.
        return mechanism==Mechanism::PokemonGift || mechanism==Mechanism::InGameEncounter;
    case Family::BerryFix:
        return mechanism==Mechanism::ServiceOnly;
    case Family::ShadowEncounter:
        return mechanism==Mechanism::ShadowEncounter;
    case Family::EventIslandEncounter:
        return mechanism==Mechanism::InGameEncounter;
    case Family::Unknown:
        return false;
    }
    return false;
}
constexpr SourceStatus sourceStatus(const OriginalCandidate& c) noexcept {
    if(c.family==Family::Unknown || c.mechanism==Mechanism::Unknown ||
       c.originalSource==OriginalSource::Unknown ||
       !c.exactRouteIndependentlyVerified)return SourceStatus::UnknownSource;
    if(!compatible(c.family,c.mechanism))return SourceStatus::RouteContradicted;
    if(!c.originalArchiveIndependentlyAuthenticated)return SourceStatus::UntrustedArchive;
    bool nonzero=false;
    for(auto byte:c.originalPayloadSha256)if(byte!=std::byte{0})nonzero=true;
    if(!c.originalPayloadDigestVerified || !nonzero)return SourceStatus::MissingPayloadDigest;
    if(!c.originalTemplateIndependentlyAttested)return SourceStatus::UnknownOriginalTemplate;
    if(!c.historicalReleaseIndependentlyCorroborated)return SourceStatus::UnknownRelease;
    if(!c.individualHistoricalReceiptKnown)return SourceStatus::MissingReceipt;
    return SourceStatus::ConditionalSource;
}
template<class T> constexpr Status compare(const Witness<T>& witness,
                                         const T& observed) noexcept {
    if(!witness.independentlyKnown)return Status::UnknownOriginal;
    switch(witness.history) {
    case History::IndependentlyVerifiedUnchanged:
        return observed==witness.original?Status::ConditionalMatch:Status::CandidateMismatch;
    case History::IndependentlyVerifiedFinal:
        return observed==witness.independentlyVerifiedFinal?
            Status::ConditionalMatch:Status::CandidateMismatch;
    case History::Unknown: return Status::UnknownHistory;
    }
    return Status::UnknownHistory;
}
constexpr Result evaluate(const Native& n,const OriginalCandidate& c) noexcept {
    Result out{};
    out.source=sourceStatus(c);
    out.nativeAvailable=n.available;
    if(!n.available)return out;
    if(!c.originalTemplateIndependentlyAttested || !c.originalArchiveIndependentlyAuthenticated ||
       !c.originalPayloadDigestVerified || !c.exactRouteIndependentlyVerified ||
       c.originalSource==OriginalSource::Unknown ||
       c.family==Family::Unknown || c.mechanism==Mechanism::Unknown ||
       !compatible(c.family,c.mechanism) ||
       c.originalPayloadSha256==std::array<std::byte,32>{}) {
        out.fields.fill(Status::UnknownOriginal);
        out.unknownMask=(1u<<FIELD_COUNT)-1u;
        return out;
    }
    out.fields = {
        compare(c.speciesInternal,n.speciesInternal),compare(c.pid,n.pid),
        compare(c.ids,n.ids),compare(c.trainerGender,n.trainerGender),
        c.languageConversionPossible ? Status::UnknownHistory : compare(c.language,n.language),
        c.nicknameOrOTReencodingPossible ? Status::UnknownHistory : compare(c.nickname,n.nickname),
        c.nicknameOrOTReencodingPossible ? Status::UnknownHistory : compare(c.otName,n.otName),
        compare(c.originGame,n.originGame),compare(c.metLevel,n.metLevel),
        compare(c.metLocation,n.metLocation),compare(c.ball,n.ball),
        compare(c.moves,n.moves),compare(c.heldItem,n.heldItem),
        compare(c.egg,n.egg),compare(c.fateful,n.fateful),
        compare(c.ribbonBits,n.ribbonBits)
    };
    for(std::size_t i=0;i<FIELD_COUNT;++i){
        const uint32_t bit=1u<<i;
        if(out.fields[i]==Status::CandidateMismatch)out.mismatchMask|=bit;
        else if(out.fields[i]==Status::ConditionalMatch)out.conditionalMatchMask|=bit;
        else out.unknownMask|=bit;
    }
    return out;
}
static_assert(FIELD_COUNT==16);
static_assert(!compatible(Family::BerryFix,Mechanism::PokemonGift));
static_assert(compatible(Family::BerryFix,Mechanism::ServiceOnly));
static_assert(!compatible(Family::ShadowEncounter,Mechanism::PokemonGift));
static_assert(compatible(Family::Channel,Mechanism::PokemonGift));
static_assert(compatible(Family::Wondercard,Mechanism::InGameEncounter));
static_assert(compare(Witness<uint8_t>{true,7,History::Unknown,0},uint8_t{8})==Status::UnknownHistory);
} // namespace Legality::Gen3DistributionWitnessEvidence
