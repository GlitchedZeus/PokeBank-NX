#include "Legality/Gen5GiftReceivability.h"
#include "Legality/Gen5Legality.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
int main() {
    size_t checks=0;
    // Receiving metadata remains paired by card. Versions and languages from
    // distinct distributions cannot be unioned into a fictional distribution.
    for(size_t i=0;i<Legality::Gen5Gift::kReceiving.size();++i) {
        auto metadata=Legality::Gen5Gift::kReceiving[i];
        for(uint8_t version=19;version<=24;++version)for(uint8_t lang=0;lang<=9;++lang) {
            const bool expected=version>=20&&version<=23&&(metadata&(1u<<(version-20)))&&lang==(metadata>>4);
            assert(Legality::Gen5Gift::permits(i,version,lang)==expected);++checks;
        }
    }
    assert(!Legality::Gen5Gift::permits(9999,20,2));
    std::ifstream input("tests/fixtures/gen15_session2/gen5_receipt.txt");assert(input);
    std::string expected,hex;size_t cases=0;
    while(input>>expected>>hex) {
        std::vector<uint8_t> raw;
        for(size_t i=0;i<hex.size();i+=2)raw.push_back(std::stoul(hex.substr(i,2),nullptr,16));
        const auto before=raw;
        const auto result=Legality::analyzePk5(raw,{.encrypted=false});
        const bool incomplete=expected=="INCOMPLETE";
        bool receipt=false;
        for(const auto& issue:result.issues)receipt|=issue.text.find("PGF receiving language/game")!=std::string::npos;
        assert(receipt==incomplete);
        assert(result.verdict()==(incomplete?Legality::Verdict::Incomplete:Legality::Verdict::NoProblemsFound));
        assert(raw==before);++cases;
    }
    assert(cases==6);
    std::cout<<"Gen V receiving metadata: "<<checks<<" version/language boundaries; unsupported receipt stays incomplete, fully supported alternate remains certified PASS\n";
}
