#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text(encoding='utf-8')
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'{path}: expected exactly one target, found {count}')
    p.write_text(text.replace(old, new, 1), encoding='utf-8')

# 1) Treat RetroArch archive-member paths as the member identity, e.g.
# archive.zip#Pokémon - Red Version.gb -> pokemonredversion.
replace_once(
    'include/UI/GameLaunchModel.h',
'''inline std::string normalizedLaunchStem(std::string_view path) {
    const size_t slash = path.find_last_of("/\\\\");
    const size_t first = slash == std::string_view::npos ? 0 : slash + 1;
    const size_t dot = path.find_last_of('.');
    const size_t last = dot == std::string_view::npos || dot < first ? path.size() : dot;
''',
'''inline std::string normalizedLaunchStem(std::string_view path) {
    const size_t slash = path.find_last_of("/\\\\");
    const size_t archiveMember = path.find_last_of('#');
    size_t first = slash == std::string_view::npos ? 0 : slash + 1;
    if (archiveMember != std::string_view::npos && archiveMember + 1 > first)
        first = archiveMember + 1;
    const size_t dot = path.find_last_of('.');
    const size_t last = dot == std::string_view::npos || dot < first ? path.size() : dot;
''')

# 2) Add physical-path validation for RetroArch's archive#member pseudo-paths.
replace_once(
    'src/UI/GameLauncher.cpp',
'''std::string parentPath(std::string path) {
    while (path.size() > 6 && path.back() == '/') path.pop_back();
    const size_t slash = path.find_last_of("/\\\\");
    if (slash == std::string::npos) return {};
    if (path.rfind("sdmc:/", 0) == 0 && slash <= 5) return "sdmc:/";
    return path.substr(0, slash);
}

bool hasExtension''',
'''std::string parentPath(std::string path) {
    while (path.size() > 6 && path.back() == '/') path.pop_back();
    const size_t slash = path.find_last_of("/\\\\");
    if (slash == std::string::npos) return {};
    if (path.rfind("sdmc:/", 0) == 0 && slash <= 5) return "sdmc:/";
    return path.substr(0, slash);
}

std::string retroArchPhysicalContentPath(std::string_view contentPath) {
    const size_t member = contentPath.find('#');
    std::string physical(contentPath.substr(0, member));
    return switchPath(std::move(physical));
}

bool retroArchContentExists(std::string_view contentPath) {
    if (contentPath.empty()) return false;
    const std::string full = switchPath(std::string(contentPath));
    if (regularFile(full)) return true;
    if (contentPath.find('#') == std::string_view::npos) return false;
    return regularFile(retroArchPhysicalContentPath(contentPath));
}

bool hasExtension''')

# 3) Restore the exact core-only RetroArch descriptor architecture that was hardware-good before
# the frontend/fallback experiments. Keep new resolver improvements separate from lifecycle.
replace_once(
    'src/UI/GameLauncher.cpp',
'''    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = std::move(launcher);
    } else {
        result.launcherPath = std::move(launcher);
    }
''',
'''    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = result.corePath;
    } else {
        result.launcherPath = std::move(launcher);
    }
''')

replace_once(
    'src/UI/GameLauncher.cpp',
'''    if (!regularFile(result.contentPath)) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "The linked game file is missing; link it again.";
        return result;
    }
''',
'''    if ((kind == GameLaunchProviderKind::RetroArch && !retroArchContentExists(result.contentPath)) ||
        (kind != GameLaunchProviderKind::RetroArch && !regularFile(result.contentPath))) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "The linked game file is missing; link it again.";
        return result;
    }
''')

replace_once(
    'src/UI/GameLauncher.cpp',
'''    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    } else {
        result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    }

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The linked emulator/launcher NRO is missing.";
    } else if (!regularFile(result.contentPath)) {
''',
'''    if (kind == GameLaunchProviderKind::RetroArch) {
        result.corePath = defaultRetroArchCore(gameId);
        result.launcherPath = result.corePath;
    } else {
        result.launcherPath = defaultLauncherPath(kind, gameId, stored.contentPath);
    }

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The linked emulator/launcher NRO is missing.";
    } else if ((kind == GameLaunchProviderKind::RetroArch && !retroArchContentExists(result.contentPath)) ||
               (kind != GameLaunchProviderKind::RetroArch && !regularFile(result.contentPath))) {
''')

