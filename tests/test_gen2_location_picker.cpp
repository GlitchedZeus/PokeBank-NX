// Crystal Met Location chooser (owner campaign phase 11): sized to its choices, the same geometry
// for drawing and hit-testing, and names only in every row (no raw landmark index or source tag).
// Gen I stores no met location, so there is no Gen I chooser to resize; this pins that too.
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/SharedMenuGeometry.h"

#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace P = PokeBank::UIModel::Gen2PokemonPicker;
namespace E = PokeVault::Integration::EncounterGuardrails;

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    // Proportional: one row per choice up to 9, then it scrolls; always above the footer.
    int previous = 0;
    for (int count = 0; count <= 40; ++count) {
        const auto g = P::locationPickerGeometry(count);
        assert(g.visible == std::clamp(count, 1, 9));
        assert(g.height == 80 + g.visible * g.rowStep + 16);           // NEW-10: compact header / bottom
        assert(g.listY == g.y + 80 && g.listY >= g.y + 52 + 20);       // rows clear the caption line
        assert(g.height >= previous);
        previous = g.height;
        assert(g.x >= 0 && g.x + g.width <= 1280 && g.y >= 0 && g.y + g.height <= 720 - 56);
        assert(g.listY + g.visible * g.rowStep <= g.y + g.height - 16); // every visible row fully inside
    }
    // Three choices is a small window, not the old fixed 568 px one.
    assert(P::locationPickerGeometry(3).height == 80 + 3 * 44 + 16 && P::locationPickerGeometry(3).height < 244);

    // Every Crystal encounter template: a real place name and a method/level detail.
    int rows = 0;
    for (uint16_t species = 1; species <= 251; ++species) {
        for (const auto& encounter : E::forGameSpecies("crystal_gbc", species)) {
            const auto text = P::locationRowText(encounter);
            assert(!text.place.empty() && text.place != "Unused / invalid");
            assert(text.place.find("0x") == std::string::npos && text.place.find('#') == std::string::npos);
            bool allDigits = true;
            for (const char c : text.place) allDigits = allDigits && std::isdigit(static_cast<unsigned char>(c));
            assert(!allDigits);
            assert(text.detail.find(E::methodName(encounter.method)) == 0);
            assert(text.detail.find(" · Lv. ") != std::string::npos);
            ++rows;
        }
    }
    assert(rows > 100);

    // Live: draw and input both use the one geometry; no developer wording; no Gen I chooser.
    const auto overlay = read("src/UI/Gen2PokemonPickerOverlay.inc");
    std::size_t uses = 0;
    for (auto at = overlay.find("locationPickerGeometry("); at != std::string::npos;
         at = overlay.find("locationPickerGeometry(", at + 1)) ++uses;
    assert(uses == 3); // draw, touch scroll, A reveal
    assert(overlay.find("Drag naturally to browse Crystal encounters") == std::string::npos);
    assert(overlay.find("\"Met Location\"") != std::string::npos);
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayFoundation.inc",
                             "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"})
        assert(read(path).find("Met Location") == std::string::npos);
    // Round 11 (the FRLG closed-list failure, Crystal path): a species with no Crystal catch
    // template (Charizard, Rayquaza-like traded-in species) used to get "No supported Crystal
    // encounter templates for this species" and no chooser. It is never a closed list.
    // Round 12 (R12-004 / R12-005, owner H08; updated deliberately): the DEFAULT list is the verified
    // templates the level reaches, the Gift / Event markers and "Other location (unverified)...";
    // every other landmark (and each template above the level) is in the UNVERIFIED list.
    {
        namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
        namespace Policy = PokeVault::Integration::MetLocationPolicy;
        assert(E::forGameSpecies("crystal_gbc", 6).empty());
        P::Model none;
        none.openLocation("crystal_gbc", 6, 0, 0, 36);
        // Charmander / Charmeleon have no Crystal templates either: markers + the entry row only.
        assert(none.locationCount() == 3 && none.encounterChoices.empty());
        assert(none.index == 0 && none.locationChoice() == nullptr);
        assert(P::locationRowTextAt(none, 0).detail == Rows::kGiftMarkerDetail);
        assert(P::locationRowTextAt(none, 1).detail == Rows::kEventMarkerDetail);
        assert(none.unverifiedEntryAt(2) && P::locationRowTextAt(none, 2).place == Policy::kUnverifiedListRowPlace);
        P::Model noneAll;
        noneAll.openLocation("crystal_gbc", 6, 0, 0, 36, true);
        assert(noneAll.locationCount() + 2 == static_cast<int>(Rows::crystalOfferedLocations().size()));
        assert(noneAll.locationCount() == 0x5F);                        // landmarks 1..0x5F
        assert(noneAll.otherLocationChoice() && *noneAll.otherLocationChoice() == 1);   // New Bark Town
        for (int i = 0; i < noneAll.locationCount(); ++i) {
            const auto text = P::locationRowTextAt(noneAll, i);
            assert(!text.place.empty() && text.place != "Unused / invalid" && text.place != "Unknown");
            assert(text.detail.find("unverified") != std::string::npos);
            assert(text.detail.find(" · Lv. ") == std::string::npos);   // never presented as a catch
        }
        // A stored non-template landmark is focused on its own row (in the unverified list).
        P::Model stored;
        stored.openLocation("crystal_gbc", 6, 48, 30, 36, true);        // Route 1
        assert(stored.otherLocationChoice() && *stored.otherLocationChoice() == 48);

        // Pidgey has Crystal templates: the ones its level reaches come first, then the markers.
        P::Model pidgey;
        pidgey.openLocation("crystal_gbc", 16, 0, 0, 100);
        assert(!pidgey.encounterChoices.empty() &&
               pidgey.locationCount() == static_cast<int>(pidgey.encounterChoices.size()) + 3);
        for (int i = 0; i < static_cast<int>(pidgey.encounterChoices.size()); ++i) {
            pidgey.index = i;
            assert(pidgey.locationChoice() && !pidgey.otherLocationChoice());
            assert(P::locationRowTextAt(pidgey, i).detail ==
                   P::locationRowText(pidgey.encounterChoices[static_cast<std::size_t>(i)]).detail);
        }
        pidgey.index = pidgey.locationCount() - 2;
        assert(!pidgey.locationChoice() && pidgey.otherLocationChoice() && *pidgey.otherLocationChoice() == 0x7F);
        assert(pidgey.unverifiedEntryAt(pidgey.locationCount() - 1));
        // R12-005: at Lv. 2 every Pidgey template above Lv. 2 is in the unverified list, saying why.
        P::Model low;
        low.openLocation("crystal_gbc", 16, 0, 0, 2, true);
        assert(!low.levelBlocked.empty());
        for (std::size_t i = 0; i < low.levelBlocked.size(); ++i)
            assert(P::locationRowTextAt(low, static_cast<int>(i)).detail.find(Policy::kLevelBelowEncounter) !=
                   std::string::npos);
        for (const auto id : low.otherLocations)
            for (const auto& t : low.levelBlocked) assert(t.location != id);
        pidgey.close();
        assert(pidgey.otherLocations.empty() && pidgey.locationCount() == 0);
        // Round 12: every row of both lists (ancestry "via ..." rows, merged time rows, rows above
        // the level) fits one line of the 640 px chooser: place (Body) + detail (Caption) inside
        // listWidth - 2 x 14 px padding - a 20 px gap (pinned Nunito metrics).
        namespace G = PokeBank::UIModel::SharedMenuGeometry;
        const int room = P::locationPickerGeometry(9).listWidth - 28 - 20;
        int widest = 0;
        for (uint16_t species = 1; species <= 251; ++species)
            for (const uint8_t level : {uint8_t{100}, uint8_t{1}})
                for (const bool unverified : {false, true}) {
                    P::Model m;
                    m.openLocation("crystal_gbc", species, 0, 0, level, unverified);
                    for (int i = 0; i < m.locationCount(); ++i) {
                        const auto text = P::locationRowTextAt(m, i);
                        const int w = G::textWidth(text.place, G::Text::Body) + G::textWidth(text.detail, G::Text::Caption);
                        if (w > room) std::cerr << "clipped: " << text.place << " | " << text.detail << '\n';
                        widest = std::max(widest, w);
                    }
                }
        assert(widest <= room);

        const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
        assert(unified.find("No supported Crystal encounter templates for this species") == std::string::npos);
        assert(unified.find("Met Level (not a listed Crystal encounter)") != std::string::npos);
        assert(overlay.find("Gen2Picker::locationRowTextAt(model, i)") != std::string::npos);
        assert(overlay.find("picker.model.otherLocationChoice()") != std::string::npos);
        assert(overlay.find("picker.model.unverifiedEntrySelected()") != std::string::npos);
    }
    std::cout << "Crystal Met Location chooser: proportional, one geometry, names only PASS\n";
    return 0;
}
