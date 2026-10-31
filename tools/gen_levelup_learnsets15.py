#!/usr/bin/env python3
"""Generate include/Pokemon/LevelUpLearnsets15Data.inc: native level-up learnsets for the
20 Generation I-V games, as used by SpeciesMoveDefaults (R2-BUG-004).

This is a data-generation tool, not a build step. The generated .inc is committed so
host and devkitA64 builds stay offline. Every move comes from a pinned source:

  Gen I   pret/pokered  data/pokemon/evos_moves.asm + base_stats/*.asm ("level 1 learnset")
          pret/pokeyellow (same layout; Yellow differs, e.g. Pikachu)
  Gen II  pret/pokegold, pret/pokecrystal  data/pokemon/evos_attacks.asm
  Gen III pret/pokeruby, pret/pokeemerald, pret/pokefirered
          src/data/pokemon/level_up_learnsets.h (+ _pointers.h). pokefirered builds both
          FireRed and LeafGreen; its #if defined(FIRERED)/(LEAFGREEN) blocks are evaluated
          per game (Deoxys form, Dugtrio level-1 order).
  Gen IV  PKHeX Resources/byte/levelup/lvlmove_dp / _pt / _hgss.pkl
  Gen V   PKHeX Resources/byte/levelup/lvlmove_bw / _b2w2.pkl
          (forms through personal_* FormStatsIndex, PKHeX PersonalInfo4 0x2A / PersonalInfo5 0x1C)

Gen I, II and III are additionally cross-checked against PKHeX's own resources for the
same games (lvlmove_rb/y/gs/c/rs/e/fr/lg; Gen I base moves against personal_rb/y 0x0F-0x12);
any disagreement aborts generation instead of picking one silently.

Gen I base moves (the base-stats "level 1 learnset") are emitted as level-0 entries ahead
of the level-up list: pret WriteMonMoves starts from them before walking evos_moves.
No other table contains level-0 entries (asserted).

Usage:  python3 tools/gen_levelup_learnsets15.py
Sources are fetched into tools/.pret_cache/<repo>/<sha>/ (gitignored) and tools/.pkhex_cache/.
"""
from __future__ import annotations

import pathlib
import re
import struct
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from pkhex_source import pkhex_path, _REF as PKHEX_REF  # noqa: E402

OUT = ROOT / "include" / "Pokemon" / "LevelUpLearnsets15Data.inc"
PRET_CACHE = ROOT / "tools" / ".pret_cache"

PRET = {
    "pokered": "af519899719f0754965776faac0e836a3b906e6d",
    "pokeyellow": "bfe2fd65b1a8ac4821f4235970ccd7204faf77fb",
    "pokegold": "ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8",
    "pokecrystal": "3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6",
    "pokeruby": "5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f",
    "pokeemerald": "731ad5bfd6e6f265508d0efcca0ba42f9dcf5881",
    "pokefirered": "037335f4c725d7c9aecdac87066f2002b4bd7e14",
}

# Table order is the C++ enum order (LevelUpTable15).
TABLES = ["RB", "Y", "GS", "C", "RS", "E", "FR", "LG", "DP", "PT", "HGSS", "BW", "B2W2"]
MAX_SPECIES = {"RB": 151, "Y": 151, "GS": 251, "C": 251, "RS": 386, "E": 386, "FR": 386,
               "LG": 386, "DP": 493, "PT": 493, "HGSS": 493, "BW": 649, "B2W2": 649}
MAX_MOVE = {1: 165, 2: 251, 3: 354, 4: 467, 5: 559}
GEN = {"RB": 1, "Y": 1, "GS": 2, "C": 2, "RS": 3, "E": 3, "FR": 3, "LG": 3,
       "DP": 4, "PT": 4, "HGSS": 4, "BW": 5, "B2W2": 5}


def fail(msg: str) -> None:
    raise SystemExit(f"gen_levelup_learnsets15: {msg}")


def pret(repo: str) -> pathlib.Path:
    sha = PRET[repo]
    dest = PRET_CACHE / repo / sha
    if (dest / ".git").exists():
        return dest
    dest.mkdir(parents=True, exist_ok=True)
    url = f"https://github.com/pret/{repo}.git"
    sys.stderr.write(f"  fetch {url} @ {sha}\n")
    for cmd in (["git", "init", "-q"], ["git", "fetch", "-q", "--depth", "1", url, sha],
                ["git", "checkout", "-q", "FETCH_HEAD"]):
        subprocess.run(cmd, cwd=dest, check=True)
    head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=dest, check=True,
                          capture_output=True, text=True).stdout.strip()
    if head != sha:
        fail(f"{repo} checked out {head}, expected {sha}")
    return dest


