#include "UI/LinkEmulatorModel.h"

#include <cassert>
using namespace PokeBank::UIModel::LinkEmulator;

static_assert(choices(Platform::GameBoy).size() == 5);
static_assert(choices(Platform::GameBoyColor).size() == 5);
static_assert(choices(Platform::GameBoyAdvance).size() == 5);
static_assert(choices(Platform::NintendoDS).size() == 6);
static_assert(choices(Platform::Nintendo3DS).size() == 5);
static_assert(choices(Platform::NintendoSwitch).empty());
static_assert(nativeSwitch(Platform::NintendoSwitch));
static_assert(!nativeSwitch(Platform::NintendoSwitch, true));
static_assert(effectivePlatform(Platform::NintendoSwitch, true) == Platform::GameBoyAdvance);
static_assert(choices(Platform::NintendoDS)[1].provider == "melonDS");
static_assert(choices(Platform::Nintendo3DS)[3].manualFile);
static_assert(wrapIndex(0, -1, choices(Platform::NintendoDS).size()) == 5);
static_assert(wrapIndex(5, 1, choices(Platform::NintendoDS).size()) == 0);
static_assert(providerMatches("melonDS", "melonDS"));
static_assert(!providerMatches("DraStic", "melonDS"));
static_assert(providerMatches("DraStic Savestate", "DraStic"));
static_assert(providerMatches("RetroArch", ""));
static_assert(providerMatches("RetroArch/DeSmuME", "RetroArch"));
static_assert(!providerMatches("RetroArch", "Checkpoint"));

int main() {
    for (auto platform : {Platform::GameBoy, Platform::GameBoyColor,
                          Platform::GameBoyAdvance, Platform::NintendoDS,
                          Platform::Nintendo3DS}) {
        auto opts = choices(platform);
        assert(!opts.empty() && opts.front().provider.empty());
        for (int i = 0; i < static_cast<int>(opts.size()); ++i) {
            assert(wrapIndex(i, static_cast<int>(opts.size()), opts.size()) == i);
            assert(wrapIndex(i, -static_cast<int>(opts.size()), opts.size()) == i);
        }
    }
}
