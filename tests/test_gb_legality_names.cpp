// Gen I/II name rules: the stored name bytes and the decoded characters.
//
//  1. Byte rules (GB::Entity1/Entity2 otRaw/nickRaw): the in-game-trade marker must be
//     followed by the terminator (pokered macros/data.asm dname writes "<TRAINER>@";
//     PKHeX EncounterTrade1.IsTrainerNameValid), a byte PKHeX's font tables give no glyph
//     is a warning (the games write none; a decoder stops there), and a Hangul page byte
//     at the start of the trainer name makes the record Korean Gold/Silver's
//     (StringConverter2KOR.IsHangul) -- on a PK1 that needs a Korean-Western trade and a
//     Time Capsule trip (no Korean Red/Blue/Yellow exists), so Uncertain, never Valid.
//  2. Character rules on the decoded names, reachable from Legality::analyze today: a
//     digit in a Western name (pokered data/text/alphabets.asm and pokecrystal
//     data/text/name_input_chars.asm offer none) and a Japanese trainer name outside the
//     kana (StringConverter1/2.GetIsJapanese; PKHeX G1CharOT). Both are warnings: the
//     other languages' and the Japanese naming screens are not disassembled.
// PKHeX verdicts quoted were obtained with tools/pkhex_gb_diff's oracle (kwsch/PKHeX@6501f0ab).
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {

namespace GB = Legality::GB;

int checks = 0, failures = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL %s\n", what.c_str());
    }
}

bool has(const std::vector<GB::Problem>& problems, const std::string& needle, bool definitive) {
    for (const auto& p : problems)
        if (p.text.find(needle) != std::string::npos && p.definitive == definitive)
            return true;
    return false;
}

bool hasIssue(const Legality::Report& r, Legality::Severity s, const std::string& needle) {
    for (const auto& issue : r.issues)
        if (issue.severity == s && issue.text.find(needle) != std::string::npos)
            return true;
    return false;
}

void dump(const GB::Result& r, const std::string& what) {
    std::fprintf(stderr, "  %s: status %d\n", what.c_str(), static_cast<int>(r.status));
    for (const auto& p : r.problems)
        std::fprintf(stderr, "    %s%s\n", p.definitive ? "" : "(warning) ", p.text.c_str());
}

// Western Gen I/II bytes: A-Z at 0x80, digits at 0xF6, space 0x7F, terminator 0x50.
std::vector<uint8_t> western(const std::string& s, std::size_t length = 11) {
    std::vector<uint8_t> out;
    for (char c : s) {
        if (c >= 'A' && c <= 'Z')
            out.push_back(static_cast<uint8_t>(0x80 + (c - 'A')));
        else if (c >= '0' && c <= '9')
            out.push_back(static_cast<uint8_t>(0xF6 + (c - '0')));
        else
            out.push_back(0x7F);
    }
    while (out.size() < length)
        out.push_back(0x50);
    return out;
}

std::vector<uint8_t> padded(std::vector<uint8_t> bytes, std::size_t length = 11) {
    while (bytes.size() < length)
        bytes.push_back(0x50);
    return bytes;
}

const std::vector<uint8_t> kHangulGold = padded({0x01, 0x61, 0x03, 0x45});  // 골드 (StringConverter2KOR)

// Rattata caught on Red's Route 1 at level 3 (the gen1 suite's rattata-route1).
GB::Entity1 rattata(const std::string& ot, const std::string& nick, bool raw) {
    GB::Entity1 e;
    e.species = 19;
    e.level = 3;
    e.moves = {33, 39, 0, 0};
    e.pp = {35, 30, 0, 0};
    e.exp = 27;
    e.catchRate = 255;
    e.type1 = e.type2 = 0;
    e.dvs = {9, 10, 11, 12};
    e.tid = 0x3039;
    e.otName = ot;
    e.nickname = nick;
    e.language = GB::Language::International;
    if (raw) {
        e.otRaw = western(ot);
        e.nickRaw = western(nick);
    }
    return e;
}

