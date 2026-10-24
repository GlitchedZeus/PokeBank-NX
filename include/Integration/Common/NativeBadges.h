#pragma once
#include <cstdint>
#include <string>
namespace PokeVault::Integration::Common {
struct NativeBadges {
    uint16_t mask=0;
    uint8_t total=0; // 0 means unavailable; games with two regions use 16.
    unsigned count() const noexcept {
        unsigned n=0;for(unsigned i=0;i<total && i<16;++i)n+=(mask>>i)&1u;return n;
    }
    std::string text() const {
        if(total==0)return "Unavailable";
        if(total==16) {
            unsigned johto=0,kanto=0;
            for(unsigned i=0;i<8;++i){johto+=(mask>>i)&1u;kanto+=(mask>>(i+8))&1u;}
            return "Johto "+std::to_string(johto)+"/8 / Kanto "+std::to_string(kanto)+"/8";
        }
        return std::to_string(count())+" / "+std::to_string(total);
    }
};
}
