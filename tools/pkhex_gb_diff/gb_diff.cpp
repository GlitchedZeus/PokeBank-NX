// Reads gen_corpus.py (PK1) / gen_corpus2.py (PK2) lines on stdin and prints PokeBank NX's
// verdict for each, in the same shape as the PKHeX oracle: <id> VALID|INVALID <origin> ; <problems>
// UNCERTAIN marks a record whose every origin was excluded only by a rule PokeBank NX does not
// treat as definitive (see GB::Status::Uncertain); compare.py audits those separately.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Keep a decoder independently generated from the chosen oracle snapshot here;
// production_parity.cpp separately exercises the native Korean save decoder.
#include "gb_korean_table.inc"

namespace {
uint16_t be16(const std::vector<uint8_t>& b, std::size_t at) {
    return static_cast<uint16_t>((b[at] << 8) | b[at + 1]);
}

const char* verdict(Legality::GB::Status s) {
    using Legality::GB::Status;
    return s == Status::Legal ? " VALID " : s == Status::Uncertain ? " UNCERTAIN " : " INVALID ";
}

/// StringConverter2KOR.LoadString: bytes 1-11 select a page for the next byte (a Hangul
/// syllable or jamo); anything else is a one-byte character; 0 or a blank cell ends it.
std::string decodeKorean(const std::vector<uint8_t>& data) {
    std::string out;
    for (std::size_t i = 0; i < data.size(); ++i) {
        uint8_t page = 0, value = data[i];
        if (value == 0)
            break;
        if (value <= 11) {
            if (++i == data.size())
                break;
            page = value;
            value = data[i];
        }
        const char* c = kKoreanTable[page][value];
        if (*c == '\0')
            break;
        out += c;
    }
    return out;
}

/// Strict hex: an odd length or a non-hex digit is a protocol error, not a shorter record.
bool hex(const std::string& s, std::vector<uint8_t>& out) {
    out.clear();
    if (s.empty() || s.size() % 2 != 0)
        return false;
    for (std::size_t i = 0; i < s.size(); i += 2) {
        const auto digit = [](char c) {
            return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        };
        const int hi = digit(s[i]), lo = digit(s[i + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out.push_back(static_cast<uint8_t>(hi * 16 + lo));
    }
    return true;
}
}

// withRaw: hand the engine the stored name bytes (the byte rules); false is what the app
// can do today, since its save parsers keep only the decoded names.
Legality::GB::Result checkPk2(const std::vector<uint8_t>& body, const std::vector<uint8_t>& ot,
                              const std::vector<uint8_t>& nick, bool jp, bool egg, bool withRaw = true) {
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
    for (std::size_t i = 0; i < 5; ++i)
        e.statExp[i] = be16(body, 11 + i * 2);
    e.dvs = {static_cast<uint8_t>(body[21] >> 4), static_cast<uint8_t>(body[21] & 15),
             static_cast<uint8_t>(body[22] >> 4), static_cast<uint8_t>(body[22] & 15)};
    e.friendship = body[27];
    e.pokerus = body[28];
    e.caughtData = static_cast<uint16_t>((body[29] << 8) | body[30]);
    e.level = body[31];
    e.partyStats = true;  // the corpus holds party structs
    e.maxHP = be16(body, 36);
    for (std::size_t i = 0; i < 5; ++i)
        e.stats[i] = be16(body, 38 + i * 2);
    e.hpKnown = true;
    e.currentHP = be16(body, 34);
    e.statusKnown = true;
    e.status = body[32];
    e.isEgg = egg;
    e.tradeTrainer = ot[0] == 0x5D;
    // PK2.Korean: not Japanese, and the trainer name starts with a Hangul page byte.
    const bool korean = !jp && !ot.empty() && ot[0] <= 11;
    e.otName = korean ? decodeKorean(ot) : Gen2::decodeGen2String(ot, region);
    e.nickname = korean ? decodeKorean(nick) : Gen2::decodeGen2String(nick, region);
    e.language = jp ? GB::Language::Japanese : korean ? GB::Language::Korean : GB::Language::International;
    if (withRaw) {
        e.otRaw = ot;
        e.nickRaw = nick;
    }
    return GB::checkGen2(e);
}

int main(int argc, char** argv) {
    const bool proofs = argc == 2 && std::string(argv[1]) == "--proof";
    const auto evidence = [proofs](const std::string& id, const Legality::GB::Result& r) {
        if (!proofs || !r.breedingProof.supported) return;
        std::cerr << id << " father=" << r.breedingProof.father;
        for (const auto& s : r.breedingProof.steps)
            std::cerr << " " << static_cast<int>(s.kind) << ":" << s.species << ":" << static_cast<int>(s.mask) << "<-" << s.parentSpecies << ":" << static_cast<int>(s.parentMask);
        std::cerr << "\n";
    };
    using namespace PokeVault::Integration::Gen1;
    namespace GB = Legality::GB;
    std::string line;
    std::size_t lineNumber = 0;
    // Every corpus line gets a verdict or the run stops: compare.py relies on it.
    const auto reject = [&](const char* why) {
        std::cerr << "gb_diff: corpus line " << lineNumber << ": " << why << '\n';
        return 2;
    };
    while (std::getline(std::cin, line)) {
        ++lineNumber;
        std::istringstream in(line);
        std::string id, kind, jp, bodyHex, otHex, nickHex, eggFlag, extra;
        if (!(in >> id >> kind >> jp >> bodyHex >> otHex >> nickHex))
            return reject("fewer than 6 fields");
        in >> eggFlag;
        if (in >> extra)
            return reject("more than 7 fields");
        std::vector<uint8_t> body, ot, nick;
        if (!hex(bodyHex, body) || !hex(otHex, ot) || !hex(nickHex, nick))
            return reject("malformed hex");
        if (ot.size() != nick.size() || (ot.size() != 6 && ot.size() != 11))
            return reject("name fields must both be 6 or 11 bytes");
        if (!(body.size() == 48 && (eggFlag == "0" || eggFlag == "1")) && !(body.size() == 44 && eggFlag.empty()))
            return reject("body length does not fit the field count");
        if (body.size() == 48) {
            const GB::Result r = checkPk2(body, ot, nick, jp == "1", eggFlag == "1");
            evidence(id,r);
            std::cout << id << verdict(r.status) << GB::describe(r.origin)
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
        e.partyStats = true;
        e.maxHP = be16(body, 0x22);
        for (std::size_t i = 0; i < 4; ++i)
            e.stats[i] = be16(body, 0x24 + i * 2);
        e.hpKnown = true;
        e.currentHP = be16(body, 1);
        e.statusKnown = true;
        e.status = body[4];
        e.boxLevelKnown = true;
        e.boxLevel = body[3];
        e.tradeTrainer = ot[0] == 0x5D;
        e.otName = decodeGen1String(ot, region);
        e.nickname = decodeGen1String(nick, region);
        e.language = jp == "1" ? GB::Language::Japanese : GB::Language::International;
        e.otRaw = ot;
        e.nickRaw = nick;
        const GB::Result r = GB::checkGen1(e);
        evidence(id,r);
        std::cout << id << verdict(r.status) << GB::describe(r.origin)
                  << (r.timeCapsule ? " [time capsule]" : "") << " ;";
        for (const auto& p : r.problems)
            std::cout << " | " << p.text;
        std::cout << '\n';
    }
}
