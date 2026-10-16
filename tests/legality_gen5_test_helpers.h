// Report and PK5 helpers shared by the Gen V legality tests
// (tests/test_legality_gen5.cpp, tests/test_legality_title_matrix.cpp).
// Header-only; each test is its own executable.
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "Legality/Gen5Legality.h"

namespace Gen5TestHelpers {

inline bool hasText(const Legality::Report& r, Legality::Severity s, const std::string& text) {
    return std::any_of(r.issues.begin(), r.issues.end(),
                       [&](const auto& i) { return i.severity == s && i.text.find(text) != std::string::npos; });
}

inline bool has(const Legality::Report& r, Legality::Severity s) {
    return std::any_of(r.issues.begin(), r.issues.end(), [s](const auto& i) { return i.severity == s; });
}

inline bool hasIssue(const Legality::Report& r, Legality::Severity s, const std::string& text) {
    return hasText(r, s, text);
}

inline bool allComplete(const Legality::Report& r) {
    const auto& c = r.coverage;
    for (auto level : {c.structure, c.sourceGame, c.internal, c.moves, c.encounter, c.eventGift, c.pidRng, c.eggBreeding, c.transfer})
        if (level != Legality::CoverageLevel::Complete) return false;
    return true;
}

// The verdict classes as Report defines them: Invalid exactly when an Invalid issue
// exists; "No problems found" only with every area Complete and no Warning.
inline bool consistent(const Legality::Report& r) {
    using Legality::Severity;
    using Legality::Verdict;
    if ((r.verdict() == Verdict::Invalid) != has(r, Severity::Invalid)) return false;
    if (r.verdict() == Verdict::NoProblemsFound) return !has(r, Severity::Warning) && allComplete(r);
    return true;
}

inline std::vector<uint8_t> fromHex(const char* hex) {
    std::vector<uint8_t> out;
    for (const char* c = hex; c[0] && c[1]; c += 2) out.push_back(static_cast<uint8_t>(std::stoi(std::string(c, 2), nullptr, 16)));
    return out;
}

inline void refreshChecksum(std::vector<uint8_t>& pk) {
    const uint16_t sum = Legality::Gen5::checksum(pk);
    pk[6] = static_cast<uint8_t>(sum);
    pk[7] = static_cast<uint8_t>(sum >> 8);
}

inline bool sameReport(const Legality::Report& a, const Legality::Report& b) {
    if (a.verdict() != b.verdict() || a.issues.size() != b.issues.size()) return false;
    for (size_t i = 0; i < a.issues.size(); ++i)
        if (a.issues[i].severity != b.issues[i].severity || a.issues[i].text != b.issues[i].text) return false;
    return true;
}

}  // namespace Gen5TestHelpers
