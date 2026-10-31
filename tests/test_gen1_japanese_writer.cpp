// Japanese Gen I (Red / Green / Blue / Yellow JP) save WRITING (owner Oct 28).
//
// The staged Gen I Pokemon editor and the Trainer name editor write the Japanese layout with the same
// bar as the international writers: strict reparse, checksums, byte preservation, Discard, export.
// Offsets here are computed independently from PKHeX 8e3eb3d4 (SAV1Offsets.cs JPN, SAV1.cs
// SIZE_BOX_LIST / BoxCount 8 / BoxSlotCount 30 / StringLength 6, PokeCrypto.cs SIZE_1JLIST = 59),
// not taken from the writer's own layout header, so a wrong writer offset fails here.
//
//   1. Synthetic Japanese saves for Red, Blue and Yellow (banks initialized, different current boxes):
//      kana nickname, level and move edits, Add into box 8 slot 30, Clone, Release, packed move,
//      Party edit, kana trainer name, play time and money through the export path; strict parse of
//      the result; the byte diff is confined to the edited records, their list / name fields and the
//      named checksums; source bytes and originalBytes() never change; Discard restores exactly; a
//      no-change finalize is byte-identical; Review says "Imported" for a .pk1 and "Add Pokémon" for
//      a Create.
//   2. Refusals: Latin on a Japanese save, kana on an international save, 6 kana, region-mismatched
//      .pk1 both ways, out-of-range box / slot for the 8 x 30 layout.
//   3. Opt-in real saves: POKEBANK_JP_FIXTURES=<folder with saves/JP_{Red,Blue,Yellow}_G1_32KiB.sav and
//      pokemon/JP_Hitmonchan_G1_Legal.pk1>. Without it: NOT TESTED / NO FIXTURE (never a failure).
//      Fixture files are never copied into the repository.
#include "Integration/Gen1/Gen1ExportTransaction.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen1/Gen1StagedTrainerEditor.h"
#include "Integration/Gen1/Gen1StorageLayout.h"
#include "PokemonFile/NativePokemonFile.h"
#include "Utils/Gen12KanaText.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include <unistd.h>

using namespace PokeVault::Integration::Gen1;
namespace F = PokeBank::PokemonFile;
using Bytes = std::vector<uint8_t>;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << "\n"; }
}
bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

// ---- Japanese layout, from PKHeX 8e3eb3d4 (independent of Gen1StorageLayout.h) ----
namespace J {
constexpr size_t Name = 0x2598, Money = 0x25EE, Tid = 0x25FB, Bag = 0x25C4, PcItems = 0x27DC;
constexpr size_t BoxIndex = 0x2842, Starter = 0x29B9, PikaFriendship = 0x2712, PlayTime = 0x2CA0;
constexpr size_t Party = 0x2ED5, CurrentBox = 0x302D, Checksum = 0x3594, MainLength = 0x0FFC;
constexpr size_t Str = 6, Slots = 30, Boxes = 8;
constexpr size_t BoxList = (Str * 2 + 33 + 1) * Slots + 2;   // SAV1.SIZE_BOX_LIST = 0x566
constexpr size_t BankBoxes = 4 * BoxList;                     // 0x1598, bank checksum follows
static_assert(BoxList == 0x566 && BankBoxes == 0x1598);
size_t bank(size_t b) { return (b < 4 ? 0x4000 : 0x6000) + (b % 4) * BoxList; }
size_t body(size_t list, size_t s) { return list + Slots + 2 + s * 33; }
size_t ot(size_t list, size_t s) { return list + Slots + 2 + Slots * 33 + s * Str; }
size_t nick(size_t list, size_t s) { return list + Slots + 2 + Slots * 33 + Slots * Str + s * Str; }
size_t partyBody(size_t s) { return Party + 8 + s * 44; }
size_t partyOT(size_t s) { return Party + 8 + 6 * 44 + s * Str; }
size_t partyNick(size_t s) { return Party + 8 + 6 * 44 + 6 * Str + s * Str; }
// The checksums a writer must refresh: main + (banks initialized) 2 x (all-boxes + 4 per-box).
std::set<size_t> checksumBytes() {
    std::set<size_t> out{Checksum};
    for (size_t b : {size_t(0x4000), size_t(0x6000)})
        for (size_t i = 0; i <= 4; ++i) out.insert(b + BankBoxes + i);
    return out;
}
}  // namespace J

uint8_t diff8(const Bytes& v, size_t start, size_t n) {
    uint8_t s = 0xFF;
    for (size_t i = start; i < start + n; ++i) s = uint8_t(s - v[i]);
    return s;
}
void be16(Bytes& v, size_t o, uint16_t x) { v[o] = uint8_t(x >> 8); v[o + 1] = uint8_t(x); }
Bytes kana(const std::string& text) {
    Bytes out;
    if (Utils::Gen12Kana::encode(text, 5, out) != Utils::Gen12Kana::Result::Ok) { check(false, "test kana " + text); out.assign(6, 0x50); }
    return out;
}
void putName(Bytes& v, size_t at, const std::string& text) { const auto k = kana(text); std::copy(k.begin(), k.end(), v.begin() + at); }

void jpChecksums(Bytes& v) {
    v[J::Checksum] = diff8(v, 0x2598, J::MainLength);
    if (v[J::BoxIndex] & 0x80)
        for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
            v[b + J::BankBoxes] = diff8(v, b, J::BankBoxes);
            for (size_t i = 0; i < 4; ++i) v[b + J::BankBoxes + 1 + i] = diff8(v, b + i * J::BoxList, J::BoxList);
        }
}

