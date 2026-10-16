/**
 * Gen4LegalityText.cpp - PK4 text decoding, species-name lookups and the word filter.
 *
 * Ports of PKHeX.Core (pinned in Gen4LegalityData.h): StringConverter4.LoadString,
 * SpeciesName.GetSpeciesNameGeneration / IsNicknamedAnyLanguage / TryGetSpeciesAnyLanguage
 * (Gen IV languages), StringConverter.HasEastAsianScriptCharacters, the digit count of
 * TrainerNameVerifier.ContainsTooManyNumbers, and WordFilter.IsFiltered for a Gen IV
 * string (WordFilterNX, then WordFilter3DS, with TextNormalizer and the species-name
 * exemptions). The filter patterns use a small regular-expression subset (literals,
 * ".", "^", "$", escapes, character classes, one level of "(a|b)" groups and the "*",
 * "+" and "?" quantifiers); the generator emits them as PKHeX loads them and the matcher
 * rejects anything outside the subset by treating that pattern as matching nothing,
 * which can only make the filter more lenient.
 */
#include "Gen4LegalityInternal.h"

#include <algorithm>
#include <unordered_map>

namespace Legality::Gen4 {

namespace {

char16_t decodeUnit(uint16_t value) noexcept {
    const D::CharMap* begin = D::kCharMap;
    const D::CharMap* end = D::kCharMap + D::kCharMapCount;
    const D::CharMap* it = std::lower_bound(begin, end, value,
                                            [](const D::CharMap& m, uint16_t v) { return m.value < v; });
    if (it == end || it->value != value) return u'￿';
    return static_cast<char16_t>(it->ch);
}

char16_t lowerInvariant(char16_t c) noexcept {
    if (c < 0x80) return (c >= u'A' && c <= u'Z') ? static_cast<char16_t>(c + 32) : c;
    const D::CharMap* begin = D::kLower;
    const D::CharMap* end = D::kLower + D::kLowerCount;
    const D::CharMap* it = std::lower_bound(begin, end, static_cast<uint16_t>(c),
                                            [](const D::CharMap& m, uint16_t v) { return m.value < v; });
    if (it == end || it->value != c) return c;
    return static_cast<char16_t>(it->ch);
}

bool inSorted(const uint16_t* begin, std::size_t count, char16_t c) noexcept {
    return std::binary_search(begin, begin + count, static_cast<uint16_t>(c));
}

bool lookupSpecies(const D::SpeciesKey* table, std::size_t count, std::u16string_view name, uint16_t& species) noexcept {
    const D::SpeciesKey* end = table + count;
    const D::SpeciesKey* it = std::lower_bound(table, end, name,
                                               [](const D::SpeciesKey& k, std::u16string_view v) {
                                                   return std::u16string_view(k.name) < v;
                                               });
    if (it == end || std::u16string_view(it->name) != name) return false;
    species = it->species;
    return true;
}

// ------------------------------------------------------------------ TextNormalizer
constexpr std::u16string_view kDakuten = u"ｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾊﾋﾌﾍﾎ";
constexpr std::u16string_view kHandakuten = u"ﾊﾋﾌﾍﾎ";
constexpr std::u16string_view kFullwidthKana =
    u"ヲァィゥェォャュョッーアイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワン";
constexpr std::u16string_view kSmallKana = u"ァィゥェォッャュョヮ";

std::u16string normalize(std::u16string_view input) {
    std::u16string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        char16_t c = input[i];
        if (c == u' ' || c == u'　' || c == u'ﾞ' || c == u'ﾟ') continue;
        uint16_t ofs = 0;
        if (c >= u'ｦ' && c <= u'ﾝ' && i + 1 < input.size()) {
            const char16_t d = input[i + 1];
            if (d == u'ﾞ' && kDakuten.find(c) != std::u16string_view::npos) ofs = 1;
            else if (d == u'ﾟ' && kHandakuten.find(c) != std::u16string_view::npos) ofs = 2;
            else if (d == u'ﾞ' && c == u'ｳ') ofs = u'ヴ' - u'ウ';
        }
        c = lowerInvariant(c);
        if (c >= u'ぁ' && c <= u'ゖ') c = static_cast<char16_t>(c + 0x60);
        else if ((c >= u'０' && c <= u'９') || (c >= u'ａ' && c <= u'ｚ')) c = static_cast<char16_t>(c - 0xFEE0);
        else if (c >= u'ｦ' && c <= u'ﾝ') c = static_cast<char16_t>(kFullwidthKana[static_cast<std::size_t>(c - u'ｦ')] + ofs);
        if (c >= u'ァ' && c <= u'ヶ') {
            if (kSmallKana.find(c) != std::u16string_view::npos) c = static_cast<char16_t>(c + 1);
            else if (c == u'ヵ') c = u'カ';
            else if (c == u'ヶ') c = u'ケ';
        }
        out.push_back(c);
    }
    return out;
}

// ------------------------------------------------------------------ regex subset
struct Atom {
    enum Kind : uint8_t { Lit, AnyChar, Class, Bol, Eol } kind = Lit;
    char quant = 0; // 0, '*', '+', '?'
    bool negate = false;
    char16_t ch = 0;
    std::vector<std::pair<char16_t, char16_t>> ranges;