// Mr. Mime from the Route 2 in-game trade at level 10 (the gen1 suite's mrmime-trade).
GB::Entity1 mrMime() {
    GB::Entity1 e;
    e.species = 122;
    e.level = 10;
    e.moves = {93, 112, 0, 0};
    e.pp = {25, 20, 0, 0};
    e.exp = 1000;
    e.catchRate = 45;
    e.type1 = e.type2 = 0x18;
    e.dvs = {9, 10, 11, 12};
    e.tid = 0x3039;
    e.tradeTrainer = true;
    e.otName = "*";
    e.nickname = "MARCEL";
    e.language = GB::Language::International;
    e.otRaw = padded({0x5D});
    e.nickRaw = western("MARCEL");
    return e;
}

// Pidgey from a Gold wild slot at level 5 (the structural suite's native PK2 Pidgey).
GB::Entity2 pidgey(const std::string& ot, const std::string& nick, bool raw) {
    GB::Entity2 e;
    e.species = 16;
    e.level = 5;
    e.moves = {33, 28, 0, 0};
    e.pp = {35, 15, 0, 0};
    e.exp = 135;
    e.dvs = {8, 8, 8, 8};
    e.tid = 1234;
    e.friendship = 70;
    e.otName = ot;
    e.nickname = nick;
    e.language = GB::Language::International;
    if (raw) {
        e.otRaw = western(ot);
        e.nickRaw = western(nick);
    }
    return e;
}

// ---------------------------------------------------------------- 1. byte rules

void tradeMarker() {
    GB::Entity1 good = mrMime();
    GB::Result r = GB::checkGen1(good);
    check(r.status == GB::Status::Legal, "Mr. Mime trade with <TRAINER>@: Legal");
    if (r.status != GB::Status::Legal)
        dump(r, "mrmime");
    // The marker followed by a letter: the games never write it (PKHeX: no trade match, then
    // Trainer:G1CharOT).
    GB::Entity1 bad = mrMime();
    bad.otRaw = padded({0x5D, 0x80});
    r = GB::checkGen1(bad);
    check(r.status == GB::Status::Illegal && has(r.problems, "must be followed by the name terminator", true),
          "Mr. Mime trade with <TRAINER>A: Illegal");
    // Without the bytes the decoded "*" is all there is: the rule is skipped (production
    // today), the trade still matches.
    GB::Entity1 plain = mrMime();
    plain.otRaw.clear();
    plain.nickRaw.clear();
    check(GB::checkGen1(plain).status == GB::Status::Legal, "Mr. Mime trade without name bytes: Legal");
    // A one-byte field holding only the marker is as the Japanese decoder sees it.
    GB::Entity1 oneByte = mrMime();
    oneByte.otRaw = {0x5D};
    check(GB::checkGen1(oneByte).status == GB::Status::Illegal, "marker alone in a one-byte field: Illegal");
    // PK2: the same marker is the Gen I trade's, and the same byte rule applies.
    GB::Entity2 p = pidgey("*", "PIDGEY", true);
    p.tradeTrainer = true;
    p.otRaw = padded({0x5D, 0x80});
    r = GB::checkGen2(p);
    check(r.status == GB::Status::Illegal && has(r.problems, "must be followed by the name terminator", true),
          "PK2 <TRAINER>A: Illegal");
}

