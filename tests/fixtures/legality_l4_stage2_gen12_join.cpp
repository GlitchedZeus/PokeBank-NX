#include "Legality/Gen12OriginalDistributionJoinEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
namespace J=Legality::Gen12OriginalDistributionJoinEvidence;
namespace G=J::Gift;
namespace A=J::Archive;

template<class T> constexpr G::Datum<T> known(T v) { return {true,v}; }
constexpr G::Name name(uint8_t first) {
    G::Name out{};out[0]=first;out[1]=0x50;return out;
}
constexpr G::Observation observed(G::Game edition=G::Game::Crystal) {
    G::Observation o{};o.game=edition;o.species=known<uint16_t>(25);
    o.level=known<uint8_t>(20);o.trainerId=known<uint16_t>(43121);
    o.trainerName=known(name('A'));o.nickname=known(name('B'));
    o.moves=known(G::Moves4{1,2,3,0});return o;
}
constexpr J::Candidate candidate(G::Game edition=G::Game::Crystal,
                                  G::Kind kind=G::Kind::StadiumGift) {
    J::Candidate c{};
    c.original.game=edition;c.original.kind=kind;
    c.original.originalMechanicIndependentlyVerified=true;
    c.original.originalRowIndependentlyVerified=true;
    c.original.originalDistributionAttested=true;
    c.original.originalReceiptKnown=true;
    c.original.levelRule=kind==G::Kind::NPCTrade?
        G::LevelRule::OfferedTradeLevel:G::LevelRule::Fixed;
    c.original.species=known<uint16_t>(25);
    c.original.level=known<uint8_t>(10);
    c.original.trainerId=known<uint16_t>(43121);
    c.original.trainerName=known(name('A'));
    c.original.nickname=known(name('B'));
    c.original.moves=known(G::Moves4{1,2,3,0});
    c.route.speciesHistory=G::History::Unchanged;
    c.route.trainerNameHistory=G::History::Unchanged;
    c.route.nicknameHistory=G::History::Unchanged;
    c.route.moveHistory=G::History::Unchanged;
    c.route.timeCapsule=G::Crossing::NoCrossingVerified;
    c.route.offeredTradeLevel=known<uint8_t>(10);
    c.editionIdentityIndependentlyVerified=true;
    c.independentlyIdentifiedSourceEdition=edition;
    c.originalRawCharsetLinkIndependentlyVerified=true;
    c.archive.platform=J::platformFor(edition);
    c.archive.family=J::familyFor(kind);
    c.archive.grade=A::ArchiveGrade::OriginalAuthenticated;
    c.archive.exactSourceIdentityIndependentlyProven=true;
    c.archive.sourceMechanismIndependentlyProven=true;
    c.archive.originalPayloadDigestVerified=true;
    c.archive.originalPayloadSha256[0]=std::byte{0x4A};
    c.archive.originalPayloadFieldsIndependentlyExtracted=true;
    c.archive.distributionReleaseIndependentlyCorroborated=true;
    c.archive.individualOriginalReceiptHistoryKnown=true;
    c.archiveRowLinkIndependentlyVerified=true;
    c.archiveRowOriginalDigestIndependentlyVerified=true;
    c.archiveRowOriginalDigest=c.archive.originalPayloadSha256;
    return c;
}
static_assert(J::evaluate(observed(),candidate()).status==J::Status::ConditionalCandidate);
static_assert(!J::evaluate(observed(),candidate()).authenticated);
int main() {
    using S=J::Status;using AS=J::AggregateStatus;
    uint64_t cases=0;
    // Exhaust eight independent source/archive/charset attestation gates,
    // each possible archived grade and exact edition families.
    for(unsigned edition=1;edition<=6;++edition)
    for(unsigned kind=4;kind<=5;++kind)
    for(unsigned grade=0;grade<=255;++grade)
    for(unsigned mask=0;mask<256;++mask) {
        auto c=candidate(static_cast<G::Game>(edition),static_cast<G::Kind>(kind));
        c.archive.grade=static_cast<A::ArchiveGrade>(grade);
        c.editionIdentityIndependentlyVerified=(mask&1)!=0;
        c.originalRawCharsetLinkIndependentlyVerified=(mask&2)!=0;
        c.archiveRowLinkIndependentlyVerified=(mask&4)!=0;
        c.archiveRowOriginalDigestIndependentlyVerified=(mask&8)!=0;
        c.archive.originalPayloadDigestVerified=(mask&16)!=0;
        c.archive.distributionReleaseIndependentlyCorroborated=(mask&32)!=0;
        c.archive.individualOriginalReceiptHistoryKnown=(mask&64)!=0;
        c.original.originalMechanicIndependentlyVerified=(mask&128)!=0;
        const auto r=J::evaluate(observed(static_cast<G::Game>(edition)),c);
        const bool compatible=kind==4 || edition>=4;
        const bool expected=compatible && (grade==1 || grade==2) && mask==255;
        assert((r.status==S::ConditionalCandidate)==expected);
        assert(!r.authenticated && !r.globallyInvalid);
        assert(!r.copyrightedPayloadEmbeddingPermitted);
        ++cases;
    }
    assert(cases==786432);
    // PCNY does not exist in Gen I. No mismatched unrelated archive family
    // can be promoted merely because it shares species/TID/numeric digest.
    auto p=candidate(G::Game::Red,G::Kind::PCNYGift);
    assert(J::evaluate(observed(G::Game::Red),p).status==S::UnknownSource);
    p=candidate();p.archive.family=A::Family::PCNYGift;
    assert(J::evaluate(observed(),p).status==S::UnknownArchiveIdentity);
    p=candidate();p.archiveRowOriginalDigest[1]=std::byte{0x77};
    assert(J::evaluate(observed(),p).status==S::UnknownArchiveIdentity);
    // A documented NPC exchange uses the offered-mon level, not a hardcoded
    // fixed gift level. The archive-distribution branch must not run here.
    auto npc=candidate(G::Game::Gold,G::Kind::NPCTrade);
    assert(J::evaluate(observed(G::Game::Gold),npc).status==S::ConditionalCandidate);
    npc.route.offeredTradeLevel.known=false;
    assert(J::evaluate(observed(G::Game::Gold),npc).status==S::UnknownHistory);
    npc=candidate(G::Game::Gold,G::Kind::NPCTrade);
    npc.route.offeredTradeLevel=known<uint8_t>(30);
    assert(J::evaluate(observed(G::Game::Gold),npc).status==S::CandidateExcluded);
    // Cross-version trade requires independent edition history. An
    // unchanged Pokémon in a different same-generation title proves none.
    auto traded=candidate(G::Game::Gold,G::Kind::NPCTrade);
    assert(J::evaluate(observed(G::Game::Crystal),traded).status==S::UnknownHistory);
    traded.route.sameGenerationEditionTransferIndependentlyVerified=true;
    assert(J::evaluate(observed(G::Game::Crystal),traded).status==S::ConditionalCandidate);
    // A Time Capsule source snapshot with Gen II-only move is excluded ONLY
    // from this documented path, while an unverified crossing stays Unknown.
    auto capsule=candidate(G::Game::Red,G::Kind::StadiumGift);
    assert(J::evaluate(observed(G::Game::Crystal),capsule).status==S::UnknownHistory);
    capsule.route.timeCapsule=G::Crossing::SnapshotVerified;
    capsule.route.crossingSpecies=known<uint16_t>(25);
    capsule.route.crossingMoves=known(G::Moves4{1,2,165,0});
    assert(J::evaluate(observed(G::Game::Crystal),capsule).status==S::ConditionalCandidate);
    capsule.route.crossingMoves.value[2]=166;
    const auto excluded=J::evaluate(observed(G::Game::Crystal),capsule);
    assert(excluded.status==S::CandidateExcluded &&
           (excluded.gift.contradicted&G::Capsule));
    // Raw name bytes may be identical across two encoding contexts without
    // being attributable to the original archival source.
    auto unencoded=candidate();unencoded.originalRawCharsetLinkIndependentlyVerified=false;
    assert(J::evaluate(observed(),unencoded).status==S::UnknownFieldEncoding);
    // Candidate-local exclusions never invalidate the whole Pokémon.
    auto first=candidate();first.original.trainerId.value=111;
    auto second=candidate();
    const std::array<J::Candidate,2> choices{first,second};
    const auto mult=J::evaluateAll(observed(),choices);
    assert(mult.status==AS::HasConditionalCandidate);
    assert(mult.excluded==1 && mult.conditional==1 && !mult.globallyInvalid);
    const std::array<J::Candidate,2> both{first,first};
    const auto all=J::evaluateAll(observed(),both);
    assert(all.status==AS::AllProvidedCandidatesExcluded && !all.globallyInvalid);
    const std::array<J::Candidate,2> withUnknown{first,unencoded};
    const auto amb=J::evaluateAll(observed(),withUnknown);
    assert(amb.status==AS::UnknownCandidates && amb.excluded==1 && amb.unknown==1);
    assert(J::evaluateAll(observed(),{}).status==AS::UnknownCandidates);
    std::array<J::Candidate,129> tooMany{};
    assert(J::evaluateAll(observed(),tooMany).examined==0);
    std::cout<<"L4 Stage2 archival gifts / Stadium / PCNY / NPC / Capsule: "
             <<cases<<" source masks, candidate-local/Unknown alternatives: PASS\n";
}
