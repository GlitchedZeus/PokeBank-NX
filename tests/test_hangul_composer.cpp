// Round 11 H27: Hangul composition for the in-app keyboard's Korean page (Utils/HangulComposer.h).
//   * the two-set rules: initial + vowel, final, double final, compound vowel, and a final moving to
//     the next syllable when a vowel follows (including half of a double final);
//   * every Gen IV Korean species name (Names::defaultSpeciesName(s, Korean, 4), 1-493) is typed
//     from its jamo keys and comes out exactly, and every character of it is in the Gen 4 character
//     table (Utils::charToGen4), so what the page builds is what the Gen IV writer can store;
//   * every key on the page is a jamo compose() handles.
#include "Utils/HangulComposer.h"
#include "Utils/Gen4Text.h"
#include "Names/DefaultSpeciesName.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace H = Utils::Hangul;

namespace {
// Types `keys` as one keyboard run, the way KeyboardState::typeHangul does.
std::u32string type(const std::u32string& keys) {
    std::u32string out;
    bool composing = false;
    for (const char32_t k : keys) {
        if (!H::isJamoKey(k)) { out.push_back(k); composing = false; continue; }
        if (composing && !out.empty()) {
            if (const auto c = H::compose(out.back(), k)) {
                out.pop_back();
                out += *c;
                continue;
            }
        }
        out.push_back(k);
        composing = true;
    }
    return out;
}
// The keys a two-set keyboard user presses for `text`.
std::u32string keysFor(const std::u32string& text) {
    std::u32string keys;
    const auto split = [&](char32_t c) {
        for (const auto& p : H::kDoubleFinals) if (p.combined == c) { keys += p.first; keys += p.second; return; }
        for (const auto& p : H::kCompoundVowels) if (p.combined == c) { keys += p.first; keys += p.second; return; }
        keys += c;
    };
    for (const char32_t c : text) {
        if (!H::isSyllable(c)) { keys += c; continue; }
        const int i = static_cast<int>(c - H::kSyllableBase);
        keys += H::kInitials[static_cast<size_t>(i / (H::kVowelCount * H::kFinalCount))];
        split(H::kFirstVowel + static_cast<char32_t>((i / H::kFinalCount) % H::kVowelCount));
        if (i % H::kFinalCount) split(H::kFinals[static_cast<size_t>(i % H::kFinalCount)]);
    }
    return keys;
}
std::u32string u32(const std::u16string& s) { return std::u32string(s.begin(), s.end()); }
}  // namespace

int main() {
    // Rules.
    assert(type(U"ㄱㅏ") == U"가");
    assert(type(U"ㄱㅏㄱ") == U"각");
    assert(type(U"ㄱㅏㄱㅅ") == U"갃");
    assert(type(U"ㄱㅏㄱㅏ") == U"가가");
    assert(type(U"ㄷㅏㄹㄱㅏ") == U"달가");
    assert(type(U"ㄷㅏㄹㄱ") == U"닭");
    assert(type(U"ㄱㅗㅏ") == U"과");
    assert(type(U"ㅇㅡㅣ") == U"의");
    assert(type(U"ㅗㅏ") == U"ㅘ");
    assert(type(U"ㄱㄱ") == U"ㄱㄱ");                 // two initials stay separate jamo
    assert(type(U"ㄱㅏㄸ") == U"가ㄸ");                // ㄸ is never a final
    assert(type(U"ㄱㅏㄸㅏ") == U"가따");
    assert(type(U"ㄱㅏ ㅏ") == U"가 ㅏ");              // a space ends the run
    assert(*H::compose(U'가', U'ㄲ') == U"갂");
    assert(!H::compose(U'A', U'ㅏ'));
    assert(!H::compose(0, U'ㄱ'));
    assert(H::encode(U"피카츄") == "\xED\x94\xBC\xEC\xB9\xB4\xEC\xB8\x84");
    assert(H::decodeOne("\xED\x94\xBC") == U'피');
    assert(H::decodeOne("A") == U'A' && H::decodeOne("") == 0 && H::decodeOne("AB") == 0);

    // Every key on the page composes.
    for (const auto& row : H::kRows)
        for (const std::string_view half : {row.lower, row.upper})
            for (size_t i = 0; i < half.size(); i += 3)
                assert(H::isJamoKey(H::decodeOne(half.substr(i, 3))));

    // Every Gen IV Korean species name types exactly and is storable in the Gen 4 table.
    int hangulNames = 0;
    for (uint16_t s = 1; s <= 493; ++s) {
        const std::u16string name = Names::defaultSpeciesName(s, 8, 4);
        assert(!name.empty());
        const std::u32string want = u32(name);
        const std::u32string got = type(keysFor(want));
        if (got != want) {
            std::cerr << "species " << s << " composes to " << H::encode(got) << " not " << H::encode(want) << "\n";
            assert(false);
        }
        for (const char16_t c : name) assert(Utils::charToGen4(c) != Utils::GEN4_TERMINATOR);
        for (const char32_t c : want) if (H::isSyllable(c)) { ++hangulNames; break; }
    }
    assert(hangulNames > 480);
    std::cout << "Hangul composition: two-set rules; all 493 Gen IV Korean species names type and store: PASS\n";
}
