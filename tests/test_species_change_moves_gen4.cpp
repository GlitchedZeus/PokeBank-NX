// Generation IV species change, Create moves and history: Diamond, Pearl, Platinum, HeartGold,
// SoulSilver through the real Gen4SharedEditor::Session::changeSpecies (applyGen4Picker's Species
// branch calls it; test_species_change_moves_gen15 pins that) and Gen4StagedPokemonEditor, ending in
// a strict export -> reparse. Round 11 (owner H28 / Extra 5 / Extra 6 / H25 / H07) replaces the
// round 3 R2-BUG-004 refill: a species change keeps only the moves the new species can use in this
// exact game (Gen4MoveCompatibility) with their PP and PP Ups and never adds one; PID-linked nature,
// shininess, IVs and EVs are kept (owner PASS); a Create draft's Pokerus and a Met Location that is
// no encounter of the new species reset (Pikachu Trophy Garden -> Arbok, the owner's case) while an
// existing Pokemon keeps both; Create needs a chosen Met Location and a move. The same species
// changes nothing; only the edited slot record and the storage footer change; Review describes the
// slot; an uncommitted draft changes nothing.
#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Names/MoveInfo.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Gen4StagedPokemonEditor;
using PokeVault::Integration::Gen4::Layout;
namespace Compat = PokeVault::Integration::Gen4MoveCompatibility;
namespace Context = PokeVault::Integration::EncounterContext45;
using PokeBank::UIModel::Gen4SharedEditor::Session;
using PokeBank::UIModel::Gen4SharedEditor::Mode;

namespace {
int flaggedGames = 0;   // Round 11 follow-up: games where Pikachu -> Fearow was flagged

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// Save fixture: the same layout tests/test_gen4_staged_pokemon_editor.cpp builds (Pikachu in box 1
// slot 1, a party Pikachu, both partitions valid, partition 0 newest).
constexpr size_t SAVE_SIZE = 0x80000;
constexpr size_t PARTITION = 0x40000;
constexpr uint32_t MAGIC = 0x20060623;

struct Spec {
    size_t generalSize;
    size_t storageStart;
    size_t storageSize;
    size_t footerSize;
    size_t trainer;
    size_t party;
    size_t boxData;
    size_t boxStride;
};

Spec spec(Layout layout) {
    if (layout == Layout::DiamondPearl)
        return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 0xFF0};
    if (layout == Layout::Platinum)
        return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 0xFF0};
    return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000};
}

void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
    b[o + 2] = static_cast<uint8_t>(v >> 16);
    b[o + 3] = static_cast<uint8_t>(v >> 24);
}
void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}
void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
    b[o + 2] = static_cast<std::byte>(v >> 16);
    b[o + 3] = static_cast<std::byte>(v >> 24);
}
void copy(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i)
        dst[off + i] = static_cast<uint8_t>(src[i]);
}
void copy(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(),
              dst.begin() + static_cast<std::ptrdiff_t>(off));
}

std::vector<std::byte> occupiedPk4(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    wb32(d, 0x00, 0x12345678u);
    wb16(d, 0x08, species);
    wb16(d, 0x0A, 1);
    wb16(d, 0x0C, 12345);
    wb16(d, 0x0E, 54321);
    wb32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    d[0x15] = std::byte{9};
    d[0x17] = std::byte{2};
    auto nickname = Utils::encodeGen4Field(u"PIKA", 11, 10, 2);
    copy(d, 0x48, nickname);
    auto ot = Utils::encodeGen4Field(u"ASH", 8, 7, 2);
    copy(d, 0x68, ot);
    d[0x83] = std::byte{4};
    d[0x84] = std::byte{25};
    d[0x86] = std::byte{17};
    return Encryption::encryptArray4(d);
}

std::vector<std::byte> occupiedPartyPk4(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    const auto stored = occupiedPk4(species);
    const auto decoded = Encryption::decryptArray4(stored);
    std::copy(decoded.begin(), decoded.end(), d.begin());
    d[0x8C] = std::byte{20};
    wb16(d, 0x8E, 35);
    wb16(d, 0x90, 35);
    wb16(d, 0x92, 25);
    wb16(d, 0x94, 20);
    wb16(d, 0x96, 30);
    wb16(d, 0x98, 25);
    wb16(d, 0x9A, 20);
    return Encryption::encryptArray4(d);
}

