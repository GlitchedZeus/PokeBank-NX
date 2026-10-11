#!/usr/bin/env python3
"""Exhaustively audit item-art lookup across supported Gen I–IV catalogs.

No device access, no item mutations and no invented native ID translations.
Print genuinely unillustrated names instead of asserting 100% artwork.
Run in host CI to detect a regression in name-to-sprite resolution.
"""
from __future__ import annotations
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NAMES = (ROOT / "src/Names/ItemNames.cpp").read_text(encoding="utf-8")
CATALOG = (ROOT / "src/Inventory/ClassicInventoryCatalog.cpp").read_text(encoding="utf-8")
G2 = (ROOT / "src/Integration/Gen2/Gen2ReadOnlyInventory.cpp").read_text(encoding="utf-8")
KEYS = (ROOT / "include/UI/ItemSpriteKey.h").read_text(encoding="utf-8")
ART = set((ROOT / "tools/pinned_item_sprite_names.txt").read_text(encoding="utf-8").splitlines())
ART = {name for name in ART if name and not name.startswith("#")}
EXTRAS = {"itemfinder", "classic-mail-envelope", "berserk-gene"}
# True native Gen II per-item icons did not exist for these unused/historic
# items. They are custom illustrations, separate from exact-source pixels.
CUSTOM_GSC = {
    "silver-leaf", "gold-leaf", "pink-bow", "polkadot-bow",
    "gs-ball", "egg-ticket", "normal-box", "gorgeous-box", "brick-piece"
}
GEN2_MAIL_ART = {
    "bluesky-mail", "eon-mail", "flower-mail", "liteblue-mail",
    "lovely-mail", "mirage-mail", "morph-mail", "music-mail",
    "portraitmail", "surf-mail",
}
ART |= EXTRAS
# Historical equivalent icons deliberately do NOT count as original-era art.
HISTORICAL_PROXIES = {
    "x-special", "exp-all",
    # All ten Gen II berries have later-generation successors with related
    # effects. Report their icons as proxies, never as exact GSC-era artwork.
    "poison-cure-berry", "paralyze-cure-berry", "mint-berry",
    "ice-berry", "burnt-berry", "mystery-berry", "berry",
    "bitter-berry", "miracle-berry", "gold-berry",
}

# Original full-sized Gen II mail art is preferred from the Archives.
# If Archives rejects the CI download, pinned ORIGINAL Crystal mail graphics
# and palettes are assembled into 160x144 reconstructed stationery. Never
# count these as exact ROM screenshots or pinned PokeAPI 24px bag images.
GENERATOR = (ROOT / "tools/gen_item_sprites.py").read_text(encoding="utf-8")
source_block = GENERATOR.split("GEN2_MAIL_SOURCES = {", 1)[1].split("}", 1)[0]
source_keys = set(re.findall(r'^\s*"([a-z0-9-]+)":\s*"[^"]+\.png",?\s*$', source_block, re.M))
assert source_keys == GEN2_MAIL_ART, (
    "Original Gen II mail sources are not registered one-to-one",
    sorted(GEN2_MAIL_ART - source_keys), sorted(source_keys - GEN2_MAIL_ART))
assert not (GEN2_MAIL_ART & ART), "Optional Gen II backgrounds must not be counted as pinned icons"

def table(name: str) -> dict[int, str]:
    pattern = r"static const char\* " + name + r"\[\] = \{([\s\S]*?)\n\s*\};"
    match = re.search(pattern, NAMES)
    assert match, f"Missing {name} raw item table"
    return {int(idx): label for label, idx in re.findall(
        r'"([^"\n]*)"\s*,\s*//\s*(\d+)', match.group(1))}

def raw_ids(name: str) -> list[int]:
    match = re.search(r"constexpr std::array<uint16_t,\s*\d+> "
                      + name + r"\{\{([\s\S]*?)\}\};", CATALOG)
    assert match, f"Missing catalog {name}"
    return [int(v) for v in re.findall(r"\b\d+\b", match.group(1))]

def normalized(value: str) -> str:
    value = value.split(" (", 1)[0].replace("é", "e").lower()
    return re.sub(r"-+$", "", re.sub(r"[ _-]+", "-",
        re.sub(r"[^a-z0-9 _-]", "", value)))

