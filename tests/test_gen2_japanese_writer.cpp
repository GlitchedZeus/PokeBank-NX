// Japanese Gold / Silver / Crystal staged writing (owner Oct 28: Japanese Gen II editing).
//
// The layout facts below are written out independently of the editor, from PKHeX 8e3eb3d4
// PKHeX.Core/Saves/Substructures/Gen12/SAV2Offsets.cs (LoadOffsetsJapanese) and Saves/SAV2.cs
// (GetFinalData: one backup copy of 0x2009..AccumulatedChecksumEnd at 0x7209, checksum at
// OverallChecksumPosition 0x2D0D and OverallChecksumPosition2 0x7F0D; boxes 1-6 at 0x4000, 7-9 at
// 0x6000, 30 slots, 6-byte names), the same values Gen2ReadOnlySave.cpp validates.
//
// 1. Synthetic Japanese Gold, Silver, Crystal (64 KiB and 32 KiB) and a 128 KiB padded Gold: edit,
//    Create into box 9 slot 30, Clone, Release, Party edit, trainer name, money, play time, bag;
//    finalize; strict parse; only the named ranges (and their 0x7209 backup and both checksums)
//    change; source / original bytes never change; Discard restores; no-change finalize is
//    byte-identical; 64 KiB Crystal keeps 0x8000.. and padded files keep their padding.
// 2. Refusals: Latin on a Japanese save, kana on an international save, six kana, Japanese .pk2
//    into an international save and the reverse, out-of-range box / slot for the 9 x 30 layout.
// 3. Opt-in real saves: POKEBANK_JP_FIXTURES=<folder with saves/ and pokemon/>, never committed.
//    Without it the real-save part prints NOT TESTED / NO FIXTURE and passes.
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen2/Gen2BattleStats.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Names/NameLanguage.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"
#include "PokemonFile/NativePokemonFile.h"
#include "Utils/Gen12KanaText.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#define REQUIRE(c)                                                                          \
    do {                                                                                    \
        if (!(c)) {                                                                         \
            std::cerr << __FILE__ << ":" << __LINE__ << ": REQUIRE(" #c ") failed\n";       \
            std::abort();                                                                   \
        }                                                                                   \
    } while (0)

