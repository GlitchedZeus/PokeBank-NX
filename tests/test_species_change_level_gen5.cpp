// Round 13 owner decision 9: an EXISTING Pokemon keeps its current level on a species change and
// its EXP is reconciled to the new species' growth rate (Pokemon/SpeciesChangeExperience.h, used by
// StagedPokemon5Record's Field::Species transaction). Generation V (Black / White / Black 2 /
// White 2): before Round 13 an existing Pokemon could not change to a species of another EXP growth
// group at all ("its level would change"); now it can, keeping its level. Through the shared Gen V
// screen state (speciesRefusal, the Edit draft, Keep), box and Party, all six growth rates, Lv 1 /
// mid / 100: the Party level byte follows EXP, every Review row replays independently (an EXP-only
// change is an Experience row, never an empty Level row), export -> strict reparse keeps level and
// EXP, and Met Location / Met Level / Pokerus / origin are kept byte for byte (owner H25).
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "UI/SharedPokemonEditorContract.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
using Action = Shared::Action;
using Slot = G::Gen5StagedPokemonWorkspace::Slot;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Field = G::StagedPokemon5Record::Field;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// tests/test_gen5_party_box_parity.cpp fixture: party Pikachu, Box 1 slot 1 Eevee, slot 2 Snivy.
void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}

// Synthetic SAV5: party Pikachu (Lv 50), Box 1 slot 1 Eevee with native
// met/egg dates, Box 1 slot 2 Snivy. Checksums stamped on the primary copy.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    sav[tr + 4] = 'N'; sav[tr + 6] = 'X';
    C::write16(sav, tr + 0x14, 12345);
    C::write16(sav, tr + 0x16, 54321);
    sav[tr + 0x1F] = version;
    std::vector<uint8_t> pk(C::PartySize, 0);
    write32(pk, 0, 0x123456FF);   // low byte 0xFF: male for every ratio in this test
    C::write16(pk, 8, 25);
    C::write16(pk, 12, 12345);
    C::write16(pk, 14, 54321);
    pk[0x8C] = 50;
    const auto party = C::encryptCandidate(pk);
    sav[L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    std::vector<uint8_t> eevee(C::StoredSize, 0);
    write32(eevee, 0, 0xABCDEFFF);
    C::write16(eevee, 8, 133);
    eevee[0x17] = 2;                        // language
    eevee[0x5F] = version;                  // origin game
    eevee[0x78] = 10; eevee[0x79] = 3; eevee[0x7A] = 6;   // egg date 2010-03-06
    eevee[0x7B] = 11; eevee[0x7C] = 12; eevee[0x7D] = 25; // met date 2011-12-25
    C::write16(eevee, 0x7E, 60003);         // egg location (raw)
    C::write16(eevee, 0x80, 16);            // met location (raw)
    const auto box0 = C::encryptCandidate(eevee);
    std::copy(box0.begin(), box0.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset));
    std::vector<uint8_t> snivy(C::StoredSize, 0);
    write32(snivy, 0, 0x01020304);
    C::write16(snivy, 8, 495);
    const auto box1 = C::encryptCandidate(snivy);
    std::copy(box1.begin(), box1.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + C::StoredSize));
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    const size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

uint8_t growth(uint16_t species, bool b2w2) { return G::personal5(species, 0, b2w2).info->growthRate; }

// The owner rule, written out independently of the shared helper.
uint32_t expected(uint8_t level, uint32_t exp, uint8_t g) {
    const uint32_t low = G::experienceForLevel5(g, level);
    const uint32_t high = level == 100 ? low : G::experienceForLevel5(g, level + 1u) - 1;
    return exp >= low && exp <= high ? exp : low;
}

std::optional<G::Pokemon5ReadOnly> view(const G::Gen5StagedPokemonWorkspace& ws, bool party) {
    return party ? ws.viewParty(0) : ws.viewBox(0, 0);
}

