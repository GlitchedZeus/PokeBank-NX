#include "Legacy/RetroArchGSCDiscovery.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen2;
using namespace PokeVault::Legacy;
namespace fs = std::filesystem;

namespace {
struct L{SourceGame game;size_t trainer,money,boxIndex,boxNames,party,checksumEnd,checksum,checksum2,gender;};
constexpr L GS{SourceGame::Gold,0x2009,0x23DB,0x2724,0x2727,0x288A,0x2D68,0x2D69,0x7E6D,0};
constexpr L C {SourceGame::Crystal,0x2009,0x23DC,0x2700,0x2703,0x2865,0x2B82,0x2D0D,0x1F0D,0x3E3D};
void be16(std::vector<uint8_t>&b,size_t o,uint16_t v){b[o]=v>>8;b[o+1]=v&0xFF;}void be24(std::vector<uint8_t>&b,size_t o,uint32_t v){b[o]=v>>16;b[o+1]=(v>>8)&0xFF;b[o+2]=v&0xFF;}
void text(std::vector<uint8_t>&b,size_t o,const std::string&s,size_t w){size_t i=0;for(;i<s.size()&&i<w;i++){char c=s[i];b[o+i]=(c>='A'&&c<='Z')?uint8_t(0x80+c-'A'):uint8_t(0xA0+c-'a');}if(i<w)b[o+i]=0x50;}
void emptyList(std::vector<uint8_t>&b,size_t o,size_t cap){b[o]=0;for(size_t i=0;i<cap;i++)b[o+1+i]=0xFF;b[o+1+cap]=0xFF;}
void final(std::vector<uint8_t>&b,const L&l){uint16_t s=0;for(size_t i=0x2009;i<=l.checksumEnd;i++)s=uint16_t(s+b[i]);for(size_t o:{l.checksum,l.checksum2}){b[o]=s&0xFF;b[o+1]=s>>8;}}
std::vector<uint8_t> fixture(const L&l,const std::string&name){std::vector<uint8_t>b(0x8000);be16(b,l.trainer,2222);text(b,l.trainer+2,name,7);be24(b,l.money,12345);b[l.boxIndex]=0;if(l.gender)b[l.gender]=1;emptyList(b,l.party,6);for(size_t box=0;box<14;box++){size_t base=box<7?0x4000:0x6000;size_t idx=box<7?box:box-7;emptyList(b,base+idx*0x450,20);text(b,l.boxNames+box*9,"BOX",9);}const size_t item=l.game==SourceGame::Crystal?0x2420:0x241F,key=l.game==SourceGame::Crystal?0x244A:0x2449,ball=l.game==SourceGame::Crystal?0x2465:0x2464,pc=l.game==SourceGame::Crystal?0x247F:0x247E;b[item]=b[key]=b[ball]=b[pc]=0;b[item+1]=b[key+1]=b[ball+1]=b[pc+1]=0xFF;final(b,l);return b;}
void write(const fs::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));assert(f.good());}
const GSCSource* findByLeaf(const GSCDiscoveryResult&r,const std::string&leaf){for(const auto&s:r.sources)if(fs::path(s.path).filename()==leaf)return &s;return nullptr;}
}

int main(){
 const fs::path root=fs::temp_directory_path()/"pokebank_gsc_discovery_fixture";fs::remove_all(root);fs::create_directories(root/"sub");
 write(root/"Pokemon Gold.srm",fixture(GS,"GOLD"));write(root/"Pokemon Silver.sav",fixture(GS,"SILVER"));write(root/"mystery.sav",fixture(GS,"MYSTERY"));write(root/"plaincrystalstructure.srm",fixture(C,"KRIS"));write(root/"Pokemon Crystal wrong.sav",fixture(GS,"WRONG"));
 auto bad=fixture(GS,"BAD");bad[0x2100]^=1;write(root/"Pokemon Gold bad.sav",bad);write(root/"ignored.state",fixture(GS,"STATE"));write(root/"sub"/"Pokemon Gold nested.srm",fixture(GS,"NEST"));
 std::vector<std::string> roots{root.string()};auto d=discoverGSCSaves(roots);assert(!d.limitReached);assert(d.activeRoot==root.string());
 const auto* gold=findByLeaf(d,"Pokemon Gold.srm");assert(gold&&gold->ready()&&gold->gameId=="gold_gbc"&&gold->save->trainer().name=="GOLD");
 const auto* silver=findByLeaf(d,"Pokemon Silver.sav");assert(silver&&silver->ready()&&silver->gameId=="silver_gbc");
 const auto* unknown=findByLeaf(d,"mystery.sav");assert(unknown&&unknown->status==GSCSourceStatus::AmbiguousIdentity&&!unknown->save);
 const auto* crystal=findByLeaf(d,"plaincrystalstructure.srm");assert(crystal&&crystal->ready()&&crystal->gameId=="crystal_gbc");
 const auto* mismatch=findByLeaf(d,"Pokemon Crystal wrong.sav");assert(mismatch&&mismatch->status==GSCSourceStatus::AmbiguousIdentity);
 const auto* corrupt=findByLeaf(d,"Pokemon Gold bad.sav");assert(corrupt&&corrupt->status==GSCSourceStatus::InvalidSave);
 assert(findByLeaf(d,"ignored.state")==nullptr);assert(findByLeaf(d,"Pokemon Gold nested.srm")!=nullptr);
 const auto oldFp=gold->contentFingerprint;auto changed=fixture(GS,"NEW");write(root/"Pokemon Gold.srm",changed);auto refreshed=discoverGSCSaves(roots);const auto* newer=findByLeaf(refreshed,"Pokemon Gold.srm");assert(newer&&newer->ready()&&newer->save->trainer().name=="NEW"&&newer->contentFingerprint!=oldFp);
 const fs::path cfg=root/"retroarch.cfg";{std::ofstream f(cfg);f<<"savefile_directory = \""<<(root/"sub").string()<<"\"\n";}auto configRoots=retroArchGSCSaveRootsFromConfig(cfg.string());assert(configRoots.size()==1&&configRoots[0]==(root/"sub").string());
 auto limited=discoverGSCSaves(roots,GSCScanLimits{2,1});assert(limited.limitReached||limited.filesExamined==1);
 fs::remove_all(root);std::cout<<"GSC bounded RetroArch discovery and refresh tests passed\n";
}
