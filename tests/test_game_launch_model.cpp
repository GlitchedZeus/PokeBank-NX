#include <cassert>
#include <string>

#include "UI/GameLaunchModel.h"

int main() {
    using namespace UI;

    assert(normalizedLaunchStem("sdmc:/retroarch/cores/savefiles/Pokemon Yellow.srm") == "pokemonyellow");
    assert(normalizedLaunchStem("/roms/gba/Pokemon - Emerald Version (USA).gba") ==
           "pokemonemeraldversionusa");
    assert(normalizedLaunchStem("Crystal") == "crystal");
    assert(normalizedLaunchStem(".sav").empty());

    assert(launchProviderIsRetroArch("retroarch"));
    assert(launchProviderIsRetroArch("RetroArch / GB"));
    assert(!launchProviderIsRetroArch("mGBA"));
    assert(!launchProviderIsRetroArch("DraStic"));

    assert(std::string(gameLaunchActionLabel(GameLaunchState::Ready)) == "Launch");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::ChooseSource)) == "Choose & Launch");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::NeedsContentLink)) == "Link Game File");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::LauncherMissing)) == "Launcher Missing");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::Unavailable)) == "Unavailable");

    GameLaunchDescriptor descriptor;
    assert(!descriptor.ready());
    descriptor.state = GameLaunchState::Ready;
    assert(descriptor.ready());
    return 0;
}
