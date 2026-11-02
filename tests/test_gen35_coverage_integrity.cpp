// Missing rule coverage must survive loss of explanatory warning text.
// These exercise actual production analyzers before testing the shared report API.
#include "Legality/Gen3Legality.h"
#include "Legality/Gen4Legality.h"
#include "Legality/Gen5Legality.h"
#include "Legality/ReportPresentation.h"
#include "../src/Legality/Gen4LegalityInternal.h"
#include "legality_gen5_test_helpers.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
using namespace Legality;
using namespace Gen5TestHelpers;
namespace {
void withoutWarnings(Report r, CoverageLevel CoverageSummary::*area) {
    assert(r.verdict()==Verdict::Incomplete&&!r.hasInvalid());
    if(r.coverage.*area!=CoverageLevel::Partial) {
        for(const auto&i:r.issues)std::cerr<<i.text<<"\n";
        std::cerr<<"Unsupported rule lost its explicit Partial coverage\n";
        assert(false);
    }
    std::erase_if(r.issues,[](const Issue&i){return i.severity==Severity::Warning;});
    assert(r.problemCount()==0&&r.verdict()==Verdict::Incomplete);
    assert(Presentation::lines(r).front().text.starts_with("Incomplete / Unknown:"));
    r.issues.push_back({Severity::Invalid,"independently proven contradiction",CheckIdentifier::Structure});
    assert(r.verdict()==Verdict::Invalid);
    assert(Presentation::lines(r).front().text.starts_with("Invalid:"));
}
std::vector<uint8_t> fixture(const char* path,const std::string& wanted) {
    std::ifstream in(path);assert(in);std::string id,expected,hex;
    while(in>>id>>expected>>hex)if(id==wanted)return fromHex(hex.c_str());
    assert(false);return {};
}
void sum3(std::vector<uint8_t>&b){const auto c=Gen3Pk3::checksum(b);b[28]=c&255;b[29]=c>>8;}
}
int main(){
    // A native Ruby starter becomes unsupported only when its held Enigma Berry
    // needs source-save evidence. Structural validity alone cannot certify it.
    auto gba=fromHex("B902FCB1F8BBFDA6BCCFC6BCBBCDBBCFCCFF0202CED9E7E8D9E6FF00F7D6000001000000870000000078000021002D00000000002328000000000000000000000000000000200021FFE6FF0D00000000");
    const auto completeGba=analyzePk3(gba,"ruby_gba");
    assert(completeGba.verdict()==Verdict::NoProblemsFound);
    gba[34]=175;gba[35]=0;sum3(gba);const auto original=gba;
    const auto berry=analyzePk3(gba,"ruby_gba");
    withoutWarnings(berry,&CoverageSummary::internal);assert(gba==original);
    auto withoutText=berry;
    std::erase_if(withoutText.issues,[](const Issue&i){return i.severity==Severity::Warning;});
    Gen3Pk3::Verification verification;
    verification.ran=true;verification.report=withoutText;verification.certified=false;
    auto composed=completeGba;
    applyGen3Analyzer(composed,verification,"ruby_gba");
    assert(composed.problemCount()==0&&composed.verdict()==Verdict::Incomplete);
    assert(composed.coverage.internal==CoverageLevel::Partial);
    applyGen3Analyzer(composed,verification,"ruby_gba");
    assert(composed.verdict()==Verdict::Incomplete);
    // A stale external certification boolean cannot overrule the actual
    // report's unresolved coverage, even when its warning strings are absent.
    verification.certified=true;composed=completeGba;
    applyGen3Analyzer(composed,verification,"ruby_gba");
    assert(composed.verdict()==Verdict::Incomplete);
    gba[56]=255;gba[57]=255;gba[58]=1;sum3(gba);
    const auto contradiction=analyzePk3(gba,"ruby_gba");
    assert(contradiction.verdict()==Verdict::Invalid);
    const auto invalid=std::find_if(contradiction.issues.begin(),contradiction.issues.end(),[](const Issue&i){return i.severity==Severity::Invalid;});
    assert(invalid!=contradiction.issues.end());
    verification.certified=false;verification.report=contradiction;composed=completeGba;
    composed.issues.push_back({Severity::Warning,invalid->text,invalid->identifier});
    applyGen3Analyzer(composed,verification,"ruby_gba");
    assert(composed.verdict()==Verdict::Invalid); // same text must not mask severity

    Gen4::Origin5 origin;
    origin.analyzed=true;origin.encounterMatched=true;origin.allMovesVerified=true;
    origin.breedingCoverage=CoverageLevel::Partial;
    assert(origin.checks.empty()&&!origin.clean());
    origin.breedingCoverage=CoverageLevel::Complete;
    assert(origin.clean());

    // Ranch's actual ability/PID source is unavailable. Retained mismatch stays
    // partial even when a presentation layer drops its Warning issue string.
    const auto ranch=fixture("tests/fixtures/gen15_session2/ranch_ability.txt","h60141-ability");
    withoutWarnings(analyzePk4(ranch,"diamond_nds"),&CoverageSummary::pidRng);
    const auto ordinary=fixture("tests/fixtures/gen15_session2/ranch_ability.txt","ordinary-pid-slot-control");
    assert(analyzePk4(ordinary,"diamond_nds").verdict()==Verdict::NoProblemsFound);

    // The GenV hidden-flag/actual ability uncertainty must not be overwritten by
    // successful name checks at the end of analysis.
    auto scizor=fromHex("EDB39CC40000B7AFD40000004AFE1CB4E80300004665000200000000000000000000000000000000D3000000000000001900000000000000FEE4163B0000000000140100000000005300630069007A006F007200FFFF000000000000000000150000000000000000540065007300740065007200FFFF00000000001A0A0F00004B0000190A000000");
    withoutWarnings(analyzePk5(scizor,{.encrypted=false}),&CoverageSummary::internal);
    scizor[0x15]=135;refreshChecksum(scizor);
    assert(analyzePk5(scizor,{.encrypted=false}).verdict()==Verdict::NoProblemsFound);
    const auto basculin=fromHex("6D21B61F0000058826020000D6FB6724E803000046450002000000000000000000000000000000003700FD001D002C00190A0F19000000009E822215000000000A12000000000000420061007300630075006C0069006E00FFFF0000000000160000000000000000540065007300740065007200FFFF00000000001A0A1200004B0000040A000000");
    withoutWarnings(analyzePk5(basculin,{.encrypted=false}),&CoverageSummary::internal);
    std::ifstream events("tests/fixtures/gen15_session2/global_link.txt");assert(events);
    std::string id,expected,hex;size_t unsupported=0,receiptPartial=0,namePartial=0;
    while(events>>id>>expected>>hex)if(expected=="INCOMPLETE"){
        const auto bytes=fromHex(hex.c_str()),before=bytes;
        const auto report=analyzePk5(bytes,{.encrypted=false});
        const bool receiptUnknown=std::any_of(report.issues.begin(),report.issues.end(),[](const Issue&i){
            return i.severity==Severity::Warning && i.identifier==CheckIdentifier::EventGift;
        });
        // Some retained promotion records have a fully supported receipt and
        // only an unresolved OT-name rule. That does not invalidate the event
        // coverage; require the applicable uncertainty to survive instead.
        withoutWarnings(report,receiptUnknown?&CoverageSummary::eventGift:&CoverageSummary::internal);
        if(receiptUnknown)++receiptPartial;else ++namePartial;
        assert(bytes==before);++unsupported;
    }
    assert(unsupported==44&&receiptPartial==40&&namePartial==4);
    std::ifstream eggs("tests/fixtures/gen15_session2/gen5_bw_paired_family.txt");assert(eggs);size_t pairs=0;
    while(eggs>>id>>expected>>hex){
        withoutWarnings(analyzePk5(fromHex(hex.c_str()),{.encrypted=false}),&CoverageSummary::eggBreeding);++pairs;
    }
    assert(pairs==2);
    std::ifstream transfers("tests/fixtures/gen15_session2/transferred_joint_coverage.txt");assert(transfers);size_t transferCases=0;
    while(transfers>>id>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),before=bytes;
        auto report=analyzePk5(bytes,{.encrypted=false});
        if(expected=="NOPROBLEMS")assert(report.verdict()==Verdict::NoProblemsFound);
        else if(expected=="INCOMPLETE") {
            withoutWarnings(report,&CoverageSummary::eggBreeding);
            assert(report.coverage.moves==CoverageLevel::Partial&&report.coverage.transfer==CoverageLevel::Partial);
        } else {
            assert(report.verdict()==Verdict::Invalid);
            std::erase_if(report.issues,[](const Issue&i){return i.severity==Severity::Warning;});
            assert(report.verdict()==Verdict::Invalid);
        }
        assert(bytes==before);++transferCases;
    }
    assert(transferCases==5);
    // Actual retained XD Dragonite inputs hit the native RNG search guard.
    // A capped origin remains unknown, but cannot negate a PP/EV contradiction.
    std::ifstream capped("tests/fixtures/gen15_session2/gen3_rng_cap_priority.txt");assert(capped);
    size_t cappedCases=0;
    while(capped>>id>>expected>>hex) {
        const auto bytes=fromHex(hex.c_str()),before=bytes;
        for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba","firered_gba","leafgreen_gba"}) {
            const auto report=analyzePk3(bytes,game);
            assert(bytes==before&&hasText(report,Severity::Warning,"RNG search reached its limit"));
            assert(report.coverage.pidRng==CoverageLevel::Partial);
            if(expected=="INCOMPLETE") {
                withoutWarnings(report,&CoverageSummary::pidRng);
                auto ev=bytes;ev[56]=255;ev[57]=255;ev[58]=1;sum3(ev);
                const auto invalid=analyzePk3(ev,game);
                assert(invalid.verdict()==Verdict::Invalid);
                assert(hasText(invalid,Severity::Invalid,"EV total 511 is above 510"));
                assert(hasText(invalid,Severity::Warning,"RNG search reached its limit"));
            } else {
                assert(expected=="INVALID"&&report.verdict()==Verdict::Invalid);
                assert(hasText(report,Severity::Invalid,"Move 1 PP is above its maximum"));
                assert(!hasText(report,Severity::Invalid,"PID/IV correlation"));
                auto withoutText=report;
                std::erase_if(withoutText.issues,[](const Issue&i){return i.severity==Severity::Warning;});
                assert(withoutText.verdict()==Verdict::Invalid);
            }
        }
        ++cappedCases;
    }
    assert(cappedCases==2);
    std::cout<<"Actual GenIII-V coverage: source Berry, Ranch RNG, GenV ability,40 unsupported events,4 name-only promotion controls and2 BW paired histories remainIncomplete without warnings; 5 GenIII/IV→V transferred joint-history controls; hard contradictions outrank partial; clean controls PASS\n";
}
