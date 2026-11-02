// PokeBank NX application QA: Gen I-V acceptance matrix runner (Thread 4, QA-owned).
//   test_qa_matrix [--markdown <path>]
// Exit status is non-zero when any cell is Fail, when a version/cell is missing, or when a cell is
// recorded twice or outside its scope (cellApplies; out-of-scope cells print "-"). Known defects
// (status Known) do not fail the run; their strict repro is in test_qa_known_failures.
// Cross-generation fact differences are reported, not failed.
#include "test_qa_matrix.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

namespace {

const char* symbol(qamatrix::Status s) {
    switch (s) {
        case qamatrix::Status::Pass: return "PASS";
        case qamatrix::Status::Fail: return "FAIL";
        case qamatrix::Status::Known: return "KNOWN";
        case qamatrix::Status::HardwareOnly: return "HW";
        case qamatrix::Status::Unsupported: return "N/A";
    }
    return "?";
}

std::string escapeCell(std::string text) {
    std::string out;
    for (char c : text) {
        if (c == '|') out += "\\|";
        else if (c == '\n') out += ' ';
        else out += c;
    }
    return out;
}

} // namespace

int main(int argc, char** argv) {
    std::string markdownPath;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--markdown" && i + 1 < argc) markdownPath = argv[++i];
        else {
            std::cerr << "usage: test_qa_matrix [--markdown <path>]\n";
            return 2;
        }
    }

    qamatrix::Matrix m;
    qamatrix::runGen12(m);
    qamatrix::runGen3(m);
    qamatrix::runGen4(m);
    qamatrix::runGen5(m);

    int problems = 0;
    std::map<std::pair<std::string, std::string>, const qamatrix::Entry*> grid;
    for (const auto& e : m.entries()) {
        if (std::find(qamatrix::kVersions.begin(), qamatrix::kVersions.end(), e.version) ==
                qamatrix::kVersions.end() ||
            std::find(qamatrix::kCells.begin(), qamatrix::kCells.end(), e.cell) == qamatrix::kCells.end()) {
            std::cerr << "UNKNOWN row/column " << e.version << " / " << e.cell << "\n";
            ++problems;
            continue;
        }
        if (!qamatrix::cellApplies(e.cell, e.version)) {
            std::cerr << "OUT-OF-SCOPE " << e.version << " / " << e.cell << " recorded\n";
            ++problems;
            continue;
        }
        if (!grid.emplace(std::make_pair(e.version, e.cell), &e).second) {
            std::cerr << "DUPLICATE " << e.version << " / " << e.cell << "\n";
            ++problems;
        }
        if (e.status == qamatrix::Status::Fail) {
            std::cerr << "FAIL " << e.version << " / " << e.cell << ": " << e.note << "\n";
            ++problems;
        }
    }
    std::map<qamatrix::Status, int> totals;
    for (auto version : qamatrix::kVersions)
        for (auto cell : qamatrix::kCells) {
            if (!qamatrix::cellApplies(cell, version)) continue;
            const auto it = grid.find({std::string(version), std::string(cell)});
            if (it == grid.end()) {
                std::cerr << "MISSING " << version << " / " << cell << "\n";
                ++problems;
            } else {
                ++totals[it->second->status];
            }
        }

    // S06: every version's export cell must have proven that its strict reparse detects a corrupted
    // checksummed byte in the published edited save (exportCorruptionProbe in test_qa_matrix.h).
    {
        const auto& facts = m.facts();
        const auto it = facts.find(std::string(qamatrix::kExportChecksumFact));
        for (auto version : qamatrix::kVersions) {
            const bool ok = it != facts.end() && it->second.count(std::string(version)) &&
                            it->second.at(std::string(version)) == qamatrix::kExportChecksumDetected;
            if (!ok) {
                std::cerr << "S06 " << version << ": no " << qamatrix::kExportChecksumFact << "="
                          << qamatrix::kExportChecksumDetected << " fact from the export cell\n";
                ++problems;
            }
        }
    }

    std::ostringstream md;
    md << "| Version |";
    for (auto cell : qamatrix::kCells) md << ' ' << cell << " |";
    md << "\n| --- |";
    for (size_t i = 0; i < qamatrix::kCells.size(); ++i) md << " --- |";
    md << "\n";
    for (auto version : qamatrix::kVersions) {
        md << "| " << version << " |";
        for (auto cell : qamatrix::kCells) {
            if (!qamatrix::cellApplies(cell, version)) {
                md << " - |";
                continue;
            }
            const auto it = grid.find({std::string(version), std::string(cell)});
            md << ' ' << (it == grid.end() ? "MISSING" : symbol(it->second->status)) << " |";
        }
        md << "\n";
    }
    md << "\nTotals:";
    for (auto s : {qamatrix::Status::Pass, qamatrix::Status::Known, qamatrix::Status::HardwareOnly,
                   qamatrix::Status::Unsupported, qamatrix::Status::Fail})
        md << ' ' << symbol(s) << '=' << totals[s];
    md << "\n\n### Cell notes\n\n| Version | Cell | Status | Note |\n| --- | --- | --- | --- |\n";
    for (auto version : qamatrix::kVersions)
        for (auto cell : qamatrix::kCells) {
            const auto it = grid.find({std::string(version), std::string(cell)});
            if (it == grid.end() || it->second->note.empty()) continue;
            md << "| " << version << " | " << cell << " | " << symbol(it->second->status) << " | "
               << escapeCell(it->second->note) << " |\n";
        }

    md << "\n### Cross-generation comparison\n\n";
    md << "| Fact | Values (versions) | Consistent |\n| --- | --- | --- |\n";
    int inconsistent = 0;
    for (const auto& [key, byVersion] : m.facts()) {
        std::map<std::string, std::vector<std::string>> groups;
        for (auto version : qamatrix::kVersions) {
            const auto it = byVersion.find(std::string(version));
            if (it != byVersion.end()) groups[it->second].push_back(it->first);
        }
        std::ostringstream values;
        bool first = true;
        for (const auto& [value, versions] : groups) {
            if (!first) values << "; ";
            first = false;
            values << '`' << value << "` (";
            for (size_t i = 0; i < versions.size(); ++i) values << (i ? ", " : "") << versions[i];
            values << ')';
        }
        // "n/a" = the format has no such data (e.g. Gen I shiny): not an inconsistency.
        size_t applicable = 0;
        for (const auto& group : groups) applicable += group.first == "n/a" ? 0 : 1;
        const bool same = applicable <= 1;
        if (!same) ++inconsistent;
        md << "| `" << key << "` | " << escapeCell(values.str()) << " | " << (same ? "yes" : "**NO**") << " |\n";
    }

    std::cout << md.str();
    if (!markdownPath.empty()) {
        std::ofstream out(markdownPath, std::ios::binary);
        out << md.str();
        if (!out) {
            std::cerr << "cannot write " << markdownPath << "\n";
            return 2;
        }
    }
    std::cerr << "matrix: " << m.entries().size() << " cells, " << inconsistent
              << " inconsistent cross-generation fact(s), " << problems << " problem(s)\n";
    return problems == 0 ? 0 : 1;
}
