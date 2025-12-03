#ifndef UI_BACKUP_SELECTION_SCREEN_H
#define UI_BACKUP_SELECTION_SCREEN_H

#include <vector>
#include <string>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/PKSEFramebuffer.h"

// Backup Selection Screen
class BackupSelectionScreen : public UIScreen {
public:
    BackupSelectionScreen(u64 titleId, const std::string& titleName);
    void update(const PadState& pad) override;
    void draw(PKSEFramebuffer& fb) override;
    bool shouldExit() const override { return goBack; }

    bool hasSelectedBackup() const { return backupSelected; }
    bool shouldCreateNewBackup() const { return createNewBackup; }
    const std::string& getSelectedBackupPath() const { return selectedBackupPath; }

private:
    struct BackupInfo {
        std::string timestamp;
        std::string displayName;
    };

    u64 titleId;
    std::string titleName;
    std::string gameDirectory;
    std::vector<BackupInfo> backups;
    int selectedIndex;
    bool backupSelected;
    bool createNewBackup;  // True if user wants to load from title directly
    bool goBack;
    std::string selectedBackupPath;

    void loadBackups();
    void drawBackupList(PKSEFramebuffer& fb);
    std::string formatTimestamp(const std::string& timestamp);
};

#endif
