#pragma once

#include "UI/SharedReviewPresentation.h"

#include <string>
#include <vector>

// Round 6 P3: Gen I Review rows in the shared "<where> / <field>" over "<value>" shape. A Gen I
// change line is "<where>: <field>: <before> -> <after>" (or "<where>: <field>: <value>" for a new
// value, e.g. a created Pokemon's Moves). The value is everything after the LAST ": "; the parts
// before it are the title, joined with " / ". A line without ": " stays one unstructured row.
namespace PokeBank::UIModel::Gen1ReviewRows {

inline SharedReview::Rows fromChangeLines(const std::vector<std::string>& lines) {
    SharedReview::Rows rows;
    rows.reserve(lines.size());
    for (const auto& line : lines) {
        const auto split = line.rfind(": ");
        if (split == std::string::npos) {
            rows.emplace_back("Staged change", line);
            continue;
        }
        std::string title = line.substr(0, split);
        for (auto at = title.find(": "); at != std::string::npos; at = title.find(": ", at + 3))
            title.replace(at, 2, " / ");
        rows.emplace_back(std::move(title), line.substr(split + 2));
    }
    return rows;
}

} // namespace PokeBank::UIModel::Gen1ReviewRows
