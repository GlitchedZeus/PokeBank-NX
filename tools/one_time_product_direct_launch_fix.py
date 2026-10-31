#!/usr/bin/env python3
from pathlib import Path


def replace_exact(path: Path, old: str, new: str, count: int = 1) -> None:
    text = path.read_text(encoding="utf-8")
    actual = text.count(old)
    if actual != count:
        raise SystemExit(
            f"{path}: expected {count} occurrence(s), found {actual}: {old[:100]!r}"
        )
    path.write_text(text.replace(old, new), encoding="utf-8")


save = Path("src/UI/SaveSelectScreen.cpp")

# Dock logos/labels are neutral at rest and blue only while focused.
replace_exact(
    save,
    "const Color ink = focused ? Colors::SelectedText : Colors::TextPrimary;",
    "const Color ink = focused ? Colors::Info : Colors::TextPrimary;",
)
replace_exact(
    save,
    '''fb.drawText(cx - lw / 2, cy + buttonD / 2 + 10, dockLabels[i],
                        focused ? Colors::SelectedText
                                : i == 0 ? Colors::Info : Colors::TextSecondary,
                        TextStyle::Caption);''',
    '''fb.drawText(cx - lw / 2, cy + buttonD / 2 + 10, dockLabels[i],
                        focused ? Colors::Info : Colors::TextSecondary,
                        TextStyle::Caption);''',
)

# Full PKSE-style Games is the save/profile assignment browser: no trainer portraits.
replace_exact(
    save,
    '''            const auto portrait = trainerPortraitForGame(
                title.gameId, title.trainerGenderKnown, title.trainerGender);
            drawTrainerPortrait(fb, x + CLASSIC_TILE_W - 58, y + 18, 48, 58,
                                portrait, false);

''',
    "",
)
replace_exact(
    save,
    '''            // Warm the small set of game-card / trainer assets before the full grid becomes
            // interactive. The caches make later entries effectively free, and row-to-row
            // navigation no longer stalls on first-use PNG/control-icon decoding.
            if (user) {
                for (const auto& title : user->titles) {
                    const std::string_view artKey =
                        title.sourceKind == SelectedSourceKind::RetroArchFRLG
                            ? std::string_view(title.artworkKey)
                            : std::string_view(title.gameId);
                    (void)SystemIcons::gameCardIcon(artKey, title.titleId);
                    const auto portrait = trainerPortraitForGame(
                        title.gameId, title.trainerGenderKnown, title.trainerGender);
                    if (portrait.assetKey && portrait.assetKey[0] != '\\0')
                        (void)SystemIcons::trainerPortrait(portrait.assetKey);
                }
            }
''',
    '''            // Warm only game-card artwork before the full assignment browser becomes
            // interactive. Trainer portraits belong on Product Home/workspace, not this grid.
            if (user) {
                for (const auto& title : user->titles) {
                    const std::string_view artKey =
                        title.sourceKind == SelectedSourceKind::RetroArchFRLG
                            ? std::string_view(title.artworkKey)
                            : std::string_view(title.gameId);
                    (void)SystemIcons::gameCardIcon(artKey, title.titleId);
                }
            }
''',
)

# Reveal more of the real region art while preserving readable text.
replace_exact(save, "Color(5, 14, 30, 96)", "Color(5, 14, 30, 54)")
replace_exact(save, "Color(3, 10, 24, 184)", "Color(3, 10, 24, 132)")
replace_exact(save, "Color(220, 232, 248, 72)", "Color(220, 232, 248, 92)")

# ZR means launch. Missing ROM metadata stays a setup notice; it never opens a file picker.
replace_exact(
    save,
    '''        if (descriptor.state == GameLaunchState::NeedsContentLink) {
            openGameFilePicker(title.gameId, provider, instance.path(), key, true);
            return false;
        }
''',
    '''        if (descriptor.state == GameLaunchState::NeedsContentLink) {
            legacyNotice = descriptor.detail.empty()
                ? "Direct launch could not resolve this game's ROM. Use X / Save-Source to set it up."
                : descriptor.detail;
            return false;
        }
''',
)
replace_exact(
    save,
    '''        if (launchDescriptor.state == GameLaunchState::NeedsContentLink)
            return beginLaunchLinkForCurrentTitle();
''',
    '''        if (launchDescriptor.state == GameLaunchState::NeedsContentLink) {
            hubNotice = launchDescriptor.detail.empty()
                ? "Direct launch could not resolve this game's ROM. Use X / Save-Source to set it up."
                : launchDescriptor.detail;
            return false;
        }
''',
)
replace_exact(
    save,
    '''            const std::string launchLabel = gameLaunchActionLabel(launchDescriptor.state);
            const bool launchActionable = launchDescriptor.ready() ||
                launchDescriptor.state == GameLaunchState::NeedsContentLink ||
                launchDescriptor.state == GameLaunchState::ChooseSource;
''',
    '''            const std::string launchLabel =
                launchDescriptor.state == GameLaunchState::NeedsContentLink
                    ? std::string("Setup Required")
                    : std::string(gameLaunchActionLabel(launchDescriptor.state));
            const bool launchActionable = launchDescriptor.ready() ||
                launchDescriptor.state == GameLaunchState::ChooseSource;
''',
)