// A stored Pikachu (internal 0x54) at level 5 (EXP 125, medium-fast), Thunder Shock / Growl; the DV
// byte varies per slot so every record is distinct.
void storedPikachu(Bytes& v, size_t at, uint8_t dvTag) {
    std::fill_n(v.begin() + at, 33, 0);
    v[at] = 0x54; be16(v, at + 1, 20); v[at + 3] = 5; v[at + 5] = v[at + 6] = 23; v[at + 7] = 190;
    v[at + 8] = 84; v[at + 9] = 45; be16(v, at + 12, 0x1234); v[at + 16] = 125;
    v[at + 27] = uint8_t(0x80 | (dvTag & 0x0F)); v[at + 28] = 0x88; v[at + 29] = 30; v[at + 30] = 40;
}
void writeBox(Bytes& v, size_t list, size_t count, uint8_t tag) {
    std::fill_n(v.begin() + list, J::BoxList, 0);
    v[list] = uint8_t(count);
    for (size_t s = 0; s < count; ++s) v[list + 1 + s] = 0x54;
    v[list + 1 + count] = 0xFF;
    std::fill_n(v.begin() + J::ot(list, 0), J::Slots * J::Str * 2, 0x50);
    const char* digits[] = {"０", "１", "２", "３", "４", "５", "６", "７", "８", "９"};
    for (size_t s = 0; s < count; ++s) {
        storedPikachu(v, J::body(list, s), uint8_t(tag + s));
        putName(v, J::ot(list, s), "サトシ");
        putName(v, J::nick(list, s), std::string("ピカ") + digits[s / 10] + digits[s % 10]);
    }
}

// Synthetic Japanese R/G/B/Y save: player サトシ, money 3175, banks initialized when `initialized`,
// one Party Pikachu, box 1 with 4, box 8 with 29 Pokemon (when the banks are initialized).
Bytes japaneseSave(SourceGame game, uint8_t currentBox, bool initialized) {
    Bytes v(kRawSaveSize, 0);
    std::fill_n(v.begin() + J::Name, J::Str, 0x50); putName(v, J::Name, "サトシ");
    v[J::Money] = 0x00; v[J::Money + 1] = 0x31; v[J::Money + 2] = 0x75;
    be16(v, J::Tid, 0x1234);
    v[J::Bag] = 0; v[J::Bag + 1] = 0xFF; v[J::PcItems] = 0; v[J::PcItems + 1] = 0xFF;
    v[J::Starter] = game == SourceGame::Yellow ? 0x54 : 0x99;
    v[J::PikaFriendship] = game == SourceGame::Yellow ? 90 : 0;
    v[J::PlayTime] = 12; v[J::PlayTime + 2] = 34; v[J::PlayTime + 3] = 56; v[J::PlayTime + 4] = 17;
    v[J::BoxIndex] = uint8_t(currentBox | (initialized ? 0x80 : 0));
    // Party: one Pikachu L5 (party body = stored 33 + level + five stats).
    v[J::Party] = 1; v[J::Party + 1] = 0x54; v[J::Party + 2] = 0xFF;
    std::fill_n(v.begin() + J::partyOT(0), 12 * J::Str, 0x50);
    storedPikachu(v, J::partyBody(0), 8);
    v[J::partyBody(0) + 33] = 5;
    const auto stats = StagedPokemonEditor::calculateBattleStats(25, 5, {8, 8, 8, 8}, {}).asArray();
    for (size_t i = 0; i < 5; ++i) be16(v, J::partyBody(0) + 34 + i * 2, stats[i]);
    putName(v, J::partyOT(0), "サトシ"); putName(v, J::partyNick(0), "ピカチュウ");
    if (initialized) {
        for (size_t b = 0; b < J::Boxes; ++b) writeBox(v, J::bank(b), b == 0 ? 4 : b == 7 ? 29 : 0, uint8_t(b * 3));
        std::copy_n(v.begin() + J::bank(currentBox), J::BoxList, v.begin() + J::CurrentBox);
    } else {
        // PKSM-Core / the game: uninitialized banks are not read; only the current box exists.
        writeBox(v, J::CurrentBox, 4, 0);
    }
    jpChecksums(v);
    return v;
}

// International R/B/Y with one boxed Pikachu (as tests/test_gen1_trainer_edit.cpp).
Bytes internationalSave(SourceGame game) {
    Bytes v(kRawSaveSize, 0);
    std::fill_n(v.begin() + 0x2598, 11, 0x50);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83;                              // RED
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 2;
    v[0x29C3] = game == SourceGame::Yellow ? 0x54 : 0x99;
    const size_t box = 0x30C0, at = box + 22;
    v[box] = 1; v[box + 1] = 0x54; v[box + 2] = 0xFF;
    storedPikachu(v, at, 8);
    std::fill_n(v.begin() + box + 682, 11, 0x50); v[box + 682] = 0x91; v[box + 683] = 0x84; v[box + 684] = 0x83;
    std::fill_n(v.begin() + box + 902, 11, 0x50);
    v[box + 902] = 0x8F; v[box + 903] = 0x88; v[box + 904] = 0x8A; v[box + 905] = 0x80;  // PIKA
    v[0x3523] = diff8(v, 0x2598, 0x0F8B);
    return v;
}

Bytes staged(const StagedPokemonEditor& e) { return {e.stagedBytes().begin(), e.stagedBytes().end()}; }
Bytes original(const StagedPokemonEditor& e) { return {e.originalBytes().begin(), e.originalBytes().end()}; }

std::optional<F::Decoded> decodeFile(const Bytes& b, const char* name) {
    std::string e;
    auto d = F::decode(b, name, e);
    if (!d) std::cerr << "decode refused: " << e << "\n";
    return d;
}

