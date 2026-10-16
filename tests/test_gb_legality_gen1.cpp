// Gen I legality through the production analyzer, on raw PK1 party records.
//
// Every expected verdict below is what PKHeX.Core (kwsch/PKHeX@6501f0ab, GB cartridge-era
// settings) reports for the same bytes; tools/pkhex_gb_diff reproduces them. The deliberate
// differences are marked:
//  - PKHeX does not compare level with EXP for Gen I, while the games keep them in step
//    (battle, Rare Candy and Day Care all recompute).
//  - A Mew that matches no recorded distribution, even one carrying a tour trainer name,
//    is Incomplete with a warning instead of a verdict either way (PKHeX: invalid; the
//    event list is not known to be complete, and the in-game Mew glitch, which PKHeX does
//    not model, gives any trainer and DVs).
//
// The Gen I evolution-order rule (a pre-evolution's higher-level move next to an evolved
// form's lower-level one) is satisfied by Pokemon Stadium 2's move reminder, which reads
// Red/Blue/Yellow cartridges directly: "exeggutor-stomp-after-poisonpowder" (catch rate
// 90, so it never visited Gen II) learned PoisonPowder as Exeggcute at 37, evolved, and
// was reminded of Stomp. PKHeX calls it valid too.
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
    {"rattata-statexp-battle", "a5000f03000000ff21270000303900001b0205050a0003004dea609abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"rattata-statexp-vitamins", "a5000f03000000ff21270000303900001b640000000a00000000009abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", true},
    {"rattata-statexp-one-zero", "a5000f03000000ff21270000303900001b0205050a0000004dea609abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"rattata-statexp-eleven-vitamins", "a5000f03000000ff21270000303900001b6e0000000000000000009abc0000000003000f0008000700090007", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"rattata-ot-seven-letters", "a5000f03000000ff21270000303900001b000000000000000000009abc0000000003000f0008000700090007", "8081828384858650505050", "8c8e8d5050505050505050", true},
    {"rattata-ot-eight-letters", "a5000f03000000ff21270000303900001b000000000000000000009abc0000000003000f0008000700090007", "8081828384858687505050", "8c8e8d5050505050505050", false},
    {"rattata-ot-empty", "a5000f03000000ff21270000303900001b000000000000000000009abc0000000003000f0008000700090007", "5050505050505050505050", "8c8e8d5050505050505050", false},
    {"rattata-nickname-empty", "a5000f03000000ff21270000303900001b000000000000000000009abc0000000003000f0008000700090007", "8092875050505050505050", "5050505050505050505050", false},
    {"mew-unrecorded-event", "150019050018182d01000000303900008700000000000000000000a1c5000000000500190010000f0010000f", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"mew-unrecorded-event-bad-catch-rate", "150019050018180601000000303900008700000000000000000000a1c5000000000500190010000f0010000f", "8092875050505050505050", "8c8e8d5050505050505050", false},
    {"mew-unrecorded-event-ot-too-long", "150019050018182d01000000303900008700000000000000000000a1c5000000000500190010000f0010000f", "8081828384858687888950", "8c8e8d5050505050505050", false},
    // Japanese records (6-byte names, Japanese Blue encounters, Japanese trade names).
    {"defect-catch-item-jp-2867", "18003e1d0015036a33638437cb860077161448b28151b2fe8b879ea884000000001d003e0022001d00320041", "09e3a6135050", "09ab50505050", true},  // PKHeX: EncounterSlot1:72:RD:15
    {"defect-catch-jp-2435", "93004f2400080355686d655f2bf7009c474c47eddbf61b53cf6b02e8a50000000024004f0033002b0050005b", "858c9e505050", "09ab50505050", true},  // PKHeX: EncounterSlot1:92:RD:20
    {"defect-catch-jp-2783", "07007727000304322b635c18461600cb9f000000000000000000001e5000000000270077004d004b004b003f", "8f888b505050", "418550505050", false},  // PKHeX: EncounterTrade1:32:RB:2
    {"defect-drop-move-jp-2608", "91005c23000404be7d2d2b00db0f00a77b000000000000000000005c4a0000000023005c0040005a0027002f", "b1b6c8505050", "4388a1ab5050", true},  // PKHeX: EncounterSlot1:104:RD:20
    {"defect-drop-move-jp-2631", "8800e843000303be6a32670026a10496db00000000000000000000dc3d000000004300e800a30079004c006d", "8a938b505050", "418550505050", false},  // PKHeX: EncounterTrade1:89:YW:5
    {"defect-level-jp-2451", "58001204001a1a2d236856612c780000503bc649fa3e267f288b7f392800000000040012000a000900090009", "8f888b505050", "4388a1ab5050", false},  // PKHeX: EncounterInvalid:147:RBY:0
    {"defect-level-jp-2830", "2e00541f0007164b0aa38d9360ce00745fad64408cfd030005e18ff6b0000000001f00540049003a001e0036", "8b08a6505050", "b1b23eb35050", true},  // PKHeX: EncounterSlot1:46:BU:22
    {"defect-move-jp-2473", "a6004a1e000000ff815e629ea380006978140050005a00000028000c38000000001e004a0035003000410027", "b1b6c8505050", "418550505050", false},  // PKHeX: EncounterSlot1:19:YW:3
    {"defect-move-jp-3569", "2900a338000101b40243428234660294c7000000000000000000002f95000000003800a30077006400410042", "09e3a6135050", "b1b23eb35050", true},  // PKHeX: EncounterSlot1:66:BU:15
    {"defect-nick-empty-jp-2669", "2700883c0005047899786a595efe0338748f1f86ddb2c8ba67cfe2e208000000003c008800870091002f0044", "8b08a6505050", "505050505050", false},  // PKHeX: EncounterSlot1:75:RD:41
    {"defect-nick-long-jp-2494", "4d002e12000000ff0a2d2c06c1490016c80000000000000000000097c20000000012002e0018001400290014", "8a938b505050", "878789898988", false},  // PKHeX: EncounterSlot1:52:BU:10
    {"defect-nick-long-jp-2960", "96005822000002ff111c621210fd00813c0000000000000000000000bb00000000220058002d002a003c002e", "09e3a6135050", "878781868650", true},  // PKHeX: EncounterSlot1:16:BU:13
    {"defect-ot-empty-jp-2503", "a6003d17000000ff2127269eb822002f87000000000000000000004df30000000017003d002c00260038001d", "505050505050", "09ab50505050", false},  // PKHeX: EncounterSlot1:19:RD:3
    {"defect-ot-long-jp-2459", "7400a43f000002be632d1f6158ba03d0bf000000000000000000003f21000000003f00a40093007000850051", "888188808650", "4388a1ab5050", true},  // PKHeX: EncounterSlot1:84:BU:26
    {"defect-ot-long-jp-2779", "1800280f001503be333023a4494a00107a00000000000000000000b8bd000000000f002800140011001d0026", "848289878488", "b1b23eb35050", false},  // PKHeX: EncounterSlot1:72:RD:15
    {"defect-statexp-jp-2542", "18002f12001503be8230242887d7001c7aa000000000000000000015b80000000012002f001300130022002b", "b1b6c8505050", "b1b23eb35050", false},  // PKHeX: EncounterSlot1:72:RD:15
    {"defect-types-jp-2672", "26007034000314643c698273792202086d3c003200640050000000108f00000000340070002a0024007a0091", "8b08a6505050", "418550505050", false},  // PKHeX: EncounterSlot1:64:BU:51
    {"grown-jp-2422", "23007d31000002ff77412b61951a01cb9100000000000000000000cafd0000000031007d0068004e0075004d", "8b08a6505050", "09ab50505050", true},  // PKHeX: EncounterSlot1:21:RD:5
    {"grown-static-jp-2478", "830108510018180381857036388c0a22ed3200460064003c000000a2ab0000000051010800c7009a00e70110", "8b08a6505050", "418550505050", true},  // PKHeX: EncounterStatic1:150:RBY:70
    {"random-jp-2428", "26008745001818646f2d4a00b71b0507531e0014000a00500046006ccf00000000450087003d003e00a600bf", "9e9486505050", "418550505050", false},  // PKHeX: EncounterStatic1:63:RB:6
    {"random-jp-3732", "1b009251001515f26b666400b9880a22edec9d59f17f60f9a5d89fa56000000000510092005e006600980076", "8b08a6505050", "09ab50505050", true},  // PKHeX: EncounterSlot1:120:BU:32
    {"static-jp-2292", "5c002b12001515e19100000059d90016c800000000000000000000df260000000012002b00180023001b0020", "858c9e505050", "418550505050", true},  // PKHeX: EncounterStatic1:116:BU:18
    {"trade-jp-2299", "2a0013050018182d5d700000922a00007d00000000000000000000b4c300000000050013000a000b000f000f", "5d5050505050", "19d819d85050", true},  // PKHeX: EncounterTrade1:122:RB:5
    {"trade-jp-2305", "0f001c08000303eb2d210a00172400013a00000000000000000000b6420000000008001c000e000e000c000b", "5d5050505050", "505050505050", false},  // PKHeX: EncounterTrade1:29:RB:2
    {"trade-wrong-nick-jp-2301", "2a00200b0018182d5d7000003ba700053300000000000000000000dcdb000000000b002000110015001b001d", "5d5050505050", "958da1985050", false},  // PKHeX: EncounterTrade1:122:BU:3
    {"trade-wrong-ot-jp-2300", "2a001d0a0018182d5d7000008eb20003e800000000000000000000560d000000000a001d000f00130017001b", "09e3a6135050", "19d819d85050", false},  // PKHeX: EncounterTrade1:122:BU:3
    {"trade-wrong-ot-jp-2309", "03001a08000303eb2b211e001dd900013a00000000000000000000beab0000000008001a000f000d000e000d", "8a938b505050", "90adac41e350", true},  // PKHeX: EncounterSlot1:32:RD:3
    {"wild-jp-1306", "b900230d001603ff4700000085ca0004ed00000000000000000000ae15000000000d002300140016000d0019", "8b08a6505050", "4388a1ab5050", true},  // PKHeX: EncounterSlot1:43:BU:13
    {"exeggutor-stomp-after-poisonpowder", "0a00b6380016185a8c17734debdf035980f205cc52224bafe33ddb36c5000000003800b60072006a00500096", "8092875050505050505050", "8c8e8d5050505050505050", true},
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
        r.ppUps[i] = b[0x1D + i] >> 6;
    }
    r.trainerId = be16(b, 0x0C);
    r.experience = (static_cast<uint32_t>(b[0x0E]) << 16) | (b[0x0F] << 8) | b[0x10];
    for (std::size_t i = 0; i < 5; ++i)
        r.statExperience[i] = be16(b, 0x11 + i * 2);
    const uint16_t dv = be16(b, 0x1B);
    const uint8_t atk = dv >> 12, def = (dv >> 8) & 15, spe = (dv >> 4) & 15, spc = dv & 15;
    r.dvs = {static_cast<uint8_t>(((atk & 1) << 3) | ((def & 1) << 2) | ((spe & 1) << 1) | (spc & 1)),
             atk, def, spe, spc};
    r.level = b[0x21];
    r.partyRecord = true;
    r.maxHP = be16(b, 0x22);
    // A Japanese save stores 6-byte names, an international one 11-byte names.
    const auto region = ot.size() == 6 ? RegionLayout::Japanese : RegionLayout::International;
    r.originalTrainer = decodeGen1String(ot, region);
    r.nickname = decodeGen1String(nick, region);
    for (std::size_t i = 0; i < b.size(); ++i)
        r.rawBody[i] = b[i];
    r.rawBodySize = b.size();
    return Pokemon::Pokemon1ReadOnly(r);
}

