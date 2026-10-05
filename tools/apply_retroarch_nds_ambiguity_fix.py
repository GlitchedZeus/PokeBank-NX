from pathlib import Path


PATH = Path("src/UI/GameLauncher.cpp")
text = PATH.read_text(encoding="utf-8")

# NDS RetroArch launches need a trusted default core just like GB/GBC/GBA.
# Prefer DeSmuME when installed, then melonDS. This is executable selection only;
# content still comes from the validated playlist/link flow below.
fn_start = text.index("std::string defaultRetroArchCore(std::string_view gameId) {")
fn_end = text.index("\n}\n\nGameLaunchProviderKind providerKindForSourcePath", fn_start)
fn = text[fn_start:fn_end]
if 'gameId.substr(gameId.size() - 4) == "_nds"' not in fn:
    marker = "    return {};\n"
    if fn.count(marker) != 1:
        raise SystemExit("defaultRetroArchCore return marker changed")
    nds = (
        '    if (gameId.size() >= 4 && gameId.substr(gameId.size() - 4) == "_nds")\n'
        '        return firstExisting({"sdmc:/retroarch/cores/desmume_libretro_libnx.nro",\n'
        '                              "sdmc:/retroarch/cores/melonds_libretro_libnx.nro"});\n'
    )
    fn = fn.replace(marker, nds + marker, 1)
    text = text[:fn_start] + fn + text[fn_end:]

# The reconciler must inspect every viable same-stem playlist entry before deciding.
# Returning on the first hit silently turns an ambiguous library into an arbitrary launch.
early = '''                result.contentPath = std::move(content);
                result.corePath = std::move(core);
                result.launcherPath = result.corePath;
                result.state = GameLaunchState::Ready;
                result.detail = "RetroArch playlist matched this save to its game file and core.";
                ::closedir(dir);
                return result;
'''
replacement = '''                matches.push_back({std::move(content), std::move(core)});
                pos += 6;
'''
if early in text:
    text = text.replace(early, replacement, 1)
elif replacement not in text:
    raise SystemExit("RetroArch playlist first-match block changed")

# A unique playlist entry may use a valid core different from the preferred default;
# launcherPath must follow the chosen playlist core.
unique = '''        if (matches.size() == 1) {
            result.contentPath = std::move(matches.front().content);
            result.corePath = std::move(matches.front().core);
            result.state = GameLaunchState::Ready;
'''
unique_fixed = '''        if (matches.size() == 1) {
            result.contentPath = std::move(matches.front().content);
            result.corePath = std::move(matches.front().core);
            result.launcherPath = result.corePath;
            result.state = GameLaunchState::Ready;
'''
if unique in text:
    text = text.replace(unique, unique_fixed, 1)
elif unique_fixed not in text:
    raise SystemExit("RetroArch unique-match block changed")

PATH.write_text(text, encoding="utf-8")
print("Applied RetroArch NDS core + ambiguity reconciliation fix.")
