#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <bits/stdc++.h>

#include "Globals.h"
#include "Save/GetSaveFileContents.h"
#include "UI/SpriteManager.h"
#include "UI/UI.h"
#include "Utils/Logger.h"
#include "Utils/HelperUtilities.h"
#include "Utils/FileUtilities.h"

int main()
{
    logInfoToFile("Initializing PKSE...");

    // Clean up old log files
    Utils::cleanupOldLogs();

    // Initialize the ns service
    Result nsServiceInitializeResult = nsInitialize();
    if (R_FAILED(nsServiceInitializeResult)) {
        logErrorToFile("Failed to initialize ns service");
        return -1;
    }

    // Initialize account service
    Result accountServiceInitializeResult = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(accountServiceInitializeResult)) {
        logErrorToFile("Failed to initialize account service");
        nsExit();
        return -1;
    }

    // Initialize ROMFS for accessing bundled sprites
    Utils::logInfoToFile("Initializing ROMFS...");
    Result romfsInitResult = romfsInit();
    bool romfsInitialized = false;
    if (R_FAILED(romfsInitResult)) {
        logErrorToFile("Failed to initialize ROMFS - sprites will not be available");
        // Don't exit, app can still run without sprites
    } else {
        Utils::logInfoToFile("ROMFS initialized successfully");
        romfsInitialized = true;
    }

    // Initialize sprite manager for Pokemon images
    Utils::logInfoToFile("Initializing Sprite Manager...");
    UI::SpriteManager::init();

    // Test sprite loading
    Utils::logInfoToFile("Testing sprite loading...");
    UI::Sprite* testSprite = UI::SpriteManager::getSprite(25, false); // Pikachu
    if (testSprite && testSprite->data) {
        logInfoToFile(("SUCCESS: Test sprite loaded! (" +
            std::to_string(testSprite->width) + "x" +
            std::to_string(testSprite->height) + ")").c_str());
    } else {
        Utils::logInfoToFile("WARNING: Test sprite failed to load - sprites may not be available");
    }

    Utils::logInfoToFile("Starting UI Manager...");

    UI::UIManager uiManager;
    uiManager.run();

    // Cleanup
    Utils::logInfoToFile("Cleaning up Sprite Manager...");
    UI::SpriteManager::cleanup();

    if (romfsInitialized) {
        Utils::logInfoToFile("Cleaning up ROMFS...");
        romfsExit();
    }

    accountExit();
    nsExit();

    return 0;
}
