#pragma once
// L4 Gen V/VI PKHeX-first structural evidence, never an actual receipt.
// Source layout: PKHeX.Core/MysteryGifts/PGF.cs, WC6.cs, WC6Full.cs.
// Ball and distribution restriction decisions require external evidence.
// No source writes, code/assets from retail distribution, or PK5 codec.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen56MysteryGiftSourceEvidence {
using Bytes=std::span<const std::byte>;
using Digest=std::array<std::byte,32>;
enum class Format:uint8_t { Unknown, PGF, WC6, WC6Full };
enum class State:uint8_t { Unavailable, NonPokemon, PokemonProjection };
enum class Finding:uint8_t { UnknownNative, UnknownGift, UnknownSource,
    UnknownReceipt, UnknownHistoricalChange, UnknownRestrictions,
    CandidateExcluded, ConditionalCandidate };
struct Projection {
    State state=State::Unavailable;
    Format format=Format::Unknown;
    uint16_t card=0,species=0,tid=0,sid=0;
    uint8_t origin=0,language=0,ball=0,egg=0;
    uint16_t ribbons=0;
    bool versionRestrictionInWrapper=false,languageRestrictionInWrapper=false;
    uint8_t restrictVersion=0,restrictLanguage=0;
    // A serialized card's used bit and date are not original receipt evidence.
};
constexpr uint8_t u8(Bytes v,size_t i) noexcept {return std::to_integer<uint8_t>(v[i]);}
constexpr uint16_t le16(Bytes v,size_t i) noexcept {
    return uint16_t(u8(v,i))|(uint16_t(u8(v,i+1))<<8);
}
constexpr bool nonzero(const Digest& d) noexcept {
    for(auto b:d)if(b!=std::byte{0})return true;
    return false;
}
constexpr Projection inspect(Bytes data) noexcept {
    Projection p{};
    // Deliberately require exact individual file length. Never truncate.
    if(data.size()==0xCC) {
        p.format=Format::PGF;
        if(u8(data,0xB3)!=1) {
            p.state=u8(data,0xB3)==2||u8(data,0xB3)==3?
                State::NonPokemon:State::Unavailable;
            return p;
        }
        p.card=le16(data,0xB0);p.species=le16(data,0x1A);
        p.tid=le16(data,0);p.sid=le16(data,2);
        p.origin=u8(data,4);p.language=u8(data,0x1D);
        p.ball=u8(data,0x0E);p.egg=u8(data,0x5C);
        p.ribbons=le16(data,0x0C);
    } else if(data.size()==0x108||data.size()==0x310) {
        const bool full=data.size()==0x310;
        p.format=full?Format::WC6Full:Format::WC6;
        const Bytes card=full?data.subspan(0x208,0x108):data;
        if(u8(card,0x51)!=0) {
            p.state=u8(card,0x51)==1?State::NonPokemon:State::Unavailable;
            return p;
        }
        p.card=le16(card,0);p.species=le16(card,0x82);
        p.tid=le16(card,0x68);p.sid=le16(card,0x6A);
        p.origin=u8(card,0x6C);p.language=u8(card,0x85);
        p.ball=u8(card,0x76);p.egg=u8(card,0xD1);
        p.ribbons=le16(card,0x74);
        if(full) {
            p.versionRestrictionInWrapper=true;
            p.languageRestrictionInWrapper=true;
            p.restrictVersion=u8(data,0);
            p.restrictLanguage=u8(data,0x1FF);
        }
    } else return p;
    // An empty WC6 file is marked CardType=0. Species=0 is never a
    // Pokémon distribution (PKHeX EncounterEvent excludes Species==0).
    if(p.species==0 || p.egg>1)return p;
    p.state=State::PokemonProjection;
    return p;
}
struct Source {
    bool originalIndividualFileIdentified=false;
    bool originalArchiveDigestIndependentlyVerified=false;
    Digest originalArchiveSha256{};
    bool historicallyReleasedIndependentlyCorroborated=false;
    bool exactSourceRowLinkedToCardIndependentlyVerified=false;
    bool individualRecipientSaveIndependentlyVerified=false;
    bool giftReceiptLinkedToOriginalFileIndependentlyVerified=false;
    bool recipientToCurrentIndividualIndependentlyVerified=false;
    // Legacy/source-owned PKM bytes can change from trading, evolution,
    // transfer, or later ribbons. Never infer preservation from matching.
    bool originalValuesPreservedToObservedIndependentlyVerified=false;
    // PKHeX MysteryGiftVerifier uses hash-keyed mgrestrict*.pkl source
    // rows; neither these masks nor original metadata are inferred.
    bool restrictionRowIndependentlyVerified=false;
    bool giftHashMatchedRestrictionRowIndependentlyVerified=false;
    uint16_t allowedLanguages=0;
    uint16_t allowedRegions=0;
    bool eventBallIdentityIndependentlyVerified=false;
    bool eventRibbonIdentityIndependentlyVerified=false;
};
struct Native {
    bool validated=false;
    uint16_t tid=0,sid=0,species=0;
    uint8_t language=0,origin=0,ball=0,region=0;
    uint16_t ribbons=0;
    bool regionIndependentlyObserved=false;
};
struct Result {
    Finding status=Finding::UnknownNative;
    uint32_t contradiction=0,unknown=0;
    bool authenticated=false,globallyInvalid=false,globallyLegal=false;
    bool payloadEmbeddingPermitted=false;
};
enum Field:uint32_t { Species=1u,Trainer=2u,Version=4u,
    Language=8u,Region=16u,Ball=32u,Ribbons=64u };