void scenario(G::SaveFamily family, std::string_view game, uint8_t version, bool party, uint16_t from, uint16_t to,
              uint8_t level, bool mid) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const auto sav = fixture(family, version);
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    require(static_cast<bool>(parsed), std::string(game) + " parses: " + error);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    G::Gen5SharedScreenState ui;
    const Slot slot = party ? Slot{Region::Party, 0, 0} : Slot{Region::Box, 0, 0};
    const uint8_t gFrom = growth(from, b2w2), gTo = growth(to, b2w2);
    uint32_t exp = G::experienceForLevel5(gFrom, level);
    if (mid && level < 100) exp += (G::experienceForLevel5(gFrom, level + 1u) - exp) / 2;
    const std::string what = std::string(game) + (party ? " Party " : " Box ") + std::to_string(from) + " Lv" +
                             std::to_string(level) + (mid ? "+" : "") + " -> " + std::to_string(to);

    // Seed the existing Pokemon: a name (Gen V never renames automatically), `from`, the exact EXP.
    {
        const bool ok_ = ui.openActions(ws, slot, error) && ui.activate(ws, Action::Edit, error);
        require(ok_, what + ": open " + error);
    }
    {
        const bool ok_ = ui.editDraft().stageName(Field::NicknameText, "MON", error);
        require(ok_, what + ": name " + error);
    }
    if (ui.draft().current()->species() != from) {
        require(ui.speciesRefusal(from).empty(), what + ": seed refusal " + ui.speciesRefusal(from));
        {
            const bool ok_ = ui.editDraft().stage(Field::Species, 0, from, error);
            require(ok_, what + ": seed species " + error);
        }
    }
    {
        const bool ok_ = ui.editDraft().stage(Field::Experience, 0, exp, error);
        require(ok_, what + ": seed EXP " + error);
    }
    {
        const bool ok_ = ui.keep(ws, error);
        require(ok_, what + ": seed keep " + error);
    }
    const auto seeded = *view(ws, party);
    require(seeded.species() == from && seeded.experience() == exp && G::EditorFields::level(seeded, b2w2) == level,
            what + ": seeded");

    // The species change, exactly as applySpecies stages it on its probe.
    {
        const bool ok_ = ui.openActions(ws, slot, error) && ui.activate(ws, Action::Edit, error);
        require(ok_, what + ": reopen " + error);
    }
    require(ui.speciesRefusal(to).empty(), what + ": a different growth group is no longer refused: " +
                                               ui.speciesRefusal(to));
    auto probe = ui.editDraft();
    {
        const bool ok_ = probe.stage(Field::Species, 0, to, error);
        require(ok_, what + ": species " + error);
    }
    const auto now = *probe.current();
    const uint32_t want = expected(level, exp, gTo);
    require(now.species() == to && G::EditorFields::level(now, b2w2) == level,
            what + ": level kept (got " + std::to_string(G::EditorFields::level(now, b2w2)) + ")");
    require(now.experience() == want, what + ": EXP (got " + std::to_string(now.experience()) + ", want " +
                                          std::to_string(want) + ")");
    if (party) require(now.partyLevel() == level, what + ": Party level byte");
    require(now.metLocation() == seeded.metLocation() && now.metLevel() == seeded.metLevel() &&
                now.pokerusMode() == seeded.pokerusMode() && now.originVersion() == seeded.originVersion() &&
                now.pid() == seeded.pid(),
            what + ": history and PID kept");
    ui.editDraft() = std::move(probe);
    {
        const bool ok_ = ui.keep(ws, error);
        require(ok_, what + ": keep " + error);
    }
    const auto staged = *view(ws, party);
    require(staged.species() == to && staged.experience() == want && G::EditorFields::level(staged, b2w2) == level,
            what + ": staged");
    for (const auto& change : ws.pendingReview()) {
        const auto verified = G::verifyStagedReview(change, &error);
        require(verified.has_value(), what + ": Review replays: " + error);
        for (const auto& row : verified->fields)
            if (row.field == Field::Level) require(row.after == level && row.before != row.after, what + ": Level row");
    }
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    require(out.has_value(), what + ": export " + error);
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    require(static_cast<bool>(reopened), what + ": strict reparse " + error);
    const auto back = party ? reopened->partyPokemon(0) : reopened->boxPokemon(0, 0);
    require(back && back->species() == to && back->experience() == want && G::EditorFields::level(*back, b2w2) == level &&
                back->metLocation() == seeded.metLocation() && back->metLevel() == seeded.metLevel() &&
                back->pokerusMode() == seeded.pokerusMode(),
            what + ": reparsed level / EXP / history");
}