namespace jp {

using Bytes = std::vector<uint8_t>;
namespace F = PokeBank::PokemonFile;

// PKHeX 8e3eb3d4 SAV2Offsets.cs LoadOffsetsJapanese.
struct JL {
    SourceGame game;
    VersionFamily family;
    size_t money, idx, names, party, end, tm, items, key, balls, pc, gender;
    size_t size;
};
constexpr size_t kNone = static_cast<size_t>(-1);
constexpr JL JGold  {SourceGame::Gold,   VersionFamily::GoldSilver, 0x23BC, 0x2705, 0x2708, 0x283E, 0x2C8B,
                     0x23C7, 0x2400, 0x242A, 0x2445, 0x245F, kNone, 0x8000};
constexpr JL JSilver{SourceGame::Silver, VersionFamily::GoldSilver, 0x23BC, 0x2705, 0x2708, 0x283E, 0x2C8B,
                     0x23C7, 0x2400, 0x242A, 0x2445, 0x245F, kNone, 0x8000};
constexpr JL JCrystal{SourceGame::Crystal, VersionFamily::Crystal, 0x23BE, 0x26E2, 0x26E5, 0x281A, 0x2AE2,
                      0x23C9, 0x2402, 0x242C, 0x2447, 0x2461, 0x8000, 0x10000};
constexpr size_t kTrainerName = 0x200B, kStr = 6, kCap = 30, kBoxes = 9, kSplit = 6;
constexpr size_t kCurrent = 0x2D10, kSum = 0x2D0D, kSum2 = 0x7F0D, kBackup = 0x7209, kClock = 0x2034;
constexpr size_t kListLen = 1 + (kCap + 1) + kCap * 32 + 2 * kCap * kStr;            // 0x548
constexpr size_t kPartyLen = 1 + 7 + 6 * 48 + 2 * 6 * kStr;                         // 0x170
static_assert(kListLen == 0x548 && kPartyLen == 0x170);

size_t boxStart(size_t b) { return b < kSplit ? 0x4000 + b * (kListLen + 2) : 0x6000 + (b - kSplit) * (kListLen + 2); }
size_t boxBody(size_t b) { return boxStart(b) + 1 + kCap + 1; }
size_t boxOT(size_t b) { return boxBody(b) + kCap * 32; }
size_t boxNick(size_t b) { return boxOT(b) + kCap * kStr; }

Bytes kana(std::string_view s) {
    Bytes out;
    REQUIRE(Utils::Gen12Kana::encode(s, 5, out) == Utils::Gen12Kana::Result::Ok);
    return out;
}
void put(Bytes& d, size_t o, const Bytes& v) { std::copy(v.begin(), v.end(), d.begin() + static_cast<std::ptrdiff_t>(o)); }
void be16(Bytes& d, size_t o, uint16_t v) { d[o] = uint8_t(v >> 8); d[o + 1] = uint8_t(v); }
void be24(Bytes& d, size_t o, uint32_t v) { d[o] = uint8_t(v >> 16); d[o + 1] = uint8_t(v >> 8); d[o + 2] = uint8_t(v); }

// A 32-byte stored body (pret box_struct) with a consistent level / EXP and legal PP.
void body(Bytes& d, size_t b, uint8_t species, uint8_t level, uint8_t atkDef = 0x98, uint8_t speSpc = 0x76) {
    const auto* personal = personalRecord(species);
    REQUIRE(personal);
    d[b] = species; d[b + 1] = 0;
    d[b + 2] = 33; d[b + 3] = 45; d[b + 4] = 0; d[b + 5] = 0;          // Tackle, Growl
    be16(d, b + 6, 0x1234);
    be24(d, b + 8, Pokemon::getExpForLevel(level, personal->experienceGrowth));
    for (int i = 0; i < 5; ++i) be16(d, b + 11 + 2 * i, uint16_t(100 * (i + 1)));
    d[b + 21] = atkDef; d[b + 22] = speSpc;
    d[b + 23] = 35; d[b + 24] = 40; d[b + 25] = 0; d[b + 26] = 0;
    d[b + 27] = 70; d[b + 28] = 0; be16(d, b + 29, 0); d[b + 31] = level;
}
void boxMon(Bytes& d, size_t box, size_t slot, uint8_t species, uint8_t level, std::string_view nick,
            bool egg = false) {
    d[boxStart(box) + 1 + slot] = egg ? 0xFD : species;
    body(d, boxBody(box) + slot * 32, species, level);
    put(d, boxOT(box) + slot * kStr, kana("ヒロ"));
    put(d, boxNick(box) + slot * kStr, kana(nick));
}
void boxCount(Bytes& d, size_t box, size_t count) { d[boxStart(box)] = uint8_t(count); d[boxStart(box) + 1 + count] = 0xFF; }
void pairs(Bytes& d, size_t o, std::initializer_list<std::pair<uint8_t, uint8_t>> e) {
    d[o] = uint8_t(e.size()); size_t p = o + 1;
    for (auto [id, q] : e) { d[p++] = id; d[p++] = q; }
    d[p] = 0xFF;
}
uint16_t sum(const Bytes& d, size_t end) { uint16_t s = 0; for (size_t i = 0x2009; i <= end; ++i) s = uint16_t(s + d[i]); return s; }
uint16_t le16(const Bytes& d, size_t o) { return uint16_t(d[o] | (d[o + 1] << 8)); }
void seal(Bytes& d, const JL& l) {
    const uint16_t s = sum(d, l.end);
    d[kSum] = uint8_t(s); d[kSum + 1] = uint8_t(s >> 8);
    std::copy(d.begin() + 0x2009, d.begin() + static_cast<std::ptrdiff_t>(l.end + 1), d.begin() + kBackup);
    d[kSum2] = uint8_t(s); d[kSum2 + 1] = uint8_t(s >> 8);
}

// Every byte starts as a pattern, so an unexpected write anywhere is visible.
Bytes fixture(const JL& l, size_t size = 0) {
    Bytes d(size ? size : l.size);
    for (size_t i = 0; i < d.size(); ++i) d[i] = uint8_t(i * 37 + 11);
    d[0x2009] = 0x12; d[0x200A] = 0x34;
    put(d, kTrainerName, kana("ヒロ"));
    be24(d, l.money, 3000);
    be16(d, kClock, 12); d[kClock + 2] = 30; d[kClock + 3] = 15;
    d[l.idx] = 0;                                                    // current box: Box 1
    if (l.gender != kNone && l.gender < d.size()) d[l.gender] = 1;
    std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(l.tm), 57, 0);
    pairs(d, l.items, {{0x12, 5}});                                  // Potion x5
    d[l.key] = 0; d[l.key + 1] = 0xFF;
    pairs(d, l.balls, {{0x05, 10}});                                 // Poke Ball x10
    d[l.pc] = 0; d[l.pc + 1] = 0xFF;
    for (size_t b = 0; b < kBoxes; ++b) {
        boxCount(d, b, 0);
        Bytes n(9, 0x50); n[0] = uint8_t(0xF7 + b);                  // "１".."９"
        put(d, l.names + b * 9, n);
    }
    boxCount(d, 0, 2); boxMon(d, 0, 0, 25, 12, "ピカチュウ"); boxMon(d, 0, 1, 155, 9, "ヒノアラシ");
    boxCount(d, 1, 1); boxMon(d, 1, 0, 152, 5, "タマゴ", true);       // an Egg (list species 0xFD)
    boxCount(d, 6, 1); boxMon(d, 6, 0, 158, 7, "ワニノコ");            // bank 3 (box 7)
    boxCount(d, 8, 29);                                              // box 9: slot 30 left free
    for (size_t s = 0; s < 29; ++s) boxMon(d, 8, s, 16, 3, "ポッポ");
    std::copy(d.begin() + static_cast<std::ptrdiff_t>(boxStart(0)),
              d.begin() + static_cast<std::ptrdiff_t>(boxStart(0) + kListLen), d.begin() + kCurrent);
    // Party: one Pichu-free Pikachu with its stored battle stats.
    const size_t p = l.party;
    d[p] = 1; d[p + 1] = 25; d[p + 2] = 0xFF;
    body(d, p + 8, 25, 15);
    const auto stats = calculateBattleStats(25, 15, {9, 8, 7, 6}, {100, 200, 300, 400, 500}).asArray();
    d[p + 8 + 32] = 0; d[p + 8 + 33] = 0; be16(d, p + 8 + 34, stats[0]);
    for (size_t i = 0; i < 6; ++i) be16(d, p + 8 + 36 + 2 * i, stats[i]);
    put(d, p + 8 + 6 * 48, kana("ヒロ"));
    put(d, p + 8 + 6 * 48 + 6 * kStr, kana("ピカ"));
    seal(d, l);
    return d;
}

