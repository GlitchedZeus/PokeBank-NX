
#include <cstdio>
#include <sys/stat.h>

#include "Globals.h"
#include "Save/GetSaveFileContents.h"
#include "UI/UI.h"
#include "UI/SaveSelectScreen.h"
#include "UI/BackupSelectionScreen.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/HelperUtilities.h"
#include "Utils/Logger.h"
#include "Utils/FileUtilities.h"
#include "Trainer/Trainer.h"
#include "Games/GameIdentity.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Legacy/FRLGSourceBrowser.h"

using namespace Utils;
using namespace Trainer;

namespace UI {
    UIManager::UIManager()
        : running(true),
          legacySourceBindings(BASE_SAVE_DIRECTORY + "/legacy_source_bindings.cfg") {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        hidInitializeTouchScreen();  // enable the touchscreen alongside the gamepad

        mkdir(BASE_SAVE_DIRECTORY.c_str(), 0777);
        if (!legacySourceBindings.load())
            logErrorToFile("Legacy source bindings contain malformed or unreadable rows");

        // Discover only RetroArch's configured/conventional save roots. The runtime catalog keeps
        // the accepted Gen III path and the strict Gen I R/B/Y path separately typed, then presents
        // both through the same read-only Game Sources cards. No legacy source exposes a write path.
        legacyFRLGSources = PokeVault::Legacy::discoverConfiguredRetroArchFRLGSaves();
        size_t ready = 0;
        size_t ambiguous = 0;
        size_t rejected = 0;
        size_t gen1Ready = 0;
        size_t gen3Ready = 0;
        for (const auto& source : legacyFRLGSources.sources) {
            if (source.ready()) {
                ++ready;
                if (source.isGen1()) ++gen1Ready;
                if (source.isGen3()) ++gen3Ready;
            } else if (source.status == PokeVault::Legacy::LegacySourceStatus::AmbiguousIdentity) {
                ++ambiguous;
            } else {
                ++rejected;
            }
        }
        char legacySummary[224];
        snprintf(legacySummary, sizeof(legacySummary),
                 "RetroArch legacy: %zu files checked, %zu ready (Gen I %zu, Gen III %zu), %zu ambiguous, %zu rejected%s",
                 legacyFRLGSources.filesExamined, ready, gen1Ready, gen3Ready, ambiguous, rejected,
                 legacyFRLGSources.limitReached ? ", scan limit reached" : "");
        logInfoToFile(legacySummary);
    }

    UIManager::~UIManager() {
    }

    void UIManager::run() {
        while (appletMainLoop() && running) {
            handleSaveSelection();
        }
    }

    // Combined JKSV-style user + title picker: pick a user's avatar and one of their supported
    // Pokemon game icons in a single screen, then go straight to backup selection.
    void UIManager::handleSaveSelection() {
        SaveSelectScreen selectScreen(legacyFRLGSources, legacySourceBindings);
        fb.startFade();

        while (appletMainLoop() && running && !selectScreen.shouldExit()) {
            padUpdate(&pad);
            touch.update();
            selectScreen.update(pad, touch);
            selectScreen.draw(fb);
            fb.drawFadeOverlay();
            fb.flush();

            if (selectScreen.hasSelectedTitle()) {
                if (selectScreen.getSelectedSourceKind() ==
                    SaveSelectScreen::SelectedSourceKind::RetroArchFRLG) {
                    std::string error;
                    if (!handleLegacyFRLGView(selectScreen.getSelectedUser(),
                                              selectScreen.getSelectedLegacySourceIndex(),
                                              selectScreen.getSelectedGameId(), error))
                        logErrorToFile("Legacy RetroArch source refused open", error.c_str());
                } else {
                    handleBackupSelection(selectScreen.getSelectedUser(),
                                          selectScreen.getSelectedTitleId(),
                                          selectScreen.getSelectedTitleName());
                }
                // Back from backup/trainer -> return so run() rebuilds the picker (re-lists saves).
                return;
            }
        }

        running = false;   // + pressed -> exit the app
    }

