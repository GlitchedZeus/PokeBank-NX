#pragma once
// L1 E06/E08/E11: original game and ORIGINAL roaming capture state,
// witnessed separately from a detached or evolved PK3. Retail R/S/FRLG
// store only the bottom eight IV bits in the roaming save instance; Emerald
// stores full Method 1 IVs. Comparing the *original pre-PID state* forward
// avoids false rejection caused by 8-bit roamer seed-reversal collisions.
// No failure here proves all historical sources impossible.
#include "Legality/Gen3PidIvCorrelation.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3OriginalRoamerStage {
enum class Game : uint8_t {
    Unknown,Ruby,Sapphire,Emerald,FireRed,LeafGreen
};
enum class Starter : uint8_t {
    Unknown,Bulbasaur,Charmander,Squirtle
};
enum class Status : uint8_t {
    UnknownOriginalGame,UnknownOriginalRoamerActivation,
    UnknownOriginalRoamerSelection,UnknownOriginalRoute,
    UnknownOriginalPidIvStage,MalformedOriginalIvs,
    CandidateOriginalSpeciesContradicted,
    CandidateOriginalLevelContradicted,
    CandidateOriginalRouteContradicted,
    CandidateOriginalPidIvContradicted,
    ConditionalOriginalRoamerHistory
};
struct Candidate {
    Game originalGame=Game::Unknown;
    Starter originalStarter=Starter::Unknown;
    // Pinned Emerald television option: Red 0 => Latias, Blue 1 => Latios.
    uint8_t originalEmeraldTvChoice=0xFFu;
    uint16_t originalSpecies=0;
    uint8_t originalLevel=0;
    // Retail roaming map GROUP 0 in R/S/E; GROUP 3 in FR/LG.
    uint8_t originalMapGroup=0xFFu;
    // R/S roamer.c map numbers are original raw Hoenn map IDs.
    // Emerald's symbolic MAP_ROUTE110 etc are intentionally NOT assumed
    // equal to the original R/S raw map numbering without source proof.
    uint8_t originalMapNum=0xFFu;
    // FR/LG is attested by original map's Route 1..25 label. This value
    // is NOT presumed to equal original mapNum or PK3 met-location number.
    uint8_t originalKantoRouteNumber=0;
    uint32_t originalPid=0,originalPrePidSeed=0;
    std::array<uint8_t,6> originalIVs{};
    bool originalGameIndependentlyAttested=false;
    bool originalRoamerActivationIndependentlyAttested=false;
    bool originalRoamerChoiceIndependentlyAttested=false;
    bool originalMapContextIndependentlyAttested=false;
    bool originalPidIvAndSeedIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    uint16_t sourceSpecies=0;
    uint8_t sourceLevel=0;
    uint32_t originalSeed=0;
    bool usesTruncatedRoamerIvStorage=false;
};

