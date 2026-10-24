#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace PokeVault::Integration::Common {
// Read-only native timer information. Invalid minute/second bytes are
// unavailable, never normalized into an invented amount of play time.
struct NativePlayTime {
    uint16_t hours=0;
    uint8_t minutes=0,seconds=0;
    bool maximum=false; // Gen I's separately stored timer saturation flag.
};
inline std::optional<NativePlayTime> nativePlayTime(uint16_t hours,uint8_t minutes,
                                                   uint8_t seconds,bool maximum=false) {
    if(minutes>59 || seconds>59)return std::nullopt;
    return NativePlayTime{hours,minutes,seconds,maximum};
}
inline std::string playTimeText(const std::optional<NativePlayTime>& value) {
    if(!value)return "Unavailable";
    const auto two=[](unsigned v){return (v<10?"0":"")+std::to_string(v);};
    return std::to_string(value->hours)+":"+two(value->minutes)+":"+two(value->seconds)+
        (value->maximum?"+":"");
}
}
