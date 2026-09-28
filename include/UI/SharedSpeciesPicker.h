#pragma once
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Pokemon/PersonalInfoTable.h"
#include "UI/ClassicTypeBadges.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/Common.h"
#include <algorithm>
#include <array>
#include <string_view>

namespace UI::SharedSpeciesPicker {

struct ModalGeometry {
    int x = 0;
    int y = 0;
    int width = 1080;
    int height = 520;
};

inline constexpr int ModalWidth = 1080;
inline constexpr int ModalHeight = 520;
inline constexpr int NavBarHeight = 48;

// One shared shell for Gen I / II / III species choice.
// Theme accent belongs on the focused row and hints, not around the entire dialog.
inline ModalGeometry modalGeometry(const PKSEFramebuffer& fb) {
    const int contentHeight = std::max(ModalHeight, fb.getHeight() - NavBarHeight);
    return {
        (fb.getWidth() - ModalWidth) / 2,
        (contentHeight - ModalHeight) / 2,
        ModalWidth,
        ModalHeight
    };
}

inline ModalGeometry drawModalChrome(
    PKSEFramebuffer& fb, std::string_view title = "Choose Species") {
    const auto g = modalGeometry(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - NavBarHeight, Color(0, 0, 0, 105));
    fb.drawSoftShadow(g.x, g.y, g.width, g.height, 18);
    fb.drawFilledRoundedRect(g.x, g.y, g.width, g.height, 18, Colors::Panel);
    fb.drawRoundedRect(g.x, g.y, g.width, g.height, 18, Colors::Divider, 1);
    fb.drawText(g.x + 24, g.y + 18, std::string(title), Colors::Text, TextStyle::Heading);
    return g;
}

// Convert canonical type ids to the classic badge-table numbering used by the
// Gen I-IV shared picker previews.
inline uint8_t canonicalToClassicType(uint8_t normalized) noexcept {
    if (normalized <= 5) return normalized;
    if (normalized == 6) return 7;
    if (normalized == 7) return 8;
    if (normalized == 8) return 9;
    if (normalized >= 9 && normalized <= 16)
        return static_cast<uint8_t>(20 + (normalized - 9));
    return 0xFF;
}

namespace {

// Keep the native pre-Gen-IV tables local. Later generations can pass an exact
// type pair into drawContent without making every picker user link their data table.
inline std::array<uint8_t, 2> classicPickerTypes(uint16_t species, int speciesCount) {
    if (speciesCount == 151)
        return PokeVault::Integration::Gen1::StagedPokemonEditor::personalTypes(species);
    if (speciesCount == 251) {
        if (const auto* personal = PokeVault::Integration::Gen2::personalRecord(species))
            return {personal->rawType1, personal->rawType2};
    }
    if (speciesCount == 386) {
        const auto& personal = Pokemon::getPersonalInfoG3(species);
        return {canonicalToClassicType(personal.type1),
                canonicalToClassicType(personal.type2)};
    }
    return {0, 0};
}

} // namespace

// Extracted from the accepted Gen I visual picker and shared with Generation II.
// Browsing is preview-only; the generation supplies only its dex bound and text.
template <class RowText, class TitleText>
void drawContent(PKSEFramebuffer& fb, int x, int y, int selectedSpecies,
                 int speciesCount, bool previewShiny, RowText rowText, TitleText titleText,
                 bool shinyToggleAvailable = true,
                 std::array<uint8_t, 2> previewTypes = {0xFF, 0xFF}) {
        fb.drawText(x + 24, y + 50,
                    shinyToggleAvailable
                        ? "One species row • hover preview only • Y chooses intended Normal/Shiny appearance"
                        : "One species row • hover preview only • Shiny is edited from the shared field",
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

        // Generation-correct type badges update with the highlighted species.
        const auto types = (previewTypes[0] == 0xFF && previewTypes[1] == 0xFF)
            ? classicPickerTypes(preview, speciesCount)
            : previewTypes;
        ClassicTypeBadges::drawPairCentered(fb, previewX, previewW, y + 344, types[0], types[1]);

        fb.drawText(previewX, y + 374,
            std::string("Intended: ") + (previewShiny ? "Shiny" : "Normal"),
            previewShiny ? Colors::ShinyStar : Colors::Accent, TextStyle::Body);
        fb.drawText(previewX, y + 406, "A commits to draft/editor only", Colors::TextDim, TextStyle::Caption);
        fb.drawText(previewX, y + 430,
                    shinyToggleAvailable ? "B restores previous committed choice"
                                         : "B cancels species choice; Shiny remains separate",
                    Colors::TextDim, TextStyle::Caption);
}
} // namespace UI::SharedSpeciesPicker
