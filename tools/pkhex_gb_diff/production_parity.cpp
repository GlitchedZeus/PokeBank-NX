// Production parity for the Gen I/II differential.
//
// gb_diff.cpp checks GB::checkGen1/checkGen2 against PKHeX on raw corpus bytes. The app
// does not call those functions on bytes: it parses a save into PokemonRecord, wraps it in
// Pokemon1ReadOnly/Pokemon2ReadOnly and calls Legality::analyze. This driver runs the same
// corpus through that production path and compares its verdict with the origin search's:
//   Legal -> No problems found, Illegal -> Invalid, Uncertain -> Incomplete.
// Korean PK2 records are skipped (PokeBank NX does not read Korean saves). The app tells
// Japanese from international records by their decoded names; when the names show neither,
// the language is unknown and the search accepts both, so a production verdict that is
// only more lenient than the search is counted, not failed.
// Exit 1 on any production Invalid the search does not give, or any other disagreement.

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Reuse gb_diff.cpp's corpus decoding (hex, be16, Korean names, checkPk2) unchanged.
#define main gb_diff_main
#include "gb_diff.cpp"
#undef main

// Legality.cpp keeps historical Trainer namespace wrappers (same shim as the
// exact-source legality tests).
namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

namespace {
namespace GB = Legality::GB;
namespace Gen1 = PokeVault::Integration::Gen1;
namespace Gen2 = PokeVault::Integration::Gen2;

const char* word(GB::Status s) {
    return s == GB::Status::Legal ? "LEGAL" : s == GB::Status::Illegal ? "ILLEGAL" : "UNCERTAIN";
}
const char* word(Legality::Verdict v) {
    return v == Legality::Verdict::NoProblemsFound ? "LEGAL" : v == Legality::Verdict::Invalid ? "ILLEGAL" : "UNCERTAIN";
}
// Same rule as Legality.cpp gen1Language: kana means Japanese, a Latin capital international.
bool namesShowLanguage(const std::string& ot, const std::string& nick) {
    for (const std::string* s : {&ot, &nick})
        for (unsigned char c : *s)
            if (c == 0xE3 || (c >= 'A' && c <= 'Z'))
                return true;
    return false;
}
}

