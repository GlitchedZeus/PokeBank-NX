// L5 Stage 3: independently implemented, method-scoped Gen I-IV reference differential.
// Only pinned public rule SEMANTICS are reimplemented; no PKHeX code is included.
// This is not a whole-Pokemon verdict or authenticated retail specimen.
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
enum class Rule : uint8_t { None, EggEV, Total510, GBStats, EVGain100, EVGainEXP };
enum class Grade : uint8_t { NoContradiction, Invalid, UnknownHistory, FormatRejected };
enum class Delta : uint8_t { Agreement, ReferenceMissingInNX, NXOverstates,
                             UnresolvedHistory, ParserMismatch, SeverityMismatch };
struct Input {
    int generation = 3;
    bool egg = false;
    std::array<uint16_t,6> ev{};      // Gen III/IV native byte values.
    std::array<uint16_t,5> statExp{}; // Gen I/II native 16-bit values.
    bool sourceKnown = true;
    bool encounterKnown = false;
    bool encounterAt100 = false;
    bool unchangedEXP = false;
    bool nativeStructureGood = true;
};
struct Judgement { Grade grade; Rule rule; };
struct Classification { Delta category; Judgement reference; Judgement nxRule; };
uint64_t assertions=0, comparisons=0, agreements=0, gaps=0, unknowns=0, parser=0, sev=0;
#define REQUIRE(v) do { ++assertions; if (!(v)) { std::cerr << "L5-Stage3 FAIL line " << __LINE__ << ": " << #v << '\n'; std::abort(); } } while(false)
constexpr uint32_t sumEV(const Input& x) noexcept {
    uint32_t n = 0;
    if (x.generation < 3) { for (uint16_t stat : x.statExp) n += stat; }
    else { for (uint16_t ev : x.ev) n += ev; }
    return n;
}
constexpr bool withinVitamin34(const Input& x) noexcept {
    for (uint16_t ev : x.ev) if (ev != 0 && (ev > 100 || ev % 10 != 0)) return false;
    return true;
}
constexpr bool learnedGBStatExp(const Input& x) noexcept {
    for (uint16_t stat : x.statExp)
        if (stat > 25600 || stat % 2560 != 0) return true;
    return false;
}
// Reference assertions explicitly bounded to the pinned method's local evidence.
// Reference: PKHeX@6501f0a EffortValueVerifier / EffortValues / PK1 / PK2.
constexpr Judgement reference(const Input& x) noexcept {
    if (!x.nativeStructureGood) return {Grade::FormatRejected, Rule::None};
    if (x.egg && sumEV(x) != 0) return {Grade::Invalid, Rule::EggEV};
    if (x.generation <= 2) {
        if (learnedGBStatExp(x)) {
            for (uint16_t stat : x.statExp)
                if (stat == 0) return {Grade::Invalid, Rule::GBStats};
        }
        return {Grade::NoContradiction, Rule::None};
    }
    if (sumEV(x) > 510) return {Grade::Invalid, Rule::Total510};
    if (x.generation == 3 || x.generation == 4) {
        if (!withinVitamin34(x)) {
            if (!x.encounterKnown) return {Grade::UnknownHistory, Rule::None};
            if (x.encounterAt100) return {Grade::Invalid, Rule::EVGain100};
            if (x.unchangedEXP) return {Grade::Invalid, Rule::EVGainEXP};
        }
    }
    return {Grade::NoContradiction, Rule::None};
}
// The deliberately narrow, independently read CURRENT NX production EV path:
// Gen I/II adapter getters are neutral, no explicit stat-experience checks;
// Gen III/IV current engine checks sum>510 and unhatched nonzero EVs.
// This is a source-contract MODEL until paired with live Legality::analyze.
constexpr Judgement nxCurrent(const Input& x) noexcept {
    if (!x.nativeStructureGood) return {Grade::FormatRejected, Rule::None};
    if (x.generation <= 2) return {Grade::NoContradiction, Rule::None};
    if (sumEV(x) > 510) return {Grade::Invalid, Rule::Total510};
    if (x.egg && sumEV(x) != 0) return {Grade::Invalid, Rule::EggEV};
    return {Grade::NoContradiction, Rule::None};
}
constexpr Classification compare(const Input& x) noexcept {
    const Judgement ref=reference(x), nx=nxCurrent(x);
    if (ref.grade==Grade::FormatRejected || nx.grade==Grade::FormatRejected)
        return {ref.grade==nx.grade?Delta::Agreement:Delta::ParserMismatch,ref,nx};
    if (ref.grade==Grade::UnknownHistory || nx.grade==Grade::UnknownHistory)
        return {Delta::UnresolvedHistory,ref,nx};
    if (ref.grade==nx.grade && ref.rule==nx.rule)
        return {Delta::Agreement,ref,nx};
    if (ref.grade==Grade::Invalid && nx.grade!=Grade::Invalid)
        return {Delta::ReferenceMissingInNX,ref,nx};
    if (nx.grade==Grade::Invalid && ref.grade!=Grade::Invalid)
        return {Delta::NXOverstates,ref,nx};
    return {Delta::SeverityMismatch,ref,nx};
}
void evaluate(const Input& x, Delta expected) {
    const auto got=compare(x);
    ++comparisons;
    REQUIRE(got.category==expected);
    switch(got.category) {
        case Delta::Agreement:++agreements;break;
        case Delta::ReferenceMissingInNX:++gaps;break;
        case Delta::UnresolvedHistory:++unknowns;break;
        case Delta::ParserMismatch:++parser;break;
        case Delta::SeverityMismatch:++sev;break;
        case Delta::NXOverstates:break;
    }
}
void directed() {
    // Actual pinned reference behavioral controls (no fabricated encounter history).
    for(int gen:{1,2}) {
        Input x; x.generation=gen; x.egg=(gen==2);
        for(uint16_t v:{0u,1u,2559u,2560u,25600u,25601u,65535u}) {
            x.statExp={v,0,0,0,0};
            evaluate(x, v>0&&gen==2?Delta::ReferenceMissingInNX:
                        (v!=0 && v!=2560 && v!=25600 && gen==1?Delta::ReferenceMissingInNX:Delta::Agreement));
        }
        x.egg=false;x.statExp={1,1,1,1,1};evaluate(x,Delta::Agreement);
        x.statExp={1,0,1,1,1};evaluate(x,Delta::ReferenceMissingInNX);
        x.statExp={2560,0,0,0,0};evaluate(x,Delta::Agreement);
    }
    for(int gen:{3,4}) {
        Input x; x.generation=gen;
        for(int ev:{0,1,100,101,252,253,254,255}) {
            x.ev={static_cast<uint16_t>(ev),0,0,0,0,0};
            evaluate(x,ev==0||ev==100?Delta::Agreement:Delta::UnresolvedHistory);
            x.egg=true;
            evaluate(x,Delta::Agreement);
            x.egg=false;
            x.encounterKnown=true;x.encounterAt100=true;
            evaluate(x,ev==0||ev==100?Delta::Agreement:Delta::ReferenceMissingInNX);
            x.encounterAt100=false;x.unchangedEXP=true;
            evaluate(x,ev==0||ev==100?Delta::Agreement:Delta::ReferenceMissingInNX);
            x.encounterKnown=false;x.unchangedEXP=false;
        }
        x.ev={255,255,0,0,0,0};evaluate(x,Delta::UnresolvedHistory);
        x.ev={255,255,1,0,0,0};evaluate(x,Delta::Agreement);
        x.nativeStructureGood=false;evaluate(x,Delta::Agreement);
    }
}
void exhaustive() {
    // Complete 256x256 two-stat EV domain for both original Gen III/IV formats.
    for(int gen:{3,4}) {
        Input x;x.generation=gen;
        for(uint16_t a=0;a<256;a++)for(uint16_t b=0;b<256;b++) {
            x.ev={a,b,0,0,0,0};
            const bool vitamins=withinVitamin34(x);
            evaluate(x,vitamins?Delta::Agreement:Delta::UnresolvedHistory);
            x.encounterKnown=true; x.encounterAt100=true;
            evaluate(x,vitamins?Delta::Agreement:Delta::ReferenceMissingInNX);
            x.encounterAt100=false; x.unchangedEXP=true;
            evaluate(x,vitamins?Delta::Agreement:Delta::ReferenceMissingInNX);
            x.encounterKnown=false;x.unchangedEXP=false;
            x.egg=true;evaluate(x,Delta::Agreement);
            x.egg=false;
        }
    }
    // Exhaust each 16-bit Gen I/II stat-experience value in one slot.
    for(int gen:{1,2}) {
        Input x;x.generation=gen;
        for(uint32_t v=0;v<=65535;v++) {
            x.statExp={static_cast<uint16_t>(v),0,0,0,0};
            bool invalid=learnedGBStatExp(x) && v!=0;
            evaluate(x, invalid?Delta::ReferenceMissingInNX:Delta::Agreement);
        }
    }
}
void classificationNegativeControls() {
    Input x; x.generation=3; x.ev={253,0,0,0,0,0};
    REQUIRE(compare(x).category==Delta::UnresolvedHistory);
    // Demonstrate that missing encounter data must never be treated as a hard contradiction.
    x.encounterKnown=true;x.encounterAt100=true;
    REQUIRE(compare(x).category==Delta::ReferenceMissingInNX);
    x.nativeStructureGood=false;
    REQUIRE(compare(x).category==Delta::Agreement);
    // Make sure a truly missing parser result differs from a verdict, not "illegal".
    REQUIRE((Grade::FormatRejected!=Grade::Invalid));
    // A known source label by itself does not complete an original route.
    x.nativeStructureGood=true;x.sourceKnown=false;x.encounterKnown=false;
    REQUIRE(compare(x).category==Delta::UnresolvedHistory);
}
}
int main() {
    directed(); exhaustive();classificationNegativeControls();
    REQUIRE(comparisons==65536ull*2ull*4ull+65536ull*2ull+20ull+70ull);
    REQUIRE(gaps>0 && unknowns>0 && agreements>0);
    REQUIRE(parser==0 && sev==0);
    std::cout << "L5 Stage3 PINNED reference DIFFERENTIAL PASS: " << comparisons
              << " compared; agreement=" << agreements << ", source-scope gaps=" << gaps
              << ", unresolved histories=" << unknowns << "; assertions=" << assertions
              << " (method scoped, synthetic, no live PKHeX/full-analyzer execution)\n";
}
