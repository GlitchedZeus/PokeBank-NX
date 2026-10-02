
#include <cstdio>
#include <sys/stat.h>
#include <iomanip>
#include <sstream>

#include "Globals.h"
#include "Save/GetSaveFileContents.h"
#include "UI/UI.h"
#include "UI/AppShellScreen.h"
#include "UI/SaveSelectScreen.h"
#include "UI/BackupSelectionScreen.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Dialogs/KeyboardDialog.h"
#include "Utils/HelperUtilities.h"
#include "Utils/Logger.h"
#include "Utils/FileUtilities.h"
#include "Utils/MoveTransactionProduction.h"
#include "Utils/PokeBankPaths.h"
#include "Trainer/Trainer.h"
#include "Games/GameIdentity.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Integration/Gen4/Gen4AssignedSource.h"

using namespace Utils;
using namespace Trainer;

namespace UI {
    namespace {
        std::string profileIdentity(AccountUid uid) {
            std::ostringstream output;
            output << std::hex << std::setfill('0')
                   << std::setw(16) << static_cast<unsigned long long>(uid.uid[0])
                   << std::setw(16) << static_cast<unsigned long long>(uid.uid[1]);
            return output.str();
        }
    }

    UIManager::UIManager()
        : running(true),
          legacySourceBindings(PokeBank::Paths::legacySourceBindingsFile()) {
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        hidInitializeTouchScreen();  // enable the touchscreen alongside the gamepad
        Utils::setKeyboardPresenter([this](const Utils::KeyboardRequest& request) { return runKeyboard(request); });

        std::string pathError;
        if (!PokeBank::Paths::ensureConfigRoot(&pathError))
            logErrorToFile("Could not initialize PokeBank NX runtime root", pathError.c_str());
        if (!legacySourceBindings.load())
            logErrorToFile("Legacy source bindings contain malformed or unreadable rows");

        // Discover only explicitly bounded emulator battery-save roots. RetroArch keeps its
        // configured/conventional root; mGBA is additive only when /mGBA/config.ini explicitly
        // defines savegamePath; Tico is limited to its fixed GB/GBC/GBA battery-save directories.
        // Same-directory-as-ROM mGBA saves and arbitrary emulator trees are never crawled.
        // Generation-specific strict parsers remain separate and every source stays read-only.
        legacyFRLGSources = PokeVault::Legacy::discoverConfiguredLegacySaves();
        size_t ready = 0;
        size_t ambiguous = 0;
        size_t rejected = 0;
        size_t gen1Ready = 0;
        size_t gen2Ready = 0;
        size_t gen3Ready = 0;
        for (const auto& source : legacyFRLGSources.sources) {
            if (source.ready()) {
                ++ready;
                if (source.isGen1()) ++gen1Ready;
                if (source.isGen2()) ++gen2Ready;
                if (source.isGen3()) ++gen3Ready;
            } else if (source.status == PokeVault::Legacy::LegacySourceStatus::AmbiguousIdentity) {
                ++ambiguous;
            } else {
                ++rejected;
            }
        }
        char legacySummary[256];
        snprintf(legacySummary, sizeof(legacySummary),
                 "Legacy emulator sources: %zu files checked, %zu ready (Gen I %zu, Gen II %zu, Gen III %zu), %zu ambiguous, %zu rejected%s",
                 legacyFRLGSources.filesExamined, ready, gen1Ready, gen2Ready, gen3Ready,
                 ambiguous, rejected,
                 legacyFRLGSources.limitReached ? ", scan limit reached" : "");
        logInfoToFile(legacySummary);
    }

    UIManager::~UIManager() {
        Utils::setKeyboardPresenter(nullptr);
    }

    void UIManager::drawKeyboardBackdrop() {
        if (keyboardBackdrop)
            keyboardBackdrop->draw(fb);
        else
            fb.clear(Colors::Background);
    }

