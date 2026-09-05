#include <cassert>

#include "UI/SpriteLayout.h"

int main() {
    using PokeBank::UIModel::containSprite;

    const auto square = containSprite(10, 20, 120, 120, 512, 512);
    assert(square.x == 10 && square.y == 20);
    assert(square.width == 120 && square.height == 120);

    const auto wide = containSprite(10, 20, 120, 120, 512, 256);
    assert(wide.x == 10 && wide.y == 50);
    assert(wide.width == 120 && wide.height == 60);

    const auto tall = containSprite(10, 20, 120, 120, 256, 512);
    assert(tall.x == 40 && tall.y == 20);
    assert(tall.width == 60 && tall.height == 120);

    const auto invalid = containSprite(10, 20, 120, 120, 0, 512);
    assert(invalid.width == 0 && invalid.height == 0);

    // Repeated calls cannot drift, bob, or change dimensions because layout has no time input.
    assert(containSprite(3, 7, 151, 149, 475, 380).x ==
           containSprite(3, 7, 151, 149, 475, 380).x);
    assert(containSprite(3, 7, 151, 149, 475, 380).height ==
           containSprite(3, 7, 151, 149, 475, 380).height);
}