std::shared_ptr<const ReadOnlySave> strict(const Bytes& d, SourceGame g) {
    auto r = parse(d, g);
    if (!r) std::cerr << "strict parse failed: " << errorName(r.error) << " " << r.detail << "\n";
    REQUIRE(r);
    REQUIRE(r.save->metadata().region == RegionLayout::Japanese);
    return r.save;
}

// The ranges a finalize may change: what the caller names, its 0x7209 backup when inside the
// checksummed block, and the two checksums.
struct Allowed {
    std::vector<bool> ok;
    size_t end;
    Allowed(size_t size, size_t checksumEnd) : ok(size, false), end(checksumEnd) {
        add(kSum, 2); add(kSum2, 2);
    }
    void add(size_t begin, size_t length) {
        for (size_t i = begin; i < begin + length; ++i) {
            ok[i] = true;
            if (i >= 0x2009 && i <= end) ok[kBackup + (i - 0x2009)] = true;
        }
    }
};
void onlyAllowedDiffer(const Bytes& before, const Bytes& after, const Allowed& a) {
    REQUIRE(before.size() == after.size());
    for (size_t i = 0; i < before.size(); ++i)
        if (before[i] != after[i] && !a.ok[i]) {
            std::cerr << "unexpected change at 0x" << std::hex << i << std::dec << "\n";
            REQUIRE(false);
        }
}
void backupAndChecksumsAgree(const Bytes& d, size_t end) {
    const uint16_t s = sum(d, end);
    REQUIRE(le16(d, kSum) == s && le16(d, kSum2) == s);
    REQUIRE(std::equal(d.begin() + 0x2009, d.begin() + static_cast<std::ptrdiff_t>(end + 1), d.begin() + kBackup));
}

F::Decoded decoded(const Bytes& b, const char* name) {
    std::string error;
    auto d = F::decode(b, name, error);
    if (!d) std::cerr << name << ": " << error << "\n";
    REQUIRE(d);
    return *d;
}
bool has(const std::string& text, std::string_view part) { return text.find(part) != std::string::npos; }

