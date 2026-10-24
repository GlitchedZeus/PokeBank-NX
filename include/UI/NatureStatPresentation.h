#pragma once
#include "UI/NatureStatEffect.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include <string>

namespace UI::NatureStats {
inline Color statColor(unsigned nature, unsigned row, Color normal) {
    const int modifier = PokeBank::UIModel::NatureStatEffect::modifier(nature, row);
    return modifier > 0 ? Colors::Success : modifier < 0 ? Colors::Error : normal;
}
inline void drawEffect(PKSEFramebuffer& fb, int x, int y, unsigned nature) {
    namespace E = PokeBank::UIModel::NatureStatEffect;
    if (nature >= 25) return;
    if (E::neutral(nature)) {
        fb.drawText(x, y, "Neutral (no stat change)", Colors::TextDim, TextStyle::Caption);
        return;
    }
    fb.drawText(x, y, std::string("+10% ") + E::statName(nature / 5 + 1), Colors::Success, TextStyle::Caption);
    fb.drawText(x + 110, y, std::string("-10% ") + E::statName(nature % 5 + 1), Colors::Error, TextStyle::Caption);
}
} // namespace UI::NatureStats
