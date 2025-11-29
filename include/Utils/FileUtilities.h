#ifndef UTILS_FILE_UTILITIES_H
#define UTILS_FILE_UTILITIES_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <switch.h>

uint8_t* readAllBytes(const char* path, size_t* outSize);
bool copyDirectory(const char* srcPath, const char* destPath);
bool copyFile(const char* srcPath, const char* destPath);
bool backupSaveData(u64 titleId, std::string titleName);
bool restoreModifiedSave(u64 titleId, const char* modifiedSavePath, const char* backupDir, std::vector<std::string> saveFiles);
std::string getTimestamp();
std::vector<std::string> listBackupDirectories(const char* gameDirectory);
void limitBackups(const char* gameDirectory, int maxBackups);

#endif
