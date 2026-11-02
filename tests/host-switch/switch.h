#pragma once
#include <cstdint>
// Host renderer only: distinct controller masks let the actual ScreenChrome
// publish and clear footer hit targets. The native build uses devkitPro/libnx.
enum : uint64_t {
    HidNpadButton_A=1ull<<0, HidNpadButton_B=1ull<<1,
    HidNpadButton_X=1ull<<2, HidNpadButton_Y=1ull<<3,
    HidNpadButton_L=1ull<<6, HidNpadButton_R=1ull<<7,
    HidNpadButton_ZL=1ull<<8, HidNpadButton_ZR=1ull<<9,
    HidNpadButton_Plus=1ull<<10, HidNpadButton_Minus=1ull<<11,
    HidNpadButton_Left=1ull<<12, HidNpadButton_Up=1ull<<13,
    HidNpadButton_Right=1ull<<14, HidNpadButton_Down=1ull<<15,
};