void glyphlessBytes() {
    // "A" then a control byte, as a Western decoder reads "A" and stops. PKHeX: VALID (reads "A").
    GB::Entity1 e = rattata("A", "MON", true);
    e.otRaw = padded({0x80, 0x05, 0x81});
    GB::Result r = GB::checkGen1(e);
    check(r.status == GB::Status::Uncertain && has(r.problems, "no glyph in the Western font", false),
          "PK1 control byte inside the trainer name: Uncertain");
    if (r.status != GB::Status::Uncertain)
        dump(r, "control byte");
    // Each table's own NUL entries, Gen I: 0x4E and 0xE4 have no glyph; in Gen II they do.
    for (uint8_t b : {0x4E, 0xE4}) {
        GB::Entity1 one = rattata("A", "MON", true);
        one.nickRaw = padded({0x8C, b, 0x8D});
        check(GB::checkGen1(one).status == GB::Status::Uncertain, "PK1 nickname byte " + std::to_string(b) + " is glyphless");
        GB::Entity2 two = pidgey("GOLD", "M", true);
        two.nickRaw = padded({0x8C, b, 0x8D});
        check(GB::checkGen2(two).status == GB::Status::Legal, "PK2 nickname byte " + std::to_string(b) + " has a glyph");
    }
    // 0xDE: a glyph in Gen I (left arrow), none in Gen II.
    GB::Entity1 arrow1 = rattata("A", "MON", true);
    arrow1.nickRaw = padded({0x8C, 0xDE, 0x8D});
    check(GB::checkGen1(arrow1).status == GB::Status::Legal, "PK1 nickname byte 0xDE has a glyph");
    GB::Entity2 arrow2 = pidgey("GOLD", "M", true);
    arrow2.nickRaw = padded({0x8C, 0xDE, 0x8D});
    check(GB::checkGen2(arrow2).status == GB::Status::Uncertain, "PK2 nickname byte 0xDE is glyphless");
    // Every byte a Western name can hold stays Legal: letters, digits aside, accents,
    // punctuation, the ligatures, the space.
    for (unsigned b = 0x70; b <= 0xF5; ++b) {
        if (b == 0x74 || (b >= 0x76 && b <= 0x78) || (b >= 0xD7 && b <= 0xDD) || b == 0xE4 || b == 0xE5)
            continue;
        GB::Entity1 one = rattata("A", "MON", true);
        one.nickRaw = padded({0x8C, static_cast<uint8_t>(b), 0x8D});
        check(GB::checkGen1(one).status == GB::Status::Legal, "PK1 nickname byte " + std::to_string(b) + " is a glyph");
    }
    // Without the bytes nothing is known: Legal.
    check(GB::checkGen1(rattata("A", "MON", false)).status == GB::Status::Legal, "PK1 without name bytes: Legal");
    // The terminator ends the scan: bytes after it are padding, whatever they hold.
    GB::Entity1 after = rattata("AB", "MON", true);
    after.otRaw = {0x80, 0x81, 0x50, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05};
    check(GB::checkGen1(after).status == GB::Status::Legal, "PK1 bytes after the terminator are ignored");
    // Japanese font: 0x14 has no glyph, 0x8A (サ) has one.
    GB::Entity1 jp = rattata("サシ", "モン", false);
    jp.language = GB::Language::Japanese;
    jp.otRaw = {0x8A, 0x14, 0x8B, 0x50, 0x50, 0x50};
    jp.nickRaw = {0x92, 0xAB, 0x50, 0x50, 0x50, 0x50};
    r = GB::checkGen1(jp);
    check(r.status == GB::Status::Uncertain && has(r.problems, "no glyph in the Japanese font", false),
          "PK1 Japanese trainer name with a glyphless byte: Uncertain");
    jp.otRaw = {0x8A, 0x8B, 0x50, 0x50, 0x50, 0x50};
    check(GB::checkGen1(jp).status == GB::Status::Legal, "PK1 Japanese names in kana: Legal");
}

