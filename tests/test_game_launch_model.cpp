#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "UI/GameLaunchModel.h"

namespace {

// Faithful port of libnx runtime/argv.c argvSetup() tokenisation (libnx 20cbcfd9): whitespace
// separates, a leading '"' opens an argument that ends at the next '"', and there is no escape
// character. Every argv PokeBank or the return host builds is checked against this.
std::vector<std::string> libnxSplitArgv(const std::string& args) {
    std::vector<std::string> out;
    const char* argstart = nullptr;
    size_t arglen = 0;
    bool quote = false;
    for (size_t i = 0; i < args.size(); ++i) {
        const char c = args[i];
        if (!argstart && std::isspace(static_cast<unsigned char>(c))) continue;
        if (!argstart) {
            if (c == '"') { argstart = &args[i + 1]; quote = true; }
            else { argstart = &args[i]; ++arglen; }
            continue;
        }
        const bool end = quote ? c == '"' : std::isspace(static_cast<unsigned char>(c)) != 0;
        if (!end) ++arglen;
        if (end && arglen) {
            out.emplace_back(argstart, arglen);
            argstart = nullptr;
            quote = false;
            arglen = 0;
        }
    }
    if (arglen) out.emplace_back(argstart, arglen);
    return out;
}

// Runs the host's own argv parser on a loader argv string, as main() receives it.
RhParseResult hostParse(const std::string& argvString, RhState& state) {
    std::vector<std::string> tokens = libnxSplitArgv(argvString);
    std::vector<char*> argv;
    for (auto& token : tokens) argv.push_back(token.data());
    argv.push_back(nullptr);
    return rh_parse_args(static_cast<int>(tokens.size()), argv.data(), &state);
}

void testReturnHostPolicy() {
    using namespace UI;
    // Quoting matches libnx exactly: verbatim, never escaped; '"' and control bytes are refused.
    char buf[64] = "";
    assert(rh_append_quoted(buf, sizeof(buf), "sdmc:/a b.nro"));
    assert(rh_append_quoted(buf, sizeof(buf), "C:\\dir"));
    assert(std::string(buf) == "\"sdmc:/a b.nro\" \"C:\\dir\"");
    assert((libnxSplitArgv(buf) == std::vector<std::string>{"sdmc:/a b.nro", "C:\\dir"}));
    assert(!rh_append_quoted(buf, sizeof(buf), "bad\"quote"));
    assert(!rh_append_quoted(buf, sizeof(buf), "tab\there"));
    assert(!rh_append_quoted(buf, sizeof(buf), ""));
    char tiny[8] = "";
    assert(!rh_append_quoted(tiny, sizeof(tiny), "123456"));  // needs 9 bytes with quotes+NUL
    assert(rh_append_quoted(tiny, sizeof(tiny), "12345"));
    assert(std::string(tiny) == "\"12345\"");

    assert(rh_same_sd_path("sdmc:/switch/DrasticDS.nro", "/switch/drasticds.NRO"));
    assert(!rh_same_sd_path("sdmc:/switch/DrasticDS.nro", "sdmc:/switch/drastic/.emu/DrasticDS_nx.nro"));
    assert(!rh_same_sd_path("", "/x"));

    RhState state;
    // RetroArch: core + ROM; a normal Quit schedules nothing -> back to PokeBank.
    assert(hostParse("\"sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro\" "
                     "\"sdmc:/switch/PokeBank-NX.nro\" \"return-on-exit\" "
                     "\"sdmc:/retroarch/cores/gambatte_libretro_libnx.nro\" "
                     "\"sdmc:/roms/gb/Pokemon Red.gb\"", state) == RH_PARSE_OK);
    assert(state.mode == RH_MODE_EXIT && state.hops == 0);
    assert(std::string(state.returnPath) == "sdmc:/switch/PokeBank-NX.nro");
    assert((libnxSplitArgv(state.childArgv) == std::vector<std::string>{
        "sdmc:/retroarch/cores/gambatte_libretro_libnx.nro", "sdmc:/roms/gb/Pokemon Red.gb"}));
    state.hops = 1;
    assert(rh_next_action(&state, "") == RH_ACTION_RETURN);
    assert(rh_next_action(&state, nullptr) == RH_ACTION_RETURN);
    // A core restart / content reload re-requests the same core: honoured, never intercepted.
    state.hops = 3;
    assert(rh_next_action(&state, "sdmc:/retroarch/cores/gambatte_libretro_libnx.nro") ==
           RH_ACTION_CHAIN);
    char returnArgv[RH_LOADER_PATH_CAP + 4];
    assert(rh_build_return_argv(&state, returnArgv, sizeof(returnArgv)));
    assert((libnxSplitArgv(returnArgv) == std::vector<std::string>{"sdmc:/switch/PokeBank-NX.nro"}));

    // DraStic: launcher (hop 1) -> emulator (hop 2) -> "Quit to launcher" names the launcher.
    assert(hostParse("\"h.nro\" \"sdmc:/switch/PokeBank-NX.nro\" \"return-on-launcher\" "
                     "\"/switch/DrasticDS.nro\" \"sdmc:/switch/drastic/games/Pokemon Platinum.nds\"",
                     state) == RH_PARSE_OK);
    assert(state.mode == RH_MODE_LAUNCHER);
    state.hops = 1;
    assert(rh_next_action(&state, "sdmc:/switch/drastic/.emu/DrasticDS_nx.nro") == RH_ACTION_CHAIN);
    assert(rh_next_action(&state, "sdmc:/switch/DrasticDS.nro") == RH_ACTION_CHAIN); // self-restart
    state.hops = 2;
    assert(rh_next_action(&state, "sdmc:/switch/DrasticDS.nro") == RH_ACTION_RETURN_INTERCEPTED);
    assert(rh_next_action(&state, "") == RH_ACTION_RETURN);

    // Malformed protocol never starts a child.
    assert(hostParse("\"h.nro\" \"r.nro\" \"x.nro\"", state) == RH_PARSE_ARGC);
    assert(hostParse("\"h.nro\" \"r.nro\" \"sideways\" \"x.nro\"", state) == RH_PARSE_MODE);
    assert(hostParse("\"h.nro\" \"r.nro\" \"return-on-exit\" \"a\" \"b\" \"c\"", state) == RH_PARSE_ARGC);
    const std::string longPath(RH_LOADER_PATH_CAP, 'x');
    assert(hostParse("\"h.nro\" \"" + longPath + "\" \"return-on-exit\" \"x.nro\"", state) ==
           RH_PARSE_RETURN_PATH);
    assert(hostParse("\"h.nro\" \"r.nro\" \"return-on-exit\" \"" + longPath + "\"", state) ==
           RH_PARSE_CHILD_PATH);
}

void testChainPlans() {
    using namespace UI;
    GameLaunchDescriptor retro;
    retro.backend = GameLaunchBackend::RetroArch;
    retro.state = GameLaunchState::Ready;
    retro.providerId = "retroarch";
    retro.corePath = "sdmc:/retroarch/cores/mgba_libretro_libnx.nro";
    retro.launcherPath = retro.corePath;
    retro.contentPath = "sdmc:/roms/gba/Pokemon - Emerald Version (USA, Europe).gba";

    // Default (no opt-in): the hardware-accepted direct core handoff, byte for byte.
    auto plan = buildGameLaunchChainPlan(retro);
    assert(plan.ok && !plan.viaReturnHost);
    assert(plan.nroPath == retro.corePath);
    assert((libnxSplitArgv(plan.argv) == std::vector<std::string>{retro.corePath, retro.contentPath}));
    assert(plan.quitRoute == GameLaunchQuitRoute::LoaderDefault);
    assert(std::string(gameLaunchQuitRouteNotice(plan.quitRoute)).find("Homebrew Menu") != std::string::npos);

    GameLaunchReturnOptions options;
    options.pokeBankNroPath = "sdmc:/switch/PokeBank-NX.nro";
    options.returnHostNroPath = "sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro";
    options.returnHostEnabled = true;
    plan = buildGameLaunchChainPlan(retro, options);
    assert(plan.ok && plan.viaReturnHost && plan.returnHostSkipped.empty());
    assert(plan.nroPath == options.returnHostNroPath);
    assert(plan.quitRoute == GameLaunchQuitRoute::ReturnsToPokeBank);
    // The host's own parser recovers exactly what the direct path would have passed.
    RhState state;
    assert(hostParse(plan.argv, state) == RH_PARSE_OK);
    assert(state.mode == RH_MODE_EXIT);
    assert(std::string(state.returnPath) == options.pokeBankNroPath);
    assert(std::string(state.childPath) == retro.corePath);
    assert(std::string(state.childArgv) == buildGameLaunchChainPlan(retro).argv);

    // DraStic launches its launcher NRO with the ROM; Quit to launcher goes back to it.
    GameLaunchDescriptor drastic;
    drastic.backend = GameLaunchBackend::HomebrewNro;
    drastic.state = GameLaunchState::Ready;
    drastic.providerId = "drastic";
    drastic.launcherPath = "sdmc:/switch/DrasticDS.nro";
    drastic.contentPath = "sdmc:/switch/drastic/games/Pokemon Platinum.nds";
    plan = buildGameLaunchChainPlan(drastic);
    assert(plan.ok && plan.quitRoute == GameLaunchQuitRoute::EmulatorLauncher);
    // argv[0] stays DraStic's own NRO: libnx mounts an NRO's RomFS from argv[0].
    assert(libnxSplitArgv(plan.argv).front() == drastic.launcherPath);
    plan = buildGameLaunchChainPlan(drastic, options);
    assert(plan.ok && plan.viaReturnHost);
    assert(hostParse(plan.argv, state) == RH_PARSE_OK && state.mode == RH_MODE_LAUNCHER);
    assert(std::string(state.firstChildPath) == drastic.launcherPath);

    // Launcher-only never forwards a ROM.
    drastic.state = GameLaunchState::LauncherOnly;
    plan = buildGameLaunchChainPlan(drastic);
    assert(plan.ok && libnxSplitArgv(plan.argv) == std::vector<std::string>{drastic.launcherPath});
    drastic.state = GameLaunchState::Ready;

    // Values hbloader/libnx cannot carry are refused before PokeBank exits.
    GameLaunchDescriptor bad = drastic;
    bad.contentPath = "sdmc:/roms/Pokemon \"Platinum\".nds";
    plan = buildGameLaunchChainPlan(bad);
    assert(!plan.ok && !plan.error.empty());
    bad = drastic;
    bad.launcherPath = "sdmc:/" + std::string(RH_LOADER_PATH_CAP, 'a') + ".nro";
    assert(!buildGameLaunchChainPlan(bad).ok);
    bad = drastic;
    bad.contentPath = "sdmc:/" + std::string(500, 'b') + ".nds";
    assert(buildGameLaunchChainPlan(bad).ok);  // fits the loader directly
    // ...but the longer hosted argv falls back to the proven direct path instead of failing.
    GameLaunchReturnOptions longOptions = options;
    const std::string longHost = "sdmc:/" + std::string(500, 'h') + ".nro";
    const std::string longReturn = "sdmc:/" + std::string(500, 'r') + ".nro";
    longOptions.returnHostNroPath = longHost;
    longOptions.pokeBankNroPath = longReturn;
    bad.launcherPath = "sdmc:/" + std::string(500, 'l') + ".nro";
    plan = buildGameLaunchChainPlan(bad, longOptions);
    assert(plan.ok && !plan.viaReturnHost && !plan.returnHostSkipped.empty());
    assert(plan.quitRoute == GameLaunchQuitRoute::EmulatorLauncher);
    // Unknown own path -> direct.
    GameLaunchReturnOptions noSelf = options;
    noSelf.pokeBankNroPath = {};
    plan = buildGameLaunchChainPlan(retro, noSelf);
    assert(plan.ok && !plan.viaReturnHost && !plan.returnHostSkipped.empty());

    GameLaunchDescriptor title;
    title.backend = GameLaunchBackend::SwitchTitle;
    title.state = GameLaunchState::Ready;
    assert(!buildGameLaunchChainPlan(title).ok);
}

void testReturnSelection() {
    using namespace UI;
    GameLaunchReturnSelection selection;
    selection.profileIdentity = "00112233445566778899aabbccddeeff";
    selection.gameId = "emerald_gba";
    selection.sourceIdentity = "retroarch|sdmc:/retroarch/saves/Pokemon Emerald.srm";
    selection.sourceKind = 2;
    selection.titleId = 0x0100ABCD12345000ULL;
    selection.userIndex = 1;
    selection.titleIndex = 7;
    selection.classicGamesActive = true;
    selection.savedAt = 1'760'000'000;
    std::string text;
    assert(serializeGameLaunchReturnSelection(selection, text));
    GameLaunchReturnSelection parsed;
    assert(parseGameLaunchReturnSelection(text, selection.savedAt + 3600, parsed));
    assert(parsed.profileIdentity == selection.profileIdentity && parsed.gameId == selection.gameId);
    assert(parsed.sourceIdentity == selection.sourceIdentity && parsed.sourceKind == 2);
    assert(parsed.titleId == selection.titleId && parsed.userIndex == 1 && parsed.titleIndex == 7);
    assert(parsed.classicGamesActive && parsed.savedAt == selection.savedAt);
    // CRLF tolerated; unknown keys ignored.
    std::string crlf;
    for (char c : text) { if (c == '\n') crlf += "\r\n"; else crlf.push_back(c); }
    assert(parseGameLaunchReturnSelection(crlf + "future=1\n", selection.savedAt, parsed));

    // Stale, future-dated, truncated, or corrupted records are ignored.
    assert(!parseGameLaunchReturnSelection(
        text, selection.savedAt + kGameLaunchReturnSelectionMaxAgeSeconds + 1, parsed));
    assert(parsed.gameId.empty());
    assert(!parseGameLaunchReturnSelection(text, selection.savedAt - 3600, parsed));
    assert(!parseGameLaunchReturnSelection(text.substr(0, text.find("saved=")), selection.savedAt, parsed));
    assert(!parseGameLaunchReturnSelection("POKEBANK_LAUNCH_RETURN_V0\ngame=x\nsaved=1\n", 1, parsed));
    std::string badKind = text;
    badKind.replace(badKind.find("kind=2"), 6, "kind=-2");
    assert(!parseGameLaunchReturnSelection(badKind, selection.savedAt, parsed));
    std::string badTitle = text;
    badTitle.replace(badTitle.find("title=") + 6, 1, "G");
    assert(!parseGameLaunchReturnSelection(badTitle, selection.savedAt, parsed));
    // Values that would break the line format are refused at write time.
    GameLaunchReturnSelection unsafe = selection;
    unsafe.sourceIdentity = "line\nbreak";
    assert(!serializeGameLaunchReturnSelection(unsafe, text));
    unsafe = selection;
    unsafe.gameId.clear();
    assert(!serializeGameLaunchReturnSelection(unsafe, text));
}

} // namespace

