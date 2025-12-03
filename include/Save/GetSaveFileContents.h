#ifndef GET_SAVE_FILE_CONTENTS_H
#define GET_SAVE_FILE_CONTENTS_H

#include <string>
#include <variant>

#include <switch.h>

#include "Enums/GameVersion.h"
#include "Trainer/Trainer.h"
#include "Trainer/Trainer7LGPE.h"
#include "Trainer/Trainer8SWSH.h"
#include "Trainer/Trainer9LZA.h"

using namespace Enums;
using namespace Trainer;

namespace Save {
    // Type alias for trainer variants (supports different generation trainers)
    using TrainerVariant = std::variant<Trainer7LGPE, Trainer8SWSH, Trainer9LZA>;

    /**
     * Save File Reading/Writing Functions
     *
     * Different Pokemon games use different save file formats:
     * - Let's Go (GG): savedata.bin (1MB, simpler encryption)
     * - Sword/Shield (SWSH): main (1.6MB, block-based encryption)
     * - Other games: various formats (not yet implemented)
     *
     * These functions provide game-specific save file handling.
     */

    // ========================================
    // Generic Functions (Auto-detect game)
    // ========================================

    /**
     * Reads trainer info from a save file, auto-detecting the game version.
     *
     * @param backupDir The backup directory containing the save file
     * @param titleId The game's title ID (used to detect game version)
     * @return TrainerVariant containing either Trainer7 or Trainer8
     */
    TrainerVariant readTrainerInfo(const char* backupDir, u64 titleId);

    /**
     * Saves trainer info to a save file, auto-detecting the game version.
     *
     * @param trainer The trainer data to save (base Trainer reference)
     * @param backupDir The backup directory to save to
     * @param titleId The game's title ID (used to detect game version)
     * @return true if save was successful, false otherwise
     */
    bool saveTrainerInfo(Trainer::Trainer& trainer, const char* backupDir, u64 titleId);

    // ========================================
    // Game-Specific Functions
    // ========================================

    /**
     * Reads trainer info from a Pokemon Let's Go Pikachu/Eevee save file.
     *
     * Let's Go save format:
     * - File: "savedata.bin" (1,048,576 bytes = 1MB)
     * - Encryption: Simpler than SwSh, different block structure
     * - No external hash (checksum is internal)
     *
     * @param backupDir The backup directory containing the "savedata.bin" file
     * @return Trainer7 object with loaded data
     */
    Trainer7LGPE readTrainerInfoLetsGo(const char* backupDir);

    /**
     * Saves trainer info to a Pokemon Let's Go Pikachu/Eevee save file.
     *
     * @param trainer The trainer data to save (Trainer7)
     * @param backupDir The backup directory to save to
     * @param titleId The game's title ID
     * @return true if save was successful, false otherwise
     */
    bool saveTrainerInfoLetsGo(Trainer7LGPE& trainer, const char* backupDir, u64 titleId);

    /**
     * Reads trainer info from a Pokemon Sword/Shield save file.
     *
     * Sword/Shield save format:
     * - File: "main" (1,603,146 bytes typically)
     * - Encryption: Block-based with SwSh encryption
     * - Hash: Last 32 bytes
     *
     * @param backupDir The backup directory containing the "main" file
     * @return Trainer8 object with loaded data
     */
    Trainer8SWSH readTrainerInfoSwSh(const char* backupDir);

    /**
     * Saves trainer info to a Pokemon Sword/Shield save file.
     *
     * @param trainer The trainer data to save (Trainer8)
     * @param backupDir The backup directory to save to
     * @param titleId The game's title ID
     * @return true if save was successful, false otherwise
     */
    bool saveTrainerInfoSwSh(Trainer8SWSH& trainer, const char* backupDir, u64 titleId);

    /**
     * Reads trainer info from a Pokemon Sword/Shield save file.
     *
     * Sword/Shield save format:
     * - File: "main" (1,603,146 bytes typically)
     * - Encryption: Block-based with SwSh encryption
     * - Hash: Last 32 bytes
     *
     * @param backupDir The backup directory containing the "main" file
     * @return Trainer8 object with loaded data
     */
    Trainer9LZA readTrainerInfoLZA(const char* backupDir);

    /**
     * Saves trainer info to a Pokemon Sword/Shield save file.
     *
     * @param trainer The trainer data to save (Trainer8)
     * @param backupDir The backup directory to save to
     * @param titleId The game's title ID
     * @return true if save was successful, false otherwise
     */
    bool saveTrainerInfoLZA(Trainer9LZA& trainer, const char* backupDir, u64 titleId);
}

#endif