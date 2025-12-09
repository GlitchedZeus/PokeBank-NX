#include <cstdio>
#include <vector>
#include <locale>
#include <codecvt>

#include <errno.h>
#include <bits/basic_string.h>
#include <sys/stat.h>

#include "Globals.h"
#include "Save/Block.h"
#include "Save/GetSaveFileContents.h"
#include "Utils/FileUtilities.h"
#include "Utils/Logger.h"
#include "Utils/HelperUtilities.h"
#include "Utils/StringHelpers.h"
#include "Trainer/Trainer.h"
#include "Trainer/Trainer7LGPE.h"
#include "Trainer/Trainer8SWSH.h"
#include "Trainer/Trainer9LZA.h"
#include "Encryption/Encryption.h"
#include "Enums/GameVersion.h"

using namespace Utils;
using namespace Trainer;
using namespace Encryption;
using namespace Enums;

namespace Save {
    // ========================================
    // Generic Functions (Auto-detect game)
    // ========================================

    TrainerVariant readTrainerInfo(const char* backupDir, u64 titleId) {
        /**
         * Auto-detects the game version and calls the appropriate reading function.
         *
         * This function determines which game is being loaded based on the title ID,
         * then calls the game-specific reading function with the correct save file name
         * and decryption method.
         *
         * Returns a variant type containing either Trainer7 or Trainer8.
         */

        GameVersion version = getGameVersion(titleId);
        GameVersion group = getGameGroup(version);

        char buffer[512];
        snprintf(buffer, sizeof(buffer), "Detected game: %s (Group: %s)",
            getGameVersionName(version).c_str(), getGameVersionName(group).c_str());
        Utils::logInfoToFile(buffer);

        switch (group) {
            case GameVersion::GG:  // Let's Go Pikachu/Eevee
                return readTrainerInfoLetsGo(backupDir);

            case GameVersion::SWSH:  // Sword/Shield
                return readTrainerInfoSwSh(backupDir);

            case GameVersion::ZA:
                return readTrainerInfoLZA(backupDir);

            default:
                logErrorToFile("Unsupported game version");
                // Return empty Trainer7 as fallback (better error handling needed)
                return Trainer::Trainer7LGPE(std::vector<Block>());
        }
    }

    bool saveTrainerInfo(Trainer::Trainer& trainer, const char* backupDir, u64 titleId, AccountUid userUid) {
        /**
         * Auto-detects the game version and calls the appropriate saving function.
         * Uses virtual getGameGroup() method to determine concrete type without RTTI.
         */

        GameVersion version = getGameVersion(titleId);
        GameVersion group = getGameGroup(version);

        char buffer[512];
        snprintf(buffer, sizeof(buffer), "Saving for game: %s (Group: %s)",
            getGameVersionName(version).c_str(), getGameVersionName(group).c_str());
        Utils::logInfoToFile(buffer);

        // Use virtual getGameGroup() to determine the concrete type (no RTTI required)
        GameVersion trainerGroup = trainer.getGameGroup();

        if (trainerGroup == GameVersion::GG) {
            // Let's Go - cast is safe because we checked the type via virtual method
            return saveTrainerInfoLetsGo(static_cast<Trainer::Trainer7LGPE&>(trainer), backupDir, titleId, userUid);
        } else if (trainerGroup == GameVersion::SWSH) {
            // Sword/Shield - cast is safe because we checked the type via virtual method
            return saveTrainerInfoSwSh(static_cast<Trainer::Trainer8SWSH&>(trainer), backupDir, titleId, userUid);
        } else if (trainerGroup == GameVersion::ZA) {
            // Legends: Z-A - cast is safe because we checked the type via virtual method
            return saveTrainerInfoLZA(static_cast<Trainer9LZA&>(trainer), backupDir, titleId, userUid);
        } else {
            logErrorToFile("Unsupported trainer type");
            return false;
        }
    }

    // ========================================
    // Game-Specific Functions - Let's Go
    // ========================================