launcher = Path("src/UI/GameLauncher.cpp")
text = launcher.read_text(encoding="utf-8")


def rep(old: str, new: str, count: int = 1) -> None:
    global text
    actual = text.count(old)
    if actual != count:
        raise SystemExit(
            f"GameLauncher.cpp: expected {count}, found {actual}: {old[:100]!r}"
        )
    text = text.replace(old, new)


rep(
    '#include "Utils/PokeBankPaths.h"\n',
    '#include "Utils/PokeBankPaths.h"\n#include "Games/GameIdentity.h"\n',
)

anchor = '''bool readTextFile(const std::string& path, std::string& out, size_t maxSize = 4 * 1024 * 1024) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    if (std::fseek(f, 0, SEEK_END) != 0) { std::fclose(f); return false; }
    const long size = std::ftell(f);
    if (size < 0 || static_cast<size_t>(size) > maxSize) { std::fclose(f); return false; }
    std::rewind(f);
    out.resize(static_cast<size_t>(size));
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size() || !closeOk) {
        out.clear();
        return false;
    }
    return true;
}
'''
helpers = anchor + r'''

std::string compactGameTitle(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (unsigned char c : value)
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

#ifdef __SWITCH__
uint64_t installedGameForwarderTitle(std::string_view gameId) {
    static std::map<std::string, uint64_t> cache;
    const std::string key(gameId);
    if (const auto found = cache.find(key); found != cache.end()) return found->second;

    const auto* game = PokeVault::Games::findGame(gameId);
    if (!game) { cache[key] = 0; return 0; }
    const std::string wanted = compactGameTitle(game->title);
    if (wanted.size() < 4) { cache[key] = 0; return 0; }

    uint64_t unique = 0;
    int matches = 0;
    s32 offset = 0;
    while (offset < 2048) {
        NsApplicationRecord records[32]{};
        s32 count = 0;
        if (R_FAILED(nsListApplicationRecord(records, 32, offset, &count)) || count <= 0) break;
        for (s32 i = 0; i < count; ++i) {
            auto* control = static_cast<NsApplicationControlData*>(
                std::malloc(sizeof(NsApplicationControlData)));
            if (!control) continue;
            u64 outSize = 0;
            const Result rc = nsGetApplicationControlData(
                NsApplicationControlSource_Storage, records[i].application_id,
                control, sizeof(NsApplicationControlData), &outSize);
            if (R_SUCCEEDED(rc) && outSize != 0) {
                NacpLanguageEntry* language = nullptr;
                if (R_SUCCEEDED(nacpGetLanguageEntry(&control->nacp, &language)) && language) {
                    const std::string name = compactGameTitle(language->name);
                    if (name.find("pokemon") != std::string::npos &&
                        name.find(wanted) != std::string::npos) {
                        unique = records[i].application_id;
                        ++matches;
                    }
                }
            }
            std::free(control);
            if (matches > 1) break;
        }
        if (matches > 1 || count < 32) break;
        offset += count;
    }
    if (matches != 1) unique = 0;
    cache[key] = unique;
    return unique;
}
#endif

std::vector<std::string> configuredDraSticLibraryRoots() {
    std::vector<std::string> roots{
        "sdmc:/switch/drastic/games",
        "sdmc:/switch/drastic/roms",
        "sdmc:/roms/nds",
        "sdmc:/roms/NDS"
    };
    std::string ini;
    if (readTextFile("sdmc:/switch/drastic/launcher.ini", ini, 512 * 1024)) {
        size_t start = 0;
        while (start < ini.size()) {
            const size_t end = ini.find('\n', start);
            std::string line = ini.substr(start,
                (end == std::string::npos ? ini.size() : end) - start);
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
                line.pop_back();
            const size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string value = line.substr(eq + 1);
                const size_t first = value.find_first_not_of(" \t\"");
                if (first != std::string::npos) value.erase(0, first); else value.clear();
                while (!value.empty() &&
                       (value.back() == ' ' || value.back() == '\t' || value.back() == '\"'))
                    value.pop_back();
                value = switchPath(value);
                if (value.rfind("sdmc:/", 0) == 0 && directory(value))
                    roots.push_back(std::move(value));
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
    return roots;
}
'''
rep(anchor, helpers)

