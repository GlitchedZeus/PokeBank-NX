#include "Legality/Gen4NativeBreedingIVHistory.h"
#include "Encryption/Encryption4.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace N=Legality::Gen4NativeBreedingIVHistory;
namespace H=Legality::Gen4EggIVHistoricalCandidateSet;
namespace IV=Legality::Gen4EggIVInheritanceStage;

// Synthetic, checksum-valid RETAIL-FORMAT encrypted original PK4 records.
// This is a native-parser/control-flow test, not a preserved retail egg.
void w16(std::vector<std::byte>& b,size_t o,uint16_t x) {
    b[o]=static_cast<std::byte>(x&255u);
    b[o+1]=static_cast<std::byte>((x>>8)&255u);
}
void w32(std::vector<std::byte>& b,size_t o,uint32_t x) {
    w16(b,o,static_cast<uint16_t>(x));
    w16(b,o+2,static_cast<uint16_t>(x>>16));
}
uint32_t pack(const std::array<uint8_t,6>& ivs,bool egg) {
    uint32_t v=0;
    for(unsigned i=0;i<6;++i)v|=static_cast<uint32_t>(ivs[i])<<(5*i);
    if(egg)v|=1u<<30;
    return v;
}
std::vector<std::byte> encrypted(uint32_t pid,uint16_t item,
            const std::array<uint8_t,6>& ivs,bool egg) {
    std::vector<std::byte> p(Encryption::SIZE_STORED4,std::byte{0});
    w32(p,0,pid);
    w16(p,8,25); // Pikachu, nonzero valid species
    w16(p,10,item);
    w32(p,0x38,pack(ivs,egg));
    w16(p,6,Encryption::checksum4(p));
    return Encryption::encryptArray4(p);
}
int main() {
    const std::array<uint8_t,6> initial{0,3,6,9,12,15};
    const std::array<uint8_t,6> left{7,12,17,22,27,0};
    const std::array<uint8_t,6> right{13,24,3,14,25,4};
    // HGSS source-verified first-seed single Power Weight vector.
    const auto generated=IV::replay("heartgold_nds",0,initial,left,right,294,0);
    assert((generated.childIVs==std::array<uint8_t,6>({7,24,6,9,27,15})));
    assert(generated.finalSeed==0x31B0DDE4u);
    // Deliberately give the retained parent PK4 NO Power item: the
    // proposed collection-time Power item comes only from attestation.
    auto p0=encrypted(0x12345678u,0,left,false);
    auto p1=encrypted(0x23456789u,0,right,false);
    auto child=encrypted(0x3456789Au,0,generated.childIVs,true);
    const auto original0=p0,original1=p1,originalChild=child;
    Pokemon::Pokemon4ReadOnly parent0(p0,Enums::GameVersion::HGSS);
    Pokemon::Pokemon4ReadOnly parent1(p1,Enums::GameVersion::HGSS);
    Pokemon::Pokemon4ReadOnly egg(child,Enums::GameVersion::HGSS);
    assert(parent0.valid() && parent1.valid() && egg.valid());
    assert(egg.isEgg() && !parent0.isEgg() && !parent1.isEgg());
    N::Input in{};
    in.game="heartgold_nds";
    in.slot0=&parent0;in.slot1=&parent1;in.originalEgg=&egg;
    in.slot0StageIdentity=0x10101;in.slot1StageIdentity=0x10101;
    in.eggStageIdentity=0x10101;
    in.parentSnapshotsIndependentlyVerified=true;
    in.physicalDaycareOrderIndependentlyVerified=true;
    in.originalUnhatchedEggIndependentlyVerified=true;
    in.containingSourceGameIndependentlyVerified=true;
    in.sharedStageLinkIndependentlyVerified=true;
    in.originalInitialChildIVsIndependentlyVerified=true;
    in.originalInitialChildIVs=initial;
    in.historicalAlternatives.count=1;
    in.historicalAlternatives.allIVStageHypothesesIndependentlyEnumerated=true;
    auto& candidate=in.historicalAlternatives.histories[0];
    candidate.routeIdentity=0x47;
    candidate.stageIdentity=in.eggStageIdentity;
    candidate.independentSourceAttestation=true;
    candidate.historicalHeldItemsKnown=true;
    candidate.physicalSlot0Item=294;candidate.physicalSlot1Item=0;
    candidate.finalRNGStateKnown=true;candidate.finalRNGState=generated.finalSeed;
    const auto yes=N::evaluate(in);
    assert(yes.status==N::Status::Evaluated);
    assert(yes.histories.status==H::Status::ConditionalHistoryMatched);
    assert(yes.histories.matched==1);
    // Switch item histories at the same original stage; candidate-local
    // mismatch must not invalidate a Pokémon globally.
    auto wrongItems=in;
    wrongItems.historicalAlternatives.histories[0].physicalSlot0Item=0;
    const auto no=N::evaluate(wrongItems);
    assert(no.status==N::Status::Evaluated);
    assert(no.histories.status==H::Status::AllEnumeratedStageHypothesesContradicted);
    // Legitimately ambiguous archived original-daycare item hypotheses:
    // one source-backed candidate conflicts, a DIFFERENT original held-
    // item hypothesis matches, and a duplicate untrusted route exists.
    // The duplicate must not erase the source-backed native match; it
    // also cannot contribute to claims of exhaustive contradiction.
    auto alternatives=in;
    alternatives.historicalAlternatives.count=3;
    alternatives.historicalAlternatives.histories[0].physicalSlot0Item=0;
    alternatives.historicalAlternatives.histories[1]=candidate;
    alternatives.historicalAlternatives.histories[1].routeIdentity=0x48;
    alternatives.historicalAlternatives.histories[2]=candidate;
    alternatives.historicalAlternatives.histories[2].routeIdentity=0x48;
    const auto mixed=N::evaluate(alternatives);
    assert(mixed.status==N::Status::Evaluated);
    assert(mixed.histories.status==H::Status::ConditionalHistoryMatched);
    assert(mixed.histories.matched==1 && mixed.histories.contradicted==1 &&
           mixed.histories.unknown==1);
    alternatives.historicalAlternatives.histories[1].historicalHeldItemsKnown=false;
    assert(N::evaluate(alternatives).histories.status==
           H::Status::MissingOrDuplicateRouteIdentity);
    // Separately attested item histories do not come from whatever
    // held item a valid present-day native PK4 happens to contain.
    const auto& originalSourceBytes=egg.originalEncryptedBytes();
    assert(originalSourceBytes.size()==child.size());
    for(size_t i=0;i<child.size();++i)
        assert(originalSourceBytes[i]==child[i]);
    auto unknownItem=in;unknownItem.historicalAlternatives.histories[0].historicalHeldItemsKnown=false;
    assert(N::evaluate(unknownItem).histories.status==H::Status::UnresolvedCandidateHistory);
    auto missingSeed=in;missingSeed.historicalAlternatives.histories[0].finalRNGStateKnown=false;
    assert(N::evaluate(missingSeed).histories.status==H::Status::UnresolvedCandidateHistory);
    auto missingInitial=in;missingInitial.originalInitialChildIVsIndependentlyVerified=false;
    assert(N::evaluate(missingInitial).histories.status==H::Status::UnknownOriginalParentStage);
    auto missingOrder=in;missingOrder.physicalDaycareOrderIndependentlyVerified=false;
    assert(N::evaluate(missingOrder).status==N::Status::UnknownParentSnapshot);
    auto wrongEvent=in;wrongEvent.slot1StageIdentity++;
    assert(N::evaluate(wrongEvent).status==N::Status::IncoherentStageLink);
    auto absentEvent=in;absentEvent.sharedStageLinkIndependentlyVerified=false;
    assert(N::evaluate(absentEvent).status==N::Status::UnknownStageLink);
    auto wrongGame=in;wrongGame.game="platinum_nds";
    assert(N::evaluate(wrongGame).status==N::Status::UnknownSourceGame);
    auto dp=in;dp.game="diamond_nds";
    assert(N::evaluate(dp).status==N::Status::UnsupportedGame);
    auto notEgg=encrypted(0x3456789Au,0,generated.childIVs,false);
    Pokemon::Pokemon4ReadOnly hatched(notEgg,Enums::GameVersion::HGSS);
    auto hatchedIn=in;hatchedIn.originalEgg=&hatched;
    assert(N::evaluate(hatchedIn).status==N::Status::UnknownOriginalUnhatchedEgg);
    auto bad=child;bad[11]^=std::byte{0x20};
    Pokemon::Pokemon4ReadOnly invalid(bad,Enums::GameVersion::HGSS);
    auto invalidIn=in;invalidIn.originalEgg=&invalid;
    assert(N::evaluate(invalidIn).status==N::Status::InvalidNativeRecord);
    // Never mutate source or staged native PK4 bytes.
    assert(p0==original0 && p1==original1 && child==originalChild);
    std::cout<<"L2 native encrypted PK4 IV-history + positive/negative/Unknown PASS\n";
}
