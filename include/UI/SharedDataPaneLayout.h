#pragma once

// Text layout for the native-data pane of SharedPokemonShell::drawDataAndGraph (every Gen I-V
// editor). Lifted from Thread C's Gen2Workspace::layoutDataRows (e59df023) so there is ONE
// implementation: "Label: value" on one line when it fits, otherwise the dim label then the value
// word-wrapped beneath it; a word wider than the pane is broken (on UTF-8 code-point boundaries);
// lines that would run below the pane end with "..." instead. Nothing can spill into the graph
// beside it or past the panel. Pure and measured through a callback, so host tests use it as is.

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::SharedDataPane {

inline constexpr int kPaneWidth = 198;       // drawDataAndGraph dataW
inline constexpr int kTextInset = 10;        // text inset either side
inline constexpr int kTextWidth = kPaneWidth - 2 * kTextInset;
inline constexpr int kLineHeight = 20;
inline constexpr int kTextTop = 34;          // below the pane title
inline constexpr int kTextBottom = 8;

struct Line {
    std::string text;
    bool label = false; // dim label line (the value follows on its own line)
    int y = 0;          // offset from the first text line
};

// Byte length of the longest prefix of `text` (at most `bytes`) that ends on a code point.
inline std::size_t codePointPrefix(const std::string& text, std::size_t bytes) {
    if (bytes >= text.size()) return text.size();
    while (bytes > 0 && (static_cast<unsigned char>(text[bytes]) & 0xC0) == 0x80) --bytes;
    return bytes;
}

template <class Rows, class Measure>
std::vector<Line> layoutRows(const Rows& rows, int width, int height, Measure measure) {
    std::vector<std::pair<std::string, bool>> wrapped;
    const auto wrapValue = [&](const std::string& value) {
        std::string line;
        std::size_t i = 0;
        while (i < value.size()) {
            std::size_t end = value.find(' ', i);
            if (end == std::string::npos) end = value.size();
            std::string word = value.substr(i, end - i);
            i = end < value.size() ? end + 1 : end;
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (measure(candidate) <= width) { line = candidate; continue; }
            if (!line.empty()) { wrapped.push_back({line, false}); line.clear(); }
            while (!word.empty() && measure(word) > width) {
                std::size_t cut = word.size();
                while (cut > 1 && measure(word.substr(0, codePointPrefix(word, cut))) > width) --cut;
                cut = std::max<std::size_t>(1, codePointPrefix(word, cut));
                // Never split inside a code point, even when one glyph alone is too wide.
                while (cut < word.size() && (static_cast<unsigned char>(word[cut]) & 0xC0) == 0x80) ++cut;
                wrapped.push_back({word.substr(0, cut), false});
                word = word.substr(cut);
            }
            line = word;
        }
        if (!line.empty()) wrapped.push_back({line, false});
    };
    for (const auto& row : rows) {
        const std::string combined = std::string(row.label) + ": " + std::string(row.value);
        if (measure(combined) <= width) { wrapped.push_back({combined, false}); continue; }
        wrapped.push_back({std::string(row.label), true});
        wrapValue(std::string(row.value));
    }
    const int capacity = std::max(0, height / kLineHeight);
    std::vector<Line> lines;
    for (std::size_t i = 0; i < wrapped.size() && static_cast<int>(lines.size()) < capacity; ++i)
        lines.push_back({wrapped[i].first, wrapped[i].second,
                         static_cast<int>(lines.size()) * kLineHeight});
    if (lines.size() < wrapped.size() && !lines.empty()) {
        auto& last = lines.back();
        last.label = false;
        std::string text = last.text;
        while (!text.empty() && measure(text + "...") > width)
            text.resize(codePointPrefix(text, text.size() - 1));
        last.text = text + "...";
    }
    return lines;
}

} // namespace PokeBank::UIModel::SharedDataPane
