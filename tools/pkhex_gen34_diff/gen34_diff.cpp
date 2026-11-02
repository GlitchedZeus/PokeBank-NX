// Reads the Gen III/IV oracle's lines on stdin and prints PokeBank NX's verdict:
//   <id> INVALID|NOPROBLEMS|INCOMPLETE|LOADFAIL <coverage> ; <invalid issue> | ... | W: <warning issue> | ...
// Each record is re-encrypted and loaded through the production entity types
// (Pokemon3FRLG for PK3, Pokemon4ReadOnly + Pokemon4ReadOnlyView for PK4), then
// analyzed with the save-format group and the exact source game of its cartridge.
// <coverage> lists the CoverageSummary fields that are not Complete (or "-").
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

// Legality.cpp keeps historical Trainer namespace wrappers (same shim as the
// exact-source legality tests).
namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

namespace {
struct Cartridge { std::string_view oracle, gameId; Enums::GameVersion group; };
constexpr Cartridge kCarts[] = {
    {"R", "ruby_gba", Enums::GameVersion::FRLG},      {"S", "sapphire_gba", Enums::GameVersion::FRLG},
    {"E", "emerald_gba", Enums::GameVersion::FRLG},   {"FR", "firered_gba", Enums::GameVersion::FRLG},
    {"LG", "leafgreen_gba", Enums::GameVersion::FRLG},
    {"D", "diamond_nds", Enums::GameVersion::DP},     {"P", "pearl_nds", Enums::GameVersion::DP},
    {"Pt", "platinum_nds", Enums::GameVersion::PT},   {"HG", "heartgold_nds", Enums::GameVersion::HGSS},
    {"SS", "soulsilver_nds", Enums::GameVersion::HGSS},
};

std::string coverage(const Legality::CoverageSummary& c) {
    using L = Legality::CoverageLevel;
    std::string out;
    auto add = [&](const char* name, L level) {
        if (level == L::Complete) return;
        if (!out.empty()) out += ',';
        out += name;
        out += level == L::None ? ":none" : ":partial";
    };
    add("structure", c.structure); add("source", c.sourceGame); add("internal", c.internal);
    add("moves", c.moves); add("encounter", c.encounter); add("event", c.eventGift);
    add("pidrng", c.pidRng); add("egg", c.eggBreeding); add("transfer", c.transfer);
    return out.empty() ? "-" : out;
}

void print(const std::string& id, const Legality::Report& report) {
    const auto verdict = report.verdict();
    std::cout << id << ' '
              << (verdict == Legality::Verdict::Invalid ? "INVALID"
                  : verdict == Legality::Verdict::NoProblemsFound ? "NOPROBLEMS" : "INCOMPLETE")
              << ' ' << coverage(report.coverage) << " ;";
    bool first = true;
    for (const auto& issue : report.issues) {
        if (issue.severity != Legality::Severity::Invalid) continue;
        std::cout << (first ? " " : " | ") << issue.text;
        first = false;
    }
    for (const auto& issue : report.issues) {
        if (issue.severity != Legality::Severity::Warning) continue;
        std::cout << (first ? " " : " | ") << "W: " << issue.text;
        first = false;
    }
    std::cout << '\n';
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string id, cart, hex;
        in >> id >> cart >> hex;
        std::vector<std::byte> bytes;
        for (std::size_t i = 0; i + 1 < hex.size(); i += 2)
            bytes.push_back(static_cast<std::byte>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        const Cartridge* c = nullptr;
        for (const auto& k : kCarts) if (k.oracle == cart) c = &k;
        if (!c) { std::cout << id << " LOADFAIL - ; unknown cartridge " << cart << '\n'; continue; }

        if (bytes.size() == Encryption::SIZE_STORED3_FRLG) {
            std::byte* enc = Encryption::encryptArray3FRLG(bytes);
            Pokemon::Pokemon3FRLG pk(std::span<const std::byte>(enc, bytes.size()));
            delete[] enc;
            pk.setSaveLanguage(2); // English save, matching the oracle's trainer
            if (!pk.sizeValid()) { std::cout << id << " LOADFAIL - ; PK3 size\n"; continue; }
            print(id, Legality::analyze(pk, c->group, c->gameId));
        } else if (bytes.size() == Encryption::SIZE_STORED4) {
            const auto enc = Encryption::encryptArray4(bytes);
            Pokemon::Pokemon4ReadOnly source(enc, c->group);
            if (!source.valid()) {
                std::cout << id << " LOADFAIL - ; PK4 rejected (size " << source.sizeValid()
                          << " checksum " << source.checksumValid() << " sanity " << source.sanityValid() << ")\n";
                continue;
            }
            const Pokemon::Pokemon4ReadOnlyView view(source);
            print(id, Legality::analyze(view, c->group, c->gameId));
        } else {
            std::cout << id << " LOADFAIL - ; record length " << bytes.size() << '\n';
        }
    }
}
