#pragma once

// R2-BUG-013: controller buttons named in help windows and modal instructions are drawn as the
// app's coloured controller badges (ScreenChrome::buttonGlyph: green A, red B, styled X / Y,
// L / R and ZL / ZR shoulders, +/-, D-pad + Left Stick), never as plain "A/B/X/Y" text.
//
// This header only decides WHERE the badges go: it splits an instruction string into text and
// button segments. Drawing is ScreenChrome::drawGlyphText (the one glyph renderer). Pure and
// host-tested (tests/test_glyph_instruction.cpp).
//
// Two grammars, both used by real strings in the app:
//  * helpLine  — help-window rows "KEYS   description", several rows per line allowed when
//    separated by a wide gap ("+   Options     -   Close help"). KEYS may be one button, a
//    pair ("L / R", "ZL / ZR", "X / Y") or "D-pad / Left Stick". A row whose key part is not a
//    known button (a sentence, a source banner) stays plain text.
//  * sentence  — instruction prose ("Press Y to delete; A or B keeps it.", "Y: Confirm  B: Keep").
//    A single-letter button becomes a badge only in an unambiguous position: after
//    press / hold / release / use / tap, after "or" / "and" / "/" that follows a badge, or as a
//    "K:" label at the start of a run. Multi-letter controls (L/R, ZL, ZR, D-pad, D-pad/Stick)
//    are unambiguous everywhere. An English article "A" is never turned into a badge.

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::UIModel::GlyphInstruction {

enum class Kind : unsigned char { Text, Glyph };

struct Segment {
    Kind kind = Kind::Text;
    std::string text;   // Text: the characters to draw. Glyph: a buttonGlyph token ("A", "L/R", ...).
    bool operator==(const Segment&) const = default;
};
using Segments = std::vector<Segment>;

// The buttonGlyph token for a written control name, or "" when it is not a control.
[[nodiscard]] inline std::string canonicalButton(std::string_view s) {
    std::string t;
    for (char c : s) if (c != ' ') t += c;            // "L / R" -> "L/R"
    if (t == "A" || t == "B" || t == "X" || t == "Y" || t == "L" || t == "R" ||
        t == "ZL" || t == "ZR" || t == "L/R" || t == "ZL/ZR" || t == "+" || t == "-")
        return t;
    if (t == "Plus") return "+";
    if (t == "Minus") return "-";
    if (t == "D-pad/LeftStick" || t == "D-pad/Stick" || t == "D-Pad/LeftStick" || t == "LeftStick" ||
        t == "D-pad" || t == "D-Pad" || t == "Arrows")
        return "D-pad/Stick";
    if (t == "Up/Down" || t == "Left/Right" || t == "Left" || t == "Right" || t == "Up" || t == "Down")
        return t;
    return {};
}

namespace detail {

inline void appendText(Segments& out, std::string_view s) {
    if (s.empty()) return;
    if (!out.empty() && out.back().kind == Kind::Text) out.back().text += s;
    else out.push_back({Kind::Text, std::string(s)});
}
inline void appendGlyph(Segments& out, std::string token) { out.push_back({Kind::Glyph, std::move(token)}); }

// The key part of one help row as badges: "X / Y" -> X "/" Y; "L / R" -> one L/R badge.
inline bool keyCluster(std::string_view keys, Segments& out) {
    if (const std::string whole = canonicalButton(keys); !whole.empty()) {
        appendGlyph(out, whole);
        return true;
    }
    Segments parts;
    std::size_t i = 0;
    while (i <= keys.size()) {
        const std::size_t slash = keys.find('/', i);
        std::string_view part = keys.substr(i, slash == std::string_view::npos ? std::string_view::npos : slash - i);
        while (!part.empty() && part.front() == ' ') part.remove_prefix(1);
        while (!part.empty() && part.back() == ' ') part.remove_suffix(1);
        const std::string token = canonicalButton(part);
        if (token.empty()) return false;
        if (!parts.empty()) appendText(parts, " / ");
        appendGlyph(parts, token);
        if (slash == std::string_view::npos) break;
        i = slash + 1;
    }
    for (auto& p : parts) out.push_back(std::move(p));
    return !parts.empty();
}

inline std::size_t spaceRun(std::string_view s, std::size_t from, std::size_t minRun) {
    for (std::size_t i = from; i < s.size(); ++i) {
        if (s[i] != ' ') continue;
        std::size_t j = i;
        while (j < s.size() && s[j] == ' ') ++j;
        if (j - i >= minRun) return i;
        i = j;
    }
    return std::string_view::npos;
}

inline std::string lower(std::string_view s) {
    std::string r(s);
    for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

} // namespace detail

// Help-window row(s): "KEYS   description" groups separated by 5+ spaces.
[[nodiscard]] inline Segments helpLine(std::string_view line) {
    Segments out;
    std::size_t i = 0;
    bool first = true;
    while (i < line.size()) {
        const std::size_t groupEnd = detail::spaceRun(line, i, 5);
        const std::string_view group = line.substr(i, groupEnd == std::string_view::npos ? std::string_view::npos : groupEnd - i);
        if (!first) detail::appendText(out, "     ");
        first = false;
        const std::size_t sep = detail::spaceRun(group, 0, 2);
        Segments keys;
        if (sep != std::string_view::npos && detail::keyCluster(group.substr(0, sep), keys)) {
            for (auto& k : keys) out.push_back(std::move(k));
            std::size_t d = sep;
            while (d < group.size() && group[d] == ' ') ++d;
            detail::appendText(out, "  ");
            detail::appendText(out, group.substr(d));
        } else {
            detail::appendText(out, group);
        }
        if (groupEnd == std::string_view::npos) break;
        i = groupEnd;
        while (i < line.size() && line[i] == ' ') ++i;
    }
    return out;
}

// Instruction prose with buttons named inline.
[[nodiscard]] inline Segments sentence(std::string_view text) {
    Segments out;
    std::string prevWord;           // lower-case previous word (with any trailing punctuation)
    bool prevWasGlyph = false;      // the previous word was a badge with no trailing punctuation
    bool joinAfterGlyph = false;    // the previous word is "or" / "and" and the one before was a badge
    bool runStart = true;           // start of text or after a 3+ space gap
    std::size_t i = 0;
    while (i < text.size()) {
        if (text[i] == ' ') {
            std::size_t j = i;
            while (j < text.size() && text[j] == ' ') ++j;
            detail::appendText(out, text.substr(i, j - i));
            if (j - i >= 3) { runStart = true; prevWord.clear(); prevWasGlyph = false; joinAfterGlyph = false; }
            i = j;
            continue;
        }
        std::size_t j = i;
        while (j < text.size() && text[j] != ' ') ++j;
        std::string_view word = text.substr(i, j - i);
        // Split trailing punctuation (",;.:)") from the word; a trailing ':' marks a "K:" label.
        std::size_t core = word.size();
        while (core > 0 && std::string_view(",;.:)!?").find(word[core - 1]) != std::string_view::npos) --core;
        const std::string_view stem = word.substr(0, core);
        const std::string_view tail = word.substr(core);
        const std::string token = canonicalButton(stem);
        // Only controls that can't be an English word are badges without context ("ZL", "L/R",
        // "D-pad", "Up/Down"); "A", "Left", "Up", "+" need a verb, a joiner or a "K:" label.
        const bool single = !(token == "L/R" || token == "ZL/ZR" || token == "ZL" || token == "ZR" ||
                              token == "Up/Down" || token == "Left/Right" ||
                              (token == "D-pad/Stick" && stem != "Arrows"));
        const bool label = !tail.empty() && tail.front() == ':' && runStart;
        const bool afterVerb = prevWord == "press" || prevWord == "hold" || prevWord == "release" ||
                               prevWord == "use" || prevWord == "tap";
        // Lookahead: "A or B keeps it" -> the first of a joined button pair is a badge too.
        bool pairLead = false;
        if (!token.empty() && tail.empty()) {
            std::size_t k = j;
            auto nextWord = [&](std::size_t& at) {
                while (at < text.size() && text[at] == ' ') ++at;
                const std::size_t b = at;
                while (at < text.size() && text[at] != ' ') ++at;
                return text.substr(b, at - b);
            };
            const std::string joiner = detail::lower(nextWord(k));
            std::string_view other = nextWord(k);
            while (!other.empty() && std::string_view(",;.:)!?").find(other.back()) != std::string_view::npos)
                other.remove_suffix(1);
            pairLead = (joiner == "or" || joiner == "and") && !canonicalButton(other).empty();
        }
        const bool glyph = !token.empty() && (!single || label || afterVerb || joinAfterGlyph || pairLead);
        const std::string lowered = detail::lower(stem);
        const bool wasGlyph = prevWasGlyph;
        Segments pair;   // "A/B: Cancel" -> A / B badges
        if (token.empty() && stem.find('/') != std::string_view::npos && stem.size() <= 7 &&
            detail::keyCluster(stem, pair)) {
            for (auto& p : pair) out.push_back(std::move(p));
            detail::appendText(out, runStart && !tail.empty() && tail.front() == ':' ? tail.substr(1) : tail);
            prevWasGlyph = tail.empty();
            joinAfterGlyph = false;
            prevWord = detail::lower(stem) + std::string(tail);
            runStart = false;
            i = j;
            continue;
        }
        if (glyph) {
            detail::appendGlyph(out, token);
            // A "K:" label drops its colon (the badge already reads as a key); other punctuation stays.
            detail::appendText(out, label ? tail.substr(1) : tail);
            prevWasGlyph = tail.empty();
        } else {
            detail::appendText(out, word);
            prevWasGlyph = false;
        }
        joinAfterGlyph = !glyph && wasGlyph && tail.empty() && (lowered == "or" || lowered == "and");
        prevWord = lowered + std::string(tail);   // "press," is not a verb before a badge
        runStart = false;
        i = j;
    }
    return out;
}

// Number of badges in a parse (host tests use it to prove every real instruction gets its badges).
[[nodiscard]] inline int glyphCount(const Segments& segments) {
    int n = 0;
    for (const auto& s : segments) n += s.kind == Kind::Glyph;
    return n;
}

} // namespace PokeBank::UIModel::GlyphInstruction