// Every differing byte must be inside `allowed`.
bool confined(const Bytes& a, const Bytes& b, const std::set<size_t>& allowed, const std::string& what) {
    bool ok = a.size() == b.size();
    for (size_t i = 0; ok && i < a.size(); ++i)
        if (a[i] != b[i] && !allowed.count(i)) {
            std::cerr << what << ": unexpected change at 0x" << std::hex << i << std::dec << "\n";
            ok = false;
        }
    return ok;
}
void addRange(std::set<size_t>& s, size_t start, size_t n) { for (size_t i = 0; i < n; ++i) s.insert(start + i); }
// One box slot's bytes: species marker, body, OT, nickname.
void addSlot(std::set<size_t>& s, size_t list, size_t slot) {
    s.insert(list + 1 + slot); addRange(s, J::body(list, slot), 33);
    addRange(s, J::ot(list, slot), J::Str); addRange(s, J::nick(list, slot), J::Str);
}

const PokemonChange* row(const StagedPokemonEditor& e, const std::string& key) {
    for (const auto& c : e.pendingChanges()) if (c.key == key) return &c;
    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// 1. Synthetic Japanese saves.
void synthetic(SourceGame game, uint8_t currentBox) {
    const std::string tag = std::string(sourceGameId(game)) + " (current box " + std::to_string(currentBox + 1) + ")";
    const Bytes raw = japaneseSave(game, currentBox, true);
    const Bytes source = raw;
    auto parsed = parse(raw, game);
    check(parsed && parsed.save->metadata().region == RegionLayout::Japanese, tag + ": synthetic save parses as Japanese");
    if (!parsed) { std::cerr << parsed.detail << "\n"; return; }
    std::string error;
    auto e = StagedPokemonEditor::create(*parsed.save, error);
    check(e && error.empty(), tag + ": Japanese editor opens (" + error + ")");
    if (!e) return;
    check(e->layout().boxCount == 8 && e->layout().capacity == 30 && e->layout().stringLength == 6, tag + ": 8 x 30 layout");
    check(e->view().boxes().size() == 8 && e->boxedPokemon(7, 28, error) && !e->boxedPokemon(7, 29, error),
          tag + ": box 8 holds 29");

    // No-change finalize is byte-identical.
    check(e->finalizedBytes(error) == source, tag + ": no-change finalize is byte-identical");

    // Nickname (kana), level and a move on box 8 slot 1.
    BoxPokemonEdit edit;
    edit.nickname = "ライチュ"; edit.level = 10; edit.moves = std::array<uint8_t, 4>{84, 33, 0, 0};
    check(e->stageEdit(7, 0, edit, error), tag + ": stage nickname/level/move (" + error + ")");
    auto p = e->boxedPokemon(7, 0, error);
    check(p && p->nickname == "ライチュ" && p->level == 10 && p->moves[1] == 33 && p->pp[1] == 35 && p->experience == 1000,
          tag + ": edited record reads back");
    // Party nickname (kana).
    BoxPokemonEdit partyEdit; partyEdit.nickname = "ピカ";
    check(e->stagePartyPokemonEdit(0, partyEdit, error), tag + ": stage Party nickname (" + error + ")");
    // Add into box 8 slot 30: the default nickname is the katakana species name.
    check(e->appendSlot(7, error) == std::optional<size_t>(29), tag + ": box 8 appends at slot 30");
    BoxPokemonCreate create; create.dvs = {15, 10, 10, 10};
    check(e->stageAdd(7, 29, create, error), tag + ": Add into box 8 slot 30 (" + error + ")");
    p = e->boxedPokemon(7, 29, error);
    check(p && p->species == 25 && p->nickname == "ピカチュウ" && p->originalTrainer == "サトシ" && p->level == 5,
          tag + ": created Pikachu reads ピカチュウ / サトシ");
    check(!e->appendSlot(7, error) && has(error, "full"), tag + ": box 8 is now full (30)");
    // Clone box 1 slot 1 to box 1 slot 5; Release box 1 slot 2 (compaction).
    const auto cloneSource = *e->boxedPokemon(0, 0, error);
    check(e->stageClone(0, 0, 0, 4, error), tag + ": Clone (" + error + ")");
    check(e->stageRemove(0, 1, error), tag + ": Release (" + error + ")");
    p = e->boxedPokemon(0, 3, error);
    check(p && p->nickname == cloneSource.nickname && p->rawBody == cloneSource.rawBody, tag + ": clone compacted to slot 4");
    check(!e->boxedPokemon(0, 4, error), tag + ": box 1 holds 4 after Clone + Release");
    // Review: Create keeps "Add Pokémon".
    const auto* add = row(*e, "7/29/presence");
    check(add && has(add->label, "Box 8 Slot 30") && has(add->label, "Add Pokémon") && has(add->afterValue, "Pikachu"),
          tag + ": Review shows Add Pokémon for the Create");

    // Trainer name (kana), money and play time through the trainer workspace.
    auto t = StagedTrainerEditor::create(*parsed.save, error);
    check(t && t->nameEditable() && t->nameMax() == 5, tag + ": Japanese trainer name is editable, 5 characters");
    check(t->stageTrainerName("レッド", error), tag + ": stage kana trainer name (" + error + ")");
    check(t->stageMoney(123456, error), tag + ": stage money");
    check(t->stagePlayTime({200, 1, 2, false}, error), tag + ": stage play time");
    const Bytes trainerCandidate = t->finalize(error);
    check(!trainerCandidate.empty(), tag + ": trainer finalize");

    const Bytes before = staged(*e);
    const Bytes out = e->finalizedBytes(error, {}, trainerCandidate);
    check(!out.empty(), tag + ": finalize with trainer (" + error + ")");
    if (out.size() != kRawSaveSize) return;
    check(staged(*e) == before, tag + ": finalize does not change the staged image");
    const auto reread = parse(out, game);
    check(reread && reread.save->metadata().region == RegionLayout::Japanese, tag + ": strict reparse of the result");
    if (reread) {
        const auto& s = *reread.save;
        check(s.trainer().name == "レッド" && s.trainer().money == 123456 && s.trainer().playTime &&
              s.trainer().playTime->hours == 200, tag + ": reparse reads the trainer edits");
        check(s.boxes()[7].slots[0] && s.boxes()[7].slots[0]->nickname == "ライチュ" && s.boxes()[7].slots[0]->level == 10,
              tag + ": reparse reads the box edit");
        check(s.boxes()[7].slots[29] && s.boxes()[7].slots[29]->nickname == "ピカチュウ", tag + ": reparse reads the Add");
        check(s.party()[0].nickname == "ピカ", tag + ": reparse reads the Party edit");
        check(out[J::Checksum] == diff8(out, 0x2598, J::MainLength), tag + ": main checksum at 0x3594");
        for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
            check(out[b + J::BankBoxes] == diff8(out, b, J::BankBoxes), tag + ": bank checksum");
            for (size_t i = 0; i < 4; ++i)
                check(out[b + J::BankBoxes + 1 + i] == diff8(out, b + i * J::BoxList, J::BoxList), tag + ": box checksum");
        }
    }
    // Byte diff: box 8 slots 1 and 30 (+ count / terminator), box 1 slots 2..5 (+ count / terminator),
    // the Party nickname, the player name, money, play-time clock, and the named checksums; each
    // box list both where the game reads it now (bank) and, for the current box, at 0x302D.
    std::set<size_t> allowed = J::checksumBytes();
    for (size_t list : {J::bank(7), J::bank(0)}) {
        const bool box8 = list == J::bank(7);
        std::vector<size_t> lists{list};
        if ((box8 && currentBox == 7) || (!box8 && currentBox == 0)) lists.push_back(J::CurrentBox);
        for (size_t l : lists) {
            allowed.insert(l);                                                   // count
            if (box8) { addSlot(allowed, l, 0); addSlot(allowed, l, 29); allowed.insert(l + 1 + 30); }  // + 0xFF cap
            else for (size_t s = 1; s <= 5; ++s) { if (s <= 4) addSlot(allowed, l, s); allowed.insert(l + 1 + s); }
        }
    }
    addRange(allowed, J::partyNick(0), J::Str);
    addRange(allowed, J::Name, J::Str);
    addRange(allowed, J::Money, 3);
    addRange(allowed, J::PlayTime, 4);
    check(confined(source, out, allowed, tag), tag + ": byte diff confined to the edited records, names and checksums");
    // Field-level spot checks against independent offsets.
    check(std::equal(out.begin() + J::Name, out.begin() + J::Name + 6, kana("レッド").begin()), tag + ": name bytes at 0x2598");
    check(out[J::Money] == 0x12 && out[J::Money + 1] == 0x34 && out[J::Money + 2] == 0x56, tag + ": money BCD at 0x25EE");
    check(std::equal(out.begin() + J::nick(J::bank(7), 0), out.begin() + J::nick(J::bank(7), 0) + 6, kana("ライチュ").begin()),
          tag + ": nickname bytes at the box 8 slot 1 nickname field");
    check(out[J::body(J::bank(7), 0) + 3] == 10 && out[J::body(J::bank(7), 0) + 9] == 33, tag + ": level / move bytes");
    if (currentBox == 7 || currentBox == 0) {
        const size_t mirrored = currentBox == 7 ? J::bank(7) : J::bank(0);
        check(std::equal(out.begin() + J::CurrentBox, out.begin() + J::CurrentBox + J::BoxList, out.begin() + mirrored),
              tag + ": current box and its bank copy agree");
    }
    // Source never changes.
    check(raw == source && original(*e) == source && std::equal(source.begin(), source.end(), parsed.save->sourceBytes().begin()),
          tag + ": source bytes and originalBytes() never change");

    // Import a Japanese .pk1 exported from this very save (59 bytes), then Review says "Imported".
    const auto file = e->exportBoxPokemonFile(0, 0, error);
    check(file && file->size() == 59, tag + ": box export is a 59-byte Japanese .pk1");
    const auto pk1 = file ? decodeFile(*file, "jp.pk1") : std::nullopt;
    check(pk1 && pk1->japanese && pk1->generation == 1, tag + ": exported .pk1 decodes as Japanese Gen I");
    if (pk1) {
        check(e->stageImport(1, 0, *pk1, error), tag + ": Japanese .pk1 imports into the Japanese save (" + error + ")");
        const auto* imp = row(*e, "1/0/presence");
        check(imp && has(imp->label, "Box 2 Slot 1: Imported") && imp->beforeValue == "Empty" && has(imp->afterValue, "Pikachu"),
              tag + ": Review shows Imported for the .pk1");
        check(e->exportBoxPokemonFile(1, 0, error) == *file, tag + ": re-export of the import is byte-identical");
        // An edited import is still labelled Imported; a Create still says Add Pokémon.
        BoxPokemonEdit relabel; relabel.nickname = "ピカピ";
        check(e->stageEdit(1, 0, relabel, error), tag + ": edit the import");
        imp = row(*e, "1/0/presence");
        check(imp && has(imp->label, "Imported"), tag + ": an edited import stays Imported");
        check(e->stageAdd(1, 1, {}, error), tag + ": Create after the import");
        const auto* created = row(*e, "1/1/presence");
        check(created && has(created->label, "Add Pokémon"), tag + ": the Create beside it says Add Pokémon");
        const auto out2 = e->finalizedBytes(error);
        const auto reread2 = parse(out2, game);
        check(reread2 && reread2.save->boxes()[1].slots[0] && reread2.save->boxes()[1].slots[0]->nickname == "ピカピ",
              tag + ": import + edit survive finalize");
    }
    // Party export: 59 bytes, stored body / OT / nickname copied as stored.
    const auto partyFile = e->exportPartyPokemonFile(0, error);
    check(partyFile && partyFile->size() == 59 && std::equal(partyFile->begin() + 53, partyFile->end(), kana("ピカ").begin()),
          tag + ": Party export is 59 bytes with the staged nickname");

    // Packed move on the 8 x 30 layout: carry box 1 slot 1 to box 3.
    check(e->beginPackedMove(0, 0, error), tag + ": begin packed move");
    size_t placed = 99;
    check(e->placePackedMove(2, 0, placed, error) && placed == 0, tag + ": place in box 3 (" + error + ")");
    check(e->boxedPokemon(2, 0, error) && parse(e->finalizedBytes(error), game), tag + ": packed move result reparses");

    // Discard restores exactly.
    e->discard();
    t->discard();
    check(staged(*e) == source && !e->hasPendingChanges() && e->pendingChanges().empty(), tag + ": Discard restores exactly");
    check(std::equal(t->stagedBytes().begin(), t->stagedBytes().end(), source.begin()) && t->trainerName() == "サトシ",
          tag + ": trainer Discard restores exactly");
    check(e->finalizedBytes(error) == source, tag + ": after Discard, finalize is byte-identical");
}

