// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <span>
#include <unordered_map>
#include <vector>

namespace Legality::EvExp {
using Evs = std::array<uint8_t, 6>; // HP, Attack, Defense, Speed, Sp.Atk, Sp.Def
enum class Verdict : uint8_t { Proven, Impossible, Incomplete };
enum class Operation : uint8_t { Vitamin, Berry, Battle };
struct Battle {
    Evs yield{};
    // A source-supported upper bound on the actual EXP awarded in this
    // encounter/recipient context. Spare EXP can be filled with Rare Candies
    // (decline unwanted moves/evolution), then Day Care within the final level.
    uint32_t experience = 0;
    uint16_t species = 0;
    uint8_t level = 0;
    uint8_t version = 0;
};
struct Request {
    Evs target{};
    uint32_t availableExperience = 0;
    uint8_t generation = 0;
    bool initialEvsKnownZero = false;
    bool battlesRequireExperience = false;
    bool canHavePokerus = false;
    bool berriesAvailable = false;
    size_t stateLimit = 2048;
};
struct Step {
    Operation operation = Operation::Vitamin;
    uint16_t battle = 0;
    uint8_t stat = 0;
    // 0: no EV item, 1: Macho Brace, 2..7: the six Power items.
    uint8_t item = 0;
    bool pokerus = false;
};
struct Result {
    Verdict verdict = Verdict::Incomplete;
    uint32_t necessaryExperience = 0, witnessExperience = 0;
    size_t explored = 0;
    bool exhausted = false;
    std::vector<Step> witness;
};
inline unsigned total(const Evs& evs) {
    unsigned sum = 0; for (auto ev : evs) sum += ev; return sum;
}
inline Evs apply(Evs evs, const Step& step, std::span<const Battle> battles, uint8_t generation) {
    if (step.operation == Operation::Vitamin) {
        const auto i = step.stat;
        if (i < 6 && evs[i] < 100)
            evs[i] += static_cast<uint8_t>(std::min({10u, 100u-evs[i], 510u-total(evs)}));
    } else if (step.operation == Operation::Berry) {
        const auto i = step.stat;
        if (i < 6) evs[i] = generation == 4 && evs[i] > 100 ? 100 : evs[i] > 10 ? evs[i]-10 : 0;
    } else if (step.battle < battles.size()) {
        const auto& b = battles[step.battle];
        for (unsigned i = 0; i < 6; ++i) {
            unsigned gain = b.yield[i];
            if (step.item == 1) gain *= 2;
            else if (generation == 4 && step.item == i+2) gain += 4;
            if (step.pokerus) gain *= 2;
            evs[i] += static_cast<uint8_t>(std::min({gain, 255u-evs[i], 510u-total(evs)}));
        }
    }
    return evs;
}

// This is deliberately two proofs. The global necessary bound covers all
// native battle yields, vitamin/berry sequences and item/recipient alternatives.
// The bounded search constructs one complete six-stat action sequence. Failure
// of that search, or an incomplete action catalog, NEVER proves impossibility.
inline Result solve(const Request& r, std::span<const Battle> battles) {
    Result out;
    if (total(r.target) > 510) { out.verdict = Verdict::Impossible; return out; }
    if ((r.generation != 3 && r.generation != 4) || !r.initialEvsKnownZero) return out;
    unsigned beyondVitamins = 0;
    bool vitaminOnly = true;
    for (auto ev : r.target) {
        beyondVitamins += ev > 100 ? ev-100 : 0;
        vitaminOnly &= ev <= 100 && ev % 10 == 0;
    }
    // Relax even the stored Pokerus state for the negative proof. One held
    // item cannot be both a Macho Brace and a Power item; native opponents
    // yield at most three total EVs. These are joint, not per-stat budgets.
    const unsigned maximumBattleGain = r.generation == 3 ? 12 : 14;
    out.necessaryExperience = (beyondVitamins + maximumBattleGain-1) / maximumBattleGain;
    if (!vitaminOnly) out.necessaryExperience = std::max(1u, out.necessaryExperience);
    if (r.battlesRequireExperience && r.availableExperience < out.necessaryExperience) {
        out.verdict = Verdict::Impossible; return out;
    }
    Evs initial{};
    for (uint8_t i = 0; i < 6; ++i)
        for (unsigned j = 0; j < std::min<unsigned>(r.target[i],100)/10; ++j) {
            const Step step{Operation::Vitamin,0,i,0,false};
            initial = apply(initial,step,battles,r.generation); out.witness.push_back(step);
        }
    if (initial == r.target) { out.verdict = Verdict::Proven; return out; }
    const auto initialSteps = out.witness;
    out.witness.clear();
    struct Action { Step step; uint32_t experience; };
    std::vector<Action> actions;
    for (uint8_t i = 0; i < 6; ++i) {
        actions.push_back({{Operation::Vitamin,0,i,0,false},0});
        if (r.berriesAvailable) actions.push_back({{Operation::Berry,0,i,0,false},0});
    }
    for (size_t b = 0; b < battles.size() && b <= 65535; ++b) {
        if (!battles[b].experience || !total(battles[b].yield) || total(battles[b].yield) > 3) continue;
        for (uint8_t item = 0; item < (r.generation == 3 ? 2 : 8); ++item)
            for (unsigned virus = 0; virus <= unsigned(r.canHavePokerus); ++virus)
                actions.push_back({{Operation::Battle,static_cast<uint16_t>(b),0,item,bool(virus)},battles[b].experience});
    }
    for (const auto& action : actions) {
        if (action.step.operation == Operation::Battle && action.experience <= r.availableExperience &&
            apply(initial,action.step,battles,r.generation) == r.target) {
            out.verdict=Verdict::Proven;out.witnessExperience=action.experience;
            out.witness=initialSteps;out.witness.push_back(action.step);return out;
        }
    }
    // A deliberately simple constructive route avoids exploring a large
    // state space when six independently sourced one-EV opponents suffice.
    std::array<size_t,6> basis{};basis.fill(battles.size());
    for (size_t b=0;b<battles.size();++b) if (battles[b].experience && total(battles[b].yield)==1)
        for (size_t stat=0;stat<6;++stat) if (battles[b].yield[stat]==1 &&
            (basis[stat]==battles.size() || battles[b].experience<battles[basis[stat]].experience)) basis[stat]=b;
    uint64_t cost=0;bool constructible=true;
    for (size_t stat=0;stat<6;++stat) if (r.target[stat]>initial[stat]) {
        if (basis[stat]==battles.size()) {constructible=false;break;}
        cost+=uint64_t(r.target[stat]-initial[stat])*battles[basis[stat]].experience;
    }
    if (constructible && cost<=r.availableExperience) {
        out.verdict=Verdict::Proven;out.witnessExperience=static_cast<uint32_t>(cost);out.witness=initialSteps;
        for (uint8_t stat=0;stat<6;++stat)
            for (unsigned i=initial[stat];i<r.target[stat];++i)
                out.witness.push_back({Operation::Battle,static_cast<uint16_t>(basis[stat]),0,0,false});
        return out;
    }
    const auto key = [](Evs evs, bool virus) {
        uint64_t k = uint64_t(virus) << 48;
        for (unsigned i = 0; i < 6; ++i) k |= uint64_t(evs[i]) << (8*i);
        return k;
    };
    const auto distance = [&](Evs evs) {
        unsigned n = 0; for (unsigned i=0;i<6;++i) n += evs[i]>r.target[i] ? evs[i]-r.target[i] : r.target[i]-evs[i]; return n;
    };
    struct Node { Evs evs; bool virus; uint32_t cost; size_t parent,action; };
    std::vector<Node> nodes{{initial,false,0,0,0}};
    std::unordered_map<uint64_t,uint32_t> costs{{key(initial,false),0}};
    using QueueItem = std::pair<uint64_t,size_t>;
    std::priority_queue<QueueItem,std::vector<QueueItem>,std::greater<QueueItem>> pending;
    pending.emplace(distance(initial),0);
    while (!pending.empty()) {
        const auto index = pending.top().second; pending.pop();
        const Node current = nodes[index];
        if (costs[key(current.evs,current.virus)] != current.cost) continue;
        ++out.explored;
        if (current.evs == r.target) {
            out.verdict = Verdict::Proven; out.witnessExperience = current.cost;
            for (auto n=index; n; n=nodes[n].parent) out.witness.push_back(actions[nodes[n].action].step);
            std::reverse(out.witness.begin(),out.witness.end());
            out.witness.insert(out.witness.begin(),initialSteps.begin(),initialSteps.end());
            return out;
        }
        for (size_t a = 0; a < actions.size(); ++a) {
            const auto& action = actions[a];
            if (current.virus && action.step.operation == Operation::Battle && !action.step.pokerus) continue;
            if (action.experience > r.availableExperience-current.cost) continue;
            auto next = apply(current.evs,action.step,battles,r.generation);
            if (next == current.evs) continue;
            const bool virus = current.virus || action.step.pokerus;
            const auto k=key(next,virus);const auto cost=current.cost+action.experience;
            const auto old=costs.find(k);if (old!=costs.end() && old->second<=cost) continue;
            if (nodes.size() >= r.stateLimit) { out.exhausted = true; continue; }
            costs[k]=cost;nodes.push_back({next,virus,cost,index,a});
            // Goal-directed order only affects which constructive witness is
            // found first. Exhaustion retains Incomplete, never Invalid.
            pending.emplace(uint64_t(cost)+distance(next),nodes.size()-1);
        }
    }
    return out;
}
} // namespace Legality::EvExp