# ------------------------------------------------------------------------- pret asm --
def asm_consts(path: pathlib.Path) -> dict[str, int]:
    """`const_def [n]` / `const NAME` enumeration -> {NAME: value}."""
    out: dict[str, int] = {}
    value = None
    for raw in path.read_text().splitlines():
        line = raw.split(";", 1)[0].strip()
        m = re.match(r"const_def\s*(\S+)?", line)
        if m:
            value = int(m.group(1).replace("$", "0x"), 0) if m.group(1) else 0
            continue
        m = re.match(r"const\s+(\w+)", line)
        if m and value is not None:
            out[m.group(1)] = value
            value += 1
            continue
        m = re.match(r"const_skip\s*(\d+)?", line)
        if m and value is not None:
            value += int(m.group(1) or 1)
    return out


def asm_blocks(path: pathlib.Path) -> dict[str, list[list[str]]]:
    """Global label -> list of `db` operand lists; consecutive labels share a block."""
    blocks: dict[str, list[list[str]]] = {}
    pending: list[str] = []
    current: list[list[str]] | None = None
    for raw in path.read_text().splitlines():
        line = raw.split(";", 1)[0].rstrip()
        m = re.match(r"^(\w+):{1,2}\s*$", line)
        if m:
            if current is not None and not current:
                pending.append(m.group(1))
            else:
                pending = [m.group(1)]
                current = []
            for label in pending:
                blocks[label] = current  # shared list object
            continue
        m = re.match(r"^\s+db\s+(.*)$", line)
        if m and current is not None:
            current.append([t.strip() for t in m.group(1).split(",")])
    return blocks


def pointer_table(path: pathlib.Path, name: str) -> list[str]:
    labels: list[str] = []
    inside = False
    for raw in path.read_text().splitlines():
        line = raw.split(";", 1)[0].strip()
        if re.match(rf"^{name}:{{1,2}}$", line):
            inside = True
            continue
        if inside:
            m = re.match(r"dw\s+(\w+)", line)
            if m:
                labels.append(m.group(1))
            elif line.startswith("assert_table_length") or re.match(r"^\w+:", line):
                break
    return labels


def evo_learnset(block: list[list[str]], moves: dict[str, int]) -> list[tuple[int, int]]:
    """Skip the evolution list (up to the first `db 0`), then `db level, MOVE` until `db 0`."""
    i = 0
    while i < len(block) and block[i] != ["0"]:
        i += 1
    if i == len(block):
        fail("evolution list without terminator")
    out = []
    for ops in block[i + 1:]:
        if ops == ["0"]:
            return out
        if len(ops) != 2:
            fail(f"unexpected learnset row {ops}")
        out.append((int(ops[0]), moves[ops[1]]))
    fail("learnset without terminator")
    return out


def gen1(repo: str) -> dict[tuple[int, int], list[tuple[int, int]]]:
    root = pret(repo)
    moves = asm_consts(root / "constants" / "move_constants.asm")
    internal = {v: k for k, v in asm_consts(root / "constants" / "pokemon_constants.asm").items()}
    dex = asm_consts(root / "constants" / "pokedex_constants.asm")
    evos = root / "data" / "pokemon" / "evos_moves.asm"
    pointers = pointer_table(evos, "EvosMovesPointerTable")
    blocks = asm_blocks(evos)
    base: dict[int, list[int]] = {}
    for f in sorted((root / "data" / "pokemon" / "base_stats").glob("*.asm")):
        text = f.read_text()
        m = re.search(r"db\s+(DEX_\w+)\s*;\s*pokedex id", text)
        b = re.search(r"db\s+([\w, ]+?)\s*;\s*level 1 learnset", text)
        if not m or not b:
            fail(f"{f.name}: no dex id / level 1 learnset")
        base[dex[m.group(1)]] = [moves[t.strip()] for t in b.group(1).split(",")]
    out: dict[tuple[int, int], list[tuple[int, int]]] = {}
    for index, label in enumerate(pointers, start=1):
        name = internal.get(index)
        if name is None or f"DEX_{name}" not in dex:
            continue  # MissingNo slots
        species = dex[f"DEX_{name}"]
        rows = [(0, m) for m in base[species] if m]
        rows += evo_learnset(blocks[label], moves)
        out[(species, 0)] = rows
    if sorted(s for s, _ in out) != list(range(1, 152)):
        fail(f"{repo}: expected species 1..151")
    return out


