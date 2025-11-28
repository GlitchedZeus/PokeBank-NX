#ifndef UI_USER_SELECTION_SCREEN_H
#define UI_USER_SELECTION_SCREEN_H

#include <vector>
#include <string>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/PKSEFramebuffer.h"

// User Selection Screen
class UserSelectionScreen : public UIScreen {
public:
    UserSelectionScreen();
    void update(const PadState& pad) override;
    void draw(PKSEFramebuffer& fb) override;
    bool shouldExit() const override { return exitRequested; }

    bool hasSelectedUser() const { return userSelected; }
    AccountUid getSelectedUser() const { return selectedUserUid; }

private:
    struct UserInfo {
        AccountUid uid;
        std::string name;
        bool isValid;
    };

    std::vector<UserInfo> users;
    int selectedIndex;
    bool userSelected;
    bool exitRequested;
    AccountUid selectedUserUid;

    void loadUsers();
    void drawUserList(PKSEFramebuffer& fb);
};

#endif
