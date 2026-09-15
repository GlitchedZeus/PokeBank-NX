#include "Inventory/ClassicInventoryCatalog.h"

#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Names/ItemNames.h"
#include "Names/MoveNames.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <vector>

namespace PokeVault::Inventory {
namespace {

template <typename T>
bool contains(std::span<const T> values, T value) noexcept {
    return std::find(values.begin(), values.end(), value) != values.end();
}

template <std::size_t N>
std::span<const uint16_t> asSpan(const std::array<uint16_t, N>& values) noexcept {
    return {values.data(), values.size()};
}

std::span<const uint16_t> asSpan(const std::vector<uint16_t>& values) noexcept {
    return {values.data(), values.size()};
}

constexpr std::array<ClassicPocket, 6> kGen1Pockets{{
    ClassicPocket::Items, ClassicPocket::Medicines, ClassicPocket::Balls,
    ClassicPocket::KeyItems, ClassicPocket::TMHM, ClassicPocket::PCItems,
}};
constexpr std::array<ClassicPocket, 5> kGen2Pockets{{
    ClassicPocket::Items, ClassicPocket::Balls, ClassicPocket::KeyItems,
    ClassicPocket::TMHM, ClassicPocket::PCItems,
}};
constexpr std::array<ClassicPocket, 6> kGen3Pockets{{
    ClassicPocket::Items, ClassicPocket::KeyItems, ClassicPocket::Balls,
    ClassicPocket::TMHM, ClassicPocket::Berries, ClassicPocket::PCItems,
}};

constexpr std::array<uint16_t, 18> kG1Items{{
    10,29,30,32,33,34,46,47,49,51,55,56,57,58,65,66,67,68,
}};
constexpr std::array<uint16_t, 27> kG1Medicines{{
    11,12,13,14,15,16,17,18,19,20,35,36,37,38,39,40,50,52,53,54,60,61,62,79,80,81,82,
}};
constexpr std::array<uint16_t, 4> kG1Balls{{1,2,3,4}};
constexpr std::array<uint16_t, 20> kG1Keys{{
    5,6,31,41,42,43,45,48,63,64,69,70,71,72,73,74,75,76,77,78,
}};
constexpr std::array<uint16_t, 55> kG1Machines{{
    196,197,198,199,200,
    201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
    216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,
    231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,
    246,247,248,249,250,
}};
constexpr std::array<uint16_t, 124> kG1PC{{
    1,2,3,4,5,6,10,11,12,13,14,15,16,17,18,19,20,
    29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,45,46,47,48,49,51,52,53,54,55,56,57,58,
    60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,
    196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,
    218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
    240,241,242,243,244,245,246,247,248,249,250,
}};

constexpr std::array<uint16_t, 131> kG2Items{{
    3,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,26,27,28,29,30,31,32,33,34,35,36,37,
    38,39,40,41,42,43,44,46,47,48,49,51,52,53,57,60,62,63,64,65,72,73,74,75,76,77,78,79,
    80,81,82,83,84,85,86,87,88,89,91,92,93,94,95,96,97,98,99,101,102,103,104,105,106,107,
    108,109,110,111,112,113,114,117,118,119,121,122,123,124,125,126,131,132,138,139,140,143,
    144,146,150,151,152,156,158,163,167,168,169,170,172,173,174,180,181,182,183,184,185,186,
    187,188,189,
}};
constexpr std::array<uint16_t, 11> kG2Balls{{1,2,4,5,157,159,160,161,164,165,166}};
constexpr std::array<uint16_t, 18> kG2KeysGS{{7,54,55,58,59,61,66,67,68,69,71,127,128,130,133,134,175,178}};
constexpr std::array<uint16_t, 22> kG2KeysC{{7,54,55,58,59,61,66,67,68,69,71,127,128,130,133,134,175,178,70,115,116,129}};
constexpr std::array<uint16_t, 57> kG2Machines{{
    191,192,193,194,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,
    214,215,216,217,218,219,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,
    237,238,239,240,241,242,243,244,245,246,247,248,249,
}};

constexpr std::array<uint16_t, 138> kG3Items{{
    13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,
    42,43,45,46,47,48,49,50,51,63,64,65,66,67,68,69,70,71,73,74,75,76,77,78,79,80,81,83,84,
    85,86,93,94,95,96,97,98,103,104,106,107,108,109,110,111,121,122,123,124,125,126,127,128,
    129,130,131,132,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,
    197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,
    219,220,221,222,223,224,225,254,255,256,257,258,
}};
constexpr std::array<uint16_t, 11> kG3Balls{{1,2,3,4,6,7,8,9,10,11,12}};
constexpr std::array<uint16_t, 43> kG3Berries{{
    133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,
    155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
}};
constexpr std::array<uint16_t, 58> kG3Machines{{
    289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,304,305,306,307,308,
    309,310,311,312,313,314,315,316,317,318,319,320,321,322,323,324,325,326,327,328,
    329,330,331,332,333,334,335,336,337,338,339,340,341,342,343,344,345,346,
}};
constexpr std::array<uint16_t, 29> kG3KeysRS{{
    259,260,261,262,263,264,265,266,268,269,270,271,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,287,288,
}};
constexpr std::array<uint16_t, 32> kG3KeysE{{
    259,260,261,262,263,264,265,266,268,269,270,271,272,273,274,275,278,279,280,281,282,283,284,285,286,287,288,370,371,372,375,376,
}};
constexpr std::array<uint16_t, 32> kG3KeysFRLG{{
    260,261,262,263,264,265,349,350,351,352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,368,369,370,371,372,373,374,
}};

constexpr std::array<uint16_t, 55> kG1MachineMoves{{
    15,19,57,70,148,
    5,13,14,18,25,92,32,34,36,38,61,55,58,59,63,6,66,68,69,99,72,76,82,85,87,89,90,91,
    94,100,102,104,115,117,118,120,121,126,129,130,135,138,143,156,86,149,153,157,161,164,
}};
constexpr std::array<uint16_t, 57> kG2MachineMoves{{
    223,29,174,205,46,92,192,249,244,237,241,230,173,59,63,196,182,240,202,203,
    218,76,231,225,87,89,216,91,94,247,189,104,8,207,214,188,201,126,129,111,
    9,138,197,156,213,168,211,7,210,171,15,19,57,70,148,250,127,
}};
constexpr std::array<uint16_t, 58> kG3MachineMoves{{
    264,337,352,347,46,92,258,339,331,526,241,269,58,59,63,113,182,240,202,219,218,76,231,85,
    87,89,216,91,94,247,280,104,115,351,53,188,201,126,317,332,259,263,521,156,213,168,211,285,
    503,315,15,19,57,70,148,249,127,431,
}};

template <std::size_t N>
void append(std::vector<uint16_t>& out, const std::array<uint16_t, N>& values) {
    out.insert(out.end(), values.begin(), values.end());
}

std::span<const uint16_t> gen2Pc(bool crystal) noexcept {
    static const std::vector<uint16_t> gs = [] {
        std::vector<uint16_t> out;
        out.reserve(kG2Items.size() + kG2Balls.size() + kG2Machines.size() + kG2KeysGS.size());
        append(out, kG2Items);
        append(out, kG2Balls);
        append(out, kG2Machines);
        append(out, kG2KeysGS);
        return out;
    }();
    static const std::vector<uint16_t> c = [] {
        std::vector<uint16_t> out;
        out.reserve(kG2Items.size() + kG2Balls.size() + kG2Machines.size() + kG2KeysC.size());
        append(out, kG2Items);
        append(out, kG2Balls);
        append(out, kG2Machines);
        append(out, kG2KeysC);
        return out;
    }();
    return crystal ? asSpan(c) : asSpan(gs);
}

std::span<const uint16_t> gen3Pc(ClassicGame game) noexcept {
    static const std::vector<uint16_t> rs = [] {
        std::vector<uint16_t> out;
        append(out, kG3Items);
        append(out, kG3KeysRS);
        append(out, kG3Berries);
        append(out, kG3Balls);
        append(out, kG3Machines);
        return out;
    }();
    static const std::vector<uint16_t> emerald = [] {
        std::vector<uint16_t> out;
        append(out, kG3Items);
        append(out, kG3Berries);
        append(out, kG3Balls);
        append(out, kG3Machines);
        return out;
    }();
    static const std::vector<uint16_t> frlg = [] {
        std::vector<uint16_t> out;
        append(out, kG3Items);
        return out;
    }();
    if (game == ClassicGame::Emerald) return asSpan(emerald);
    if (game == ClassicGame::FireRed || game == ClassicGame::LeafGreen) return asSpan(frlg);
    return asSpan(rs);
}

std::string machineLabel(const char* prefix, int number, int width, uint16_t move) {
    std::ostringstream out;
    out << prefix << std::setw(width) << std::setfill('0') << number << " — " << Names::getMoveName(move);
    return out.str();
}

} // namespace

std::optional<ClassicGame> classicGameFromId(std::string_view id) noexcept {
    if (id == "red_gb") return ClassicGame::Red;
    if (id == "blue_gb") return ClassicGame::Blue;
    if (id == "yellow_gb") return ClassicGame::Yellow;
    if (id == "gold_gbc") return ClassicGame::Gold;
    if (id == "silver_gbc") return ClassicGame::Silver;
    if (id == "crystal_gbc") return ClassicGame::Crystal;
    if (id == "ruby_gba") return ClassicGame::Ruby;
    if (id == "sapphire_gba") return ClassicGame::Sapphire;
    if (id == "emerald_gba") return ClassicGame::Emerald;
    if (id == "firered_gba") return ClassicGame::FireRed;
    if (id == "leafgreen_gba") return ClassicGame::LeafGreen;
    return std::nullopt;
}

std::string_view classicGameId(ClassicGame game) noexcept {
    switch (game) {
        case ClassicGame::Red: return "red_gb";
        case ClassicGame::Blue: return "blue_gb";
        case ClassicGame::Yellow: return "yellow_gb";
        case ClassicGame::Gold: return "gold_gbc";
        case ClassicGame::Silver: return "silver_gbc";
        case ClassicGame::Crystal: return "crystal_gbc";
        case ClassicGame::Ruby: return "ruby_gba";
        case ClassicGame::Sapphire: return "sapphire_gba";
        case ClassicGame::Emerald: return "emerald_gba";
        case ClassicGame::FireRed: return "firered_gba";
        case ClassicGame::LeafGreen: return "leafgreen_gba";
    }
    return {};
}

std::string_view pocketName(ClassicPocket pocket) noexcept {
    switch (pocket) {
        case ClassicPocket::Items: return "Items";
        case ClassicPocket::Medicines: return "Medicines";
        case ClassicPocket::Balls: return "Poké Balls";
        case ClassicPocket::KeyItems: return "Key Items";
        case ClassicPocket::TMHM: return "TM/HM";
        case ClassicPocket::Berries: return "Berries";
        case ClassicPocket::PCItems: return "PC Items";
    }
    return "Items";
}

std::span<const ClassicPocket> supportedPockets(ClassicGame game) noexcept {
    if (game <= ClassicGame::Yellow) return kGen1Pockets;
    if (game <= ClassicGame::Crystal) return kGen2Pockets;
    return kGen3Pockets;
}

std::span<const uint16_t> addableItems(ClassicGame game, ClassicPocket pocket) noexcept {
    if (game <= ClassicGame::Yellow) {
        switch (pocket) {
            case ClassicPocket::Items: return asSpan(kG1Items);
            case ClassicPocket::Medicines: return asSpan(kG1Medicines);
            case ClassicPocket::Balls: return asSpan(kG1Balls);
            case ClassicPocket::KeyItems: return asSpan(kG1Keys);
            case ClassicPocket::TMHM: return asSpan(kG1Machines);
            case ClassicPocket::PCItems: return asSpan(kG1PC);
            default: return {};
        }
    }
    if (game <= ClassicGame::Crystal) {
        switch (pocket) {
            case ClassicPocket::Items: return asSpan(kG2Items);
            case ClassicPocket::Balls: return asSpan(kG2Balls);
            case ClassicPocket::KeyItems:
                return game == ClassicGame::Crystal ? asSpan(kG2KeysC) : asSpan(kG2KeysGS);
            case ClassicPocket::TMHM: return asSpan(kG2Machines);
            case ClassicPocket::PCItems: return gen2Pc(game == ClassicGame::Crystal);
            default: return {};
        }
    }
    switch (pocket) {
        case ClassicPocket::Items: return asSpan(kG3Items);
        case ClassicPocket::Balls: return asSpan(kG3Balls);
        case ClassicPocket::TMHM: return asSpan(kG3Machines);
        case ClassicPocket::Berries: return asSpan(kG3Berries);
        case ClassicPocket::KeyItems:
            if (game == ClassicGame::Emerald) return asSpan(kG3KeysE);
            if (game == ClassicGame::FireRed || game == ClassicGame::LeafGreen) return asSpan(kG3KeysFRLG);
            return asSpan(kG3KeysRS);
        case ClassicPocket::PCItems: return gen3Pc(game);
        default: return {};
    }
}

bool supportsPocket(ClassicGame game, ClassicPocket pocket) noexcept {
    return contains<ClassicPocket>(supportedPockets(game), pocket);
}

bool isAddableItem(ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept {
    return itemId != 0 && supportsPocket(game, pocket) && contains<uint16_t>(addableItems(game, pocket), itemId);
}

QuantityRule quantityRule(ClassicGame game, ClassicPocket pocket, uint16_t itemId) noexcept {
    if (!isAddableItem(game, pocket, itemId)) return {false, 0, 0};
    if (pocket == ClassicPocket::KeyItems) return {false, 1, 1};
    if (pocket == ClassicPocket::TMHM) {
        if (game <= ClassicGame::Yellow && itemId >= 196 && itemId <= 200) return {false, 1, 1};
        if (game >= ClassicGame::Gold && game <= ClassicGame::Crystal && itemId >= 243) return {false, 1, 1};
        if (game >= ClassicGame::Ruby && itemId >= 339) return {false, 1, 1};
    }
    return {true, 1, 99};
}

bool isStorySensitiveKeyItem(ClassicGame, ClassicPocket pocket, uint16_t) noexcept {
    return pocket == ClassicPocket::KeyItems;
}

uint16_t classicMachineMove(ClassicGame game, uint16_t itemId) noexcept {
    if (game <= ClassicGame::Yellow) {
        if (itemId < 196 || itemId > 250) return 0;
        return kG1MachineMoves[itemId - 196];
    }
    if (game <= ClassicGame::Crystal) {
        const auto it = std::find(kG2Machines.begin(), kG2Machines.end(), itemId);
        if (it == kG2Machines.end()) return 0;
        return kG2MachineMoves[static_cast<std::size_t>(it - kG2Machines.begin())];
    }
    if (itemId < 289 || itemId > 346) return 0;
    return kG3MachineMoves[itemId - 289];
}

std::string displayItemName(ClassicGame game, ClassicPocket pocket, uint16_t itemId) {
    const uint16_t move = classicMachineMove(game, itemId);
    // Generation II PC Items can contain machines too. If the exact id is a machine, present the
    // same polished TM/HM + move label regardless of which legitimate inventory category owns it.
    if (move != 0 && (pocket == ClassicPocket::TMHM ||
                      (game >= ClassicGame::Gold && game <= ClassicGame::Crystal &&
                       pocket == ClassicPocket::PCItems))) {
        if (game <= ClassicGame::Yellow) {
            if (itemId <= 200) return machineLabel("HM", itemId - 195, 2, move);
            return machineLabel("TM", itemId - 200, 2, move);
        }
        if (game <= ClassicGame::Crystal) {
            const auto it = std::find(kG2Machines.begin(), kG2Machines.end(), itemId);
            if (it != kG2Machines.end()) {
                const int index = static_cast<int>(it - kG2Machines.begin());
                if (index < 50) return machineLabel("TM", index + 1, 2, move);
                return machineLabel("HM", index - 49, 2, move);
            }
        } else {
            if (itemId <= 338) return machineLabel("TM", itemId - 288, 2, move);
            return machineLabel("HM", itemId - 338, 2, move);
        }
    }
    if (game <= ClassicGame::Yellow) return Names::getItemNameG1(itemId);
    if (game <= ClassicGame::Crystal) {
        const auto name = PokeVault::Integration::Gen2::gen2ItemDisplayName(static_cast<uint8_t>(itemId));
        return name.empty() ? ("Item " + std::to_string(itemId)) : name;
    }
    return Names::getItemNameG3(itemId);
}

} // namespace PokeVault::Inventory