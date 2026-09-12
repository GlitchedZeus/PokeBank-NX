#include "Legacy/RetroArchGSCDiscovery.h"

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
struct L {
    std::size_t money, idx, names, party, current, end, sum;
    uint8_t cap, boxes, str, split;
};
constexpr L IGS{0x23DB,0x2724,0x2727,0x288A,0x2D6C,0x2D68,0x2D69,20,14,11,7};
constexpr L IC {0x23DC,0x2700,0x2703,0x2865,0x2D10,0x2B82,0x2D0D,20,14,11,7};

std::size_t listLen(std::size_t cap,std::size_t body,std::size_t str){return 1+(cap+1)+cap*body+2*cap*str;}
std::size_t stride(const L& l){return listLen(l.cap,32,l.str)+2;}
std::size_t boxStart(const L& l,std::size_t b){return b<l.split?0x4000+b*stride(l):0x6000+(b-l.split)*stride(l);}
void emptyList(std::vector<uint8_t>& d,std::size_t ofs){d[ofs]=0;d[ofs+1]=0xFF;}
void name(std::vector<uint8_t>& d,std::size_t ofs,std::size_t len,char c){
    std::fill_n(d.begin()+static_cast<std::ptrdiff_t>(ofs),static_cast<std::ptrdiff_t>(len),0x50);
    if(c>='A'&&c<='Z')d[ofs]=static_cast<uint8_t>(0x80+c-'A');
}
void pokemonList(std::vector<uint8_t>& d,std::size_t ofs,std::size_t cap,std::size_t bodySize,std::size_t str){
    constexpr uint8_t species=25;
    d[ofs]=1;d[ofs+1]=species;d[ofs+2]=0xFF;
    const std::size_t body=ofs+1+(cap+1);
    d[body]=species; d[body+6]=0x12; d[body+7]=0x34; d[body+21]=0x7A; d[body+22]=0xAA; d[body+31]=15;
    const std::size_t ot=body+cap*bodySize; const std::size_t nick=ot+cap*str;
    name(d,ot,str,'O');name(d,nick,str,'M');
}
void checksum(std::vector<uint8_t>& d,const L& l){
    uint16_t s=0;for(std::size_t i=0x2009;i<=l.end;i++)s=static_cast<uint16_t>(s+d[i]);
    d[l.sum]=static_cast<uint8_t>(s);d[l.sum+1]=static_cast<uint8_t>(s>>8);
}
std::vector<uint8_t> fixture(const L& l){
    std::vector<uint8_t>d(0x8000);
    d[0x2009]=0x12;d[0x200A]=0x34;name(d,0x200B,l.str,'A');
    d[l.money]=0x01;d[l.money+1]=0xE2;d[l.money+2]=0x40;d[l.idx]=0;
    for(std::size_t b=0;b<l.boxes;b++){emptyList(d,boxStart(l,b));name(d,l.names+b*9,9,static_cast<char>('A'+(b%26)));}
    pokemonList(d,boxStart(l,0),l.cap,32,l.str);emptyList(d,l.current);pokemonList(d,l.party,6,48,l.str);
    checksum(d,l);return d;
}
void writeFile(const fs::path& path,const std::vector<uint8_t>& bytes){
    fs::create_directories(path.parent_path());std::ofstream f(path,std::ios::binary);assert(f);f.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));assert(f.good());
}
const GSCSource* byPath(const GSCDiscoveryResult& r,std::string_view leaf){
    for(const auto& s:r.sources) {
        if(fs::path(s.path).filename()==leaf) return &s;
    }
    return nullptr;
}
}