constexpr Game gameForId(std::string_view id) noexcept {
    if(id=="ruby_gba")return Game::Ruby;
    if(id=="sapphire_gba")return Game::Sapphire;
    if(id=="emerald_gba")return Game::Emerald;
    if(id=="firered_gba")return Game::FireRed;
    if(id=="leafgreen_gba")return Game::LeafGreen;
    return Game::Unknown;
}
constexpr bool isRse(Game game) noexcept {
    return game==Game::Ruby||game==Game::Sapphire||game==Game::Emerald;
}
constexpr bool isFrlg(Game game) noexcept {
    return game==Game::FireRed||game==Game::LeafGreen;
}
constexpr bool validRubySapphireOriginalMap(uint8_t map) noexcept {
    return map==0x19u || map==0x1Au ||
           (map>=0x20u && map<=0x31u);
}
constexpr uint16_t speciesForOriginalSource(Game game,Starter starter,
                                            uint8_t tvChoice) noexcept {
    if(game==Game::Ruby)return 381u;    // Ruby Latios
    if(game==Game::Sapphire)return 380u;// Sapphire Latias
    if(game==Game::Emerald)
        return tvChoice==0u?380u:tvChoice==1u?381u:0u;
    if(!isFrlg(game))return 0;
    switch(starter){
        case Starter::Bulbasaur:return 244u;  // Entei
        case Starter::Charmander:return 245u; // Suicune
        case Starter::Squirtle:return 243u;   // Raikou
        case Starter::Unknown:return 0u;
    }
    return 0u;
}
constexpr uint32_t packedIv1(const std::array<uint8_t,6>& iv) noexcept {
    return uint32_t(iv[0])|(uint32_t(iv[1])<<5)|(uint32_t(iv[2])<<10);
}
constexpr uint32_t packedIv2(const std::array<uint8_t,6>& iv) noexcept {
    return uint32_t(iv[3])|(uint32_t(iv[4])<<5)|(uint32_t(iv[5])<<10);
}
inline Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameIndependentlyAttested ||
       c.originalGame==Game::Unknown)return {};
    if(!c.originalRoamerActivationIndependentlyAttested)
        return {Status::UnknownOriginalRoamerActivation};
    if((c.originalGame==Game::Emerald ||
        isFrlg(c.originalGame)) &&
       !c.originalRoamerChoiceIndependentlyAttested)
        return {Status::UnknownOriginalRoamerSelection};
    const uint16_t expected=speciesForOriginalSource(
        c.originalGame,c.originalStarter,c.originalEmeraldTvChoice);
    if(expected==0u)return {Status::UnknownOriginalRoamerSelection};
    const uint8_t level=isFrlg(c.originalGame)?50u:40u;
    const bool truncated=c.originalGame!=Game::Emerald;
    if(!c.originalMapContextIndependentlyAttested)
        return {Status::UnknownOriginalRoute,expected,level,0,truncated};
    if(!c.originalPidIvAndSeedIndependentlyAttested)
        return {Status::UnknownOriginalPidIvStage,expected,level,0,truncated};
    if(!Gen3PidIv::validStoredIVs(c.originalIVs))
        return {Status::MalformedOriginalIvs,expected,level,0,truncated};
    if(c.originalSpecies!=expected)
        return {Status::CandidateOriginalSpeciesContradicted,expected,level,0,truncated};
    if(c.originalLevel!=level)
        return {Status::CandidateOriginalLevelContradicted,expected,level,0,truncated};
    if(c.originalMapGroup!=(isFrlg(c.originalGame)?3u:0u) ||
       (isFrlg(c.originalGame) ?
           (c.originalKantoRouteNumber<1u ||
            c.originalKantoRouteNumber>25u) :
        c.originalGame==Game::Emerald ?
            c.originalMapNum==0xFFu :
            !validRubySapphireOriginalMap(c.originalMapNum)))
        return {Status::CandidateOriginalRouteContradicted,expected,level,0,truncated};

    // Source-stage original RNG seed: two PID halves then two 15-bit IV
    // words. In R/S/FRLG only low 8 bits of IV32 survive the buggy
    // roaming save serialization; no reverse-first-seed shortcut allowed.
    uint32_t state=c.originalPrePidSeed;
    state=Gen3PidIv::Detail::next(state);
    const uint32_t pidLo=state>>16;
    state=Gen3PidIv::Detail::next(state);
    const uint32_t pidHi=state>>16;
    const uint32_t generatedPid=(pidHi<<16)|pidLo;
    state=Gen3PidIv::Detail::next(state);
    const uint32_t iv1=(state>>16)&0x7FFFu;
    state=Gen3PidIv::Detail::next(state);
    const uint32_t iv2=(state>>16)&0x7FFFu;
    const uint32_t wanted1=truncated?iv1&0xFFu:iv1;
    const uint32_t wanted2=truncated?0u:iv2;
    if(c.originalPid!=generatedPid ||
       packedIv1(c.originalIVs)!=wanted1 ||
       packedIv2(c.originalIVs)!=wanted2)
        return {Status::CandidateOriginalPidIvContradicted,
                expected,level,c.originalPrePidSeed,truncated};
    return {Status::ConditionalOriginalRoamerHistory,
            expected,level,c.originalPrePidSeed,truncated};
}
} // namespace Legality::Gen3OriginalRoamerStage