def gen2(repo: str) -> dict[tuple[int, int], list[tuple[int, int]]]:
    root = pret(repo)
    moves = asm_consts(root / "constants" / "move_constants.asm")
    pointers = pointer_table(root / "data" / "pokemon" / "evos_attacks_pointers.asm", "EvosAttacksPointers")
    blocks = asm_blocks(root / "data" / "pokemon" / "evos_attacks.asm")
    if len(pointers) != 251:
        fail(f"{repo}: {len(pointers)} pointers")
    return {(i, 0): evo_learnset(blocks[label], moves) for i, label in enumerate(pointers, start=1)}


# ---------------------------------------------------------------------------- pret C --
def c_defines(path: pathlib.Path, prefix: str) -> dict[str, int]:
    out = {}
    for m in re.finditer(rf"^#define\s+({prefix}\w+)\s+(\d+)\s*$", path.read_text(), re.M):
        out[m.group(1)] = int(m.group(2))
    return out


def national_dex(path: pathlib.Path) -> dict[str, int]:
    text = path.read_text()
    body = text[text.index("enum {"):]
    body = body[:body.index("};")]
    names = re.findall(r"\b(NATIONAL_DEX_\w+)", body)
    return {n: i for i, n in enumerate(names)}


def preprocess(text: str, defined: set[str]) -> str:
    """Evaluate #if defined(X) / #elif defined(X) / #else / #endif; other lines pass."""
    out, stack = [], []  # stack of (taking, any_taken)
    for line in text.splitlines():
        s = line.strip()
        m = re.match(r"#(el)?if\s+defined\((\w+)\)\s*$", s)
        if m and not m.group(1):
            parent = all(t for t, _ in stack)
            take = parent and m.group(2) in defined
            stack.append((take, take))
            continue
        if m and m.group(1):
            taking, any_taken = stack.pop()
            parent = all(t for t, _ in stack)
            take = parent and not any_taken and m.group(2) in defined
            stack.append((take, any_taken or take))
            continue
        if re.match(r"#else\b", s):
            taking, any_taken = stack.pop()
            parent = all(t for t, _ in stack)
            stack.append((parent and not any_taken, True))
            continue
        if re.match(r"#endif\b", s) and stack:
            stack.pop()
            continue
        if s.startswith("#if"):
            if not re.match(r"#ifndef\s+\w+_H\b|#ifndef\s+GUARD_", s):
                fail(f"unhandled preprocessor line: {s}")
            stack.append((True, True))
            continue
        if all(t for t, _ in stack):
            out.append(line)
    if stack:
        fail("unbalanced preprocessor")
    return "\n".join(out)


def gen3(repo: str, defined: set[str]) -> dict[tuple[int, int], list[tuple[int, int]]]:
    root = pret(repo)
    emerald = pret("pokeemerald")
    species = c_defines(root / "include" / "constants" / "species.h", "SPECIES_")
    moves = c_defines(root / "include" / "constants" / "moves.h", "MOVE_")
    dex = national_dex(emerald / "include" / "constants" / "pokedex.h")
    data = root / "src" / "data" / "pokemon"
    sets_text = preprocess((data / "level_up_learnsets.h").read_text(), defined)
    sets: dict[str, list[tuple[int, int]]] = {}
    for m in re.finditer(r"(?:static\s+)?const u16 (\w+)\[\]\s*=\s*\{(.*?)\};", sets_text, re.S):
        rows = [(int(l), moves[mv]) for l, mv in re.findall(r"LEVEL_UP_MOVE\(\s*(\d+)\s*,\s*(MOVE_\w+)\s*\)", m.group(2))]
        if m.group(1) in sets:
            fail(f"{repo}: duplicate {m.group(1)} after preprocessing")
        sets[m.group(1)] = rows
    ptr_text = preprocess((data / "level_up_learnset_pointers.h").read_text(), defined)
    pairs = re.findall(r"\[(SPECIES_\w+)\]\s*=\s*(\w+)", ptr_text)
    if not pairs:
        # pokeruby: a positional table, element i is internal species i (0 = SPECIES_NONE).
        body = re.search(r"gLevelUpLearnsets\[\]\s*=\s*\{(.*?)\};", ptr_text, re.S).group(1)
        by_id = {v: k for k, v in species.items()}
        pairs = [(by_id[i], label) for i, label in enumerate(re.findall(r"\b(g\w+LevelUpLearnset)\b", body)) if i in by_id]
    out = {}
    for name, label in pairs:
        internal = species[name]
        if name.startswith("SPECIES_OLD_UNOWN") or name in ("SPECIES_NONE", "SPECIES_EGG"):
            continue
        nat = dex.get("NATIONAL_DEX_" + name[len("SPECIES_"):])
        if nat is None or not 1 <= nat <= 386:
            fail(f"{repo}: no national dex for {name} ({internal})")
        out[(nat, 0)] = sets[label]
    if sorted(s for s, _ in out) != list(range(1, 387)):
        fail(f"{repo}: expected species 1..386, got {len(out)}")
    return out