void syntheticGame(const JL& l, size_t size, const char* label) {
    const Bytes raw = fixture(l, size);
    const Bytes pristine = raw;
    auto save = strict(raw, l.game);
    REQUIRE(save->metadata().boxCount == 9 && save->metadata().boxCapacity == 30);
    REQUIRE(save->boxes()[8].slots[28] && !save->boxes()[8].slots[29]);
    std::string error;
    auto e = StagedEditor::create(*save, error);
    if (!e) std::cerr << label << ": " << error << "\n";
    REQUIRE(e && error.empty());

    // No-change finalize is byte-identical, also for the padded / 64 KiB envelopes.
    REQUIRE(e->finalizedBytes(error) == raw);

    // --- Pokemon edit: kana nickname, level, one move (Box 1 slot 1, the current box).
    BoxPokemonEdit edit;
    edit.nickname = "ピカ";
    edit.level = 20;
    edit.moves = std::array<uint8_t, 4>{33, 45, 84, 0};                // + ThunderShock
    REQUIRE(e->stageBoxPokemonEdit(0, 0, edit, error));
    // Party: the shared glyph べ is typed as hiragana and stored / read back as katakana ベ.
    BoxPokemonEdit partyEdit;
    partyEdit.nickname = "べべ";
    REQUIRE(e->stagePartyPokemonEdit(0, partyEdit, error));
    REQUIRE(e->partyPokemon(0, error)->nickname == "ベベ");
    // --- Create into the last slot of the 9 x 30 layout: Box 9 slot 30, default katakana name.
    BoxPokemonCreate create;
    create.species = 152;
    create.level = 5;
    create.moves = {33, 45, 0, 0};
    create.pp = {35, 40, 0, 0};
    create.dvs = {10, 10, 10, 10};
    size_t slot = 99;
    REQUIRE(e->stageAddBoxPokemon(8, create, slot, error) && slot == 29);
    REQUIRE(e->boxedPokemon(8, 29, error)->nickname == "チコリータ");
    REQUIRE(e->boxedPokemon(8, 29, error)->originalTrainer == "ヒロ");
    REQUIRE(!e->stageAddBoxPokemon(8, create, slot, error) && has(error, "full"));
    // --- Clone Box 1 slot 2 into bank 3 (Box 7), then Release the original Box 7 Pokemon.
    REQUIRE(e->stageCloneBoxPokemon(0, 1, 6, slot, error) && slot == 1);
    REQUIRE(e->stageReleaseBoxPokemon(6, 0, error));
    REQUIRE(e->boxedPokemon(6, 0, error)->nickname == "ヒノアラシ");
    // --- Trainer name, money, play time, bag.
    REQUIRE(e->stageTrainerName("ゴールド", error));
    REQUIRE(e->stageMoney(123456, error));
    REQUIRE(e->stagePlayTime({101, 2, 3}, error));
    REQUIRE(e->stageInventoryQuantity(InventoryPocket::Items, 0x12, 9, error));
    REQUIRE(e->stageInventoryQuantity(InventoryPocket::Balls, 0x02, 3, error));   // Ultra Ball added

    const Bytes out = e->finalizedBytes(error);
    if (out.empty()) std::cerr << label << ": " << error << "\n";
    REQUIRE(out.size() == raw.size());
    REQUIRE(raw == pristine);                                              // caller bytes untouched
    REQUIRE(std::equal(e->originalBytes().begin(), e->originalBytes().end(), pristine.begin()));

    auto re = strict(out, l.game);
    REQUIRE(re->trainer().name == "ゴールド" && re->trainer().money == 123456);
    REQUIRE(re->trainer().playTime && re->trainer().playTime->hours == 101);
    const auto& m = *re->boxes()[0].slots[0];
    REQUIRE(m.nickname == "ピカ" && m.level == 20 && m.moves[2] == 84 && m.pp[2] == 30);
    REQUIRE(re->party()[0].nickname == "ベベ" && re->party()[0].originalTrainer == "ヒロ");
    REQUIRE(re->boxes()[8].slots[29] && re->boxes()[8].slots[29]->species == 152 &&
            re->boxes()[8].slots[29]->nickname == "チコリータ");
    REQUIRE(re->boxes()[6].slots[0]->nickname == "ヒノアラシ" && !re->boxes()[6].slots[1]);
    REQUIRE(re->boxes()[0].slots[1]->nickname == "ヒノアラシ");             // clone source kept
    const auto inv = decodeInventory(re->payloadBytes(), RegionLayout::Japanese, l.family);
    REQUIRE(inv.available && inv.items.size() == 1 && inv.items[0].quantity == 9 && inv.balls.size() == 2);

    // Exactly the named ranges changed (with their 0x7209 backup and both checksums).
    Allowed a(out.size(), l.end);
    a.add(kTrainerName, kStr);
    a.add(l.money, 3);
    a.add(kClock, 4);
    a.add(l.items, 1 + 2 * 20 + 1);
    a.add(l.balls, 1 + 2 * 12 + 1);
    a.add(boxStart(0), kListLen);
    a.add(kCurrent, kListLen);                                             // Box 1 is the current box
    a.add(boxStart(6), kListLen);
    a.add(boxStart(8), kListLen);
    a.add(l.party, kPartyLen);
    onlyAllowedDiffer(raw, out, a);
    backupAndChecksumsAgree(out, l.end);
    // The current-box copy mirrors Box 1 exactly.
    REQUIRE(std::equal(out.begin() + static_cast<std::ptrdiff_t>(boxStart(0)),
                       out.begin() + static_cast<std::ptrdiff_t>(boxStart(0) + kListLen), out.begin() + kCurrent));
    // Everything from 0x8000 on (64 KiB Crystal bank 4, padding) is untouched.
    REQUIRE(std::equal(raw.begin() + 0x8000, raw.end(), out.begin() + 0x8000));

    // Discard restores exactly; a no-change finalize after it is the original again.
    e->discard();
    REQUIRE(std::equal(e->stagedBytes().begin(), e->stagedBytes().end(), pristine.begin()));
    REQUIRE(e->finalizedBytes(error) == pristine && !e->hasPendingChanges());

    // --- Export / import: Box, Party and Egg files are 63-byte Japanese .pk2 that re-import exactly.
    auto boxFile = e->exportBoxPokemonFile(0, 0, error);
    REQUIRE(boxFile && boxFile->size() == 63);
    const auto boxDecoded = decoded(*boxFile, "pika.pk2");
    REQUIRE(boxDecoded.japanese && boxDecoded.generation == 2);
    auto partyFile = e->exportPartyPokemonFile(0, error);
    REQUIRE(partyFile && partyFile->size() == 63 && decoded(*partyFile, "party.pk2").japanese);
    auto eggFile = e->exportBoxPokemonFile(1, 0, error);
    REQUIRE(eggFile && (*eggFile)[1] == 0xFD && (*eggFile)[3] == 152);
    REQUIRE(e->stageImportBoxPokemon(2, 0, boxDecoded, error));
    REQUIRE(e->stageImportBoxPokemon(2, 1, decoded(*eggFile, "egg.pk2"), error));
    const Bytes imported = e->finalizedBytes(error);
    REQUIRE(!imported.empty());
    auto ri = strict(imported, l.game);
    REQUIRE(ri->boxes()[2].slots[1]->isEgg);
    auto e2 = StagedEditor::create(*ri, error);
    REQUIRE(e2);
    REQUIRE(*e2->exportBoxPokemonFile(2, 0, error) == *boxFile);
    REQUIRE(*e2->exportBoxPokemonFile(2, 1, error) == *eggFile);
    Allowed ai(imported.size(), l.end);
    ai.add(boxStart(2), kListLen);
    onlyAllowedDiffer(raw, imported, ai);
    backupAndChecksumsAgree(imported, l.end);

    // --- Trainer gender: Japanese Crystal stores it at 0x8000, outside the checksum, only in a
    // 64 KiB image.
    e->discard();
    if (l.family == VersionFamily::Crystal && raw.size() >= 0x10000 && save->metadata().payloadSize == 0x10000) {
        REQUIRE(e->trainerGenderEditable() && e->trainerGender() == 1);
        REQUIRE(e->stageTrainerGender(0, error));
        const Bytes g = e->finalizedBytes(error);
        REQUIRE(!g.empty() && strict(g, l.game)->trainer().gender == 0);
        for (size_t i = 0; i < raw.size(); ++i) REQUIRE((g[i] != raw[i]) == (i == 0x8000));
        e->discard();
    } else {
        REQUIRE(!e->trainerGenderEditable() && !e->stageTrainerGender(0, error));
    }
    std::cout << "Japanese " << label << " staged writer: PASS\n";
}