void koreanPk1() {
    // A Korean Gold/Silver Rattata brought to Red through a Western Gold/Silver and the Time
    // Capsule: catch-rate byte 83 is a held item the Capsule wrote. PKHeX: VALID, as a Red
    // Route 1 native (PK1.Korean is set, yet EncounterSlot1:19:RD:3 is accepted).
    GB::Entity1 e = rattata("", "", true);
    e.catchRate = 83;
    e.otRaw = kHangulGold;
    e.nickRaw = kHangulGold;
    GB::Result r = GB::checkGen1(e);
    check(r.status == GB::Status::Uncertain &&
              has(r.problems, "Korean Gold/Silver trainer name on a Red/Blue/Yellow record", false),
          "Korean-named PK1 with a held-item catch rate: Uncertain");
    if (r.status != GB::Status::Uncertain)
        dump(r, "korean pk1");
    check(r.origin.generation == 2, "Korean-named PK1: the origin found is in Gen II");
    check(!has(r.problems, "Original trainer name is empty", true), "Korean-named PK1: the Hangul name is not read as empty");
    // Spore is in no Rattata learnset: Illegal whatever the route.
    GB::Entity1 spore = e;
    spore.moves = {147, 0, 0, 0};
    spore.pp = {15, 0, 0, 0};
    check(GB::checkGen1(spore).status == GB::Status::Illegal, "Korean-named PK1 with Spore: Illegal");
    // Six Hangul characters do not fit the Korean naming screen's five.
    GB::Entity1 longName = e;
    longName.otRaw = {0x01, 0x61, 0x03, 0x45, 0x01, 0x61, 0x03, 0x45, 0x01, 0x61, 0x03};
    r = GB::checkGen1(longName);
    check(r.status == GB::Status::Illegal && has(r.problems, "longer than 5", true),
          "Korean-named PK1 with a six-character trainer name: Illegal");
    // A Japanese record's first byte is kana, never a Hangul page.
    GB::Entity1 jp = rattata("ガ", "モン", false);
    jp.language = GB::Language::Japanese;
    jp.otRaw = {0x05, 0x50, 0x50, 0x50, 0x50, 0x50};
    check(GB::checkGen1(jp).status == GB::Status::Legal, "Japanese ガ is not a Hangul page byte");
}

void koreanPk2() {
    // The stored bytes make the record Korean even when the caller decoded nothing
    // (a Western decoder stops at 0x01): a Gold wild Pidgey is Legal.
    GB::Entity2 e = pidgey("", "", true);
    e.otRaw = kHangulGold;
    e.nickRaw = kHangulGold;
    GB::Result r = GB::checkGen2(e);
    check(r.status == GB::Status::Legal, "Korean PK2 known by its bytes: Legal");
    if (r.status != GB::Status::Legal)
        dump(r, "korean pk2");
    // A Latin letter in the trainer name: outside the Hangul pages (PKHeX G1CharOT; a
    // warning here, the Korean naming screen is not disassembled).
    GB::Entity2 latin = e;
    latin.otRaw = padded({0x01, 0x61, 0x80});
    r = GB::checkGen2(latin);
    check(r.status == GB::Status::Uncertain && has(r.problems, "outside the Hangul pages", false),
          "Korean PK2 trainer name with a Latin letter: Uncertain");
    // Seven Latin letters as a nickname: over the Korean limit of 5 (PKHeX NickLengthLong).
    GB::Entity2 nick = e;
    nick.nickRaw = western("ENEZJEX");
    r = GB::checkGen2(nick);
    check(r.status == GB::Status::Illegal && has(r.problems, "Nickname is longer than 5", true),
          "Korean PK2 with a seven-letter nickname: Illegal");
    // Five Hangul characters fill ten bytes: the limit, Legal.
    GB::Entity2 five = e;
    five.otRaw = {0x01, 0x61, 0x03, 0x45, 0x01, 0x61, 0x03, 0x45, 0x01, 0x61, 0x50};
    check(GB::checkGen2(five).status == GB::Status::Legal, "Korean PK2 five-character trainer name: Legal");
}

// ---------------------------------------------------------------- 2. character rules