    Utils::KeyboardResult UIManager::runKeyboard(const Utils::KeyboardRequest& request) {
        Dialogs::KeyboardState keyboard;
        keyboard.open(request, fb.getWidth(), fb.getHeight());
        Dialogs::setActiveKeyboard(&keyboard);
        while (!keyboard.finished && appletMainLoop()) {
            padUpdate(&pad);
            touch.update();
            keyboard.update(pad, touch);
            if (keyboard.finished) break;
            drawKeyboardBackdrop();
            Dialogs::drawKeyboard(keyboard, fb);
            fb.flush();
        }
        Dialogs::setActiveKeyboard(nullptr);

        constexpr u64 answeringButtons = HidNpadButton_A | HidNpadButton_B | HidNpadButton_X | HidNpadButton_Y |
                                         HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR |
                                         HidNpadButton_Plus | HidNpadButton_Minus | HidNpadButton_Up |
                                         HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right;
        constexpr int releaseFrameLimit = 60;
        for (int releaseFrame = 0; releaseFrame < releaseFrameLimit && appletMainLoop(); ++releaseFrame) {
            drawKeyboardBackdrop();
            fb.flush();
            padUpdate(&pad);
            touch.update();
            const bool buttonStillMoving = ((padGetButtons(&pad) | padGetButtonsUp(&pad)) & answeringButtons) != 0;
            if (!buttonStillMoving && !touch.isDown() && !touch.justReleased()) break;
        }
        return keyboard.result();
    }

    void UIManager::run() {
        // The approved Games/product-home screen is now the app root. Secondary destinations
        // reuse the existing AppShell overlays, then return directly to the product home.
        fb.startFade();

        while (appletMainLoop() && running) {
            const auto destination = handleSaveSelection();
            if (!running) break;
            if (destination == SaveSelectScreen::MainMenuDestination::None) continue;

            AppShellScreen shell(appShellNavigationValid ? &appShellNavigation : nullptr);
            using Dest = SaveSelectScreen::MainMenuDestination;
            using Section = PokeBank::UIModel::AppShellSection;
            switch (destination) {
                case Dest::MasterVault: shell.openSection(Section::MasterVault); break;
                case Dest::Pokedex:     shell.openSection(Section::Pokedex); break;
                case Dest::Banks:       shell.openSection(Section::Banks); break;
                case Dest::Search:      shell.openSection(Section::Search); break;
                case Dest::More:        shell.openSection(Section::More); break;
                case Dest::Settings:    shell.openSection(Section::Settings); break;
                case Dest::None:        break;
            }

            fb.startFade();
            while (appletMainLoop() && running && shell.hasOverlay()) {
                padUpdate(&pad);
                touch.update();
                shell.update(pad, touch);
                if (!shell.hasOverlay()) break;
                shell.draw(fb);
                fb.drawFadeOverlay();
                fb.flush();
            }
            appShellNavigation = shell.navigationState();
            appShellNavigationValid = true;
            fb.startFade();
        }
    }
    // Product Home owns game/profile/save selection. Returning from a loaded backup/trainer
    // rebuilds it so newly-created saves stay visible; secondary destinations return here.
    SaveSelectScreen::MainMenuDestination UIManager::handleSaveSelection() {
        while (running) {
            SaveSelectScreen selectScreen(
                legacyFRLGSources, legacySourceBindings,
                productHomeNavigationValid ? &productHomeNavigation : nullptr);
            fb.startFade();
            bool rebuildPicker = false;

            while (appletMainLoop() && running && !selectScreen.shouldExit()) {
                padUpdate(&pad);
                touch.update();
                keyboardBackdrop = &selectScreen;
                selectScreen.update(pad, touch);

                // A screen that selected a destination or requested exit is already retired.
                // Never paint one more stale frame after update() changes its terminal state.
                if (!selectScreen.hasSelectedTitle() && !selectScreen.shouldExit()) {
                    selectScreen.draw(fb);
                    fb.drawFadeOverlay();
                    fb.flush();
                }

                if (selectScreen.hasSelectedTitle()) {
                    productHomeNavigation = selectScreen.navigationState();
                    productHomeNavigationValid = true;
                    if (selectScreen.getSelectedSourceKind() ==
                        SaveSelectScreen::SelectedSourceKind::RetroArchFRLG) {
                        std::string error;
                        if (!handleLegacyFRLGView(selectScreen.getSelectedUser(),
                                                  selectScreen.getSelectedLegacySourceIndex(),
                                                  selectScreen.getSelectedGameId(),
                                                  selectScreen.getOpenIntent(), error))
                            logErrorToFile("Legacy emulator source refused open", error.c_str());
                    } else if (selectScreen.getSelectedSourceKind() ==
                               SaveSelectScreen::SelectedSourceKind::Gen4AssignedFile) {
                        std::string error;
                        if (!handleGen4View(selectScreen.getSelectedUser(),
                                            selectScreen.getSelectedGameId(),
                                            selectScreen.getOpenIntent(), error))
                            logErrorToFile("Generation IV assigned source refused open", error.c_str());
                    } else {
                        if (selectScreen.getOpenIntent() == SaveSelectScreen::OpenIntent::Items) {
                            handleItemsQuickOpen(selectScreen.getSelectedUser(),
                                                 selectScreen.getSelectedTitleId(),
                                                 selectScreen.getSelectedTitleName());
                        } else if (selectScreen.getOpenIntent() == SaveSelectScreen::OpenIntent::Backups) {
                            handleBackupSelection(selectScreen.getSelectedUser(),
                                                  selectScreen.getSelectedTitleId(),
                                                  selectScreen.getSelectedTitleName(),
                                                  selectScreen.getOpenIntent());
                        } else {
                            handleDefaultQuickOpen(selectScreen.getSelectedUser(),
                                                   selectScreen.getSelectedTitleId(),
                                                   selectScreen.getSelectedTitleName());
                        }
                    }
                    // Return to the same Product Home instance. Rebuilding here re-ran account/save
                    // enumeration and source parsing before a frame could draw, which looked like a freeze.
                    selectScreen.resumeAfterEditor();
                    fb.startFade();
                    continue;
                }
                if (selectScreen.shouldExit()) break;
            }

            if (!running) return SaveSelectScreen::MainMenuDestination::None;
            if (selectScreen.hasRequestedAppExit()) {
                running = false;
                return SaveSelectScreen::MainMenuDestination::None;
            }
            if (selectScreen.shouldExit()) {
                productHomeNavigation = selectScreen.navigationState();
                productHomeNavigationValid = true;
                return selectScreen.getRequestedMainMenuDestination();
            }
            if (!rebuildPicker) return SaveSelectScreen::MainMenuDestination::None;
        }
        return SaveSelectScreen::MainMenuDestination::None;
    }

