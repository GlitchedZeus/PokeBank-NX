#pragma once
// Master Vault search with highlighted matches (owner v1.0 requirement; PBX-VLT-002).
//
// Pure model, no I/O: filters Vault entries by a free-text query and reports, per row, WHERE the
// query matched so the browser can highlight it. Matching is case-insensitive for ASCII letters and
// byte-exact otherwise (UTF-8 nicknames match on their own bytes, never on a lossy fold). Searched
// fields, in display order: nickname, species, OT, source game, Vault ID. Every whitespace-separated
// term must match some field (AND); an empty query lists everything.

#include "Vault/VaultStore.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PokeBank::Vault::Search {

enum class Field : uint8_t { Nickname, Species, OtName, SourceGame, VaultId };

struct Span {
    Field field = Field::Nickname;
    std::size_t start = 0;   // byte offset into the field's text
    std::size_t length = 0;
};

struct Hit {
    std::size_t entryIndex = 0;   // index into Store::entries()
    std::vector<Span> spans;      // every match of every term, for highlighting
};

[[nodiscard]] inline char fold(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// Every non-overlapping occurrence of `needle` in `hay` (case-insensitive ASCII).
[[nodiscard]] inline std::vector<std::size_t> occurrences(std::string_view hay, std::string_view needle) {
    std::vector<std::size_t> out;
    if (needle.empty() || needle.size() > hay.size()) return out;
    for (std::size_t i = 0; i + needle.size() <= hay.size();) {
        bool same = true;
        for (std::size_t k = 0; k < needle.size() && same; ++k) same = fold(hay[i + k]) == fold(needle[k]);
        if (same) {
            out.push_back(i);
            i += needle.size();
        } else {
            ++i;
        }
    }
    return out;
}

[[nodiscard]] inline std::vector<std::string> terms(std::string_view query) {
    std::vector<std::string> out;
    std::string current;
    for (const char c : query) {
        if (c == ' ' || c == '\t') {
            if (!current.empty()) out.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) out.push_back(std::move(current));
    return out;
}

[[nodiscard]] inline std::string fieldText(const Entry& e, Field f) {
    switch (f) {
        case Field::Nickname: return e.summary.nickname;
        case Field::Species: return e.summary.speciesName;
        case Field::OtName: return e.summary.otName;
        case Field::SourceGame: return e.provenance.sourceGame;
        case Field::VaultId: return e.id.text();
    }
    return {};
}

// generation 0 = every generation; otherwise only that generation's records (withdraw lists only
// what the open save can take).
[[nodiscard]] inline std::vector<Hit> search(const std::vector<Entry>& entries, std::string_view query,
                                             int generation = 0) {
    constexpr Field kFields[] = {Field::Nickname, Field::Species, Field::OtName, Field::SourceGame, Field::VaultId};
    const auto wanted = terms(query);
    std::vector<Hit> hits;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        if (generation != 0 && e.generation != generation) continue;
        Hit hit;
        hit.entryIndex = i;
        bool all = true;
        for (const auto& term : wanted) {
            bool any = false;
            for (const Field f : kFields) {
                const std::string text = fieldText(e, f);
                for (const std::size_t at : occurrences(text, term)) {
                    hit.spans.push_back({f, at, term.size()});
                    any = true;
                }
            }
            if (!any) { all = false; break; }
        }
        if (all) hits.push_back(std::move(hit));
    }
    return hits;
}

// The spans of one field, sorted and merged, for drawing one label with highlighted runs.
[[nodiscard]] inline std::vector<Span> spansFor(const Hit& hit, Field field) {
    std::vector<Span> out;
    for (const auto& s : hit.spans)
        if (s.field == field) out.push_back(s);
    for (std::size_t i = 1; i < out.size(); ++i)   // insertion sort: a handful of spans
        for (std::size_t j = i; j > 0 && out[j].start < out[j - 1].start; --j) std::swap(out[j], out[j - 1]);
    std::vector<Span> merged;
    for (const auto& s : out) {
        if (!merged.empty() && s.start <= merged.back().start + merged.back().length) {
            const std::size_t end = std::max(merged.back().start + merged.back().length, s.start + s.length);
            merged.back().length = end - merged.back().start;
        } else {
            merged.push_back(s);
        }
    }
    return merged;
}

}  // namespace PokeBank::Vault::Search