int main(){
    const fs::path root=fs::temp_directory_path()/"pokebank-gsc-discovery";fs::remove_all(root);fs::create_directories(root);
    const auto gs=fixture(IGS), crystal=fixture(IC);
    writeFile(root/"Pokemon Gold.srm",gs);writeFile(root/"Pokemon Silver.sav",gs);writeFile(root/"mystery.srm",gs);
    writeFile(root/"Pokemon Crystal.srm",crystal);writeFile(root/"unknown.sav",crystal);
    writeFile(root/"Pokemon Gold Crystal.srm",gs);
    auto bad=gs;bad[0x2100]^=1;writeFile(root/"Pokemon Gold bad.srm",bad);
    writeFile(root/"Pokemon Silver short.sav",std::vector<uint8_t>(123));
    {std::ofstream f(root/"Pokemon Gold.state");f<<"savestate";}

    std::vector<std::string> roots{root.string()};const auto result=discoverGSCSaves(roots);
    assert(result.filesExamined==8);
    const auto* gold=byPath(result,"Pokemon Gold.srm");assert(gold&&gold->ready()&&gold->gameId=="gold_gbc");
    const auto* silver=byPath(result,"Pokemon Silver.sav");assert(silver&&silver->ready()&&silver->gameId=="silver_gbc");
    const auto* ambiguous=byPath(result,"mystery.srm");assert(ambiguous&&ambiguous->status==GSCSourceStatus::AmbiguousIdentity);
    const auto* namedC=byPath(result,"Pokemon Crystal.srm");assert(namedC&&namedC->ready()&&namedC->gameId=="crystal_gbc");
    const auto* structuralC=byPath(result,"unknown.sav");assert(structuralC&&structuralC->ready()&&structuralC->gameId=="crystal_gbc");
    const auto* multi=byPath(result,"Pokemon Gold Crystal.srm");assert(multi&&multi->status==GSCSourceStatus::AmbiguousIdentity);
    const auto* invalid=byPath(result,"Pokemon Gold bad.srm");assert(invalid&&invalid->status==GSCSourceStatus::InvalidSave);
    const auto* shortSave=byPath(result,"Pokemon Silver short.sav");assert(shortSave&&shortSave->status==GSCSourceStatus::InvalidSave);
    assert(byPath(result,"Pokemon Gold.state")==nullptr);

    auto wrapped=gs;wrapped.insert(wrapped.end(),7,0);writeFile(root/"wrapped"/"Pokemon Gold.srm",wrapped);
    const std::vector<std::string> wrappedRoot{(root/"wrapped").string()};auto wr=discoverGSCSaves(wrappedRoot);assert(wr.sources.size()==1&&wr.sources[0].ready());assert(wr.sources[0].save->metadata().rtcFooterSize==7);

    const std::string stableIdentity=gold->sourceIdentity;const std::string oldFingerprint=gold->contentFingerprint;
    auto changed=gs;changed[0x200B]=0x81;checksum(changed,IGS);writeFile(root/"Pokemon Gold.srm",changed);
    const auto refreshed=discoverGSCSaves(roots);const auto* refreshedGold=byPath(refreshed,"Pokemon Gold.srm");assert(refreshedGold&&refreshedGold->ready());assert(refreshedGold->sourceIdentity==stableIdentity);assert(refreshedGold->contentFingerprint!=oldFingerprint);

    const auto limited=discoverGSCSaves(roots,{2,2});assert(limited.filesExamined==2&&limited.limitReached);
    writeFile(root/"deep"/"one"/"two"/"Pokemon Gold.srm",gs);const std::vector<std::string> deepRoot{(root/"deep").string()};
    const auto shallow=discoverGSCSaves(deepRoot,{1,256});assert(shallow.sources.empty());const auto deep=discoverGSCSaves(deepRoot,{2,256});assert(deep.sources.size()==1&&deep.sources[0].ready());

    const fs::path cfgDir=root/"config";const fs::path configured=cfgDir/"configured";const fs::path fallback=root/"fallback";fs::create_directories(configured);fs::create_directories(fallback);
    writeFile(configured/"Pokemon Silver.srm",gs);writeFile(fallback/"Pokemon Gold.srm",gs);
    const fs::path cfg=cfgDir/"retroarch.cfg";{std::ofstream f(cfg);f<<"savefile_directory = \"configured\"\n";}
    auto configuredResult=discoverConfiguredRetroArchGSCSaves({},cfg.string(),fallback.string());assert(configuredResult.activeRootKind==GSCDiscoveryResult::RootKind::Configured);assert(configuredResult.sources.size()==1&&configuredResult.sources[0].gameId=="silver_gbc");
    fs::remove_all(configured);auto fallbackResult=discoverConfiguredRetroArchGSCSaves({},cfg.string(),fallback.string());assert(fallbackResult.activeRootKind==GSCDiscoveryResult::RootKind::ConventionalFallback);assert(fallbackResult.sources.size()==1&&fallbackResult.sources[0].gameId=="gold_gbc");

    fs::remove_all(root);std::cout<<"Generation II bounded RetroArch discovery: PASS\n";
}
