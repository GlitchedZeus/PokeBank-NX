#include "Legacy/GSCSourceBrowser.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen2;
using namespace PokeVault::Legacy;
namespace fs = std::filesystem;
namespace {
struct L{size_t trainer,money,boxIndex,boxNames,party,end,cs1,cs2,gender;bool crystal;};constexpr L GS{0x2009,0x23DB,0x2724,0x2727,0x288A,0x2D68,0x2D69,0x7E6D,0,false};constexpr L C{0x2009,0x23DC,0x2700,0x2703,0x2865,0x2B82,0x2D0D,0x1F0D,0x3E3D,true};
void be16(std::vector<uint8_t>&b,size_t o,uint16_t v){b[o]=v>>8;b[o+1]=v&255;}void be24(std::vector<uint8_t>&b,size_t o,uint32_t v){b[o]=v>>16;b[o+1]=(v>>8)&255;b[o+2]=v&255;}void txt(std::vector<uint8_t>&b,size_t o,const char*s,size_t n){size_t i=0;for(;s[i]&&i<n;i++)b[o+i]=uint8_t(0x80+s[i]-'A');if(i<n)b[o+i]=0x50;}void list(std::vector<uint8_t>&b,size_t o,int n){b[o]=0;for(int i=0;i<n;i++)b[o+1+i]=0xFF;b[o+1+n]=0xFF;}void sum(std::vector<uint8_t>&b,const L&l){uint16_t s=0;for(size_t i=0x2009;i<=l.end;i++)s=uint16_t(s+b[i]);for(auto o:{l.cs1,l.cs2}){b[o]=s&255;b[o+1]=s>>8;}}
std::vector<uint8_t> fixture(const L&l,const char*name){std::vector<uint8_t>b(0x8000);be16(b,l.trainer,7);txt(b,l.trainer+2,name,7);be24(b,l.money,999);b[l.boxIndex]=0;if(l.gender)b[l.gender]=1;list(b,l.party,6);for(int x=0;x<14;x++){size_t base=x<7?0x4000:0x6000;list(b,base+(x%7)*0x450,20);txt(b,l.boxNames+x*9,"BOX",9);}size_t item=l.crystal?0x2420:0x241F,key=l.crystal?0x244A:0x2449,ball=l.crystal?0x2465:0x2464,pc=l.crystal?0x247F:0x247E;b[item]=b[key]=b[ball]=b[pc]=0;b[item+1]=b[key+1]=b[ball+1]=b[pc+1]=0xFF;sum(b,l);return b;}void write(const fs::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());}
}
int main(){const fs::path root=fs::temp_directory_path()/"pokebank_gsc_browser_fixture";fs::remove_all(root);fs::create_directories(root);write(root/"Pokemon Gold.sav",fixture(GS,"GOLD"));write(root/"Pokemon Silver.sav",fixture(GS,"SILVER"));write(root/"unknown.srm",fixture(C,"KRIS"));std::vector<std::string> roots{root.string()};auto d=discoverGSCSaves(roots);auto cards=buildGSCSourceCards(d);assert(cards.size()==3);for(const auto&card:cards){assert(card.platform=="Game Boy Color");assert(card.provider=="RETROARCH");assert(card.instances.size()==1);assert(!card.instances[0].sourceIdentity.empty());const auto* s=resolveGSCSaveInstance(d,card,0);assert(s&&s->ready()&&s->gameId==card.gameId);}auto cards2=buildGSCSourceCards(d);assert(cards2.size()==cards.size());for(size_t i=0;i<cards.size();i++)assert(cards[i].instances[0].sourceIdentity==cards2[i].instances[0].sourceIdentity);fs::remove_all(root);std::cout<<"GSC source-browser tests passed\n";}
