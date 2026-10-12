from pathlib import Path

p = Path('src/UI/GameLauncher.cpp')
s = p.read_text(encoding='utf-8')

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
if s.count(old_core) != 1:
    raise SystemExit(f'RetroArch default-core block changed; expected 1, found {s.count(old_core)}')
s = s.replace(old_core, new_core, 1)

old_match = '''                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.launcherPath = result.corePath;
                result.state = GameLaunchState::Ready;
                result.detail = "RetroArch playlist matched this save to its game file and core.";
                ::closedir(dir);
                return result;
'''
new_match = '''                matches.push_back({std::move(content), std::move(core)});
                pos += 6;
'''
if s.count(old_match) != 1:
    raise SystemExit(f'RetroArch immediate-return block changed; expected 1, found {s.count(old_match)}')
s = s.replace(old_match, new_match, 1)

p.write_text(s, encoding='utf-8')
print('Reconciled RetroArch NDS core resolution and ambiguity handling while preserving current launch behavior.')