bool hasSeverity(const Legality::Report& report, Legality::Severity severity) {
    for (const auto& i : report.issues)
        if (i.severity == severity)
            return true;
    return false;
}

bool uncertainCase(const std::string& name) {
    // rattata-statexp-one-zero: PKHeX rejects battle stat experience next to a zero stat
    // (EVs:EffortShouldBeZero), but pret/pokered divides each enemy base stat among the
    // recipients before adding it (DivideExpDataByNumMonsGainingExp), so a battle can add
    // 0 to one stat. Neither verdict is proven from the record alone.
    // rattata-statexp-eleven-vitamins: HP 28160 is above the 25600 vitamin cap, so part of
    // it came from battles, while the other stats are 0. That is the same case: e.g. a
    // Jigglypuff (HP 115, others below 72) split six ways under Exp. All adds 1 HP and 0
    // elsewhere per battle.
    // defect-statexp-jp-2542 is the same zero-next-to-battle-gains case on a Japanese record.
    return name == "mew-unrecorded-event" || name == "mew-wrong-dvs" ||
           name == "rattata-statexp-one-zero" || name == "rattata-statexp-eleven-vitamins" ||
           name == "defect-statexp-jp-2542";
}
}  // namespace

int main() {
    int checked = 0, incomplete = 0;
    for (const Case& c : kCases) {
        const auto pk = decode(c);
        // The Gen I verdict cannot depend on which save the Pokemon currently sits in.
        for (const char* label : {"", "red_gb", "yellow_gb"}) {
            const auto report = Legality::analyze(pk, Enums::GameVersion::RBY, label);
            const auto verdict = report.verdict();
            const std::string name = c.name;
            if (uncertainCase(name)) {
                // Neither verdict is proven: warnings, no Invalid, and an unfinished origin check.
                if (verdict != Legality::Verdict::Incomplete || hasSeverity(report, Legality::Severity::Invalid) ||
                    !hasSeverity(report, Legality::Severity::Warning) ||
                    report.coverage.encounter != Legality::CoverageLevel::Partial) {
                    std::cerr << "MISMATCH " << c.name << " [" << label << "]: expected Incomplete\n";
                    for (const auto& i : report.issues)
                        std::cerr << "    " << static_cast<int>(i.severity) << " " << i.text << "\n";
                    return 1;
                }
                ++checked;
                ++incomplete;
                continue;
            }
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
    std::cout << "Gen I legality vs PKHeX verdicts: " << checked << " checks PASS (" << incomplete
              << " deliberately Incomplete)\n";
    return 0;
}