int main() {
    std::map<std::string, std::size_t> table;
    std::size_t total = 0, skippedKorean = 0, lenientUnknownLanguage = 0, failures = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string id, kind, jp, bodyHex, otHex, nickHex, eggFlag;
        if (!(in >> id >> kind >> jp >> bodyHex >> otHex >> nickHex)) {
            std::cerr << "production_parity: malformed corpus line\n";
            return 2;
        }
        in >> eggFlag;
        std::vector<uint8_t> body, ot, nick;
        if (!hex(bodyHex, body) || !hex(otHex, ot) || !hex(nickHex, nick)) {
            std::cerr << "production_parity: malformed hex\n";
            return 2;
        }
        const bool japanese = jp == "1";
        GB::Status search;
        Legality::Report report;
        std::string otName, nickname;
        if (body.size() == 48) {
            if (!japanese && !ot.empty() && ot[0] <= 11) {
                ++skippedKorean;
                continue;
            }
            const bool egg = eggFlag == "1";
            search = checkPk2(body, ot, nick, japanese, egg).status;
            const auto region = japanese ? Gen2::RegionLayout::Japanese : Gen2::RegionLayout::International;
            // Field for field what Gen2ReadOnlySave.cpp parsePokemon stores.
            Gen2::PokemonRecord rec;
            rec.species = body[0];
            rec.heldItem = body[1];
            std::copy_n(body.begin() + 2, 4, rec.moves.begin());
            rec.trainerId = static_cast<uint16_t>((body[6] << 8) | body[7]);
            rec.experience = (static_cast<uint32_t>(body[8]) << 16) | (body[9] << 8) | body[10];
            for (std::size_t i = 0; i < 5; ++i)
                rec.statExperience[i] = be16(body, 11 + i * 2);
            rec.dvs[1] = body[21] >> 4;
            rec.dvs[2] = body[21] & 15;
            rec.dvs[3] = body[22] >> 4;
            rec.dvs[4] = body[22] & 15;
            rec.dvs[0] = static_cast<uint8_t>(((rec.dvs[1] & 1) << 3) | ((rec.dvs[2] & 1) << 2) |
                                              ((rec.dvs[3] & 1) << 1) | (rec.dvs[4] & 1));
            for (std::size_t i = 0; i < 4; ++i) {
                rec.pp[i] = body[23 + i] & 0x3F;
                rec.ppUps[i] = body[23 + i] >> 6;
            }
            rec.friendship = body[27];
            rec.pokerus = body[28];
            rec.caughtData = static_cast<uint16_t>((body[29] << 8) | body[30]);
            rec.level = body[31];
            rec.isEgg = egg;
            rec.originalTrainer = Gen2::decodeGen2String(ot, region);
            rec.nickname = Gen2::decodeGen2String(nick, region);
            rec.partyRecord = true;
            rec.rawBodySize = body.size();
            std::copy(body.begin(), body.end(), rec.rawBody.begin());
            otName = rec.originalTrainer;
            nickname = rec.nickname;
            if (rec.species == 0 || rec.species > 251 || rec.level > 100) {
                // The save parser rejects these slots before legality runs.
                ++table["parser-rejected"];
                continue;
            }
            const Pokemon::Pokemon2ReadOnly pk(rec);
            report = Legality::analyze(pk, pk.getGameGroup(), "");
        } else {
            const auto region = japanese ? Gen1::RegionLayout::Japanese : Gen1::RegionLayout::International;
            GB::Entity1 e;
            e.species = static_cast<uint8_t>(Gen1::gen1InternalToNational(body[0]));
            e.level = body[0x21];
            e.exp = (static_cast<uint32_t>(body[0x0E]) << 16) | (body[0x0F] << 8) | body[0x10];
            e.type1 = body[5];
            e.type2 = body[6];
            e.catchRate = body[7];
            for (int i = 0; i < 4; ++i) {
                e.moves[i] = body[8 + i];
                e.pp[i] = body[0x1D + i] & 0x3F;
                e.ppUps[i] = body[0x1D + i] >> 6;
            }
            e.tid = static_cast<uint16_t>((body[0x0C] << 8) | body[0x0D]);
            for (std::size_t i = 0; i < 5; ++i)
                e.statExp[i] = be16(body, 0x11 + i * 2);
            e.dvs = {static_cast<uint8_t>(body[0x1B] >> 4), static_cast<uint8_t>(body[0x1B] & 15),
                     static_cast<uint8_t>(body[0x1C] >> 4), static_cast<uint8_t>(body[0x1C] & 15)};
            e.tradeTrainer = ot[0] == 0x5D;
            e.otName = Gen1::decodeGen1String(ot, region);
            e.nickname = Gen1::decodeGen1String(nick, region);
            e.language = japanese ? GB::Language::Japanese : GB::Language::International;
            search = GB::checkGen1(e).status;
            Gen1::PokemonRecord rec;
            rec.rawSpecies = body[0];
            rec.species = Gen1::gen1InternalToNational(body[0]);
            rec.currentHP = be16(body, 1);
            rec.status = body[4];
            rec.type1 = body[5];
            rec.type2 = body[6];
            rec.catchRate = body[7];
            for (int i = 0; i < 4; ++i)
                rec.moves[static_cast<std::size_t>(i)] = body[8 + i];
            rec.trainerId = be16(body, 0x0C);
            rec.experience = e.exp;
            rec.statExperience = e.statExp;
            rec.dvs = {static_cast<uint8_t>(((e.dvs[0] & 1) << 3) | ((e.dvs[1] & 1) << 2) |
                                            ((e.dvs[2] & 1) << 1) | (e.dvs[3] & 1)),
                       e.dvs[0], e.dvs[1], e.dvs[2], e.dvs[3]};
            rec.pp = e.pp;
            rec.ppUps = e.ppUps;
            rec.level = body[0x21];
            rec.originalTrainer = e.otName;
            rec.nickname = e.nickname;
            rec.partyRecord = true;
            rec.rawBodySize = body.size();
            std::copy(body.begin(), body.end(), rec.rawBody.begin());
            otName = rec.originalTrainer;
            nickname = rec.nickname;
            if (rec.species == 0 || rec.level > 100) {
                ++table["parser-rejected"];
                continue;
            }
            const Pokemon::Pokemon1ReadOnly pk(rec);
            report = Legality::analyze(pk, pk.getGameGroup(), "");
        }
        ++total;
        const std::string want = word(search), got = word(report.verdict());
        const std::string format = body.size() == 48 ? "PK2" : "PK1";
        ++table[format + " search " + want + " / production " + got];
        if (want == got)
            continue;
        // Production may only be more lenient, and only when the names hide the language.
        const bool lenient = got != "ILLEGAL" && (want == "ILLEGAL" || got == "LEGAL");
        if (lenient && !namesShowLanguage(otName, nickname)) {
            ++lenientUnknownLanguage;
            continue;
        }
        if (++failures <= 12) {
            std::cout << "MISMATCH " << id << ' ' << format << " search " << want << " production " << got << '\n';
            for (const auto& issue : report.issues)
                if (issue.severity != Legality::Severity::Info)
                    std::cout << "    " << (issue.severity == Legality::Severity::Invalid ? "Invalid: " : "Warning: ")
                              << issue.text << '\n';
        }
    }
    std::cout << "production parity: " << total << " records through Legality::analyze, "
              << skippedKorean << " Korean skipped, " << lenientUnknownLanguage
              << " lenient because the names hide the language\n";
    for (const auto& [k, v] : table)
        std::cout << "  " << k << ": " << v << '\n';
    if (failures) {
        std::cout << "FAIL: " << failures << " production verdicts differ from the origin search\n";
        return 1;
    }
    std::cout << "PASS: production verdicts match the origin search\n";
    return 0;
}
