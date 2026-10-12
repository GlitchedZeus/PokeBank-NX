// Independent L5 format/provenance adversarial model: synthetic bytes ONLY.
// This is NOT production parsing, an authenticated original record, or proof of legality.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>
#include <algorithm>
using Bytes=std::vector<uint8_t>;
static unsigned long long checks=0;
#define CHECK(x) do {++checks; if(!(x)){std::cerr<<"failed "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
static uint16_t le16(std::span<const uint8_t> b,size_t i){return uint16_t(b[i])|(uint16_t(b[i+1])<<8);}
static uint16_t be16(std::span<const uint8_t> b,size_t i){return uint16_t(b[i])<<8|b[i+1];}
static void put16(Bytes& b,size_t i,uint16_t x){b[i]=uint8_t(x);b[i+1]=uint8_t(x>>8);}
static unsigned sum16(std::span<const uint8_t> b,size_t begin,size_t end){unsigned x=0;for(size_t i=begin;i<end;i+=2)x+=le16(b,i);return x&65535u;}
enum class Result{Empty,StructuralReject,UnknownSource,PlausibleNative};
struct Answer{Result result; bool conflict;uint8_t attackDv,hpDv,ppUps,level;};
static int sourceGen(std::string_view s){
 if(s=="red_gb"||s=="blue_gb"||s=="yellow_gb")return 1;
 if(s=="gold_gbc"||s=="silver_gbc"||s=="crystal_gbc")return 2;
 if(s=="ruby_gba"||s=="sapphire_gba"||s=="emerald_gba"||s=="firered_gba"||s=="leafgreen_gba")return 3;
 if(s=="diamond_nds"||s=="pearl_nds"||s=="platinum_nds"||s=="heartgold_nds"||s=="soulsilver_nds")return 4;
 return 0;
}
static Answer inspect(int gen,std::span<const uint8_t> b,uint8_t marker,std::string_view source){
 Answer a{Result::StructuralReject,false,0,0,0,0};
 a.conflict=sourceGen(source)!=0&&sourceGen(source)!=gen;
 const size_t n=b.size();
 if(gen==1){
  if(n!=33&&n!=44)return a;
  if(b[0]==0)return {Result::Empty,a.conflict,0,0,0,0};
  if(marker!=b[0]||b[0]==0xff||b[0]==0x1f)return a;
  a.level=b[n==44?0x21:0x03];if(a.level>100)return a;
  const uint16_t packed=be16(b,0x1b);
  const uint8_t atk=uint8_t(packed>>12),def=uint8_t((packed>>8)&15),spe=uint8_t((packed>>4)&15),spc=uint8_t(packed&15);
  a.attackDv=atk;a.hpDv=uint8_t(((atk&1)<<3)|((def&1)<<2)|((spe&1)<<1)|(spc&1));
  a.ppUps=b[0x1d]>>6;
 }else if(gen==2){
  if(n!=32&&n!=48)return a;
  if(b[0]==0)return {Result::Empty,a.conflict,0,0,0,0};
  if(b[0]>251||(marker!=b[0]&&marker!=0xfd))return a;
  a.level=b[31];if(a.level>100)return a;
  const uint8_t atk=b[21]>>4,def=b[21]&15,spe=b[22]>>4,spc=b[22]&15;
  a.attackDv=atk;a.hpDv=uint8_t(((atk&1)<<3)|((def&1)<<2)|((spe&1)<<1)|(spc&1));
  a.ppUps=b[23]>>6;
 }else if(gen==3||gen==4){
  if((gen==3&&(n!=80&&n!=100))||(gen==4&&(n!=136&&n!=236)))return a;
  const size_t begin=gen==3?0x20:0x08, end=gen==3?0x50:0x88;
  const uint16_t species=le16(b,begin);
  if(gen==4&&le16(b,4)!=0)return a;
  if(le16(b,gen==3?0x1c:6)!=sum16(b,begin,end))return a;
  if(species==0)return {Result::Empty,a.conflict,0,0,0,0};
  if((gen==3&&species>386)||(gen==4&&species>493))return a;
 }else return a;
 a.result=a.conflict||source.empty()||sourceGen(source)==0?Result::UnknownSource:Result::PlausibleNative;
 return a;
}
// PokeList headers are not tightly packed past the first sentinel: stale
// species after the immediate terminator are legitimate, not list entries.
// The Gen I model deliberately recognizes only two pinned internal IDs.
static bool listHeader(std::span<const uint8_t> h,size_t capacity,int gen){
 if(h.size()<capacity+2)return false;
 const size_t count=h[0];if(count>capacity)return false;
 if(h[1+count]!=0xff)return false;
 for(size_t i=0;i<count;i++){
  const uint8_t v=h[1+i];
  if(gen==1){if(v!=153&&v!=173)return false;}
  else if(v==0||v==0xff)return false;
 }
 return true;
}
static void testListHeaders(){
 for(int gen:{1,2})for(size_t cap:{6u,20u,30u})
  for(size_t count=0;count<=cap;count++){
   Bytes h(cap+2,173);h[0]=uint8_t(count);
   for(size_t i=0;i<count;i++)h[1+i]=uint8_t(gen==1?153:1);
   h[1+count]=0xff;
   CHECK(listHeader(h,cap,gen));
   CHECK(listHeader(h,cap+1,gen)==false); // truncation/cap mismatch
   if(count){
    auto bad=h;bad[1]=0;CHECK(!listHeader(bad,cap,gen));
    bad=h;bad[1]=0xff;CHECK(!listHeader(bad,cap,gen));
    if(gen==2){bad=h;bad[1]=0xfd;CHECK(listHeader(bad,cap,gen));}
   }
   auto corrupt=h;corrupt[1+count]=0;
   CHECK(!listHeader(corrupt,cap,gen));
   // Stale values after the first 0xff terminator do not invalidate.
   if(count<cap){auto stale=h;stale[2+count]=0;CHECK(listHeader(stale,cap,gen));}
   auto huge=h;huge[0]=uint8_t(cap+1);CHECK(!listHeader(huge,cap,gen));
  }
}
static Bytes specimen(int gen,bool party){
 Bytes b(gen==1?(party?44:33):gen==2?(party?48:32):gen==3?(party?100:80):(party?236:136),0);
 if(gen==1){b[0]=153;b[party?0x21:0x03]=5;b[0x1b]=0x12;b[0x1c]=0x34;b[0x1d]=0xc1;}
 if(gen==2){b[0]=1;b[31]=5;b[21]=0x12;b[22]=0x34;b[23]=0xc1;}
 if(gen==3||gen==4){const size_t k=gen==3?0x20:8;put16(b,k,1);put16(b,gen==3?0x1c:6,uint16_t(sum16(b,k,gen==3?0x50:0x88)));}
 return b;
}
static std::string_view matching(int g){return g==1?"red_gb":g==2?"gold_gbc":g==3?"ruby_gba":"platinum_nds";}
static std::string_view wrong(int g){return g==1?"platinum_nds":g==2?"firered_gba":g==3?"red_gb":"gold_gbc";}
int main(){
 testListHeaders();
 for(int gen=1;gen<=4;gen++)for(bool party:{false,true}){
  const auto base=specimen(gen,party);const auto immutable=base;
  const uint8_t marker=gen==1?153:1;
  for(auto source:{matching(gen),wrong(gen),std::string_view{},std::string_view("not_a_game")}){
   const Answer x=inspect(gen,base,marker,source);
   CHECK(x.result==(source==matching(gen)?Result::PlausibleNative:Result::UnknownSource));
   CHECK(x.conflict==(source==wrong(gen)));
   CHECK(base==immutable);
  }
  for(size_t len=0;len<=260;len++){
   if(len==base.size())continue;
   Bytes truncated(len,0xff);
   CHECK(inspect(gen,truncated,marker,matching(gen)).result==Result::StructuralReject);
  }
  auto empty=base;empty[gen<=2?0:(gen==3?0x20:8)]=0;
  if(gen>=3)put16(empty,gen==3?0x1c:6,uint16_t(sum16(empty,gen==3?0x20:8,gen==3?0x50:0x88)));
  CHECK(inspect(gen,empty,marker,matching(gen)).result==Result::Empty);
  CHECK(base==immutable);
  if(gen<=2){
   auto mismatch=base;CHECK(inspect(gen,mismatch,gen==1?173:2,matching(gen)).result==Result::StructuralReject);
   auto level=base;level[gen==1?(party?0x21:3):31]=101;
   CHECK(inspect(gen,level,marker,matching(gen)).result==Result::StructuralReject);
   if(gen==2){CHECK(inspect(gen,base,0xfd,matching(gen)).result==Result::PlausibleNative);}
   for(unsigned packed=0;packed<65536;packed++){
    auto x=base;
    if(gen==1){x[0x1b]=uint8_t(packed>>8);x[0x1c]=uint8_t(packed);}
    else{x[21]=uint8_t(packed>>8);x[22]=uint8_t(packed);}
    const auto ans=inspect(gen,x,marker,matching(gen));
    const uint8_t expected=uint8_t((((packed>>12)&1)<<3)|(((packed>>8)&1)<<2)|(((packed>>4)&1)<<1)|(packed&1));
    CHECK(ans.hpDv==expected);
    CHECK(ans.attackDv==(packed>>12));
    CHECK(ans.ppUps==3);
    CHECK(ans.result==Result::PlausibleNative);
   }
  }else{
   // Every flipped bit in checksum-covered canonical record data must invalidate
   // the original checksum; the input and surrounding header stay immutable.
   const size_t lo=gen==3?0x20:8,hi=gen==3?0x50:0x88;
   for(size_t byte=lo;byte<hi;byte++)for(unsigned bit=0;bit<8;bit++){
    auto damaged=base;damaged[byte]^=uint8_t(1u<<bit);
    CHECK(inspect(gen,damaged,marker,matching(gen)).result==Result::StructuralReject);
    CHECK(base==immutable);
   }
   auto checksum=base;checksum[gen==3?0x1c:6]^=1;
   CHECK(inspect(gen,checksum,marker,matching(gen)).result==Result::StructuralReject);
   if(gen==4){auto sanity=base;sanity[4]=1;CHECK(inspect(gen,sanity,marker,matching(gen)).result==Result::StructuralReject);}
   for(uint16_t ev:{0u,1u,252u,253u,254u,255u}){
    auto x=base;const size_t at=gen==3?0x38:0x18;
    x[at]=uint8_t(ev);put16(x,gen==3?0x1c:6,uint16_t(sum16(x,lo,hi)));
    CHECK(inspect(gen,x,marker,matching(gen)).result==Result::PlausibleNative);
    CHECK(inspect(gen,x,marker,wrong(gen)).result==Result::UnknownSource);
   }
  }
 }
 std::cout<<"L5 Stage2 native-format adversarial MODEL PASS "<<checks<<" assertions (synthetic; no production codec or original retail)\n";
}
