#include "Legality/Gen3OriginalRoamerFrameWitness.h"
#include <cassert>
#include <cstdint>
#include <iostream>
int main(){
 namespace H=Legality::Gen3OriginalRoamerFrameWitness;
 namespace W=Legality::Gen3OriginalWildSourceStage;
 using S=H::Status;
 static_assert(H::expectedSpecies(W::Game::Ruby,255,255)==381);
 static_assert(H::expectedSpecies(W::Game::Sapphire,255,255)==380);
 static_assert(H::expectedSpecies(W::Game::Emerald,255,0)==380);
 static_assert(H::expectedSpecies(W::Game::Emerald,255,1)==381);
 static_assert(H::expectedSpecies(W::Game::FireRed,0,255)==244);
 static_assert(H::expectedSpecies(W::Game::LeafGreen,1,255)==245);
 unsigned positives=0,negatives=0,unknown=0;
 for(int game=0;game<5;++game){
  for(uint32_t i=0;i<1024;++i){
   H::Frame f{};f.exactGame=static_cast<W::Game>(game);
   f.originalStarter=static_cast<uint8_t>(i%3);f.originalEmeraldTvChoice=static_cast<uint8_t>(i%2);
   f.originalSpecies=H::expectedSpecies(f.exactGame,f.originalStarter,f.originalEmeraldTvChoice);
   f.originalPrePidSeed=i*1000003u+0xCAFEu;
   f.originalGameAttested=f.originalRoamerActivationAttested=true;
   f.originalRoamerChoiceAttested=f.originalPrePidFrameAttested=true;
   f.originalSpeciesAttested=f.originalIvsAttested=true;
   // Separate 64-bit reference LCRNG replay; never reverse PID as original frame.
   auto step=[](uint32_t s){return static_cast<uint32_t>((uint64_t(s)*1103515245ull+24691ull)&0xffffffffull);};
   uint32_t a=step(f.originalPrePidSeed),b=step(a),c=step(b),d=step(c);
   f.originalPid=(a>>16)|((b>>16)<<16);
   uint16_t iv1=static_cast<uint16_t>((c>>16)&0x7fff),iv2=static_cast<uint16_t>((d>>16)&0x7fff);
   if(f.exactGame!=W::Game::Emerald){iv1&=255u;iv2=0;}
   f.originalIVs={static_cast<uint8_t>(iv1&31),static_cast<uint8_t>((iv1>>5)&31),
     static_cast<uint8_t>((iv1>>10)&31),static_cast<uint8_t>(iv2&31),
     static_cast<uint8_t>((iv2>>5)&31),static_cast<uint8_t>((iv2>>10)&31)};
   const auto r=H::evaluate(f);
   assert(r.status==S::OriginalFrameCompatibleHistoryUnverified);
   assert(r.truncated==(f.exactGame!=W::Game::Emerald));++positives;
   auto x=f;x.originalSpecies^=1u;
   assert(H::evaluate(x).status==S::CandidateSpeciesContradicted);++negatives;
   x=f;x.originalPid^=1u;
   assert(H::evaluate(x).status==S::CandidatePidContradicted);++negatives;
   x=f;x.originalIVs[0]^=1u;
   assert(H::evaluate(x).status==S::CandidateIvsContradicted);++negatives;
   x=f;x.originalGameAttested=false;
   assert(H::evaluate(x).status==S::UnknownOriginalGame);++unknown;
   x=f;x.originalPrePidFrameAttested=false;
   assert(H::evaluate(x).status==S::UnknownOriginalCaptureFrame);++unknown;
   x=f;x.originalIvsAttested=false;
   assert(H::evaluate(x).status==S::UnknownOriginalIvs);++unknown;
   x=f;x.originalIVs[5]=255;
   assert(H::evaluate(x).status==S::UnknownOriginalIvs);++unknown;
  }
 }
 assert(positives==5120);
 std::cout<<"L1 Gen3 original roamer frames "<<positives<<" positive, "<<negatives
          <<" candidate-local, "<<unknown<<" Unknown: PASS\n";
}