// Uninitialized banks: Add into another box initializes them the way the game's first box switch does.
void uninitializedBanks(SourceGame game) {
    const Bytes raw = japaneseSave(game, 2, false);
    auto parsed = parse(raw, game);
    check(parsed && !parsed.save->metadata().boxesInitialized, "uninitialized Japanese save parses");
    if (!parsed) return;
    std::string error;
    auto e = StagedPokemonEditor::create(*parsed.save, error);
    check(e && e->stageAdd(7, 0, {}, error), "Add into box 8 of an uninitialized save (" + error + ")");
    const auto out = e->finalizedBytes(error);
    const auto reread = parse(out, game);
    check(reread && reread.save->metadata().boxesInitialized && reread.save->boxes()[7].slots[0] &&
          reread.save->boxes()[2].slots[3], "banks initialized: box 8 holds the Add, box 3 keeps the current box");
    if (reread) check(std::equal(out.begin() + J::CurrentBox, out.begin() + J::CurrentBox + J::BoxList, out.begin() + J::bank(2)),
                      "current box copied into its bank");
    // Outside the bank area only the box index (bit 7) and the main checksum change.
    std::set<size_t> allowed{J::BoxIndex, J::Checksum};
    addRange(allowed, 0x4000, 0x4000);
    check(confined(raw, out, allowed, "uninitialized"), "uninitialized: diff confined to banks, box index and checksum");
    e->discard();
    check(staged(*e) == raw, "uninitialized: Discard restores");
}

