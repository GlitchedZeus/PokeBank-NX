// Text rules for the Gen V legality analyzer; see include/Legality/Gen5Text.h.
// References are PKHeX.Core at the pinned commit: StringConverter5, SpeciesName,
// Language, TextNormalizer, WordFilter, WordFilterNX, WordFilter3DS, WordFilter5 and
// TrainerNameVerifier.
#include "Legality/Gen5Text.h"

#include <algorithm>
#include <array>
#include <unordered_map>
#include <vector>

#include "Legality/Gen5LegalityData.h"

namespace Legality::Gen5Text {

namespace D = Gen5Data;

// ---------------------------------------------------------------- character classes

namespace {

struct Range { char16_t lo, hi; };

// Kept small on purpose: printable ASCII, Latin-1 and Latin Extended-A, Cyrillic
// (used by the filter lists), general punctuation, number forms, arrows / math /
// technical symbols, enclosed alphanumerics, box drawing / shapes / misc symbols,
// CJK symbols and kana, Hangul compatibility jamo, CJK ideographs (incl. extension
// A), Hangul syllables, the private use area (Gen V game glyphs) and the half-width /
// full-width forms.
constexpr std::array<Range, 15> kDomain{{
    {0x0020, 0x007E}, {0x00A0, 0x017F}, {0x0400, 0x04FF}, {0x2010, 0x2027}, {0x2030, 0x205E},
    {0x2150, 0x218B}, {0x2190, 0x23FF}, {0x2460, 0x26FF}, {0x3000, 0x30FF},
    {0x3130, 0x318F}, {0x3400, 0x4DBF}, {0x4E00, 0x9FFF}, {0xAC00, 0xD7A3},
    {0xE000, 0xF8FF}, {0xFF01, 0xFFEE},
}};

} // namespace

bool inDomain(char16_t c) noexcept {
    for (const auto& r : kDomain)
        if (c >= r.lo && c <= r.hi) return true;
    return false;
}

bool inDomain(std::u16string_view text) noexcept {
    return std::all_of(text.begin(), text.end(), [](char16_t c) { return inDomain(c); });
}

char16_t toLowerInvariant(char16_t c) noexcept {
    if (c >= u'A' && c <= u'Z') return static_cast<char16_t>(c + 32);
    if (c < 0xC0) return c;
    if (c <= 0xDE) return c == 0xD7 ? c : static_cast<char16_t>(c + 32);
    if (c < 0x100) return c;
    if (c <= 0x17F) {
        // Latin Extended-A pairs; U+0130, U+0131, U+0138, U+0149 and U+017F have no
        // invariant lower-case change; U+0178 lowers to U+00FF.
        if (c <= 0x137) return (c & 1) || c == 0x130 ? c : static_cast<char16_t>(c + 1);
        if (c <= 0x148) return (c & 1) && c != 0x149 ? static_cast<char16_t>(c + 1) : c;
        if (c == 0x149) return c;
        if (c <= 0x177) return (c & 1) ? c : static_cast<char16_t>(c + 1);
        if (c == 0x178) return 0xFF;
        if (c <= 0x17E) return (c & 1) ? static_cast<char16_t>(c + 1) : c;
        return c;
    }
    if (c >= 0x400 && c <= 0x4FF) {
        if (c <= 0x40F) return static_cast<char16_t>(c + 0x50);
        if (c <= 0x42F) return static_cast<char16_t>(c + 0x20);
        if (c < 0x460) return c;
        if (c <= 0x481 || (c >= 0x48A && c <= 0x4BF) || c >= 0x4D0) return (c & 1) ? c : static_cast<char16_t>(c + 1);
        if (c == 0x4C0) return 0x4CF;
        if (c >= 0x4C1 && c <= 0x4CE) return (c & 1) ? static_cast<char16_t>(c + 1) : c;
        return c;
    }
    if (c >= 0x2160 && c <= 0x216F) return static_cast<char16_t>(c + 16);
    if (c == 0x2183) return 0x2184;
    if (c >= 0x24B6 && c <= 0x24CF) return static_cast<char16_t>(c + 26);
    if (c >= 0xFF21 && c <= 0xFF3A) return static_cast<char16_t>(c + 32);
    return c;
}

char16_t toUpperInvariant(char16_t c) noexcept {
    if (c >= u'a' && c <= u'z') return static_cast<char16_t>(c - 32);
    if (c == 0xB5) return 0x39C;
    if (c < 0xE0) return c;
    if (c <= 0xFE) return c == 0xF7 ? c : static_cast<char16_t>(c - 32);
    if (c == 0xFF) return 0x178;
    if (c < 0x100) return c;
    if (c <= 0x17F) {
        if (c <= 0x137) return (c & 1) && c != 0x131 ? static_cast<char16_t>(c - 1) : c;
        if (c <= 0x148) return (c & 1) || c == 0x138 ? c : static_cast<char16_t>(c - 1);
        if (c == 0x149) return c;
        if (c <= 0x177) return (c & 1) ? static_cast<char16_t>(c - 1) : c;
        if (c == 0x178) return c;
        if (c <= 0x17E) return (c & 1) ? c : static_cast<char16_t>(c - 1);
        return 0x53; // long s
    }
    if (c >= 0x400 && c <= 0x4FF) {
        if (c < 0x430) return c;
        if (c <= 0x44F) return static_cast<char16_t>(c - 0x20);
        if (c <= 0x45F) return static_cast<char16_t>(c - 0x50);
        if (c <= 0x481 || (c >= 0x48A && c <= 0x4BF) || c >= 0x4D0) return (c & 1) ? static_cast<char16_t>(c - 1) : c;
        if (c == 0x4CF) return 0x4C0;
        if (c >= 0x4C1 && c <= 0x4CE) return (c & 1) ? c : static_cast<char16_t>(c - 1);
        return c;
    }
    if (c >= 0x2170 && c <= 0x217F) return static_cast<char16_t>(c - 16);
    if (c == 0x2184) return 0x2183;
    if (c >= 0x24D0 && c <= 0x24E9) return static_cast<char16_t>(c - 26);
    if (c >= 0xFF41 && c <= 0xFF5A) return static_cast<char16_t>(c - 32);
    return c;
}

bool isNumber(char16_t c) noexcept {
    if (c >= u'0' && c <= u'9') return true;
    switch (c) {
        case 0xB2: case 0xB3: case 0xB9: case 0xBC: case 0xBD: case 0xBE: case 0x3007:
            return true;
        default:
            break;
    }
    return (c >= 0x2150 && c <= 0x2182) || (c >= 0x2185 && c <= 0x2189) ||
           (c >= 0x2460 && c <= 0x249B) || (c >= 0x24EA && c <= 0x24FF) ||
           (c >= 0x3021 && c <= 0x3029) || (c >= 0x3038 && c <= 0x303A) ||
           (c >= 0xFF10 && c <= 0xFF19);
}

bool isWhiteSpace(char16_t c) noexcept {
    return c == 0x20 || c == 0xA0 || c == 0x3000;
}

// ---------------------------------------------------------------- strings

std::u16string load5(std::span<const uint16_t> units) {
    std::u16string out;
    for (uint16_t u : units) {
        if (u == 0xFFFF || u == 0) break;
        out.push_back(u == 0x246D ? u'♂' : u == 0x246E ? u'♀' : static_cast<char16_t>(u));
    }
    return out;
}

namespace {

// TextNormalizer: half-width katakana ｦ..ﾝ to full width.
constexpr char16_t kFullwidthKana[] =
    u"ヲァィゥェォャュョッーアイウ"
    u"エオカキクケコサシスセソタチ"
    u"ツテトナニヌネノハヒフヘホマ"
    u"ミムメモヤユヨラリルレロワン";
constexpr char16_t kHalfDakuten = 0xFF9E, kHalfHandakuten = 0xFF9F;

bool takesDakuten(char16_t c) { return (c >= 0xFF76 && c <= 0xFF84) || (c >= 0xFF8A && c <= 0xFF8E); }
bool takesHandakuten(char16_t c) { return c >= 0xFF8A && c <= 0xFF8E; }
bool isSmallKana(char16_t c) {
    switch (c) {
        case 0x30A1: case 0x30A3: case 0x30A5: case 0x30A7: case 0x30A9:
        case 0x30C3: case 0x30E3: case 0x30E5: case 0x30E7: case 0x30EE:
            return true;
        default:
            return false;
    }
}

} // namespace

std::u16string normalizeConsole(std::u16string_view input) {
    std::u16string out;
    out.reserve(input.size());
    for (std::size_t i = 0; i < input.size(); ++i) {
        char16_t c = input[i];
        if (c == u' ' || c == 0x3000 || c == kHalfDakuten || c == kHalfHandakuten) continue;
        char16_t ofs = 0;
        if (c >= 0xFF66 && c <= 0xFF9D && i + 1 < input.size()) {
            const char16_t d = input[i + 1];
            if (d == kHalfDakuten && takesDakuten(c)) ofs = 1;
            else if (d == kHalfHandakuten && takesHandakuten(c)) ofs = 2;
            else if (d == kHalfDakuten && c == 0xFF73) ofs = 0x30F4 - 0x30A6; // ｳﾞ -> ヴ
        }
        c = toLowerInvariant(c);
        if (c >= 0x3041 && c <= 0x3096) c = static_cast<char16_t>(c + 0x60);
        else if ((c >= 0xFF10 && c <= 0xFF19) || (c >= 0xFF41 && c <= 0xFF5A)) c = static_cast<char16_t>(c - 0xFEE0);
        else if (c >= 0xFF66 && c <= 0xFF9D) c = static_cast<char16_t>(kFullwidthKana[c - 0xFF66] + ofs);
        if (c >= 0x30A1 && c <= 0x30F6) {
            if (isSmallKana(c)) c = static_cast<char16_t>(c + 1);
            else if (c == 0x30F5) c = 0x30AB;
            else if (c == 0x30F6) c = 0x30B1;
        }
        out.push_back(c);
    }
    return out;
}

std::u16string normalize5(std::u16string_view input) {
    std::u16string out(input);
    for (auto& c : out) {
        c = toUpperInvariant(c);
        if (c >= 0x30A1 && c <= 0x30F6) c = static_cast<char16_t>(c - 0x60);
        else if (c >= 0xFF21 && c <= 0xFF3A) c = static_cast<char16_t>(c - 0xFEE0);
    }
    return out;
}

// ---------------------------------------------------------------- species names

namespace {

constexpr uint8_t kLanguages3[] = {1, 2, 3, 4, 5, 7};
constexpr uint8_t kLanguages456[] = {1, 2, 3, 4, 5, 7, 8};
constexpr uint8_t kLanguages789[] = {1, 2, 3, 4, 5, 7, 8, 9, 10};

int languageRow(uint8_t language) noexcept {
    for (std::size_t i = 0; i < D::kSpeciesLanguageCount; ++i)
        if (D::kSpeciesNameLanguages[i] == language) return static_cast<int>(i);
    return -1;
}

std::u16string lowered(std::u16string_view text) {
    std::u16string out(text);
    for (auto& c : out) c = toLowerInvariant(c);
    return out;
}

using NameMap = std::unordered_map<std::u16string, uint16_t>;

// SpeciesName.GetDictionary: per language, name -> species (a later species with the
// same key replaces an earlier one).
const std::array<NameMap, D::kSpeciesLanguageCount>& nameMaps(bool lower) {
    static const auto build = [](bool low) {
        std::array<NameMap, D::kSpeciesLanguageCount> maps;
        for (std::size_t row = 0; row < D::kSpeciesLanguageCount; ++row)
            for (uint16_t s = 1; s < D::kSpeciesNameCount; ++s) {
                const std::u16string_view name = D::kSpeciesNames[row][s];
                maps[row][low ? lowered(name) : std::u16string(name)] = s;
            }
        return maps;
    };
    static const auto exact = build(false);
    static const auto low = build(true);
    return lower ? low : exact;
}

uint16_t lookup(std::u16string_view name, uint8_t generation, bool lower) {
    const auto& maps = nameMaps(lower);
    const std::u16string key = lower ? lowered(name) : std::u16string(name);
    for (uint8_t lang : gameLanguages(generation)) {
        const int row = languageRow(lang);
        if (row < 0) continue;
        const auto it = maps[static_cast<std::size_t>(row)].find(key);
        if (it != maps[static_cast<std::size_t>(row)].end()) return it->second;
    }
    return 0;
}

} // namespace

std::span<const uint8_t> gameLanguages(uint8_t generation) noexcept {
    if (generation <= 3) return kLanguages3;
    if (generation <= 6) return kLanguages456;
    return kLanguages789;
}

std::u16string_view speciesName(uint16_t species, uint8_t language) noexcept {
    const int row = languageRow(language);
    if (row < 0 || species >= D::kSpeciesNameCount) return {};
    return D::kSpeciesNames[row][species];
}

std::u16string_view eggName(uint8_t language) noexcept {
    return speciesName(0, language);
}

std::u16string speciesNameGeneration(uint16_t species, uint8_t language, uint8_t generation) {
    const bool apostrophe = language == 2 || language == 4 || language == 7;
    if (generation >= 5) {
        if (species == 83 && apostrophe) return u"Farfetch'd"; // Gen V has no slanted apostrophe
        return std::u16string(speciesName(species, language));
    }
    // GetSpeciesName1234, generation 3/4: upper case outside Japanese and Korean,
    // French E/I accents stripped, straight apostrophe for Farfetch'd.
    std::u16string name(speciesName(species, language));
    if (species == 0 || language == 1 || language == 8) return name;
    for (auto& c : name) {
        c = toUpperInvariant(c);
        if (language == 3 && c >= 0xC8 && c <= 0xCF) {
            if (c <= 0xCA) c = u'E';
            else if (c == 0xCF) c = u'I';
        }
    }
    if (species == 83 && apostrophe && name.size() >= 2) name[name.size() - 2] = u'\'';
    return name;
}

uint16_t speciesAnyLanguage(std::u16string_view name, uint8_t generation) {
    return lookup(name, generation, false);
}

uint16_t speciesAnyLanguageCaseInsensitive(std::u16string_view name, uint8_t generation) {
    return lookup(name, generation, true);
}

bool isNicknamedAnyLanguage(uint16_t species, std::u16string_view name, uint8_t generation) {
    for (uint8_t lang : gameLanguages(generation))
        if (name == speciesNameGeneration(species, lang, generation)) return false;
    return true;
}

// ---------------------------------------------------------------- word filter

namespace {

// The regular-expression subset PKHeX's lists use: literals, \-escaped punctuation,
// '.', '^', '$', [set] / [^set], (a|b|..) groups of literals, and ?, *, + on one atom.
struct Atom {
    enum Kind : uint8_t { Lit, Any, Set, NotSet, Group, Begin, End } kind = Lit;
    char16_t c = 0;
    std::vector<std::u16string> items; // Set: one string of members; Group: alternatives
    uint8_t min = 1, max = 1;          // max 255: unbounded
};
struct Pattern {
    std::vector<Atom> atoms;
    bool ok = true;
};

bool isMeta(char16_t c) {
    return c == u'.' || c == u'^' || c == u'$' || c == u'[' || c == u']' || c == u'(' || c == u')' ||
           c == u'|' || c == u'?' || c == u'*' || c == u'+' || c == u'{' || c == u'}' || c == u'\\';
}

Pattern parse(std::u16string_view p) {
    Pattern out;
    for (std::size_t i = 0; i < p.size(); ++i) {
        const char16_t c = p[i];
        Atom a;
        if (c == u'?' || c == u'*' || c == u'+') {
            if (out.atoms.empty() || out.atoms.back().kind == Atom::Begin || out.atoms.back().kind == Atom::End ||
                out.atoms.back().max != 1 || out.atoms.back().min != 1) {
                out.ok = false;
                return out;
            }
            auto& last = out.atoms.back();
            if (c == u'?') { last.min = 0; last.max = 1; }
            else if (c == u'*') { last.min = 0; last.max = 255; }
            else { last.min = 1; last.max = 255; }
            continue;
        }
        if (c == u'^') a.kind = Atom::Begin;
        else if (c == u'$') a.kind = Atom::End;
        else if (c == u'.') a.kind = Atom::Any;
        else if (c == u'\\') {
            if (i + 1 >= p.size() || !isMeta(p[i + 1])) { out.ok = false; return out; }
            a.kind = Atom::Lit;
            a.c = p[++i];
        } else if (c == u'[') {
            const std::size_t end = p.find(u']', i + 1);
            if (end == std::u16string_view::npos || end == i + 1) { out.ok = false; return out; }
            std::u16string_view body = p.substr(i + 1, end - i - 1);
            a.kind = Atom::Set;
            if (body.front() == u'^') { a.kind = Atom::NotSet; body.remove_prefix(1); }
            if (body.empty()) { out.ok = false; return out; }
            for (char16_t m : body)
                if (m == u'-' || m == u'\\' || m == u'[') { out.ok = false; return out; }
            a.items.emplace_back(body);
            i = end;
        } else if (c == u'(') {
            const std::size_t end = p.find(u')', i + 1);
            if (end == std::u16string_view::npos) { out.ok = false; return out; }
            const std::u16string_view body = p.substr(i + 1, end - i - 1);
            a.kind = Atom::Group;
            std::u16string cur;
            for (char16_t m : body) {
                if (m == u'|') { a.items.push_back(cur); cur.clear(); continue; }
                if (isMeta(m)) { out.ok = false; return out; }
                cur.push_back(m);
            }
            a.items.push_back(cur);
            i = end;
        } else if (isMeta(c)) {
            out.ok = false;
            return out;
        } else {
            a.kind = Atom::Lit;
            a.c = c;
        }
        out.atoms.push_back(std::move(a));
    }
    return out;
}

// One unit at `pos` matches a single-unit atom.
bool unitMatches(const Atom& a, std::u16string_view s, std::size_t pos) {
    if (pos >= s.size()) return false;
    const char16_t c = s[pos];
    switch (a.kind) {
        case Atom::Lit: return c == a.c;
        case Atom::Any: return c != u'\n';
        case Atom::Set: return a.items[0].find(c) != std::u16string::npos;
        case Atom::NotSet: return a.items[0].find(c) == std::u16string::npos;
        default: return false;
    }
}

// Backtracking match of atoms[at..] at `pos`, `count` occurrences of atoms[at] taken.
bool matchAt(const std::vector<Atom>& atoms, std::size_t at, std::u16string_view s, std::size_t pos, int count) {
    if (at == atoms.size()) return true;
    const Atom& a = atoms[at];
    if (a.kind == Atom::Begin) return pos == 0 && matchAt(atoms, at + 1, s, pos, 0);
    // .NET '$' without Multiline: the end, or just before a final '\n'.
    if (a.kind == Atom::End)
        return (pos == s.size() || (pos + 1 == s.size() && s[pos] == u'\n')) && matchAt(atoms, at + 1, s, pos, 0);
    if (count < a.max) {
        if (a.kind == Atom::Group) {
            for (const auto& alt : a.items) {
                if (alt.empty() && count >= a.min) continue; // an empty repeat makes no progress
                if (s.substr(pos, alt.size()) == alt && matchAt(atoms, at, s, pos + alt.size(), count + 1)) return true;
            }
        } else if (unitMatches(a, s, pos) && matchAt(atoms, at, s, pos + 1, count + 1)) {
            return true;
        }
    }
    return count >= a.min && matchAt(atoms, at + 1, s, pos, 0);
}

// The longest run of plain single literals: any match contains it.
std::u16string requiredLiteral(const std::vector<Atom>& atoms) {
    std::u16string best, cur;
    for (const auto& a : atoms) {
        if (a.kind == Atom::Lit && a.min == 1 && a.max == 1) {
            cur.push_back(a.c);
            if (cur.size() > best.size()) best = cur;
        } else {
            cur.clear();
        }
    }
    return best;
}

bool search(const Pattern& p, const std::u16string& required, std::u16string_view s) {
    if (!required.empty() && s.find(required) == std::u16string_view::npos) return false;
    const bool anchored = !p.atoms.empty() && p.atoms.front().kind == Atom::Begin;
    for (std::size_t start = 0; start <= s.size(); ++start) {
        if (matchAt(p.atoms, 0, s, start, 0)) return true;
        if (anchored) break;
    }
    return false;
}

struct CompiledList {
    std::vector<Pattern> patterns;
    std::vector<std::u16string> required;
};
struct Compiled {
    CompiledList nx, ds;
    std::size_t unsupported = 0;
};

const Compiled& compiled() {
    static const Compiled c = [] {
        Compiled out;
        auto add = [&](CompiledList& list, const char16_t* const* source, std::size_t count) {
            for (std::size_t i = 0; i < count; ++i) {
                list.patterns.push_back(parse(source[i]));
                list.required.push_back(requiredLiteral(list.patterns.back().atoms));
                out.unsupported += !list.patterns.back().ok;
            }
        };
        add(out.nx, D::kBadWordsSwitch, D::kBadWordsSwitchCount);
        add(out.ds, D::kBadWords3DS, D::kBadWords3DSCount);
        return out;
    }();
    return c;
}

// WordFilter.TryMatch: first pattern (in list order) found in the normalized text.
// A pattern the matcher cannot parse counts as a match, so text is never passed
// as clean on a pattern this module does not understand.
bool tryMatch(const CompiledList& list, std::u16string_view message, std::size_t& index) {
    const std::u16string clean = normalizeConsole(message);
    for (std::size_t i = 0; i < list.patterns.size(); ++i) {
        if (!list.patterns[i].ok || search(list.patterns[i], list.required[i], clean)) {
            index = i;
            return true;
        }
    }
    return false;
}

uint16_t maxSpecies(uint8_t generation) {
    return generation <= 3 ? 386 : generation == 4 ? 493 : 649;
}

} // namespace

FilterHit wordFilter(std::u16string_view message, uint8_t originGeneration) {
    FilterHit hit;
    if (message.size() <= 1 || std::all_of(message.begin(), message.end(), isWhiteSpace)) return hit;
    const auto& c = compiled();
    // WordFilterNX: species names of the origin context are allowed.
    const uint16_t nxSpecies = speciesAnyLanguageCaseInsensitive(message, originGeneration);
    if (!(nxSpecies != 0 && nxSpecies <= maxSpecies(originGeneration)) && tryMatch(c.nx, message, hit.index)) {
        hit.list = Filter::Switch;
        return hit;
    }
    // WordFilter3DS (original is not Gen VI): Gen VII species names, any case.
    const uint16_t dsSpecies = speciesAnyLanguageCaseInsensitive(message, 7);
    if (!(dsSpecies != 0 && dsSpecies <= 807) && tryMatch(c.ds, message, hit.index)) {
        hit.list = Filter::Console3DS;
        return hit;
    }
    if (originGeneration == 5) {
        const std::u16string clean = normalize5(message);
        // Dictionary semantics: the last duplicate word holds the index.
        for (std::size_t i = D::kBadWordsGen5Count; i-- > 0;) {
            if (clean == D::kBadWordsGen5[i]) {
                hit.list = Filter::Gen5;
                hit.index = i;
                return hit;
            }
        }
    }
    return hit;
}

bool tooManyNumbers(std::u16string_view text, uint8_t originGeneration) {
    if (originGeneration <= 3) return false;
    const std::size_t max = originGeneration < 6 ? 4 : 5;
    if (text.size() <= max) return false;
    return static_cast<std::size_t>(std::count_if(text.begin(), text.end(), isNumber)) > max;
}

std::u16string_view filterPattern(Filter list, std::size_t index) noexcept {
    switch (list) {
        case Filter::Switch: return index < D::kBadWordsSwitchCount ? D::kBadWordsSwitch[index] : u"";
        case Filter::Console3DS: return index < D::kBadWords3DSCount ? D::kBadWords3DS[index] : u"";
        case Filter::Gen5: return index < D::kBadWordsGen5Count ? D::kBadWordsGen5[index] : u"";
        default: return {};
    }
}

std::size_t unsupportedPatterns() {
    return compiled().unsupported;
}

} // namespace Legality::Gen5Text