    void UIManager::handleDefaultQuickOpen(AccountUid userUid, u64 titleId,
                                           const std::string& titleName) {
        // Product Home Open is intentionally frictionless: create the same protected app-owned
        // backup/working copy first, then enter the player/game workspace. Backup history is only
        // shown when the user explicitly opens Current Game -> Backups.
        logInfoToFile("Direct game open: creating protected backup/working copy first",
                      titleName.c_str());
        const std::string backupPath =
            backupSaveData(userUid, titleId, titleName, g_autoBackupEnabled);
        if (backupPath.empty()) {
            logErrorToFile("Direct game open refused: backup creation failed",
                           titleName.c_str());
            return;
        }
        std::string error;
        if (!handleTrainerView(userUid, titleId, titleName, backupPath, false,
                               SaveSelectScreen::OpenIntent::Default, error)) {
            logErrorToFile("Direct game open failed after backup", error.c_str());
        }
    }

    void UIManager::handleItemsQuickOpen(AccountUid userUid, u64 titleId,
                                         const std::string& titleName) {
        logInfoToFile("Items quick-open: creating protected working backup first",
                      titleName.c_str());
        const std::string backupPath =
            backupSaveData(userUid, titleId, titleName, g_autoBackupEnabled);
        if (backupPath.empty()) {
            logErrorToFile("Items quick-open refused: backup creation failed",
                           titleName.c_str());
            return;
        }
        std::string error;
        if (!handleTrainerView(userUid, titleId, titleName, backupPath, false,
                               SaveSelectScreen::OpenIntent::Items, error)) {
            logErrorToFile("Items quick-open failed after backup", error.c_str());
        }
    }