replace_once(
    'src/UI/GameLauncher.cpp',
'''    result.launcherPath = defaultLauncherPath(GameLaunchProviderKind::RetroArch, gameId, sourcePath);
    result.corePath = defaultRetroArchCore(gameId);

    if (result.launcherPath.empty() || !regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "The RetroArch frontend NRO is not installed at a known path.";
        return result;
    }
    if (result.corePath.empty() || !regularFile(result.corePath)) {
''',
'''    result.corePath = defaultRetroArchCore(gameId);
    result.launcherPath = result.corePath;

    if (result.corePath.empty() || !regularFile(result.corePath)) {
''')

# 4) Accept playlist archive-member content when the physical archive exists.
replace_once(
    'src/UI/GameLauncher.cpp',
'''                if (!gameLaunchContentSupported(gameId, content)) { pos += 6; continue; }
                if (!regularFile(content)) { pos += 6; continue; }
                sawFamilyCompatibleContent = true;
''',
'''                if (!gameLaunchContentSupported(gameId, content)) { pos += 6; continue; }
                if (!retroArchContentExists(content)) { pos += 6; continue; }
                sawFamilyCompatibleContent = true;
''')

# 5) If no usable playlist entry exists, search only bounded/common ROM roots. Never scan every
# installed HOME application on the launch frame.
replace_once(
    'src/UI/GameLauncher.cpp',
'''    result.state = GameLaunchState::NeedsContentLink;
    return result;
}

GameLaunchDescriptor resolveKnownHomebrew''',
'''    std::vector<std::string> looseRoots{"sdmc:/retroarch/downloads"};
    const auto suffix = [&](std::string_view value) {
        return gameId.size() >= value.size() &&
               gameId.substr(gameId.size() - value.size()) == value;
    };
    if (suffix("_gb")) {
        looseRoots.insert(looseRoots.end(), {"sdmc:/retroarch/roms/gb", "sdmc:/retroarch/roms/GB",
                                             "sdmc:/roms/gb", "sdmc:/roms/GB"});
    } else if (suffix("_gbc")) {
        looseRoots.insert(looseRoots.end(), {"sdmc:/retroarch/roms/gbc", "sdmc:/retroarch/roms/GBC",
                                             "sdmc:/roms/gbc", "sdmc:/roms/GBC"});
    } else if (suffix("_gba")) {
        looseRoots.insert(looseRoots.end(), {"sdmc:/retroarch/roms/gba", "sdmc:/retroarch/roms/GBA",
                                             "sdmc:/roms/gba", "sdmc:/roms/GBA"});
    }
    auto looseMatches = findContentMatches(looseRoots, sourcePath, gameId, 1, 384);
    if (!looseMatches.empty()) {
        int bestScore = -1;
        size_t bestIndex = 0;
        bool trueTie = false;
        for (size_t i = 0; i < looseMatches.size(); ++i) {
            const int score = gameLaunchCandidateScore(gameId, sourcePath, looseMatches[i]);
            if (score > bestScore) {
                bestScore = score;
                bestIndex = i;
                trueTie = false;
            } else if (score == bestScore &&
                       normalizedLaunchStem(looseMatches[i]) != normalizedLaunchStem(looseMatches[bestIndex])) {
                trueTie = true;
            }
        }
        if (bestScore >= 0 && !trueTie) {
            return directDescriptor(GameLaunchProviderKind::RetroArch, gameId, "retroarch",
                                    result.corePath, looseMatches[bestIndex],
                                    "Matched the exact release in a bounded RetroArch ROM root.");
        }
    }

    result.state = GameLaunchState::NeedsContentLink;
    return result;
}

GameLaunchDescriptor resolveKnownHomebrew''')

# 6) DraStic: matching is already release-specific. If multiple copies remain tied, choose the
# stable first strongest copy instead of forcing an in-app ROM chooser. No matches still falls back
# to launching DraStic itself.
replace_once(
    'src/UI/GameLauncher.cpp',
'''        if (bestScore >= 0 && !tied)
            return directDescriptor(kind, gameId, providerId, launcher, matches[bestIndex],
                                    "Selected the strongest exact-release game-file match.");
''',
'''        if (bestScore >= 0 && (!tied || kind == GameLaunchProviderKind::DraStic))
            return directDescriptor(kind, gameId, providerId, launcher, matches[bestIndex],
                                    tied && kind == GameLaunchProviderKind::DraStic
                                        ? "Selected a deterministic duplicate copy of the exact DS release."
                                        : "Selected the strongest exact-release game-file match.");
''')

