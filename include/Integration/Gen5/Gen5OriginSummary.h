#ifndef POKEBANK_GEN5_ORIGIN_SUMMARY_H
#define POKEBANK_GEN5_ORIGIN_SUMMARY_H

#include "Pokemon/Pokemon5ReadOnly.h"

#include <cstdint>
#include <string>

namespace PokeVault::Integration::Gen5 {

// Read-only origin line for the shared PK5 View (Gen IV parity: met date,
// egg flag, egg date/location shown, never edited). Raw native values only;
// no location name or legality conclusion is inferred here.
[[nodiscard]] inline std::string nativeDate5(const Pokemon5ReadOnly::NativeDate5& d) {
    if(d.month==0 && d.day==0)return "none";
    const auto two=[](unsigned v) {
        return std::string(v<10?"0":"")+std::to_string(v);
    };
    return std::to_string(d.year)+"-"+two(d.month)+"-"+two(d.day);
}

[[nodiscard]] inline std::string originSummary5(const Pokemon5ReadOnly& pk) {
    std::string out="Language "+std::to_string(pk.language())+
        "  |  Origin game "+std::to_string(pk.originVersion())+
        "  |  Met "+nativeDate5(pk.metDate());
    if(pk.eggLocation()!=0 || pk.eggDate().month!=0)
        out+="  |  Egg "+nativeDate5(pk.eggDate())+" @ "+std::to_string(pk.eggLocation());
    if(pk.egg())out+="  |  Egg";
    return out;
}

} // namespace PokeVault::Integration::Gen5
#endif