    void UIManager::handleBackupSelection(AccountUid userUid, u64 titleId,
                                          const std::string& titleName,
                                          SaveSelectScreen::OpenIntent intent) {
        BackupSelectionScreen backupScreen(userUid, titleId, titleName);
        fb.startFade();

        while (appletMainLoop() && running && !backupScreen.shouldExit()) {
            padUpdate(&pad);
            touch.update();
            backupScreen.update(pad, touch);

            // Selection/exit retires this chooser immediately; do not flash it again.
            if (!backupScreen.shouldExit() && !backupScreen.hasSelectedBackup()) {
                backupScreen.draw(fb);
                fb.drawFadeOverlay();
                fb.flush();
            }

            if (backupScreen.hasSelectedBackup()) {
                if (backupScreen.shouldCreateNewBackup()) {
                    logInfoToFile("Creating new backup for", titleName.c_str());

                    // Auto-backup ON -> new timestamped history folder (kept indefinitely; the user
                    // decides when to delete backups, so we never prune). OFF -> reuse a single
                    // "Working" copy so backups don't pile up.
                    std::string backupPath = backupSaveData(userUid, titleId, titleName, g_autoBackupEnabled);
                    if (backupPath.empty()) {
                        logErrorToFile("Failed to back up save data");
                        backupScreen.reportFailure("Couldn't create the backup. Check SD card space and try again.");
                        continue;
                    }
                    std::string error;
                    if (!handleTrainerView(userUid, titleId, titleName, backupPath, false, intent, error)) {
                        backupScreen.reportFailure(error);
                        continue;
                    }
                } else {
                    // Use existing backup
                    logInfoToFile("Loading existing backup", backupScreen.getSelectedBackupPath().c_str());
                    std::string error;
                    if (!handleTrainerView(userUid, titleId, titleName,
                                           backupScreen.getSelectedBackupPath(), false, intent, error)) {
                        backupScreen.reportFailure(error);
                        continue;
                    }
                }
                return;
            }
            if (backupScreen.shouldExit()) break;
        }
    }

    bool UIManager::handleTrainerView(AccountUid userUid, u64 titleId, const std::string& titleName,
                                      const std::string& backupDir, bool loadedFromCart,
                                      SaveSelectScreen::OpenIntent intent, std::string& error) {
        logInfoToFile("Loading save from", backupDir.c_str());

        // A04b startup gate: reconcile any interrupted durable Move BEFORE parsing this workspace.
        // If recovery changes a Bank/workspace image, the Trainer below is therefore constructed
        // from the committed/recovered authoritative bytes rather than a stale pre-recovery model.
        const auto moveRecovery =
            PokeBank::Storage::MoveTx::Production::recoverPendingMoveTransactions();
        if (moveRecovery.mutationLocked) {
            logErrorToFile("Move transaction recovery requires attention",
                           moveRecovery.notice.c_str());
        } else if (moveRecovery.recovered > 0) {
            logInfoToFile("Recovered interrupted Pokemon Move transaction(s)",
                          std::to_string(moveRecovery.recovered).c_str());
        }

        if (!Save::validateTrainerSaveForOpen(backupDir.c_str(), titleId, error)) {
            logErrorToFile("Save validation refused open", error.c_str());
            return false;
        }

        TrainerVariant trainerVariant = readTrainerInfo(backupDir.c_str(), titleId);

        std::visit([&](auto& trainer) {
            TrainerViewScreen trainerScreen(
                trainer, titleName, backupDir, titleId, userUid,
                loadedFromCart ? PokeVault::Safety::SourceKind::InstalledGame
                               : PokeVault::Safety::SourceKind::BackupOrStaged);
            trainerScreen.setMoveRecoveryState(moveRecovery.mutationLocked, moveRecovery.notice);
            if (intent == SaveSelectScreen::OpenIntent::Items)
                trainerScreen.openItemsShortcut();
            fb.startFade();

            while (appletMainLoop() && !trainerScreen.shouldExit() && !trainerScreen.hasRequestedExit()) {
                padUpdate(&pad);
                touch.update();
                keyboardBackdrop = &trainerScreen;
                trainerScreen.update(pad, touch);
                if (trainerScreen.shouldExit() || trainerScreen.hasRequestedExit()) break;
                trainerScreen.draw(fb);
                fb.drawFadeOverlay();
                fb.flush();
            }

            if (trainerScreen.hasRequestedExit()) running = false;
        }, trainerVariant);
        return true;
    }

