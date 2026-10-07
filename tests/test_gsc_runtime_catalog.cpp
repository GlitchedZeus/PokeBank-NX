#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/RetroArchFRLGDiscovery.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Legacy;
namespace fs = std::filesystem;

namespace {
struct L { std::size_t money,idx,names,party,current,end,sum; uint8_t cap,boxes,str,split; };
constexpr L IGS{0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,20,14,11,7};
constexpr L IC {0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,20,14,11,7};
std::size_t listLen(std::size_t cap,std::size_t body,std::size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
std::size_t stride(const L& l){return listLen(l.cap,32,l.str)+2;}
std::size_t boxStart(const L& l,std::size_t b){return b<l.split?0x4000+b*stride(l):0x6000+(b-l.split)*stride(l);}
void empty(std::vector<uint8_t>& d,std::size_t o){d[o]=0;d[o+1]=0xFF;}
void text(std::vector<uint8_t>& d,std::size_t o,std::size_t n,char c){std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(o),static_cast<std::ptrdiff_t>(n),0x50);d[o]=static_cast<uint8_t>(0x80+c-'A');}
void mon(std::vector<uint8_t>& d,std::size_t o,std::size_t cap,std::size_t body,std::size_t str,uint8_t species){d[o]=1;d[o+1]=species;d[o+2]=0xFF;auto b=o+1+(cap+1);d[b]=species;d[b+6]=0x12;d[b+7]=0x34;d[b+21]=0x7A;d[b+22]=0xAA;d[b+31]=15;auto ot=b+cap*body;auto nick=ot+cap*str;text(d,ot,str,'O');text(d,nick,str,'M');}
void sum(std::vector<uint8_t>& d,const L& l){uint16_t s=0;for(std::size_t i=0x2009;i<=l.end;++i)s=static_cast<uint16_t>(s+d[i]);d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);}
std::vector<uint8_t> fixture(const L& l,char trainer='A'){std::vector<uint8_t>d(0x8000);d[0x2009]=0x12;d[0x200A]=0x34;text(d,0x200B,l.str,trainer);d[l.money]=1;d[l.money+1]=0xE2;d[l.money+2]=0x40;d[l.idx]=0;for(std::size_t b=0;b<l.boxes;++b){empty(d,boxStart(l,b));text(d,l.names+b*9,9,static_cast<char>('A'+b%26));}mon(d,boxStart(l,0),l.cap,32,l.str,1);empty(d,l.current);mon(d,l.party,6,48,l.str,25);sum(d,l);return d;}
void write(const fs::path& p,const std::vector<uint8_t>& b){fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size()));assert(f.good());}
const FRLGSource* source(const FRLGDiscoveryResult& r,std::string_view id){auto it=std::find_if(r.sources.begin(),r.sources.end(),[&](const auto& s){return s.gameId==id&&s.ready();});return it==r.sources.end()?nullptr:&*it;}
const FRLGSourceCard* card(const std::vector<FRLGSourceCard>& c,std::string_view id){auto it=std::find_if(c.begin(),c.end(),[&](const auto& x){return x.gameId==id;});return it==c.end()?nullptr:&*it;}
}

int main(){
    const fs::path root=fs::temp_directory_path()/"pokebank-gsc-runtime-catalog";fs::remove_all(root);fs::create_directories(root);
    const fs::path saves=root/"saves";write(saves/"Pokemon Gold.srm",fixture(IGS));write(saves/"Pokemon Silver.srm",fixture(IGS));write(saves/"Pokemon Crystal.srm",fixture(IC));
    const fs::path cfg=root/"retroarch.cfg";{std::ofstream f(cfg);f<<"savefile_directory = \"saves\"\n";}

    auto result=discoverConfiguredRetroArchFRLGSaves({},cfg.string(),(root/"fallback").string());
    assert(result.activeRootKind==FRLGDiscoveryResult::RootKind::Configured);
    const auto* gold=source(result,"gold_gbc");const auto* silver=source(result,"silver_gbc");const auto* crystal=source(result,"crystal_gbc");
    assert(gold&&silver&&crystal);assert(gold->isGen2()&&!gold->isGen1()&&!gold->isGen3());
    assert(gold->gen2Save->metadata().sourceGameId=="gold_gbc");
    auto cards=buildFRLGSourceCards(result);const auto* goldCard=card(cards,"gold_gbc");const auto* silverCard=card(cards,"silver_gbc");const auto* crystalCard=card(cards,"crystal_gbc");
    assert(goldCard&&silverCard&&crystalCard);assert(goldCard->platformLabel=="GBC"&&goldCard->sourceLabel=="RETROARCH");assert(goldCard->instances.size()==1);
    assert(resolveFRLGSaveInstance(result,*goldCard,0)==gold);

    const std::string stableId=gold->sourceIdentity;const std::string oldFingerprint=gold->contentFingerprint;
    write(saves/"Pokemon Gold.srm",fixture(IGS,'B'));
    auto refreshed=discoverConfiguredRetroArchFRLGSaves({},cfg.string(),(root/"fallback").string());const auto* gold2=source(refreshed,"gold_gbc");
    assert(gold2&&gold2->sourceIdentity==stableId&&gold2->contentFingerprint!=oldFingerprint);assert(gold2->gen2Save->trainer().name=="B");
    auto refreshedCards=buildFRLGSourceCards(refreshed);const auto* refreshedGold=card(refreshedCards,"gold_gbc");assert(refreshedGold&&refreshedGold->instances.size()==1);assert(resolveFRLGSaveInstance(refreshed,*refreshedGold,0)==gold2);

    fs::remove_all(root);std::cout<<"Generation II shared runtime catalog + refresh: PASS\n";
}