    bool single(char16_t c) const noexcept {
        switch (kind) {
        case Lit: return c == ch;
        case AnyChar: return c != u'\n';
        case Class: {
            bool in = false;
            for (const auto& r : ranges) if (c >= r.first && c <= r.second) { in = true; break; }
            return in != negate;
        }
        default: return false;
        }
    }
};
using Seq = std::vector<Atom>;

bool matchAt(const Atom* p, const Atom* pe, std::u16string_view s, std::size_t i) {
    while (p != pe) {
        if (p->kind == Atom::Bol) { if (i != 0) return false; ++p; continue; }
        if (p->kind == Atom::Eol) {
            if (!(i == s.size() || (i + 1 == s.size() && s[i] == u'\n'))) return false;
            ++p;
            continue;
        }
        if (p->quant == '*' || p->quant == '+') {
            std::size_t j = i;
            while (j < s.size() && p->single(s[j])) ++j;
            const std::size_t min = p->quant == '+' ? i + 1 : i;
            if (j < min) return false;
            for (;;) {
                if (matchAt(p + 1, pe, s, j)) return true;
                if (j == min) return false;
                --j;
            }
        }
        if (p->quant == '?') {
            if (i < s.size() && p->single(s[i]) && matchAt(p + 1, pe, s, i + 1)) return true;
            ++p;
            continue;
        }
        if (i < s.size() && p->single(s[i])) { ++i; ++p; continue; }
        return false;
    }
    return true;
}

/// Parses one atom at pattern[i]; returns false when the syntax is outside the subset.
bool parseAtom(std::u16string_view pat, std::size_t& i, Atom& a) {
    const char16_t c = pat[i];
    if (c == u'\\') {
        if (i + 1 >= pat.size()) return false;
        a.kind = Atom::Lit;
        a.ch = pat[i + 1];
        i += 2;
        return true;
    }
    if (c == u'.') { a.kind = Atom::AnyChar; ++i; return true; }
    if (c == u'^') { a.kind = Atom::Bol; ++i; return true; }
    if (c == u'$') { a.kind = Atom::Eol; ++i; return true; }
    if (c == u'[') {
        a.kind = Atom::Class;
        ++i;
        if (i < pat.size() && pat[i] == u'^') { a.negate = true; ++i; }
        bool first = true;
        while (i < pat.size() && (pat[i] != u']' || first)) {
            first = false;
            char16_t lo = pat[i];
            if (lo == u'\\') { if (i + 1 >= pat.size()) return false; lo = pat[i + 1]; i += 2; }
            else ++i;
            char16_t hi = lo;
            if (i + 1 < pat.size() && pat[i] == u'-' && pat[i + 1] != u']') {
                hi = pat[i + 1];
                if (hi == u'\\') return false;
                i += 2;
            }
            a.ranges.emplace_back(lo, hi);
        }
        if (i >= pat.size()) return false;
        ++i; // ]
        return true;
    }
    if (c == u'(' || c == u')' || c == u'|' || c == u'*' || c == u'+' || c == u'?' || c == u'{' || c == u'}') return false;
    a.kind = Atom::Lit;
    a.ch = c;
    ++i;
    return true;
}

bool parseQuant(std::u16string_view pat, std::size_t& i, Atom& a) {
    if (i < pat.size() && (pat[i] == u'*' || pat[i] == u'+' || pat[i] == u'?')) {
        if (a.kind == Atom::Bol || a.kind == Atom::Eol) return false;
        a.quant = static_cast<char>(pat[i]);
        ++i;
        if (i < pat.size() && (pat[i] == u'*' || pat[i] == u'+' || pat[i] == u'?' || pat[i] == u'{')) return false;
    }
    return true;
}

/// Expands one pattern into flat alternatives (groups are one level deep, without quantifiers).
bool compile(std::u16string_view pat, std::vector<Seq>& out) {
    std::vector<Seq> partial(1);
    std::size_t i = 0;
    while (i < pat.size()) {
        if (pat[i] == u'(') {
            ++i;
            std::vector<Seq> alts(1);
            while (i < pat.size() && pat[i] != u')') {
                if (pat[i] == u'|') { alts.emplace_back(); ++i; continue; }
                Atom a;
                if (!parseAtom(pat, i, a) || !parseQuant(pat, i, a)) return false;
                alts.back().push_back(std::move(a));
            }
            if (i >= pat.size()) return false;
            ++i; // )
            if (i < pat.size() && (pat[i] == u'*' || pat[i] == u'+' || pat[i] == u'?' || pat[i] == u'{')) return false;
            std::vector<Seq> next;
            next.reserve(partial.size() * alts.size());
            for (const Seq& p : partial)
                for (const Seq& alt : alts) {
                    Seq s = p;
                    s.insert(s.end(), alt.begin(), alt.end());
                    next.push_back(std::move(s));
                }
            partial = std::move(next);
            continue;
        }
        Atom a;
        if (!parseAtom(pat, i, a) || !parseQuant(pat, i, a)) return false;
        for (Seq& p : partial) p.push_back(a);
    }
    for (Seq& p : partial) out.push_back(std::move(p));
    return true;
}

/// One compiled alternative with its prefilter: the longest run of required literal chars.
struct Compiled {
    Seq seq;
    std::u16string literal;
    bool leadingAnyStar = false;
};

struct Filter {
    std::vector<Compiled> patterns;
    std::unordered_map<char16_t, std::vector<uint32_t>> byChar; // literal's first char -> patterns
    std::vector<uint32_t> always;                               // no required literal

