#include "Legality/Gen3GameCubeOriginalNativeStage.h"
#include "Legality/Gen3GameCubeOriginalTransferHistory.h"
#include "Legality/Gen4SinnohOriginalHoneyHistory.h"
#include "l1_native_encrypted_formats.h"
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
namespace N=L1NativeEncryptedFormats;
namespace G=Legality::Gen3GameCubeOriginalNativeStage;
namespace H=Legality::Gen4SinnohOriginalHoneyHistory;
int main(){
    unsigned pk3pass=0,pk4pass=0,nativeNeg=0,unknown=0;
    for(unsigned shuffle=0;shuffle<24;++shuffle){
        for(unsigned rawSize:{0x50u,0x64u}){
            for(unsigned gc=0;gc<2;++gc){
                std::vector<uint8_t> source(rawSize,0);
                const uint32_t pid=0x11223300u+shuffle;
                const uint32_t ot=0x55667788u^pid;
                N::wr32(source,0,pid);N::wr32(source,4,ot);
                for(unsigned i=0x20;i<0x50;++i)
                    source[i]=static_cast<uint8_t>((i*41u+shuffle*7u+gc*9u)&255u);
                N::wr16(source,0x20,197u);
                N::wr16(source,0x46,static_cast<uint16_t>((15u<<7)|37u));
                N::wr32(source,0x48,0x26a2a925u);
                const auto encrypted=N::encrypt3(source);
                assert(encrypted.size()==rawSize && N::valid3(encrypted));
                assert(!std::equal(encrypted.begin()+0x20,encrypted.begin()+0x50,source.begin()+0x20));
                auto decrypted=N::decrypt3(encrypted);
                // Checksum is in unencrypted header and is refreshed when encryption runs.
                N::wr16(source,0x1C,N::checksum(source,0x20,0x50));
                assert(decrypted==source);
                const std::vector<uint8_t> untouched=encrypted;
                G::Candidate c{};
                c.originalGame=gc?G::Game::XD:G::Game::Colosseum;
                c.originalRoute=G::Route::ShadowPurified;
                c.originalSpecies=N::rd16(decrypted,0x20);
                c.originalPersonality=N::rd32(decrypted,0);
                c.originalTrainerId=N::rd32(decrypted,4);
                c.native.species=c.originalSpecies;
                c.native.personality=c.originalPersonality;
                c.native.originalTrainerId=c.originalTrainerId;
                c.native.originGame=static_cast<uint16_t>((N::rd16(decrypted,0x46)>>7)&15u);
                c.native.checksumAndDecryptionVerified=N::valid3(encrypted);
                c.originalGameIndependentlyAttested=true;
                c.originalSceneIndependentlyAttested=true;
                c.originalCapturedPidSpeciesOtIndependentlyAttested=true;
                c.originalPurificationIndependentlyAttested=true;
                assert(G::evaluate(c).status==G::Status::ConditionalGameCubeOriginHistory);
                // Original encrypted PK3 buffer -> native physical fields
                // -> strictly conditional independently attested scene and
                // original capture/recipient witnesses. The synthetic source
                // flags exercise the bridge; they do NOT authenticate retail
                // GameCube provenance.
                namespace T=Legality::Gen3GameCubeOriginalTransferHistory;
                const uint32_t iv32=N::rd32(decrypted,0x48);
                const std::array<uint8_t,6> decodedIvs{{
                    static_cast<uint8_t>(iv32&31u),
                    static_cast<uint8_t>((iv32>>5)&31u),
                    static_cast<uint8_t>((iv32>>10)&31u),
                    static_cast<uint8_t>((iv32>>15)&31u),
                    static_cast<uint8_t>((iv32>>20)&31u),
                    static_cast<uint8_t>((iv32>>25)&31u)
                }};
                T::Candidate capture{};
                capture.specimenIdentity=shuffle+1;
                capture.originalGameCubeSaveIdentity=10;
                capture.originalPhysicalSourceRoute=11;
                capture.recipientSaveIdentity=12;
                capture.originalRootIndependentlyAttested=true;
                capture.originalSceneAndRouteIndependentlyAttested=true;
                capture.originalSourceEvaluatorIndependentlyReplayed=true;
                capture.sourceCheck=T::SourceCheck::SourceMatched;
                capture.originalIvsIndependentlyAttested=true;
                capture.originalCapturedIvs=decodedIvs;
                capture.originalRecipientTransferIndependentlyAttested=true;
                capture.native=c;
                capture.native.originalRoute=G::Route::FixedGiftOrTrade;
                capture.native.native.ivs=decodedIvs;
                assert(T::evaluate(capture).status==
                       T::Status::ConditionalGameCubeTransferOnly);
                auto missed=capture;missed.originalIvsIndependentlyAttested=false;
                assert(T::evaluate(missed).status==
                       T::Status::UnknownIndependentNativeIvs);
                missed=capture;missed.native.native.ivs[0]^=1u;
                assert(T::evaluate(missed).status==
                       T::Status::CandidateNativeIvsContradicted);
                missed=capture;missed.native.native.ivs[0]=255u;
                assert(T::evaluate(missed).status==
                       T::Status::UnknownMalformedNativeIvs);
                missed=capture;missed.native.native.checksumAndDecryptionVerified=
                    false;
                assert(T::evaluate(missed).status==
                       T::Status::UnknownNativeSnapshot);
                ++pk3pass;
                auto bad=c;bad.native.species^=1u;
                assert(G::evaluate(bad).status==G::Status::CandidateNativeIdentityContradicted);++nativeNeg;
                bad=c;bad.originalGameIndependentlyAttested=false;
                assert(G::evaluate(bad).status==G::Status::UnknownOriginalGame);++unknown;
                bad=c;bad.originalPurificationIndependentlyAttested=false;
                assert(G::evaluate(bad).status==G::Status::UnknownPurificationHistory);++unknown;
                bad=c;bad.native.originGame=3;
                assert(G::evaluate(bad).status==G::Status::CandidateNativeOriginContradicted);++nativeNeg;
                auto corrupt=encrypted;corrupt[0x20]^=1u;
                assert(!N::valid3(corrupt));++nativeNeg;
                assert(encrypted==untouched);
            }
        }
    }
    for(unsigned shuffled=0;shuffled<32;++shuffled){
        for(unsigned rawSize:{0x88u,0xECu}){
            for(unsigned variant=0;variant<9;++variant){
                std::vector<uint8_t> source(rawSize,0);
                const uint32_t pid=static_cast<uint32_t>((shuffled<<13)|(variant<<3)|0x123u);
                N::wr32(source,0,pid);
                for(unsigned i=8;i<0x88;++i)
                    source[i]=static_cast<uint8_t>((i*29u+variant*17u+shuffled*11u)&255u);
                N::wr16(source,8,446);
                N::wr32(source,0x38,0x12345678u);
                source[0x5F]=12; // example Platinum native origin marker
                N::wr16(source,0x80,20);
                source[0x84]=12;
                if(rawSize==0xEC)
                    for(unsigned i=0x88;i<rawSize;++i)source[i]=static_cast<uint8_t>(i^variant);
                const auto encrypted=N::encrypt4(source);
                assert(N::valid4(encrypted));
                assert(!std::equal(encrypted.begin()+8,encrypted.begin()+0x88,source.begin()+8));
                N::wr16(source,6,N::checksum(source,8,0x88));
                const auto decrypted=N::decrypt4(encrypted);
                assert(decrypted==source);
                assert(N::rd16(decrypted,8)==446u);
                assert(N::rd16(decrypted,0x80)==20u);
                assert(decrypted[0x5F]==12u);
                auto tampered=encrypted;tampered[0x41]^=0x40u;
                assert(!N::valid4(tampered));++nativeNeg;
                // Original honey save-state, unlike native PK4, must still
                // be independently attested before joining this snapshot.
                H::Candidate honey{};honey.exactGame="platinum_nds";
                honey.originalSpecies=N::rd16(decrypted,8);
                honey.originalTreeIndex=0;
                honey.originalMetLocation=H::kOriginalMetLocations[0];
                honey.originalMinutesRemaining=1080;
                honey.savedEncounterGroup=H::Group::Munchlax;
                honey.savedEncounterRow=1;
                assert(H::evaluate(honey).status==H::Status::UnknownOriginalSave);++unknown;
                ++pk4pass;
            }
        }
    }
    assert(N::decrypt3(std::array<uint8_t,1>{0}).empty());
    assert(N::decrypt4(std::array<uint8_t,1>{0}).empty());
    std::cout<<"L1 S4 native encrypted PK3: "<<pk3pass<<", native PK4: "<<pk4pass
        <<", local negatives "<<nativeNeg<<", Unknown "<<unknown<<" PASS\n";
}
