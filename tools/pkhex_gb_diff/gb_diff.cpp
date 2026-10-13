// Reads gen_corpus.py (PK1) / gen_corpus2.py (PK2) lines on stdin and prints PokeBank NX's
// verdict for each, in the same shape as the PKHeX oracle: <id> VALID|INVALID <origin> ; <problems>
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<uint8_t> hex(const std::string& s) {
    std::vector<uint8_t> out;
    for (std::size_t i = 0; i + 1 < s.size(); i += 2)
        out.push_back(static_cast<uint8_t>(std::stoi(s.substr(i, 2), nullptr, 16)));
    return out;
}
}

Legality::GB::Result checkPk2(const std::vector<uint8_t>& body, const std::vector<uint8_t>& ot,
                              const std::vector<uint8_t>& nick, bool jp, bool egg) {
    namespace Gen2 = PokeVault::Integration::Gen2;
    namespace GB = Legality::GB;
    const auto region = jp ? Gen2::RegionLayout::Japanese : Gen2::RegionLayout::International;
    GB::Entity2 e;
    e.species = body[0];
    e.heldItem = body[1];
    for (int i = 0; i < 4; ++i) {
        e.moves[i] = body[2 + i];
        e.pp[i] = body[23 + i] & 0x3F;
        e.ppUps[i] = body[23 + i] >> 6;
    }
    e.tid = static_cast<uint16_t>((body[6] << 8) | body[7]);
    e.exp = (static_cast<uint32_t>(body[8]) << 16) | (body[9] << 8) | body[10];
    e.dvs = {static_cast<uint8_t>(body[21] >> 4), static_cast<uint8_t>(body[21] & 15),
             static_cast<uint8_t>(body[22] >> 4), static_cast<uint8_t>(body[22] & 15)};
    e.friendship = body[27];
    e.caughtData = static_cast<uint16_t>((body[29] << 8) | body[30]);
    e.level = body[31];
    e.isEgg = egg;
    e.tradeTrainer = ot[0] == 0x5D;
    e.otName = Gen2::decodeGen2String(ot, region);
    e.nickname = Gen2::decodeGen2String(nick, region);
    e.language = jp ? GB::Language::Japanese : GB::Language::International;
    return GB::checkGen2(e);
}

int main() {
    using namespace PokeVault::Integration::Gen1;
    namespace GB = Legality::GB;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string id, kind, jp, bodyHex, otHex, nickHex, eggFlag;
        if (!(in >> id >> kind >> jp >> bodyHex >> otHex >> nickHex))
            continue;
        in >> eggFlag;
        const auto body = hex(bodyHex), ot = hex(otHex), nick = hex(nickHex);
        if (body.size() == 48) {
            const GB::Result r = checkPk2(body, ot, nick, jp == "1", eggFlag == "1");
            std::cout << id << (r.status == GB::Status::Legal ? " VALID " : " INVALID ") << GB::describe(r.origin)
                      << (r.timeCapsule ? " [time capsule]" : "") << " ;";
            for (const auto& p : r.problems)
                std::cout << " | " << p.text;
            std::cout << '\n';
            continue;
        }
        const auto region = jp == "1" ? RegionLayout::Japanese : RegionLayout::International;
        GB::Entity1 e;
        e.species = static_cast<uint8_t>(gen1InternalToNational(body[0]));
        e.level = body[0x21];
        e.type1 = body[5];
        e.type2 = body[6];
        e.catchRate = body[7];
        for (int i = 0; i < 4; ++i) {
            e.moves[i] = body[8 + i];
            e.pp[i] = body[0x1D + i] & 0x3F;
            e.ppUps[i] = body[0x1D + i] >> 6;
        }
        e.tid = static_cast<uint16_t>((body[0x0C] << 8) | body[0x0D]);
        e.dvs = {static_cast<uint8_t>(body[0x1B] >> 4), static_cast<uint8_t>(body[0x1B] & 15),
                 static_cast<uint8_t>(body[0x1C] >> 4), static_cast<uint8_t>(body[0x1C] & 15)};
        e.tradeTrainer = ot[0] == 0x5D;
        e.otName = decodeGen1String(ot, region);
        e.nickname = decodeGen1String(nick, region);
        e.language = jp == "1" ? GB::Language::Japanese : GB::Language::International;
        const GB::Result r = GB::checkGen1(e);
        std::cout << id << (r.status == GB::Status::Legal ? " VALID " : " INVALID ") << GB::describe(r.origin)
                  << (r.timeCapsule ? " [time capsule]" : "") << " ;";
        for (const auto& p : r.problems)
            std::cout << " | " << p.text;
        std::cout << '\n';
    }
}
