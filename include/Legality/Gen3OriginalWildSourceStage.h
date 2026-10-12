#pragma once
// L1 Stage 3: bounded, ORIGINAL R/S/E/FR/LG physical wild-source witnesses.
// Data copied as compact independently decoded source rows from pinned pret
// Ruby/Sapphire, Emerald and FireRed/LeafGreen wild_encounters.json files.
// This header checks a source template, never a global creature verdict.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen3OriginalWildSourceStage {
enum class Game : uint8_t { Ruby, Sapphire, Emerald, FireRed, LeafGreen, Unknown };
enum class Method : uint8_t { Grass, Surf, RockSmash, Fishing, Unknown };
enum class Rod : uint8_t { Old, Good, Super, Unknown };
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalSource, UnknownOriginalRod,
    UnknownOriginalIdentity, UnknownOriginalLevel, ForeignSource,
    CandidateGameContradicted, CandidateMethodContradicted,
    CandidateSpeciesContradicted, CandidateLevelContradicted,
    CandidateRodContradicted, SourceTemplateCompatibleHistoryUnverified
};
inline constexpr std::array<std::string_view,30> kSpecies{{
    "BELLSPROUT","CARVANHA","ELECTRIKE","GLOOM","GULPIN","GYARADOS",
    "HORSEA","KECLEON","KINGLER","KRABBY","LINOONE","MAGIKARP",
    "MINUN","ODDISH","PELIPPER","PIDGEY","PLUSLE","POOCHYENA",
    "PSYDUCK","RATTATA","SEADRA","SLOWPOKE","TENTACOOL","TROPIUS",
    "VENONAT","WAILMER","WEEPINBELL","WINGULL","WURMPLE","ZIGZAGOON"
}};
struct Group { Game game; std::string_view map; Method method; std::string_view packed; };
// Each six uppercase hexadecimal digits encode species dictionary index,
// min level, max level. Physical source-row order and duplicates are kept.
inline constexpr std::array<Group,33> kGroups{{
{Game::Ruby,"ROUTE101",Method::Grass,"1C02021D02021C02021C03031D03031D03031C03031D0303110202110202110303110303"},
{Game::Ruby,"ROUTE110",Method::Grass,"1D0C0C020C0C040C0C020D0D0C0D0D0D0D0D0C0D0D040D0D1B0C0C1B0C0C100C0C100D0D"},
{Game::Ruby,"ROUTE110",Method::Surf,"1605231B0A1E1B0F190E191E0E191E"},
{Game::Ruby,"ROUTE110",Method::Fishing,"0B050A16050A0B0A1E160A1E190A1E19191E191E2319141919232819282D"},
{Game::Ruby,"ROUTE119",Method::Grass,"1D19190A19191D1B1B0D19190A1B1B0D1A1A0D1B1B0D1818171919171A1A171B1B071919"},
{Game::Ruby,"ROUTE119",Method::Surf,"1605231B0A1E1B0F190E191E0E191E"},
{Game::Ruby,"ROUTE119",Method::Fishing,"0B050A16050A0B0A1E160A1E010A1E01191E011E2301141901232801282D"},
{Game::Sapphire,"ROUTE101",Method::Grass,"1C02021D02021C02021C03031D03031D03031C03031D0303110202110202110303110303"},
{Game::Sapphire,"ROUTE110",Method::Grass,"1D0C0C020C0C040C0C020D0D100D0D0D0D0D100D0D040D0D1B0C0C1B0C0C0C0C0C0C0D0D"},
{Game::Sapphire,"ROUTE110",Method::Surf,"1605231B0A1E1B0F190E191E0E191E"},
{Game::Sapphire,"ROUTE110",Method::Fishing,"0B050A16050A0B0A1E160A1E190A1E19191E191E2319141919232819282D"},
{Game::Sapphire,"ROUTE119",Method::Grass,"1D19190A19191D1B1B0D19190A1B1B0D1A1A0D1B1B0D1818171919171A1A171B1B071919"},
{Game::Sapphire,"ROUTE119",Method::Surf,"1605231B0A1E1B0F190E191E0E191E"},
{Game::Sapphire,"ROUTE119",Method::Fishing,"0B050A16050A0B0A1E160A1E010A1E01191E011E2301141901232801282D"},
{Game::Emerald,"ROUTE101",Method::Grass,"1C02021102021C02021C03031103031103031C03031103031D02021D02021D03031D0303"},
{Game::Emerald,"ROUTE110",Method::Grass,"110C0C020C0C040C0C020D0D0C0D0D0D0D0D0C0D0D040D0D1B0C0C1B0C0C100C0C100D0D"},
{Game::Emerald,"ROUTE110",Method::Surf,"1605231B0A1E1B0F190E191E0E191E"},
{Game::Emerald,"ROUTE110",Method::Fishing,"0B050A16050A0B0A1E160A1E190A1E19191E191E2319141919232819282D"},
{Game::Emerald,"ROUTE119",Method::Grass,"1D19190A19191D1B1B0D19190A1B1B0D1A1A0D1B1B0D1818171919171A1A171B1B071919"},
{Game::Emerald,"ROUTE119",Method::Surf,"1605231B0A1E1B0F190E191E0E191E"},
{Game::Emerald,"ROUTE119",Method::Fishing,"0B050A16050A0B0A1E160A1E010A1E01191E011E2301141901232801282D"},
{Game::FireRed,"ROUTE1",Method::Grass,"0F03031303030F03031303030F02021302020F03031303030F04041304040F0505130404"},
{Game::FireRed,"ROUTE12",Method::Grass,"0D18181818180D16160F17170F1919181A1A0D1A1A0F1B1B0F1717031C1C0F1717031E1E"},
{Game::FireRed,"ROUTE12",Method::Surf,"16050A160A1416141E161E23162328"},
{Game::FireRed,"ROUTE12",Method::Fishing,"0B05050B050506050F0B050F09050F060F19060F19050F19061923121923"},
{Game::FireRed,"ROUTE19",Method::Surf,"16050A160A1416141E161E23162328"},
{Game::FireRed,"ROUTE19",Method::Fishing,"0B05050B050506050F0B050F09050F060F19060F19050F19141923121923"},
{Game::LeafGreen,"ROUTE1",Method::Grass,"0F03031303030F03031303030F02021302020F03031303030F04041304040F0505130404"},
{Game::LeafGreen,"ROUTE12",Method::Grass,"0018181818180016160F17170F1919181A1A001A1A0F1B1B0F17171A1C1C0F17171A1E1E"},
{Game::LeafGreen,"ROUTE12",Method::Surf,"16050A160A1416141E161E23162328"},
{Game::LeafGreen,"ROUTE12",Method::Fishing,"0B05050B050509050F0B050F06050F090F19090F19050F19091923151923"},
{Game::LeafGreen,"ROUTE19",Method::Surf,"16050A160A1416141E161E23162328"},
{Game::LeafGreen,"ROUTE19",Method::Fishing,"0B05050B050509050F0B050F06050F090F19090F19050F19081923151923"}
}};
constexpr uint8_t digit(char c) noexcept { return c >= '0'&&c <= '9' ? static_cast<uint8_t>(c-'0') : c >= 'A' && c <= 'F' ? static_cast<uint8_t>(c-'A'+10) : 255; }
constexpr uint8_t byteAt(std::string_view s,std::size_t pos) noexcept {
    if(pos+1>=s.size()) return 255;
    const uint8_t a=digit(s[pos]),b=digit(s[pos+1]);
    return a>15||b>15?255:static_cast<uint8_t>(a*16+b);
}
struct Slot { uint8_t speciesIndex=255,minLevel=0,maxLevel=0; };
constexpr Slot decode(const Group& group,std::size_t physicalSlot) noexcept {
    const std::size_t pos=physicalSlot*6;
    if(pos+6>group.packed.size())return {};
    return {byteAt(group.packed,pos),byteAt(group.packed,pos+2),byteAt(group.packed,pos+4)};
}
constexpr std::size_t slotCount(const Group& group) noexcept { return group.packed.size()/6; }
constexpr Rod requiredRod(std::size_t physicalSlot) noexcept {
    return physicalSlot<2?Rod::Old:physicalSlot<5?Rod::Good:physicalSlot<10?Rod::Super:Rod::Unknown;
}
struct Candidate {
    Game originalGame=Game::Unknown;
    std::size_t exactGroupIndex=0,physicalSlot=0;
    std::string_view originalMap{};
    Method originalMethod=Method::Unknown;
    Rod originalRod=Rod::Unknown;
    std::string_view originalSpecies{};
    uint8_t originalLevel=0;
    bool originalGameAttested=false,originalGroupAndSlotAttested=false;
    bool originalMapAndMethodAttested=false,originalSpeciesAttested=false;
    bool originalLevelAttested=false,originalRodAttested=false;
};
struct Result { Status status=Status::UnknownOriginalSource; Slot source{}; };
constexpr Result evaluate(const Candidate& c) noexcept {
    if(!c.originalGameAttested || c.originalGame>=Game::Unknown)
        return {Status::UnknownOriginalGame};
    if(!c.originalGroupAndSlotAttested || c.exactGroupIndex>=kGroups.size())
        return {Status::UnknownOriginalSource};
    const Group& group=kGroups[c.exactGroupIndex];
    if(c.physicalSlot>=slotCount(group))return {Status::UnknownOriginalSource};
    const auto row=decode(group,c.physicalSlot);
    if(row.speciesIndex>=kSpecies.size() || row.minLevel==0 || row.minLevel>row.maxLevel)
        return {Status::UnknownOriginalSource};
    if(group.game!=c.originalGame)return {Status::ForeignSource,row};
    if(!c.originalMapAndMethodAttested || c.originalMap.empty() || c.originalMethod==Method::Unknown)
        return {Status::UnknownOriginalSource,row};
    if(!c.originalSpeciesAttested || c.originalSpecies.empty())
        return {Status::UnknownOriginalIdentity,row};
    if(!c.originalLevelAttested || c.originalLevel==0 || c.originalLevel>100)
        return {Status::UnknownOriginalLevel,row};
    if(group.method==Method::Fishing && (!c.originalRodAttested || c.originalRod==Rod::Unknown))
        return {Status::UnknownOriginalRod,row};
    if(c.originalMap!=group.map)return {Status::UnknownOriginalSource,row}; // unmodeled map may have other valid origins
    if(c.originalMethod!=group.method)return {Status::CandidateMethodContradicted,row};
    if(group.method==Method::Fishing && c.originalRod!=requiredRod(c.physicalSlot))
        return {Status::CandidateRodContradicted,row};
    if(c.originalSpecies!=kSpecies[row.speciesIndex])
        return {Status::CandidateSpeciesContradicted,row};
    if(c.originalLevel<row.minLevel || c.originalLevel>row.maxLevel)
        return {Status::CandidateLevelContradicted,row};
    return {Status::SourceTemplateCompatibleHistoryUnverified,row};
}
} // namespace Legality::Gen3OriginalWildSourceStage
