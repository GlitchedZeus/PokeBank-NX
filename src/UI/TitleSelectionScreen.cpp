#include "UI/TitleSelectionScreen.h"
#include "UI/Common.h"
#include "Utils/Utilities.h"
#include "Utils/Logger.h"
#include "Enums/GameVersion.h"

// UI Layout constants
constexpr int LEFT_PANEL_X = 0;
constexpr int LEFT_PANEL_Y = 70;
constexpr int LEFT_TITLE_SELECTION_PANEL_WIDTH = 500;
constexpr int LEFT_TITLE_SELECTION_PANEL_HEIGHT = 500;

TitleSelectionScreen::TitleSelectionScreen(AccountUid userUid)
    : userUid(userUid), selectedIndex(0), titleSelected(false), goBack(false) {
    loadTitles();
}

// /// Vector to hold pokemon title IDs
// std::vector<u64> pokemonTitleIds = {
//     // We only want to allow titles that are supported or being developed for integration
    
//     // Gen 7
//     // 0x010003F003A34000, // Let's Go Pikachu
//     // 0x0100187003A36000, // Let's Go Eevee
//     // Gen 8
//     0x0100ABF008968000, // Pokemon Sword
//     0x01008DB008C2C000, // Pokemon Shield
//     // 0x0100000011D90000, // Brilliant Diamond
//     // 0x010018E011D92000, // Shining Pearl
//     // 0x01001F5010DFA000, // Legends: Arceus
//     // // Gen 9
//     // TODO: CURRENTLY WORKING ON SV INTEGRATION
//     // 0x0100A3D008C5C000, // Scarlet
//     // 0x01008F6008C5E000, // Violet
//     // 0x0100F43008C44000, // Legends: Z-A
// };

// /**
//  * Checks if a title ID belongs to a mainline Pokemon game for Nintendo Switch.
//  *
//  * This function checks against all mainline Pokemon games available on Switch:
//  * - Pokemon Let's Go Pikachu/Eevee (Gen 7)
//  * - Pokemon Sword/Shield (Gen 8)
//  * - Pokemon Brilliant Diamond/Shining Pearl (Gen 8 remakes)
//  * - Pokemon Legends: Arceus (Gen 8)
//  * - Pokemon Scarlet/Violet (Gen 9)
//  * - Pokemon Legends: Z-A (Gen 9)
//  *
//  * @param titleId The application title ID to check
//  * @return true if the title ID is a Pokemon game, false otherwise
//  */
// bool TitleSelectionScreen::isPokemonTitle(u64 titleId) const {
//     for(int i = 0; i < pokemonTitleIds.size(); i++) {
//         if (titleId == pokemonTitleIds[i]) { return true; }
//     }

//     return false;
// }

void TitleSelectionScreen::loadTitles() {
    titles.clear();

    NsApplicationRecord records[100];
    s32 recordCount = 0;

    Result rc = nsListApplicationRecord(records, 100, 0, &recordCount);
    if (R_FAILED(rc)) {
        logErrorToFile("Failed to list application records");
        return;
    }

    for (s32 i = 0; i < recordCount; i++) {
        u64 titleId = records[i].application_id;

        // Filter to show only Pokemon titles using isPokemonTitle function
        GameVersion gameVersion = getGameVersion(titleId);
        if (gameVersion != GameVersion::Invalid) {
            TitleInfo info;
            info.titleId = titleId;
            // Apply sanitization and formatting to handle special characters
            info.name = "Pokemon " + getGameVersionName(gameVersion);
            titles.push_back(info);
        }
    }

    if (titles.empty()) {
        TitleInfo info;
        info.titleId = 0;
        info.name = "No Pokemon titles found";
        titles.push_back(info);
    }
}

void TitleSelectionScreen::update(const PadState& pad) {
    u64 kDown = padGetButtonsDown(&pad);

    if (kDown & HidNpadButton_Up) {
        selectedIndex = (selectedIndex - 1 + titles.size()) % titles.size();
    }
    if (kDown & HidNpadButton_Down) {
        selectedIndex = (selectedIndex + 1) % titles.size();
    }
    if (kDown & HidNpadButton_A) {
        if (!titles.empty() && titles[selectedIndex].titleId != 0) {
            selectedTitleId = titles[selectedIndex].titleId;
            selectedTitleName = titles[selectedIndex].name;
            titleSelected = true;
        }
    }
    if (kDown & HidNpadButton_B || kDown & HidNpadButton_Plus) {
        goBack = true;
    }
}

void TitleSelectionScreen::draw(PKSEFramebuffer& fb) {
    fb.clear(Colors::Background);

    // Draw title bar
    fb.drawFilledRect(0, 0, fb.getWidth(), 60, Colors::Panel);
    fb.drawText(20, 20, "PKSE - Pokemon Save Editor v0.0.1", Colors::Text);
    fb.drawRect(0, 0, fb.getWidth(), 60, Colors::Border);

    // Draw title selection panel
    fb.drawFilledRect(LEFT_PANEL_X, LEFT_PANEL_Y, LEFT_TITLE_SELECTION_PANEL_WIDTH, LEFT_TITLE_SELECTION_PANEL_HEIGHT, Colors::Panel);
    fb.drawRect(LEFT_PANEL_X, LEFT_PANEL_Y, LEFT_TITLE_SELECTION_PANEL_WIDTH, LEFT_TITLE_SELECTION_PANEL_HEIGHT, Colors::Border);

    // Draw panel title
    fb.drawText(LEFT_PANEL_X + 20, LEFT_PANEL_Y + 20, "Select Pokemon Title", Colors::Text);
    fb.drawFilledRect(LEFT_PANEL_X + 20, LEFT_PANEL_Y + 45, LEFT_TITLE_SELECTION_PANEL_WIDTH - 40, 2, Colors::Border);

    // Draw title list
    drawTitleList(fb);

    // Draw instructions
    fb.drawText(LEFT_PANEL_X + 20, LEFT_PANEL_Y + LEFT_TITLE_SELECTION_PANEL_HEIGHT + 20, "Press A to select  |  Press B to go back", Colors::TextDim);
}

void TitleSelectionScreen::drawTitleList(PKSEFramebuffer& fb) {
    int itemHeight = 50;
    int startY = LEFT_PANEL_Y + 60;

    for (size_t i = 0; i < titles.size(); i++) {
        int itemY = startY + (i * itemHeight);

        if ((int)i == selectedIndex) {
            fb.drawFilledRect(LEFT_PANEL_X + 10, itemY, 480, itemHeight - 5, Colors::Selected);
        }

        std::string displayText = "> " + titles[i].name;
        fb.drawText(LEFT_PANEL_X + 30, itemY + 15, displayText, Colors::Text);
    }
}
