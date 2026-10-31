// Differential driver: PokeBank NX Gen VI-VIII (PK6/PK7/PK8/PB8) verdicts against a PKHeX corpus.
//
//   gen67_diff [SHOW] < corpus.txt
//
// Corpus lines come from oracle/Program.cs ("generate"):
//   <id> <6|7|8|8b> <game> <box|party> <decrypted hex> VALID|INVALID <encounter>:<species> <group> ; <PKHeX results>
// Each record is analyzed decrypted, with the line's game and slot. Exit status 1 when
//   - PokeBank NX reports Invalid for a record PKHeX accepts (a false Invalid), or
//   - PokeBank NX reports "No problems found" for a record PKHeX rejects (a false Valid).
// Records PKHeX rejects that PokeBank NX leaves Incomplete are counted, not failed:
// Unknown is never turned into Invalid.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Legality/ModernLegality.h"

namespace {

// Rule label without the numbers in the message ("Move 2 PP ..." -> "Move # PP ...").
std::string ruleOf(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c))) { if (out.empty() || out.back() != '#') out.push_back('#'); }
        else out.push_back(c);
    }
    return out;
}

std::vector<uint8_t> fromHex(const std::string& hex) {
    std::vector<uint8_t> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<uint8_t>(std::stoi(hex.substr(2 * i, 2), nullptr, 16));
    return out;
}

const char* verdictName(Legality::Verdict v) {
    switch (v) {
        case Legality::Verdict::Invalid: return "Invalid";
        case Legality::Verdict::NoProblemsFound: return "NoProblemsFound";
        case Legality::Verdict::Incomplete: return "Incomplete";
    }
    return "?";
}

} // namespace

int main(int argc, char** argv) {
    const size_t show = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : 20;
    std::string line;
    size_t total = 0, pkValid = 0, pkInvalid = 0, detected = 0, undetected = 0, falseInvalid = 0, falseValid = 0, malformed = 0;
    std::map<std::string, size_t> detectedByRule, falseByRule, undetectedByPkhex;
    std::map<std::string, std::pair<size_t, size_t>> byGroup; // group -> {PKHeX invalid, detected}
    std::vector<std::string> falseInvalidLines;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string id, fmt, game, slot, hex, verdict, enc, group;
        if (!(in >> id >> fmt >> game >> slot >> hex >> verdict >> enc >> group)) { ++malformed; continue; }
        ++total;
        Legality::Modern::Input input;
        input.format = fmt == "6" ? Legality::Modern::Format::PK6
                     : fmt == "7" ? Legality::Modern::Format::PK7
                     : fmt == "7b" ? Legality::Modern::Format::PB7 : fmt == "8b" ? Legality::Modern::Format::PB8 : fmt == "9" ? Legality::Modern::Format::PK9 : Legality::Modern::Format::PK8;
        input.game = static_cast<Legality::Modern::Game>(std::stoi(game));
        input.slot = slot == "party" ? Legality::Modern::Slot::Party : Legality::Modern::Slot::Box;
        input.encrypted = false;
        const auto bytes = fromHex(hex);
        const auto report = Legality::Modern::analyze(bytes, input);
        const auto ours = report.verdict();
        const bool pkhexValid = verdict == "VALID";
        (pkhexValid ? pkValid : pkInvalid)++;
        std::string firstInvalid, firstText;
        for (const auto& issue : report.issues)
            if (issue.severity == Legality::Severity::Invalid) { firstText = issue.text; firstInvalid = ruleOf(issue.text); break; }
        if (pkhexValid && ours == Legality::Verdict::Invalid) {
            ++falseInvalid;
            ++falseByRule[firstInvalid];
            falseInvalidLines.push_back(id + " " + fmt + " " + game + " " + slot + " " + enc + " " + group + " :: " + firstText + "\n    " + hex);
        } else if (!pkhexValid && ours == Legality::Verdict::NoProblemsFound) {
            ++falseValid;
            std::cout << "FALSE VALID " << line << "\n";
        } else if (!pkhexValid) {
            auto& g = byGroup[group];
            ++g.first;
            if (ours == Legality::Verdict::Invalid) { ++detected; ++g.second; ++detectedByRule[firstInvalid]; }
            else {
                ++undetected;
                // First PKHeX result that is not a "~" (Fishy) line, for the gap summary.
                const auto results = line.substr(line.find(" ; ") + 3);
                std::istringstream rs(results);
                std::string r;
                while (rs >> r && r[0] == '~') {}
                ++undetectedByPkhex[r];
            }
        }
        if (report.verdict() != Legality::Verdict::Invalid && report.verdict() != Legality::Verdict::Incomplete &&
            report.verdict() != Legality::Verdict::NoProblemsFound)
            std::cout << "NO VERDICT " << id << " " << verdictName(ours) << "\n";
    }
    std::cout << "records " << total << " (PKHeX valid " << pkValid << ", invalid " << pkInvalid << "), malformed lines " << malformed << "\n";
    std::cout << "false Invalid " << falseInvalid << ", false Valid " << falseValid << "\n";
    std::cout << "PKHeX-invalid detected as Invalid " << detected << ", left Incomplete " << undetected << "\n";
    for (const auto& [g, c] : byGroup) std::cout << "  group " << g << ": " << c.second << "/" << c.first << " detected\n";
    std::cout << "detected by first rule:\n";
    for (const auto& [r, c] : detectedByRule) std::cout << "  " << c << "  " << r << "\n";
    std::cout << "left Incomplete, by first PKHeX result:\n";
    std::vector<std::pair<size_t, std::string>> gaps;
    for (const auto& [r, c] : undetectedByPkhex) gaps.emplace_back(c, r);
    std::sort(gaps.rbegin(), gaps.rend());
    for (size_t i = 0; i < gaps.size() && i < 25; ++i) std::cout << "  " << gaps[i].first << "  " << gaps[i].second << "\n";
    if (falseInvalid) {
        std::cout << "false Invalid by rule:\n";
        for (const auto& [r, c] : falseByRule) std::cout << "  " << c << "  " << r << "\n";
        for (size_t i = 0; i < falseInvalidLines.size() && i < show; ++i) std::cout << "  " << falseInvalidLines[i] << "\n";
    }
    return falseInvalid || falseValid || malformed ? 1 : 0;
}
