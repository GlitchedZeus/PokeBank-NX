// PokeBank NX side of the Gen V PKHeX differential (tools/pkhex_gen5_diff/run.sh).
//
//   gen5_diff               oracle "generate" lines on stdin; prints PokeBank NX's verdict:
//                           <id> INVALID|NOPROBLEMS|INCOMPLETE ; <invalid issue> | ...
//   gen5_diff warnings      the same, listing Warning issues instead of Invalid ones
//   gen5_diff casemap       oracle "casemap" lines: .NET casing / IsNumber / IsWhiteSpace
//   gen5_diff patterns      oracle "patterns" lines: the word-filter lists as PKHeX loads them
//   gen5_diff wordfilter    oracle "wordfilter" lines: PKHeX word-filter verdicts
//   gen5_diff fixednames    oracle "fixednames" lines: names Gen III/IV encounters fix
// The four checks print a summary and exit 1 on any disagreement.
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Legality/Gen5Legality.h"
#include "Legality/Gen5LegalityData.h"
#include "Legality/Gen5Text.h"

namespace {

namespace T = Legality::Gen5Text;

std::u16string fromHex(const std::string& hex) {
    std::u16string out;
    for (std::size_t i = 0; i + 3 < hex.size(); i += 4)
        out.push_back(static_cast<char16_t>(std::stoul(hex.substr(i, 4), nullptr, 16)));
    return out;
}

std::string toHex(std::u16string_view s) {
    static const char* digits = "0123456789ABCDEF";
    std::string out;
    for (char16_t c : s)
        for (int shift = 12; shift >= 0; shift -= 4) out.push_back(digits[(c >> shift) & 15]);
    return out;
}

int casemap() {
    std::string line;
    std::size_t checked = 0, bad = 0;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string unit, lower, upper;
        int number = 0, white = 0;
        in >> unit >> lower >> upper >> number >> white;
        const auto c = static_cast<char16_t>(std::stoul(unit, nullptr, 16));
        if (!T::inDomain(c)) continue;
        ++checked;
        const bool ok = T::toLowerInvariant(c) == std::stoul(lower, nullptr, 16) &&
                        T::toUpperInvariant(c) == std::stoul(upper, nullptr, 16) &&
                        T::isNumber(c) == (number != 0) && T::isWhiteSpace(c) == (white != 0);
        if (!ok && bad++ < 20)
            std::cout << "casemap mismatch U+" << unit << ": .NET " << lower << ' ' << upper << ' ' << number << ' ' << white
                      << ", ours " << toHex(std::u16string(1, T::toLowerInvariant(c))) << ' '
                      << toHex(std::u16string(1, T::toUpperInvariant(c))) << ' ' << T::isNumber(c) << ' ' << T::isWhiteSpace(c) << '\n';
    }
    std::cout << "text domain casing/classes vs .NET: " << checked << " units, " << bad << " mismatches\n";
    return bad == 0 && checked > 0 ? 0 : 1;
}

int patterns() {
    std::string line;
    std::size_t seen[3] = {0, 0, 0}, bad = 0;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string list, hex;
        std::size_t index = 0;
        in >> list >> index >> hex;
        const int which = list == "nx" ? 0 : list == "3ds" ? 1 : 2;
        const auto kind = which == 0 ? T::Filter::Switch : which == 1 ? T::Filter::Console3DS : T::Filter::Gen5;
        ++seen[which];
        if (T::filterPattern(kind, index) != fromHex(hex) && bad++ < 20)
            std::cout << "pattern mismatch " << list << ' ' << index << '\n';
    }
    namespace D = Legality::Gen5Data;
    if (seen[0] != D::kBadWordsSwitchCount || seen[1] != D::kBadWords3DSCount || seen[2] != D::kBadWordsGen5Count) {
        std::cout << "pattern list sizes differ from PKHeX\n";
        ++bad;
    }
    const std::size_t unsupported = T::unsupportedPatterns();
    std::cout << "word-filter lists vs PKHeX: " << seen[0] << '/' << seen[1] << '/' << seen[2] << " entries, " << bad
              << " mismatches, " << unsupported << " patterns the matcher cannot parse\n";
    return bad == 0 && unsupported == 0 ? 0 : 1;
}

