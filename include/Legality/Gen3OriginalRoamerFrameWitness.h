#pragma once
// L1 Stage 3: independent original PRE-PID roamer source frame replay.
// Distinguishes R/S/FRLG's truncated 8-bit save IV bug from Emerald full IVs.
// This is a frame/capture-stage witness only, not global legality.
// References: pret/pokeruby@5784633 and pret/pokeemerald@731ad5b
// roamer.c and src/random.c, pret/pokefirered@037335f roamer.c.
#include "Legality/Gen3OriginalWildSourceStage.h"
#include <array>
#include <cstdint>
namespace Legality::Gen3OriginalRoamerFrameWitness {
namespace W=Legality::Gen3OriginalWildSourceStage;
enum class Status : uint8_t {
 UnknownOriginalGame, UnknownOriginalSelection, UnknownOriginalActivation,
 UnknownOriginalCaptureFrame, UnknownOriginalIvs, UnknownOriginalSpecies,
 CandidateSpeciesContradicted, CandidatePidContradicted, CandidateIvsContradicted,
 OriginalFrameCompatibleHistoryUnverified
};
struct Frame {
 W::Game exactGame=W::Game::Unknown;
 uint8_t originalStarter=255,originalEmeraldTvChoice=255;
 uint16_t originalSpecies=0;
 uint32_t originalPrePidSeed=0,originalPid=0;
 std::array<uint8_t,6> originalIVs{}; // HP,Atk,Def,Spe,SpA,SpD
 bool originalGameAttested=false,originalRoamerActivationAttested=false;
 bool originalRoamerChoiceAttested=false,originalPrePidFrameAttested=false;
 bool originalSpeciesAttested=false,originalIvsAttested=false;
};
struct Result {Status status=Status::UnknownOriginalGame;uint16_t expectedSpecies=0;bool truncated=false;};
constexpr uint32_t next(uint32_t s) noexcept {return s*1103515245u+24691u;}
constexpr uint16_t expectedSpecies(W::Game game,uint8_t starter,uint8_t tvChoice) noexcept {
 switch(game){
 case W::Game::Ruby:return 381;
 case W::Game::Sapphire:return 380;
 case W::Game::Emerald:return tvChoice==0?380:tvChoice==1?381:0;
 case W::Game::FireRed:case W::Game::LeafGreen:
  return starter==0?244:starter==1?245:starter==2?243:0;
 default:return 0;
 }
}
constexpr Result evaluate(const Frame& c) noexcept {
 if(!c.originalGameAttested||c.exactGame>=W::Game::Unknown)return {};
 if(!c.originalRoamerActivationAttested)return {Status::UnknownOriginalActivation};
 if((c.exactGame==W::Game::Emerald||c.exactGame==W::Game::FireRed||c.exactGame==W::Game::LeafGreen)
    && !c.originalRoamerChoiceAttested)return {Status::UnknownOriginalSelection};
 const uint16_t species=expectedSpecies(c.exactGame,c.originalStarter,c.originalEmeraldTvChoice);
 if(species==0)return {Status::UnknownOriginalSelection};
 const bool truncated=c.exactGame!=W::Game::Emerald;
 if(!c.originalPrePidFrameAttested)return {Status::UnknownOriginalCaptureFrame,species,truncated};
 if(!c.originalSpeciesAttested || c.originalSpecies==0)return {Status::UnknownOriginalSpecies,species,truncated};
 if(!c.originalIvsAttested)return {Status::UnknownOriginalIvs,species,truncated};
 for(auto iv:c.originalIVs)if(iv>31)return {Status::UnknownOriginalIvs,species,truncated};
 if(c.originalSpecies!=species)return {Status::CandidateSpeciesContradicted,species,truncated};
 uint32_t rng=next(c.originalPrePidSeed);const uint32_t low=rng>>16;
 rng=next(rng);const uint32_t pid=low|((rng>>16)<<16);
 rng=next(rng);const uint16_t first=static_cast<uint16_t>((rng>>16)&0x7fffu);
 rng=next(rng);const uint16_t second=static_cast<uint16_t>((rng>>16)&0x7fffu);
 if(c.originalPid!=pid)return {Status::CandidatePidContradicted,species,truncated};
 const uint16_t storedFirst=truncated?static_cast<uint16_t>(first&0xff):first;
 const uint16_t storedSecond=truncated?0:second;
 const uint16_t observedFirst=static_cast<uint16_t>(c.originalIVs[0]|(c.originalIVs[1]<<5)|(c.originalIVs[2]<<10));
 const uint16_t observedSecond=static_cast<uint16_t>(c.originalIVs[3]|(c.originalIVs[4]<<5)|(c.originalIVs[5]<<10));
 if(storedFirst!=observedFirst || storedSecond!=observedSecond)
  return {Status::CandidateIvsContradicted,species,truncated};
 return {Status::OriginalFrameCompatibleHistoryUnverified,species,truncated};
}
} // namespace Legality::Gen3OriginalRoamerFrameWitness
