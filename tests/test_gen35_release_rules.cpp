// Rules and counterexamples from the official PKHeX26.10.18 release audit.
// All fixture provenance is explicit; none is an authenticated distribution receipt.
#include "Legality/Gen5Legality.h"
#include "Legality/Gen4Legality.h"
#include "Legality/Gen3Legality.h"
#include "Legality/Gen5GlobalLinkCatalog.h"
#include "Legality/Gen4RanchIV.h"
#include "legality_gen5_test_helpers.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
using namespace Gen5TestHelpers;
using Legality::Verdict;
int main() {
    size_t cases=0,unsupported=0,regional=0;
    std::ifstream input("tests/fixtures/gen15_session2/global_link.txt");assert(input);
    std::string id,expected,hex;
    while(input>>id>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),original=bytes;
        const auto report=Legality::analyzePk5(bytes,{.encrypted=false});
        assert(report.verdict()==(expected=="NOPROBLEMS"?Verdict::NoProblemsFound:Verdict::Incomplete));
        assert(bytes==original&&!has(report,Legality::Severity::Invalid));
        if(id.starts_with("korean-")) {
            // Korean8 is supported by the promotion. An unmodelled long OT
            // stays uncertain for its own reason, never a Spanish7 mix-up.
            assert(report.coverage.eventGift==Legality::CoverageLevel::Complete);
            assert(!hasText(report,Legality::Severity::Warning,"Global Link"));++regional;
        } else if(expected=="INCOMPLETE") {
            assert(report.coverage.eventGift==Legality::CoverageLevel::Partial);
            assert(hasText(report,Legality::Severity::Warning,"Global Link"));++unsupported;
        }
        ++cases;
    }
    assert(input.eof()&&unsupported==40&&regional==8);
    // Native atomic battle witness: Power Bracer gives ATK+4 while the same
    // low-level Hoothoot battle gives HP+1 for only1 EXP. The current reference
    // rejects this reachable joint result; do not import its per-stat sum.
    std::ifstream evs("tests/fixtures/gen15_session2/ev_exp.txt");assert(evs);
    while(evs>>id>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),original=bytes;
        const auto report=Legality::analyzePk4(bytes,"diamond_nds");
        assert(report.verdict()==(expected=="NOPROBLEMS"?Verdict::NoProblemsFound:Verdict::Invalid));
        assert(bytes==original);
        if(id=="no-exp")assert(hasText(report,Legality::Severity::Invalid,"gained no EXP"));
        ++cases;
    }
    assert(evs.eof()&&cases==56);
    std::ifstream ranch("tests/fixtures/gen15_session2/ranch_iv.txt");assert(ranch);
    int format=0;
    while(ranch>>id>>format>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),original=bytes;
        const auto report=format==4?Legality::analyzePk4(bytes,"diamond_nds")
            :Legality::analyzePk5(bytes,{.encrypted=false});
        assert(report.verdict()==(expected=="NOPROBLEMS"?Verdict::NoProblemsFound:Verdict::Incomplete));
        assert(bytes==original&&!has(report,Legality::Severity::Invalid));
        if(expected=="INCOMPLETE")assert(hasText(report,Legality::Severity::Warning,"Ranch IVs have no state"));
        ++cases;
    }
    assert(ranch.eof()&&cases==60);
    // Published reference-model constructions, not acquired event specimens.
    // Retain both correct and contradictory dual-OT genders, the old endpoints,
    // the newly accepted midnight seeds, and seed214 across PK3->PK4->PK5.
    std::ifstream berry("tests/fixtures/gen15_session2/berry_fix.txt");assert(berry);
    size_t berryClean=0,berryInvalid=0;
    while(berry>>id>>format>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),original=bytes;
        const auto report=format==3?Legality::analyzePk3(bytes,"sapphire_gba")
            :format==4?Legality::analyzePk4(bytes,"diamond_nds")
            :Legality::analyzePk5(bytes,{.encrypted=false});
        assert(report.verdict()==(expected=="NOPROBLEMS"?Verdict::NoProblemsFound:Verdict::Invalid));
        assert(bytes==original);
        if(expected=="NOPROBLEMS")++berryClean;else ++berryInvalid;
        ++cases;
    }
    assert(berry.eof()&&berryClean==30&&berryInvalid==42&&cases==132);
    // A known full RNG state must survive reversal for both sign-bit aliases.
    uint32_t seed=0x20261027;
    for(size_t i=0;i<256;++i) {
        seed=seed*1664525u+1013904223u;
        uint32_t state=seed*0x41C64E6Du+12345u;
        const auto first=(state>>16)&0x7FFFu;
        state=state*0x41C64E6Du+12345u;state=state*0x41C64E6Du+12345u;
        const auto third=(state>>16)&0x7FFFu;
        const std::array<uint8_t,6> ivs{static_cast<uint8_t>(third&31),static_cast<uint8_t>((third>>5)&31),
            static_cast<uint8_t>((third>>10)&31),static_cast<uint8_t>(first&31),
            static_cast<uint8_t>((first>>5)&31),static_cast<uint8_t>((first>>10)&31)};
        assert(Legality::Gen4RanchIV::hasState(ivs));
    }
    std::cout<<"Official release rules: "<<cases<<" cases; 40 unsupported PGL histories, supported regional controls, "
        "native joint EV/EXP counterexample, zero-EXP negative, Ranch PK4/PK5, Berry Fix PK3/PK4/PK5 and256 RNG states PASS\n";
}