# --------------------------------------------------------------------------- PKHeX ---
def bin_entries(data: bytes, magic: bytes) -> list[bytes]:
    """PKHeX BinLinkerAccessor16: 2-byte tag, u16 count, (count + 1) u16 offsets."""
    if len(data) < 6:
        fail("truncated BinLinker resource")
    count = struct.unpack_from("<H", data, 2)[0]
    offsets = [struct.unpack_from("<H", data, 4 + i * 2)[0] for i in range(count + 1)]
    if offsets != sorted(offsets) or offsets[-1] > len(data):
        fail("invalid BinLinker offsets")
    return [data[offsets[i]:offsets[i + 1]] for i in range(count)]


def learnset16(entry: bytes) -> list[tuple[int, int]]:
    """PKHeX LearnsetReader.ReadLearnset16: u16 move[count], then u8 level[count]."""
    if len(entry) % 3:
        fail(f"learnset entry size {len(entry)}")
    n = len(entry) // 3
    mv = struct.unpack_from(f"<{n}H", entry, 0)
    return [(entry[2 * n + i], mv[i]) for i in range(n)]


def pkhex_levelup(name: str, magic: bytes) -> list[list[tuple[int, int]]]:
    data = pathlib.Path(pkhex_path(f"Resources/byte/levelup/lvlmove_{name}.pkl")).read_bytes()
    return [learnset16(e) for e in bin_entries(data, magic)]


def pkhex_forms(personal: str, size: int, stats_off: int, count_off: int, max_species: int):
    data = pathlib.Path(pkhex_path(f"Resources/byte/personal/{personal}")).read_bytes()
    if len(data) % size:
        fail(f"{personal}: size {len(data)} not a multiple of {size}")
    rows = [data[i:i + size] for i in range(0, len(data), size)]
    forms = {}
    for s in range(1, max_species + 1):
        count = rows[s][count_off]
        index = struct.unpack_from("<H", rows[s], stats_off)[0]
        if count > 1 and index:
            for f in range(1, count):
                forms[(s, f)] = index + f - 1
    return forms, len(rows)


def gen45(name: str, magic: bytes, personal: str, size: int, stats_off: int, count_off: int,
          max_species: int):
    sets = pkhex_levelup(name, magic)
    forms, rows = pkhex_forms(personal, size, stats_off, count_off, max_species)
    if len(sets) != rows:
        fail(f"{name}: {len(sets)} learnsets vs {rows} personal rows")
    out = {(s, 0): sets[s] for s in range(1, max_species + 1)}
    for key, index in forms.items():
        if sets[index] != out[(key[0], 0)]:
            out[key] = sets[index]  # only forms whose learnset actually differs
    return out


def crosscheck(table: str, ours, name: str, magic: bytes) -> None:
    theirs = pkhex_levelup(name, magic)
    for (s, f), rows in ours.items():
        if f == 0 and theirs[s] != rows:
            fail(f"{table} species {s}: pret {rows} != PKHeX lvlmove_{name} {theirs[s]}")


def crosscheck1(table: str, ours, name: str) -> None:
    """Gen I: level-up rows vs lvlmove_<name>, base moves vs personal_<name> 0x0F-0x12."""
    theirs = pkhex_levelup(name, b"")
    personal = pathlib.Path(pkhex_path(f"Resources/byte/personal/personal_{name}")).read_bytes()
    for (s, _), rows in ours.items():
        levelup = [r for r in rows if r[0] > 0]
        base = [m for lvl, m in rows if lvl == 0]
        row = personal[s * 0x1C:(s + 1) * 0x1C]
        if levelup != theirs[s]:
            fail(f"{table} species {s}: pret level-up {levelup} != PKHeX lvlmove_{name} {theirs[s]}")
        if base != [m for m in row[0x0F:0x13] if m]:
            fail(f"{table} species {s}: pret base moves {base} != PKHeX personal_{name} {list(row[0x0F:0x13])}")


