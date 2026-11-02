#pragma once
// Gen V-IX integration boundary. A modern save/provider label must not be
// mistaken for an authenticated PKM storage format or acquisition history.
//
// Current project has native entity implementations for LGPE, SWSH, BDSP,
// PLA, SV and Z-A, but NOT yet PK5, PK6, or the other original PK7 families.
// Only explicitly implemented storage groups are classified below.
// Source-specific encounter, move, event, breeding and transfer history
// remains Unknown until independently implemented and corroborated.
#include "Enums/GameVersion.h"
#include <cstdint>

namespace Legality::Gen59FormatDomain {

enum class Kind : uint8_t {
    Unknown,
    LetsGo,
    SwordShield,
    BrilliantDiamondShiningPearl,
    LegendsArceus,
    ScarletViolet,
    LegendsZA
};
struct Domain {
    Kind kind=Kind::Unknown;
    uint8_t generation=0;
    uint16_t maxNationalSpecies=0; // 0 = NO certified upper bound
    uint16_t maxMove=0;            // 0 = NO certified upper bound
    bool mintedStatNature=false;
    bool nativeLayoutSupported=false;
};

// The ceiling is an upper bound on possible National Dex identity in the
// *target storage game*, NOT its regional dex, and not a claim that any
// specific species is obtainable by any particular acquisition method.
// Gen IX Z-A is deliberately left unbounded: later DLC/patch species require
// independent source verification before a hard Invalid can be justified.
constexpr Domain classify(Enums::GameVersion group) noexcept {
    using G=Enums::GameVersion;
    switch(group) {
        case G::GG:
        case G::GP:
        case G::GE:
            return {Kind::LetsGo,7,809,0,false,true};
        case G::SWSH:
        case G::SW:
        case G::SH:
            return {Kind::SwordShield,8,898,0,true,true};
        case G::BDSP:
        case G::BD:
        case G::SP:
            return {Kind::BrilliantDiamondShiningPearl,8,493,0,true,true};
        case G::PLA:
            return {Kind::LegendsArceus,8,905,0,true,true};
        case G::SV:
        case G::SL:
        case G::VL:
            return {Kind::ScarletViolet,9,1025,0,true,true};
        case G::ZA:
            return {Kind::LegendsZA,9,0,0,true,true};
        default:
            return {}; // Gen V/VI and other PK7 formats are NOT implemented.
    }
}
constexpr uint8_t generationFromGroup(Enums::GameVersion group) noexcept {
    return classify(group).generation;
}
constexpr uint16_t maxSpecies(Enums::GameVersion group) noexcept {
    return classify(group).maxNationalSpecies;
}
constexpr uint16_t maxMove(Enums::GameVersion group) noexcept {
    return classify(group).maxMove;
}
constexpr bool impossibleSpecies(Enums::GameVersion group,
                                 uint16_t species) noexcept {
    const auto max=maxSpecies(group);
    return max!=0 && species>max;
}
constexpr bool impossibleMove(Enums::GameVersion group,
                              uint16_t move) noexcept {
    const auto max=maxMove(group);
    return max!=0 && move>max;
}
constexpr bool mintedStatNature(Enums::GameVersion group) noexcept {
    return classify(group).mintedStatNature;
}
constexpr bool supportedNativeLayout(Enums::GameVersion group) noexcept {
    return classify(group).nativeLayoutSupported;
}

static_assert(classify(Enums::GameVersion::GG).generation==7);
static_assert(classify(Enums::GameVersion::BDSP).maxNationalSpecies==493);
static_assert(classify(Enums::GameVersion::SWSH).generation==8);
static_assert(classify(Enums::GameVersion::SV).maxNationalSpecies==1025);
static_assert(classify(Enums::GameVersion::ZA).maxNationalSpecies==0);
static_assert(!supportedNativeLayout(Enums::GameVersion::Gen7B));
static_assert(!supportedNativeLayout(Enums::GameVersion::Gen8));
static_assert(!supportedNativeLayout(Enums::GameVersion::Gen9));
static_assert(!supportedNativeLayout(Enums::GameVersion::Any));
static_assert(!impossibleMove(Enums::GameVersion::SV,65535));
} // namespace Legality::Gen59FormatDomain
