// R2-REROLL (round 3 hardware screenshots 3 and 4): Gen II Unown letter from DVs and the editor sprite.
//
// pret pokecrystal engine/gfx/load_pics.asm GetUnownLetter packs bits 1-2 of each stored DV as
// atk.def.spd.spc (atk highest) and divides by $ff / NUM_UNOWN + 1 = 10, giving A..Z = 0..25 (the
// game then adds 1). engine/gfx/color.asm CheckShininess: Attack DV bit 1 set (SHINY_ATK_MASK),
// Defense / Speed / Special DV = 10.
//
// Owner's Silver screenshots: shiny Unown 14/10/10/10 and non-shiny Unown 15/8/8/3 showed the same
// sprite. Audit result: both DV sets are native (the shiny one satisfies CheckShininess exactly), but
// they are DIFFERENT letters, V and T. Proven defects, fixed:
//   1. the Gen II editor sprite (Gen2SharedPokemonSurface.inc drawSharedDetails) ignored the form,
//      so every Unown drew the same letter;
//   2. the draft's `form` was never recomputed when DVs or species changed (applyEditableDVs,
//      Session::setSpecies, Gen2PokemonPicker::applySpeciesAppearance), only on reparse.
// Shiny and DV rules themselves were already correct and are unchanged.
#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/Gen12DVReroll.h"
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/Gen2PokemonSession.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
namespace Picker = PokeBank::UIModel::Gen2PokemonPicker;
namespace DV = PokeBank::UIModel::Gen12DVReroll;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// pokecrystal GetUnownLetter, transcribed bit operation by bit operation on the two stored DV bytes
// (atk:def in the first byte, spd:spc in the second), as an independent reference.
int pokecrystalLetter(uint8_t atk, uint8_t def, uint8_t spd, uint8_t spc) {
    const uint8_t first = static_cast<uint8_t>(atk << 4 | def);
    const uint8_t second = static_cast<uint8_t>(spd << 4 | spc);
    uint8_t b = static_cast<uint8_t>((first & 0x60) << 1);               // atk: and %01100000, sla
    uint8_t a = first & 0x06;                                             // def: and %00000110
    a = static_cast<uint8_t>(((a << 4) | (a >> 4)) >> 1);                 //      swap, srl
    b |= a;
    a = second & 0x60;                                                    // spd: and %01100000
    a = static_cast<uint8_t>((((a << 4) | (a >> 4)) & 0xFF) << 1);        //      swap, sla
    b |= a;
    a = static_cast<uint8_t>((second & 0x06) >> 1);                       // spc: and, srl
    b |= a;
    return b / (0xFF / 26 + 1);
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), std::string("open ") + path);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
    // The rule agrees with pokecrystal for every one of the 65536 DV combinations.
    for (unsigned v = 0; v < 0x10000u; ++v) {
        const DV::DVs dvs{static_cast<uint8_t>(v >> 12), static_cast<uint8_t>((v >> 8) & 15),
                          static_cast<uint8_t>((v >> 4) & 15), static_cast<uint8_t>(v & 15)};
        require(DV::unownLetter(dvs) == pokecrystalLetter(dvs[0], dvs[1], dvs[2], dvs[3]),
                "Gen12DVReroll::unownLetter == pokecrystal GetUnownLetter");
    }
    // The owner's two Silver Unown: both native, different letters.
    require(DV::isShiny({14, 10, 10, 10}) && !DV::isShiny({15, 8, 8, 3}), "screenshot shiny states are native");
    require(DV::unownLetter({14, 10, 10, 10}) == 21, "shiny 14/10/10/10 is Unown V");
    require(DV::unownLetter({15, 8, 8, 3}) == 19, "non-shiny 15/8/8/3 is Unown T");
    // A shiny Unown can only be I or V (the other three DVs are pinned at 10).
    for (const uint8_t atk : DV::kShinyAttack) {
        const int letter = DV::unownLetter({atk, 10, 10, 10});
        require(letter == 8 || letter == 21, "shiny Unown is I or V");
    }

    // The draft's form follows its DVs through the real session paths.
    const auto raw = fixture(GS, true);
    auto parsed = parse(raw, SourceGame::Silver);
    require(static_cast<bool>(parsed), "fixture parses");
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    const auto entry = *editor->boxedPokemon(1, 0, error);
    Rules::Session s;
    s.begin(entry, Rules::SessionMode::Edit);
    // A DV edit on the draft (as the DV rows stage it): 15/8/8/3, the owner's non-shiny Unown.
    s.working.dvs = {9, 15, 8, 8, 3};
    require(Picker::applySpeciesAppearance(s, 201, false, "silver_gbc"), "Pikachu -> Unown, not shiny");
    require(s.working.species == 201 && !s.working.shiny, "Unown, not shiny");
    require(s.working.form == DV::unownLetter(Rules::storedDVs(s.working)), "draft form follows DVs (species change)");
    require(s.working.form == 19, "15/8/8/3 draft is Unown T");
    Rules::Session shiny;
    shiny.begin(entry, Rules::SessionMode::Edit);
    require(Picker::applySpeciesAppearance(shiny, 201, true, "silver_gbc"), "Pikachu -> shiny Unown");
    require(shiny.working.shiny && DV::isShiny(Rules::storedDVs(shiny.working)), "shiny DVs");
    require(shiny.working.form == DV::unownLetter(Rules::storedDVs(shiny.working)),
            "draft form follows the shiny DVs (appearance change)");
    require(shiny.working.form == 8 || shiny.working.form == 21, "shiny draft is I or V");
    // Chooser must render the exact form a confirmed species selection would
    // produce, without modifying the Pokémon currently being edited.
    for (bool wantsShiny : {false, true}) {
        auto hypothetical = s;
        require(Picker::applySpeciesAppearance(hypothetical, 201, wantsShiny, "silver_gbc"),
                "hypothetical species appearance");
        const auto originalDvs = Rules::storedDVs(s.working);
        const int form = Picker::previewUnownForm(s, wantsShiny, "silver_gbc");
        require(form == hypothetical.working.form && form >= 0 && form < 26,
                "chooser form equals the eventual native Unown letter");
        require(Rules::storedDVs(s.working) == originalDvs,
                "preview does not alter working DV bytes");
        if (wantsShiny) require(form == 8 || form == 21, "shiny preview can only be I or V");
    }
    const auto chooserSource = read("include/UI/SharedSpeciesPicker.h");
    const auto chooserOverlay = read("src/UI/Gen2PokemonPickerOverlay.inc");
    // Round 11 SPRITE: one preview artwork in the current shiny state, so the native letter
    // computed for that state is never paired with the other appearance.
    require(chooserSource.find("getIconSprite(species, form, previewShiny)") != std::string::npos &&
            chooserSource.find("getSprite(preview, form, previewShiny)") != std::string::npos &&
            chooserSource.find("getSprite(preview, form, false)") == std::string::npos &&
            chooserSource.find("getSprite(preview, form, true)") == std::string::npos,
            "picker renders one preview using the native Unown form for its shiny state");
    require(chooserOverlay.find("Gen2Picker::previewUnownForm(") != std::string::npos &&
            chooserOverlay.find("&overlay.speciesChooser, unownForm)") != std::string::npos,
            "Gen II supplies computed form to shared chooser");

    // Another species: form 0.
    Rules::Session other;
    other.begin(shiny.working, Rules::SessionMode::Edit);
    require(other.setSpecies(25, "silver_gbc") && other.working.form == 0, "non-Unown form 0");

    // Keep -> reparse: the saved letter (Gen2ReadOnlySave) equals the draft letter. Round 11 (owner
    // H28): Unown keeps none of Pikachu's moves and none is filled in, so the user picks Hidden Power
    // (Unown's only move) before Keep (CreateRequiredFields).
    require(s.setMove(0, 237), "Hidden Power");
    require(s.keep(*editor, 1, 0, error), "keep: " + error);
    const auto back = editor->boxedPokemon(1, 0, error);
    require(back && back->species == 201 && back->form == s.working.form, "reparsed letter == draft letter");

    // The two UI-only paths (Switch translation unit) are pinned by source.
    const auto surface = read("src/UI/Gen2SharedPokemonSurface.inc");
    require(surface.find("PokeBank::UIModel::Gen12DVReroll::unownLetter({p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]})") != std::string::npos &&
            surface.find("SpriteManager::getSprite(p.species, spriteForm, p.shiny)") != std::string::npos &&
            surface.find("SpriteManager::getSprite(p.species, p.shiny)") == std::string::npos,
            "editor sprite draws the DV-derived Unown letter");
    const auto foundation = read("src/UI/Gen2PokemonEditorFoundation.inc");
    require(foundation.find("p.form = p.species == 201 ? static_cast<uint8_t>(PokeBank::UIModel::Gen12DVReroll::unownLetter(dvs)) : 0;") != std::string::npos,
            "applyEditableDVs keeps the draft letter in step with every DV change");

    std::cout << "Gen II Unown letter (pokecrystal GetUnownLetter, all 65536 DV sets; screenshots V vs T; "
                 "draft form and editor sprite follow DVs): PASS\n";
    return 0;
}
