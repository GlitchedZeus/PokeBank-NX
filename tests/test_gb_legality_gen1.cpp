// Gen I legality through the production analyzer, on raw PK1 party records.
//
// Every expected verdict below is what PKHeX.Core (kwsch/PKHeX@6501f0ab, GB cartridge-era
// settings) reports for the same bytes; tools/pkhex_gb_diff reproduces them. The one
// deliberate difference is marked: PKHeX does not compare level with EXP for Gen I,
// while the games keep them in step (battle, Rare Candy and Day Care all recompute).
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {
struct Case {
    const char* name;
    const char* body;  // 44-byte party record
    const char* ot;
    const char* nick;
    bool pkhexValid;
};

const Case kCases[] = {
    {"rattata-route1", "a5000f03000000ff21270000303900001b000000000000000000009abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"mewtwo-l5", "83001a05001818035d32815e303900009c000000000000000000009abc0000000005001a0010000f00130015", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"bulbasaur-hyperbeam", "990014050016032d212d3f003039000087000000000000000000009abc00000000050014000a000a000a000c", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"caterpie-thunderbolt", "7b001405000707ff21515500303900007d000000000000000000009abc0000000005001400080009000a0008", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"charizard-l2", "b4000f020014022d0a2d00003039000009000000000000000000009abc0000000002000f0008000800090008", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"rattata-forgot-move", "a5000f03000000ff21000000303900001b000000000000000000009abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"rattata-forgot-move-tradeback", "a5000f030000000021000000303900001b000000000000000000009abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"pikachu-stadium-surf", "54001d0a001717be542d390030390003e8000000000000000000009abc000000000a001d0011000d00190011", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"pikachu-yellow-starter", "54001305001717a3542d0000303900007d000000000000000000009abc00000000050013000b0009000f000b", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"kadabra-yellow-twistedspoon", "2600321400181860645d00003039001554000000000000000000009abc000000001400320016001500330039", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"pikachu-oddegg-dizzypunch", "54002c1400171700542d92003039001f400000000000000000000000000000000014002c001b001100290019", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"mrmime-trade", "2a001e0a0018182d5d70000030390003e8000000000000000000009abc000000000a001e000f00140019001b", "5d50505050505050505050", "8c809182848b5050505050", true},
    {"mrmime-trade-renamed", "2a001e0a0018182d5d70000030390003e8000000000000000000009abc000000000a001e000f00140019001b", "5d50505050505050505050", "8c888c8882505050505050", false},
    {"mrmime-normal-ot", "2a001e0a0018182d5d70000030390003e8000000000000000000009abc000000000a001e000f00140019001b", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"mrmime-normal-ot-tradeback", "2a00280f001818005d7000003039000d2f000000000000000000009abc000000000f00280015001b00230026", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"porygon-borrowed-types", "aa003c140014142d21a09f003039001f40000000000000000000009abc0000000014003c0020002500190027", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"porygon-impossible-types", "aa003c140014152d21a09f003039001f40000000000000000000009abc0000000014003c0020002500190027", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"mew-europe-tour", "150019050018182d01000000303900008700000000000000000000a1c5000000000500190010000f0010000f", "988e928788918050505050", "8c8e8d5050505050505050", true},
    {"mew-wrong-dvs", "15001a050018182d01000000303900008700000000000000000000ffff0000000005001a0010001000100010", "988e928788918050505050", "8c8e8d5050505050505050", false},
    {"psyduck-stadium", "2f002b0f001515a80a85000007d0000d2f000000000000000000009abc000000000f002b0017001600180017", "9293808388948c50505050", "8c8e8d5050505050505050", true},
    {"pidgey-wrong-types", "24001405001414ff211c00003039000087000000000000000000009abc00000000050014000a000a000b0009", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"magnemite-steel-stadium2", "ad00341900170900213100003039003d09000000000000000000009abc00000000190034001b002d0021003a", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"dragonite-catch-rate", "4200bf3c001a022d23562b523039041eb0000000000000000000009abc000000003c00bf00b000830072008b", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"pikachu-gen2-tm-thunderpunch", "5400301400171700542d56093039001f40000000000000000000009abc00000000140030001e0015002d001d", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"pikachu-gen2-tm-no-item", "54003014001717be542d56093039001f40000000000000000000009abc00000000140030001e0015002d001d", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"level-exp-mismatch", "a5000f03000000ff212700003039001388000000000000000000009abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", true},
};

std::vector<uint8_t> hex(const std::string& s) {
    std::vector<uint8_t> out;
    for (std::size_t i = 0; i + 1 < s.size(); i += 2)
        out.push_back(static_cast<uint8_t>(std::stoi(s.substr(i, 2), nullptr, 16)));
    return out;
}

uint16_t be16(const std::vector<uint8_t>& b, std::size_t o) {
    return static_cast<uint16_t>((b[o] << 8) | b[o + 1]);
}

// Field layout of a Gen I party record (pret pokered, PKHeX PK1).
Pokemon::Pokemon1ReadOnly decode(const Case& c) {
    using namespace PokeVault::Integration::Gen1;
    const auto b = hex(c.body), ot = hex(c.ot), nick = hex(c.nick);
    PokemonRecord r{};
    r.rawSpecies = b[0];
    r.species = gen1InternalToNational(b[0]);
    r.currentHP = be16(b, 0x01);
    r.type1 = b[5];
    r.type2 = b[6];
    r.catchRate = b[7];
    for (int i = 0; i < 4; ++i) {
        r.moves[i] = b[8 + i];
        r.pp[i] = b[0x1D + i] & 0x3F;
    }
    r.trainerId = be16(b, 0x0C);
    r.experience = (static_cast<uint32_t>(b[0x0E]) << 16) | (b[0x0F] << 8) | b[0x10];
    const uint16_t dv = be16(b, 0x1B);
    const uint8_t atk = dv >> 12, def = (dv >> 8) & 15, spe = (dv >> 4) & 15, spc = dv & 15;
    r.dvs = {static_cast<uint8_t>(((atk & 1) << 3) | ((def & 1) << 2) | ((spe & 1) << 1) | (spc & 1)),
             atk, def, spe, spc};
    r.level = b[0x21];
    r.partyRecord = true;
    r.maxHP = be16(b, 0x22);
    r.originalTrainer = decodeGen1String(ot, RegionLayout::International);
    r.nickname = decodeGen1String(nick, RegionLayout::International);
    for (std::size_t i = 0; i < b.size(); ++i)
        r.rawBody[i] = b[i];
    r.rawBodySize = b.size();
    return Pokemon::Pokemon1ReadOnly(r);
}
}  // namespace

int main() {
    int checked = 0;
    for (const Case& c : kCases) {
        const auto pk = decode(c);
        // The Gen I verdict cannot depend on which save the Pokemon currently sits in.
        for (const char* label : {"", "red_gb", "yellow_gb"}) {
            const auto report = Legality::analyze(pk, Enums::GameVersion::RBY, label);
            const auto verdict = report.verdict();
            const std::string name = c.name;
            // Deliberately stricter than PKHeX: level must match EXP.
            const bool expectValid = name == "level-exp-mismatch" ? false : c.pkhexValid;
            if ((verdict == Legality::Verdict::NoProblemsFound) != expectValid ||
                verdict == Legality::Verdict::Incomplete) {
                std::cerr << "MISMATCH " << c.name << " [" << label << "]: expected "
                          << (expectValid ? "valid" : "invalid") << "\n";
                for (const auto& i : report.issues)
                    std::cerr << "    " << static_cast<int>(i.severity) << " " << i.text << "\n";
                return 1;
            }
            ++checked;
        }
    }
    std::cout << "Gen I legality vs PKHeX verdicts: " << checked << " checks PASS\n";
    return 0;
}
