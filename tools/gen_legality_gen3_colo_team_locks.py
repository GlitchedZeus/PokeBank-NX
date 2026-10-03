#!/usr/bin/env python3
"""Generate normal Pokemon Colosseum prior-team lock evidence from pinned PKHeX source."""

from __future__ import annotations

import os
import re

import pkhex_source

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "include", "Legality", "Gen3ColoShadowTeamLockData.inc")
TEAMS = "Legality/Encounters/Data/Gen3/Encounters3ColoTeams.cs"
SHADOW = "Legality/Encounters/Data/Gen3/Encounters3ColoShadow.cs"
NORMAL_SETS = ["First", "ColoMakuhita", "Gligar", "Murkrow", "Heracross", "Ursaring"]


def read(relpath: str) -> str:
    with open(pkhex_source.pkhex_path(relpath), encoding="utf-8") as handle:
        return handle.read()


def main() -> int:
    teams = read(TEAMS)
    shadow = read(SHADOW)

    set_map: dict[str, list[str]] = {}
    for match in re.finditer(
        r"public static readonly TeamLock\[\]\s+([A-Za-z0-9_]+)\s*=\s*\[([^\]]*)\]",
        teams,
    ):
        set_map[match.group(1)] = [x.strip() for x in match.group(2).split(",") if x.strip()]

    for name in NORMAL_SETS:
        if name not in set_map:
            raise ValueError("missing normal Colosseum team set " + name)
    if set_map["First"]:
        raise ValueError("Colosseum First team must remain empty")

    starts = list(re.finditer(
        r"public static readonly TeamLock\s+([A-Za-z0-9_]+)\s*=\s*new\(",
        shadow,
    ))
    definitions: dict[str, list[tuple[int, int, int]]] = {}
    for i, match in enumerate(starts):
        finish = starts[i + 1].start() if i + 1 < len(starts) else len(shadow)
        block = shadow[match.start():finish]
        locks: list[tuple[int, int, int]] = []
        for lock in re.finditer(r"new NPCLock\(([^)]*)\)", block):
            args = [x.strip() for x in lock.group(1).split(",")]
            if len(args) != 4:
                raise ValueError("unexpected non-normal Colosseum NPCLock in " + match.group(1))
            locks.append((int(args[1]), int(args[2]), int(args[3])))
        definitions[match.group(1)] = locks

    locks: list[tuple[int, int, int]] = []
    variants: list[tuple[int, int]] = []
    set_to_variant = [-1]
    for set_name in NORMAL_SETS[1:]:
        members = set_map[set_name]
        if len(members) != 1:
            raise ValueError("expected one normal Colosseum variant for " + set_name)
        member = members[0]
        member_locks = definitions.get(member)
        if member_locks is None:
            raise ValueError("missing TeamLock " + member)
        offset = len(locks)
        locks.extend(member_locks)
        set_to_variant.append(len(variants))
        variants.append((offset, len(member_locks)))

    if len(NORMAL_SETS) != 6 or len(variants) != 5 or len(locks) != 13:
        raise ValueError("unexpected normal Colosseum lock dimensions")

    lines = [
        "// GENERATED from PKHeX @ %s" % pkhex_source._REF,
        "// Sources: Encounters3ColoTeams.cs, Encounters3ColoShadow.cs.",
        "// Normal Colosseum prior-team locks only; Japanese E-Reader teams are intentionally excluded.",
        "inline constexpr std::array<LockEntry, %d> kLocks{{" % len(locks),
    ]
    lines += ["    {%d, %d, %d}," % row for row in locks]
    lines += ["}};", "", "inline constexpr std::array<VariantEntry, %d> kVariants{{" % len(variants)]
    lines += ["    {%d, %d}," % row for row in variants]
    lines += [
        "}};",
        "",
        "// TeamSet enum order: First, ColoMakuhita, Gligar, Murkrow, Heracross, Ursaring.",
        "inline constexpr std::array<int8_t, %d> kSetToVariant{{%s}};" %
        (len(set_to_variant), ", ".join(str(x) for x in set_to_variant)),
        "",
    ]

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("wrote %d normal Colosseum variants and %d locks to %s" %
          (len(variants), len(locks), OUT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
