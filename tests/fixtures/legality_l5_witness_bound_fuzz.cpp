// L5 Q16 source-triage fuzz, independent & read-only. NO original save, no global legality verdict.
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
enum class Result : uint8_t { Compatible, ContradictsOneCandidate, Unknown };
struct Witness {
    bool versionKnown=false, nativeFieldsKnown=false, stageKnown=false, methodsExhaustive=false;
    bool originalSourceKnown=false, sameSource=true, duplicateRoute=false, missingStage=false;
    bool allegedContradiction=false;
    std::array<int, 6> ivs{{0,0,0,0,0,0}};
    std::array<int, 4> moves{{0,0,0,0}};
    int maximumMove=467;
};
constexpr Result classify(const Witness& h) {
    // A malformed native-specified field is a LOCAL candidate contradiction.
    if (h.nativeFieldsKnown) {
        for (int x : h.ivs) if (x < 0 || x > 31)
            return Result::ContradictsOneCandidate;
        for (int x : h.moves) if (x < 0 || x > h.maximumMove)
            return Result::ContradictsOneCandidate;
    }
    // A duplicate route or holey/ambiguous witness cannot certify an exhausted history.
    if (!h.versionKnown || !h.nativeFieldsKnown || !h.stageKnown ||
        !h.originalSourceKnown || !h.methodsExhaustive || !h.sameSource ||
        h.duplicateRoute || h.missingStage) return Result::Unknown;
    return h.allegedContradiction
        ? Result::ContradictsOneCandidate : Result::Compatible;
}
constexpr Witness full() {
    Witness w{};
    w.versionKnown=w.nativeFieldsKnown=w.stageKnown=w.methodsExhaustive=
        w.originalSourceKnown=w.sameSource=true;
    w.ivs={{0,1,2,3,30,31}};
    w.moves={{1,2,3,4}};
    return w;
}
constexpr bool tests() {
    auto x=full();
    if (classify(x)!=Result::Compatible) return false;
    x.ivs[0]=32;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x.ivs[0]=-1;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x=full(); x.moves[2]=468;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x=full(); x.moves[2]=-1;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x=full(); x.ivs[0]=31;
    if (classify(x)!=Result::Compatible) return false;
    x=full(); x.moves[0]=467;
    if (classify(x)!=Result::Compatible) return false;
    x=full(); x.versionKnown=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.nativeFieldsKnown=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.stageKnown=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.methodsExhaustive=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.originalSourceKnown=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.sameSource=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.duplicateRoute=true;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.missingStage=true;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.allegedContradiction=true;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x.methodsExhaustive=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.allegedContradiction=true; x.stageKnown=false;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.allegedContradiction=true; x.duplicateRoute=true;
    if (classify(x)!=Result::Unknown) return false;
    x=full(); x.maximumMove=354; x.moves[0]=355;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x=full(); x.maximumMove=165; x.moves[0]=165;
    if (classify(x)!=Result::Compatible) return false;
    x=full(); x.maximumMove=165; x.moves[0]=166;
    if (classify(x)!=Result::ContradictsOneCandidate) return false;
    x=full(); x.moves={{0,0,0,0}};
    if (classify(x)!=Result::Compatible) return false;
    x=full(); x.moves={{1,1,2,3}};
    if (classify(x)!=Result::Compatible) return false; // this model does NOT test move-slot uniqueness
    return true;
}
static_assert(tests(), "24 source-scope triage assertions must hold");
int main() {
    uint32_t seed=0x42d1c037u;
    auto next=[&]() -> uint32_t {
        seed=seed*1664525u+1013904223u;
        return seed;
    };
    constexpr int samples=65536;
    int p=0, n=0, u=0;
    for (int i=0;i<samples;i++) {
        auto x=full();
        for (int& v:x.ivs) v=static_cast<int>(next()%32u);
        for (int& v:x.moves) v=static_cast<int>(next()%468u);
        x.duplicateRoute=(next()&7u)==0;
        x.missingStage=(next()&15u)==0;
        x.allegedContradiction=(next()&1u)!=0;
        if (classify(x)==Result::Compatible) p++;
        else if (classify(x)==Result::ContradictsOneCandidate) n++;
        else u++;
        auto missing=x;missing.stageKnown=false;
        assert(classify(missing)==Result::Unknown);
        if (x.duplicateRoute || x.missingStage)
            assert(classify(x)==Result::Unknown);
    }
    assert(p+n+u==samples && p>0 && n>0 && u>0);
    std::cout<<"L5 Q16: 24 scoped assertions plus "<<samples
             <<" deterministic candidate trials PASS ("<<p<<"/"<<n<<"/"<<u
             <<" compatible/contradicted-one/unknown)\n";
}