void digits() {
    // PKHeX: VALID for every one of these.
    GB::Result r = GB::checkGen1(rattata("A1", "MON", true));
    check(r.status == GB::Status::Uncertain && has(r.problems, "Trainer name has a digit", false),
          "PK1 digit in the trainer name: Uncertain");
    r = GB::checkGen1(rattata("ASH", "MON2", false));
    check(r.status == GB::Status::Uncertain && has(r.problems, "Nickname has a digit", false),
          "PK1 digit in the nickname (decoded only): Uncertain");
    r = GB::checkGen2(pidgey("GOLD", "PIDGEY7", true));
    check(r.status == GB::Status::Uncertain && has(r.problems, "Nickname has a digit", false),
          "PK2 digit in the nickname: Uncertain");
    check(GB::checkGen2(pidgey("GOLD", "PIDGEY", true)).status == GB::Status::Legal, "PK2 letters only: Legal");
    // An egg's name is the game's own: not judged.
    GB::Entity2 egg = pidgey("GOLD", "EGG", true);
    egg.isEgg = true;
    egg.nickname = "EGG";
    egg.otName = "G1";
    egg.otRaw = western("G1");
    r = GB::checkGen2(egg);
    check(has(r.problems, "Trainer name has a digit", false) && !has(r.problems, "Nickname has a digit", false),
          "PK2 egg: the trainer name is judged, the egg name is not");
}

void japaneseTrainerName() {
    // StringConverter1.GetIsJapanese: kana only. PKHeX: Trainer:G1CharOT for a full-width
    // digit or ？ on a PK1; ？ and ！ pass on a PK2 (StringConverter2.GetIsJapanese).
    GB::Entity1 e = rattata("サ１", "モン", false);
    e.language = GB::Language::Japanese;
    e.otRaw = {0x8A, 0xF7, 0x50, 0x50, 0x50, 0x50};
    GB::Result r = GB::checkGen1(e);
    check(r.status == GB::Status::Uncertain && has(r.problems, "outside the kana", false),
          "PK1 Japanese trainer name with a full-width digit: Uncertain");
    e.otName = "サ？";
    e.otRaw = {0x8A, 0xE6, 0x50, 0x50, 0x50, 0x50};
    check(GB::checkGen1(e).status == GB::Status::Uncertain, "PK1 Japanese trainer name with ？: Uncertain");
    e.otName = "サシス";
    e.otRaw = {0x8A, 0x8B, 0x8C, 0x50, 0x50, 0x50};
    check(GB::checkGen1(e).status == GB::Status::Legal, "PK1 Japanese trainer name in kana: Legal");
    e.otName = "ゴールド";  // ー is U+30FC, the last kana PKHeX accepts
    e.otRaw = {0x09, 0xE3, 0xA6, 0x13, 0x50, 0x50};
    check(GB::checkGen1(e).status == GB::Status::Legal, "PK1 Japanese trainer name with ー: Legal");
    GB::Entity2 p = pidgey("サ？", "モン", false);
    p.language = GB::Language::Japanese;
    p.otRaw = {0x8A, 0xE6, 0x50, 0x50, 0x50, 0x50};
    p.nickRaw = {0x92, 0xAB, 0x50, 0x50, 0x50, 0x50};
    check(GB::checkGen2(p).status == GB::Status::Legal, "PK2 Japanese trainer name with ？: Legal");
    p.otName = "サ１";
    p.otRaw = {0x8A, 0xF7, 0x50, 0x50, 0x50, 0x50};
    check(GB::checkGen2(p).status == GB::Status::Uncertain, "PK2 Japanese trainer name with a digit: Uncertain");
    // The nickname is not held to the kana (PKHeX checks the trainer name only).
    p.otName = "サシ";
    p.otRaw = {0x8A, 0x8B, 0x50, 0x50, 0x50, 0x50};
    p.nickname = "モ１";
    p.nickRaw = {0x92, 0xF7, 0x50, 0x50, 0x50, 0x50};
    check(GB::checkGen2(p).status == GB::Status::Legal, "PK2 Japanese nickname with a digit: Legal");
}

// ---------------------------------------------------------------- production path

