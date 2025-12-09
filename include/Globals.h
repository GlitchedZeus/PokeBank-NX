#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>

inline constexpr std::string BASE_SAVE_DIRECTORY = "sdmc:/PKSE";

inline constexpr uint32_t SIZE_HASH_IN_BYTES = 32;

/// Save control flag - when false, only saves to ModifiedSave directory
/// Set to true to restore the modified save back to the game's save device
inline constexpr bool SAVE_TO_TITLE = true; // TODO: We need to make this an in-app option for debugging/development purposes.

/// Determines how many backup saves are allowed
inline constexpr uint8_t BACKUP_SAVE_LIMIT = 5; // TODO: We can make this value modifyable by the user at a later time

/// Number of days to retain debug log files
inline constexpr uint8_t LOG_RETENTION_DAYS = 30;

#endif