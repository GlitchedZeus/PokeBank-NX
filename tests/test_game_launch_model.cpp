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

    assert(gameLaunchProviderKind("mGBA") == GameLaunchProviderKind::MGBA);
    assert(gameLaunchProviderKind("Tico") == GameLaunchProviderKind::Tico);
    assert(gameLaunchProviderKind("DraStic") == GameLaunchProviderKind::DraStic);
    assert(gameLaunchProviderKind("melonDS") == GameLaunchProviderKind::MelonDS);
    assert(gameLaunchProviderAcceptsContentArgument(GameLaunchProviderKind::DraStic));
    assert(gameLaunchProviderAcceptsContentArgument(GameLaunchProviderKind::MelonDS));
    assert(gameLaunchProviderAcceptsContentArgument(GameLaunchProviderKind::MGBA));
    assert(gameLaunchProviderAcceptsContentArgument(GameLaunchProviderKind::RetroArch));
    assert(gameLaunchProviderKind("Manual") == GameLaunchProviderKind::Unknown);

    assert(gameLaunchContentSupported("yellow_gb", "/roms/Pokemon Yellow.gb"));
    assert(gameLaunchContentSupported("crystal_gbc", "/roms/Pokemon Crystal.gbc"));
    assert(gameLaunchContentSupported("emerald_gba", "/roms/Pokemon Emerald.gba"));
    assert(gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.nds"));
    assert(gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.zip"));
    assert(gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.rar"));
    assert(!gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.gba"));

    assert(gameLaunchBindingFamilyPrefix("profile", "emerald_gba") ==
           "profile|emerald_gba|");
    assert(gameLaunchBindingFamilyPrefix("", "emerald_gba").empty());
    assert(gameLaunchBindingKey("profile", "emerald_gba", "source") ==
           "profile|emerald_gba|source");
    assert(gameLaunchBindingKey("", "emerald_gba", "source").empty());

    assert(std::string(gameLaunchActionLabel(GameLaunchState::Ready)) == "Launch");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::LauncherOnly)) == "Launch Emulator");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::ChooseSource)) == "Choose & Launch");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::NeedsContentLink)) == "Link Game File");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::LauncherMissing)) == "Launcher Missing");
    assert(std::string(gameLaunchActionLabel(GameLaunchState::Unavailable)) == "Unavailable");

    GameLaunchDescriptor descriptor;
    assert(!descriptor.ready());
    descriptor.state = GameLaunchState::Ready;
    assert(descriptor.ready());
    descriptor.state = GameLaunchState::LauncherOnly;
    assert(descriptor.ready());
    return 0;
}