    void UIManager::handleBackupSelection(AccountUid userUid, u64 titleId, const std::string& titleName) {
        BackupSelectionScreen backupScreen(titleId, titleName);
        fb.startFade();

        while (appletMainLoop() && running && !backupScreen.shouldExit()) {
            padUpdate(&pad);
            touch.update();
            backupScreen.update(pad, touch);
            backupScreen.draw(fb);
            fb.drawFadeOverlay();
            fb.flush();

            if (backupScreen.hasSelectedBackup()) {
                if (backupScreen.shouldCreateNewBackup()) {
                    logInfoToFile("Creating new backup for", titleName.c_str());

                    // Auto-backup ON -> new timestamped history folder (kept indefinitely; the user
                    // decides when to delete backups, so we never prune). OFF -> reuse a single
                    // "Working" copy so backups don't pile up.
                    std::string backupPath = backupSaveData(userUid, titleId, titleName, g_autoBackupEnabled);
                    if (backupPath.empty()) {
                        logErrorToFile("Failed to back up save data");
                        // Tell the user and stay put. Returning here (the old behaviour) dropped them
                        // back at the save picker with no message, which is exactly what pressing B
                        // does -- so a failed backup was indistinguishable from a cancel.
                        backupScreen.reportFailure("Couldn't create the backup. Check SD card space and try again.");
                        continue;
                    }
                    std::string error;
                    if (!handleTrainerView(userUid, titleId, titleName, backupPath, true, error)) {
                        backupScreen.reportFailure(error);
                        continue;
                    }
                } else {
                    // Use existing backup
                    logInfoToFile("Loading existing backup", backupScreen.getSelectedBackupPath().c_str());
                    std::string error;
                    if (!handleTrainerView(userUid, titleId, titleName,
                                           backupScreen.getSelectedBackupPath(), false, error)) {
                        backupScreen.reportFailure(error);
                        continue;
                    }
                }
                return;
            }
        }
    }

    bool UIManager::handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName,
                                      const std::string& backupDir, bool loadedFromCart,
                                      std::string& error) {
        logInfoToFile("Loading save from", backupDir.c_str());

        if (!Save::validateTrainerSaveForOpen(backupDir.c_str(), titleId, error)) {
            logErrorToFile("Save validation refused open", error.c_str());
            return false;
        }

        // Read trainer data from the specified backup directory
        // Auto-detects game version and uses appropriate reading function
        TrainerVariant trainerVariant = readTrainerInfo(backupDir.c_str(), titleId);

        // Use std::visit to extract reference and create TrainerViewScreen
        std::visit([&](auto& trainer) {
            TrainerViewScreen trainerScreen(
                trainer, titleName, backupDir, titleId, userUid,
                loadedFromCart ? PokeVault::Safety::SourceKind::InstalledGame
                               : PokeVault::Safety::SourceKind::BackupOrStaged);
            fb.startFade();

            while (appletMainLoop() && !trainerScreen.shouldExit() && !trainerScreen.hasRequestedExit()) {
                padUpdate(&pad);
                touch.update();
                trainerScreen.update(pad, touch);
                trainerScreen.draw(fb);
                fb.drawFadeOverlay();
                fb.flush();
            }

            // If user pressed + to exit app, stop running
            if (trainerScreen.hasRequestedExit()) {
                running = false;
            }
        }, trainerVariant);
        return true;
    }

    bool UIManager::handleLegacyFRLGView(
        AccountUid userUid, size_t sourceIndex, const std::string& gameId, std::string& error) {
        error.clear();
        if (sourceIndex >= legacyFRLGSources.sources.size()) {
            error = "RetroArch source selection is stale";
            return false;
        }

        const auto& selected = legacyFRLGSources.sources[sourceIndex];
        if (!selected.ready() || selected.gameId != gameId) {
            error = "RetroArch source is no longer a validated legacy save";
            return false;
        }
        const auto* identity = PokeVault::Games::findGame(selected.gameId);
        if (!identity) {
            error = "RetroArch source has no stable game identity";
            return false;
        }

        std::unique_ptr<Trainer::Trainer> trainer;
        if (selected.isGen1()) {
            if (identity->platform != PokeVault::Games::Platform::GameBoy ||
                selected.gen1Save->metadata().sourceGameId != gameId) {
                error = "RetroArch source is no longer a validated Generation I save";
                return false;
            }
            trainer = PokeVault::Legacy::RBYReadOnlyTrainer::create(*selected.gen1Save, error);
        } else if (selected.isGen3()) {
            if (identity->platform != PokeVault::Games::Platform::GameBoyAdvance ||
                selected.save->metadata().sourceGameId != gameId) {
                error = "RetroArch source is no longer a validated Generation III save";
                return false;
            }
            trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*selected.save, error);
            const bool rseInventoryUnavailable = selected.save->inventory().empty() &&
                (selected.gameId == "ruby_gba" || selected.gameId == "sapphire_gba" ||
                 selected.gameId == "emerald_gba");
            if (rseInventoryUnavailable)
                logErrorToFile("RSE inventory validation failed; save remains open read-only",
                               selected.gameId.c_str());
        }
        if (!trainer) {
            if (error.empty()) error = "validated RetroArch source has no supported read-only trainer bridge";
            return false;
        }

        TrainerViewScreen trainerScreen(
            *trainer, "Pokemon " + std::string(identity->title), selected.path, 0, userUid,
            PokeVault::Safety::SourceKind::RetroArchLegacy, selected.gameId);
        fb.startFade();
        while (appletMainLoop() && !trainerScreen.shouldExit() &&
               !trainerScreen.hasRequestedExit()) {
            padUpdate(&pad);
            touch.update();
            trainerScreen.update(pad, touch);
            trainerScreen.draw(fb);
            fb.drawFadeOverlay();
            fb.flush();
        }
        if (trainerScreen.hasRequestedExit()) running = false;
        return true;
    }
}