    bool UIManager::handleLegacyFRLGView(
        AccountUid userUid, size_t sourceIndex, const std::string& gameId,
        SaveSelectScreen::OpenIntent intent, std::string& error) {
        error.clear();
        if (sourceIndex >= legacyFRLGSources.sources.size()) {
            error = "Emulator source selection is stale";
            return false;
        }

        const auto& selected = legacyFRLGSources.sources[sourceIndex];
        if (!selected.ready() || selected.gameId != gameId) {
            error = "Emulator source is no longer a validated legacy save";
            return false;
        }
        const auto* identity = PokeVault::Games::findGame(selected.gameId);
        if (!identity) {
            error = "Emulator source has no stable game identity";
            return false;
        }

        std::unique_ptr<Trainer::Trainer> trainer;
        if (selected.isGen1()) {
            if (identity->platform != PokeVault::Games::Platform::GameBoy ||
                selected.gen1Save->metadata().sourceGameId != gameId) {
                error = "Emulator source is no longer a validated Generation I save";
                return false;
            }
            trainer = PokeVault::Legacy::RBYReadOnlyTrainer::create(*selected.gen1Save, error);
        } else if (selected.isGen2()) {
            if (identity->platform != PokeVault::Games::Platform::GameBoyColor ||
                selected.gen2Save->metadata().sourceGameId != gameId) {
                error = "Emulator source is no longer a validated Generation II save";
                return false;
            }
            auto gscTrainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*selected.gen2Save, error);
            if (gscTrainer && !gscTrainer->inventoryAvailable())
                logErrorToFile("GSC optional inventory validation failed; save remains open read-only",
                               selected.gameId.c_str());
            trainer = std::move(gscTrainer);
        } else if (selected.isGen3()) {
            if (identity->platform != PokeVault::Games::Platform::GameBoyAdvance ||
                selected.save->metadata().sourceGameId != gameId) {
                error = "Emulator source is no longer a validated Generation III save";
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
            if (error.empty()) error = "validated emulator source has no supported read-only trainer bridge";
            return false;
        }

        TrainerViewScreen trainerScreen(
            *trainer, "Pokemon " + std::string(identity->title), selected.path, 0, userUid,
            PokeVault::Safety::SourceKind::RetroArchLegacy, selected.gameId,
            selected.providerLabel);
        if (intent == SaveSelectScreen::OpenIntent::Items)
            trainerScreen.openItemsShortcut();
        fb.startFade();
        while (appletMainLoop() && !trainerScreen.shouldExit() &&
               !trainerScreen.hasRequestedExit()) {
            padUpdate(&pad);
            touch.update();
            trainerScreen.update(pad, touch);
            if (trainerScreen.shouldExit() || trainerScreen.hasRequestedExit()) break;
            trainerScreen.draw(fb);
            fb.drawFadeOverlay();
            fb.flush();
        }
        if (trainerScreen.hasRequestedExit()) running = false;
        return true;
    }
    bool UIManager::handleGen4View(
        AccountUid userUid, const std::string& gameId,
        SaveSelectScreen::OpenIntent intent, std::string& error) {
        error.clear();
        const auto* identity = PokeVault::Games::findGame(gameId);
        if (!identity || identity->platform != PokeVault::Games::Platform::NintendoDS ||
            identity->dataGeneration != 4 ||
            identity->support != PokeVault::Games::SourceSupport::ReadOnly) {
            error = "Generation IV game card is not a registered read-only Nintendo DS source";
            return false;
        }

        auto opened = PokeVault::Integration::Gen4::openAssignedSource(
            legacySourceBindings, profileIdentity(userUid), gameId);
        if (opened.status != PokeVault::Integration::Gen4::OpenStatus::Ready || !opened.save) {
            error = opened.diagnostic.empty()
                ? "Generation IV assigned source failed strict validation"
                : opened.diagnostic;
            return false;
        }

        auto trainer = PokeVault::Legacy::Gen4ReadOnlyTrainer::create(
            *opened.save, gameId, error);
        if (!trainer) {
            if (error.empty()) error = "Generation IV read-only presentation bridge failed";
            return false;
        }

        const std::string sourcePath = opened.source.binding.sourcePath;
        logInfoToFile("Opening assigned Generation IV source read-only", sourcePath.c_str());
        TrainerViewScreen trainerScreen(
            *trainer, "Pokemon " + std::string(identity->title), sourcePath, 0, userUid,
            PokeVault::Safety::SourceKind::ExternalLegacy, gameId,
            opened.source.binding.sourceType);
        if (intent == SaveSelectScreen::OpenIntent::Items)
            trainerScreen.openItemsShortcut();
        fb.startFade();
        while (appletMainLoop() && !trainerScreen.shouldExit() &&
               !trainerScreen.hasRequestedExit()) {
            padUpdate(&pad);
            touch.update();
            trainerScreen.update(pad, touch);
            if (trainerScreen.shouldExit() || trainerScreen.hasRequestedExit()) break;
            trainerScreen.draw(fb);
            fb.drawFadeOverlay();
            fb.flush();
        }
        if (trainerScreen.hasRequestedExit()) running = false;
        return true;
    }

}