    Trainer7LGPE readTrainerInfoLetsGo(const char* backupDir) {
        /**
         * Reads Pokemon Let's Go Pikachu/Eevee save file.
         *
         * Let's Go save format:
         * - File: savedata.bin (1,048,576 bytes = 1MB exactly)
         * - Encryption: Simpler than SwSh, different block structure
         * - Checksum: Internal (not external like SwSh)
         *
         * TODO: Implement Let's Go save file reading
         * For now, this is a placeholder that returns an empty trainer.
         */

        char savePath[512];
        snprintf(savePath, sizeof(savePath), "%s/savedata.bin", backupDir);

        char buffer[LOG_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Reading Let's Go save from: %s", savePath);
        Utils::logInfoToFile(buffer);

        size_t fileSize = 0;
        uint8_t* file = readAllBytes(savePath, &fileSize);

        char fileSizeBuffer[512];
        snprintf(fileSizeBuffer, sizeof(fileSizeBuffer), "0x%016llX", static_cast<unsigned long long>(fileSize));
        logInfoToFile("Filesize", fileSizeBuffer);

        // Expected size: 1,048,576 bytes (1MB)
        if (fileSize != 1048576) {
            logErrorToFile("Warning: Let's Go save file size is not 1MB");
        }

        // TODO: Implement Let's Go decryption
        // For now, log a warning and return empty trainer
        logErrorToFile("Let's Go save file reading is not yet fully implemented");

        delete[] file;

        // Return empty trainer for now
        return Trainer7LGPE(std::vector<Block>());
    }

    bool saveTrainerInfoLetsGo(Trainer7LGPE& trainer, const char* backupDir, u64 titleId, AccountUid userUid) {
        /**
         * Saves Pokemon Let's Go Pikachu/Eevee save file.
         *
         * TODO: Implement Let's Go save file writing
         * For now, this is a placeholder that logs an error.
         */

        logErrorToFile("Let's Go save file writing is not yet fully implemented");

        return false;
    }

    // ========================================
    // Game-Specific Functions - Sword/Shield
    // ========================================

    Trainer8SWSH readTrainerInfoSwSh(const char* backupDir) {
        char mainPath[512];
        snprintf(mainPath, sizeof(mainPath), "%s/main", backupDir);

        char buffer[LOG_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Reading trainer info from: %s", mainPath);
        logInfoToFile(buffer);

        size_t fileSize = 0;
        uint8_t* file = readAllBytes(mainPath, &fileSize);

        char fileSizeBuffer[512];
        snprintf(fileSizeBuffer, sizeof(fileSizeBuffer), "0x%016llX", static_cast<unsigned long long>(fileSize));
        // Filesize should be 1,603,146 bytes (size)
        logInfoToFile("Filesize", fileSizeBuffer);

        // Extract hash (last 32 bytes) - not currently used but may be needed for validation
        // const uint8_t* hash = (fileSize >= SIZE_HASH_IN_BYTES) ? (file + fileSize - SIZE_HASH_IN_BYTES) : nullptr;

        std::vector<Block> blocks = decrypt(file, fileSize);
        Trainer8SWSH trainer(blocks);

        delete[] file;
        return trainer;
    }

    bool saveTrainerInfoSwSh(Trainer8SWSH& trainer, const char* backupDir, u64 titleId, AccountUid userUid) {
        // Create ModifiedSave directory
        char modifiedSaveDir[512];
        snprintf(modifiedSaveDir, sizeof(modifiedSaveDir), "%s/ModifiedSave", backupDir);

        // Create directory if it doesn't exist
        if (mkdir(modifiedSaveDir, 0777) != 0 && errno != EEXIST) {
            logErrorToFile("Failed to create ModifiedSave directory", modifiedSaveDir);
            return false;
        }

        // Update item block with modified data
        trainer.updateItemBlock();

        // Update party block with modified Pokemon data
        trainer.updatePartyBlock();

        // Update box block with modified Pokemon data
        trainer.updateBoxBlock();

        // Encrypt the modified blocks (hash is calculated automatically)
        std::vector<uint8_t> encryptedData = encrypt(trainer.getBlocks());

        // Write to file
        char savePath[1024];
        snprintf(savePath, sizeof(savePath), "%s/main", modifiedSaveDir);

        FILE* outFile = fopen(savePath, "wb");
        if (!outFile) {
            logErrorToFile("Failed to open file for writing", savePath);
            return false;
        }

        size_t written = fwrite(encryptedData.data(), 1, encryptedData.size(), outFile);
        fclose(outFile);

        if (written != encryptedData.size()) {
            logErrorToFile("Failed to write complete save file", savePath);
            return false;
        }

        std::string successMsg = std::string("Successfully saved modified save to: ") + savePath;
        logInfoToFile(successMsg.c_str());

        // Only restore to title if SAVE_TO_TITLE is enabled
        if (SAVE_TO_TITLE) {
            // Restore the modified save back to the game's save device
            logInfoToFile("Restoring modified save to game save device...");
            std::vector<std::string> saveFiles = {"main", "backup", "poke_trade"};
            if (!restoreModifiedSave(userUid, titleId, modifiedSaveDir, backupDir, saveFiles)) {
                logErrorToFile("Failed to restore modified save to game");
                return false;
            }
        } else {
            logInfoToFile("SAVE_TO_TITLE is disabled - save written to ModifiedSave directory only");
        }

        return true;
    }

    // ========================================
    // Game-Specific Functions - Legends: Z-A
    // ========================================

    Trainer9LZA readTrainerInfoLZA(const char* backupDir) {
        char mainPath[512];
        snprintf(mainPath, sizeof(mainPath), "%s/main", backupDir);

        char buffer[LOG_BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Reading trainer info from: %s", mainPath);
        logInfoToFile(buffer);

        size_t fileSize = 0;
        uint8_t* file = readAllBytes(mainPath, &fileSize);

        char fileSizeBuffer[512];
        snprintf(fileSizeBuffer, sizeof(fileSizeBuffer), "0x%016llX", static_cast<unsigned long long>(fileSize));
        // Filesize should be 3,093,124 bytes (size)
        logInfoToFile("Filesize", fileSizeBuffer);

        // Extract hash (last 32 bytes) - not currently used but may be needed for validation
        // const uint8_t* hash = (fileSize >= SIZE_HASH_IN_BYTES) ? (file + fileSize - SIZE_HASH_IN_BYTES) : nullptr;

        std::vector<Block> blocks = decrypt(file, fileSize);
        Trainer9LZA trainer(blocks);

        delete[] file;
        return trainer;
    }

    bool saveTrainerInfoLZA(Trainer9LZA& trainer, const char* backupDir, u64 titleId, AccountUid userUid) {
        // Create ModifiedSave directory
        char modifiedSaveDir[512];
        snprintf(modifiedSaveDir, sizeof(modifiedSaveDir), "%s/ModifiedSave", backupDir);

        // Create directory if it doesn't exist
        if (mkdir(modifiedSaveDir, 0777) != 0 && errno != EEXIST) {
            logErrorToFile("Failed to create ModifiedSave directory", modifiedSaveDir);
            return false;
        }

        // Update item block with modified data
        trainer.updateItemBlock();

        // Update party block with modified Pokemon data
        trainer.updatePartyBlock();

        // Update box block with modified Pokemon data
        trainer.updateBoxBlock();

        // Encrypt the modified blocks (hash is calculated automatically)
        std::vector<uint8_t> encryptedData = encrypt(trainer.getBlocks());

        // Write to file
        char savePath[1024];
        snprintf(savePath, sizeof(savePath), "%s/main", modifiedSaveDir);

        FILE* outFile = fopen(savePath, "wb");
        if (!outFile) {
            logErrorToFile("Failed to open file for writing", savePath);
            return false;
        }

        size_t written = fwrite(encryptedData.data(), 1, encryptedData.size(), outFile);
        fclose(outFile);

        if (written != encryptedData.size()) {
            logErrorToFile("Failed to write complete save file", savePath);
            return false;
        }

        std::string successMsg = std::string("Successfully saved modified save to: ") + savePath;
        logInfoToFile(successMsg.c_str());

        // Only restore to title if SAVE_TO_TITLE is enabled
        if (SAVE_TO_TITLE) {
            // Restore the modified save back to the game's save device
            logInfoToFile("Restoring modified save to game save device...");
            std::vector<std::string> saveFiles = {"main"};
            if (!restoreModifiedSave(userUid, titleId, modifiedSaveDir, backupDir, saveFiles)) {
                logErrorToFile("Failed to restore modified save to game");
                return false;
            }
        } else {
            logInfoToFile("SAVE_TO_TITLE is disabled - save written to ModifiedSave directory only");
        }

        return true;
    }
}