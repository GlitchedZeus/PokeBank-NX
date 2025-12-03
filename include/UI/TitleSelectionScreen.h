#ifndef UI_TITLE_SELECTION_SCREEN_H
#define UI_TITLE_SELECTION_SCREEN_H

#include <vector>
#include <string>

#include <switch.h>

#include "UI/UIScreen.h"
#include "UI/PKSEFramebuffer.h"

namespace UI {
    class TitleSelectionScreen : public UIScreen {
    public:
        TitleSelectionScreen(AccountUid userUid);
        void update(const PadState& pad) override;
        void draw(PKSEFramebuffer& fb) override;
        bool shouldExit() const override { return goBack; }

        bool hasSelectedTitle() const { return titleSelected; }
        u64 getSelectedTitleId() const { return selectedTitleId; }
        const std::string& getSelectedTitleName() const { return selectedTitleName; }

    private:
        struct TitleInfo {
            u64 titleId;
            std::string name;
        };

        AccountUid userUid;
        std::vector<TitleInfo> titles;
        int selectedIndex;
        bool titleSelected;
        bool goBack;
        u64 selectedTitleId;
        std::string selectedTitleName;

        void loadTitles();
        void drawTitleList(PKSEFramebuffer& fb);
    };
}

#endif
