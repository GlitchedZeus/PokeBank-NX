#pragma once
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "UI/ClassicTypeBadges.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/Common.h"
#include <algorithm>
#include <array>

namespace UI::SharedSpeciesPicker {
namespace {

// The shared picker is currently used by the supported RBY and GSC editors only.
// Keep their exact native type tables here so the preview never guesses modern typing.
inline std::array<uint8_t, 2> classicPickerTypes(uint16_t species, int speciesCount) {
    if (speciesCount == 151)
        return PokeVault::Integration::Gen1::StagedPokemonEditor::personalTypes(species);
    if (speciesCount == 251) {
        if (const auto* personal = PokeVault::Integration::Gen2::personalRecord(species))
            return {personal->rawType1, personal->rawType2};
    }
    return {0, 0};
}

} // namespace

// Extracted from the accepted Gen I visual picker and shared with Generation II.
// Browsing is preview-only; the generation supplies only its dex bound and text.
template <class RowText, class TitleText>
void drawContent(PKSEFramebuffer& fb, int x, int y, int selectedSpecies,
                 int speciesCount, bool previewShiny, RowText rowText, TitleText titleText) {
        fb.drawText(x + 24, y + 50, "One species row • hover preview only • Y chooses intended Normal/Shiny appearance",
                    Colors::TextDim, TextStyle::Caption);
        constexpr int visible = 9;
        const int start = std::clamp(selectedSpecies - visible / 2, 1, speciesCount - visible + 1);
        const int listX = x + 20, listW = 560;
        for (int i = 0; i < visible; ++i) {
            const int species = start + i;
            const int rowY = y + 84 + i * 43;
            const bool selected = species == selectedSpecies;
            if (selected) fb.drawSelectionHighlight(listX, rowY, listW, 39);
            fb.drawText(listX + 16, rowY + 9,
                        rowText(static_cast<uint16_t>(species)),
                        selected ? Colors::Text : Colors::TextDim);
        }

        const uint16_t preview = static_cast<uint16_t>(selectedSpecies);
        const int previewX = x + 610;
        constexpr int previewW = 353;
        fb.drawText(previewX, y + 86, titleText(preview), Colors::Text, TextStyle::Heading);
        fb.drawText(previewX, y + 122, "NORMAL", !previewShiny ? Colors::Accent : Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX + 190, y + 122, "SHINY", previewShiny ? Colors::ShinyStar : Colors::TextDim, TextStyle::Caption);
        if (auto* normal = SpriteManager::getSprite(preview, false); normal && normal->data)
            fb.drawSpriteStaticContained(previewX, y + 152, 165, 190, normal->width, normal->height, normal->data, normal->channels);
        else fb.drawText(previewX + 20, y + 235, "No sprite", Colors::TextDim, TextStyle::Caption);
        if (auto* shiny = SpriteManager::getSprite(preview, true); shiny && shiny->data)
            fb.drawSpriteStaticContained(previewX + 188, y + 152, 165, 190, shiny->width, shiny->height, shiny->data, shiny->channels);
        else fb.drawText(previewX + 208, y + 235, "No shiny sprite", Colors::TextDim, TextStyle::Caption);

        // Generation-correct canonical type badges update with the highlighted species.
        const auto types = classicPickerTypes(preview, speciesCount);
        ClassicTypeBadges::drawPairCentered(fb, previewX, previewW, y + 344, types[0], types[1]);

        fb.drawText(previewX, y + 374,
            std::string("Intended: ") + (previewShiny ? "Shiny" : "Normal"),
            previewShiny ? Colors::ShinyStar : Colors::Accent, TextStyle::Body);
        fb.drawText(previewX, y + 406, "A commits to draft/editor only", Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX, y + 430, "B restores previous committed choice", Colors::TextDim, TextStyle::Caption);
}
} // namespace UI::SharedSpeciesPicker
