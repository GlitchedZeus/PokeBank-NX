#!/usr/bin/env python3
"""HG/SS encounter placeholders frozen PKHeX still matches but PokeBank NX rejects.

docs/legality/campaign-20261028/README.md, from pret/pokeheartgold (pinned in
HGSS_SAFARI_SOURCE_PINS.json and HGSS_SWARM_SOURCE_PINS.json):
  - Safari Zone (met location 202): with the Safari system flag set, land, surf and rod
    encounters all go through FieldSystem_GenerateSafariEncounter and the ten-slot Safari
    tables (src/field/encounter_check.c); without it the battle cannot give a Safari Ball.
    The ordinary-bank rows (types below 10) PKHeX keeps at 202 are placeholders, so a Safari
    Ball capture must fit a real Safari slot and frame.
  - Rod swarms: tools/legality_completion/hgss_swarm_repair.py removes Magikarp Good/Super
    Rod rows the native map/type swarm gate cannot enable. The removed rows are listed in
    HGSS_SWARM_REMOVED_ROWS.json, which tools/gen_legality_gen4_data/run.sh --check keeps in
    step with the repair.
A record matches only when such a placeholder row explains its species and level and the
analyzer's only Invalid issues are the encounter/tile/RNG-frame ones, so any other complaint
still fails the oracle. The move, nickname and ribbon checks read the matched encounter (its
species, level and evolution history), so their complaints may follow from the missing
placeholder too; they are allowed when the analyzer found no encounter at all, or when the
record evolved after capture (pre-evolution level-up moves and the any-language nickname
allowance need the pre-evolution encounter, which another species' slot cannot give). Real Safari and retained rod rows stay enforced by the analyzer; the
oracle line does not say which row PKHeX matched.

  hgss_placeholders.py gen5 JOINED   prints the Gen V oracle's PKHeX-VALID / ours-INVALID
                                     lines no placeholder explains; counts go to stderr.
"""
import json
import pathlib
import re
import sys
from collections import Counter

REPO = pathlib.Path(__file__).resolve().parents[2]
SAFARI_LOCATION = 202
POKE_TRANSFER = 30001
SAFARI_BALL = 5
MAGIKARP = 129
SAFARI_TYPES = 10   # SlotType4 values from 10 up are the real Safari tables

ENCOUNTER_ISSUES = {
    "No matching Gen IV encounter for this species, level, location, ball and PID/IV",
    "No Generation IV encounter matches this transferred Pokemon (species, level, ball, PID/IV)",
    "Encounter ground tile type does not match the encounter",
    "The PID/IV does not match any reachable RNG frame for this wild encounter and lead",
}

NO_ENCOUNTER = {
    "No matching Gen IV encounter for this species, level, location, ball and PID/IV",
    "No Generation IV encounter matches this transferred Pokemon (species, level, ball, PID/IV)",
}
# Checks that depend on the matched encounter (see above).
DEPENDENT_ISSUES = re.compile(r"Move [1-4] \(#\d+\) cannot be learned by this Pokemon in (Generation IV"
                              r"|any generation it lived in \(III/IV before transfer, V after\))"
                              r"|Nickname does not match the species name for its language"
                              r"|\d+ ribbon\(s\) are not obtainable or are missing a prerequisite")

_rows = None


def _array(source, signature):
    body = source.split("const " + signature, 1)[1].split("= {", 1)[1].split("\n};", 1)[0]
    return [[int(x.strip().rstrip("uUlL"), 0) for x in m[1:-1].split(",")] for m in re.findall(r"\{[^{}]+\}", body)]


