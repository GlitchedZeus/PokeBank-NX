#pragma once
#include "UI/SharedEditorPresentation.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::SharedReview {
using Row=std::pair<std::string,std::string>;
using Rows=std::vector<Row>;
// Classic export adapters already produce named, semantic change descriptions.
// Keep their full text; split the context from the value without interpreting IDs.
inline Rows fromLines(const std::vector<std::string>& lines) {
    Rows rows;
    for (const auto& line : lines) {
        const auto separator = line.find(": ");
        rows.emplace_back(separator == std::string::npos ? "Staged change" : line.substr(0, separator),
            separator == std::string::npos ? line : line.substr(separator + 2));
    }
    return rows;
}
inline constexpr size_t kPageSize=4;
inline constexpr size_t pages(size_t rows) { return rows?1+(rows-1)/kPageSize:1; }
inline constexpr size_t clamp(size_t page,size_t rows) { return std::min(page,pages(rows)-1); }
inline constexpr size_t move(size_t page,int delta,size_t rows) {
    page=clamp(page,rows);
    return delta<0?(page? page-1:0):delta>0?std::min(page+1,pages(rows)-1):page;
}
inline SharedEditorPresentation::Footer footer(bool changed,size_t rows) {
    SharedEditorPresentation::Footer out;
    if(pages(rows)>1)out.push("L/R","Review Pages");
    if(changed) {out.push("X","Export Copy");out.push("Y","Discard All");}
    out.push("B","Back");return out;
}
}
