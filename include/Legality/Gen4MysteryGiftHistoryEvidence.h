#pragma once
// L4 Stage 3: a single hypothetical Gen IV gift origin's historical fields.
// Original PCD/PGT bytes and receipt are independent from the present PK4.
// No result here is a global legal/illegal or historical-authenticity claim.
#include "Legality/Gen4MysteryGiftContainerEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4MysteryGiftHistoryEvidence {
namespace C=Gen4MysteryGiftContainerEvidence;
enum class History : uint8_t { Unknown, Unchanged, VerifiedChangedFinal };
enum class Status : uint8_t { UnavailableNative, UnknownOriginalSource,
    NonPokemonGift, UnknownRangerReceipt, CandidateContradicted,
    UnresolvedHistory, ConditionalCandidate };
enum Field : uint32_t {
    Species=1u<<0, Trainer=1u<<1, Language=1u<<2, OriginGame=1u<<3,
    EggState=1u<<4, Ball=1u<<5, MoveSet=1u<<6,
    Ribbon=1u<<7, RawTrainer=1u<<8, RawNickname=1u<<9,
    Fateful=1u<<10, GiftType=1u<<11
};
template<class T> struct Datum {
    bool known=false;
    T value{};
};
using Moves=std::array<uint16_t,4>;
using Ribbons=std::array<uint32_t,3>;
using RawOT=std::array<std::byte,16>;
using RawNicknameBytes=std::array<std::byte,22>;
struct Observed {
    bool nativeValid=false;
    uint16_t species=0;
    uint16_t tid=0, sid=0;
    uint8_t language=0, originVersion=0, ball=0;
    bool egg=false, fateful=false;
    Moves moves{};
    Ribbons ribbons{};
    RawOT rawOT{};
    RawNicknameBytes rawNickname{};
};
struct Source {
    bool originalFieldsIndependentlyVerified=false;
    C::GiftType giftType=C::GiftType::None;
    Datum<uint16_t> originalSpecies{};
    Datum<uint16_t> originalTid{}, originalSid{};
    Datum<uint8_t> originalLanguage{}, originalOriginVersion{};
    Datum<uint8_t> originalBall{};
    Datum<bool> originalEgg{}, originalFateful{};
    Datum<Moves> originalMoves{};
    Datum<Ribbons> requiredAtReceiptRibbons{};
    Datum<RawOT> originalRawOT{};
    Datum<RawNicknameBytes> originalRawNickname{};
    History speciesHistory=History::Unknown;
    Datum<uint16_t> verifiedFinalSpecies{};
    History trainerHistory=History::Unknown;
    Datum<std::array<uint16_t,2>> verifiedFinalTrainer{};
    History languageHistory=History::Unknown;
    Datum<uint8_t> verifiedFinalLanguage{};
    History originHistory=History::Unknown;
    Datum<uint8_t> verifiedFinalOrigin{};
    History eggHistory=History::Unknown;
    Datum<bool> verifiedFinalEgg{};
    History ballHistory=History::Unknown;
    Datum<uint8_t> verifiedFinalBall{};
    History movesHistory=History::Unknown;
    Datum<Moves> verifiedFinalMoves{};
    History rawOTHistory=History::Unknown;
    Datum<RawOT> verifiedFinalRawOT{};
    History nicknameHistory=History::Unknown;
    Datum<RawNicknameBytes> verifiedFinalNickname{};
    // Ranger gift recipient and anti-shiny PID/IV proof must be checked by
    // Gen4EventRangerGiftHistory, NOT inferred from a Manaphy egg template.
    bool rangerGiftProofIndependentlyVerified=false;
    // Ribbons only tested when a source independently verifies a persistent
    // mask. Later-added ribbons do not contradict the original source.
    bool ribbonRetentionIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnavailableNative;
    uint32_t contradicted=0, unknown=0;
    bool authenticated=false;
};
template<class T>
constexpr void compare(uint32_t bit,const Datum<T>& initial,
    const T& current,History history,const Datum<T>& final,
    uint32_t& contradicted,uint32_t& unknown) noexcept {
    if(!initial.known || history==History::Unknown) {unknown|=bit;return;}
    if(history==History::Unchanged) {
        if(initial.value!=current)contradicted|=bit;
    } else if(!final.known) unknown|=bit;
    else if(final.value!=current)contradicted|=bit;
}
constexpr void compareBool(uint32_t bit,const Datum<bool>& initial,
    bool current,History history,const Datum<bool>& final,
    uint32_t& contradicted,uint32_t& unknown) noexcept {
    compare(bit,initial,current,history,final,contradicted,unknown);
}
constexpr Result evaluate(const Observed& obs,const C::Projection& container,
                          const C::Attestation& archive,
                          const Source& src) noexcept {
    Result out{};
    if(!obs.nativeValid)return out;
    out.status=Status::UnknownOriginalSource;
    if(C::attest(container,archive)!=C::ArchiveStatus::ConditionalOriginalSource ||
       !src.originalFieldsIndependentlyVerified)return out;
    if(container.delivery==C::Delivery::ItemOrService ||
       container.delivery==C::Delivery::KeyItemUnlock ||
       container.delivery==C::Delivery::Other) {
        out.status=Status::NonPokemonGift;return out;
    }
    if(container.type!=src.giftType)out.contradicted|=GiftType;
    compare(Species,src.originalSpecies,obs.species,src.speciesHistory,
            src.verifiedFinalSpecies,out.contradicted,out.unknown);
    if(!src.originalTid.known || !src.originalSid.known ||
       src.trainerHistory==History::Unknown)out.unknown|=Trainer;
    else if(src.trainerHistory==History::Unchanged) {
        if(obs.tid!=src.originalTid.value || obs.sid!=src.originalSid.value)
            out.contradicted|=Trainer;
    } else if(!src.verifiedFinalTrainer.known)out.unknown|=Trainer;
    else if(obs.tid!=src.verifiedFinalTrainer.value[0] ||
            obs.sid!=src.verifiedFinalTrainer.value[1])out.contradicted|=Trainer;
    compare(Language,src.originalLanguage,obs.language,src.languageHistory,
            src.verifiedFinalLanguage,out.contradicted,out.unknown);
    compare(OriginGame,src.originalOriginVersion,obs.originVersion,
            src.originHistory,src.verifiedFinalOrigin,out.contradicted,out.unknown);
    compareBool(EggState,src.originalEgg,obs.egg,src.eggHistory,
                src.verifiedFinalEgg,out.contradicted,out.unknown);
    compare(Ball,src.originalBall,obs.ball,src.ballHistory,
            src.verifiedFinalBall,out.contradicted,out.unknown);
    compare(MoveSet,src.originalMoves,obs.moves,src.movesHistory,
            src.verifiedFinalMoves,out.contradicted,out.unknown);
    compare(RawTrainer,src.originalRawOT,obs.rawOT,src.rawOTHistory,
            src.verifiedFinalRawOT,out.contradicted,out.unknown);
    compare(RawNickname,src.originalRawNickname,obs.rawNickname,
            src.nicknameHistory,src.verifiedFinalNickname,
            out.contradicted,out.unknown);
    if(!src.originalFateful.known)out.unknown|=Fateful;
    else if(obs.fateful!=src.originalFateful.value) {
        // Fateful flag retention is normally persistent, but detached PK4
        // does not establish an uninterrupted original-stage history.
        out.unknown|=Fateful;
    }
    if(!src.requiredAtReceiptRibbons.known ||
       !src.ribbonRetentionIndependentlyVerified)out.unknown|=Ribbon;
    else for(std::size_t i=0;i<3;++i) {
        if((obs.ribbons[i]&src.requiredAtReceiptRibbons.value[i])!=
            src.requiredAtReceiptRibbons.value[i])out.contradicted|=Ribbon;
    }
    if(out.contradicted)out.status=Status::CandidateContradicted;
    else if(container.delivery==C::Delivery::RangerManaphyEgg &&
            !src.rangerGiftProofIndependentlyVerified)
        out.status=Status::UnknownRangerReceipt;
    else if(out.unknown)out.status=Status::UnresolvedHistory;
    else out.status=Status::ConditionalCandidate;
    return out;
}
static_assert((Species|Trainer|Language|OriginGame|EggState|Ball|MoveSet|
               Ribbon|RawTrainer|RawNickname|Fateful|GiftType)==0xfffu);
} // namespace Legality::Gen4MysteryGiftHistoryEvidence
