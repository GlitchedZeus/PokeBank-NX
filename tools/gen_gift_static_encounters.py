#!/usr/bin/env python3
"""Generate the Gen II-V gift / static / in-game trade / fossil / roaming encounter table.

Round 13 owner decision 7: "Gift/static encounters require proper source-backed data before v1.0
acceptance. Markers alone are provisional."

Output (committed, compiled by the build):
  include/Integration/Encounter/GiftStaticEncountersData.inc
  docs/data/gift-static-encounter-sources.json   (source files, commit SHAs, sha256, row counts)

Primary source: kwsch/PKHeX at PKHEX_PIN (the same commit the Gen IV / V wild catalogue,
tools/gen_gen45_encounter_context.py, is pinned to):
  Legality/Encounters/Data/Gen2/Encounters2.cs        StaticGSC, StaticC, StaticOddEggC, CelebiVC,
                                                      TradeGift_GSC
  Legality/Encounters/Data/Gen3/Encounters3RSE.cs     StaticRSE/R/S/E, TradeGift_RS/E
  Legality/Encounters/Data/Gen3/Encounters3FRLG.cs    StaticFRLG/FR/LG, TradeGift_FRLG/FR/LG
  Legality/Encounters/Data/Gen4/Encounters4DPPt.cs    StaticDPPt/DP/Pt/D/P, TradeGift_DPPtIngame
  Legality/Encounters/Data/Gen4/Encounters4HGSS.cs    Encounter_HGSS, StaticHG/SS, TradeGift_HGSS
  Legality/Encounters/Data/Gen5/Encounters5BW.cs      Encounter_BW, StaticB/W, TradeGift_BW/B/W
  Legality/Encounters/Data/Gen5/Encounters5B2W2.cs    Encounter_B2W2_Regular, StaticB2/W2,
                                                      Encounter_B2W2_N, TradeGift_B2W2/B2/W2
  Legality/Encounters/Templates/Gen*/Encounter{Static,Trade}*.cs   met location of a trade
                                                      (LinkTrade*NPC), level ranges, roamer
                                                      met-location sets
  Game/Locations/Locations.cs                         LinkTrade2NPC 126, LinkTrade3NPC 254,
                                                      LinkTrade4NPC 2001, LinkTrade5NPC 30002

Second source (pret decompilations, pinned):
  pret/pokecrystal: EVERY Crystal row is matched to the map script that creates it
    (maps/*.asm `givepoke` = gift, `loadwildmon` = static, Game Corner `givepoke` = prize) and the
    map's landmark (data/maps/maps.asm + constants/landmark_constants.asm) must equal PKHeX's
    location; Crystal in-game trades come from data/events/npc_trades.asm (PKHeX's GSC list also
    holds Gold / Silver-only trades); engine/pokemon/caught_data.asm SetGiftMonCaughtData writes
    LANDMARK_GIFT (0x7E), caught level 0 and time 0 for trades and OT gifts (Spearow, Shuckle).
  pret/pokefirered, pret/pokeemerald, pret/pokeruby: spot checks (`givemon` / `setwildbattle`
    species + level in data/maps/<map>/scripts.inc, and the map's region_map_section index from
    src/data/region_map/region_map_sections.json equals PKHeX's met location).
  Gen IV / V: PKHeX only (no pret Gen V decompilation; the Gen IV script banks are not mapped to
    met locations in a form this generator can check). Stated in the coverage doc.

Not extracted (no data here; they stay unverified in the editor and are listed as gaps):
  event distributions (Mystery Gift / Wonder Cards, Gen IV 3000+ / Gen V 40000+ locations),
  Pokewalker (binary resource), My Pokemon Ranch, Colosseum / XD bonus discs, Dream World /
  Dream Radar, Gold / Silver (no editor support for their games beyond Crystal's caught data).

Run:  python3 tools/gen_gift_static_encounters.py          (fetches into the gitignored caches)
      python3 tools/gen_gift_static_encounters.py --check  (fails if the committed files differ)
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT_INC = ROOT / "include" / "Integration" / "Encounter" / "GiftStaticEncountersData.inc"
OUT_JSON = ROOT / "docs" / "data" / "gift-static-encounter-sources.json"
PKHEX_CACHE = ROOT / "tools" / ".pkhex_cache"
PRET_CACHE = ROOT / "tools" / ".pret_cache"

PKHEX_PIN = "6042efa241a9925ce57e645329ea7f976deabdd3"
PRET = {
    "pokecrystal": "3c813b0b4d82b39bf62185aceffb76fd6026c70b",
    "pokeemerald": "731ad5bfd6e6f265508d0efcca0ba42f9dcf5881",
    "pokeruby": "5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f",
    "pokefirered": "ce6406d8c2eaf46d543c8a2606de0c3cb1951982",
}

HASHES: dict[str, str] = {}


def fail(msg: str) -> None:
    raise SystemExit(f"gen_gift_static_encounters: {msg}")


def fetch(owner_repo: str, sha: str, rel: str, cache: pathlib.Path) -> str:
    dest = cache / rel
    if not dest.exists():
        url = f"https://raw.githubusercontent.com/{owner_repo}/{sha}/{rel}"
        sys.stderr.write(f"  fetch {url}\n")
        with urllib.request.urlopen(url) as resp:
            data = resp.read()
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
    data = dest.read_bytes()
    HASHES[f"{owner_repo}@{sha}:{rel}"] = hashlib.sha256(data).hexdigest()
    return data.decode("utf-8")


def pkhex(rel: str) -> str:
    return fetch("kwsch/PKHeX", PKHEX_PIN, "PKHeX.Core/" + rel, PKHEX_CACHE / PKHEX_PIN / "PKHeX.Core")


def pret(repo: str, rel: str) -> str:
    return fetch("pret/" + repo, PRET[repo], rel, PRET_CACHE / repo / PRET[repo])


# ---- games -------------------------------------------------------------------------------------
GAME_IDS = {
    "C": ["crystal_gbc"],
    "R": ["ruby_gba"], "S": ["sapphire_gba"], "E": ["emerald_gba"],
    "RS": ["ruby_gba", "sapphire_gba"], "RSE": ["ruby_gba", "sapphire_gba", "emerald_gba"],
    "FR": ["firered_gba"], "LG": ["leafgreen_gba"], "FRLG": ["firered_gba", "leafgreen_gba"],
    "D": ["diamond_nds"], "P": ["pearl_nds"], "Pt": ["platinum_nds"],
    "DP": ["diamond_nds", "pearl_nds"], "DPPt": ["diamond_nds", "pearl_nds", "platinum_nds"],
    "HG": ["heartgold_nds"], "SS": ["soulsilver_nds"], "HGSS": ["heartgold_nds", "soulsilver_nds"],
    "B": ["black_nds"], "W": ["white_nds"], "BW": ["black_nds", "white_nds"],
    "B2": ["black2_nds"], "W2": ["white2_nds"], "B2W2": ["black2_nds", "white2_nds"],
    "GSC": ["crystal_gbc"],
}
GAME_ORDER = ["crystal_gbc", "ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba",
              "diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds", "soulsilver_nds",
              "black_nds", "white_nds", "black2_nds", "white2_nds"]
GEN_OF = {g: (2 if g == "crystal_gbc" else 3 if g.endswith("_gba") else
              4 if g in ("diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds", "soulsilver_nds") else 5)
          for g in GAME_ORDER}

KINDS = ["Gift", "Prize", "Fossil", "Static", "Roaming", "Trade", "EventStatic", "EggGift", "Swarm"]
FLAG_FATEFUL, FLAG_NS_POKEMON, FLAG_CAUGHT_LEVEL_UNRECORDED = 1, 2, 4
# Revived from a fossil when given with a fixed Poke Ball (PKHeX groups them under "// Fossil(s)";
# the DP / Pt rows carry no section comment, so the species set decides).
FOSSIL_SPECIES = {138, 140, 142, 345, 347, 408, 410, 564, 566}

# Owner decision (Nov 2): "Do not label a Pokemon's complete acquisition history Verified simply
# because its encounter exists in the game's scripts." Encounters that need a ticket / key item
# distribution, an event-distributed Pokemon, or a Pokemon from another game: the encounter row is
# script-backed, but the prerequisite is not verifiable from a save, so acquisition evidence is
# Incomplete. (games, species, met location) -> (prerequisite, reason). Rows not listed were reviewed
# as reachable by in-game progress alone; see docs/data/GIFT_STATIC_ENCOUNTER_COVERAGE.md.
EMERALD, RS = ("emerald_gba",), ("ruby_gba", "sapphire_gba")
FRLG, HGSS, BW = ("firered_gba", "leafgreen_gba"), ("heartgold_nds", "soulsilver_nds"), ("black_nds", "white_nds")
PREREQS = [
    (("crystal_gbc",), 251, 14, "TicketOrItem", "GS Ball (event; Japanese Crystal / 3DS Virtual Console only)"),
    (EMERALD, 249, 211, "TicketOrItem", "MysticTicket (Navel Rock)"),
    (EMERALD, 250, 211, "TicketOrItem", "MysticTicket (Navel Rock)"),
    (EMERALD, 386, 200, "TicketOrItem", "AuroraTicket (Birth Island)"),
    (EMERALD, 151, 201, "TicketOrItem", "Old Sea Map (Faraway Island)"),
    (EMERALD, 380, 73, "TicketOrItem", "Eon Ticket (Southern Island)"),
    (EMERALD, 381, 73, "TicketOrItem", "Eon Ticket (Southern Island)"),
    (("ruby_gba",), 380, 73, "TicketOrItem", "Eon Ticket (Southern Island)"),
    (("sapphire_gba",), 381, 73, "TicketOrItem", "Eon Ticket (Southern Island)"),
    (FRLG, 249, 174, "TicketOrItem", "MysticTicket (Navel Rock)"),
    (FRLG, 250, 174, "TicketOrItem", "MysticTicket (Navel Rock)"),
    (FRLG, 386, 187, "TicketOrItem", "AuroraTicket (Birth Island)"),
    (("platinum_nds",), 492, 63, "TicketOrItem", "Oak's Letter (Flower Paradise)"),
    (("platinum_nds",), 491, 79, "TicketOrItem", "Member Card (Newmoon Island)"),
    (("heartgold_nds",), 381, 140, "TicketOrItem", "Enigma Stone (event)"),
    (("soulsilver_nds",), 380, 140, "TicketOrItem", "Enigma Stone (event)"),
    (BW, 494, 62, "TicketOrItem", "Liberty Pass (Liberty Garden)"),
    (BW, 570, 8, "EventPokemon", "event Celebi in the party"),
    (BW, 571, 72, "EventPokemon", "event shiny Raikou / Entei / Suicune in the party"),
    (HGSS, 483, 231, "EventPokemon", "event Arceus in the party (Sinjoh Ruins)"),
    (HGSS, 484, 231, "EventPokemon", "event Arceus in the party (Sinjoh Ruins)"),
    (HGSS, 487, 231, "EventPokemon", "event Arceus in the party (Sinjoh Ruins)"),
    (HGSS, 172, 214, "EventPokemon", "event Pichu in the party (Ilex Forest)"),
    (HGSS, 384, 232, "OtherGame", "Kyogre and Groudon in the party (one from the other version)"),
    (BW, 645, 70, "OtherGame", "Tornadus and Thundurus in the party (one from the other version)"),
    (("diamond_nds", "pearl_nds", "platinum_nds"), 486, 64, "OtherGame",
     "Regirock, Regice and Registeel in the party (not obtainable in this game alone)"),
    (("platinum_nds",), 377, 125, "OtherGame", "Regigigas in the party (itself needs the Regis)"),
    (("platinum_nds",), 378, 124, "OtherGame", "Regigigas in the party (itself needs the Regis)"),
    (("platinum_nds",), 379, 123, "OtherGame", "Regigigas in the party (itself needs the Regis)"),
]
PREREQ_KINDS = ["None", "TicketOrItem", "EventPokemon", "OtherGame"]
CONFIDENCE = ["PretChecked", "PretSpotChecked", "SingleSource"]


# ---- PKHeX C# parsing ----------------------------------------------------------------------------
ARRAY_RE = re.compile(r"(?:public|internal|private)\s+static\s+readonly\s+(\w+)(\[\])?\s+(\w+)\s*=")


def arrays(text: str) -> dict[str, tuple[str, list[tuple[str, str, str]]]]:
    """name -> (element type, [(section comment, entry text, trailing comment)])."""
    out = {}
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        m = ARRAY_RE.search(lines[i])
        if not m:
            i += 1
            continue
        typ, is_array, name = m.group(1), m.group(2), m.group(3)
        if not is_array:   # a single template (CelebiVC)
            entry = lines[i][lines[i].index("=") + 1:]
            body, _, comment = entry.partition("//")
            out[name] = (typ, [("", body.strip().rstrip(";"), comment.strip())])
            i += 1
            continue
        rest = lines[i][lines[i].index("=") + 1:].strip()
        i += 1
        if rest:   # a one-line initializer (string tables, GetAreas / GetAll / DreamWorld loaders)
            continue
        entries, section, buf, depth = [], "", "", 0
        while i < len(lines) and not lines[i].strip().startswith("];"):
            s = lines[i].strip()
            i += 1
            if not buf:
                if s.startswith("//"):
                    if not s.replace(" ", "").startswith("//new("):
                        section = s.lstrip("/").strip()
                    continue
                if not s.startswith("new("):
                    continue
            code, _, comment = s.partition("//")
            buf += " " + code
            depth += code.count("{") - code.count("}") + code.count("(") - code.count(")")
            if depth == 0 and i < len(lines) and lines[i].strip().startswith("{"):
                continue   # initializer on the following lines (HG/SS Spiky-eared Pichu)
            if depth == 0:
                entries.append((section, buf.strip().rstrip(","), comment.strip()))
                buf = ""
        out[name] = (typ, entries)
    return out


def field(entry: str, name: str) -> str | None:
    m = re.search(r"\b" + name + r"\s*=\s*([^,}]+)", entry)
    return m.group(1).strip() if m else None


def num(value: str) -> int:
    return int(value, 0) if value.lower().startswith("0x") else int(value.lstrip("0") or "0")


def positional(entry: str) -> list[str]:
    inner = entry[entry.index("(") + 1:]
    depth, args, cur = 0, [], ""
    for ch in inner:
        if ch == "(":
            depth += 1
        elif ch == ")":
            if depth == 0:
                break
            depth -= 1
        if ch == "," and depth == 0:
            args.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        args.append(cur.strip())
    return args


VERSION_RE = re.compile(r"^(GSC|GS|GD|SI|C|RSE|RS|R|S|E|FRLG|FR|LG|DPPt|DP|D|P|Pt|HGSS|HG|SS|BW|B|W|B2W2|B2|W2)$")


def version_of(args: list[str]) -> str | None:
    for a in args:
        if VERSION_RE.match(a.strip()):
            return a.strip()
    return None


# ---- roamer met-location sets (PKHeX templates) -------------------------------------------------
def roam_sets() -> dict:
    s2 = pkhex("Legality/Encounters/Templates/Gen2/EncounterStatic2.cs")
    m = re.search(r"RoamLocations\s*=\s*0b([01_]+);", s2) or fail("Gen2 RoamLocations")
    bits2 = int(m.group(1).replace("_", ""), 2)
    gen2 = [i for i in range(64) if bits2 >> i & 1]

    s3 = pkhex("Legality/Encounters/Templates/Gen3/EncounterStatic3.cs")
    rse = re.search(r"Version <= GameVersion\.E\)\s*return met is >= (\d+) and <= (\d+);", s3) or fail("Gen3 RSE roam")
    frlg = re.search(r"Route 1-25[^\n]*\n\s*return met is >= (\d+) and <= (\d+);", s3) or fail("Gen3 FRLG roam")
    gen3 = {"rse": list(range(int(rse.group(1)), int(rse.group(2)) + 1)),
            "frlg": list(range(int(frlg.group(1)), int(frlg.group(2)) + 1))}

    s4 = pkhex("Legality/Encounters/Templates/Gen4/EncounterStatic4.cs")
    gen4 = {}
    for region in "SJH":
        first = int((re.search(r"First" + region + r"\s*=\s*(\d+);", s4) or fail("Gen4 First" + region)).group(1))
        locs = set()
        for kind in ("Grass", "Water"):
            mm = re.search(r"Permit" + kind + region + r"\s*=\s*0x([0-9A-Fa-f_]+);", s4) or fail("Gen4 permit")
            bits = int(mm.group(1).replace("_", ""), 16)
            locs |= {first + i for i in range(64) if bits >> i & 1}
        gen4[first] = sorted(locs)

    s5 = pkhex("Legality/Encounters/Templates/Gen5/EncounterStatic5.cs")
    m5 = re.search(r"IsRoamerMet\(ushort location\)\s*\{[^}]*?0b([01]+)", s5, re.S) or fail("Gen5 roam")
    bits5 = int(m5.group(1), 2)
    gen5 = [i for i in range(32) if bits5 >> i & 1]
    return {"gen2": gen2, "gen3": gen3, "gen4": gen4, "gen5": gen5}


def trade_locations() -> dict[int, int]:
    loc = pkhex("Game/Locations/Locations.cs")
    out = {}
    for gen in (2, 3, 4, 5):
        m = re.search(rf"LinkTrade{gen}NPC\s*=\s*(\d+);", loc) or fail(f"LinkTrade{gen}NPC")
        out[gen] = int(m.group(1))
    # The templates must use exactly these constants as the trade's met location.
    for rel, gen in (("Gen2/EncounterTrade2.cs", 2), ("Gen3/EncounterTrade3.cs", 3),
                     ("Gen4/EncounterTrade4PID.cs", 4), ("Gen5/EncounterTrade5BW.cs", 5),
                     ("Gen5/EncounterTrade5B2W2.cs", 5)):
        t = pkhex("Legality/Encounters/Templates/" + rel)
        if f"Locations.LinkTrade{gen}NPC" not in t:
            fail(f"{rel} no longer uses LinkTrade{gen}NPC")
    t4 = pkhex("Legality/Encounters/Templates/Gen4/EncounterTrade4PID.cs")
    if not re.search(r"if \(!IsMetUnset\)\s*return pk\.MetLevel == Level;\s*return pk\.MetLevel >= LevelMin;", t4):
        fail("EncounterTrade4PID.IsMatchLevel changed; re-check Gen IV trade level ranges")
    return out


# ---- pret: Crystal ------------------------------------------------------------------------------
def crystal_constants():
    species = []
    for line in pret("pokecrystal", "constants/pokemon_constants.asm").splitlines():
        m = re.match(r"\s*const\s+(\w+)\s*;\s*([0-9a-fA-F]{2})", line)
        if m and len(species) < 251:
            species.append(m.group(1))
    ids = {name: i + 1 for i, name in enumerate(species)}
    if ids.get("CELEBI") != 251 or ids.get("EEVEE") != 133:
        fail("pokecrystal species constants")
    lm, value, step = {}, 0, 1   # rgbds const_def [start[, step]] / const NAME
    for line in pret("pokecrystal", "constants/landmark_constants.asm").splitlines():
        d = re.match(r"\s*const_def(?:\s+(\$?-?\w+))?(?:,\s*(-?\d+))?", line)
        if d:
            value = int(d.group(1).replace("$", "0x"), 0) if d.group(1) else 0
            step = int(d.group(2)) if d.group(2) else 1
            continue
        m = re.match(r"\s*const\s+(\w+)", line)
        if m:
            lm[m.group(1)] = value
            value += step
    if lm.get("LANDMARK_GOLDENROD_CITY") != 16:
        fail("pokecrystal landmark constants")
    maps = {}
    for line in pret("pokecrystal", "data/maps/maps.asm").splitlines():
        m = re.match(r"\s*map\s+(\w+),\s*\w+,\s*\w+,\s*(LANDMARK_\w+)", line)
        if m:
            maps[m.group(1)] = lm[m.group(2)]
    return ids, lm, maps


# (map, command, SPECIES, level) for every Crystal static / gift / prize row. The generator fails
# unless each PKHeX Crystal row (other than swarm / roamer / egg rows) matches exactly one of these.
CRYSTAL_SCRIPTS = [
    ("ElmsLab", "givepoke", "CHIKORITA", 5), ("ElmsLab", "givepoke", "CYNDAQUIL", 5),
    ("ElmsLab", "givepoke", "TOTODILE", 5), ("BillsFamilysHouse", "givepoke", "EEVEE", 20),
    ("UnionCaveB2F", "loadwildmon", "LAPRAS", 20), ("Route36", "loadwildmon", "SUDOWOODO", 20),
    ("MountMortarB1F", "givepoke", "TYROGUE", 10), ("LakeOfRage", "loadwildmon", "GYARADOS", 30),
    ("TeamRocketBaseB1F", "loadwildmon", "VOLTORB", 23), ("TeamRocketBaseB1F", "loadwildmon", "GEODUDE", 21),
    ("TeamRocketBaseB1F", "loadwildmon", "KOFFING", 21), ("TeamRocketBaseB2F", "loadwildmon", "ELECTRODE", 23),
    ("VermilionCity", "loadwildmon", "SNORLAX", 50),
    ("CeladonGameCornerPrizeRoom", "givepoke", "PIKACHU", 25),
    ("CeladonGameCornerPrizeRoom", "givepoke", "PORYGON", 15),
    ("CeladonGameCornerPrizeRoom", "givepoke", "LARVITAR", 40),
    ("GoldenrodGameCorner", "givepoke", "ABRA", 5), ("GoldenrodGameCorner", "givepoke", "CUBONE", 15),
    ("GoldenrodGameCorner", "givepoke", "WOBBUFFET", 15),
    ("TinTower1F", "loadwildmon", "SUICUNE", 40), ("DragonShrine", "givepoke", "DRATINI", 15),
    ("WhirlIslandLugiaChamber", "loadwildmon", "LUGIA", 60), ("TinTowerRoof", "loadwildmon", "HO_OH", 60),
    ("IlexForest", "loadwildmon", "CELEBI", 30),
]


def crystal_script_rows():
    ids, lm, maps = crystal_constants()
    rows = {}
    for mapname, cmd, sp, level in CRYSTAL_SCRIPTS:
        text = pret("pokecrystal", f"maps/{mapname}.asm")
        if not re.search(rf"^\s*{cmd}\s+{sp},\s*{level}\b", text, re.M):
            fail(f"pokecrystal maps/{mapname}.asm has no `{cmd} {sp}, {level}`")
        if mapname == "IlexForest":
            kind = "EventStatic"   # the GS Ball event (PKHeX CelebiVC)
        elif cmd == "loadwildmon":
            kind = "Static"
        elif "GameCorner" in mapname:
            kind = "Prize"
        else:
            kind = "Gift"
        rows[(ids[sp], level, maps[mapname])] = (kind, f"pret/pokecrystal maps/{mapname}.asm")
    # OT gifts: SetGiftMonCaughtData writes LANDMARK_GIFT, caught level 0 and time 0.
    caught = pret("pokecrystal", "engine/pokemon/caught_data.asm")
    m = re.search(r"SetGiftMonCaughtData:\s*\n\s*xor a\s*\n\s*ld \[hli\], a\s*\n\s*ld a, (LANDMARK_\w+)", caught)
    if not m or lm[m.group(1)] != 0x7E:
        fail("pokecrystal SetGiftMonCaughtData no longer writes level 0 / LANDMARK_GIFT 0x7E")
    gate = pret("pokecrystal", "maps/Route35GoldenrodGate.asm")
    if not re.search(r"givepoke SPEAROW, 10, NO_ITEM, GiftSpearowName, GiftSpearowOTName", gate):
        fail("pokecrystal Route35GoldenrodGate Spearow OT gift")
    shuckle = pret("pokecrystal", "engine/events/shuckle.asm")
    if "ld a, SHUCKLE" not in shuckle or "farcall SetGiftPartyMonCaughtData" not in shuckle or \
            not re.search(r"ld \[wCurPartyLevel\], a", shuckle) or "Level 15 Shuckle" not in shuckle:
        fail("pokecrystal GiveShuckle")
    pret("pokecrystal", "maps/ManiasHouse.asm")
    ot_gifts = {(ids["SPEAROW"], 10): "pret/pokecrystal maps/Route35GoldenrodGate.asm",
                (ids["SHUCKLE"], 15): "pret/pokecrystal engine/events/shuckle.asm"}
    trades = set()
    tradetext = pret("pokecrystal", "data/events/npc_trades.asm")
    for line in tradetext.splitlines():
        mm = re.match(r"\s*npctrade\s+\w+,\s*(\w+),\s*(\w+),", line)
        if mm:
            trades.add(ids[mm.group(2)])
    npc = pret("pokecrystal", "engine/events/npc_trade.asm")
    if "farcall SetGiftPartyMonCaughtData" not in npc:
        fail("pokecrystal npc_trade caught data")
    if len(trades) != 7:
        fail(f"pokecrystal npc_trades.asm: expected 7 trades, got {len(trades)}")
    return rows, ot_gifts, trades


# ---- pret: Gen III spot checks ------------------------------------------------------------------
GEN3_CHECKS = [
    # (repo, game ids, map, command, SPECIES, level, PKHeX location)
    ("pokefirered", ["firered_gba", "leafgreen_gba"], "SilphCo_7F", "givemon", "LAPRAS", 25, 134),
    ("pokefirered", ["firered_gba", "leafgreen_gba"], "CeladonCity_Condominiums_RoofRoom", "givemon", "EEVEE", 25, 94),
    ("pokefirered", ["firered_gba", "leafgreen_gba"], "Route4_PokemonCenter_1F", "givemon", "MAGIKARP", 5, 104),
    ("pokefirered", ["firered_gba", "leafgreen_gba"], "Route12", "setwildbattle", "SNORLAX", 30, 112),
    ("pokefirered", ["firered_gba", "leafgreen_gba"], "PowerPlant", "setwildbattle", "ZAPDOS", 50, 142),
    ("pokefirered", ["firered_gba", "leafgreen_gba"], "PowerPlant", "setwildbattle", "ELECTRODE", 34, 142),
    ("pokeemerald", ["emerald_gba"], "MossdeepCity_StevensHouse", "givemon", "BELDUM", 5, 13),
    ("pokeemerald", ["emerald_gba"], "Route119_WeatherInstitute_2F", "givemon", "CASTFORM", 25, 34),
    ("pokeemerald", ["emerald_gba"], "LittlerootTown_ProfessorBirchsLab", "givemon", "CHIKORITA", 5, 0),
    ("pokeruby", ["ruby_gba", "sapphire_gba"], "MossdeepCity_StevensHouse", "givemon", "BELDUM", 5, 13),
]


def gen3_checks(rows):
    sections = {}
    for repo in ("pokeemerald", "pokefirered"):
        js = json.loads(pret(repo, "src/data/region_map/region_map_sections.json"))
        sections[repo] = {s["id"]: i for i, s in enumerate(js["map_sections"])}
    names = {}
    for repo in ("pokeemerald", "pokefirered", "pokeruby"):
        names[repo] = {}
    done = []
    for repo, games, mapname, cmd, sp, level, location in GEN3_CHECKS:
        script = pret(repo, f"data/maps/{mapname}/scripts.inc")
        if not re.search(rf"^\s*{cmd}\s+SPECIES_{sp},\s*{level}\b", script, re.M):
            fail(f"{repo} {mapname}: no `{cmd} SPECIES_{sp}, {level}`")
        secrepo = "pokeemerald" if repo == "pokeruby" else repo
        mapjson = json.loads(pret(secrepo, f"data/maps/{mapname}/map.json"))
        if sections[secrepo][mapjson["region_map_section"]] != location:
            fail(f"{mapname}: region map section {mapjson['region_map_section']} != {location}")
        for game in games:
            hit = [r for r in rows if r["game"] == game and r["location"] == location and
                   r["minLevel"] == level and r["name"].upper().replace("'", "") == sp]
            if not hit:
                fail(f"PKHeX has no {game} {sp} Lv {level} at {location} (pret {repo} {mapname})")
            for r in hit:
                r["pret"] = f"pret/{repo} data/maps/{mapname}/scripts.inc"
        done.append(f"{repo}:{mapname}:{sp}:{level}:{location}")
    return done


# ---- extraction -------------------------------------------------------------------------------
def species_names() -> dict[int, str]:
    # English names for the cross-checks only (PKHeX resource text).
    text = pkhex("Resources/text/other/en/text_Species_en.txt")
    return {i: n.strip() for i, n in enumerate(text.splitlines())}


def build():
    names = species_names()
    trade_loc = trade_locations()
    roam = roam_sets()
    rows = []

    def add(game, species, location, lo, hi, kind, flags=0, form=0, egg=0, src="", comment=""):
        if not (0 < species <= (251 if GEN_OF[game] == 2 else 386 if GEN_OF[game] == 3 else
                                 493 if GEN_OF[game] == 4 else 649)):
            fail(f"species {species} outside {game}")
        if not (1 <= lo <= hi <= 100):
            fail(f"level {lo}-{hi} for {game} {species}")
        rows.append(dict(game=game, species=species, location=location, minLevel=lo, maxLevel=hi,
                         kind=kind, flags=flags, form=form, egg=egg, src=src, comment=comment,
                         name=names[species], pret=""))

    # ---------------- Crystal ----------------
    crystal_rows, ot_gifts, crystal_trades = crystal_script_rows()
    g2 = arrays(pkhex("Legality/Encounters/Data/Gen2/Encounters2.cs"))
    src2 = "PKHeX Encounters2.cs"
    matched = set()
    for name in ("StaticGSC", "StaticC", "StaticOddEggC", "CelebiVC"):
        _, entries = g2[name]
        for section, entry, comment in entries:
            args = positional(entry)
            species, level = num(args[0]), num(args[1])
            loc = field(entry, "Location")
            location = num(loc) if loc else 0
            if field(entry, "IsEgg") == "true":
                add("crystal_gbc", species, 0, level, level, "EggGift", src=f"{src2} {name}", comment=comment)
                continue
            if "Swarm" in comment:
                add("crystal_gbc", species, location, level, level, "Swarm", src=f"{src2} {name}", comment=comment)
                continue
            if section.startswith("Roamer") and species in (243, 244, 245) and location != 23:
                for route in roam["gen2"]:
                    add("crystal_gbc", species, route, level, level, "Roaming",
                        src=f"{src2} {name} + EncounterStatic2.RoamLocations", comment=comment)
                continue
            key = (species, level, location)
            if key not in crystal_rows:
                fail(f"Crystal row {key} ({comment}) has no pret script check")
            kind, pret_src = crystal_rows[key]
            matched.add(key)
            if any(r["game"] == "crystal_gbc" and (r["species"], r["minLevel"], r["location"]) == key
                   for r in rows):
                continue   # Dratini is listed twice (Dragon's Den quiz moves); one row per place
            add("crystal_gbc", species, location, level, level, kind, src=f"{src2} {name}", comment=comment)
            rows[-1]["pret"] = pret_src
    if matched != set(crystal_rows):
        fail(f"pret Crystal scripts without a PKHeX row: {set(crystal_rows) - matched}")
    _, entries = g2["TradeGift_GSC"]
    seen_trade = set()
    for section, entry, comment in entries:
        args = positional(entry)
        species, level = num(args[2]), num(args[3])
        if (species, level) in ot_gifts:
            add("crystal_gbc", species, trade_loc[2], level, level, "Gift",
                flags=FLAG_CAUGHT_LEVEL_UNRECORDED, src=f"{src2} TradeGift_GSC", comment=comment)
            rows[-1]["pret"] = ot_gifts[(species, level)]
            continue
        if species not in crystal_trades:
            continue   # Gold / Silver-only trade (pret npc_trades.asm)
        seen_trade.add(species)
        add("crystal_gbc", species, trade_loc[2], level, 100, "Trade",
            flags=FLAG_CAUGHT_LEVEL_UNRECORDED, src=f"{src2} TradeGift_GSC", comment=comment)
        rows[-1]["pret"] = "pret/pokecrystal data/events/npc_trades.asm"
    if seen_trade != crystal_trades:
        fail(f"Crystal trades without PKHeX rows: {crystal_trades - seen_trade}")

    # ---------------- Gen III-V statics ----------------
    files = {
        3: [("Legality/Encounters/Data/Gen3/Encounters3RSE.cs", ["StaticRSE", "StaticR", "StaticS", "StaticE"],
             ["TradeGift_RS", "TradeGift_E"]),
            ("Legality/Encounters/Data/Gen3/Encounters3FRLG.cs", ["StaticFRLG", "StaticFR", "StaticLG"],
             ["TradeGift_FRLG", "TradeGift_FR", "TradeGift_LG"])],
        4: [("Legality/Encounters/Data/Gen4/Encounters4DPPt.cs", ["StaticDPPt", "StaticDP", "StaticPt", "StaticD", "StaticP"],
             ["TradeGift_DPPtIngame"]),
            ("Legality/Encounters/Data/Gen4/Encounters4HGSS.cs", ["Encounter_HGSS", "StaticHG", "StaticSS"],
             ["TradeGift_HGSS"])],
        5: [("Legality/Encounters/Data/Gen5/Encounters5BW.cs", ["Encounter_BW", "StaticB", "StaticW"],
             ["TradeGift_BW", "TradeGift_B", "TradeGift_W"]),
            ("Legality/Encounters/Data/Gen5/Encounters5B2W2.cs",
             ["Encounter_B2W2_Regular", "StaticB2", "StaticW2", "Encounter_B2W2_N"],
             ["TradeGift_B2W2"])],
    }
    for gen, specs in files.items():
        for rel, statics, trades in specs:
            parsed = arrays(pkhex(rel))
            short = rel.rsplit("/", 1)[1]
            for name in statics:
                if name not in parsed:
                    fail(f"{short} has no {name}")
                _, entries = parsed[name]
                for section, entry, comment in entries:
                    args = positional(entry)
                    version = version_of(args) or ("B2W2" if name == "Encounter_B2W2_N" else None)
                    if version is None:
                        fail(f"{short} {name}: no version in {entry}")
                    sp = field(entry, "Species")
                    lv = field(entry, "Level")
                    species = num(sp) if sp else num(args[0])
                    level = num(lv) if lv else num(args[1])
                    location = num(field(entry, "Location") or "0")
                    egg = num(field(entry, "EggLocation") or "0")
                    form = num(field(entry, "Form") or "0")
                    gift = field(entry, "FixedBall") == "Ball.Poke"
                    fateful = field(entry, "FatefulEncounter") == "true"
                    flags = (FLAG_FATEFUL if fateful else 0) | (FLAG_NS_POKEMON if name == "Encounter_B2W2_N" else 0)
                    for game in GAME_IDS[version]:
                        src = f"PKHeX {short} {name}"
                        if egg or field(entry, "IsEgg") == "true":
                            add(game, species, location, level, level, "EggGift", flags, form, egg, src, comment)
                        elif field(entry, "IsRoaming") == "true":
                            if gen == 3:
                                routes = roam["gen3"]["frlg" if game in ("firered_gba", "leafgreen_gba") else "rse"]
                                rsrc = "EncounterStatic3.IsMatchLocation"
                            elif gen == 4:
                                routes = roam["gen4"].get(location) or fail(f"Gen4 roamer location {location}")
                                rsrc = "EncounterStatic4.Permit*"
                            else:
                                routes = roam["gen5"]
                                rsrc = "EncounterStatic5.IsRoamerMet"
                            for route in routes:
                                add(game, species, route, level, level, "Roaming", flags, form, 0, f"{src} + {rsrc}", comment)
                        elif fateful or section == "Event":
                            add(game, species, location, level, level, "EventStatic", flags, form, 0, src, comment)
                        elif gift:
                            kind = ("Fossil" if species in FOSSIL_SPECIES else
                                    "Prize" if "Game Corner" in section or "Game Corner" in comment else "Gift")
                            add(game, species, location, level, level, kind, flags, form, 0, src, comment)
                        else:
                            add(game, species, location, level, level, "Static", flags, form, 0, src, comment)
            for name in trades:
                if name not in parsed:
                    fail(f"{short} has no {name}")
                _, entries = parsed[name]
                for section, entry, comment in entries:
                    args = positional(entry)
                    version = version_of(args) or fail(f"{short} {name}: no version in {entry}")
                    sp, lv = field(entry, "Species"), field(entry, "Level")
                    if sp:
                        species, level = num(sp), num(lv)
                    else:   # (names, index, version, pid, species, level)
                        species, level = num(args[4]), num(args[5])
                    form = num(field(entry, "Form") or "0")
                    met = field(entry, "MetLocation")
                    for game in GAME_IDS[version]:
                        src = f"PKHeX {short} {name}"
                        if met:   # HG/SS Webster's Spearow / Kirk's Shuckle: given at a place, at
                                  # exactly this level (EncounterTrade4PID.IsMatchLevel: MetLevel == Level)
                            add(game, species, num(met), level, level, "Gift", 0, form, 0, src, comment)
                        elif gen in (3, 4):
                            # The traded Pokemon arrives at the offered Pokemon's level:
                            # EncounterTrade3 LevelMin = Level, LevelMax = 100;
                            # EncounterTrade4PID.IsMatchLevel (met unset): MetLevel >= LevelMin.
                            add(game, species, trade_loc[gen], level, 100, "Trade", 0, form, 0, src, comment)
                        else:     # EncounterTrade5BW / 5B2W2: MetLevel == Level
                            add(game, species, trade_loc[gen], level, level, "Trade", 0, form, 0, src, comment)

    # One row per (game, species, location, level range, kind, flags, form); the B2/W2 lists repeat
    # the single-version trades (TradeGift_B2W2 already holds Petilil / Cottonee).
    unique, seen = [], set()
    for r in rows:
        key = (r["game"], r["species"], r["location"], r["minLevel"], r["maxLevel"], r["kind"], r["flags"], r["form"], r["egg"])
        if key in seen:
            continue
        seen.add(key)
        unique.append(r)
    rows = unique
    checks = gen3_checks(rows)
    for r in rows:
        r["prereq"], r["reason"] = "None", ""
        for games, sp, loc, kind, reason in PREREQS:
            if r["game"] in games and r["species"] == sp and r["location"] == loc and r["kind"] != "EggGift":
                r["prereq"], r["reason"] = kind, reason
        # Source confidence: a row matched to a pret script is cross-checked (every Crystal static /
        # gift / prize / trade row; the Gen III spot checks); every other row is PKHeX alone.
        r["confidence"] = ("PretChecked" if r["pret"] and r["game"] == "crystal_gbc" else
                           "PretSpotChecked" if r["pret"] else "SingleSource")
    for games, sp, loc, kind, reason in PREREQS:
        for g in games:
            if not any(r["game"] == g and r["species"] == sp and r["location"] == loc for r in rows):
                fail(f"prerequisite row {g} {sp} @ {loc} has no data row")
    rows.sort(key=lambda r: (GAME_ORDER.index(r["game"]), r["species"], r["location"], r["minLevel"],
                             r["maxLevel"], KINDS.index(r["kind"]), r["form"]))
    return rows, checks


def render(rows) -> str:
    lines = [
        "// GENERATED by tools/gen_gift_static_encounters.py; do not edit.",
        f"// Primary source: kwsch/PKHeX {PKHEX_PIN} (Legality/Encounters/Data/Gen2-5 static / trade",
        "// tables; trade met locations, level ranges and roamer route sets from the Templates).",
        f"// Crystal rows matched to pret/pokecrystal {PRET['pokecrystal']} map scripts / npc_trades.asm.",
        f"// Gen III spot checks: pret/pokefirered {PRET['pokefirered']}, pret/pokeemerald",
        f"// {PRET['pokeemerald']}, pret/pokeruby {PRET['pokeruby']}.",
        "// Files and sha256: docs/data/gift-static-encounter-sources.json. Coverage and gaps:",
        "// docs/data/GIFT_STATIC_ENCOUNTER_COVERAGE.md. Event distributions are NOT here (no data).",
        "// {game, species, met location, min level, max level, kind, flags, form, egg location,",
        "//  access prerequisite, source confidence}",
        "inline constexpr Row kRows[] = {",
    ]
    for r in rows:
        lines.append('    {"%s", %d, %d, %d, %d, Kind::%s, %d, %d, %d, Prerequisite::%s, Confidence::%s},' % (
            r["game"], r["species"], r["location"], r["minLevel"], r["maxLevel"], r["kind"],
            r["flags"], r["form"], r["egg"], r["prereq"], r["confidence"]))
    lines.append("};")
    lines.append("// Why each prerequisite row's acquisition evidence is Incomplete (same order as kRows).")
    lines.append("inline constexpr PrerequisiteReason kPrerequisiteReasons[] = {")
    for r in rows:
        if r["prereq"] != "None":
            lines.append('    {"%s", %d, %d, "%s"},' % (r["game"], r["species"], r["location"], r["reason"]))
    lines.append("};")
    return "\n".join(lines) + "\n"


def manifest(rows, checks) -> str:
    counts = {}
    for g in GAME_ORDER:
        per = {}
        for r in rows:
            if r["game"] == g:
                per[r["kind"]] = per.get(r["kind"], 0) + 1
        conf, pre = {}, {}
        for r in rows:
            if r["game"] == g:
                conf[r["confidence"]] = conf.get(r["confidence"], 0) + 1
                if r["prereq"] != "None":
                    pre[r["prereq"]] = pre.get(r["prereq"], 0) + 1
        counts[g] = {"rows": sum(per.values()), "by_kind": dict(sorted(per.items())),
                     "by_confidence": dict(sorted(conf.items())), "prerequisite_rows": dict(sorted(pre.items())),
                     "species": len({r["species"] for r in rows if r["game"] == g})}
    doc = {
        "generator": "tools/gen_gift_static_encounters.py",
        "sources": {"kwsch/PKHeX": PKHEX_PIN, **{"pret/" + k: v for k, v in PRET.items()}},
        "inputs_sha256": dict(sorted(HASHES.items())),
        "pret_crosschecks": {
            "crystal": "every static / gift / prize / OT-gift / trade row (map script + landmark)",
            "gen3": checks,
        },
        "counts": counts,
        "total_rows": len(rows),
    }
    return json.dumps(doc, indent=2, sort_keys=False) + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    rows, checks = build()
    inc, man = render(rows), manifest(rows, checks)
    if args.check:
        if OUT_INC.read_text() != inc or OUT_JSON.read_text() != man:
            fail("committed output differs; rerun without --check")
        print("gen_gift_static_encounters: up to date")
        return
    OUT_INC.write_text(inc)
    OUT_JSON.write_text(man)
    print(f"wrote {OUT_INC.relative_to(ROOT)} ({len(rows)} rows), {OUT_JSON.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