int wordfilter() {
    std::string line;
    std::size_t checked = 0, skipped = 0, filtered = 0, bad = 0;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string hex, flags;
        in >> hex >> flags;
        if (flags.size() != 3) { ++bad; continue; }
        const std::u16string message = fromHex(hex);
        if (!T::inDomain(message)) { ++skipped; continue; }
        ++checked;
        const uint8_t origins[3] = {5, 4, 3};
        for (int k = 0; k < 3; ++k) {
            const bool ours = T::wordFilter(message, origins[k]).list != T::Filter::None;
            filtered += ours;
            if (ours != (flags[static_cast<std::size_t>(k)] == '1') && bad++ < 20)
                std::cout << "word filter mismatch (origin Gen " << int(origins[k]) << ") " << hex << ": PKHeX "
                          << flags[static_cast<std::size_t>(k)] << ", ours " << ours << '\n';
        }
    }
    std::cout << "word filter vs PKHeX: " << checked << " strings x 3 origins (" << filtered << " filtered), "
              << skipped << " outside the text domain, " << bad << " mismatches\n";
    return bad == 0 && checked > 0 ? 0 : 1;
}

// The transfer rules treat a name in kTransferTradeNames as possibly fixed by an
// in-game trade, a non-fateful Gen IV OT outside it as the player's, and a non-fateful
// Pokemon whose OT is outside it as starting with no contest stats. They hold only if
// every fixed Gen III/IV nickname, every fixed OT of a non-fateful Gen IV encounter
// and the OT of every non-fateful encounter with contest stats is in the table.
int fixednames() {
    namespace D = Legality::Gen5Data;
    auto inTable = [](const std::u16string& s) {
        for (std::size_t i = 0; i < D::kTransferTradeNameCount; ++i)
            if (s == D::kTransferTradeNames[i]) return true;
        return false;
    };
    std::string line;
    std::size_t nicks = 0, ots = 0, bad = 0;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        int gen = 0, fixedNick = 0, fixedOT = 0, fateful = 0, lang = 0;
        std::string nickHex, otHex, what;
        in >> gen >> fixedNick >> fixedOT >> fateful >> lang >> nickHex >> otHex >> what;
        const std::u16string nick = nickHex == "-" ? u"" : fromHex(nickHex);
        const std::u16string ot = otHex == "-" ? u"" : fromHex(otHex);
        if (fixedNick) {
            ++nicks;
            if (!inTable(nick) && bad++ < 20) std::cout << "fixed nickname not in the trade-name table: " << line << '\n';
        }
        // Non-fateful Gen IV fixed OTs (the OT rules; an empty OT gets none) and the OT of
        // every in-game trade that starts with contest stats of its own (PKHeX
        // IContestStatsReadOnly: EncounterTrade3 and EncounterTrade4PID; XD trades have none).
        const bool trade = what.rfind("EncounterTrade3:", 0) == 0 || what.rfind("EncounterTrade4PID:", 0) == 0;
        if (fixedOT && !ot.empty() && ((gen == 4 && !fateful) || trade)) {
            ++ots;
            if (!inTable(ot) && bad++ < 20) std::cout << "fixed OT not in the trade-name table: " << line << '\n';
        }
    }
    std::cout << "Gen III/IV fixed names vs the trade-name table: " << nicks << " nicknames, " << ots
              << " fixed OTs, " << bad << " missing\n";
    return bad == 0 && nicks > 0 && ots > 0 ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
    auto listed = Legality::Severity::Invalid;
    if (argc == 2) {
        const std::string mode = argv[1];
        if (mode == "casemap") return casemap();
        if (mode == "patterns") return patterns();
        if (mode == "wordfilter") return wordfilter();
        if (mode == "fixednames") return fixednames();
        if (mode != "warnings") {
            std::cerr << "usage: gen5_diff [warnings|casemap|patterns|wordfilter|fixednames]\n";
            return 2;
        }
        listed = Legality::Severity::Warning;
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string id, hex;
        in >> id >> hex;
        std::vector<uint8_t> bytes;
        for (std::size_t i = 0; i + 1 < hex.size(); i += 2)
            bytes.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        const auto report = Legality::analyzePk5(bytes, {.encrypted = false});
        const auto verdict = report.verdict();
        std::cout << id << ' '
                  << (verdict == Legality::Verdict::Invalid ? "INVALID"
                      : verdict == Legality::Verdict::NoProblemsFound ? "NOPROBLEMS" : "INCOMPLETE")
                  << " ;";
        bool first = true;
        for (const auto& issue : report.issues) {
            if (issue.severity != listed) continue;
            std::cout << (first ? " " : " | ") << issue.text;
            first = false;
        }
        if (listed == Legality::Severity::Warning) {
            const auto& c = report.coverage;
            const std::pair<const char*, Legality::CoverageLevel> areas[] = {
                {"structure", c.structure}, {"sourceGame", c.sourceGame}, {"internal", c.internal},
                {"moves", c.moves}, {"encounter", c.encounter}, {"eventGift", c.eventGift},
                {"pidRng", c.pidRng}, {"eggBreeding", c.eggBreeding}, {"transfer", c.transfer}};
            for (const auto& [name, level] : areas)
                if (level != Legality::CoverageLevel::Complete) std::cout << " [" << name << ']';
        }
        std::cout << '\n';
    }
}
