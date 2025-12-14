#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>

#include <switch.h>
#include <sys/dirent.h>
#include <sys/unistd.h>

#include "Globals.h"
#include "Utils/Logger.h"

namespace Utils {
    uint8_t* readAllBytes(const char* path, size_t* outSize) {
        FILE* file = fopen(path, "rb");
        if (!file) {
            perror("Failed to open file");
            return NULL;
        }

        // Get the file size
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        rewind(file);

        // Allocate memory to hold the file contents
        uint8_t* buffer = (unsigned char*)malloc(fileSize);
        if (!buffer) {
            perror("Failed to allocate memory");
            fclose(file);
            return NULL;
        }

        // Read the file contents into the buffer
        size_t bytesRead = fread(buffer, 1, fileSize, file);
        if (bytesRead != fileSize) {
            perror("Failed to read the entire file");
            free(buffer);
            fclose(file);
            return NULL;
        }

        fclose(file);

        // Return the buffer and the size of the file
        if (outSize)
            *outSize = fileSize;
        return buffer;
    }

    bool copyDirectoryRecursive(const char* srcPath, const char* destPath) {
        DIR* dir = opendir(srcPath);
        if (!dir) {
            logErrorToFile("Failed to open source directory", srcPath);
            logErrorToFile("opendir error", strerror(errno));
            return false;
        }

        bool overallSuccess = true;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

            char srcFilePath[512];
            char destFilePath[512];
            snprintf(srcFilePath, sizeof(srcFilePath), "%s/%s", srcPath, entry->d_name);
            snprintf(destFilePath, sizeof(destFilePath), "%s/%s", destPath, entry->d_name);

            if (entry->d_type == DT_DIR) {
                // Recursively create and copy subdirectory
                if (mkdir(destFilePath, 0777) != 0 && errno != EEXIST) {
                    logErrorToFile("Failed to create subdirectory", destFilePath);
                    overallSuccess = false;
                } else {
                    if (!copyDirectoryRecursive(srcFilePath, destFilePath)) {
                        overallSuccess = false;
                    }
                }
            } else if (entry->d_type == DT_REG) {
                size_t size = 0;
                unsigned char* data = readAllBytes(srcFilePath, &size);
                if (!data) {
                    logErrorToFile("Failed to read file", srcFilePath);
                    overallSuccess = false;
                    continue;
                }

                FILE* out = fopen(destFilePath, "wb");
                if (!out) {
                    logErrorToFile("Failed to open for writing", destFilePath);
                    logErrorToFile("fopen error", strerror(errno));
                    free(data);
                    overallSuccess = false;
                    continue;
                }

                if (fwrite(data, 1, size, out) != size) {
                    logErrorToFile("Failed to write complete file", destFilePath);
                    overallSuccess = false;
                } else {
                    logInfoToFile("Successfully copied file", entry->d_name);
                    logInfoToFile("File size (bytes)", std::to_string(size).c_str());
                }
                fclose(out);
                free(data);
            }
        }
        closedir(dir);
        return overallSuccess;
    }

    bool copyDirectory(const char* srcPath, const char* destPath) {
        // Create destination directory if needed
        if (mkdir(destPath, 0777) != 0 && errno != EEXIST) {
            logErrorToFile("Failed to create destination directory", destPath);
            logErrorToFile("mkdir error", strerror(errno));
            return false;
        }
        logInfoToFile("Created/copied to destination directory", destPath);
        return copyDirectoryRecursive(srcPath, destPath);
    }

    bool copyFile(const char* srcPath, const char* destPath) {
        size_t size = 0;
        unsigned char* data = readAllBytes(srcPath, &size);
        if (!data) {
            logErrorToFile("Failed to read file", srcPath);
            return false;
        }

        FILE* out = fopen(destPath, "wb");
        if (!out) {
            logErrorToFile("Failed to open for writing", destPath);
            logErrorToFile("fopen error", strerror(errno));
            free(data);
            return false;
        }

        bool success = true;
        if (fwrite(data, 1, size, out) != size) {
            logErrorToFile("Failed to write complete file", destPath);
            success = false;
        } else {
            logInfoToFile("Successfully copied file to", destPath);
        }

        fclose(out);
        free(data);
        return success;
    }

    bool deleteDirectoryRecursive(const char* path) {
        DIR* dir = opendir(path);
        if (!dir) {
            logErrorToFile("Failed to open directory for deletion", path);
            return false;
        }

        bool success = true;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char fullPath[1024];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                // Recursively delete subdirectory
                if (!deleteDirectoryRecursive(fullPath)) {
                    success = false;
                }
            } else {
                // Delete file
                if (remove(fullPath) != 0) {
                    logErrorToFile("Failed to delete file", fullPath);
                    success = false;
                }
            }
        }
        closedir(dir);

        // Remove the directory itself
        if (rmdir(path) != 0) {
            logErrorToFile("Failed to remove directory", path);
            return false;
        }

        return success;
    }

    std::string getTimestamp() {
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);

        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);
        return std::string(buffer);
    }

    bool backupSaveData(AccountUid userUid, u64 titleId, std::string titleName) {
        char titleBuf[32];
        snprintf(titleBuf, sizeof(titleBuf), "0x%016llX", static_cast<unsigned long long>(titleId));
        logInfoToFile("Pokemon titleId: ", titleBuf);
        logInfoToFile("Pokemon Title name: ", titleName.c_str());

        // Create base game directory: PKSE/{titleName}/
        char gameDirectory[512];
        snprintf(gameDirectory, sizeof(gameDirectory), "%s/%s", BASE_SAVE_DIRECTORY.c_str(), titleName.c_str());

        // Create timestamped backup directory: PKSE/{titleName}/{timestamp}/
        std::string timestamp = getTimestamp();
        char timestampedBackupDirectory[1024];
        snprintf(timestampedBackupDirectory, sizeof(timestampedBackupDirectory), "%s/%s", gameDirectory, timestamp.c_str());

        logInfoToFile("Backup directory", timestampedBackupDirectory);
        logInfoToFile("Backing up save for title", titleName.c_str());

        if (mkdir(BASE_SAVE_DIRECTORY.c_str(), 0777) != 0 && errno != EEXIST) {
            logErrorToFile("Failed to create base directory", BASE_SAVE_DIRECTORY.c_str());
            logErrorToFile("mkdir error", strerror(errno));
            return false;
        }
        if (mkdir(gameDirectory, 0777) != 0 && errno != EEXIST) {
            logErrorToFile("Failed to create game directory", gameDirectory);
            logErrorToFile("mkdir error", strerror(errno));
            return false;
        }
        if (mkdir(timestampedBackupDirectory, 0777) != 0 && errno != EEXIST) {
            logErrorToFile("Failed to create timestamped backup directory", timestampedBackupDirectory);
            logErrorToFile("mkdir error", strerror(errno));
            return false;
        }

        char buffer[LOG_BUFFER_SIZE];

        Result result = fsdevMountSaveData("save", titleId, userUid);

        if (R_FAILED(result)) {
            snprintf(buffer, sizeof(buffer), "fsdevMountSaveData failed for titleId 0x%016lX: 0x%x", titleId, result);
            logErrorToFile(buffer);
            return false;
        }

        logInfoToFile("Successfully mounted save:/");

        bool copySuccess = copyDirectory("save:/", timestampedBackupDirectory);

        fsdevUnmountDevice("save");

        if (copySuccess) {
            logInfoToFile("Backup completed successfully!");
            return true;
        } else {
            logErrorToFile("Backup failed during file copying.");
            return false;
        }
    }

    bool restoreModifiedSave(AccountUid userUid, u64 titleId, const char* modifiedSavePath, const char* backupDir, std::vector<std::string> saveFiles) {
        char buffer[LOG_BUFFER_SIZE];
        logInfoToFile("Restoring modified save to game", modifiedSavePath);
        
        Result result = fsdevMountSaveData("save", titleId, userUid);

        if (R_FAILED(result)) {
            snprintf(buffer, sizeof(buffer), "fsdevMountSaveData failed for titleId 0x%016lX: 0x%x", titleId, result);
            logErrorToFile(buffer);
            return false;
        }

        logInfoToFile("Successfully mounted save:/ for restore");

        // Copy only the save files (not subdirectories like ModifiedSave)
        // Pokemon Sword/Shield has: main, backup, poke_trade
        logInfoToFile("Copying original save files to save:/", backupDir);

        // List of files to copy from the backup
        // const char* saveFiles[] = {"main", "backup", "poke_trade"};
        bool copyAllSuccess = true;

        for (size_t i = 0; i < saveFiles.size(); i++) {
            char srcPath[512];
            char destPath[512];
            snprintf(srcPath, sizeof(srcPath), "%s/%s", backupDir, saveFiles[i].c_str());
            snprintf(destPath, sizeof(destPath), "save:/%s", saveFiles[i].c_str());

            if (!copyFile(srcPath, destPath)) {
                snprintf(buffer, sizeof(buffer), "Failed to copy %s", saveFiles[i].c_str());
                logErrorToFile(buffer);
                copyAllSuccess = false;
                break;
            }
        }

        if (!copyAllSuccess) {
            logErrorToFile("Failed to copy original backup files");
            fsdevUnmountDevice("save");
            return false;
        }

        // Then, overwrite the 'main' file with the modified version
        logInfoToFile("Overwriting main file with modified version");

        char modifiedMainPath[512];
        snprintf(modifiedMainPath, sizeof(modifiedMainPath), "%s/main", modifiedSavePath);

        bool copyModifiedSuccess = copyFile(modifiedMainPath, "save:/main");

        if (!copyModifiedSuccess) {
            logErrorToFile("Failed to restore modified main file.");
            fsdevUnmountDevice("save");
            return false;
        }

        // CRITICAL: Commit changes to the save device before unmounting
        // Without this, changes remain in memory buffers and are never written to disk
        logInfoToFile("Committing changes to save device...");

        Result commitResult = fsdevCommitDevice("save");
        if (R_FAILED(commitResult)) {
            snprintf(buffer, sizeof(buffer), "fsdevCommitDevice failed: 0x%x", commitResult);
            logErrorToFile(buffer);
            fsdevUnmountDevice("save");
            return false;
        }

        logInfoToFile("Successfully committed changes to save device");

        fsdevUnmountDevice("save");

        logInfoToFile("Modified save restored successfully!");
        return true;
    }

    std::vector<std::string> listBackupDirectories(const char* gameDirectory) {
        std::vector<std::string> backupDirs;

        // TODO: Directory listing could be empty, no save backups. We don't need to log an error in that case.
        DIR* dir = opendir(gameDirectory);
        if (!dir) {
            logErrorToFile("Failed to open game directory for backup listing", gameDirectory);
            return backupDirs;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Check if it's a directory and matches timestamp format (YYYYMMDD_HHMMSS)
            if (entry->d_type == DT_DIR) {
                std::string dirName = entry->d_name;
                // Simple validation: timestamp should be 15 chars (YYYYMMDD_HHMMSS)
                if (dirName.length() == 15 && dirName[8] == '_') {
                    backupDirs.push_back(dirName);
                }
            }
        }
        closedir(dir);

        // Sort in descending order (newest first)
        std::sort(backupDirs.begin(), backupDirs.end(), std::greater<std::string>());

        return backupDirs;
    }

    void limitBackups(const char* gameDirectory, int maxBackups) {
        std::vector<std::string> backupDirs = listBackupDirectories(gameDirectory);

        // If we have more backups than the limit, delete the oldest ones
        if (backupDirs.size() > static_cast<size_t>(maxBackups)) {
            logInfoToFile("Limiting backups to maximum of", std::to_string(maxBackups).c_str());

            for (size_t i = maxBackups; i < backupDirs.size(); i++) {
                char backupPath[512];
                snprintf(backupPath, sizeof(backupPath), "%s/%s", gameDirectory, backupDirs[i].c_str());

                logInfoToFile("Deleting old backup", backupPath);

                // Delete the directory recursively
                // We'll use a simple approach: delete all files first, then the directory
                DIR* dir = opendir(backupPath);
                if (dir) {
                    struct dirent* entry;
                    while ((entry = readdir(dir)) != NULL) {
                        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                            continue;
                        }

                        char filePath[1024];
                        snprintf(filePath, sizeof(filePath), "%s/%s", backupPath, entry->d_name);

                        if (entry->d_type == DT_REG) {
                            remove(filePath);
                        } else if (entry->d_type == DT_DIR) {
                            // For subdirectories, we need to delete recursively
                            // For now, we'll just try to remove it (works if empty)
                            rmdir(filePath);
                        }
                    }
                    closedir(dir);

                    // Remove the backup directory itself
                    if (rmdir(backupPath) == 0) {
                        logInfoToFile("Successfully deleted backup", backupDirs[i].c_str());
                    } else {
                        logErrorToFile("Failed to delete backup directory", backupPath);
                    }
                }
            }
        }
    }
}