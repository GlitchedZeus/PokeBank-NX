#include "Legality/Gen4WildSpecialStageEvidence.h"
#include <array>
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
            bool found=false;
            for(uint32_t seed=0;seed<4096 && !found;++seed){
                c.prePidSeed=seed;c.pid=R::sequentialPid(seed);
                for(uint32_t level=W::minLevel(row);level<=W::maxLevel(row);++level){
                    c.originalLevel=static_cast<uint8_t>(level);
                    if(!R::matchNoLeadRow(
                            true,row,seed,c.pid,c.originalLevel).matched())
                        continue;
                    const auto pass=S::evaluate(c);
                    assert(pass.status==St::KnownConditionalSpecialEncounter);
                    assert(pass.matchedRng!=R::Method::None);
                    found=true;break;
                }
            }
            assert(found && "pinned HGSS source row must admit a no-lead test seed");
            auto bad=c;bad.originalSpecies=0;
            assert(S::evaluate(bad).status==St::CandidateRowContradicted);
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
