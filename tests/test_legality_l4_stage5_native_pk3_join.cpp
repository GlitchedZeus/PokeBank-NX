#include "Legality/Gen3DistributionNativePK3Evidence.h"
#include "Legality/Gen14EventCoordinatorJoinEvidence.h"
#include "Legality/Gen14EventCoordinatorRoutes.h"
#include "Encryption/Encryption3FRLG.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <vector>
namespace N=Legality::Gen3DistributionNativePK3Evidence;
namespace J=Legality::Gen14EventCoordinatorJoinEvidence;
namespace P=Legality::Gen14HistoricalEventPathEvidence;
namespace G=Legality::Gen3DistributionWitnessEvidence;
namespace R=Legality::Gen14EventCoordinatorRoutes;
void w16(std::vector<std::byte>& v,size_t i,uint16_t n){v[i]=std::byte(n&255u);v[i+1]=std::byte(n>>8);}
void w32(std::vector<std::byte>& v,size_t i,uint32_t n){w16(v,i,uint16_t(n));w16(v,i+2,uint16_t(n>>16));}
J::Input original(const G::Native& n){
    J::Input in{};
    in.nativeRecordChecksumValidated=n.available;
    in.original.key={J::Family::Gen3Distribution,42,0,4,n.ids.tid,n.ids.sid,n.originGame,n.language};
    in.original.sourceKeyIndependentlyIdentified=true;
    in.original.originalArchiveIndependentlyAuthenticated=true;
    in.original.digestIndependentlyVerified=true;
    in.original.originalDigest[0]=std::byte{0x42};
    in.original.releaseIndependentlyCorroborated=true;
    in.original.individualReceiptIndependentlyCorroborated=true;
    in.original.originalFieldsIndependentlyExtracted=true;
    in.crossLane.key=in.original.key;
    in.crossLane.pinnedStructuralProjectionIndependentlyChecked=true;
    in.crossLane.originalArchiveIndependentlyAuthenticated=true;
    in.crossLane.originalDigestIndependentlyVerified=true;
    in.crossLane.originalDigest=in.original.originalDigest;
    in.originalHistory.originalGeneration=P::Generation::III;
    in.originalHistory.exactOriginalSourceIndependentlyAttested=true;
    in.originalHistory.originalFieldsIndependentlyExtracted=true;
    in.originalHistory.exhaustiveHistoricalChainVerified=true;
    in.originalHistory.original.known=P::TrainerIDs|P::OriginalBall|P::Language|P::OriginVersion;
    in.originalHistory.original.value[1]=(uint64_t(n.ids.sid)<<16)|n.ids.tid;
    in.originalHistory.original.value[3]=n.ball;
    in.originalHistory.original.value[2]=n.language;
    in.originalHistory.original.value[4]=n.originGame;
    in.observed=in.originalHistory.original;
    in.nativeGeneration=P::Generation::III;
    return in;
}
int main(){
    unsigned layouts=0,negatives=0;
    for(const size_t sz:{Encryption::SIZE_STORED3_FRLG,Encryption::SIZE_PARTY3_FRLG}) {
        for(uint32_t permutation=0;permutation<24;++permutation){
            std::vector<std::byte> plain(sz);
            w32(plain,0,permutation+24);w16(plain,4,7200);w16(plain,6,58364);
            w16(plain,0x20,4);plain[0x12]=std::byte{2};
            w16(plain,0x46,static_cast<uint16_t>(2u<<7|4u<<11|10u));
            plain[0x08]=std::byte{0xc1};plain[0x14]=std::byte{0xc2};
            w16(plain,0x1c,Encryption::checksum3FRLG(plain));
            const std::unique_ptr<std::byte[]> enc(Encryption::encryptArray3FRLG(plain));
            std::vector<std::byte> source(enc.get(),enc.get()+sz);
            const auto preserved=source;
            const auto n=N::inspectEncrypted(source);
            assert(n.available&&n.speciesInternal==4&&n.ids.tid==7200&&n.ball==4);
            auto in=original(n);
            assert(J::evaluate(in).outcome==J::Outcome::ConditionalCandidate);
            // Negative metadata bit flips on an independently witnessed
            // candidate history must exclude only that single origin.
            for(unsigned f=0;f<4;++f){
                auto different=in;
                constexpr unsigned indexes[]={1,3,2,4};
                different.observed.value[indexes[f]]^=1u;
                auto r=J::evaluate(different);
                assert(r.outcome==J::Outcome::CandidateFieldExcluded);
                assert(!r.globallyInvalid&&!r.authenticated);
                ++negatives;
            }
            // Missing L5 independent archive evidence suppresses even a
            // real native mismatch. Current Q07 has precisely this state.
            in.observed.value[1]^=1u;
            in.crossLane.originalArchiveIndependentlyAuthenticated=false;
            assert(J::evaluate(in).outcome==J::Outcome::UnknownOriginalArchive);
            ++negatives;
            // Tampered encrypted block must not yield any native witness.
            auto corrupt=source;corrupt[0x20]^=std::byte{0x80};
            const auto bad=N::inspectEncrypted(corrupt);
            assert(!bad.available);
            auto invalid=original(bad);
            assert(J::evaluate(invalid).outcome==J::Outcome::NativeUnavailable);
            ++negatives;
            // Bad Egg quarantine without modifying source.
            corrupt=source;corrupt[0x13]|=std::byte{1};
            assert(!N::inspectEncrypted(corrupt).available);
            assert(source==preserved);
            ++layouts;
        }
    }
    assert(layouts==48&&negatives==288);
    std::cout<<"L4 Stage 5 strict native PK3->coordinator join: "<<layouts<<" real layouts / "<<negatives<<" boundary negatives PASS\n";
}