// What Gen1ReadOnlySave.cpp parsePokemon stores for the Rattata above, decoded names only.
Legality::Report productionPk1(const std::string& ot, const std::string& nick) {
    PokeVault::Integration::Gen1::PokemonRecord rec{};
    rec.rawSpecies = 0xA5;
    rec.species = 19;
    rec.currentHP = 15;
    rec.type1 = rec.type2 = 0;
    rec.catchRate = 255;
    rec.moves = {33, 39, 0, 0};
    rec.trainerId = 0x3039;
    rec.experience = 27;
    rec.dvs = {0x0A, 9, 10, 11, 12};
    rec.pp = {35, 30, 0, 0};
    rec.level = 3;
    rec.originalTrainer = ot;
    rec.nickname = nick;
    rec.partyRecord = true;
    const Pokemon::Pokemon1ReadOnly pk(rec);
    return Legality::analyze(pk, pk.getGameGroup(), "red_gb");
}

Legality::Report productionPk2(const std::string& ot, const std::string& nick) {
    PokeVault::Integration::Gen2::PokemonRecord rec{};
    rec.species = 16;
    rec.level = 5;
    rec.experience = 135;
    rec.moves = {33, 28, 0, 0};
    rec.pp = {35, 15, 0, 0};
    rec.trainerId = 1234;
    rec.dvs = {0, 8, 8, 8, 8};
    rec.friendship = 70;
    rec.originalTrainer = ot;
    rec.nickname = nick;
    const Pokemon::Pokemon2ReadOnly pk(rec);
    return Legality::analyze(pk, Enums::GameVersion::GSC, "gold_gbc");
}

void production() {
    const auto clean = productionPk1("ASH", "MON");
    check(clean.verdict() == Legality::Verdict::NoProblemsFound, "production PK1 letters: No problems found");
    const auto digit = productionPk1("ASH", "RAT2");
    check(digit.verdict() == Legality::Verdict::Incomplete && !digit.hasInvalid() &&
              hasIssue(digit, Legality::Severity::Warning, "Nickname has a digit") &&
              digit.coverage.encounter == Legality::CoverageLevel::Partial,
          "production PK1 digit in the nickname: Incomplete with a Warning");
    const auto jp = productionPk1("サ１", "モン");
    check(jp.verdict() == Legality::Verdict::Incomplete && !jp.hasInvalid() &&
              hasIssue(jp, Legality::Severity::Warning, "outside the kana"),
          "production PK1 Japanese trainer name with a digit: Incomplete");
    const auto jpOk = productionPk1("サシス", "モン");
    check(jpOk.verdict() == Legality::Verdict::NoProblemsFound, "production PK1 Japanese kana: No problems found");
    const auto two = productionPk2("GOLD", "PIDGEY7");
    check(two.verdict() == Legality::Verdict::Incomplete && !two.hasInvalid() &&
              hasIssue(two, Legality::Severity::Warning, "Nickname has a digit"),
          "production PK2 digit in the nickname: Incomplete");
    const auto twoJp = productionPk2("サ？", "モン");
    check(twoJp.verdict() == Legality::Verdict::NoProblemsFound, "production PK2 Japanese trainer name with ？: No problems found");
    // The byte rules need the stored name bytes, which the save parsers do not carry yet
    // (docs/legality/handoff/lane-d-raw-names.patch): a control byte inside a name reads
    // as a shorter name today, with no warning.
    const auto truncated = productionPk1("A", "MON");
    check(truncated.verdict() == Legality::Verdict::NoProblemsFound,
          "production PK1 cannot see a glyphless byte behind the decoded name (handoff)");
}

} // namespace

int main() {
    tradeMarker();
    glyphlessBytes();
    koreanPk1();
    koreanPk2();
    digits();
    japaneseTrainerName();
    production();
    if (failures != 0) {
        std::fprintf(stderr, "Gen I/II name rules: %d of %d checks FAILED\n", failures, checks);
        return EXIT_FAILURE;
    }
    std::printf("Gen I/II name rules: %d checks PASS\n", checks);
    return EXIT_SUCCESS;
}
