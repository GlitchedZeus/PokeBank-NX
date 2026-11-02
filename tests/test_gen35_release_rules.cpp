// Rules and counterexamples from the official PKHeX26.10.18 release audit.
// All fixture provenance is explicit; none is an authenticated distribution receipt.
#include "Legality/Gen5Legality.h"
#include "Legality/Gen4Legality.h"
#include "Legality/Gen3Legality.h"
#include "Legality/Gen5GlobalLinkCatalog.h"
#include "Legality/Gen4RanchIV.h"
#include "../src/Legality/Gen4LegalityInternal.h"
#include <sstream>
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
    std::ifstream context("tests/fixtures/gen15_session2/evolved_trade_hatch.txt");assert(context);
    size_t contextClean=0,contextInvalid=0;
    std::string game;
    while(context>>id>>format>>game>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),original=bytes;
        const auto report=format==4?Legality::analyzePk4(bytes,game)
            :Legality::analyzePk5(bytes,{.encrypted=false});
        assert(report.verdict()==(expected=="NOPROBLEMS"?Verdict::NoProblemsFound:Verdict::Invalid));
        assert(bytes==original);
        if(expected=="NOPROBLEMS")++contextClean;else ++contextInvalid;
        ++cases;
    }
    assert(context.eof()&&contextClean==4&&contextInvalid==4&&cases==140);
    std::ifstream eggPid("tests/fixtures/gen15_session2/gen3_egg_pid.txt");assert(eggPid);
    size_t eggPidClean=0,eggPidInvalid=0;
    while(eggPid>>id>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),original=bytes;
        const auto report=Legality::analyzePk3(bytes,"emerald_gba");
        assert(report.verdict()==(expected=="NOPROBLEMS"?Verdict::NoProblemsFound:Verdict::Invalid));
        assert(bytes==original);
        if(expected=="NOPROBLEMS")++eggPidClean;else {
            ++eggPidInvalid;
            assert(hasText(report,Legality::Severity::Invalid,"PID bit 15"));
        }
        ++cases;
    }
    assert(eggPid.eof()&&eggPidClean==3&&eggPidInvalid==1&&cases==144);
    // Production joint-budget negatives. Every individual excess is small,
    // but29 combined EVs above vitamin ceilings cannot fit two native actions.
    // Preserve the exact old positive as the source of these mutations.
    std::ifstream jointInput("tests/fixtures/gen15_session2/ev_exp.txt");
    jointInput>>id>>expected>>hex; // unchanged zero-EV control
    jointInput>>id>>expected>>hex; assert(jointInput && id=="correlated" && expected=="NOPROBLEMS");
    auto joint=fromHex(hex.c_str());
    auto setExp=[&](uint32_t value) { for(unsigned i=0;i<4;++i)joint[0x10+i]=value>>(8*i); };
    const auto originalExp=uint32_t(joint[0x10])|(uint32_t(joint[0x11])<<8)|(uint32_t(joint[0x12])<<16)|(uint32_t(joint[0x13])<<24);
    setExp(originalExp+1); // existing control has gained1 EXP; now2
    const std::array<uint8_t,6> target{114,114,101,0,0,0};
    std::copy(target.begin(),target.end(),joint.begin()+0x18);refreshChecksum(joint);
    const auto saved=joint;
    const auto impossible=Legality::analyzePk4(joint,"diamond_nds");
    assert(impossible.verdict()==Verdict::Invalid);
    assert(hasText(impossible,Legality::Severity::Invalid,"joint EV gains"));
    assert(joint==saved && consistent(impossible));
    // Search/catalog failure must keep an otherwise supported report uncertain.
    // This total is within the global necessary bound but the deliberately
    // limited positive catalog cannot certify its low-EXP multi-stat history.
    joint[0x18]=110;joint[0x19]=110;joint[0x1A]=101;refreshChecksum(joint);
    const auto unresolved=Legality::analyzePk4(joint,"diamond_nds");
    assert(unresolved.verdict()==Verdict::Incomplete && consistent(unresolved));
    assert(hasText(unresolved,Legality::Severity::Warning,"joint EV/EXP"));
    // Matching encounter and per-slot learnability cannot certify a missing
    // required origin history, even with no warning strings at all.
    Legality::Gen4::Origin5 origin;
    origin.analyzed=origin.encounterMatched=origin.allMovesVerified=true;
    assert(origin.checks.empty() && !origin.clean());
    origin.breedingCoverage=Legality::CoverageLevel::Complete;
    assert(origin.clean());
    origin.breedingCoverage=Legality::CoverageLevel::Partial;
    assert(!origin.clean());
    // Constructed hatched records retain a multi-step inherited move after
    // Move Deleter. Official release accepts all10; old native adapters could
    // not certify any of their complete parent chains.
    std::ifstream recursive("tests/fixtures/gen15_session2/native_recursive_breeding.txt");assert(recursive);
    size_t recursiveCases=0;std::string row,cart;
    while(std::getline(recursive,row)) {
        std::istringstream fields(row);fields>>id>>cart>>hex;assert(fields);
        const auto bytes=fromHex(hex.c_str()),before=bytes;
        const std::string game=cart=="R"?"ruby_gba":cart=="S"?"sapphire_gba":cart=="E"?"emerald_gba":cart=="FR"?"firered_gba":cart=="LG"?"leafgreen_gba":cart=="D"?"diamond_nds":cart=="P"?"pearl_nds":cart=="Pt"?"platinum_nds":cart=="HG"?"heartgold_nds":"soulsilver_nds";
        const auto report=bytes.size()==80?Legality::analyzePk3(bytes,game):Legality::analyzePk4(bytes,game);
        if(report.verdict()!=Verdict::NoProblemsFound) {std::cerr<<id<<" lacks full parent proof\n";for(const auto& issue:report.issues)std::cerr<<issue.text<<"\n";assert(false);}
        assert(consistent(report)&&bytes==before);++recursiveCases;
    }
    assert(recursiveCases==10);
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
    std::cout<<"Official release rules: "<<cases<<" original fixtures +2 joint EV checks +10 native recursive controls; 40 unsupported PGL histories, supported regional controls, "
        "native joint EV/EXP counterexample, zero-EXP negative, Ranch PK4/PK5, Berry Fix PK3/PK4/PK5 and256 RNG states PASS\n";
}