void refusals() {
    const Bytes raw = fixture(JGold);
    auto save = strict(raw, SourceGame::Gold);
    std::string error;
    auto e = StagedEditor::create(*save, error);
    REQUIRE(e);
    const auto unchanged = [&] {
        return std::equal(e->stagedBytes().begin(), e->stagedBytes().end(), raw.begin()) && !e->hasPendingChanges();
    };
    // Latin text on a Japanese save.
    REQUIRE(!e->stageTrainerName("ASH", error) && has(error, "Japanese Generation II save cannot store"));
    BoxPokemonEdit latin; latin.nickname = "PIKA";
    REQUIRE(!e->stageBoxPokemonEdit(0, 0, latin, error) && has(error, "cannot store"));
    BoxPokemonEdit latinOT; latinOT.otName = "Red";
    REQUIRE(!e->stageBoxPokemonEdit(0, 0, latinOT, error) && has(error, "cannot store"));
    // Six kana.
    REQUIRE(!e->stageTrainerName("ピカチュウだ", error) && has(error, "5 characters"));
    BoxPokemonEdit six; six.nickname = "ピカチュウだ";
    REQUIRE(!e->stageBoxPokemonEdit(0, 0, six, error) && has(error, "5 characters"));
    BoxPokemonCreate create; create.species = 25; create.nickname = "ピカチュウだ";
    size_t slot = 0;
    REQUIRE(!e->stageAddBoxPokemon(3, create, slot, error) && has(error, "5 characters"));
    REQUIRE(unchanged());
    // Out-of-range box / slot for 9 x 30.
    BoxPokemonEdit ok; ok.level = 30;
    REQUIRE(!e->boxedPokemon(9, 0, error) && has(error, "box index"));
    REQUIRE(!e->boxedPokemon(0, 30, error) && has(error, "box slot"));
    REQUIRE(!e->stageBoxPokemonEdit(9, 0, ok, error));
    REQUIRE(!e->stageBoxPokemonEdit(0, 30, ok, error));
    create.nickname = "ピカ";
    REQUIRE(!e->stageAddBoxPokemon(9, create, slot, error));
    REQUIRE(!e->stageCloneBoxPokemon(0, 0, 9, slot, error));
    REQUIRE(!e->stageCloneBoxPokemon(0, 30, 3, slot, error));
    REQUIRE(!e->stageReleaseBoxPokemon(9, 0, error) && !e->stageReleaseBoxPokemon(0, 30, error));
    REQUIRE(!e->beginPackedMove(9, 0, error) && !e->beginPackedMove(0, 30, error));
    REQUIRE(!e->exportBoxPokemonFile(9, 0, error) && !e->exportBoxPokemonFile(0, 30, error));
    REQUIRE(unchanged());
    // The 30th slot of a box (index 29) is real: a packed move can place there.
    REQUIRE(e->beginPackedMove(0, 1, error));
    size_t placed = 0;
    REQUIRE(!e->placePackedMove(9, 0, placed, error) && !e->placePackedMove(3, 30, placed, error));
    REQUIRE(e->placePackedMove(8, 29, placed, error) && placed == 29);
    REQUIRE(e->boxedPokemon(8, 29, error)->nickname == "ヒノアラシ");
    const Bytes moved = e->finalizedBytes(error);
    REQUIRE(!moved.empty() && strict(moved, SourceGame::Gold)->boxes()[0].slots[1] == std::nullopt);
    e->discard();

    // .pk2 regions: a Japanese file never goes into an international save and the reverse.
    auto jpFile = e->exportBoxPokemonFile(0, 0, error);
    REQUIRE(jpFile);
    const Bytes intlRaw = ::fixture(GS);
    auto intlSave = parse(intlRaw, SourceGame::Gold);
    REQUIRE(intlSave);
    auto intl = StagedEditor::create(*intlSave.save, error);
    REQUIRE(intl);
    REQUIRE(!intl->stageImportBoxPokemon(2, 0, decoded(*jpFile, "jp.pk2"), error) &&
            error == "This .pk2 comes from a Japanese game; it cannot go into an international save (no region conversion)");
    auto intlFile = intl->exportBoxPokemonFile(0, 0, error);
    REQUIRE(intlFile && intlFile->size() == 73);
    REQUIRE(!e->stageImportBoxPokemon(2, 0, decoded(*intlFile, "intl.pk2"), error) &&
            error == "This .pk2 comes from an international game; it cannot go into a Japanese save (no region conversion)");
    REQUIRE(!e->stageImportBoxPokemon(9, 0, decoded(*jpFile, "jp.pk2"), error));
    REQUIRE(!e->stageImportBoxPokemon(2, 30, decoded(*jpFile, "jp.pk2"), error));
    REQUIRE(unchanged());
    // Kana on an international save: refused, no region conversion.
    REQUIRE(!intl->stageTrainerName("ゴールド", error) && has(error, "Japanese characters"));
    BoxPokemonEdit kanaNick; kanaNick.nickname = "ピカ";
    REQUIRE(!intl->stageBoxPokemonEdit(0, 0, kanaNick, error) && has(error, "Japanese characters"));
    REQUIRE(std::equal(intl->stagedBytes().begin(), intl->stagedBytes().end(), intlRaw.begin()));
    // The international writer is unchanged: Latin still works there.
    REQUIRE(intl->stageTrainerName("GOLD", error) && !intl->finalizedBytes(error).empty());

    // Every Gen II species' Japanese default name is storable, so Create never needs a nickname.
    for (uint16_t s = 1; s <= 251; ++s) {
        Bytes out;
        const auto r = Utils::Gen12Kana::encode(
            Names::getSpeciesNameLocalized(s, static_cast<std::size_t>(Names::NameLanguage::Japanese)), 5, out);
        if (r != Utils::Gen12Kana::Result::Ok) std::cerr << "species " << s << " default name not storable\n";
        REQUIRE(r == Utils::Gen12Kana::Result::Ok);
    }
    std::cout << "Japanese Gen II refusals (Latin / kana / length / .pk2 region / 9 x 30 range): PASS\n";
}

