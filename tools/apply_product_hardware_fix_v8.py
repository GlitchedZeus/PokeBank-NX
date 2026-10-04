#!/usr/bin/env python3
from pathlib import Path
import re


def once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, got {count}")
    return text.replace(old, new, 1)


launcher_path = Path("src/UI/GameLauncher.cpp")
launcher = launcher_path.read_text(encoding="utf-8")
launcher = once(
    launcher,
    """std::string compactGameTitle(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (unsigned char c : value)
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}
""",
    """std::string compactGameTitle(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        // HOME forwarders commonly use the official “Pokémon” spelling. Normalize UTF-8 é/É
        // instead of dropping it and turning the identity word into “pokmon”.
        if (c == 0xC3 && i + 1 < value.size()) {
            const unsigned char next = static_cast<unsigned char>(value[i + 1]);
            if (next == 0xA9 || next == 0x89) {
                out.push_back('e');
                ++i;
                continue;
            }
        }
        if (c < 0x80 && std::isalnum(c))
            out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

bool installedForwarderNameMatches(std::string_view gameId, std::string_view normalizedName) {
    auto shaped = [&](std::string_view token) {
        const std::string plain(token);
        const std::string version = plain + "version";
        const std::string pokemon = "pokemon" + plain;
        const std::string pokemonVersion = pokemon + "version";
        return normalizedName == plain || normalizedName == version ||
               normalizedName == pokemon || normalizedName == pokemonVersion ||
               normalizedName.rfind(pokemon, 0) == 0;
    };

    // Release-specific matching avoids collisions such as Red/FireRed, Gold/HeartGold and
    // Diamond/Brilliant Diamond while accepting both plain and official Pokémon forwarder names.
    if (gameId == "red_gb") return shaped("red");
    if (gameId == "blue_gb") return shaped("blue");
    if (gameId == "yellow_gb") return shaped("yellow");
    if (gameId == "gold_gbc") return shaped("gold");
    if (gameId == "silver_gbc") return shaped("silver");
    if (gameId == "crystal_gbc") return shaped("crystal");
    if (gameId == "ruby_gba") return shaped("ruby");
    if (gameId == "sapphire_gba") return shaped("sapphire");
    if (gameId == "firered_gba") return shaped("firered");
    if (gameId == "leafgreen_gba") return shaped("leafgreen");
    if (gameId == "emerald_gba") return shaped("emerald");
    if (gameId == "diamond_nds") return shaped("diamond");
    if (gameId == "pearl_nds") return shaped("pearl");
    if (gameId == "platinum_nds") return shaped("platinum");
    if (gameId == "heartgold_nds") return shaped("heartgold");
    if (gameId == "soulsilver_nds") return shaped("soulsilver");
    return false;
}
""",
    "forwarder title normalizer",
)
launcher = once(
    launcher,
    """    const auto* game = PokeVault::Games::findGame(gameId);
    if (!game) { cache[key] = 0; return 0; }
    const std::string wanted = compactGameTitle(game->title);
    if (wanted.size() < 4) { cache[key] = 0; return 0; }
""",
    """    if (!PokeVault::Games::findGame(gameId)) { cache[key] = 0; return 0; }
""",
    "forwarder lookup setup",
)
launcher, count = re.subn(
    r'if \(name\.find\("pokemon"\) != std::string::npos &&\s*name\.find\(wanted\) != std::string::npos\) \{',
    'if (installedForwarderNameMatches(gameId, name)) {',
    launcher,
    count=1,
)
if count != 1:
    raise SystemExit(f"installed forwarder matcher: expected exactly one match, got {count}")
launcher_path.write_text(launcher, encoding="utf-8")


source_path = Path("src/UI/SaveSelectScreen.cpp")
source = source_path.read_text(encoding="utf-8")
source = once(source, "Color(5, 14, 30, 54)", "Color(5, 14, 30, 40)", "region scrim")
source = once(source, "Color(3, 10, 24, 132)", "Color(3, 10, 24, 104)", "hero glass")
source = once(
    source,
    "fb.drawFilledCircle(cx, cy, buttonD / 2, Colors::SurfaceRaised);",
    "fb.drawFilledCircle(cx, cy, buttonD / 2,\n"
    "                                focused ? withAlpha(Colors::Info, 46) : Colors::PanelAlt);",
    "dock focus fill",
)

pattern = re.compile(
    r'else if \(game\.sourceKind == SelectedSourceKind::RetroArchFRLG\) \{\s*'
    r'legacyInstanceIndex = 0;\s*legacyInstanceScroll = 0;\s*legacyNotice\.clear\(\);\s*'
    r'overlay = Overlay::LegacyInstances;\s*\}',
    re.S,
)
replacement = """else if (game.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            // Full Games / Quick Games X is the profile-assignment surface. Offer a still-unclaimed
            // validated save for this exact release before falling back to already assigned instances.
            rebuildUnassignedLegacySources();
            unassignedLegacySources.erase(
                std::remove_if(unassignedLegacySources.begin(), unassignedLegacySources.end(),
                    [&](const LegacyAssignmentEntry& entry) { return entry.gameId != game.gameId; }),
                unassignedLegacySources.end());
            if (!unassignedLegacySources.empty()) {
                legacyAssignmentIndex = 0;
                legacyAssignmentScroll = 0;
                legacyNotice = "Choose a validated save to assign to this profile.";
                overlay = Overlay::LegacyAssignment;
            } else {
                legacyInstanceIndex = 0;
                legacyInstanceScroll = 0;
                legacyNotice.clear();
                overlay = Overlay::LegacyInstances;
            }
        }"""
# First occurrence is openSaveSourceForCurrentTitle. The later Current Game workspace source entry
# intentionally keeps its existing-instances behavior.
source, count = pattern.subn(replacement, source, count=1)
if count != 1:
    raise SystemExit(f"legacy assignment route: expected exactly one match, got {count}")
source_path.write_text(source, encoding="utf-8")


tests_path = Path("tests/test_game_hub_contract.py")
tests = tests_path.read_text(encoding="utf-8")
tests = tests.replace("Color(3, 10, 24, 132)", "Color(3, 10, 24, 104)")
tests = tests.replace(
    '"fb.drawFilledCircle(cx, cy, buttonD / 2, Colors::SurfaceRaised);" in source,',
    '"focused ? withAlpha(Colors::Info, 46) : Colors::PanelAlt" in source,',
)
extra = '''
require("drawTrainerPortrait" not in classic_draw,
        "the PKSE/full Games browser must not show trainer portraits")
require("installedForwarderNameMatches" in launcher and 'gameId == "platinum_nds"' in launcher and
        'gameId == "emerald_gba"' in launcher,
        "installed HOME forwarders must be matched by exact release identity before emulator fallback")
require("Choose a validated save to assign to this profile." in source and
        "entry.gameId != game.gameId" in source,
        "Games X must offer unassigned saves for the selected game/profile")
'''
if extra.strip() not in tests:
    tests += extra
tests_path.write_text(tests, encoding="utf-8")