def rows():
    """(version, species) -> level ranges of Safari placeholders; version -> removed swarm rows."""
    global _rows
    if _rows is None:
        source = (REPO / "src/Legality/Gen4LegalityData.cpp").read_text()
        areas, slots = _array(source, "Area4 kAreas[]"), _array(source, "Slot4 kSlots[]")
        safari = {}
        for version, location, kind, _rate, _tile, start, count in areas:
            if version in (7, 8) and location == SAFARI_LOCATION and kind < SAFARI_TYPES:
                for slot in slots[start:start + count]:
                    safari.setdefault((version, slot[0]), set()).add((slot[2], slot[3]))
        removed = json.loads((REPO / "docs/legality/campaign-20261028/HGSS_SWARM_REMOVED_ROWS.json").read_text())
        assert removed["species"] == MAGIKARP
        swarm = {}
        for version, location, _kind, low, high in removed["rows"]:
            swarm.setdefault(version, set()).add((location, low, high))
        _rows = safari, swarm
    return _rows


def u16(hexdata, offset):
    return int(hexdata[2 * offset + 2:2 * offset + 4] + hexdata[2 * offset:2 * offset + 2], 16)


def u8(hexdata, offset):
    return int(hexdata[2 * offset:2 * offset + 2], 16)


def _fields(hexdata, enc):
    """Origin version, the species PKHeX's wild slot gives (before any evolution), ball and met
    level of an HG/SS-origin non-egg record."""
    kind, _, species = enc.partition(":")
    if len(hexdata) < 2 * 0x88 or kind != "EncounterSlot4" or not species.isdigit():
        return None
    version = u8(hexdata, 0x5F)
    if version not in (7, 8) or u16(hexdata, 0x7E):
        return None
    return version, int(species), u8(hexdata, 0x83), u8(hexdata, 0x84) & 0x7F


def _issues_fit(hexdata, species, issues):
    if not issues:
        return False
    dependent = u16(hexdata, 0x08) != species or any(i in NO_ENCOUNTER for i in issues)
    return all(i in ENCOUNTER_ISSUES or (dependent and DEPENDENT_ISSUES.fullmatch(i)) for i in issues)


def native(cart, hexdata, enc, issues):
    """An HG/SS-origin PK4 on any Gen IV cart (met location in the Pt/HG/SS field 0x46)."""
    f = _fields(hexdata, enc)
    if f is None or u16(hexdata, 0x44) or not _issues_fit(hexdata, f[1], issues):
        return None
    version, species, ball, level = f
    location = u16(hexdata, 0x46)
    safari, swarm = rows()
    if location == SAFARI_LOCATION and ball == SAFARI_BALL:
        if any(low <= level <= high for low, high in safari.get((version, species), ())):
            return "hgss-safari-placeholder"
    elif species == MAGIKARP and ball != SAFARI_BALL:
        if any(loc == location and low <= level <= high for loc, low, high in swarm.get(version, ())):
            return "hgss-swarm-placeholder"
    return None


def transferred(hexdata, enc, issues):
    """An HG/SS-origin PK5 after Poke Transfer: the Gen IV met location is gone and the met
    level is the level at transfer, so a placeholder at or below it is enough."""
    f = _fields(hexdata, enc)
    if f is None or u16(hexdata, 0x80) != POKE_TRANSFER or not _issues_fit(hexdata, f[1], issues):
        return None
    version, species, ball, level = f
    safari, swarm = rows()
    if ball == SAFARI_BALL:
        if any(low <= level for low, _high in safari.get((version, species), ())):
            return "hgss-safari-placeholder"
    elif species == MAGIKARP:
        if any(low <= level for _loc, low, _high in swarm.get(version, ())):
            return "hgss-swarm-placeholder"
    return None


def gen5(path):
    counts = Counter()
    for line in open(path):
        left, right = line.rstrip("\n").split("\t")
        a, b = left.split(" ; ")[0].split(" "), right.split(" ")
        if a[2] != "VALID" or b[1] != "INVALID":
            continue
        issues = [i.strip() for i in right.partition(" ;")[2].split(" | ") if i.strip() and not i.strip().startswith("W: ")]
        name = transferred(a[1], a[3], issues)
        if name:
            counts[name] += 1
        else:
            print(line, end="")
    for name, n in sorted(counts.items()):
        print(f"deliberately stricter ({name}): {n}", file=sys.stderr)


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] != "gen5":
        sys.exit(__doc__)
    gen5(sys.argv[2])
