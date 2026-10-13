#pragma once
// L2 Phase A: PKHeX.Core Gen V/VI EGG TEMPLATE permission adapter.
// Source: kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e
// GPL-3.0-or-later, original PKHeX copyright holders. PokeBank NX AGPLv3.
// Source paths: Legality/Verifiers/Ability/AbilityBreedLegality.cs
//               Legality/Encounters/Templates/Gen5/EncounterEgg5.cs
//               Legality/Encounters/Templates/Gen6/EncounterEgg6.cs
//               Legality/Breeding.cs
// Exact upstream ability bit-mask (BanHidden5) preserved; other functions
// below are the smallest native adapters, not a PK5 or PK6 parser.
#include <array>
#include <cstdint>
namespace Legality::Gen56PKHeXEggTemplate {
enum class Game : std::uint8_t { Unknown, Black, White, Black2, White2, X, Y, OmegaRuby, AlphaSapphire };
enum class Status : std::uint8_t { Unsupported, IncompleteEvidence, LocalContradiction, ConditionallyCompatible };
constexpr unsigned generation(Game g) noexcept {
    return g>=Game::Black && g<=Game::White2 ? 5u :
           g>=Game::X && g<=Game::AlphaSapphire ? 6u : 0u;
}
constexpr std::array<std::uint8_t,82> BanHidden5 = {
  0x92,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
  0x10,0x10,0x20,0x00,0x01,0x11,0x02,0x00,0x49,0x00,0x00,
  0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x10,0x00,0x90,0x04,
  0x00,0x00,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x86,0x80,
  0x49,0x00,0x40,0x00,0x48,0x02,0x00,0x00,0x10,0x00,0x12,
  0x00,0x00,0x00,0x00,0x80,0x00,0x82,0x24,0x80,0x0A,0x00,
  0x00,0x0C,0x00,0x00,0x44,0x44,0x00,0x00,0xA0,0x84,0x80,
  0x40,0x08,0x12
};
static_assert(BanHidden5.size()==82);
// Literal port of AbilityBreedLegality.IsHiddenPossible5.
constexpr bool isHiddenPossible5(std::uint16_t species) noexcept {
    auto index=static_cast<unsigned>(species)>>3u;
    return index>=BanHidden5.size() || (BanHidden5[index] & (1u<<(species&7u)))==0;
}
// Literal port of AbilityBreedLegality.IsHiddenPossible6.
constexpr bool isHiddenPossible6(std::uint16_t species,std::uint8_t form) noexcept {
    switch(species){
        case 351: case 455: case 479: case 489: case 566: case 615:
        case 679: case 676: return false; // Castform, Carnivine, Rotom,
                                          // Phione, Archen, Cryogonal, Honedge, Furfrou
        case 669: return form!=2 && form!=4; // Flabebe
        case 710: return form!=1 && form!=2; // Pumpkaboo
        default:return true;
    }
}
constexpr bool splitBreedNotBaby(std::uint16_t species) noexcept {
    // Breeding.IsSplitBreedNotBabySpecies4 (applies to both Gen V and VI).
    switch(species){
        case 183: case 202: case 113: case 122: case 143:
        case 185: case 226: case 315: case 358: return true;
        default:return false;
    }
}
constexpr bool speciesDeterminedByGender(std::uint16_t species) noexcept {
    // Breeding.IsGenderSpeciesDetermination (Gen III–VI relevant subset).
    return species==29 || species==32 || species==313 || species==314;
}
constexpr bool validSpecies(Game game,std::uint16_t species) noexcept {
    const auto gen=generation(game);
    return species!=0 && (gen==5?species<=649:gen==6?species<=721:false);
}
struct Claim {
    Game originGame=Game::Unknown;
    std::uint16_t originalEggSpecies=0;
    std::uint8_t form=0;
    std::uint8_t originalAbilityNumber=0; // 1,2 or 4, as defined by PKHeX
    std::uint16_t originalBall=0; // Poké Ball=4, per PKHeX Ball enum
    bool originalEggSourceIndependentlyAttested=false;
    bool originalAbilityIndependentlyAttested=false;
    bool originalBallIndependentlyAttested=false;
    bool originalBallInheritanceMethodIndependentlyVerified=false;
};
struct Result {Status status=Status::IncompleteEvidence;bool hiddenPossible=false;bool ballPermissionSourceKnown=false;};
constexpr Result evaluate(const Claim& c) noexcept {
    Result result{};
    const auto gen=generation(c.originGame);
    if(!validSpecies(c.originGame,c.originalEggSpecies)) {result.status=Status::Unsupported;return result;}
    if(c.form>31 || (c.originalAbilityNumber!=1 && c.originalAbilityNumber!=2 && c.originalAbilityNumber!=4)) {
        result.status=Status::IncompleteEvidence;return result;
    }
    result.hiddenPossible=gen==5?isHiddenPossible5(c.originalEggSpecies):
                                     isHiddenPossible6(c.originalEggSpecies,c.form);
    if(!c.originalEggSourceIndependentlyAttested ||
       !c.originalAbilityIndependentlyAttested ||
       !c.originalBallIndependentlyAttested) return result;
    if(c.originalAbilityNumber==4 && !result.hiddenPossible) {
        result.status=Status::LocalContradiction;return result;
    }
    // EncounterEgg5.FixedBall == Ball.Poke, EncounterEgg6.FixedBall == None.
    if(gen==5 && c.originalBall!=4) {result.status=Status::LocalContradiction;return result;}
    result.ballPermissionSourceKnown=gen==5 || c.originalBall==4 ||
                                     c.originalBallInheritanceMethodIndependentlyVerified;
    if(!result.ballPermissionSourceKnown) return result;
    result.status=Status::ConditionallyCompatible;
    return result;
}
static_assert(!isHiddenPossible5(1));
static_assert(!isHiddenPossible6(669,2));
static_assert(isHiddenPossible6(669,1));
static_assert(splitBreedNotBaby(183));
} // namespace Legality::Gen56PKHeXEggTemplate
