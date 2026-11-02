#pragma once
#include "Legality/Legality.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include <sstream>
#include <cassert>
namespace GBFixture {
struct Result {std::string id,kind;std::vector<uint8_t> body;Legality::Report report;};
inline uint16_t be(const std::vector<uint8_t>& b,size_t p){return (b[p]<<8)|b[p+1];}
inline std::vector<uint8_t> hex(const std::string& s){assert(s.size()%2==0);std::vector<uint8_t>b;for(size_t i=0;i<s.size();i+=2)b.push_back(std::stoul(s.substr(i,2),nullptr,16));return b;}
inline Result analyze(const std::string& line) {
    Result result;std::istringstream in(line);int lang;std::string body,ot,nick;in>>result.id>>result.kind>>lang>>body>>ot>>nick;
    result.body=hex(body);const auto o=hex(ot),n=hex(nick);const auto& b=result.body;
    if(b.size()==48) {
        namespace G=PokeVault::Integration::Gen2;int egg;in>>egg;assert(in);
        G::PokemonRecord rec;rec.species=b[0];rec.heldItem=b[1];rec.trainerId=be(b,6);rec.experience=(b[8]<<16)|(b[9]<<8)|b[10];
        for(size_t i=0;i<4;++i){rec.moves[i]=b[2+i];rec.pp[i]=b[23+i]&63;rec.ppUps[i]=b[23+i]>>6;}
        for(size_t i=0;i<5;++i)rec.statExperience[i]=be(b,11+2*i);
        const auto attack=b[21]>>4,defense=b[21]&15,speed=b[22]>>4,special=b[22]&15;
        rec.dvs={static_cast<uint8_t>(((attack&1)<<3)|((defense&1)<<2)|((speed&1)<<1)|(special&1)),static_cast<uint8_t>(attack),static_cast<uint8_t>(defense),static_cast<uint8_t>(speed),static_cast<uint8_t>(special)};
        rec.friendship=b[27];rec.pokerus=b[28];rec.caughtData=be(b,29);rec.level=b[31];rec.isEgg=egg;
        rec.nameEncoding=lang==1?G::RegionLayout::Japanese:lang==8?G::RegionLayout::Korean:G::RegionLayout::International;
        rec.originalTrainerRaw=o;rec.nicknameRaw=n;rec.originalTrainer=G::decodeGen2String(o,*rec.nameEncoding);rec.nickname=G::decodeGen2String(n,*rec.nameEncoding);
        rec.partyRecord=true;rec.rawBodySize=b.size();std::copy(b.begin(),b.end(),rec.rawBody.begin());
        Pokemon::Pokemon2ReadOnly pk(rec);result.report=Legality::analyze(pk,pk.getGameGroup());
    } else {
        assert(b.size()==44);namespace G=PokeVault::Integration::Gen1;G::PokemonRecord rec;
        rec.rawSpecies=b[0];rec.species=G::gen1InternalToNational(b[0]);rec.currentHP=be(b,1);rec.status=b[4];rec.type1=b[5];rec.type2=b[6];rec.catchRate=b[7];
        for(size_t i=0;i<4;++i){rec.moves[i]=b[8+i];rec.pp[i]=b[29+i]&63;rec.ppUps[i]=b[29+i]>>6;}
        rec.trainerId=be(b,12);rec.experience=(b[14]<<16)|(b[15]<<8)|b[16];for(size_t i=0;i<5;++i)rec.statExperience[i]=be(b,17+2*i);
        const auto a=b[27]>>4,d=b[27]&15,s=b[28]>>4,c=b[28]&15;rec.dvs={static_cast<uint8_t>(((a&1)<<3)|((d&1)<<2)|((s&1)<<1)|(c&1)),static_cast<uint8_t>(a),static_cast<uint8_t>(d),static_cast<uint8_t>(s),static_cast<uint8_t>(c)};
        rec.level=b[33];rec.originalTrainerRaw=o;rec.nicknameRaw=n;rec.originalTrainer=G::decodeGen1String(o,G::RegionLayout::Japanese);rec.nickname=G::decodeGen1String(n,G::RegionLayout::Japanese);
        rec.partyRecord=true;rec.rawBodySize=b.size();std::copy(b.begin(),b.end(),rec.rawBody.begin());
        Pokemon::Pokemon1ReadOnly pk(rec);result.report=Legality::analyze(pk,pk.getGameGroup());
    }
    return result;
}
}