anchor = '''std::vector<std::string> findContentMatches(std::initializer_list<std::string> roots,
                                            std::string_view sourcePath,
                                            std::string_view gameId,
                                            size_t maxDepth = 3,
                                            size_t maxFiles = 512) {
    std::vector<std::string> matches;
    std::set<std::string> visited;
    size_t examined = 0;
    for (const auto& root : roots) {
        collectContentMatches(root, sourcePath, gameId, 0, maxDepth,
                              examined, maxFiles, visited, matches);
        if (examined >= maxFiles) break;
    }
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}
'''
overload = anchor + r'''

std::vector<std::string> findContentMatches(const std::vector<std::string>& roots,
                                            std::string_view sourcePath,
                                            std::string_view gameId,
                                            size_t maxDepth,
                                            size_t maxFiles) {
    std::vector<std::string> matches;
    std::set<std::string> visited;
    size_t examined = 0;
    for (const auto& root : roots) {
        collectContentMatches(root, sourcePath, gameId, 0, maxDepth,
                              examined, maxFiles, visited, matches);
        if (examined >= maxFiles) break;
    }
    std::sort(matches.begin(), matches.end());
    matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    return matches;
}
'''
rep(anchor, overload)

# RetroArch: launch the libretro core NRO itself, not the frontend with -L.
rep(
    '''    result.providerId = std::string(providerId);
    result.launcherPath = std::move(launcher);
    result.contentPath = std::move(content);
    if (kind == GameLaunchProviderKind::RetroArch)
        result.corePath = defaultRetroArchCore(gameId);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
''',
    '''    result.providerId = std::string(providerId);
    result.contentPath = std::move(content);
    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = result.corePath;
    } else {
        result.launcherPath = std::move(launcher);
    }

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
''',
)

rep(
    '''    result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    result.contentPath = stored.contentPath;
    if (kind == GameLaunchProviderKind::RetroArch)
        result.corePath = defaultRetroArchCore(gameId);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
''',
    '''    result.contentPath = stored.contentPath;
    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = result.corePath;
    } else {
        result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    }

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
''',
)

rep(
    '''    result.backend = GameLaunchBackend::RetroArch;
    result.providerId = "retroarch";
    result.launcherPath = defaultLauncherPath(
        GameLaunchProviderKind::RetroArch, gameId, sourcePath);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "RetroArch is not installed at a known path.";
        return result;
    }
''',
    '''    result.backend = GameLaunchBackend::RetroArch;
    result.providerId = "retroarch";
    result.corePath = defaultRetroArchCore(gameId);
    result.launcherPath = result.corePath;

    if (result.corePath.empty() || !regularFile(result.corePath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "A compatible RetroArch core NRO is not installed.";
        return result;
    }
''',
)

rep(
    '''                core = switchPath(core);
                if (core == "DETECT" || core.empty() || !regularFile(core)) {
                    pos += 6;
                    continue;
                }

                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.state = GameLaunchState::Ready;
''',
    '''                core = switchPath(core);
                if (core == "DETECT" || core.empty() || !regularFile(core))
                    core = defaultRetroArchCore(gameId);
                if (core.empty() || !regularFile(core)) {
                    pos += 6;
                    continue;
                }

                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.launcherPath = result.corePath;
                result.state = GameLaunchState::Ready;
''',
)

# DraStic: use the same configured game-library roots its working forwarders use.
rep(
    '''        } else if (kind == GameLaunchProviderKind::DraStic) {
            matches = findContentMatches({
                                             "sdmc:/switch/drastic/games",
                                             "sdmc:/switch/drastic/roms",
                                             "sdmc:/switch/drastic",
                                             "sdmc:/roms/nds",
                                             "sdmc:/roms/NDS"
                                         },
                                         sourcePath, gameId, 6, 2048);
''',
    '''        } else if (kind == GameLaunchProviderKind::DraStic) {
            const auto roots = configuredDraSticLibraryRoots();
            matches = findContentMatches(roots, sourcePath, gameId, 6, 4096);
''',
)

