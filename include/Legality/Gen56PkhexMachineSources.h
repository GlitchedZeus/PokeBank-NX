#pragma once
// L3 Gen V/VI PKHeX-first machine-source slice; GPL-3.0-or-later-derived
// numerical mapping, retained provenance and obligations in Phase A docs.
// Source: kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e
// PKHeX.Core/PersonalInfo/Info/PersonalInfo5BW.cs (MachineMoves)
// PKHeX.Core/Legality/LearnSource/Sources/LearnSource{5BW,5B2W2,6XY,6AO}.cs
// Note: this maps machine item -> move, NOT species compatibility or possession.
#include <array>
#include <cstdint>
#include <span>
namespace Legality::Gen56PkhexMachineSources {
enum class Game : std::uint8_t {
 Unknown, Diamond, Pearl, Platinum, HeartGold, SoulSilver,
 Black, White, Black2, White2, X, Y, OmegaRuby, AlphaSapphire
};
constexpr std::uint8_t generation(Game g) noexcept {
 if(g>=Game::Diamond&&g<=Game::SoulSilver)return 4;
 if(g>=Game::Black&&g<=Game::White2)return 5;
 if(g>=Game::X&&g<=Game::AlphaSapphire)return 6;
 return 0;
}
constexpr std::uint16_t maxMove(Game g) noexcept {
 switch(generation(g)){case 4:return 467;case 5:return 559;case 6:return 621;default:return 0;}
}
constexpr std::uint16_t maxSpecies(Game g) noexcept {
 switch(generation(g)){case 4:return 493;case 5:return 649;case 6:return 721;default:return 0;}
}
// Machine bit indices from PKHeX PersonalInfo5BW and LearnSource6XY/AO.
// Array is TM01 onwards, followed by HM01 onwards. No cross-game HM index reuse.
inline constexpr std::array<std::uint16_t,101> kGen5 = {{
468,337,473,347,46,92,258,339,474,237,241,269,58,59,63,113,182,240,477,219,
218,76,479,85,87,89,216,91,94,247,280,104,115,482,53,188,201,126,317,332,
259,263,488,156,213,168,490,496,497,315,502,411,412,206,503,374,451,507,510,511,
261,512,373,153,421,371,514,416,397,148,444,521,86,360,14,522,244,523,524,157,
404,525,526,398,138,447,207,365,369,164,430,433,528,249,555,
15,19,57,70,127,291
}};
inline constexpr std::array<std::uint16_t,105> kXY = {{
468,337,473,347,46,92,258,339,474,237,241,269,58,59,63,113,182,240,355,219,
218,76,479,85,87,89,216,91,94,247,280,104,115,482,53,188,201,126,317,332,
259,263,488,156,213,168,490,496,497,315,211,411,412,206,503,374,451,507,510,511,
261,512,373,153,421,371,514,416,397,148,444,521,86,360,14,522,244,523,524,157,
404,525,611,398,138,447,207,214,369,164,430,433,528,249,555,267,399,612,605,590,
15,19,57,70,127
}};
inline constexpr std::array<std::uint16_t,107> kORAS = {{
468,337,473,347,46,92,258,339,474,237,241,269,58,59,63,113,182,240,355,219,
218,76,479,85,87,89,216,91,94,247,280,104,115,482,53,188,201,126,317,332,
259,263,488,156,213,168,490,496,497,315,211,411,412,206,503,374,451,507,510,511,
261,512,373,153,421,371,514,416,397,148,444,521,86,360,14,522,244,523,524,157,
404,525,611,398,138,447,207,214,369,164,430,433,528,290,555,267,399,612,605,590,
15,19,57,70,127,249,291
}};
// A structurally proved machine is still not proof it is compatible with a
// particular species/form. Missing species compatibility is always Unknown.
enum class Status : std::uint8_t { Unknown, Matches, Contradicts };
constexpr std::span<const std::uint16_t> machineTable(Game g) noexcept {
 if(g>=Game::Black&&g<=Game::White2)return kGen5;
 if(g==Game::X||g==Game::Y)return kXY;
 if(g==Game::OmegaRuby||g==Game::AlphaSapphire)return kORAS;
 return {};
}
constexpr std::uint8_t machineTMs(Game g) noexcept {
 return generation(g)==5?95:generation(g)==6?100:0;
}
constexpr std::uint8_t machineHMs(Game g) noexcept {
 return generation(g)==5?6:generation(g)==6?(g==Game::X||g==Game::Y?5:7):0;
}
// Use exact per-title map. BW TM95 is source-verified unavailable (Lock Capsule).
constexpr Status machine(Game g, bool hm, std::uint8_t number, std::uint16_t move) noexcept {
 if(!move || !machineTMs(g) || !number || number>(hm?machineHMs(g):machineTMs(g)))return Status::Unknown;
 if(!hm && number==95 && (g==Game::Black||g==Game::White))return Status::Contradicts;
 const auto table=machineTable(g);
 const auto index=static_cast<std::size_t>(hm?machineTMs(g)+number-1:number-1);
 if(index>=table.size())return Status::Unknown;
 return table[index]==move?Status::Matches:Status::Contradicts;
}
static_assert(kGen5.size()==101&&kXY.size()==105&&kORAS.size()==107);
static_assert(machine(Game::Black,false,95,555)==Status::Contradicts);
static_assert(machine(Game::Black2,false,95,555)==Status::Matches);
static_assert(machine(Game::X,false,95,555)==Status::Matches);
static_assert(machine(Game::X,true,6,249)==Status::Unknown);
static_assert(machine(Game::OmegaRuby,true,6,249)==Status::Matches);
static_assert(machine(Game::X,false,94,249)==Status::Matches);
static_assert(machine(Game::OmegaRuby,false,94,290)==Status::Matches);
static_assert(machine(Game::Y,true,5,127)==Status::Matches);
} // namespace Legality::Gen56PkhexMachineSources
