// Round 6 P6 / R2-REROLL: Gen II Unown letter selector, shiny letters, and every surface agreeing.
//
// Owner (Round 5, 1ca2faec): "Unown A, made shiny, looked like I or V". That is the native rule:
// pokecrystal engine/gfx/color.asm CheckShininess pins Defense / Speed / Special at 10 and needs
// Attack bit 1, and engine/gfx/load_pics.asm GetUnownLetter reads bits 1-2 of each DV, so a
// shiny Unown can only be I or V. This test derives that from the two rules (no hand-written
// letter list), proves the selector always yields the requested letter, keeps shiny whenever
// possible and explains when it is not, and that the stored record, sprite key and Review agree.
#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/Gen2UnownLetter.h"
#include "UI/Gen2PokemonSession.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
namespace DV = PokeBank::UIModel::Gen12DVReroll;
namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), std::string("open ") + path);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

Letter::DVs fromIndex(unsigned v) {
    return {static_cast<uint8_t>(v >> 12), static_cast<uint8_t>((v >> 8) & 15u),
            static_cast<uint8_t>((v >> 4) & 15u), static_cast<uint8_t>(v & 15u)};
}

// Independent transcriptions of the two pokecrystal routines, on the two stored DV bytes.
bool crystalShiny(const Letter::DVs& d) {
    const uint8_t first = static_cast<uint8_t>(d[0] << 4 | d[1]);
    const uint8_t second = static_cast<uint8_t>(d[2] << 4 | d[3]);
    return (first & 0x20) != 0 && (first & 0x0F) == 10 && second == 0xAA; // SHINY_ATK_MASK, DEF/SPD/SPC 10
}
int crystalLetter(const Letter::DVs& d) {
    const unsigned v = ((d[0] >> 1 & 3u) << 6) | ((d[1] >> 1 & 3u) << 4) | ((d[2] >> 1 & 3u) << 2) | (d[3] >> 1 & 3u);
    return static_cast<int>(v / (0xFF / 26 + 1));
}

// Full-scan reference for Letter::nearest (same tie rule: first in scan order with the minimum
// distance). Only the minimum distance is compared, since ties may legitimately differ.
int bruteNearestDistance(const Letter::DVs& current, int letter, bool shiny) {
    int best = -1;
    for (unsigned v = 0; v < 0x10000u; ++v) {
        const auto d = fromIndex(v);
        if (crystalShiny(d) != shiny || (letter >= 0 && crystalLetter(d) != letter)) continue;
        const int score = Letter::distance(current, d);
        if (best < 0 || score < best) best = score;
    }
    return best;
}

} // namespace

