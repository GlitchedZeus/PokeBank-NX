#include "tools/audit_legality_l5_stage6_receipt_chain.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#ifdef NDEBUG
#error "L5 Stage6 tests require assertions"
#endif
namespace C=Legality::Audit::L5ReceiptChain;
static std::uint64_t assertions=0;
#define CHECK(v) do {++assertions;if(!(v)){std::cerr<<"FAIL "<<__LINE__<<" "<<#v<<'\n';std::abort();}}while(false)
constexpr C::Input fixture(){
    C::Input in{};
    in.targetPokemon=33;in.targetSource=45;in.chainId=57;in.count=4;
    C::Artifact kinds[]{C::Artifact::CatalogProjection,C::Artifact::OriginalSave,
                         C::Artifact::OriginalActionReceipt,C::Artifact::PermissionGrant};
    std::uint64_t pH=0,pL=0;
    for(std::uint8_t i=0;i<4;++i){
        auto& r=in.records[i];r.pokemon=33;r.source=45;r.chainId=57;
        r.sequence=i+1;r.identity=i+1;
        r.contentHashHigh=100+i;r.contentHashLow=200+i;
        r.previousHashHigh=pH;r.previousHashLow=pL;
        pH=r.entryHashHigh=300+i;pL=r.entryHashLow=400+i;
        r.kind=kinds[i];r.payloadHashIndependentlyVerified=true;
        r.ownerConsentIndependentlyVerified=true;r.provenanceSignerIndependentlyVerified=true;
        r.originalDeviceCaptureVerified=true;r.originalTimelineVerified=true;
        r.redistributionExplicitlyPermitted=true;
    }
    in.independentlyTrustedHeadAvailable=true;
    in.independentlyTrustedHeadHigh=pH;in.independentlyTrustedHeadLow=pL;
    in.entireOriginalTimelineEnumerated=true;
    in.ownershipAndCollectionAuthorityVerified=true;
    return in;
}
static void directed(){
    auto a=fixture();
    CHECK(C::evaluate(a).state==C::State::IndependentlyAttestedCandidate);
    CHECK(C::evaluate(a).redistributionCleared);
    CHECK(!C::evaluate(a).wholePokemonLegal);
    for(int mode=0;mode<25;++mode){
        auto b=fixture();
        switch(mode){
            case 0:b.targetPokemon=0;break;
            case 1:b.count=17;break;
            case 2:b.records[2].pokemon=13;break;
            case 3:b.records[2].source=0;break;
            case 4:b.records[2].chainId=13;break;
            case 5:b.records[2].sequence=1;break;
            case 6:b.records[2].identity=b.records[1].identity;break;
            case 7:b.records[2].contentHashHigh=0;break;
            case 8:b.records[2].contentHashLow=0;break;
            case 9:b.records[2].entryHashHigh=0;break;
            case 10:b.records[2].entryHashLow=0;break;
            case 11:b.records[2].previousHashHigh^=1;break;
            case 12:b.records[2].previousHashLow^=1;break;
            case 13:b.records[2].kind=C::Artifact::Unknown;break;
            case 14:b.records[2].sourceChangedOrRewritten=true;break;
            case 15:b.independentlyTrustedHeadAvailable=false;break;
            case 16:b.independentlyTrustedHeadHigh^=1;break;
            case 17:b.independentlyTrustedHeadLow^=1;break;
            case 18:b.records[1].payloadHashIndependentlyVerified=false;break;
            case 19:b.records[1].ownerConsentIndependentlyVerified=false;break;
            case 20:b.records[1].provenanceSignerIndependentlyVerified=false;break;
            case 21:b.records[1].originalDeviceCaptureVerified=false;break;
            case 22:b.records[1].originalTimelineVerified=false;break;
            case 23:b.entireOriginalTimelineEnumerated=false;break;
            case 24:b.ownershipAndCollectionAuthorityVerified=false;break;
        }
        CHECK(C::evaluate(b).state!=C::State::IndependentlyAttestedCandidate);
        CHECK(!C::evaluate(b).wholePokemonLegal);
    }
    a=fixture();a.records[3].redistributionExplicitlyPermitted=false;
    CHECK(C::evaluate(a).originalCandidateAttested);
    CHECK(!C::evaluate(a).redistributionCleared);
    a=fixture();a.records[1].kind=C::Artifact::CatalogProjection;
    a.records[2].kind=C::Artifact::ReleaseWindow;
    CHECK(C::evaluate(a).state==C::State::RestrictedResearch);
    CHECK(!C::evaluate(a).archiveVerified);
}
static std::uint32_t advance(std::uint32_t& x){return x=x*1664525u+1013904223u;}
static void fuzz(){
    std::uint32_t seed=0x6c519f13u;
    for(std::uint32_t iteration=0;iteration<200000u;++iteration){
        const auto bits=advance(seed);
        auto a=fixture();
        switch(bits%20u){
            case 0:a.count=17;break;
            case 1:a.records[2].pokemon=34;break;
            case 2:a.records[3].source=44;break;
            case 3:a.records[2].identity=2;break;
            case 4:a.records[1].sequence=4;break;
            case 5:a.records[3].previousHashHigh^=1;break;
            case 6:a.records[3].previousHashLow^=1;break;
            case 7:a.records[1].payloadHashIndependentlyVerified=false;break;
            case 8:a.records[2].originalTimelineVerified=false;break;
            case 9:a.records[3].redistributionExplicitlyPermitted=false;break;
            case 10:a.independentlyTrustedHeadHigh^=1;break;
            case 11:a.independentlyTrustedHeadAvailable=false;break;
            case 12:a.records[1].kind=C::Artifact::CatalogProjection;break;
            case 13:a.records[1].originalDeviceCaptureVerified=false;break;
            case 14:a.records[1].ownerConsentIndependentlyVerified=false;break;
            case 15:a.records[1].provenanceSignerIndependentlyVerified=false;break;
            case 16:a.entireOriginalTimelineEnumerated=false;break;
            case 17:a.ownershipAndCollectionAuthorityVerified=false;break;
            case 18:a.records[3].sourceChangedOrRewritten=true;break;
            case 19:a.count=3;a.independentlyTrustedHeadHigh=302;a.independentlyTrustedHeadLow=402;break;
        }
        auto out=C::evaluate(a);
        CHECK(!out.wholePokemonLegal);
        CHECK(out.checked<=a.count);
        if(!out.originalCandidateAttested)CHECK(!out.archiveVerified);
        if(!out.intactReportedChain)CHECK(!out.redistributionCleared);
        CHECK(!out.redistributionCleared || out.originalCandidateAttested);
        if(bits%20u==0 || bits%20u==1 || bits%20u==2 || bits%20u==3 ||
           bits%20u==4 || bits%20u==5 || bits%20u==6 || bits%20u==18)
            CHECK(out.state!=C::State::IndependentlyAttestedCandidate);
    }
}
int main(){directed();fuzz();std::cout<<"L5 Stage6 provenance receipt-chain PASS "<<assertions
    <<" assertions (synthetic structural evidence, not retail originality)\n";}