// ------------------------------------------------------------------------------------------------
// 2. Refusals. Each leaves the staged image untouched.
void refusals() {
    const Bytes jpRaw = japaneseSave(SourceGame::Red, 7, true);
    const Bytes intlRaw = internationalSave(SourceGame::Red);
    auto jpParsed = parse(jpRaw, SourceGame::Red);
    auto intlParsed = parse(intlRaw, SourceGame::Red);
    check(jpParsed && intlParsed, "refusal fixtures parse");
    if (!jpParsed || !intlParsed) return;
    std::string error;
    auto jp = StagedPokemonEditor::create(*jpParsed.save, error);
    auto intl = StagedPokemonEditor::create(*intlParsed.save, error);
    auto jpTrainer = StagedTrainerEditor::create(*jpParsed.save, error);
    auto intlTrainer = StagedTrainerEditor::create(*intlParsed.save, error);
    check(jp && intl && jpTrainer && intlTrainer, "refusal editors open");
    if (!jp || !intl || !jpTrainer || !intlTrainer) return;

    const auto unchanged = [&] {
        return staged(*jp) == jpRaw && staged(*intl) == intlRaw && !jp->hasPendingChanges() && !intl->hasPendingChanges() &&
               std::equal(jpTrainer->stagedBytes().begin(), jpTrainer->stagedBytes().end(), jpRaw.begin()) &&
               std::equal(intlTrainer->stagedBytes().begin(), intlTrainer->stagedBytes().end(), intlRaw.begin());
    };
    BoxPokemonEdit latin; latin.nickname = "ASH";
    check(!jp->stageEdit(0, 0, latin, error) && has(error, "Latin"), "Latin nickname refused on a Japanese save: " + error);
    BoxPokemonEdit latinOT; latinOT.otName = "Red";
    check(!jp->stageEdit(0, 0, latinOT, error) && has(error, "Latin"), "Latin OT refused on a Japanese save");
    BoxPokemonCreate latinCreate; latinCreate.nickname = "PIKA";
    check(!jp->stageAdd(7, 29, latinCreate, error) && has(error, "Latin"), "Latin Create name refused on a Japanese save");
    check(!jpTrainer->stageTrainerName("ASH", error) && has(error, "Latin"), "Latin trainer name refused on a Japanese save");
    BoxPokemonEdit kanaNick; kanaNick.nickname = "ピカ";
    check(!intl->stageEdit(2, 0, kanaNick, error) && has(error, "Japanese characters"), "kana refused on an international save: " + error);
    check(!intlTrainer->stageTrainerName("サトシ", error) && has(error, "Japanese characters"), "kana trainer name refused on an international save");
    BoxPokemonEdit six; six.nickname = "ピカチュウア";
    check(!jp->stageEdit(0, 0, six, error) && has(error, "5 characters"), "6 kana refused: " + error);
    check(!jpTrainer->stageTrainerName("サトシサトシ", error), "6-kana trainer name refused");
    BoxPokemonEdit sixIntl; sixIntl.nickname = "ピカチュウアイ";
    check(!intl->stageEdit(2, 0, sixIntl, error) && has(error, "Japanese characters"), "long kana on an international save says why");

    // .pk1 region mismatch, both ways (existing wording kept for Japanese -> international).
    const auto jpFile = jp->exportBoxPokemonFile(0, 0, error);
    const auto intlFile = intl->exportBoxPokemonFile(2, 0, error);
    check(jpFile && jpFile->size() == 59 && intlFile && intlFile->size() == 69, "export sizes 59 / 69");
    if (jpFile && intlFile) {
        const auto jpPk1 = decodeFile(*jpFile, "jp.pk1"), intlPk1 = decodeFile(*intlFile, "intl.pk1");
        check(jpPk1 && jpPk1->japanese && intlPk1 && !intlPk1->japanese, "region of the files");
        if (jpPk1 && intlPk1) {
            check(!intl->stageImport(0, 0, *jpPk1, error) &&
                  error == "This .pk1 comes from a Japanese game; it cannot go into an international save (no region conversion)",
                  "Japanese .pk1 refused by an international save");
            check(!jp->stageImport(1, 0, *intlPk1, error) && has(error, "international game") && has(error, "no region conversion"),
                  "international .pk1 refused by a Japanese save: " + error);
            // Out of range for 8 x 30: box index 8 and slot index 30.
            check(!jp->stageImport(8, 0, *jpPk1, error) && has(error, "out of range"), "import box 9 refused");
            check(!jp->stageImport(1, 30, *jpPk1, error) && has(error, "out of range"), "import slot 31 refused");
        }
    }
    check(!jp->boxedPokemon(8, 0, error) && has(error, "out of range"), "box 9 is out of range on Japanese");
    check(!jp->boxedPokemon(0, 30, error) && has(error, "out of range"), "slot 31 is out of range on Japanese");
    check(!jp->stageAdd(8, 0, {}, error) && !jp->stageAdd(1, 30, {}, error), "Add out of range refused");
    check(!jp->appendSlot(8, error), "appendSlot box 9 refused");
    check(!jp->stageEdit(0, 30, latin, error) && !jp->stageRemove(8, 0, error) && !jp->stageClone(0, 0, 8, 0, error),
          "edit / release / clone out of range refused");
    check(!jp->revertPokemon(8, 0, error) && !jp->revertPokemon(0, 30, error), "revert out of range refused");
    check(!jp->beginPackedMove(8, 0, error) && !jp->beginPackedMove(0, 30, error), "packed move out of range refused");
    check(jp->boxedPokemon(7, 28, error).has_value(), "box 8 slot 29 (last real index for 30 slots) is in range");
    check(unchanged(), "every refusal left the staged images untouched");

    // The kana a Japanese save takes are the shared table: hiragana べ is stored as ベ and reads so.
    check(jpTrainer->stageTrainerName("べに", error) && jpTrainer->trainerName() == "ベに", "hiragana remap stores the shared glyph");
}