# Capture both single-key rules and OR aliases (such as S.S.Ticket).
# They must resolve to the same canonical artwork as the runtime.
aliases = {}
for conditions, art in re.findall(r'if \(([^)\n]+)\) return "([^"]+)";', KEYS):
    for source in re.findall(r'key == "([^"]+)"', conditions):
        assert source not in aliases or aliases[source] == art, source
        aliases[source] = art
assert aliases and len(aliases) >= 15
assert all(dst in ART or dst in GEN2_MAIL_ART or dst in CUSTOM_GSC for dst in aliases.values()), (
    "ItemSpriteKey alias has neither pinned art nor a verified supplemental source",
    {src: dst for src, dst in aliases.items()
     if dst not in ART and dst not in GEN2_MAIL_ART and dst not in CUSTOM_GSC})

gen2_base = re.search(r'kBaseNames\{([\s\S]*?)\n\};', G2)
assert gen2_base
g2_values = re.findall(r'"([^"\n]*)"', gen2_base.group(1))
assert len(g2_values) == 191
g2_aliases = dict(re.findall(
    r'if \(raw == "([^"]+)"\) return "([^"]+)";', G2))

g1 = table("ITEM1_NAMES")
g3 = table("ITEM3_NAMES")
g4 = table("ITEM_NAMES")
catalogs = {
 "Gen I": (g1, ["kG1Items", "kG1Medicines", "kG1Balls",
                 "kG1Keys", "kG1Machines", "kG1PC"]),
 "Gen II": ({i: g2_aliases.get(n, n) for i, n in enumerate(g2_values)},
            ["kG2Items", "kG2Balls", "kG2KeysGS", "kG2KeysC", "kG2Machines"]),
 "Gen III": (g3, ["kG3Items", "kG3Balls", "kG3Berries", "kG3Machines",
                  "kG3KeysRS", "kG3KeysE", "kG3KeysFRLG"]),
}
def art_key(name: str) -> str:
    key = normalized(name)
    if key.startswith(("tm", "hm")) and len(key) > 2 and key[2].isdigit():
        key = re.match(r"^(?:tm|hm)\d+", key).group()
    if key == "-doll": key = "doll"
    return aliases.get(key, key)

def audit(gen: str, entries: dict[int, str], ids: set[int]) -> list[tuple[int, str, str]]:
    unresolved = []
    match = 0
    machines = 0
    mail_motifs = 0
    custom_icons = 0
    proxy = 0
    eligible = 0
    for item_id in sorted(ids):
        name = entries.get(item_id, "")
        if gen == "Gen I" and 196 <= item_id <= 250:
            name = "HM%02d" % (item_id - 195) if item_id <= 200 else "TM%02d" % (item_id - 200)
        if gen == "Gen II" and item_id >= 191:
            machine_ids = raw_ids("kG2Machines")
            if item_id in machine_ids:
                ordinal = machine_ids.index(item_id)
                name = "TM%02d" % (ordinal + 1) if ordinal < 50 else "HM%02d" % (ordinal - 49)
        if name in ("", "???", "(None)", "NONE", "TERU-SAMA"): continue
        eligible += 1
        key = art_key(name)
        if normalized(name) in HISTORICAL_PROXIES: proxy += 1
        elif key in GEN2_MAIL_ART: mail_motifs += 1
        elif key in CUSTOM_GSC: custom_icons += 1
        elif key in ART: match += 1
        elif re.fullmatch(r"(?:tm|hm)\d+", key) and "tm-normal" in ART:
            machines += 1
        else: unresolved.append((item_id, name, key))
    print(f"{gen}: {eligible} eligible item identities, {match} exact artwork,"
          f" {machines} machine discs (type-resolved at runtime),"
          f" {mail_motifs} Crystal-source stationery, {custom_icons} new custom GSC icons,"
          f" {proxy} labelled historical proxies,"
          f" {len(unresolved)} no verified sprite")
    for item_id, name, key in unresolved:
        print(f"  MISSING {gen} native ID {item_id}: {name} [lookup={key}]")
    return unresolved

unresolved = {}
for gen, (names, groups) in catalogs.items():
    ids = set()
    for name in groups: ids.update(raw_ids(name))
    unresolved[gen] = audit(gen, names, ids)

# Gen IV held item catalog: DP and PT/HGSS union, plus documented native bag ID ranges.
# This is a superset; items with no valid native field remain visible in the report
# but not an invitation to permit writes.
gen4_held = {1,2,3,4} | set(range(6,113)) | set(range(135,420))
gen4_bag = set(range(17,113)) | set(range(135,537)) | set(range(1,17))
unresolved["Gen IV"] = audit("Gen IV", g4, gen4_held | gen4_bag)