// ---------------------------------------------------------------- opt-in real Japanese saves
Bytes readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), {});
}

struct Real { const char* file; SourceGame game; size_t end, party, money; };

void realSave(const std::filesystem::path& root, const Real& r, const Bytes& ursaring) {
    const Bytes raw = readFile(root / "saves" / r.file);
    REQUIRE(!raw.empty());
    const Bytes pristine = raw;
    auto save = strict(raw, r.game);
    std::string error;
    auto e = StagedEditor::create(*save, error);
    if (!e) std::cerr << r.file << ": " << error << "\n";
    REQUIRE(e);
    REQUIRE(e->finalizedBytes(error) == raw);                              // no-change: byte-identical

    // Export every Party and box Pokemon; each must decode as a Japanese .pk2.
    size_t exported = 0;
    for (size_t s = 0; s < save->party().size(); ++s) {
        auto f = e->exportPartyPokemonFile(s, error);
        REQUIRE(f && f->size() == 63 && decoded(*f, "p.pk2").japanese);
        ++exported;
    }
    for (size_t b = 0; b < save->boxes().size(); ++b)
        for (size_t s = 0; s < save->boxes()[b].slots.size(); ++s) {
            if (!save->boxes()[b].slots[s]) continue;
            auto f = e->exportBoxPokemonFile(b, s, error);
            if (!f) std::cerr << r.file << " box " << b << " slot " << s << ": " << error << "\n";
            REQUIRE(f && f->size() == 63 && decoded(*f, "b.pk2").japanese);
            ++exported;
        }

    // Nickname (first Party Pokemon, else the first box Pokemon) + trainer name + money.
    Allowed a(raw.size(), r.end);
    BoxPokemonEdit nick; nick.nickname = "テスト";
    std::optional<std::pair<size_t, size_t>> boxSlot;
    if (!save->party().empty() && !save->party()[0].isEgg) {
        REQUIRE(e->stagePartyPokemonEdit(0, nick, error));
        a.add(r.party, kPartyLen);
    } else {
        for (size_t b = 0; b < 9 && !boxSlot; ++b)
            if (save->boxes()[b].slots[0] && !save->boxes()[b].slots[0]->isEgg) boxSlot = {b, 0};
        REQUIRE(boxSlot);
        REQUIRE(e->stageBoxPokemonEdit(boxSlot->first, 0, nick, error));
        a.add(boxStart(boxSlot->first), kListLen);
        a.add(kCurrent, kListLen);
    }
    REQUIRE(e->stageTrainerName("テスト", error));
    REQUIRE(e->stageMoney(424242, error));
    a.add(kTrainerName, kStr);
    a.add(r.money, 3);
    const Bytes out = e->finalizedBytes(error);
    if (out.empty()) std::cerr << r.file << ": " << error << "\n";
    REQUIRE(!out.empty() && raw == pristine);
    auto re = strict(out, r.game);
    REQUIRE(re->trainer().name == "テスト" && re->trainer().money == 424242);
    REQUIRE(boxSlot ? re->boxes()[boxSlot->first].slots[0]->nickname == "テスト" : re->party()[0].nickname == "テスト");
    onlyAllowedDiffer(raw, out, a);
    backupAndChecksumsAgree(out, r.end);
    if (raw.size() > 0x8000) REQUIRE(std::equal(raw.begin() + 0x8000, raw.end(), out.begin() + 0x8000));
    size_t changed = 0;
    std::string spans;
    for (size_t i = 0; i < raw.size();) {
        if (raw[i] == out[i]) { ++i; continue; }
        size_t j = i;
        while (j < raw.size() && raw[j] != out[j]) ++j;
        changed += j - i;
        char buf[32];
        std::snprintf(buf, sizeof buf, " 0x%04zX-0x%04zX", i, j - 1);
        spans += buf;
        i = j;
    }
    std::cout << "real " << r.file << " nickname + trainer name + money changed:" << spans << "\n";

    // Import the PKHeX Ursaring into the first empty slot; re-export equals the file.
    e->discard();
    std::optional<std::pair<size_t, size_t>> empty;
    for (size_t b = 0; b < 9 && !empty; ++b) {
        size_t count = 0;
        while (count < 30 && save->boxes()[b].slots[count]) ++count;
        if (count < 30) empty = {b, count};
    }
    REQUIRE(empty);
    const auto file = decoded(ursaring, "JP_Ursaring_G2_Legal.pk2");
    REQUIRE(e->stageImportBoxPokemon(empty->first, empty->second, file, error));
    const Bytes imported = e->finalizedBytes(error);
    REQUIRE(!imported.empty());
    auto ri = strict(imported, r.game);
    REQUIRE(ri->boxes()[empty->first].slots[empty->second]->species == 217);
    auto e2 = StagedEditor::create(*ri, error);
    REQUIRE(e2);
    const auto again = e2->exportBoxPokemonFile(empty->first, empty->second, error);
    REQUIRE(again && *again == ursaring);
    Allowed ai(raw.size(), r.end);
    ai.add(boxStart(empty->first), kListLen);
    if (empty->first == save->metadata().currentBox) ai.add(kCurrent, kListLen);
    onlyAllowedDiffer(raw, imported, ai);
    backupAndChecksumsAgree(imported, r.end);
    std::cout << "real " << r.file << ": no-change identical, " << exported << " Pokemon exported, edit changed "
              << changed << " bytes in the named ranges, Ursaring import re-exports identically: PASS\n";
}

