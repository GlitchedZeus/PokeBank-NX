// Modern filter additions do not prove a contradiction in original games.
#include "Legality/Gen5Legality.h"
#include "Legality/Gen5Text.h"
#include "legality_gen5_test_helpers.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace Gen5TestHelpers;
int main() {
    using namespace Legality;
    assert(Gen5Text::unsupportedPatterns()==0);
    assert(!Gen5Text::unverifiedLookbackPattern(u"INCEL",5).empty());
    assert(!Gen5Text::unverifiedLookbackPattern(u"incel",5).empty());
    assert(Gen5Text::unverifiedLookbackPattern(u"Bulbasaur",5).empty());
    assert(Gen5Text::unverifiedLookbackPattern(u"Tester",5).empty());
    std::ifstream in("tests/fixtures/gen15_session2/gen5_false_clean38.txt");assert(in);
    std::string line;size_t count=0;
    while(std::getline(in,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);std::string id,baseline,hex;row>>id>>baseline>>hex;
        auto bytes=fromHex(hex.c_str());const auto before=bytes;
        const auto report=analyzePk5(bytes,{.encrypted=false});
        if(report.verdict()!=Verdict::Incomplete){std::cerr<<id<<" must remain incomplete\n";assert(false);}
        assert(hasText(report,Severity::Warning,"newer lookback"));
        assert(report.coverage.internal==CoverageLevel::Partial);
        assert(report.coverage.structure==CoverageLevel::Complete);
        assert(!has(report,Severity::Invalid)&&bytes==before);
        // A source-independent hard contradiction still outranks missing evidence.
        bytes[8]=255;bytes[9]=255;
        assert(analyzePk5(bytes,{.encrypted=false}).verdict()==Verdict::Invalid);
        // Unrelated benign names do not inherit a blanket uncertainty flag.
        bytes=before;const size_t offset=id.ends_with("otname")?0x68:0x48;
        const size_t chars=offset==0x68?8:11;
        for(size_t j=0;j<chars;++j){bytes[offset+2*j]=255;bytes[offset+2*j+1]=255;}
        const std::u16string safe=u"Safe";
        for(size_t j=0;j<safe.size();++j){bytes[offset+2*j]=static_cast<uint8_t>(safe[j]);bytes[offset+2*j+1]=0;}
        // PK5 checksum is checked even on decrypted input.
        uint16_t sum=0;for(size_t j=8;j<136;j+=2)sum=static_cast<uint16_t>(sum+bytes[j]+(bytes[j+1]<<8));
        bytes[6]=sum&255;bytes[7]=sum>>8;
        const auto clean=analyzePk5(bytes,{.encrypted=false});
        assert(clean.coverage.internal==CoverageLevel::Complete);
        if(clean.verdict()!=Verdict::NoProblemsFound){std::cerr<<id<<" benign control failed\n";assert(false);}
        ++count;
    }
    assert(count==38);std::cout<<"GenV historical name coverage:38 uncertainty cases,38 benign controls,38 hard contradictions PASS\n";
}