// Play time / money / name from the trainer workspace reach an international export too.
void internationalTrainerExport() {
    const Bytes raw = internationalSave(SourceGame::Blue);
    auto parsed = parse(raw, SourceGame::Blue);
    std::string error;
    auto e = parsed ? StagedPokemonEditor::create(*parsed.save, error) : nullptr;
    auto t = parsed ? StagedTrainerEditor::create(*parsed.save, error) : nullptr;
    check(e && t, "international editors open");
    if (!e || !t) return;
    check(t->stageTrainerName("ASH", error) && t->stageMoney(5, error) && t->stagePlayTime({3, 4, 5, false}, error),
          "international trainer edits stage");
    const auto out = e->finalizedBytes(error, {}, t->finalize(error));
    const auto reread = parse(out, SourceGame::Blue);
    check(reread && reread.save->trainer().name == "ASH" && reread.save->trainer().money == 5 &&
          reread.save->trainer().playTime && reread.save->trainer().playTime->hours == 3,
          "international name / money / play time survive the Pokemon editor finalize (" + error + ")");
    std::set<size_t> allowed{0x3523};
    addRange(allowed, 0x2598, 11); addRange(allowed, 0x25F3, 3); addRange(allowed, 0x2CED, 4);
    check(confined(raw, out, allowed, "international trainer"), "international trainer diff confined");
}

