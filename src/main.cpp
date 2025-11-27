#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <bits/stdc++.h>

#include "Globals.h"
#include "GetSaveFileContents.h"
#include "Utils/Logger.h"
#include "Utils/Utilities.h"
#include "Utils/FileUtilities.h"
#include "UI/SpriteManager.h"
#include "UI/UI.h"

int main()
{
    logInfoToFile("Initializing PKSE...");

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
    logInfoToFile("Initializing ROMFS...");
    Result romfsInitResult = romfsInit();
    bool romfsInitialized = false;
    if (R_FAILED(romfsInitResult)) {
        logErrorToFile("Failed to initialize ROMFS - sprites will not be available");
        // Don't exit, app can still run without sprites
    } else {
        logInfoToFile("ROMFS initialized successfully");
        romfsInitialized = true;
    }

    // Initialize sprite manager for Pokemon images
    logInfoToFile("Initializing Sprite Manager...");
    SpriteManager::init();

    // Test sprite loading
    logInfoToFile("Testing sprite loading...");
    Sprite* testSprite = SpriteManager::getSprite(25, false); // Pikachu
    if (testSprite && testSprite->data) {
        logInfoToFile("SUCCESS: Test sprite loaded! (" +
            std::to_string(testSprite->width) + "x" +
            std::to_string(testSprite->height) + ")");
    } else {
        logInfoToFile("WARNING: Test sprite failed to load - sprites may not be available");
    }

    logInfoToFile("Starting UI Manager...");

    UIManager uiManager;
    uiManager.run();

    // Cleanup
    logInfoToFile("Cleaning up Sprite Manager...");
    SpriteManager::cleanup();

    if (romfsInitialized) {
        logInfoToFile("Cleaning up ROMFS...");
        romfsExit();
    }

    accountExit();
    nsExit();

    return 0;
}