int main() {
    // 1. Derivation: the rules agree with pokecrystal on every DV set, and the shiny letters are
    //    computed from them. Result: I and V only.
    uint32_t mask = 0;
    for (unsigned v = 0; v < 0x10000u; ++v) {
        const auto d = fromIndex(v);
        require(DV::isShiny(d) == crystalShiny(d), "isShiny == CheckShininess");
        require(DV::unownLetter(d) == crystalLetter(d), "unownLetter == GetUnownLetter");
        if (crystalShiny(d)) mask |= 1u << crystalLetter(d);
    }
    require(Letter::shinyLetterMask() == mask, "shiny letter mask derived from the full DV space");
    require(mask == ((1u << 8) | (1u << 21)), "only I and V can be shiny");
    require(Letter::shinyLettersText() == "I and V", "shiny letter text");
    for (int l = 0; l < 26; ++l) require(Letter::canBeShiny(l) == ((mask >> l & 1u) != 0), "canBeShiny");
    // The shiny shortcut in nearest(): kShinyAttack x {10,10,10} is exactly the shiny set.
    int shinyCount = 0;
    for (unsigned v = 0; v < 0x10000u; ++v) shinyCount += crystalShiny(fromIndex(v));
    require(shinyCount == 8, "exactly eight shiny DV sets");
    for (const uint8_t a : DV::kShinyAttack) require(crystalShiny({a, 10, 10, 10}), "kShinyAttack sets are shiny");

    // 2. nearest() equals a full scan (sampled starting DVs x every letter x both states).
    for (unsigned v = 0; v < 0x10000u; v += 997) {
        const auto d = fromIndex(v);
        for (int l = -1; l < 26; ++l)
            for (bool shiny : {false, true}) {
                const auto c = Letter::nearest(d, l, shiny);
                const int ref = bruteNearestDistance(d, l, shiny);
                require(c.found == (ref >= 0), "nearest found == brute force");
                if (!c.found) continue;
                require(Letter::distance(d, c.dvs) == ref, "nearest distance == brute force");
                require(crystalShiny(c.dvs) == shiny && (l < 0 || crystalLetter(c.dvs) == l), "nearest constraints");
            }
    }

    // 3. Selector, exhaustively over every DV set and every requested letter.
    for (unsigned v = 0; v < 0x10000u; ++v) {
        const auto d = fromIndex(v);
        const bool shiny = crystalShiny(d);
        for (int l = 0; l < 26; ++l) {
            const auto s = Letter::selectLetter(d, l);
            if (crystalLetter(d) == l) {
                require(s.outcome == Letter::Outcome::Unchanged && s.dvs == d, "same letter is a no-op");
            } else if (shiny && !Letter::canBeShiny(l)) {
                require(s.outcome == Letter::Outcome::ShinyImpossible && s.dvs == d,
                        "impossible shiny letter refused, DVs unchanged");
            } else {
                require(s.outcome == Letter::Outcome::Changed, "letter selected");
                require(crystalLetter(s.dvs) == l, "selector yields the requested letter");
                require(crystalShiny(s.dvs) == shiny, "selector keeps the shiny state");
            }
        }
        require(Letter::selectLetter(d, -1).outcome == Letter::Outcome::Invalid &&
                Letter::selectLetter(d, 26).outcome == Letter::Outcome::Invalid, "out-of-range letter refused");

        // 4. Shiny toggle: off keeps the letter; on keeps it when it can be shiny, otherwise says so.
        const auto t = Letter::toggleShiny(d);
        require(crystalShiny(t.dvs) == !shiny, "toggle flips shiny");
        if (shiny) require(crystalLetter(t.dvs) == crystalLetter(d) && !t.letterChanged, "shiny off keeps letter");
        else if (Letter::canBeShiny(crystalLetter(d)))
            require(crystalLetter(t.dvs) == crystalLetter(d) && !t.letterChanged, "shiny on keeps I / V");
        else
            require(t.letterChanged && Letter::canBeShiny(crystalLetter(t.dvs)), "shiny on reports the letter change");
    }
    // The owner's case: Unown A made shiny becomes I or V, and the toggle reports it.
    const Letter::DVs unownA{0, 0, 0, 0};
    require(crystalLetter(unownA) == 0, "0/0/0/0 is Unown A");
    const auto madeShiny = Letter::toggleShiny(unownA);
    require(madeShiny.letterChanged && (crystalLetter(madeShiny.dvs) == 8 || crystalLetter(madeShiny.dvs) == 21),
            "shiny Unown A becomes I or V, reported");
    require(Letter::parseLetter("k") == 10 && Letter::parseLetter("Z") == 25 && Letter::parseLetter("") < 0 &&
            Letter::parseLetter("AB") < 0 && Letter::parseLetter("1") < 0, "keyboard answer parsing");

    // 5. Stored record, sprite key and Review: stage a selected letter, reparse, compare.
    const auto raw = fixture(GS, true);
    auto parsed = parse(raw, SourceGame::Silver);
    require(static_cast<bool>(parsed), "fixture parses");
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    const auto entry = *editor->boxedPokemon(1, 0, error);
    struct Case { Letter::DVs start; int letter; };
    for (const Case c : {Case{{15, 8, 8, 3}, 0}, Case{{15, 8, 8, 3}, 10}, Case{{15, 8, 8, 3}, 25},
                         Case{{0, 0, 0, 0}, 19}, Case{{14, 10, 10, 10}, 8}}) {
        const int l = c.letter;
        Rules::Session s;
        s.begin(entry, Rules::SessionMode::Edit);
        require(s.setSpecies(201, "silver_gbc"), "species -> Unown");
        const auto pick = Letter::selectLetter(c.start, l);
        require(pick.outcome == Letter::Outcome::Changed, "pick");
        require(crystalShiny(pick.dvs) == crystalShiny(c.start), "staged pick keeps shiny state");
        s.working.dvs = {StagedEditor::derivedHPDV(pick.dvs), pick.dvs[0], pick.dvs[1], pick.dvs[2], pick.dvs[3]};
        s.working.form = static_cast<uint8_t>(DV::unownLetter(pick.dvs)); // what applyEditableDVs stores
        require(s.keep(*editor, 1, 0, error), "keep: " + error);
        const auto back = editor->boxedPokemon(1, 0, error);
        require(back && back->species == 201 && back->form == l,
                "stored record letter (Party/Box sprite key pokemon->form()) == selected letter");
        bool reviewed = false;
        for (const auto& change : editor->pendingChanges())
            if (change.key == "box:1:0:unown_letter")
                reviewed = change.afterValue == std::string("Unown ") + static_cast<char>('A' + l);
        require(reviewed, "Review names the staged Unown letter");
    }

    // 6. The device-only painters and input (composite Switch TU) are pinned by source.
    const auto foundation = read("src/UI/Gen2PokemonEditorFoundation.inc");
    require(foundation.find("p.form = p.species == 201 ? static_cast<uint8_t>(PokeBank::UIModel::Gen12DVReroll::unownLetter(dvs)) : 0;") != std::string::npos,
            "every DV change recomputes the draft letter (sprite key)");
    require(foundation.find("if (p.species == PokeBank::UIModel::Gen2UnownLetter::kUnownSpecies) {\n        toggleUnownShiny(screen, p);") != std::string::npos,
            "Unown shiny toggle keeps / explains the letter");
    require(foundation.find("Letter::selectLetter(before, letter)") != std::string::npos &&
            foundation.find("Turn Shiny off first. Nothing changed") != std::string::npos,
            "letter selector applies the model and explains an impossible shiny letter");
    const auto workspace = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    require(workspace.find("if (p.species == PokeBank::UIModel::Gen2UnownLetter::kUnownSpecies) {\n            chooseUnownLetter(screen, p);") != std::string::npos,
            "Values row 6 opens the letter selector for Unown");
    require(workspace.find("SharedPokemonShell::drawPortrait(fb, x, y, p.species, p.shiny, p.form);") != std::string::npos,
            "portrait sprite keyed by the draft letter");
    require(workspace.find("unownLetter({p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]})") != std::string::npos,
            "Letter row text computed from the DVs");
    const auto finalFix = read("src/UI/Gen2HardwareFinalFix.inc");
    require(finalFix.find("gen2UnownLetterText(p)") != std::string::npos &&
            finalFix.find("gen2UnownLetterNote()") != std::string::npos,
            "final hardware Values layer shows Letter and the in-UI note");
    const auto surface = read("src/UI/Gen2SharedPokemonSurface.inc");
    require(surface.find("SpriteManager::getSprite(p.species, spriteForm, p.shiny)") != std::string::npos,
            "shared surface sprite keyed by the DV letter");
    for (const char* panel : {"src/UI/Panels/PartyPokemonPanel.cpp", "src/UI/Panels/BoxPokemonPanel.cpp"}) {
        const auto src = read(panel);
        require(src.find("pokemon->speciesID(), pokemon->form(), isShiny)") != std::string::npos,
                std::string(panel) + " sprite keyed by the stored form (reparsed from DVs)");
    }
    const auto header = read("include/UI/Gen2UnownLetter.h");
    require(header.find("uint8_t form") == std::string::npos, "the selector never stores an independent form");

    std::cout << "Gen II Unown letter selector (shiny letters derived = I and V; all 65536 DV sets x 26 letters; "
                 "shiny kept or explained; stored form, sprite key and Review agree): PASS\n";
    return 0;
}
