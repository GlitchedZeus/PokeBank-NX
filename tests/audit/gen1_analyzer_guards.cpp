// Independent application-level check: the standalone differential bypasses
// these existing native PK1 guards. Assertions remain active under -DNDEBUG.
#include "Legality/Legality.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Experience.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include <array>
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

// Host-only name lookup links, identical to the existing native PK1 suite.
// All legality, record wrapping and verdict logic below is production code.
namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

int main() {
    using PokeVault::Integration::Gen1::PokemonRecord;
    PokemonRecord base{};
    base.rawSpecies = 153; base.species = 1; base.level = 50;
    base.type1 = 22; base.type2 = 3; base.catchRate = 45;
    base.moves = {33, 45, 15, 36}; base.trainerId = 12345;
    base.experience = 117360;
    base.originalTrainer = "GOLD"; base.nickname = "MON";
    base.dvs = {15, 5, 5, 5, 5}; base.partyRecord = true;
    unsigned count = 0, failures = 0;
    auto check = [&](const PokemonRecord& record, bool invalid) {
        for (const char* label : {"", "red_gb"}) {
            const Pokemon::Pokemon1ReadOnly pk(record);
            const auto report = Legality::analyze(pk, pk.getGameGroup(), label);
            const auto expected = invalid ? Legality::Verdict::Invalid : Legality::Verdict::NoProblemsFound;
            if (report.verdict() != expected) {
                std::cerr << "Native PK1 guard FAIL: level=" << unsigned(record.level)
                          << " EXP=" << record.experience << " source=" << label
                          << " expected=" << unsigned(expected)
                          << " observed=" << unsigned(report.verdict()) << '\n';
                ++failures;
            }
            ++count;
        }
    };
    check(base, false);
    for (int i = 0; i < 4; ++i) for (int j = i+1; j < 4; ++j) {
        auto rec = base; rec.moves[j] = rec.moves[i];
        check(rec, true);
    }
    for (auto [level, exp] : std::array<std::pair<uint8_t, uint32_t>, 7>{
             {{5, 0}, {50, 0}, {50, 109923}, {50, 125126},
              {99, 1059860}, {100, 1059859}, {100, 1059861}}}) {
        auto rec = base; rec.level = level; rec.experience = exp;
        check(rec, true);
    }
    // Serialize a minimal international Red save and use the actual save
    // parser before the wrapper/analyzer. No hand-built trusted-record bypass.
    for (uint32_t xp : {1059860u, 1059861u}) {
        std::vector<uint8_t> bytes(0x8000, 0);
        auto be16 = [&](size_t offset, unsigned v) {
            bytes[offset] = static_cast<uint8_t>(v >> 8);
            bytes[offset+1] = static_cast<uint8_t>(v);
        };
        bytes[0x2598]=0x80; bytes[0x2599]=0x50;
        bytes[0x25f3]=0x12; bytes[0x25f4]=0x34; bytes[0x25f5]=0x56;
        be16(0x2605,12345); bytes[0x29c3]=153;
        bytes[0x30c0]=0; bytes[0x30c1]=0xff;
        bytes[0x2f2c]=1; bytes[0x2f2d]=153; bytes[0x2f2e]=0xff;
        const size_t body=0x2f2c+8;
        bytes[body]=153; be16(body+1,230); bytes[body+3]=100;
        bytes[body+5]=22; bytes[body+6]=3; bytes[body+7]=45;
        for(size_t i=0;i<4;++i) bytes[body+8+i]=base.moves[i];
        be16(body+12,12345);
        bytes[body+14]=static_cast<uint8_t>(xp>>16);
        bytes[body+15]=static_cast<uint8_t>(xp>>8);
        bytes[body+16]=static_cast<uint8_t>(xp);
        be16(body+27,0x5555); bytes[body+33]=100;
        const unsigned stats[5]={230,113,113,105,145};
        for(size_t i=0;i<5;++i) be16(body+34+i*2,stats[i]);
        const size_t ot=body+6*44, nickname=ot+6*11;
        bytes[ot]=0x86; bytes[ot+1]=0x8e; bytes[ot+2]=0x8b;
        bytes[ot+3]=0x83; bytes[ot+4]=0x50;
        bytes[nickname]=0x8c; bytes[nickname+1]=0x8e;
        bytes[nickname+2]=0x8d; bytes[nickname+3]=0x50;
        uint8_t checksum=0xff;
        for(size_t i=0x2598;i<0x3523;++i) checksum=static_cast<uint8_t>(checksum-bytes[i]);
        bytes[0x3523]=checksum;
        const auto original=bytes;
        namespace G1=PokeVault::Integration::Gen1;
        const auto parsed=G1::parse(bytes,G1::SourceGame::Red);
        if(!parsed || !parsed.save || parsed.save->party().size()!=1)
            return std::cerr<<"serialized Red fixture failed native parsing\n",2;
        const Pokemon::Pokemon1ReadOnly pk(parsed.save->party().front());
        const auto report=Legality::analyze(pk,pk.getGameGroup(),"red_gb");
        const auto expected=xp==1059860 ? Legality::Verdict::NoProblemsFound : Legality::Verdict::Invalid;
        if(report.verdict()!=expected) {
            std::cerr<<"Parsed Red save FAIL: EXP="<<xp<<" expected="<<unsigned(expected)
                     <<" observed="<<unsigned(report.verdict())<<'\n';
            ++failures;
        }
        if(bytes!=original || !std::equal(bytes.begin(),bytes.end(),parsed.save->sourceBytes().begin()))
            return std::cerr<<"native source bytes were mutated\n",2;
        ++count;
    }
    std::cout << "Native PK1 application guards: " << count << " checks, "
              << failures << " FAIL\n";
    return failures ? 1 : 0;
}
