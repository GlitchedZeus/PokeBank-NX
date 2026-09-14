#pragma once
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/Common.h"
#include <algorithm>

namespace UI::SharedSpeciesPicker {
// Extracted verbatim from the accepted Gen I visual picker. Generations supply
// only the dex bound, committed appearance preview and display names.
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
        fb.drawText(previewX, y + 86, titleText(preview), Colors::Text, TextStyle::Heading);
        fb.drawText(previewX, y + 122, "NORMAL", !previewShiny ? Colors::Accent : Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX + 190, y + 122, "SHINY", previewShiny ? Colors::ShinyStar : Colors::TextDim, TextStyle::Caption);
        if (auto* normal = SpriteManager::getSprite(preview, false); normal && normal->data)
            fb.drawSpriteStaticContained(previewX, y + 152, 165, 190, normal->width, normal->height, normal->data, normal->channels);
        else fb.drawText(previewX + 20, y + 235, "No sprite", Colors::TextDim, TextStyle::Caption);
        if (auto* shiny = SpriteManager::getSprite(preview, true); shiny && shiny->data)
            fb.drawSpriteStaticContained(previewX + 188, y + 152, 165, 190, shiny->width, shiny->height, shiny->data, shiny->channels);
        else fb.drawText(previewX + 208, y + 235, "No shiny sprite", Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX, y + 364,
            std::string("Intended: ") + (previewShiny ? "Shiny" : "Normal"),
            previewShiny ? Colors::ShinyStar : Colors::Accent, TextStyle::Body);
        fb.drawText(previewX, y + 400, "A commits to draft/editor only", Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX, y + 424, "B restores previous committed choice", Colors::TextDim, TextStyle::Caption);
}
} // namespace UI::SharedSpeciesPicker
