// Round 11 H27: Korean names drew as "[][][][][]" because only the console's Standard (Japanese)
// shared font was a fallback. UI::SystemFontFallbacks is the selection rule PKSEFramebuffer runs:
//   * Standard is registered before Korean, so kana / kanji keep their Japanese shapes and the
//     Korean font only adds Hangul;
//   * each console font is independent: if one fails to load the other is still registered, and
//     when both fail nothing is registered (the app draws as it did before);
//   * the face names are distinct and the fallbacks fit fontstash's table.
#include "UI/SystemFontFallbacks.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace F = UI::SystemFontFallbacks;

namespace {
struct Run {
    std::vector<F::SharedFont> asked;
    std::vector<int> added;
    F::Registered result;
};
Run run(bool standardLoads, bool koreanLoads) {
    Run r;
    int next = 10;
    r.result = F::registerAll(
        [&](F::SharedFont font, const char* face) {
            r.asked.push_back(font);
            assert(face && *face);
            const bool ok = font == F::SharedFont::Standard ? standardLoads : koreanLoads;
            return ok ? next++ : -1;
        },
        [&](int id) { r.added.push_back(id); });
    return r;
}
}  // namespace

int main() {
    static_assert(F::kOrder.size() == 2);
    assert(F::kOrder[0].font == F::SharedFont::Standard);
    assert(F::kOrder[1].font == F::SharedFont::Korean);
    assert(std::strcmp(F::kOrder[0].face, "jp") == 0 && std::strcmp(F::kOrder[1].face, "ko") == 0);
    assert(std::strcmp(F::kOrder[0].face, F::kOrder[1].face) != 0);
    assert(F::kRomfsFallbacks + F::kOrder.size() <= F::kFontstashMaxFallbacks);

    {   // Both load: Standard first, Korean last.
        const auto r = run(true, true);
        assert((r.asked == std::vector<F::SharedFont>{F::SharedFont::Standard, F::SharedFont::Korean}));
        assert((r.added == std::vector<int>{10, 11}));
        assert(r.result.japanese == 10 && r.result.korean == 11);
    }
    {   // Korean unavailable: exactly the pre-Round-11 behaviour.
        const auto r = run(true, false);
        assert((r.added == std::vector<int>{10}));
        assert(r.result.japanese == 10 && r.result.korean == -1);
    }
    {   // Standard unavailable: Hangul still gets its font.
        const auto r = run(false, true);
        assert(r.asked.size() == 2);
        assert((r.added == std::vector<int>{10}));
        assert(r.result.japanese == -1 && r.result.korean == 10);
    }
    {   // Neither: nothing registered.
        const auto r = run(false, false);
        assert(r.added.empty() && r.result.japanese == -1 && r.result.korean == -1);
    }
    std::cout << "System font fallbacks: Standard then Korean, each independent: PASS\n";
}
