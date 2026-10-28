// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// Joint move-set reachability. Each bit in a state's uint16_t is one of the
// 16 subsets of four queried moves. Independent fathers are never unioned.
// Monotone saturation terminates at the finite least fixed point: there is no
// ancestry depth/time limit whose exhaustion could become an Invalid verdict.
#include <array>
#include <cstdint>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>
namespace Legality::Breeding {
struct Species {
    uint8_t ordinary = 0;       // moves learnable together in this form
    uint8_t previousEra = 0;    // e.g. Time Capsule-only acquisition
    uint8_t inherited = 0;      // moves accepted by the native inheritance script
    std::vector<uint8_t> gifts; // joint sets from separate event/encounter origins
    std::vector<uint16_t> fathers;
    std::vector<uint16_t> evolvesTo;
    // Bits whose selected inheritance source requires BOTH parents to know the
    // move. A mother is one actual reachable individual, not a per-move union.
    // Empty mothers permits paternal-only bits (e.g. a Ditto route), never the
    // both-parent bits. Existing Gen II models leave both fields empty/zero.
    uint8_t requiresMother = 0;
    std::vector<uint16_t> mothers;
};
enum class StepKind : uint8_t { Native, Gift, PreviousEra, Evolution, Breed };
struct Step {
    uint16_t species=0,parentSpecies=0;
    uint8_t mask=0,parentMask=0;
    StepKind kind=StepKind::Native;
    uint16_t motherSpecies=0;
    uint8_t motherMask=0;
};
class FiniteChain {
public:
    explicit FiniteChain(std::span<const Species> species, uint8_t previousEraDomain=15)
        : species_(species), reachable_(species.size(),1), previousEraDomain_(previousEraDomain), evidence_(species.size()) {}
    void solve() {
        for(size_t s=1;s<species_.size();++s)
            for(auto mask:species_[s].gifts)add(s,mask,{static_cast<uint16_t>(s),0,mask,0,StepKind::Gift});
        bool changed;
        do {
            changed=false;
            for(size_t s=1;s<species_.size();++s) {
                const auto& rule=species_[s];
                // Process a snapshot; subsequent rounds close newly learned subsets.
                const uint16_t states=reachable_[s];
                for(uint8_t mask=0;mask<16;++mask)if(states&(uint16_t{1}<<mask)) {
                    changed|=add(s,mask|rule.ordinary,{static_cast<uint16_t>(s),static_cast<uint16_t>(s),mask,mask,StepKind::Native});
                    if((mask&~previousEraDomain_)==0)changed|=add(s,mask|rule.previousEra,{static_cast<uint16_t>(s),static_cast<uint16_t>(s),mask,mask,StepKind::PreviousEra});
                    for(auto to:rule.evolvesTo)if(to<species_.size())changed|=add(to,mask,{to,static_cast<uint16_t>(s),mask,mask,StepKind::Evolution});
                }
                for(auto father:rule.fathers)if(father<species_.size()) {
                    const uint16_t paternal=reachable_[father];
                    for(uint8_t mask=0;mask<16;++mask)if(paternal&(uint16_t{1}<<mask)) {
                        if(rule.mothers.empty()||rule.requiresMother==0) {
                            changed|=add(s,mask&rule.inherited&~rule.requiresMother,
                                {static_cast<uint16_t>(s),father,mask,mask,StepKind::Breed});
                        } else for(auto mother:rule.mothers)if(mother<species_.size()) {
                            const auto maternal=reachable_[mother];
                            for(uint8_t known=0;known<16;++known)if(maternal&(uint16_t{1}<<known)) {
                                const auto inherited=static_cast<uint8_t>(mask&rule.inherited&~(rule.requiresMother&~known));
                                changed|=add(s,inherited,
                                    {static_cast<uint16_t>(s),father,mask,mask,StepKind::Breed,mother,known});
                            }
                        }
                    }
                }
            }
        } while(changed);
    }
    std::vector<Step> witness(size_t species,uint8_t mask)const {
        std::vector<Step> result;
        if(!canKnow(species,mask))return result;
        std::vector<std::pair<size_t,uint8_t>> pending{{species,mask}};
        std::vector<uint16_t> visited(species_.size());
        while(!pending.empty()) {
            const auto [node,wanted]=pending.back();pending.pop_back();
            if(!wanted||(visited[node]&(uint16_t{1}<<wanted)))continue;
            visited[node]|=uint16_t{1}<<wanted;
            const auto& step=evidence_[node][wanted];result.push_back(step);
            if(step.motherSpecies)pending.emplace_back(step.motherSpecies,step.motherMask);
            if(step.parentSpecies)pending.emplace_back(step.parentSpecies,step.parentMask);
        }
        return result;
    }
    bool canKnow(size_t species,uint8_t mask)const noexcept {
        return species<reachable_.size()&&mask<16&&(reachable_[species]&(uint16_t{1}<<mask));
    }
    bool hasFather(size_t child,uint8_t mask)const noexcept {
        if(child>=species_.size()||mask>15)return false;
        const auto& rule=species_[child];
        const auto shared=static_cast<uint8_t>(mask&rule.requiresMother);
        if(shared) {
            bool motherFound=false;
            for(auto mother:rule.mothers)if(canKnow(mother,shared)){motherFound=true;break;}
            if(!motherFound)return false;
        }
        for(auto father:species_[child].fathers)if(canKnow(father,mask))return true;
        return mask==0;
    }
private:
    bool add(size_t species,uint8_t mask,Step evidence) {
        if(mask>15)return false;
        auto& r=reachable_[species];const auto old=r;
        // Forgetting is allowed; exact four-slot/order constraints are checked by
        // the native egg verifier before this necessary joint-ancestry condition.
        for(uint8_t sub=mask;;sub=(sub-1)&mask) {
            const auto bit=static_cast<uint16_t>(uint16_t{1}<<sub);
            if(!(r&bit)){evidence.mask=sub;evidence_[species][sub]=evidence;}
            r|=bit;
            if(sub==0)break;
        }
        return old!=r;
    }
    std::span<const Species> species_;
    std::vector<uint16_t> reachable_;
    uint8_t previousEraDomain_;
    std::vector<std::array<Step,16>> evidence_;
};
} // namespace Legality::Breeding