    void add(std::u16string_view pat) {
        std::vector<Seq> alts;
        if (!compile(pat, alts)) return; // outside the subset: never matches (lenient)
        for (Seq& s : alts) {
            Compiled c;
            std::u16string run;
            for (const Atom& a : s) {
                if (a.kind == Atom::Lit && (a.quant == 0 || a.quant == '+')) {
                    run.push_back(a.ch);
                    if (a.quant == '+') { if (run.size() > c.literal.size()) c.literal = run; run.clear(); }
                    continue;
                }
                if (run.size() > c.literal.size()) c.literal = run;
                run.clear();
            }
            if (run.size() > c.literal.size()) c.literal = run;
            c.leadingAnyStar = !s.empty() && s[0].kind == Atom::AnyChar && s[0].quant == '*';
            c.seq = std::move(s);
            const auto index = static_cast<uint32_t>(patterns.size());
            if (c.literal.empty()) always.push_back(index);
            else byChar[c.literal[0]].push_back(index);
            patterns.push_back(std::move(c));
        }
    }

    bool matches(const Compiled& c, std::u16string_view s) const {
        const Atom* b = c.seq.data();
        const Atom* e = b + c.seq.size();
        if (c.leadingAnyStar || (b != e && b->kind == Atom::Bol)) return matchAt(b, e, s, 0);
        for (std::size_t start = 0; start <= s.size(); ++start)
            if (matchAt(b, e, s, start)) return true;
        return false;
    }

