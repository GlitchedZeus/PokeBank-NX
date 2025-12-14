
#include "Globals.h"
#include "Save/GetSaveFileContents.h"
#include "UI/UI.h"
#include "UI/UserSelectionScreen.h"
#include "UI/TitleSelectionScreen.h"
#include "UI/BackupSelectionScreen.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/HelperUtilities.h"
#include "Utils/Logger.h"
#include "Utils/FileUtilities.h"
#include "Trainer/Trainer.h"

using namespace Utils;
using namespace Trainer;

namespace UI {
    // UIManager implementation
    UIManager::UIManager() : running(true) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
    }

    UIManager::~UIManager() {
    }

    void UIManager::run() {
        while (appletMainLoop() && running) {
            handleUserSelection();
        }
    }

    void UIManager::handleUserSelection() {
        UserSelectionScreen userScreen;

        while (appletMainLoop() && !userScreen.shouldExit()) {
            padUpdate(&pad);
            userScreen.update(pad);
            userScreen.draw(fb);
            fb.flush();

            if (userScreen.hasSelectedUser()) {
                handleTitleSelection(userScreen.getSelectedUser());
                return;  // Start over with user selection
            }
        }

        running = false;
    }

    void UIManager::handleTitleSelection(AccountUid userUid) {
        while (appletMainLoop()) {
            TitleSelectionScreen titleScreen(userUid);

            while (appletMainLoop() && !titleScreen.shouldExit()) {
                padUpdate(&pad);
                titleScreen.update(pad);
                titleScreen.draw(fb);
                fb.flush();

                if (titleScreen.hasSelectedTitle()) {
                    handleBackupSelection(userUid, titleScreen.getSelectedTitleId(), titleScreen.getSelectedTitleName());
                    // After backup selection, loop back to title selection
                    break;
                }
            }

            // If user pressed B in title selection, exit to user selection
            if (titleScreen.shouldExit()) {
                return;
            }
        }
    }

    void UIManager::handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName) {
        BackupSelectionScreen backupScreen(titleId, titleName);

        while (appletMainLoop() && !backupScreen.shouldExit()) {
            padUpdate(&pad);
            backupScreen.update(pad);
            backupScreen.draw(fb);
            fb.flush();

            if (backupScreen.hasSelectedBackup()) {
                if (backupScreen.shouldCreateNewBackup()) {
                    // Create new backup from title
                    logInfoToFile("Creating new backup for", titleName.c_str());

                    if (!backupSaveData(userUid, titleId, titleName)) {
                        logErrorToFile("Failed to back up save data");
                        return;
                    }

                    char gameDir[512];

                    snprintf(gameDir, sizeof(gameDir), "%s/%s", BASE_SAVE_DIRECTORY.c_str(), titleName.c_str());
                    limitBackups(gameDir, BACKUP_SAVE_LIMIT);

                    // Get the newest backup (most recent timestamp)
                    std::vector<std::string> backupDirs = listBackupDirectories(gameDir);
                    if (!backupDirs.empty()) {
                        char backupPath[1024];
                        snprintf(backupPath, sizeof(backupPath), "%s/%s", gameDir, backupDirs[0].c_str());
                        handleTrainerView(userUid, titleId, titleName, std::string(backupPath));
                    }
                } else {
                    // Use existing backup
                    logInfoToFile("Loading existing backup", backupScreen.getSelectedBackupPath().c_str());
                    handleTrainerView(userUid, titleId, titleName, backupScreen.getSelectedBackupPath());
                }
                return;
            }
        }
    }

    void UIManager::handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName, const std::string& backupDir) {
        logInfoToFile("Loading save from", backupDir.c_str());

        // Read trainer data from the specified backup directory
        // Auto-detects game version and uses appropriate reading function
        TrainerVariant trainerVariant = readTrainerInfo(backupDir.c_str(), titleId);

        // Use std::visit to extract reference and create TrainerViewScreen
        std::visit([&](auto& trainer) {
            TrainerViewScreen trainerScreen(trainer, titleName, backupDir, titleId, userUid);

            while (appletMainLoop() && !trainerScreen.shouldExit() && !trainerScreen.hasRequestedExit()) {
                padUpdate(&pad);
                trainerScreen.update(pad);
                trainerScreen.draw(fb);
                fb.flush();
            }

            // If user pressed + to exit app, stop running
            if (trainerScreen.hasRequestedExit()) {
                running = false;
            }
        }, trainerVariant);
    }
}
