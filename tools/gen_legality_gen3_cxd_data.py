#!/usr/bin/env python3
"""Generate the Pokemon Colosseum / XD (GameCube, PK3 origin version 15) legality tables
from a pinned PKHeX checkout.

The Gen III analyzer (src/Legality/Gen3Legality.cpp) judges GameCube-origin records with
PKHeX.Core's own hand-written encounter tables, quoted here from:

  Legality/Encounters/Data/Gen3/Encounters3Colo.cs        Colosseum starters, Plusle gift,
                                                           shadow Pokemon, e-Reader shadows,
                                                           Duking's trainer names
  Legality/Encounters/Data/Gen3/Encounters3ColoShadow.cs  Colosseum NPC team locks
  Legality/Encounters/Data/Gen3/Encounters3ColoTeams.cs   Colosseum team permutations
  Legality/Encounters/Data/Gen3/Encounters3XD.cs          XD gifts, trades, shadows, Poke Spots,
                                                           Hordel / Zaprong names
  Legality/Encounters/Data/Gen3/Encounters3XDShadow.cs    XD NPC team locks
  Legality/Encounters/Data/Gen3/Encounters3XDTeams.cs     XD team permutations

PKHeX is GPL-3.0; PokeBank NX is AGPL-3.0, which GPL-3.0 section 13 permits
combining with. The generated files carry that attribution.

Usage:
    python3 tools/gen_legality_gen3_cxd_data.py --pkhex /path/to/PKHeX [--check]

Writes include/Legality/Gen3LegalityDataCxd.h and src/Legality/Gen3LegalityDataCxd.cpp.
Output is deterministic; --check fails if the committed files differ.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))  # python3 -I drops the script directory
import gen_legality_gen3_data as base  # noqa: E402  (parsing helpers and the pinned commit)

REPO = Path(__file__).resolve().parent.parent
DATA = "Legality/Encounters/Data/Gen3/"
BALLS = {"Ball.Poke": 4}


def die(msg):
    base.die(msg)


def cs_string_list(source, name):
    """A `string[] Name = [ ... ];` array of string literals (string.Empty allowed)."""
    m = re.search(r"string\[\]\s+" + re.escape(name) + r"\s*=\s*\[([^\]]*)\];", source)
    if not m:
        die(f"string array {name} not found")
    return [base.cs_string(x, {}) for x in base.split_top(m.group(1))]


def moves_of(props):
    return base.moveset(props["Moves"]) if "Moves" in props else [0, 0, 0, 0]


def parse_team_locks(source, order):
    """`public static readonly TeamLock Name = new(species, [NPCLockDetail,] [ new NPCLock(...), ... ]);`
    in definition order (the detail comment is not needed). NPCLock(s, n, g, r) is a non-shadow lock; NPCLock(s[, true]) a
    shadow lock (state 1, or 2 when already seen)."""
    text = base.strip_comments(source)
    teams = {}
    for name, species, body in re.findall(
            r"public static readonly TeamLock\s+(\w+)\s*=\s*new\(\s*(\d+)\s*,(?:\s*\w+\s*,)?\s*\[(.*?)\]\s*\);", text, re.S):
        locks = []
        for part in base.split_top(body):
            m = re.fullmatch(r"new NPCLock\((.*)\)", part.strip(), re.S)
            if not m:
                die(f"unparsed NPCLock in {name}: {part[:60]!r}")
            args = base.split_top(m.group(1))
            if len(args) == 4:
                locks.append(dict(species=base.cs_int(args[0]), nature=base.cs_int(args[1]),
                                  gender=base.cs_int(args[2]), ratio=base.cs_int(args[3]), state=0))
            elif len(args) == 1:
                locks.append(dict(species=base.cs_int(args[0]), nature=0, gender=0, ratio=0, state=1))
            elif len(args) == 2 and args[1] in ("true", "false"):
                locks.append(dict(species=base.cs_int(args[0]), nature=0, gender=0, ratio=0,
                                  state=2 if args[1] == "true" else 1))
            else:
                die(f"unexpected NPCLock arity in {name}: {part!r}")
        if name in teams:
            die(f"duplicate TeamLock {name}")
        teams[name] = dict(name=name, species=int(species), locks=locks)
        order.append(name)
    if not teams:
        die("no TeamLock definitions found")
    return teams


def parse_team_sets(source, prefix, team_index):
    """`public static readonly TeamLock[] Name = [A, B];` arrays, in definition order."""
    text = base.strip_comments(source)
    sets = {}
    for name, body in re.findall(r"public static readonly TeamLock\[\]\s+(\w+)\s*=\s*\[(.*?)\];", text, re.S):
        members = [x.strip() for x in body.split(",") if x.strip()]
        for m in members:
            if m not in team_index:
                die(f"team set {name} references unknown TeamLock {m}")
        sets[name] = dict(name=f"{prefix}.{name}", members=[team_index[m] for m in members])
    if "First" not in sets or sets["First"]["members"]:
        die(f"{prefix} team sets must define an empty First")
    return sets


def parse_shadows(source, name, sets, set_index, allow_ball):
    out = []
    for args, p in base.cs_entries(base.cs_block(source, name)):
        if len(args) != 3:
            die(f"{name}: expected (index, gauge, teams), got {args}")
        known = {"Species", "Level", "Moves", "Location"} | ({"FixedBall"} if allow_ball else set())
        if set(p) - known:
            die(f"{name}: unhandled properties {sorted(set(p) - known)}")
        if args[2] not in sets:
            die(f"{name}: unknown team set {args[2]}")
        out.append(dict(index=base.cs_int(args[0]), gauge=base.cs_int(args[1]), team_set=set_index[args[2]],
                        species=base.cs_int(p["Species"]), level=base.cs_int(p["Level"]),
                        location=base.cs_int(p["Location"]), moves=moves_of(p),
                        ball=BALLS[p["FixedBall"]] if "FixedBall" in p else 0))
    return out


def generate(pkhex):
    core = pkhex / "PKHeX.Core"
    inputs = {}

    def read_cs(rel):
        p = core / rel
        if not p.is_file():
            die(f"missing {rel}")
        inputs[rel] = p
        return p.read_text(encoding="utf-8-sig")

    colo = read_cs(DATA + "Encounters3Colo.cs")
    colo_shadow = read_cs(DATA + "Encounters3ColoShadow.cs")
    colo_teams = read_cs(DATA + "Encounters3ColoTeams.cs")
    xd = read_cs(DATA + "Encounters3XD.cs")
    xd_shadow = read_cs(DATA + "Encounters3XDShadow.cs")
    xd_teams = read_cs(DATA + "Encounters3XDTeams.cs")

    # NPC team locks: Colosseum teams first, then XD teams, each in definition order.
    order = []
    teams = parse_team_locks(colo_shadow, order)
    teams.update(parse_team_locks(xd_shadow, order))
    team_index = {name: i for i, name in enumerate(order)}
    team_list = [teams[name] for name in order]

    colo_sets = parse_team_sets(colo_teams, "Colo", team_index)
    xd_sets = parse_team_sets(xd_teams, "XD", team_index)
    set_list, colo_set_index, xd_set_index = [], {}, {}
    for name, s in colo_sets.items():
        colo_set_index[name] = len(set_list)
        set_list.append(s)
    for name, s in xd_sets.items():
        xd_set_index[name] = len(set_list)
        set_list.append(s)

    # Colosseum: starters, Duking's names, the Plusle gift, shadows and e-Reader shadows.
    starters = []
    for args, p in base.cs_entries(base.cs_block(colo, "Starters")):
        if set(p) - {"Moves"}:
            die(f"Starters: unhandled properties {sorted(p)}")
        starters.append(dict(species=base.cs_int(args[0]), level=base.cs_int(args[1]), moves=moves_of(p)))
    duking = cs_string_list(colo, "TrainerNameDuking")
    m = re.search(r'const string TrainerNameDukingSpanish4 = "([^"]*)";', colo)
    if not m:
        die("TrainerNameDukingSpanish4 not found")
    duking_spanish4 = m.group(1)
    names = {"TrainerNameDuking": ("kTrainerNameDuking", duking)}
    gifts = []
    for args, p in base.cs_entries(base.cs_block(colo, "Gifts")):
        known = {"Location", "TID16", "OriginalTrainerGender", "Moves"}
        if set(p) - known or len(args) != 4 or args[3] != "GameVersion.CXD":
            die(f"Colosseum Gifts: unexpected template {args} {sorted(p)}")
        gifts.append(dict(species=base.cs_int(args[0]), level=base.cs_int(args[1]), names=names[args[2]][0],
                          location=base.cs_int(p["Location"]), tid=base.cs_int(p["TID16"]),
                          ot_gender=base.cs_int(p["OriginalTrainerGender"]), moves=moves_of(p)))
    colo_shadows = parse_shadows(colo, "Shadow", colo_sets, colo_set_index, allow_ball=False)
    ereader = parse_shadows(colo, "EReader", colo_sets, colo_set_index, allow_ball=False)
    for e in ereader:
        if e["location"] != 128:
            die("e-Reader shadows are expected at location 128 (EncounterShadow3Colo.IsEReader)")

    # XD: gifts (starter Eevee, Hordel's Johto starters), trades, shadows, Poke Spots.
    xd_gifts = []
    for args, p in base.cs_entries(base.cs_block(xd, "Gifts")):
        if set(p) - {"Location", "Moves"}:
            die(f"XD Gifts: unhandled properties {sorted(p)}")
        xd_gifts.append(dict(species=base.cs_int(args[0]), level=base.cs_int(args[1]),
                             location=base.cs_int(p["Location"]), moves=moves_of(p)))
    hordel = cs_string_list(xd, "Hordel")
    zaprong = cs_string_list(xd, "Zaprong")
    names["Hordel"] = ("kTrainerNameHordel", hordel)
    names["Duking"] = ("kTrainerNameDuking", duking)
    nicknames = {"Zaprong": ("kNicknameZaprong", zaprong)}
    trades = []
    for args, p in base.cs_entries(base.cs_block(xd, "Trades")):
        if set(p) - {"Location", "TID16", "Moves"} or len(args) not in (3, 4):
            die(f"XD Trades: unexpected template {args} {sorted(p)}")
        trades.append(dict(species=base.cs_int(args[0]), level=base.cs_int(args[1]), trainer=names[args[2]][0],
                           nickname=nicknames[args[3]][0] if len(args) == 4 else None,
                           location=base.cs_int(p["Location"]), tid=base.cs_int(p["TID16"]), moves=moves_of(p)))
    xd_shadows = parse_shadows(xd, "Shadow", xd_sets, xd_set_index, allow_ball=True)
    spots = []
    for args, p in base.cs_entries(base.cs_block(xd, "Slots")):
        if p or len(args) != 7:
            die(f"XD Slots: unexpected area {args} {sorted(p)}")
        v = [base.cs_int(a) for a in args]
        # EncounterArea3XD(loc, s0, l0, s1, l1, s2, l2): every slot's minimum level is 10.
        spots.append(dict(location=v[0], slots=[(v[1], 10, v[2]), (v[3], 10, v[4]), (v[5], 10, v[6])]))

    for lst in (duking, hordel, zaprong):
        if len(lst) != 8:
            die("trainer / nickname tables must have 8 language entries")
    for sh in colo_shadows + ereader + xd_shadows:
        if sh["index"] > 255 or sh["gauge"] > 0xFFFF or sh["location"] > 255 or sh["level"] > 100:
            die(f"shadow row out of range: {sh}")

    return dict(pin=base.PINNED_PKHEX, inputs=inputs, teams=team_list, sets=set_list, starters=starters,
                duking=duking, duking_spanish4=duking_spanish4, hordel=hordel, zaprong=zaprong, gifts=gifts,
                colo_shadows=colo_shadows, ereader=ereader, xd_gifts=xd_gifts, trades=trades,
                xd_shadows=xd_shadows, spots=spots)


def render(v):
    pin = v["pin"]
    header = ("// GENERATED by tools/gen_legality_gen3_cxd_data.py -- do not edit by hand.\n"
              f"// Source: kwsch/PKHeX@{pin} (PKHeX.Core Colosseum / XD encounter tables, GPL-3.0).\n"
              "// Used under GPL-3.0 section 13 alongside this AGPL-3.0 project.\n")
    teams, sets = v["teams"], v["sets"]
    lock_count = sum(len(t["locks"]) for t in teams)
    member_count = sum(len(s["members"]) for s in sets)
    u16 = base.u16str

    h = [header, "#pragma once", "", "#include <cstddef>", "#include <cstdint>", "",
         "namespace Legality::Gen3DataCxd {", "",
         f'inline constexpr const char* kPkhexCommit = "{pin}";', "",
         "/// NPCLock (Legality/RNG/CXD/NPCLock.cs): a non-shadow team member fixes nature and",
         "/// gender (state 0); a shadow member is state 1, or 2 once already seen (fewer frames).",
         "struct NpcLock { uint16_t species; uint8_t nature; uint8_t gender; uint8_t ratio; uint8_t state; };",
         f"inline constexpr std::size_t kNpcLockCount = {lock_count};",
         "extern const NpcLock kNpcLocks[];",
         "/// TeamLock (Legality/RNG/CXD/TeamLock.cs): the shadow species and its trainer's party,",
         "/// in the order the game generates them; locks are kNpcLocks[lockStart .. lockStart + lockCount).",
         "struct TeamLock { uint16_t species; uint16_t lockStart; uint8_t lockCount; const char* name; };",
         f"inline constexpr std::size_t kTeamLockCount = {len(teams)};",
         "extern const TeamLock kTeamLocks[]; // Encounters3ColoShadow.cs then Encounters3XDShadow.cs",
         "/// Team permutations (Encounters3ColoTeams.cs, Encounters3XDTeams.cs): a shadow template",
         "/// names one set; any member team may precede the shadow (LockFinder.IsAllShadowLockValid).",
         "struct TeamSet { uint16_t start; uint8_t count; const char* name; };",
         f"inline constexpr std::size_t kTeamSetCount = {len(sets)};",
         f"inline constexpr std::size_t kTeamSetMemberCount = {member_count};",
         "extern const uint16_t kTeamSetMembers[]; // indices into kTeamLocks",
         "extern const TeamSet kTeamSets[];", "",
         "/// EncounterShadow3Colo / EncounterShadow3XD: Index and Gauge as in PKHeX, fixed level,",
         "/// met location, FixedBall (0 none, 4 Poke Ball), team set index and the initial moveset.",
         "struct Shadow { uint8_t index; uint16_t gauge; uint16_t species; uint8_t level; uint8_t location;",
         "                uint8_t fixedBall; uint8_t teamSet; uint16_t moves[4]; };",
         f"inline constexpr std::size_t kColoShadowCount = {len(v['colo_shadows'])};",
         "extern const Shadow kColoShadows[]; // Encounters3Colo.Shadow",
         f"inline constexpr std::size_t kColoEReaderCount = {len(v['ereader'])};",
         "extern const Shadow kColoEReader[]; // Encounters3Colo.EReader (location 128, Japanese only)",
         f"inline constexpr std::size_t kXdShadowCount = {len(v['xd_shadows'])};",
         "extern const Shadow kXdShadows[]; // Encounters3XD.Shadow", "",
         "/// EncounterStarter3Colo: Espeon / Umbreon, male, Poke Ball, met location 254, never shiny.",
         "struct Starter { uint16_t species; uint8_t level; uint16_t moves[4]; };",
         f"inline constexpr std::size_t kColoStarterCount = {len(v['starters'])};",
         "extern const Starter kColoStarters[];",
         "inline constexpr uint8_t kColoStarterLocation = 254;", "",
         "/// EncounterGift3Colo with Version CXD (Duking's Plusle): fixed OT per language (0..7).",
         "struct Gift { uint16_t species; uint8_t level; uint8_t location; uint16_t tid; uint8_t otGender;",
         "              uint16_t moves[4]; const char16_t* const* names; };",
         f"inline constexpr std::size_t kColoGiftCount = {len(v['gifts'])};",
         "extern const Gift kColoGifts[]; // Encounters3Colo.Gifts", "",
         "/// EncounterStatic3XD: starter Eevee (location 0) and Hordel's Johto starters (16); Poke Ball,",
         "/// fateful, shiny possible.",
         "struct XdStatic { uint16_t species; uint8_t level; uint8_t location; uint16_t moves[4]; };",
         f"inline constexpr std::size_t kXdGiftCount = {len(v['xd_gifts'])};",
         "extern const XdStatic kXdGifts[]; // Encounters3XD.Gifts", "",
         "/// EncounterTrade3XD: fixed TID (SID is the player's), fixed OT per language, optional",
         "/// fixed nickname per language (nullptr: the species name), Poke Ball, fateful.",
         "struct XdTrade { uint16_t species; uint8_t level; uint8_t location; uint16_t tid; uint16_t moves[4];",
         "                 const char16_t* const* trainerNames; const char16_t* const* nicknames; };",
         f"inline constexpr std::size_t kXdTradeCount = {len(v['trades'])};",
         "extern const XdTrade kXdTrades[]; // Encounters3XD.Trades", "",
         "/// EncounterArea3XD / EncounterSlot3XD: three Poke Spots (Rock 90, Oasis 91, Cave 92), three",
         "/// slots each (slot number = position), levels 10..max, fateful, any wild ball.",
         "struct PokeSpotSlot { uint16_t species; uint8_t levelMin; uint8_t levelMax; };",
         "struct PokeSpotArea { uint8_t location; PokeSpotSlot slots[3]; };",
         f"inline constexpr std::size_t kPokeSpotCount = {len(v['spots'])};",
         "extern const PokeSpotArea kPokeSpots[]; // Encounters3XD.Slots", "",
         "/// Trainer and nickname tables indexed by language id 0..7 (empty: not distributed).",
         "extern const char16_t* const kTrainerNameDuking[8]; // Encounters3Colo.TrainerNameDuking",
         "extern const char16_t* const kTrainerNameHordel[8]; // Encounters3XD.Hordel",
         "extern const char16_t* const kNicknameZaprong[8];   // Encounters3XD.Zaprong",
         "/// Spanish Duking after a Gen III -> IV transfer (StringConverter345.TransferGlyphs34).",
         f"inline constexpr const char16_t* kTrainerNameDukingSpanish4 = {u16(v['duking_spanish4'])};", "",
         "struct SourcePin { const char* path; const char* sha256; };",
         f"inline constexpr std::size_t kSourcePinCount = {len(v['inputs'])};",
         "extern const SourcePin kSourcePins[];", "",
         "} // namespace Legality::Gen3DataCxd", ""]

    c = [header, '#include "Legality/Gen3LegalityDataCxd.h"', "", "namespace Legality::Gen3DataCxd {", ""]
    c.append("const NpcLock kNpcLocks[] = {")
    start = 0
    team_rows = []
    for t in teams:
        c.append(f"    // {t['name']} ({t['species']})")
        for lk in t["locks"]:
            c.append(f"    {{{lk['species']}, {lk['nature']}, {lk['gender']}, {lk['ratio']}, {lk['state']}}},")
        team_rows.append(f'    {{{t["species"]}, {start}, {len(t["locks"])}, "{t["name"]}"}},')
        start += len(t["locks"])
    c += ["};", "", "const TeamLock kTeamLocks[] = {"] + team_rows + ["};", ""]
    c.append("const uint16_t kTeamSetMembers[] = {")
    set_rows, start = [], 0
    for s in sets:
        if s["members"]:
            c.append("    " + ", ".join(str(m) for m in s["members"]) + f",  // {s['name']}")
        set_rows.append(f'    {{{start}, {len(s["members"])}, "{s["name"]}"}},')
        start += len(s["members"])
    if member_count == 0:
        c.append("    0,")
    c += ["};", "", "const TeamSet kTeamSets[] = {"] + set_rows + ["};", ""]

    def shadow_rows(name, rows):
        out = [f"const Shadow {name}[] = {{"]
        for s in rows:
            mv = s["moves"]
            out.append(f"    {{{s['index']}, {s['gauge']}, {s['species']}, {s['level']}, {s['location']}, "
                       f"{s['ball']}, {s['team_set']}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}}},")
        return out + ["};", ""]

    c += shadow_rows("kColoShadows", v["colo_shadows"])
    c += shadow_rows("kColoEReader", v["ereader"])
    c += shadow_rows("kXdShadows", v["xd_shadows"])
    c.append("const Starter kColoStarters[] = {")
    for s in v["starters"]:
        mv = s["moves"]
        c.append(f"    {{{s['species']}, {s['level']}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}}},")
    c += ["};", ""]
    for name, lst in (("kTrainerNameDuking", v["duking"]), ("kTrainerNameHordel", v["hordel"]),
                      ("kNicknameZaprong", v["zaprong"])):
        c.append(f"const char16_t* const {name}[8] = {{")
        c.append("    " + ", ".join(u16(x) for x in lst) + ",")
        c += ["};", ""]
    c.append("const Gift kColoGifts[] = {")
    for g in v["gifts"]:
        mv = g["moves"]
        c.append(f"    {{{g['species']}, {g['level']}, {g['location']}, {g['tid']}, {g['ot_gender']}, "
                 f"{{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}, {g['names']}}},")
    c += ["};", "", "const XdStatic kXdGifts[] = {"]
    for g in v["xd_gifts"]:
        mv = g["moves"]
        c.append(f"    {{{g['species']}, {g['level']}, {g['location']}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}}},")
    c += ["};", "", "const XdTrade kXdTrades[] = {"]
    for t in v["trades"]:
        mv = t["moves"]
        c.append(f"    {{{t['species']}, {t['level']}, {t['location']}, {t['tid']}, "
                 f"{{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}, {t['trainer']}, {t['nickname'] or 'nullptr'}}},")
    c += ["};", "", "const PokeSpotArea kPokeSpots[] = {"]
    for a in v["spots"]:
        slots = ", ".join(f"{{{s}, {lo}, {hi}}}" for s, lo, hi in a["slots"])
        c.append(f"    {{{a['location']}, {{{slots}}}}},")
    c += ["};", "", "const SourcePin kSourcePins[] = {"]
    for rel, path in sorted(v["inputs"].items()):
        c.append(f'    {{"PKHeX.Core/{rel}", "{base.sha256(path)}"}},')
    c += ["};", "", "} // namespace Legality::Gen3DataCxd", ""]
    return "\n".join(h), "\n".join(c)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pkhex", required=True, type=Path)
    ap.add_argument("--check", action="store_true", help="fail if the committed output differs")
    args = ap.parse_args()
    try:
        head = subprocess.run(["git", "-C", str(args.pkhex), "rev-parse", "HEAD"], check=True,
                              capture_output=True, text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        die(f"{args.pkhex} is not a git checkout")
    if head != base.PINNED_PKHEX:
        die(f"PKHeX checkout is at {head}, expected pinned {base.PINNED_PKHEX}")
    header, source = render(generate(args.pkhex))
    targets = ((REPO / "include/Legality/Gen3LegalityDataCxd.h", header),
               (REPO / "src/Legality/Gen3LegalityDataCxd.cpp", source))
    if args.check:
        stale = [str(p.relative_to(REPO)) for p, text in targets
                 if not p.is_file() or p.read_text(encoding="utf-8") != text]
        if stale:
            die("generated Colosseum/XD tables are stale: " + ", ".join(stale))
        print("Colosseum/XD legality tables match the pinned PKHeX sources")
        return
    for path, text in targets:
        path.write_text(text, encoding="utf-8")
        print(f"wrote {path.relative_to(REPO)}")


if __name__ == "__main__":
    main()