# Prefer an already-installed exact-game HOME forwarder before rebuilding a launch.
forwarder_hook = '''#ifdef __SWITCH__
    if (const uint64_t forwarderTitle = installedGameForwarderTitle(gameId); forwarderTitle != 0) {
        GameLaunchDescriptor result;
        result.backend = GameLaunchBackend::SwitchTitle;
        result.titleId = forwarderTitle;
        result.providerId = std::string(providerId);
        result.state = GameLaunchState::Ready;
        result.detail = "Launch the installed HOME forwarder for this exact game.";
        return result;
    }
#endif

'''
provider_anchor = '''    const GameLaunchProviderKind kind = providerKindForSourcePath(providerId, sourcePath);
    const std::string resolvedProvider = providerIdForKind(kind, providerId);
'''
rep(provider_anchor, forwarder_hook + provider_anchor, 2)

rep(
    '''    binding.launcherPath = defaultLauncherPath(kind, gameId, content);
    binding.contentPath = content;
    if (kind == GameLaunchProviderKind::RetroArch) {
        binding.corePath = defaultRetroArchCore(gameId);
''',
    '''    binding.launcherPath = defaultLauncherPath(kind, gameId, content);
    binding.contentPath = content;
    if (kind == GameLaunchProviderKind::RetroArch) {
        binding.corePath = defaultRetroArchCore(gameId);
        binding.launcherPath = binding.corePath;
''',
)

# Exact forwarder-compatible argv semantics: core NRO + ROM, with no frontend file picker.
rep(
    '''    if (descriptor.backend == GameLaunchBackend::RetroArch ||
        descriptor.backend == GameLaunchBackend::HomebrewNro) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The configured emulator/launcher NRO is missing.";
            return false;
        }

        std::string argv = quoted(descriptor.launcherPath);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.corePath.empty())
            argv += " -L " + quoted(descriptor.corePath);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);

        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Homebrew launch request failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }
''',
    '''    if (descriptor.backend == GameLaunchBackend::RetroArch) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        const std::string& target = descriptor.corePath.empty()
            ? descriptor.launcherPath : descriptor.corePath;
        if (!regularFile(target)) {
            error = "The RetroArch core NRO for this game is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !regularFile(descriptor.contentPath)) {
            error = "The linked game ROM is missing.";
            return false;
        }
        std::string argv = quoted(target);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(target.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "RetroArch game launch failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }

    if (descriptor.backend == GameLaunchBackend::HomebrewNro) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The configured emulator/launcher NRO is missing.";
            return false;
        }
        std::string argv = quoted(descriptor.launcherPath);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Homebrew game launch failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }
''',
)

launcher.write_text(text, encoding="utf-8")

# Update product contracts to lock the hardware-fix intent.
contract = Path("tests/test_game_hub_contract.py")
c = contract.read_text(encoding="utf-8")
old_glass = 'require("Color(3, 10, 24, 184)" in source and\n'
if c.count(old_glass) != 1:
    raise SystemExit("glass contract anchor changed")
c = c.replace(old_glass, 'require("Color(3, 10, 24, 132)" in source and\n')
c += r'''

# Hardware regression: Games is assignment-first; trainer art belongs to Product Home/workspace.
classic_fn = source[source.index("void SaveSelectScreen::drawClassicGameSources"):
                    source.index("void SaveSelectScreen::draw(PKSEFramebuffer&", source.index("void SaveSelectScreen::drawClassicGameSources"))]
require("drawTrainerPortrait" not in classic_fn,
        "full Games assignment browser must not render trainer portraits")
require("focused ? Colors::Info : Colors::TextSecondary" in source and
        ": i == 0 ? Colors::Info" not in source,
        "Games/Banks/Items/Search/More must turn blue only when focused")
require("Color(5, 14, 30, 54)" in source and "Color(3, 10, 24, 132)" in source,
        "Product Home region artwork must remain visible under lighter glass")
require("return beginLaunchLinkForCurrentTitle();" not in source and
        "Direct launch could not resolve this game's ROM" in source,
        "ZR Launch must never become an automatic ROM-file browser")
require('argv += " -L "' not in launcher and
        "envSetNextLoad(target.c_str(), argv.c_str())" in launcher and
        "result.launcherPath = result.corePath;" in launcher,
        "RetroArch games must chain directly into the core NRO with the ROM argument")
require("installedGameForwarderTitle" in launcher and
        '"Launch the installed HOME forwarder for this exact game."' in launcher,
        "emulator games must prefer an already-installed exact-game HOME forwarder")
require("configuredDraSticLibraryRoots" in launcher and
        '"sdmc:/switch/drastic/launcher.ini"' in launcher,
        "DraStic launch must consume configured SD library roots")
'''
contract.write_text(c, encoding="utf-8")
