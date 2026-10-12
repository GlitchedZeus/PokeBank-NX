#include "Legality/Gen4WildSpecialStageEvidence.h"
#include "Encryption/Encryption4.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <cassert>
#include <cstddef>
#include <iostream>
int main(){
    namespace S=Legality::Gen4WildSpecialStageEvidence;
    namespace W=Legality::Gen4Wild;
    namespace R=Legality::Gen4WildRng;
    using St=S::Status;
    static_assert(S::category(2)==S::Category::Fishing);
    static_assert(S::category(5)==S::Category::RockSmash);
    static_assert(S::category(6)==S::Category::Headbutt);
    static_assert(S::category(8)==S::Category::BugContest);
    static_assert(S::category(10)==S::Category::Safari);
    static_assert(S::category(0)==S::Category::Unknown);
    // Real generated HG/SS row indices from pinned Gen4WildEncounterData:
    // Type 3 fishing, 5 Rock Smash, 6 Headbutt, 8 contest, 10 Safari.
    // Each source row is separately rechecked against exact game/type.
    constexpr std::array<std::size_t,5> hg{
        9302,9271,8495,8477,8605
    };
    constexpr std::array<std::size_t,5> ss{
        15159,15104,14337,14299,14438
    };
    for(int game=0;game<2;++game){
        const auto ids=game?ss:hg;
        const auto requiredGame=game?W::Game::SoulSilver:W::Game::HeartGold;
        const auto originalId=game?"soulsilver_nds":"heartgold_nds";
        for(std::size_t k=0;k<ids.size();++k){
            const auto row=W::kPackedGen4WildEncounters[ids[k]];
            assert(W::game(row)==requiredGame);
            assert(S::category(W::method(row))==
                   static_cast<S::Category>(k==0?1:k==1?2:k==2?3:k==3?4:5));
            S::Candidate c{};
            c.exactGame=originalId;c.rowIndex=ids[k];
            c.originalWildSourceIndependentlyAttested=true;
            c.originalRowIndependentlyAttested=true;
            c.captureTimeAreaAndMethodConditionsIndependentlyAttested=true;
            c.originalNoLeadAndMethodKIndependentlyAttested=true;
            c.originalPidIvStageIndependentlyAttested=true;
            c.originalSpecies=W::species(row);
            c.originalLocation=W::location(row);
            c.originalForm=W::form(row)>=30?0:W::form(row);
            // Fixed independent Method-K source-vector controls, not a search
            // that asks the matcher under test to construct its own witness.
            // Indices 0..4: Good Rod, Rock Smash, Headbutt, Contest, Safari.
            constexpr std::array<uint32_t,5> seeds{13u,12u,17u,8u,8u};
            constexpr std::array<uint8_t,5> levels{20u,26u,5u,14u,15u};
            c.prePidSeed=seeds[k];
            c.pid=R::sequentialPid(seeds[k]);
            c.originalLevel=levels[k];
            assert(W::levelMatches(row,c.originalLevel));
            uint32_t state=seeds[k];
            state=Legality::Gen3PidIv::Detail::next(state);
            state=Legality::Gen3PidIv::Detail::next(state);
            state=Legality::Gen3PidIv::Detail::next(state);
            const uint16_t a=static_cast<uint16_t>((state>>16)&0x7FFFu);
            state=Legality::Gen3PidIv::Detail::next(state);
            const uint16_t b=static_cast<uint16_t>((state>>16)&0x7FFFu);
            c.originalIVs={
                static_cast<uint8_t>(a&31u),
                static_cast<uint8_t>((a>>5)&31u),
                static_cast<uint8_t>((a>>10)&31u),
                static_cast<uint8_t>(b&31u),
                static_cast<uint8_t>((b>>5)&31u),
                static_cast<uint8_t>((b>>10)&31u)};
            const auto pidiv=Legality::Gen4PidIv::analyze(c.pid,c.originalIVs);
            assert(pidiv.matched() && pidiv.originSeed==seeds[k]);
            const auto frame=R::matchNoLeadRow(
                true,row,c.prePidSeed,c.pid,c.originalLevel);
            assert(frame.matched());
            const auto pass=S::evaluate(c);
            assert(pass.status==St::KnownConditionalSpecialEncounter);
            assert(pass.matchedRng==frame.method);

            // A real encrypted PK4 storage format/reader, with original
            // source-game byte, native species, location, level, PID and IVs.
            // Capture history remains synthetic and independently supplied.
            std::vector<std::byte> raw(Encryption::SIZE_STORED4,std::byte{0});
            auto set16=[&](std::size_t at,uint16_t v) {
                raw[at]=static_cast<std::byte>(v&255u);
                raw[at+1]=static_cast<std::byte>(v>>8);
            };
            auto set32=[&](std::size_t at,uint32_t v) {
                set16(at,static_cast<uint16_t>(v));
                set16(at+2,static_cast<uint16_t>(v>>16));
            };
            set32(0x00,c.pid);
            set16(0x08,c.originalSpecies);
            raw[0x17]=std::byte{2}; // English
            raw[0x5F]=static_cast<std::byte>(game?8:7); // SS/HG exact retail origin
            uint32_t packedIVs=0;
            for(uint32_t j=0;j<6;++j)
                packedIVs|=static_cast<uint32_t>(c.originalIVs[j])<<(j*5);
            set32(0x38,packedIVs); // not an egg
            set16(0x80,c.originalLocation);
            raw[0x84]=static_cast<std::byte>(c.originalLevel);
            const auto encrypted=Encryption::encryptArray4(raw);
            Pokemon::Pokemon4ReadOnly native(encrypted,Enums::GameVersion::HGSS);
            assert(native.valid() && native.checksumValid() && !native.isEgg());
            assert(native.pid()==c.pid && native.ivs()==c.originalIVs);
            const std::vector<std::byte> untouched(
                native.originalEncryptedBytes().begin(),
                native.originalEncryptedBytes().end());
            S::NativeWitness witness{&native,true,true};
            assert(S::evaluateNative(c,witness).status==
                   St::KnownConditionalSpecialEncounter);
            auto unknownWitness=witness;
            unknownWitness.independentlyVerifiedSameCapturedCreature=false;
            assert(S::evaluateNative(c,unknownWitness).status==
                   St::UnknownNativeSnapshot);
            unknownWitness=witness;
            unknownWitness.independentlyVerifiedOriginalNativeSave=false;
            assert(S::evaluateNative(c,unknownWitness).status==
                   St::UnknownNativeSource);
            Pokemon::Pokemon4ReadOnly alienNative(
                encrypted,Enums::GameVersion::PT);
            assert(S::evaluateNative(c,{&alienNative,true,true}).status==
                   St::UnknownNativeSource);
            auto wrongObservation=c;wrongObservation.pid^=1u;
            assert(S::evaluateNative(wrongObservation,witness).status==
                   St::IncoherentNativeRecord);
            auto corrupted=encrypted;
            corrupted[17]^=std::byte{0xFF};
            Pokemon::Pokemon4ReadOnly badChecksum(
                corrupted,Enums::GameVersion::HGSS);
            assert(S::evaluateNative(c,{&badChecksum,true,true}).status==
                   St::IncoherentNativeRecord);
            assert(std::equal(untouched.begin(),untouched.end(),
                              native.originalEncryptedBytes().begin()));
            auto bad=c;bad.originalSpecies=0;
            assert(S::evaluate(bad).status==St::CandidateRowContradicted);
            bad=c;bad.originalIVs[0]=static_cast<uint8_t>((bad.originalIVs[0]+1u)%32u);
            assert(S::evaluate(bad).status==St::CandidatePidIvContradicted);
            bad=c;bad.pid^=1u;
            assert(S::evaluate(bad).status==St::CandidatePidIvContradicted);
            auto missing=c;missing.captureTimeAreaAndMethodConditionsIndependentlyAttested=false;
            assert(S::evaluate(missing).status==St::UnknownCaptureTimeContext);
            missing=c;missing.originalNoLeadAndMethodKIndependentlyAttested=false;
            assert(S::evaluate(missing).status==St::UnknownLeadOrRngFamily);
            missing=c;missing.originalPidIvStageIndependentlyAttested=false;
            assert(S::evaluate(missing).status==St::UnknownOriginalNatureStage);
            missing=c;missing.originalRowIndependentlyAttested=false;
            assert(S::evaluate(missing).status==St::UnknownOriginalRow);
            missing=c;missing.exactGame=game?"heartgold_nds":"soulsilver_nds";
            assert(S::evaluate(missing).status==St::UnknownOriginalRow);
        }
    }
    std::cout<<"L1 E13 HGSS five special families x 2 games: PASS\n";
}
