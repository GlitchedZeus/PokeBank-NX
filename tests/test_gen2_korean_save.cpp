// Synthetic structural fixtures; never represented as authentic cartridge saves.
// Constants: kwsch/PKHeX@6501f0ab SAV2Offsets.LoadOffsetsKorean; SAV2.GetChecksum;
// StringConverter2KOR. Read-only provider addition; no Korean edit/export support.
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Utils/Gen2KoreanTable.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
namespace G=PokeVault::Integration::Gen2;
void sum(std::vector<uint8_t>& d) { unsigned n=0;for(size_t i=0x2009;i<=0x2DAA;++i)n+=d[i];d[0x2DAB]=n;d[0x2DAC]=n>>8; }
void name(std::vector<uint8_t>& d,size_t at,size_t length) {std::fill_n(d.begin()+at,length,0x50);d[at]=1;d[at+1]=1;}
std::vector<uint8_t> fixture() {
 std::vector<uint8_t>d(0x8000);name(d,0x200B,11);d[0x2009]=0x12;d[0x200A]=0x34;
 d[0x26FC]=0;d[0x2DAE]=0;d[0x2DAF]=0xFF;
 for(size_t b=0;b<14;++b){const size_t at=b<7?0x4000+b*0x450:0x6000+(b-7)*0x450;d[at]=0;d[at+1]=0xFF;name(d,0x26FF+b*17,17);}
 d[0x28CC]=1;d[0x28CD]=19;d[0x28CE]=0xFF;
 const size_t body=0x28CC+8;d[body]=19;d[body+2]=33;d[body+8]=0;d[body+9]=0;d[body+10]=125;
 d[body+21]=0x98;d[body+22]=0x76;d[body+23]=35;d[body+27]=70;d[body+31]=5;
 name(d,body+6*48,11);name(d,body+6*48+6*11,11);
 // Empty native pockets; inventory failure must never rewrite or reject the source.
 for(size_t at:{0x2417u,0x2441u,0x245Cu,0x2476u}){d[at]=0;d[at+1]=0xFF;}
 sum(d);return d;
}
int main(){
 size_t checks=0;
 // Every byte pair must be bounded and agree with the independently generated mapping.
 for(unsigned page=1;page<=11;++page)for(unsigned index=0;index<256;++index){
  const std::array<uint8_t,3>b{static_cast<uint8_t>(page),static_cast<uint8_t>(index),0x50};
  const auto s=G::decodeGen2String(b,G::RegionLayout::Korean);
  assert(s.empty()==(Utils::Gen2KoreanTable[page][index]==0));++checks;
 }
 assert(G::decodeGen2String(std::array<uint8_t,3>{1,1,0x50},G::RegionLayout::Korean)=="가");
 assert(G::decodeGen2String(std::array<uint8_t,1>{1},G::RegionLayout::Korean).empty());
 assert(G::decodeGen2String(std::array<uint8_t,2>{0x5D,0x50},G::RegionLayout::Korean)=="*");
 auto d=fixture();const auto before=d;
 for(auto game:{G::SourceGame::Gold,G::SourceGame::Silver}){
  const auto r=G::parse(d,game);if(!r)std::cerr<<r.detail<<'\n';assert(r);
  assert(r.save->metadata().region==G::RegionLayout::Korean);
  assert(r.save->metadata().boxCount==14 && r.save->metadata().boxCapacity==20);
  assert(r.save->trainer().name=="가");assert(r.save->party()[0].originalTrainer=="가");
  assert(r.save->party()[0].nameEncoding==G::RegionLayout::Korean);
  assert(r.save->party()[0].originalTrainerRaw.size()==11);
  assert(r.save->boxes()[13].name=="가");
  assert(G::decodeInventory(d,G::RegionLayout::Korean,G::VersionFamily::GoldSilver).available);
  std::string error;assert(!G::StagedEditor::create(*r.save,error)); // future write support requires its own proofs
 }
 assert(d==before);
 assert(!G::parse(d,G::SourceGame::Crystal));
 assert(!G::decodeInventory(d,G::RegionLayout::Korean,G::VersionFamily::Crystal).available);
 d[0x2009]^=1;assert(!G::parse(d,G::SourceGame::Gold));d=before;
 d[0x28CC]=7;sum(d);assert(!G::parse(d,G::SourceGame::Gold));d=before;
 d[0x26FC]=14;sum(d);assert(!G::parse(d,G::SourceGame::Gold));
 std::cout<<"Korean Gold/Silver read-only save: "<<checks<<" character pairs, both source hints, layout/count/checksum/malformed/immutability/edit refusal PASS\n";
}
