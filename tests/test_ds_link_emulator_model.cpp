#include "UI/DSLinkEmulatorModel.h"

#include <cassert>
#include <string_view>

namespace Link = PokeBank::UIModel::DSLinkEmulator;

static_assert(Link::Choices.size() == 5);
static_assert(Link::Choices[0].title == "melonDS");
static_assert(Link::Choices[0].providerFilter == "melonDS");
static_assert(Link::Choices[4].manual);
static_assert(Link::wrapIndex(0, -1) == 4);
static_assert(Link::wrapIndex(4, 1) == 0);
static_assert(Link::wrapIndex(1, 1) == 2);
static_assert(Link::acceptsProvider("melonDS", "melonDS"));
static_assert(!Link::acceptsProvider("DraStic", "melonDS"));
static_assert(Link::acceptsProvider("DraStic Savestate", "DraStic"));
static_assert(Link::acceptsProvider("RetroArch", "RetroArch"));
static_assert(!Link::acceptsProvider("RetroArch", "DraStic"));
static_assert(Link::acceptsProvider("Manual", ""));

int main() {
    // Provider selection changes only which candidates are discovered.
    // Assignment must still use the shared Save Instances verification path.
    for (int i = 0; i < static_cast<int>(Link::Choices.size()); ++i) {
        assert(Link::wrapIndex(i, static_cast<int>(Link::Choices.size())) == i);
        assert(Link::wrapIndex(i, -static_cast<int>(Link::Choices.size())) == i);
    }
}
