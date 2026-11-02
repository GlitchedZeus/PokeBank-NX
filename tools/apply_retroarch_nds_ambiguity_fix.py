from pathlib import Path

PATH = Path("src/UI/GameLauncher.cpp")
text = PATH.read_text(encoding="utf-8")

old_core = '''std::string defaultRetroArchCore(std::string_view gameId) {
    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gba")
        return firstExisting({"sdmc:/retroarch/cores/mgba_libretro_libnx.nro"});
    if ((gameId.size() >= 3 && gameId.substr(gameId.size() - 3) == "_gb") ||
        (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gbc"))
        return firstExisting({"sdmc:/retroarch/cores/gambatte_libretro_libnx.nro",
                              "sdmc:/retroarch/cores/mgba_libretro_libnx.nro"});
    return {};
}
'''
new_core = '''std::string defaultRetroArchCore(std::string_view gameId) {
    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gba")
        return firstExisting({"sdmc:/retroarch/cores/mgba_libretro_libnx.nro"});
    if ((gameId.size() >= 3 && gameId.substr(gameId.size() - 3) == "_gb") ||
        (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_gbc"))
        return firstExisting({"sdmc:/retroarch/cores/gambatte_libretro_libnx.nro",
                              "sdmc:/retroarch/cores/mgba_libretro_libnx.nro"});
    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_nds")
        return firstExisting({"sdmc:/retroarch/cores/desmume_libretro_libnx.nro",
                              "sdmc:/retroarch/cores/melonds_libretro_libnx.nro"});
    return {};
}
'''
if new_core not in text:
    if old_core not in text:
        raise SystemExit("defaultRetroArchCore block changed")
    text = text.replace(old_core, new_core, 1)

old_early = '''                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.launcherPath = result.corePath;
                result.state = GameLaunchState::Ready;
                result.detail = "RetroArch playlist matched this save to its game file and core.";
                ::closedir(dir);
                return result;
'''
new_early = '''                matches.push_back({std::move(content), std::move(core)});
                pos += 6;
'''
if new_early not in text:
    if old_early not in text:
        raise SystemExit("RetroArch playlist first-match block changed")
    text = text.replace(old_early, new_early, 1)

old_unique = '''        if (matches.size() == 1) {
            result.contentPath = std::move(matches.front().content);
            result.corePath = std::move(matches.front().core);
            result.state = GameLaunchState::Ready;
'''
new_unique = '''        if (matches.size() == 1) {
            result.contentPath = std::move(matches.front().content);
            result.corePath = std::move(matches.front().core);
            result.launcherPath = result.corePath;
            result.state = GameLaunchState::Ready;
'''
if new_unique not in text:
    if old_unique not in text:
        raise SystemExit("RetroArch unique-match block changed")
    text = text.replace(old_unique, new_unique, 1)

PATH.write_text(text, encoding="utf-8")
print("Applied RetroArch NDS core + ambiguity reconciliation fix.")