void realSaves() {
    const char* env = std::getenv("POKEBANK_JP_FIXTURES");
    if (!env || !*env) {
        std::cout << "Real Japanese Gen II saves: NOT TESTED / NO FIXTURE (set POKEBANK_JP_FIXTURES)\n";
        return;
    }
    const std::filesystem::path root(env);
    const Bytes ursaring = readFile(root / "pokemon" / "JP_Ursaring_G2_Legal.pk2");
    REQUIRE(ursaring.size() == 63);
    for (const Real& r : {Real{"JP_Gold_G2_32KiB.sav", SourceGame::Gold, 0x2C8B, 0x283E, 0x23BC},
                          Real{"JP_Silver_G2_32KiB.sav", SourceGame::Silver, 0x2C8B, 0x283E, 0x23BC},
                          Real{"JP_Crystal_G2_64KiB.sav", SourceGame::Crystal, 0x2AE2, 0x281A, 0x23BE},
                          Real{"JP_Gold_G2_128KiB_SYNTHETIC_FF_PADDED.sav", SourceGame::Gold, 0x2C8B, 0x283E, 0x23BC}})
        realSave(root, r, ursaring);
}

}  // namespace jp

int main() {
    jp::syntheticGame(jp::JGold, 0, "Gold 32 KiB");
    jp::syntheticGame(jp::JSilver, 0, "Silver 32 KiB");
    jp::syntheticGame(jp::JCrystal, 0, "Crystal 64 KiB");
    jp::syntheticGame(jp::JCrystal, 0x8000, "Crystal 32 KiB");
    {
        // 128 KiB: a 32 KiB Gold followed by 0xFF padding (upstream PKSE 9a9a731 envelope).
        auto padded = jp::fixture(jp::JGold);
        padded.resize(kPaddedSaveSize128K, 0xFF);
        auto save = jp::strict(padded, SourceGame::Gold);
        REQUIRE(save->metadata().paddingSize == kPaddedSaveSize128K - 0x8000);
        std::string error;
        auto e = StagedEditor::create(*save, error);
        REQUIRE(e && e->finalizedBytes(error) == padded);
        REQUIRE(e->stageTrainerName("ゴールド", error) && e->stageMoney(7, error));
        const auto out = e->finalizedBytes(error);
        REQUIRE(out.size() == padded.size() && std::all_of(out.begin() + 0x8000, out.end(), [](uint8_t b) { return b == 0xFF; }));
        REQUIRE(jp::strict(out, SourceGame::Gold)->trainer().name == "ゴールド");
        std::cout << "Japanese Gold 128 KiB padded: padding kept byte for byte: PASS\n";
    }
    jp::refusals();
    jp::realSaves();
    std::cout << "Japanese Gen II staged writer: PASS\n";
    return 0;
}