# 7) Remove the blocking installed-HOME-forwarder scan from RetroArch resolution.
replace_once(
    'src/UI/GameLauncher.cpp',
'''    if (kind == GameLaunchProviderKind::RetroArch) {
        auto resolved = resolveRetroArch(gameId, sourcePath, bindingKey);
#ifdef __SWITCH__
        if (resolved.state == GameLaunchState::NeedsContentLink) {
            if (const uint64_t forwarderTitle = installedGameForwarderTitle(gameId);
                forwarderTitle != 0) {
                GameLaunchDescriptor forwarder;
                forwarder.backend = GameLaunchBackend::SwitchTitle;
                forwarder.titleId = forwarderTitle;
                forwarder.providerId = std::string(providerId);
                forwarder.state = GameLaunchState::Ready;
                forwarder.detail = "Launch the installed HOME forwarder for this exact game.";
                return forwarder;
            }
        }
#endif
        return resolved;
    }
''',
'''    if (kind == GameLaunchProviderKind::RetroArch)
        return resolveRetroArch(gameId, sourcePath, bindingKey);
''')

# 8) Restore the earlier hardware-good core-only chain exactly and support archive-member paths.
replace_once(
    'src/UI/GameLauncher.cpp',
'''    if (descriptor.backend == GameLaunchBackend::RetroArch) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The RetroArch frontend NRO is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            (descriptor.corePath.empty() || !regularFile(descriptor.corePath))) {
            error = "The RetroArch core NRO for this game is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !regularFile(descriptor.contentPath)) {
            error = "The linked game ROM is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.corePath.empty()) {
            std::string coreArgv = quoted(descriptor.corePath);
            if (!descriptor.contentPath.empty())
                coreArgv += " " + quoted(descriptor.contentPath);
            const Result directRc = envSetNextLoad(descriptor.corePath.c_str(), coreArgv.c_str());
            if (R_SUCCEEDED(directRc)) return true;
        }

        // Compatibility fallback: some hbloader/RetroArch combinations prefer entering through
        // the frontend and selecting the libretro core with -L.
        std::string argv = quoted(descriptor.launcherPath);
        if (descriptor.state != GameLaunchState::LauncherOnly && !descriptor.corePath.empty())
            argv += " -L " + quoted(descriptor.corePath);
        if (descriptor.state != GameLaunchState::LauncherOnly && !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
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
            !retroArchContentExists(descriptor.contentPath)) {
            error = "The linked game ROM is missing.";
            return false;
        }
        std::string argv = quoted(target);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(target.c_str(), argv.c_str());
''')

# 9) Lock the two new resolver shapes in host tests/contracts.
p = Path('tests/test_game_launch_model.cpp')
text = p.read_text(encoding='utf-8')
needle = '''    assert(normalizedLaunchStem("sdmc:/roms/gb/Pok\\xC3\\xA9mon - Blue Version (USA, Europe).gb") ==\n           "pokemonblueversionusaeurope");\n'''
addition = needle + '''    assert(normalizedLaunchStem("sdmc:/roms/gb/archive.zip#Pok\\xC3\\xA9mon - Red Version (USA, Europe).gb") ==\n           "pokemonredversionusaeurope");\n'''
if text.count(needle) != 1:
    raise SystemExit('tests/test_game_launch_model.cpp: missing UTF-8 Blue normalization anchor')
p.write_text(text.replace(needle, addition, 1), encoding='utf-8')

p = Path('tests/test_game_hub_contract.py')
text = p.read_text(encoding='utf-8')
anchor = '''require("refreshHubSelectionFromCache();" in drawer_update and\n        "refreshHubPreview(false);" in drawer_update and\n        "refreshHubPreview();" not in drawer_update,\n        "Quick Games A must hydrate selected save presentation without launch discovery")\n'''
extra = anchor + '''require("installedGameForwarderTitle(gameId)" not in source[source.find("if (kind == GameLaunchProviderKind::RetroArch)"):source.find("if (kind == GameLaunchProviderKind::MGBA")],\n        "RetroArch explicit launch must not block on a console-wide HOME forwarder scan")\nrequire("retroArchContentExists" in source and "archive#member" not in source,\n        "RetroArch launch path must validate archive-member content through its physical archive")\n'''
if text.count(anchor) != 1:
    raise SystemExit('tests/test_game_hub_contract.py: missing Quick Games anchor')
p.write_text(text.replace(anchor, extra, 1), encoding='utf-8')
