#pragma once
// L1 E09/E10: source-attested *original* Colosseum or XD fixed-encounter
// identity, joining an indexed PKHeX-pinned template with the accepted
// original CXD / starter PID-IV witness.  PK3 origin=15 is shared by BOTH
// games and is never independent proof of the original GameCube title.
// An apparent mismatch rules out only the attested fixed template.
#include "Legality/Gen3GameCubeFixedEncounterEvidence.h"
#include <cstddef>
#include <cstdint>
namespace Legality::Gen3GameCubeOriginalFixedHistory {
namespace F=Gen3GameCubeFixedEncounterEvidence;
enum class Game : uint8_t { Unknown, Colosseum, XD };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalRow, UnknownOriginalScene,
    UnknownOriginalNativeSnapshot, UnknownMalformedOriginalNativeIvs,
    UnknownOriginalTrainerAndText,
    UnknownRecipientSave, ForeignSourceRowNotEvidence,
    CandidateOriginalIdentityContradicted, CandidateOriginalPidIvContradicted,
    BoundedOriginalRngUnresolved, ConditionalOriginalFixedHistory
};
struct Candidate {
    Game originalGame=Game::Unknown;
    std::size_t originalRowIndex=0;
    F::Candidate originalNative{};
    bool originalGameIndependentlyAttested=false;
    bool originalPhysicalRowIndependentlyAttested=false;
    bool originalGiftOrTradeSceneIndependentlyAttested=false;
    bool originalNativeSnapshotIndependentlyAttested=false;
    bool originalTrainerAndTextIndependentlyAttested=false;
    // Bonus disc and Mt Battle rewards are Colosseum distributions whose
    // PK3 origin is Ruby or Sapphire; this recipient history is separate
    // from the ORIGINAL Colosseum disc/game scene.
    bool originalRecipientSaveIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    F::Kind fixedKind=F::Kind::None;
    F::Policy family=F::Policy::CxdStandard;
    bool recipientIsGba=false;
};
constexpr Game originalGameForRow(std::size_t index) noexcept {
    return index<3 || (index>=11 && index<F::kTemplateCount)
        ? Game::Colosseum :
        index>=3 && index<11 ? Game::XD : Game::Unknown;
}
inline Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested ||
       c.originalGame==Game::Unknown)return {};
    if(!c.originalPhysicalRowIndependentlyAttested ||
       c.originalRowIndex>=F::kTemplateCount)
        return {Status::UnknownOriginalRow};
    const F::Entry& row=F::kEntries[c.originalRowIndex];
    const bool gbaRecipient=row.originGame!=15u;
    if(originalGameForRow(c.originalRowIndex)!=c.originalGame)
        return {Status::ForeignSourceRowNotEvidence,row.kind,row.policy,gbaRecipient};
    if(!c.originalGiftOrTradeSceneIndependentlyAttested)
        return {Status::UnknownOriginalScene,row.kind,row.policy,gbaRecipient};
    if(!c.originalNativeSnapshotIndependentlyAttested)
        return {Status::UnknownOriginalNativeSnapshot,row.kind,row.policy,gbaRecipient};
    if(!c.originalTrainerAndTextIndependentlyAttested)
        return {Status::UnknownOriginalTrainerAndText,row.kind,row.policy,gbaRecipient};
    if(!Gen3PidIv::validStoredIVs(c.originalNative.ivs))
        return {Status::UnknownMalformedOriginalNativeIvs,row.kind,row.policy,gbaRecipient};
    if(gbaRecipient && !c.originalRecipientSaveIndependentlyAttested)
        return {Status::UnknownRecipientSave,row.kind,row.policy,gbaRecipient};
    if(!F::identityMatches(row,c.originalNative))
        return {Status::CandidateOriginalIdentityContradicted,
                row.kind,row.policy,gbaRecipient};
    const auto evidence=F::analyze(c.originalNative);
    if(!evidence.identityMatched || evidence.kind!=row.kind)
        return {Status::CandidateOriginalIdentityContradicted,
                row.kind,row.policy,gbaRecipient};
    if(!evidence.rngMatched)
        return {evidence.searchLimited?Status::BoundedOriginalRngUnresolved:
                Status::CandidateOriginalPidIvContradicted,
                row.kind,row.policy,gbaRecipient};
    return {Status::ConditionalOriginalFixedHistory,
            row.kind,row.policy,gbaRecipient};
}
} // namespace Legality::Gen3GameCubeOriginalFixedHistory