// The app's Export Copy path (publishVerifiedGen1Export) with Pokemon + Trainer + Bag workspaces on
// a Japanese save: the written copy reparses with every edit, only the edited fields and checksums
// differ, and the source file is never written.
Bytes readFile(const std::filesystem::path& p);
void exportTransaction() {
    namespace fs = std::filesystem;
    const Bytes raw = japaneseSave(SourceGame::Yellow, 7, true);
    auto parsed = parse(raw, SourceGame::Yellow);
    std::string error;
    auto e = parsed ? StagedPokemonEditor::create(*parsed.save, error) : nullptr;
    auto t = parsed ? StagedTrainerEditor::create(*parsed.save, error) : nullptr;
    auto inv = parsed ? StagedInventoryEditor::create(*parsed.save, error) : nullptr;
    check(e && t && inv, "export: editors open (" + error + ")");
    if (!e || !t || !inv) return;
    BoxPokemonEdit nick; nick.nickname = "ピカ";
    check(e->stageEdit(7, 0, nick, error) && t->stageTrainerName("レッド", error) &&
          inv->stageSetQuantity(PokeVault::Inventory::ClassicPocket::Medicines, 0x14, 3, error), "export: edits stage (" + error + ")");
    const fs::path root = fs::temp_directory_path() / ("pokebank-gen1-jp-writer-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path sourcePath = root / "source.srm";
    { std::ofstream f(sourcePath, std::ios::binary); f.write(reinterpret_cast<const char*>(raw.data()), static_cast<std::streamsize>(raw.size())); }
    Gen1ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = "jp_export";
    request.sourcePath = sourcePath.string();
    request.timestamp = "20261028T000000Z";
    const auto result = publishVerifiedGen1Export(*e, request, inv.get(), t.get());
    check(result.success, "export: verified Gen I export succeeds (" + result.error + ")");
    Bytes edited;
    if (result.success)
        for (const auto& entry : fs::directory_iterator(result.directory))
            if (entry.path().extension() == ".srm" && entry.path().filename().string().find("original") == std::string::npos)
                edited = readFile(entry.path());
    const auto reread = parse(edited, SourceGame::Yellow);
    check(reread && reread.save->trainer().name == "レッド" && reread.save->boxes()[7].slots[0] &&
          reread.save->boxes()[7].slots[0]->nickname == "ピカ", "export: written copy reparses with the edits");
    if (reread) {
        std::set<size_t> allowed = J::checksumBytes();
        addRange(allowed, J::Name, J::Str);
        addRange(allowed, J::Bag, 42);
        addRange(allowed, J::nick(J::bank(7), 0), J::Str);
        addRange(allowed, J::nick(J::CurrentBox, 0), J::Str);
        check(confined(raw, edited, allowed, "export"), "export: diff confined to nickname, name, Bag and checksums");
        check(edited[J::Bag] == 1 && edited[J::Bag + 1] == 0x14 && edited[J::Bag + 2] == 3 && edited[J::Bag + 3] == 0xFF,
              "export: Bag list at 0x25C4");
    }
    check(readFile(sourcePath) == raw, "export: the source save is never written");
    fs::remove_all(root);
}

// Every Gen I species gets a Japanese default nickname that the kana font can store.
void defaultNames() {
    const Bytes raw = japaneseSave(SourceGame::Red, 0, true);
    auto parsed = parse(raw, SourceGame::Red);
    std::string error;
    auto e = parsed ? StagedPokemonEditor::create(*parsed.save, error) : nullptr;
    check(e != nullptr, "default-name editor opens");
    if (!e) return;
    int ok = 0;
    for (uint16_t species = 1; species <= 151; ++species) {
        BoxPokemonCreate c; c.species = species; c.moves = {33, 0, 0, 0}; c.pp = {35, 0, 0, 0};
        if (e->stageAdd(2, 0, c, error) && e->boxedPokemon(2, 0, error) && !e->boxedPokemon(2, 0, error)->nickname.empty()) ++ok;
        else std::cerr << "species " << species << ": " << error << "\n";
        e->discard();
    }
    check(ok == 151, "all 151 species Create on a Japanese save with their katakana name");
}

// ------------------------------------------------------------------------------------------------
// 3. Opt-in real Japanese saves.
Bytes readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void realSaves() {
    const char* dir = std::getenv("POKEBANK_JP_FIXTURES");
    if (!dir) {
        std::cout << "Gen I Japanese writer: real Japanese saves NOT TESTED / NO FIXTURE (set POKEBANK_JP_FIXTURES)\n";
        return;
    }
    const std::filesystem::path root(dir);
    const Bytes hitmonchan = readFile(root / "pokemon" / "JP_Hitmonchan_G1_Legal.pk1");
    const auto hitmonchanFile = decodeFile(hitmonchan, "JP_Hitmonchan_G1_Legal.pk1");
    check(hitmonchan.size() == 59 && hitmonchanFile && hitmonchanFile->japanese, "real Hitmonchan .pk1 is a Japanese Gen I file");
    int saves = 0, exported = 0;
    for (const auto& [file, game] : {std::pair<const char*, SourceGame>{"JP_Red_G1_32KiB.sav", SourceGame::Red},
                                     {"JP_Blue_G1_32KiB.sav", SourceGame::Blue}, {"JP_Yellow_G1_32KiB.sav", SourceGame::Yellow}}) {
        const std::string tag = std::string("real ") + file;
        const Bytes raw = readFile(root / "saves" / file);
        const Bytes source = raw;
        auto parsed = parse(raw, game);
        check(parsed && parsed.save->metadata().region == RegionLayout::Japanese, tag + ": strict parse, Japanese");
        if (!parsed) continue;
        ++saves;
        std::string error;
        auto e = StagedPokemonEditor::create(*parsed.save, error);
        auto t = StagedTrainerEditor::create(*parsed.save, error);
        check(e && t, tag + ": editors open");
        if (!e || !t) continue;
        check(e->finalizedBytes(error) == source, tag + ": no-change finalize is byte-identical");

        // One nickname edit (Party slot 1), one trainer-name edit, one money edit.
        BoxPokemonEdit nick; nick.nickname = "カメ";
        check(e->stagePartyPokemonEdit(0, nick, error), tag + ": Party nickname edit (" + error + ")");
        check(t->stageTrainerName("レッド", error) && t->stageMoney(123456, error), tag + ": trainer name + money edit");
        const auto out = e->finalizedBytes(error, {}, t->finalize(error));
        check(out.size() == kRawSaveSize, tag + ": finalize (" + error + ")");
        if (out.size() != kRawSaveSize) continue;
        const auto reread = parse(out, game);
        check(reread && reread.save->party()[0].nickname == "カメ" && reread.save->trainer().name == "レッド" &&
              reread.save->trainer().money == 123456, tag + ": strict reparse reads the new values (" + error + ")");
        std::set<size_t> allowed{J::Checksum};
        addRange(allowed, J::partyNick(0), J::Str); addRange(allowed, J::Name, J::Str); addRange(allowed, J::Money, 3);
        // When the banks are initialized their checksums are refreshed too (they must stay equal).
        if (raw[J::BoxIndex] & 0x80) { const auto c = J::checksumBytes(); allowed.insert(c.begin(), c.end()); }
        check(confined(source, out, allowed, tag), tag + ": every other byte except the named checksums is unchanged");
        {
            size_t changed = 0;
            for (size_t i = 0; i < out.size(); ++i) changed += out[i] != source[i];
            std::cout << tag << ": edit changed " << changed << " bytes (party nickname 0x" << std::hex << J::partyNick(0)
                      << "+6, name 0x2598+6, money 0x25EE+3, checksum 0x3594)" << std::dec << "\n";
        }
        e->discard(); t->discard();
        check(staged(*e) == source, tag + ": Discard restores");

        // Export every Party and box Pokemon to .pk1 and decode it.
        for (size_t s = 0; s < parsed.save->party().size(); ++s) {
            const auto f = e->exportPartyPokemonFile(s, error);
            const auto d = f ? decodeFile(*f, "p.pk1") : std::nullopt;
            check(f && f->size() == 59 && d && d->japanese, tag + ": Party export decodes");
            exported += bool(d);
        }
        for (size_t b = 0; b < parsed.save->boxes().size(); ++b)
            for (size_t s = 0; s < parsed.save->boxes()[b].slots.size(); ++s) {
                if (!parsed.save->boxes()[b].slots[s]) continue;
                const auto f = e->exportBoxPokemonFile(b, s, error);
                const auto d = f ? decodeFile(*f, "b.pk1") : std::nullopt;
                check(f && f->size() == 59 && d && d->japanese, tag + ": box export decodes");
                exported += bool(d);
            }
        check(staged(*e) == source, tag + ": exports do not change the staged image");

        // Import the PKHeX Hitmonchan into an empty slot, finalize, reparse, re-export.
        if (!hitmonchanFile) continue;
        size_t box = 0;
        while (box < 8 && !e->appendSlot(box, error)) ++box;
        const auto slot = e->appendSlot(box, error);
        check(slot.has_value(), tag + ": an empty slot exists");
        if (!slot) continue;
        check(e->stageImport(box, *slot, *hitmonchanFile, error), tag + ": Hitmonchan imports (" + error + ")");
        const auto* imp = row(*e, std::to_string(box) + "/" + std::to_string(*slot) + "/presence");
        check(imp && has(imp->label, "Imported") && has(imp->afterValue, "Hitmonchan"), tag + ": Review says Imported");
        const auto out2 = e->finalizedBytes(error);
        check(out2.size() == kRawSaveSize, tag + ": finalize the import (" + error + ")");
        if (out2.size() != kRawSaveSize) continue;
        const auto reread2 = parse(out2, game);
        check(reread2 && reread2.save->boxes()[box].slots[*slot] && reread2.save->boxes()[box].slots[*slot]->species == 107,
              tag + ": reparse holds Hitmonchan");
        // Re-export from a fresh editor on the finalized save.
        auto e2 = reread2 ? StagedPokemonEditor::create(*reread2.save, error) : nullptr;
        const auto again = e2 ? e2->exportBoxPokemonFile(box, *slot, error) : std::nullopt;
        // The PKHeX file is a box-origin record (party block all zero, aa4b72e). The box keeps the
        // 33 stored bytes; export fills the party block exactly as pokered _MoveMon BOX_TO_PARTY does.
        const bool boxOrigin = std::all_of(hitmonchan.begin() + 3 + 33, hitmonchan.begin() + 47, [](uint8_t b) { return b == 0; });
        bool same = again && again->size() == hitmonchan.size();
        if (same) {
            for (size_t i = 0; i < hitmonchan.size(); ++i)
                if ((*again)[i] != hitmonchan[i] && !(boxOrigin && i >= 3 + 33 && i < 47)) same = false;
            if (boxOrigin) same = same && (*again)[3 + 0x21] == hitmonchan[3 + 3];
        }
        check(same, tag + ": re-export equals the PKHeX file (stored bytes, OT, nickname" +
                    std::string(boxOrigin ? "; party block computed" : "") + ")");
        if (again && boxOrigin) {
            // The re-exported file (now carrying the computed party block) round-trips exactly.
            const auto d = decodeFile(*again, "again.pk1");
            auto e3 = reread2 ? StagedPokemonEditor::create(*reread2.save, error) : nullptr;
            const size_t next = e3 ? e3->appendSlot(box, error).value_or(99) : 99;
            check(d && e3 && next < 30 && e3->stageImport(box, next, *d, error) && e3->exportBoxPokemonFile(box, next, error) == *again,
                  tag + ": the re-exported file re-imports byte-identically");
        }
        // Import changes stay in the banks (initialized by the import when they were not), the box
        // index byte (bit 7) and the main checksum.
        std::set<size_t> allowedImport{J::BoxIndex, J::Checksum};
        addRange(allowedImport, 0x4000, 0x4000);
        check(confined(source, out2, allowedImport, tag + " import"), tag + ": import diff confined to banks, box index, checksum");
        check(std::equal(source.begin(), source.end(), e->originalBytes().begin()) && raw == source, tag + ": source never changes");
    }
    check(saves == 3, "real-save evidence covered Red, Blue and Yellow");
    std::cout << "Gen I Japanese writer: " << saves << " real Japanese saves, " << exported << " Pokemon exported and decoded\n";
}
}  // namespace

int main() {
    synthetic(SourceGame::Red, 7);
    synthetic(SourceGame::Blue, 0);
    synthetic(SourceGame::Yellow, 3);
    for (auto g : {SourceGame::Red, SourceGame::Blue, SourceGame::Yellow}) uninitializedBanks(g);
    refusals();
    internationalTrainerExport();
    defaultNames();
    exportTransaction();
    realSaves();
    if (failures) { std::cerr << failures << " Japanese Gen I writer check(s) failed\n"; return 1; }
    std::cout << "Gen I Japanese writer (R/B/Y JP synthetic: edit, Add 8x30, Clone, Release, move, trainer, "
                 "import/export, refusals): PASS\n";
    return 0;
}