int main() {
    using namespace UI;
    testReturnHostPolicy();
    testChainPlans();
    testReturnSelection();

    assert(normalizedLaunchStem("sdmc:/retroarch/cores/savefiles/Pokemon Yellow.srm") == "pokemonyellow");
    assert(normalizedLaunchStem("/roms/gba/Pokemon - Emerald Version (USA).gba") ==
           "pokemonemeraldversionusa");
    // Hardware playlists commonly use the official UTF-8 Pokémon spelling. The é bytes must
    // normalize to e instead of disappearing and producing the unusable "pokmon..." stem.
    assert(normalizedLaunchStem("sdmc:/roms/gb/Pok\xC3\xA9mon - Red Version (USA, Europe).gb") ==
           "pokemonredversionusaeurope");
    assert(normalizedLaunchStem("sdmc:/roms/gb/Pok\xC3\xA9mon - Blue Version (USA, Europe).gb") ==
           "pokemonblueversionusaeurope");
    assert(normalizedLaunchStem("sdmc:/roms/gb/archive.zip#Pok\xC3\xA9mon - Red Version (USA, Europe).gb") ==
           "pokemonredversionusaeurope");
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
    // still resolve, including the official UTF-8 Pokémon spelling, without colliding with later
    // releases such as FireRed.
    assert(gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pokemon - Red Version (USA, Europe) (SGB Enhanced).gb"));
    assert(gameLaunchCandidateStemMatches(
        "blue_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pokemon - Blue Version (USA, Europe) (SGB Enhanced).gb"));
    assert(gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pok\xC3\xA9mon - Red Version (USA, Europe) (SGB Enhanced).gb"));
    assert(gameLaunchCandidateStemMatches(
        "blue_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pok\xC3\xA9mon - Blue Version (USA, Europe) (SGB Enhanced).gb"));
    // RetroArch may represent compressed content as archive.zip#member.gb. The member identity
    // must still resolve to the exact release and must not broaden Red into FireRed.
    assert(gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gb/Pokemon Red.zip#Pok\xC3\xA9mon - Red Version (USA, Europe).gb"));
    assert(!gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gba/FireRed.zip#Pok\xC3\xA9mon - FireRed Version (USA).gba"));
    assert(!gameLaunchCandidateStemMatches(
        "red_gb",
        "sdmc:/retroarch/cores/savefiles/main.srm",
        "sdmc:/roms/gba/Pok\xC3\xA9mon - FireRed Version (USA).gba"));
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

    // Gen V: "pokemonblack" prefixes "pokemonblack2"; the original and the sequel must never
    // claim each other's ROM, by official name or by stem containment (archives have no header).
    const std::string blackSave = "sdmc:/switch/drastic/user/backup/Pokemon Black.dsv";
    const std::string black2Save = "sdmc:/switch/drastic/user/backup/Pokemon Black 2.dsv";
    assert(gameLaunchCandidateStemMatches(
        "black_nds", blackSave, "sdmc:/switch/drastic/games/Pokemon - Black Version (USA, Europe).nds"));
    assert(!gameLaunchCandidateStemMatches(
        "black_nds", blackSave, "sdmc:/switch/drastic/games/Pokemon - Black Version 2 (USA, Europe).nds"));
    assert(!gameLaunchCandidateStemMatches(
        "black_nds", blackSave, "sdmc:/roms/nds/Pokemon Black 2.zip"));
    assert(gameLaunchCandidateStemMatches(
        "black2_nds", black2Save, "sdmc:/roms/nds/Pokemon Black 2.zip"));
    assert(!gameLaunchCandidateStemMatches(
        "black2_nds", black2Save, "sdmc:/roms/nds/Pokemon Black.nds"));
    assert(gameLaunchCandidateStemMatches(
        "white2_nds", "sdmc:/switch/drastic/user/backup/main.dsv",
        "sdmc:/roms/nds/Pokemon White 2.nds"));
    assert(!gameLaunchCandidateStemMatches(
        "white_nds", "sdmc:/switch/drastic/user/backup/main.dsv",
        "sdmc:/roms/nds/Pokemon White 2.nds"));
    assert(!gameLaunchCandidateStemMatches(
        "white_nds", "sdmc:/switch/drastic/user/backup/main.dsv",
        "sdmc:/roms/nds/Pokemon Black.nds"));
    assert(gameLaunchContentSupported("white2_nds", "/roms/Pokemon White 2.nds"));

    // Hardware regression: bounded DS discovery may see duplicate ROM/archive copies. The
    // emulator-owned raw .nds file must outrank a generic archive while unrelated releases remain
    // ineligible. This lets ZR auto-resolve the common duplicate case without weakening ambiguity
    // safety for a true tie.
    const std::string platinumSave =
        "sdmc:/switch/drastic/user/backup/Pokemon Platinum Version (USA).dsv";
    const int drasticRawScore = gameLaunchCandidateScore(
        "platinum_nds", platinumSave,
        "sdmc:/switch/drastic/games/Pokemon Platinum Version (USA).nds");
    const int genericArchiveScore = gameLaunchCandidateScore(
        "platinum_nds", platinumSave,
        "sdmc:/roms/nds/Pokemon Platinum Version (USA).zip");
    assert(drasticRawScore > genericArchiveScore);
    assert(gameLaunchCandidateScore(
               "platinum_nds", platinumSave,
               "sdmc:/switch/drastic/games/Pokemon Pearl Version (USA).nds") < 0);

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
