#include "Legality/Gen3BerryContext.h"
#include "Legality/Gen3Legality.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
namespace Trainer {
const char* getSpeciesName(uint16_t id){return Names::getSpeciesName(id);}
const char* getItemName(uint16_t id){return Names::getItemName(id);}
}
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
namespace D=PokeVault::Integration::Gen3::Detail;
void w16(std::vector<uint8_t>&b,size_t p,unsigned v){b[p]=v;b[p+1]=v>>8;}
void w32(std::vector<uint8_t>&b,size_t p,uint32_t v){w16(b,p,v);w16(b,p+2,v>>16);}
std::vector<uint8_t> source(bool berry=true,bool rs=true){
 std::vector<uint8_t>b(0x20000,0xFF);
 for(size_t id=0;id<14;++id){size_t p=id*0x1000;std::fill_n(b.begin()+p,0x1000,0);w16(b,p+0xFF4,id);w32(b,p+0xFF8,0x08012025);w32(b,p+0xFFC,3);}
 if(!rs)w32(b,0xAC,0x12345678);
 const size_t offset=rs?0x3160:0x31F8, size=rs?0x530:0x34;
 const auto at=[](size_t p){return (1+p/0xF80)*0x1000+p%0xF80;};
 if(berry){const std::string n="PUMKIN";for(size_t i=0;i<n.size();++i)b[at(offset+i)]=0xBB+n[i]-'A';b[at(offset+6)]=0xFF;b[at(offset+0xA)]=2;b[at(offset+0x14)]=3;
  unsigned sum=0;for(size_t i=0;i<size-4;++i)if(!rs||i<0xC||i>=0x14)sum+=b[at(offset+i)];
  for(size_t i=0;i<4;++i)b[at(offset+size-4+i)]=sum>>(i*8);
 }
 for(size_t id=0;id<14;++id)w16(b,id*0x1000+0xFF6,D::sectorChecksum(std::span<const uint8_t>(b).subspan(id*0x1000,D::kChunkLengths[id])));
 return b;
}
int main(){
 using C=Legality::Gen3BerryContext;size_t checks=0;
 for(bool rs:{true,false}){
  auto b=source(true,rs);const auto original=b;const auto game=rs?"ruby_gba":"emerald_gba";
  auto c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::Recognized);++checks;
  assert(c->match("leafgreen_gba")==C::Match::Unknown);
  auto unknown=C::fromSave(b,game,0);assert(unknown&&unknown->match(game)==C::Match::Unknown);
  assert(!C::fromSave(b,"diamond_nds",2));assert(!C::fromSave(b,"firered_gba",2));
  assert(b==original);
  b[0x1000]^=1;assert(!C::fromSave(b,game,2));b=source(false,rs);
  c=C::fromSave(b,game,2);assert(c&&c->match(game)==C::Match::NoData);
  b.resize(0x100);assert(!C::fromSave(b,game,2));
 }
 // Real production analyzer must distinguish missing versus supplied validated save evidence.
 // The record need not pass unrelated encounter checks to prove the item rule is reached.
 // Reuse existing PKHeX-synthetic b0 fixture from test_legality_gen3.cpp (not authentic history).
 const std::string hex="B902FCB1F8BBFDA6BCCFC6BCBBCDBBCFCCFF0202CED9E7E8D9E6FF00F7D6000001000000870000000078000021002D00000000002328000000000000000000000000000000200021FFE6FF0D00000000";
 std::vector<uint8_t> raw;for(size_t i=0;i<hex.size();i+=2)raw.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
 w16(raw,34,175);
 unsigned sum=0;for(size_t i=32;i<80;i+=2)sum+=raw[i]|(unsigned{raw[i+1]}<<8);w16(raw,28,sum);
 auto b=source();auto c=C::fromSave(b,"ruby_gba",2);assert(c);
 const auto missing=Legality::analyzePk3(raw,"ruby_gba");
 const auto supplied=Legality::analyzePk3(raw,"ruby_gba",&*c);
 const auto has=[](const Legality::Report&r){return std::any_of(r.issues.begin(),r.issues.end(),[](const auto&i){return i.text.find("Enigma Berry:")!=std::string::npos;});};
 assert(has(missing));assert(!has(supplied));
 assert(supplied.verdict()==Legality::Verdict::NoProblemsFound);
 const auto recordBefore=raw;
 // Native Pokemon3FRLG consumes encrypted cartridge bytes, while analyzePk3
 // above deliberately exercises the decoded-record entry point.
 std::byte* encrypted=Encryption::encryptArray3FRLG(std::as_bytes(std::span<const uint8_t>(raw)));
 const Pokemon::Pokemon3FRLG pk(std::span<const std::byte>(encrypted,raw.size()));
 delete[] encrypted;
 const auto production=Legality::analyze(pk,Legality::Context{Enums::GameVersion::FRLG,"ruby_gba",&*c});
 if(production.verdict()!=Legality::Verdict::NoProblemsFound)for(const auto&i:production.issues)std::cerr<<static_cast<int>(i.severity)<<": "<<i.text<<"\n";
 assert(production.verdict()==Legality::Verdict::NoProblemsFound);assert(raw==recordBefore);
 b=source(false);c=C::fromSave(b,"ruby_gba",2);assert(c);
 const auto absent=Legality::analyzePk3(raw,"ruby_gba",&*c);assert(absent.hasInvalid());assert(has(absent));
 assert(raw.size()==80);
 std::cout<<"Gen III source-local e-Reader berry context: RS/E source parse, checksum/identity/immutability/Unknown and actual analyzer item verdict PASS ("<<checks<<" families)\n";
}