void stamp(std::vector<uint8_t>& save, size_t off, size_t len,
           size_t footer, uint32_t major) {
    const size_t end = off + len;
    w32(save, end - 0x14, major);
    w32(save, end - 0x10, 1);
    w32(save, end - 0x0C, static_cast<uint32_t>(len));
    w32(save, end - 0x08, MAGIC);
    w16(save, end - 2,
        Utils::crc16ccitt(save.data() + off, len - footer));
}

std::vector<uint8_t> makeSave(Layout layout, uint8_t romCode = 7,
                                  int newerGeneral = 0, int newerStorage = 0) {
    const auto sp = spec(layout);
    std::vector<uint8_t> save(SAVE_SIZE, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto occupied = occupiedPk4();
    const auto party = occupiedPartyPk4();

    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(base),
                  save.begin() + static_cast<std::ptrdiff_t>(base + sp.generalSize), 0);
        const size_t storage = base + sp.storageStart;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(storage),
                  save.begin() + static_cast<std::ptrdiff_t>(storage + sp.storageSize), 0);

        auto trainerName = Utils::encodeGen4Field(u"Kylie", 8, 7, 2);
        copy(save, base + sp.trainer, trainerName);
        w16(save, base + sp.trainer + 0x10, 12150);
        w16(save, base + sp.trainer + 0x12, 22558);
        save[base + sp.trainer + 0x18] = 1;
        save[base + sp.trainer + 0x19] = 2;
        save[base + sp.trainer + 0x1C] = romCode;
        save[base + sp.party - 4] = 1;
        copy(save, base + sp.party, party);

        for (size_t box = 0; box < 18; ++box) {
            const size_t boxBase = storage + sp.boxData + box * sp.boxStride;
            for (size_t slot = 0; slot < 30; ++slot)
                copy(save, boxBase + slot * Encryption::SIZE_STORED4, blank);
            if (layout == Layout::HeartGoldSoulSilver) {
                std::fill(
                    save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0xFF0),
                    save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0x1000),
                    static_cast<uint8_t>(0xC0 + box));
            }
        }
        copy(save, storage + sp.boxData, occupied);

        if (layout == Layout::HeartGoldSoulSilver)
            save[storage + 0x12000] = 0;
        else
            save[storage] = 0;

        stamp(save, base, sp.generalSize, sp.footerSize,
              partition == newerGeneral ? 20 : 10);
        stamp(save, storage, sp.storageSize, sp.footerSize,
              partition == newerStorage ? 30 : 15);
    }
    return save;
}

bool usable(const char* id, const Pokemon::Pokemon4Mutable& mon, uint16_t move) {
    const auto a = Compat::classify(id, mon.species(), mon.form(), move, false);
    return a == Compat::Availability::Direct || a == Compat::Availability::Transfer;
}

// Every kept move came from `before` (moves / pp / ups) with its own PP and PP Ups and is usable;
// every usable old move is kept; empty slots trail.
void checkKept(const char* id, const std::array<uint16_t, 4>& moves, const std::array<uint8_t, 4>& pp,
               const std::array<uint8_t, 4>& ups, const Pokemon::Pokemon4Mutable& mon, const std::string& what) {
    bool empty = false;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto move = mon.moves()[i];
        if (move == 0) {
            empty = true;
            require(mon.ppUps()[i] == 0 && mon.pp()[i] == 0, what + ": empty slot 0/0");
            continue;
        }
        require(!empty, what + ": kept moves move up");
        const auto at = std::find(moves.begin(), moves.end(), move);
        require(at != moves.end(), what + ": no move is ever added");
        const auto from = static_cast<std::size_t>(at - moves.begin());
        require(mon.pp()[i] == pp[from] && mon.ppUps()[i] == ups[from], what + ": PP and PP Ups travel");
        require(usable(id, mon, move), what + ": kept move usable");
    }
    const auto now = mon.moves();
    for (const auto move : moves)
        if (move && usable(id, mon, move))
            require(std::find(now.begin(), now.end(), move) != now.end(), what + ": a usable move is kept");
}