void run(G::SaveFamily family, std::string_view game, uint8_t version) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    // Rattata Medium Fast, Volbeat Erratic, Makuhita Fluctuating, Pidgey Medium Slow, Snubbull Fast,
    // Lapras Slow: each has two ability slots and allows a male, so the fixtures' PIDs fit.
    const uint16_t species[] = {19, 313, 296, 16, 209, 131};
    require(growth(19, b2w2) == 0 && growth(313, b2w2) == 1 && growth(296, b2w2) == 2 && growth(16, b2w2) == 3 &&
                growth(209, b2w2) == 4 && growth(131, b2w2) == 5,
            "fixture growth rates");
    for (const bool party : {false, true})
        for (const auto from : species)
            for (const auto to : species) {
                if (from == to) continue;
                for (const uint8_t level : {uint8_t(1), uint8_t(37), uint8_t(100)})
                    for (const bool mid : {false, true}) scenario(family, game, version, party, from, to, level, mid);
            }

    // The fixture's Party Pikachu shows Lv 50 (Party level byte) with 0 EXP, a record whose level
    // byte disagrees with EXP. Pikachu -> Rattata (same growth rate) changes neither: the record is
    // left as found. Pikachu -> Pidgey (Medium Slow) keeps Lv 50 and sets its Lv 50 minimum
    // (117,360); Review lists ONE Experience row 0 -> 117,360 and no Level row. A draft whose EXP is
    // then edited back to 0 (Lv 1) still replays (Keep and Review compare with the replayed Species
    // step, not only the original EXP).
    for (const bool editBack : {false, true}) {
        const auto sav = fixture(family, version);
        std::string error;
        const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
        G::Gen5StagedPokemonWorkspace ws(*parsed);
        G::Gen5SharedScreenState ui;
        require(ui.openActions(ws, Slot{Region::Party, 0, 0}, error) && ui.activate(ws, Action::Edit, error), "open");
        require(ui.draft().current()->partyLevel() == 50 && ui.draft().current()->experience() == 0, "Lv 50 byte, 0 EXP");
        require(ui.editDraft().stageName(Field::NicknameText, "PIKA", error), "name");
        {
            auto same = ui.editDraft();
            require(same.stage(Field::Species, 0, 19, error), "Pikachu -> Rattata");
            require(same.current()->partyLevel() == 50 && same.current()->experience() == 0,
                    "same growth rate: level byte and EXP left as found");
        }
        require(ui.editDraft().stage(Field::Species, 0, 16, error), "Pikachu -> Pidgey");
        require(ui.draft().current()->partyLevel() == 50 && ui.draft().current()->experience() == 117360,
                "kept Lv 50 at 117,360 EXP");
        if (editBack) require(ui.editDraft().stage(Field::Experience, 0, 0, error), "EXP back to 0");
        {
            const bool kept = ui.keep(ws, error);
            require(kept, std::string(game) + " keep: " + error);
        }
        bool expRow = false, levelRow = false;
        for (const auto& change : ws.pendingReview()) {
            const auto verified = G::verifyStagedReview(change, &error);
            require(verified.has_value(), std::string(game) + " Review replays: " + error);
            for (const auto& row : verified->fields) {
                expRow = expRow || (row.field == Field::Experience && row.after == 117360u);
                levelRow = levelRow || (row.field == Field::Level && row.before == 50 && row.after == 1);
            }
        }
        // Kept level: one Experience row, no Level row. Edited back to 0 EXP: the level really
        // changed (Lv 50 -> 1), which is a Level row.
        require(editBack ? (levelRow && !expRow) : (expRow && !levelRow),
                std::string(game) + ": Review shows the kept level's EXP row, or the real level change");
        require(ws.viewParty(0)->partyLevel() == (editBack ? 1 : 50), "staged Party level byte");
    }
}

} // namespace

int main() {
    run(G::SaveFamily::BlackWhite, "black_nds", 21);
    run(G::SaveFamily::BlackWhite, "white_nds", 20);
    run(G::SaveFamily::Black2White2, "black2_nds", 23);
    run(G::SaveFamily::Black2White2, "white2_nds", 22);
    std::cout << "test_species_change_level_gen5: OK\n";
    return 0;
}