def main() -> None:
    data = {
        "RB": gen1("pokered"),
        "Y": gen1("pokeyellow"),
        "GS": gen2("pokegold"),
        "C": gen2("pokecrystal"),
        "RS": gen3("pokeruby", set()),
        "E": gen3("pokeemerald", set()),
        "FR": gen3("pokefirered", {"FIRERED"}),
        "LG": gen3("pokefirered", {"LEAFGREEN"}),
        "DP": gen45("dp", b"dp", "personal_dp", 0x2C, 0x2A, 0x29, 493),
        "PT": gen45("pt", b"pt", "personal_pt", 0x2C, 0x2A, 0x29, 493),
        "HGSS": gen45("hgss", b"hs", "personal_hgss", 0x2C, 0x2A, 0x29, 493),
        "BW": gen45("bw", b"bw", "personal_bw", 0x3C, 0x1C, 0x20, 649),
        "B2W2": gen45("b2w2", b"b2", "personal_b2w2", 0x4C, 0x1C, 0x20, 649),
    }
    for table, name in (("RB", "rb"), ("Y", "y")):
        crosscheck1(table, data[table], name)
        sys.stderr.write(f"  crosscheck {table}: pret == PKHeX lvlmove_{name} + personal_{name} base moves\n")
    for table, name, magic in (("GS", "gs", b"gs"), ("C", "c", b"c\x00"), ("RS", "rs", b"rs"),
                               ("E", "e", b"e\x00"), ("FR", "fr", b"fr"), ("LG", "lg", b"lg")):
        try:
            crosscheck(table, data[table], name, magic)
            sys.stderr.write(f"  crosscheck {table}: pret == PKHeX lvlmove_{name}\n")
        except FileNotFoundError:
            fail(f"PKHeX lvlmove_{name} missing at {PKHEX_REF}")

    for table, sets in data.items():
        for (s, f), rows in sets.items():
            for level, move in rows:
                if not 1 <= move <= MAX_MOVE[GEN[table]]:
                    fail(f"{table} {s}/{f}: move {move} out of Gen {GEN[table]} range")
                if level == 0 and GEN[table] != 1:
                    fail(f"{table} {s}/{f}: level-0 entry outside Gen I")
                if level > 100:
                    fail(f"{table} {s}/{f}: level {level}")
            if not rows:
                fail(f"{table} {s}/{f}: empty learnset")

    # Deduplicate identical move lists across tables.
    pool: list[int] = []
    seen: dict[tuple, int] = {}
    index_rows = []
    for t, table in enumerate(TABLES):
        for (s, f) in sorted(data[table]):
            rows = tuple(data[table][(s, f)])
            if rows not in seen:
                seen[rows] = len(pool)
                pool.extend((level << 16) | move for level, move in rows)
            index_rows.append((t, s, f, seen[rows], len(rows)))

    lines = [
        "// GENERATED by tools/gen_levelup_learnsets15.py -- do not edit by hand.",
        "// Native level-up learnsets, Generation I-V (R2-BUG-004). Sources (pinned):",
    ]
    lines += [f"//   pret/{repo} {sha}" for repo, sha in PRET.items()]
    lines += [f"//   kwsch/PKHeX {PKHEX_REF} (lvlmove_dp/pt/hgss/bw/b2w2, personal_*; cross-check of Gen I-III)",
              "// Entry = (level << 16) | move. Gen I level 0 = base-stats \"level 1 learnset\" move.",
              f"// {len(index_rows)} species/form rows, {len(pool)} pooled entries.",
              "",
              f"inline constexpr uint32_t kLevelUpPool15[{len(pool)}] = {{"]
    for i in range(0, len(pool), 8):
        lines.append("    " + ", ".join(f"0x{v:06X}u" for v in pool[i:i + 8]) + ",")
    lines += ["};", "",
              f"inline constexpr LevelUpIndex15 kLevelUpIndex15[{len(index_rows)}] = {{"]
    for t, s, f, start, count in index_rows:
        lines.append(f"    {{{t}, {s}, {f}, {start}, {count}}},")
    lines += ["};", ""]
    OUT.write_text("\n".join(lines))
    sys.stderr.write(f"wrote {OUT.relative_to(ROOT)}: {len(index_rows)} rows, {len(pool)} entries\n")


if __name__ == "__main__":
    main()