# Exact hardware regressions: these were blank with the old direct slug lookup.
for name, expected in {
    "Thunderstone": "thunder-stone", "X Defend": "x-defense",
    "X Special": "x-sp-atk", "Exp. All": "exp-share",
    "Poison Cure Berry": "pecha-berry", "Paralyze Cure Berry": "cheri-berry",
    "Mint Berry": "chesto-berry", "Ice Berry": "rawst-berry",
    "Burnt Berry": "aspear-berry", "Mystery Berry": "leppa-berry",
    "Berry": "oran-berry", "Bitter Berry": "persim-berry",
    "Miracle Berry": "lum-berry", "Gold Berry": "sitrus-berry",
    "Brightpowder": "bright-powder", "Parlyz Heal": "paralyze-heal",
    "Itemfinder": "itemfinder", "TM27": "tm27", "HM01": "hm01",
    "DeepSeaTooth": "deep-sea-tooth", "NeverMeltIce": "never-melt-ice",
    "Leek": "stick", "Upgrade": "up-grade",
    "Flower Mail": "flower-mail",
    "Blue Sky Mail": "bluesky-mail",
    "Lite Blue Mail": "liteblue-mail",
    "Portrait Mail": "portraitmail",
    "Surf Mail": "surf-mail",
    # Source-backed Gen IV key-item identities (index 433/434/436/450/
    # 453/464/482/483). Never equate unrelated generational item IDs.
    "Guidebook": "journal", "Sticker Case": "seal-case",
    "Sticker Bag": "seal-bag", "Bike": "bicycle",
    "Lunar Feather": "lunar-wing", "Secret Medicine": "secret-potion",
    "Silver Feather": "silver-wing", "Rainbow Feather": "rainbow-wing",
    "Air Mail": "air-mail", "Orange Mail": "orange-mail",
}.items():
    assert art_key(name) == expected, (name, art_key(name), expected)
    assert expected in ART or expected in GEN2_MAIL_ART or expected in CUSTOM_GSC or expected.startswith(("tm", "hm"))

assert all('"' + key + '"' in (ROOT / "tools/gen_item_sprites.py").read_text(encoding="utf-8") for key in GEN2_MAIL_ART)
assert 'ItemSpriteKey::isGen2Mail(key)' in (ROOT / "src/UI/SpriteManager.cpp").read_text(encoding="utf-8")
# Fallback aliases must NEVER own a second pointer to the same decoded PNG.
sprite_loader = (ROOT / "src/UI/SpriteManager.cpp").read_text(encoding="utf-8")
assert "return sprite ? sprite : fallback();" in sprite_loader
assert "itemSpriteCache.emplace(key, sprite); // unique owned allocation or miss" in sprite_loader
# Prove all originally missing sprites now have real image sources.
generator = (ROOT / "tools/gen_item_sprites.py").read_text(encoding="utf-8")
custom_src = (ROOT / "tools/render_classic_item_icons.py").read_text(encoding="utf-8")
for name in CUSTOM_GSC:
    assert f'"{name}"' in custom_src
assert "berserk-gene" in generator and "iberserk_gene.png" in generator
assert "render_classic_icons()" in generator
mail_renderer = (ROOT / "tools/render_gen2_mail_stationery.py").read_text(encoding="utf-8")
assert "render_unavailable_exact_mail(unavailable)" in generator
assert "tuple(dims) != (160, 144)" in generator
assert "GEN2_MAIL_SOURCES" in generator
for key in GEN2_MAIL_ART:
    assert f'"{key}"' in mail_renderer
items_panel = (ROOT / "src/UI/Panels/ItemsPanel.cpp").read_text(encoding="utf-8")
assert "if (selected && gscSource && ItemSpriteKey::isGen2Mail(normalized))" in items_panel
assert "art->width == 160 && art->height == 144" in items_panel
assert "SpriteManager::getItemSprite(selectedGscMail)" in items_panel
assert "return getItemSprite(\"tm-normal\")" not in sprite_loader

assert "ClassicMachineArtwork::artKey(" in (ROOT / "src/UI/Panels/ItemsPanel.cpp").read_text(encoding="utf-8")
assert all(not value for value in unresolved.values()), unresolved
assert len(g1) >= 80 and len(g3) >= 370 and len(g4) >= 530
print("Complete generation catalog coverage audit: PASS (remaining unavailable art reported)")