    bool isMatch(std::u16string_view clean) const {
        for (uint32_t i : always) if (matches(patterns[i], clean)) return true;
        std::u16string seen;
        for (char16_t ch : clean) {
            if (seen.find(ch) != std::u16string::npos) continue;
            seen.push_back(ch);
            const auto it = byChar.find(ch);
            if (it == byChar.end()) continue;
            for (uint32_t i : it->second) {
                const Compiled& c = patterns[i];
                if (clean.find(c.literal) == std::u16string_view::npos) continue;
                if (matches(c, clean)) return true;
            }
        }
        return false;
    }
};

const Filter& filterNX() {
    static const Filter f = [] {
        Filter r;
        for (std::size_t i = 0; i < D::kWordFilterNXCount; ++i) r.add(D::kWordFilterNX[i]);
        return r;
    }();
    return f;
}

const Filter& filter3DS() {
    static const Filter f = [] {
        Filter r;
        for (std::size_t i = 0; i < D::kWordFilter3DSCount; ++i) r.add(D::kWordFilter3DS[i]);
        return r;
    }();
    return f;
}

bool isWhiteSpaceOnly(std::u16string_view s) noexcept {
    for (char16_t c : s) {
        const bool ws = (c >= 0x09 && c <= 0x0D) || c == 0x20 || c == 0x85 || c == 0xA0 || c == 0x1680 ||
                        (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F ||
                        c == 0x205F || c == 0x3000;
        if (!ws) return false;
    }
    return true;
}

} // namespace

std::u16string decodeText(std::span<const uint16_t> raw) {
    std::u16string out;
    for (uint16_t v : raw) {
        if (v == 0xFFFF) break;
        out.push_back(decodeUnit(v));
    }
    return out;
}

std::u16string nicknameText(const Pk4& pk) { return decodeText(pk.nickname); }
std::u16string otText(const Pk4& pk) { return decodeText(pk.ot); }

bool decodeEqualOT(const Pk4& a, const Pk4& b) noexcept {
    for (std::size_t i = 0; i < a.ot.size(); ++i) {
        const bool endA = a.ot[i] == 0xFFFF;
        const bool endB = b.ot[i] == 0xFFFF;
        if (endA || endB) return endA == endB;
        if (decodeUnit(a.ot[i]) != decodeUnit(b.ot[i])) return false;
    }
    return true;
}

std::u16string_view speciesName4(uint16_t species, uint8_t language) noexcept {
    if (species > kMaxSpecies4 || language > 8) return {};
    return D::kSpeciesNames4[static_cast<std::size_t>(language) * 494 + species];
}

bool isNicknamedAnyLanguage4(uint16_t species, std::u16string_view nickname) noexcept {
    static constexpr uint8_t kLanguages[] = {1, 2, 3, 4, 5, 7, 8};
    for (uint8_t lang : kLanguages)
        if (nickname == speciesName4(species, lang)) return false;
    return true;
}

bool tryGetSpeciesAnyLanguage4(std::u16string_view name, uint16_t& species) noexcept {
    return lookupSpecies(D::kSpeciesGen4, D::kSpeciesGen4Count, name, species);
}

bool hasEastAsianScript(std::u16string_view s) noexcept {
    for (char16_t c : s) if (c >= 0x4E00 && c <= 0x9FFF) return true;
    return false;
}

int numberCount(std::u16string_view s) noexcept {
    int n = 0;
    for (char16_t c : s) if (inSorted(D::kNumberChars, D::kNumberCharCount, c)) ++n;
    return n;
}

bool isWordFiltered4(std::u16string_view message) { return isWordFiltered(message, 4); }

bool isWordFiltered(std::u16string_view message, uint8_t originalGeneration) {
    if (message.size() <= 1 || isWhiteSpaceOnly(message)) return false;
    std::u16string lower;
    lower.reserve(message.size());
    for (char16_t c : message) lower.push_back(lowerInvariant(c));
    const std::u16string clean = normalize(message);
    uint16_t species = 0;
    // WordFilterNX.IsFiltered(message, original): a species name of the original generation (any language,
    // case-insensitive, up to that generation's max species) is exempt.
    const bool nxExempt = originalGeneration == 3
        ? lookupSpecies(D::kSpeciesLowerGen3, D::kSpeciesLowerGen3Count, lower, species) && species <= 386
        : lookupSpecies(D::kSpeciesLowerGen4, D::kSpeciesLowerGen4Count, lower, species) && species <= kMaxSpecies4;
    if (!nxExempt && filterNX().isMatch(clean)) return true;
    // WordFilter3DS.IsFiltered(message, original Gen4) -> IsSpeciesNameGen7 (case-insensitive, <= 807).
    const bool dsExempt = lookupSpecies(D::kSpeciesLowerGen7, D::kSpeciesLowerGen7Count, lower, species) &&
                          species <= 807;
    if (!dsExempt && filter3DS().isMatch(clean)) return true;
    return false;
}

} // namespace Legality::Gen4
