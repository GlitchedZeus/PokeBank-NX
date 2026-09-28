#ifndef UI_APP_SHELL_SCREEN_H
#define UI_APP_SHELL_SCREEN_H

#include <array>
#include <string>

#include <switch.h>

#include "UI/AppShellModel.h"
#include "UI/NavigationRepeat.h"
#include "UI/UIScreen.h"

namespace UI {

    class AppShellScreen : public UIScreen {
    public:
        enum class Action {
            None,
            Games,
        };

        void update(const PadState& pad, const TouchInput& touch) override;
        void draw(PKSEFramebuffer& fb) override;
        bool shouldExit() const override { return exitRequested; }

        Action consumeAction();

    private:
        enum class Overlay {
            None,
            Settings,
            Diagnostics,
            SectionInfo,
            Help,
        };

        struct HitRect {
            int x = 0;
            int y = 0;
            int w = 0;
            int h = 0;
            int index = -1;
        };

        PokeBank::UIModel::ControllerNavigation controllerNavigation;
        int selectedIndex = 0;
        int settingsIndex = 0;
        bool exitRequested = false;
        Action pendingAction = Action::None;
        Overlay overlay = Overlay::None;
        PokeBank::UIModel::AppShellSection infoSection =
            PokeBank::UIModel::AppShellSection::Games;
        std::array<HitRect, 8> cardRects{};
        std::array<HitRect, 6> settingsRects{};
        std::string statusMessage;
        int statusFrames = 0;

        void activateSelected();
        void activateSetting();
        void drawHome(PKSEFramebuffer& fb);
        void drawSettings(PKSEFramebuffer& fb);
        void drawDiagnostics(PKSEFramebuffer& fb);
        void drawSectionInfo(PKSEFramebuffer& fb);
        void setStatus(std::string message, int frames = 300);
    };

} // namespace UI

#endif
