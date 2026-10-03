#include "Legality/Gen2TradeEvidence.h"

#include <array>
#include <cassert>
#include <iostream>

int main(){
    using namespace Legality::Gen2Trade;
    assert(count()==11);

    // Onix: fixed TID 48926 and fixed Atk/Def/Spe/Special DVs 9/6/6/6.
    const std::array<uint8_t,5> onixDvs{8,9,6,6,6};
    assert(matches("gold_gbc",95,3,48926,onixDvs,0));
    assert(!matches("gold_gbc",95,3,48925,onixDvs,0));
    auto wrong=onixDvs; wrong[1]=8;
    assert(!matches("gold_gbc",95,3,48926,wrong,0));

    // Crystal caught-data for NPC trade: location 126 and met level 0.
    assert(matches("crystal_gbc",95,3,48926,onixDvs,126));
    assert(!matches("crystal_gbc",95,3,48926,onixDvs,125));

    // Spearow/Shuckle templates use random DVs/TID, so species/level + caught data remain evidence.
    const std::array<uint8_t,5> randomDvs{0,1,2,3,4};
    assert(matches("silver_gbc",21,10,55555,randomDvs,0));
    assert(matches("crystal_gbc",213,15,12345,randomDvs,126));

    assert(hasSpecies(95));
    assert(hasSpecies(213));
    assert(!hasSpecies(250));

    std::cout<<"Gen II fixed in-game trade evidence: PASS\n";
}