constexpr Result evaluate(const Projection& gift,const Source& s,const Native& native) noexcept {
    Result result{};
    if(!native.validated)return result;
    result.status=Finding::UnknownGift;
    if(gift.state!=State::PokemonProjection)return result;
    result.status=Finding::UnknownSource;
    if(!s.originalIndividualFileIdentified ||
       !s.originalArchiveDigestIndependentlyVerified ||
       !nonzero(s.originalArchiveSha256) ||
       !s.exactSourceRowLinkedToCardIndependentlyVerified ||
       !s.historicallyReleasedIndependentlyCorroborated)return result;
    result.status=Finding::UnknownReceipt;
    if(!s.individualRecipientSaveIndependentlyVerified ||
       !s.giftReceiptLinkedToOriginalFileIndependentlyVerified ||
       !s.recipientToCurrentIndividualIndependentlyVerified)return result;
    result.status=Finding::UnknownHistoricalChange;
    if(!s.originalValuesPreservedToObservedIndependentlyVerified)return result;
    if(gift.species!=native.species)result.contradiction|=Species;
    if(gift.egg==0 && (gift.tid!=native.tid || gift.sid!=native.sid))
        result.contradiction|=Trainer;
    // 0 means no fixed game/language in PKHeX PGF/WC6. The receiving
    // version, OT and region are not reconstructible from blank fields.
    if(gift.origin && gift.origin!=native.origin)result.contradiction|=Version;
    else if(!gift.origin)result.unknown|=Version;
    if(gift.language && gift.language!=native.language)result.contradiction|=Language;
    else if(!gift.language)result.unknown|=Language;
    if(s.eventBallIdentityIndependentlyVerified && gift.ball!=0) {
        if(gift.ball!=native.ball)result.contradiction|=Ball;
    } else result.unknown|=Ball;
    if(s.eventRibbonIdentityIndependentlyVerified) {
        if(gift.ribbons!=native.ribbons)result.contradiction|=Ribbons;
    } else result.unknown|=Ribbons;
    // No restriction row -> no evidence that a language/console region
    // is authorized or forbidden by the gift's external hash record.
    if(!s.restrictionRowIndependentlyVerified ||
       !s.giftHashMatchedRestrictionRowIndependentlyVerified) {
        result.unknown|=Region;
        if(result.contradiction==0)result.status=Finding::UnknownRestrictions;
    } else {
        if(s.allowedLanguages && native.language<16 &&
           (s.allowedLanguages & (uint16_t(1)<<native.language))==0)
            result.contradiction|=Language;
        if(s.allowedLanguages && native.language>=16)result.unknown|=Language;
        if(s.allowedRegions) {
            if(!native.regionIndependentlyObserved || native.region>=16)result.unknown|=Region;
            else if((s.allowedRegions & (uint16_t(1)<<native.region))==0)
                result.contradiction|=Region;
        }
    }
    if(result.contradiction)result.status=Finding::CandidateExcluded;
    else if(result.status!=Finding::UnknownRestrictions)
        result.status=result.unknown?Finding::UnknownHistoricalChange:Finding::ConditionalCandidate;
    return result;
}
// PKHeX.Core/Legality/Verifiers/Ball/BallVerifier.cs:
// bred eggs: no pre-Gen6 inheritance; Gen6 cannot inherit Master/Cherish.
// Do NOT apply to PGF/WC6 event eggs: they are not bred encounters.
enum class EggBall:uint8_t { Unknown, CandidateMismatch, NeedsGen6SpeciesAbilityOracle, CompatibleDefault };
constexpr EggBall checkEggBall(uint8_t generation,uint8_t ball,bool independentlyBreedingEggEncounter) noexcept {
    if(!independentlyBreedingEggEncounter)return EggBall::Unknown;
    if(generation==5)return ball==4?EggBall::CompatibleDefault:EggBall::CandidateMismatch;
    if(generation==6) {
        if(ball==1||ball==16)return EggBall::CandidateMismatch; // Master, Cherish
        if(ball==4)return EggBall::CompatibleDefault;
        return EggBall::NeedsGen6SpeciesAbilityOracle;
    }
    return EggBall::Unknown;
}
static_assert(inspect(Bytes{}).state==State::Unavailable);
static_assert(!Result{}.globallyInvalid);
} // namespace Legality::Gen56MysteryGiftSourceEvidence
