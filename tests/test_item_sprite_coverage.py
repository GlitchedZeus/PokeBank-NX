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
EXTRAS = {"itemfinder", "classic-mail-envelope"}
GEN2_MAIL_ART = {
    "bluesky-mail", "eon-mail", "flower-mail", "liteblue-mail",
    "lovely-mail", "mirage-mail", "morph-mail", "music-mail",
    "portraitmail", "surf-mail",
}
ART |= EXTRAS

# Original Gen II mail art is recovered from Bulbagarden Archives separately.
# These exact stationery backgrounds are OPTIONAL at package time: the
# deterministic generic envelope is used if Archives rejects CI downloads.
# Do not count these keys as bundled PokeAPI or Smogon art.
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

aliases = dict(re.findall(r'if \(key == "([^"]+)"\) return "([^"]+)";', KEYS))
assert aliases and len(aliases) >= 15
assert all(dst in ART or dst in GEN2_MAIL_ART for dst in aliases.values()), (
    "ItemSpriteKey alias has neither pinned art nor a verified supplemental source",
    {src: dst for src, dst in aliases.items()
     if dst not in ART and dst not in GEN2_MAIL_ART})

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
    generic_mail = 0
    eligible = 0
    for item_id in sorted(ids):
        name = entries.get(item_id, "")
        if gen == "Gen I" and 196 <= item_id <= 250:
            name = "HM%02d" % (item_id - 195) if item_id <= 200 else "TM%02d" % (item_id - 200)
        if gen == "Gen II" and item_id >= 191:
            machines = raw_ids("kG2Machines")
            if item_id in machines:
                ordinal = machines.index(item_id)
                name = "TM%02d" % (ordinal + 1) if ordinal < 50 else "HM%02d" % (ordinal - 49)
        if name in ("", "???", "(None)", "NONE", "TERU-SAMA"): continue
        eligible += 1
        key = art_key(name)
        if key in GEN2_MAIL_ART: generic_mail += 1
        elif key in ART: match += 1
        elif re.fullmatch(r"(?:tm|hm)\d+", key) and "tm-normal" in ART:
            machines += 1
        else: unresolved.append((item_id, name, key))
    print(f"{gen}: {eligible} eligible item identities, {match} exact artwork,"
          f" {machines} generic machine discs, {generic_mail} generic mail envelopes,"
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
    "Brightpowder": "bright-powder", "Parlyz Heal": "paralyze-heal",
    "Itemfinder": "itemfinder", "TM27": "tm27", "HM01": "hm01",
    "DeepSeaTooth": "deep-sea-tooth", "NeverMeltIce": "never-melt-ice",
    "Leek": "stick", "Upgrade": "up-grade",
    "Flower Mail": "flower-mail",
    "Blue Sky Mail": "bluesky-mail",
    "Lite Blue Mail": "liteblue-mail",
    "Portrait Mail": "portraitmail",
    "Surf Mail": "surf-mail",
    "Air Mail": "air-mail", "Orange Mail": "orange-mail",
}.items():
    assert art_key(name) == expected, (name, art_key(name), expected)
    assert expected in ART or expected in GEN2_MAIL_ART or expected.startswith(("tm", "hm"))

assert all('"' + key + '"' in (ROOT / "tools/gen_item_sprites.py").read_text(encoding="utf-8") for key in GEN2_MAIL_ART)
assert 'ItemSpriteKey::isGen2Mail(key)' in (ROOT / "src/UI/SpriteManager.cpp").read_text(encoding="utf-8")
# Fallback aliases must NEVER own a second pointer to the same decoded PNG.
sprite_loader = (ROOT / "src/UI/SpriteManager.cpp").read_text(encoding="utf-8")
assert "return sprite ? sprite : fallback();" in sprite_loader
assert "itemSpriteCache.emplace(key, sprite); // unique owned allocation or miss" in sprite_loader
assert len(g1) >= 80 and len(g3) >= 370 and len(g4) >= 530
print("Complete generation catalog coverage audit: PASS (remaining unavailable art reported)")
