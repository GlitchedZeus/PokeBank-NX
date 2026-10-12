#pragma once
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Pokemon/PersonalInfoTable.h"
#include "UI/ClassicTypeBadges.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/Common.h"
#include "UI/TouchGesture.h"
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

// Match release-time scroll ownership to the very same clipped list used by drawContent.
// The Switch framebuffer and touch panel both use 1280x720 logical pixels; keep optional
// dimensions for render/layout tests without exposing libnx TouchInput to this header.
inline bool gestureStartedInList(int sx, int sy, int frameW = 1280, int frameH = 720) noexcept {
    const int contentHeight = std::max(ModalHeight, frameH - NavBarHeight);
    const int modalX = (frameW - ModalWidth) / 2;
    const int modalY = (contentHeight - ModalHeight) / 2;
    constexpr int listX = 20, listY = 84, listW = 560, rowStep = 43, visible = 9;
    return sx >= modalX + listX && sx < modalX + listX + listW &&
           sy >= modalY + listY && sy < modalY + listY + visible * rowStep;
}

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

inline bool touchStartedInside(const TouchGestureSnapshot& touch,
                               int x, int y, int w, int h) noexcept {
    return touch.down && touch.startX >= x && touch.startX < x + w &&
           touch.startY >= y && touch.startY < y + h;
}

} // namespace

// Extracted from the accepted Gen I visual picker and shared with Generation II.
// Browsing is preview-only; the generation supplies only its dex bound and text.
template <class RowText, class TitleText>
void drawContent(PKSEFramebuffer& fb, int x, int y, int selectedSpecies,
                 int speciesCount, bool previewShiny, RowText rowText, TitleText titleText,
                 const TouchGestureSnapshot* gesture = nullptr,
                 bool shinyToggleAvailable = true,
                 std::array<uint8_t, 2> previewTypes = {0xFF, 0xFF}) {
        fb.drawText(x + 24, y + 50,
                    shinyToggleAvailable
                        ? "Drag to browse • tap to choose • Y chooses intended Normal/Shiny appearance"
                        : "Drag to browse • tap to choose • Shiny is edited from the shared field",
                    Colors::TextDim, TextStyle::Caption);
        constexpr int visible = 9;
        constexpr int rowStep = 43;
        constexpr int rowHeight = 39;
        const int listX = x + 20, listW = 560;
        const int listY = y + 84;
        const int listH = visible * rowStep;

        // Direct-manipulation preview: while a finger is physically dragging this list, move the
        // rendered rows by the exact pixel delta. Whole rows are folded into a temporary visual
        // species and only the sub-row remainder is translated. The semantic selection is still
        // committed by the owning input handler on release, so drag can never become an accidental A.
        int visualSpecies = std::clamp(selectedSpecies, 1, speciesCount);
        int liveOffset = 0;
        if (gesture && touchStartedInside(*gesture, listX, listY, listW, listH) &&
            permitsVerticalPreview(*gesture)) {
            const int requestedRows = -gesture->deltaY / rowStep;
            visualSpecies = std::clamp(selectedSpecies + requestedRows, 1, speciesCount);
            const int appliedRows = visualSpecies - selectedSpecies;
            liveOffset = gesture->deltaY + appliedRows * rowStep;
            // Resist rather than expose empty space at either edge.
            if ((visualSpecies == 1 && liveOffset > 0) ||
                (visualSpecies == speciesCount && liveOffset < 0))
                liveOffset /= 3;
        }

        const int start = std::clamp(visualSpecies - visible / 2, 1,
                                     std::max(1, speciesCount - visible + 1));
        fb.setClipRect(listX, listY, listW, listH - 2);
        const int firstDraw = std::max(1, start - 1);
        const int lastDraw = std::min(speciesCount, start + visible);
        for (int species = firstDraw; species <= lastDraw; ++species) {
            const int rowY = listY + (species - start) * rowStep + liveOffset;
            const bool selected = species == visualSpecies;
            if (selected) fb.drawSelectionHighlight(listX, rowY, listW, rowHeight);
            fb.drawText(listX + 16, rowY + 9,
                        rowText(static_cast<uint16_t>(species)),
                        selected ? Colors::Text : Colors::TextDim);
        }
        fb.clearClip();

        const uint16_t preview = static_cast<uint16_t>(visualSpecies);
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