uint16_t metLocation(const Pokemon::Pokemon4Mutable& mon, Enums::GameVersion group) {
    const auto met = PokeBank::UIModel::Gen4SharedEditor::metLocationOf(mon, group);
    require(met.has_value(), "draft reparses");
    return met->location;
}

// The first kWild45 location where `species` itself is a wild encounter in this exact game.
std::optional<uint16_t> wildLocation(std::string_view id, uint16_t species) {
    for (const auto& entry : Context::kWild45)
        if (entry.sourceGameId == id && entry.species == species) return entry.location;
    return std::nullopt;
}

void run(Layout layout, uint8_t romCode, const char* id) {
    auto source = makeSave(layout, romCode);
    const auto original = source;
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, layout, id, &error);
    require(editor && error.empty(), std::string(id) + " editor: " + error);
    const auto group = editor->sourceGroup();
    const auto untouched = editor->stagedBytes();

    auto mon = *editor->editableBoxPokemon(0, 0, &error);
    require(mon.species() == 25, "fixture Pikachu");
    const auto storedMoves = mon.moves();
    require(mon.setMove(0, 84) && mon.setPPUps(0, 2), "give it Thunder Shock with PP Ups");
    require(mon.setMove(1, 45) && mon.setPP(1, 33), "Growl, 33 PP");
    require(mon.setMove(2, 98) && mon.setMove(3, 85) && mon.setPPUps(3, 1), "Quick Attack, Thunderbolt");
    require(mon.setPokerusMode(Pokemon::PokerusMode::Infected), "Pokerus");
    const auto ivs = mon.ivs(), evs = mon.evs();
    const uint8_t nature = mon.nature();
    const bool shiny = mon.shiny();
    const uint16_t met = metLocation(mon, group);
    const uint8_t pokerus = mon.pokerus(), metLevel = mon.metLevel();

    // An existing Pokemon (Edit): several transitions with different learnsets.
    Session s;
    s.mode = Mode::Edit;
    s.baselineMoves = storedMoves;
    s.working = mon;
    for (const uint16_t species : {26, 22, 1, 387, 133, 448, 25}) {
        const auto moves = s.working->moves();
        const auto pp = s.working->pp();
        const auto ups = s.working->ppUps();
        bool reset = true;
        require(s.changeSpecies(species, shiny, id, group, reset), std::string(id) + " species " + std::to_string(species));
        require(s.working->species() == species && !reset, "species applied; Edit never resets the encounter");
        checkKept(id, moves, pp, ups, *s.working, std::string(id) + " -> " + std::to_string(species));
        const auto now = s.working->moves();
        require(species == 25 || species == 26 || std::find(now.begin(), now.end(), 84) == now.end(),
                std::string(id) + " " + std::to_string(species) + ": Thunder Shock never survives a change away from Pikachu / Raichu");
        require(s.working->ivs() == ivs && s.working->evs() == evs && s.working->nature() == nature &&
                s.working->shiny() == shiny, "IVs, EVs, nature and shininess kept");
        require(metLocation(*s.working, group) == met && s.working->metLevel() == metLevel &&
                s.working->pokerus() == pokerus, "Edit keeps Met Location, Met Level and Pokerus");
        // Same species: not one byte changes, even after a hand edit.
        require(s.working->setMove(3, 33), "hand edit");
        const auto before = s.working->encryptedBytes();
        require(s.changeSpecies(species, shiny, id, group, reset), "reselect");
        require(s.working->encryptedBytes() == before, "same species changes nothing");
    }
    require(editor->stagedBytes() == untouched, "an uncommitted draft changes no staged byte");

    // An existing Pokemon left with no usable move cannot be kept moveless.
    {
        Session m;
        m.mode = Mode::Edit;
        auto only = mon;
        require(only.setMove(1, 0) && only.setMove(2, 0) && only.setMove(3, 0), "Thunder Shock only");
        m.baselineMoves = only.moves();   // the record had a move when the session began
        m.working = only;
        bool reset = false;
        require(m.changeSpecies(129, shiny, id, group, reset) && m.working->moves() == (std::array<uint16_t, 4>{}),
                "Magikarp cannot use Thunder Shock");
        require(!m.keep(*editor, 0, 0, error) && error == "Choose at least one move before Keep",
                std::string(id) + " Keep refused with the shared message: " + error);
        require(editor->stagedBytes() == untouched, "nothing staged");
    }

    // Pikachu -> Fearow, Keep, Review, export -> strict reparse, byte scope.
    Session f;
    std::string beginError;
    require(f.begin(Pokemon::Pokemon4ReadOnly(mon.encryptedBytes(), group), group,
                    Mode::Edit, beginError), "Edit session: " + beginError);
    f.baselineMoves = storedMoves;
    bool reset = false;
    require(f.changeSpecies(22, shiny, id, group, reset), "Pikachu -> Fearow");
    // Round 11 follow-up (owner H25 clarification 2): the history is kept, and a misfit is flagged
    // in the editor (Session::historyFlags -> the Caution data row) and in Review; never refused.
    const uint8_t originGame = static_cast<uint8_t>(Pokemon::Pokemon4ReadOnly(mon.encryptedBytes(), group).originVersion());
    const auto expectedFlags = PokeBank::UIModel::HistoryConsistency::gen45Flags(22, originGame, met, metLevel,
                                                                                f.working->level());
    require(f.historyFlags(group) == expectedFlags, std::string(id) + ": editor flags are the shared check's");
    flaggedGames += !expectedFlags.empty();
    require(f.working->moves()[0] == 45 && f.working->pp()[0] == 33, "Fearow: Thunder Shock cleared, Growl kept with its PP");
    const auto fearow = *f.working;
    require(f.keep(*editor, 0, 0, error), std::string(id) + " keep: " + error);
    const auto review = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
    require(!review.empty(), "Review describes the changed slot");
    {
        const auto rows = PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor);
        const auto caution = std::find_if(rows.begin(), rows.end(), [](const auto& row) {
            return row.first == "Box 1 / Slot 1 / Caution";
        });
        if (expectedFlags.empty())
            require(caution == rows.end(), std::string(id) + ": nothing flagged, no caution note");
        else
            require(caution != rows.end() &&
                        caution->second == PokeBank::UIModel::HistoryConsistency::joined(expectedFlags),
                    std::string(id) + ": Review carries the caution note");
    }
    const auto out = editor->finalizedBytes(&error);
    require(!out.empty() && error.empty(), std::string(id) + " export: " + error);
    const auto sp = spec(layout);
    const size_t record = sp.storageStart + sp.boxData;
    const size_t footer = sp.storageStart + sp.storageSize - sp.footerSize;
    for (size_t i = 0; i < out.size(); ++i) {
        if (out[i] == original[i]) continue;
        const bool inRecord = i >= record && i < record + Encryption::SIZE_STORED4;
        const bool inFooter = i >= footer && i < sp.storageStart + sp.storageSize;
        require(inRecord || inFooter, std::string(id) + ": byte " + std::to_string(i) + " outside the edited slot changed");
    }
    auto parsed = Gen4ReadOnlySave::parse(out, layout, id, &error);
    require(parsed && error.empty(), std::string(id) + " strict reparse: " + error);
    const auto& back = parsed->box(0, 0);
    require(back.valid() && back.species() == 22, "reparsed Fearow");
    require(back.moves() == fearow.moves() && back.pp() == fearow.pp() && back.ppUps() == fearow.ppUps(),
            "reparsed moves, PP, PP Ups");
    require(back.ivs() == ivs && back.evs() == evs, "reparsed IVs / EVs");
    {
        const Pokemon::Pokemon4ReadOnlyView view(back);
        const uint16_t backMet = back.originVersion() == 7 || back.originVersion() == 8
            ? back.metLocationExtended() : view.metLocation();
        require(backMet == met && back.metLevel() == metLevel && back.pokerusState() == pokerus &&
                    back.originVersion() == originGame,
                std::string(id) + ": Edit species change kept Met Location / Level, Pokerus and origin through reparse");
    }
    require(source == original, "source bytes never change");
    editor->discard();

    // Create (H25, the owner's case): a new Pikachu starts with no moves; given a Met Location where
    // wild Pikachu live and Pokerus, a change to Arbok resets both; a change to Raichu keeps the
    // location (Pikachu ancestry). Keep needs a chosen Met Location and a move, staging nothing until then.
    const auto pikachuAt = wildLocation(id, 25);
    for (const uint16_t next : {24, 26}) {
        auto draft = editor->createBoxDraft(0, 1, 25, &error);
        require(draft.has_value(), std::string(id) + " Create draft: " + error);
        Session c;
        require(c.beginCreate(std::move(*draft), error), "beginCreate: " + error);
        require(c.working->moves() == (std::array<uint16_t, 4>{}), "Create starts with no moves");
        require(metLocation(*c.working, group) == 0, std::string(id) + " Create starts with no Met Location: " + std::to_string(metLocation(*c.working, group)));
        if (!pikachuAt) break;
        require(c.working->setMetLocation(*pikachuAt) && c.working->setPokerusMode(Pokemon::PokerusMode::Infected),
                "Pikachu met where wild Pikachu live, with Pokerus");
        // Round 11 follow-up (updated): the picker records the explicit choice (the chosen flag, not
        // "location != 0"); the draft's value alone no longer counts as chosen.
        require(!c.metLocationChosen, "a value the picker did not apply is not a choice");
        c.noteMetLocationChosen();
        bool arbokThere = false;
        for (const auto& entry : Context::kWild45)
            arbokThere |= entry.sourceGameId == id && (entry.species == 24 || entry.species == 23) &&
                          entry.location == *pikachuAt;
        bool cleared = false;
        require(c.changeSpecies(next, false, id, group, cleared), "Create species change");
        require(c.working->species() == next && c.working->pokerus() == 0, "Create: Pokerus resets");
        require(c.working->moves() == (std::array<uint16_t, 4>{}), "blank Create stays blank");
        if (next == 24 && !arbokThere)
            require(cleared && metLocation(*c.working, group) == 0 && c.working->metLevel() == c.working->level(),
                    std::string(id) + ": Arbok was never met there -> Met Location back to not chosen");
        if (next == 26)
            require(!cleared && metLocation(*c.working, group) == *pikachuAt,
                    std::string(id) + ": a Pikachu location stays valid for Raichu");
        // The reset is visible: the status names what was reset.
        require(c.speciesChangeResetStatus(cleared) ==
                    (cleared ? std::string("Species changed: Pokérus cleared and Met Location needs a new choice")
                             : std::string("Species changed: Pokérus cleared")),
                std::string(id) + " Create reset status: " + c.speciesChangeResetStatus(cleared));
        require(c.metLocationChosen == !cleared, "a reset location must be chosen again");
        const auto reparsed = Pokemon::Pokemon4ReadOnly(c.working->encryptedBytes(), group);
        require(reparsed.valid() && !reparsed.empty() && reparsed.species() == next, "draft bytes reparse");

        const auto beforeKeep = editor->stagedBytes();
        if (metLocation(*c.working, group) == 0) {
            require(!c.keepCreate(*editor, 0, 1, error) &&
                    error == "Choose a Met Location and at least one move before Keep", std::string(id) + ": " + error);
            require(c.working->setMetLocation(*pikachuAt), "choose a location");
            c.noteMetLocationChosen();
        }
        require(!c.keepCreate(*editor, 0, 1, error) && error == "Choose at least one move before Keep",
                std::string(id) + ": " + error);
        require(editor->stagedBytes() == beforeKeep && c.creating(), "a refused Keep stages nothing; draft stays open");
        require(c.working->setMove(0, 33), "user picks Tackle");
        require(c.keepCreate(*editor, 0, 1, error), std::string(id) + " keepCreate: " + error);
        auto created = editor->editableBoxPokemon(0, 1, &error);
        require(created && created->species() == next && created->moves()[0] == 33, "created record staged");
        editor->discard();
    }
}

} // namespace

int main() {
    // Pikachu's fixture location must make at least one game's Fearow a flagged (suspect) case.
    run(Layout::DiamondPearl, 10, "diamond_nds");
    run(Layout::DiamondPearl, 11, "pearl_nds");
    run(Layout::Platinum, 12, "platinum_nds");
    run(Layout::HeartGoldSoulSilver, 7, "heartgold_nds");
    run(Layout::HeartGoldSoulSilver, 8, "soulsilver_nds");
    require(flaggedGames > 0, "the H25 caution is exercised in at least one Gen IV game");
    std::cout << "Gen IV species change keeps usable moves, Create history reset and required fields (Diamond, "
                 "Pearl, Platinum, HeartGold, SoulSilver; keep, Review, export, reparse, byte scope): PASS\n";
    return 0;
}
