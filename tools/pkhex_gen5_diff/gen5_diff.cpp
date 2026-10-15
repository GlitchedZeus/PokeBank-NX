// Reads the Gen V oracle's lines on stdin and prints PokeBank NX's verdict:
//   <id> INVALID|NOPROBLEMS|INCOMPLETE ; <invalid issue> | <invalid issue> ...
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Legality/Gen5Legality.h"

int main() {
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
            if (issue.severity != Legality::Severity::Invalid) continue;
            std::cout << (first ? " " : " | ") << issue.text;
            first = false;
        }
        std::cout << '\n';
    }
}
