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

    // DraStic accepted a selected ROM/archive as positional argv[1] starting in 1.1.0. Keep the
    // product launch model from regressing to the older behavior where the NRO opens successfully
    // but ignores the selected game and shows DraStic's own file chooser.
    assert(!drasticDirectLaunchVersionSupportsPositionalRom("1.0.9"));
    assert(!drasticDirectLaunchVersionSupportsPositionalRom("v1.0.99"));
    assert(drasticDirectLaunchVersionSupportsPositionalRom("1.1.0"));
    assert(drasticDirectLaunchVersionSupportsPositionalRom("v1.1.2"));
    assert(drasticDirectLaunchVersionSupportsPositionalRom("2.0.0"));
    assert(!drasticDirectLaunchVersionSupportsPositionalRom("unknown"));

    assert(gameLaunchContentSupported("yellow_gb", "/roms/Pokemon Yellow.gb"));
    assert(gameLaunchContentSupported("crystal_gbc", "/roms/Pokemon Crystal.gbc"));
    assert(gameLaunchContentSupported("emerald_gba", "/roms/Pokemon Emerald.gba"));
    assert(gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.nds"));
    assert(gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.zip"));
    assert(gameLaunchContentSupported("platinum_nds", "/roms/Pokemon Platinum.rar"));

    // Hardware regression: Red and Blue saves can have generic/short save names while the
    // RetroArch playlist uses full No-Intro-style ROM names. These short release identities must
    // still resolve, without colliding with later releases such as FireRed.
    assert(gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pokemon - Red Version (USA, Europe) (SGB Enhanced).gb"));
    assert(gameLaunchCandidateStemMatches(
        "blue_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pokemon - Blue Version (USA, Europe) (SGB Enhanced).gb"));
    assert(!gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gba/Pokemon - FireRed Version (USA).gba"));
    assert(!gameLaunchCandidateStemMatches(
        "gold_gbc",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/nds/Pokemon - HeartGold Version (USA).nds"));

    assert(gameLaunchCandidateStemMatches(
        "platinum_nds",
        "sdmc:/switch/drastic/user/backup/Pokemon Platinum.dsv",
        "sdmc:/switch/drastic/games/Pokemon Platinum Version (USA).nds"));
    assert(gameLaunchCandidateStemMatches(
        "heartgold_nds",
        "sdmc:/switch/drastic/user/backup/main.dsv",
        "sdmc:/roms/nds/Pokemon HeartGold Version.nds"));
    assert(!gameLaunchCandidateStemMatches(
        "platinum_nds",
        "sdmc:/switch/drastic/user/backup/main.dsv",
        "sdmc:/roms/nds/Pokemon Pearl Version.nds"));
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
