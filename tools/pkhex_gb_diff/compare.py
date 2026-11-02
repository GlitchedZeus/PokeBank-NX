#!/usr/bin/env python3
"""Compare PKHeX and PokeBank NX verdicts on the same Gen I or Gen II corpus.

Usage: compare.py CORPUS PKHEX_OUT POKEBANK_OUT [--show N]
Prints an agreement table per corpus kind and the first N disagreements of each
direction, with both programs' explanations. Exits 1 if any verdict differs, if
either program skipped a corpus record, or if either printed a record the corpus
does not have.

PokeBank NX may also answer UNCERTAIN: every origin was excluded, but only by a rule
it does not treat as definitive. Those records are counted in their own columns, and
each one must cite a reason from UNCERTAIN_REASONS; any other UNCERTAIN fails.

A few verdicts differ from PKHeX's GB-era settings on purpose (DELIBERATE): each is
counted on its own and must name its documented origin; any other difference fails.
Every record lands in exactly one of: matching legal, matching invalid, deliberately
different, deliberately uncertain, or differing (a failure).
"""

import argparse
import collections
import re

# The only rules PokeBank NX reports as not definitive (Problem::definitive == false).
UNCERTAIN_REASONS = (
    # LearnGroup1.FlagEvolutionSlots: PKHeX states the rule but never reaches it for PK1,
    # because GetPrevious never returns null, so no independent implementation checks it.
    "pre-evolution could not still have been at",
    # Mew and Celebi: no recorded distribution matches, and the GB-era event list is not
    # known to be complete. PKHeX answers INVALID here.
    "was only distributed at events",
    # Korean Gold/Silver: PKHeX allows no Crystal, Stadium 2 or Gen I content; Bulbapedia
    # documents Korean-Western cartridge trades that bring it in. PKHeX answers INVALID.
    "Needs a trade between Korean Gold/Silver and a Western game",
    # Korean Gold/Silver with Crystal met data: code initializes the unused bytes
    # to zero, but matching Korean-build/retail behavior remains unverified here.
    # A WIP Korean disassembly exists; PKHeX ignores these bytes and may answer VALID.
    "Korean Gold/Silver do not record met data",
    # A Korean trainer name with one-byte-page characters: PKHeX's G1CharOT, which has not
    # been checked against the Korean naming screen. PKHeX answers INVALID.
    "Korean trainer name has characters outside the Hangul pages",
)


# Differences named by PKHeX's own result: (PKHeX verdict, PokeBank NX verdict, the result
# codes that must be PKHeX's only complaints).
DELIBERATE_PKHEX = (
    # PKHeX's word filter (a PKHeX setting) rejects some nicknames; Game Boy games filter
    # nothing, and PokeBank NX does not judge words.
    ("INVALID", "VALID", ("WordFilterFlaggedPattern", "WordFilterTooManyNumbers")),
)


def deliberate_pkhex(a, b):
    """True when every PKHeX complaint is one of a DELIBERATE_PKHEX entry's codes."""
    codes = a[1].partition(";")[2].split()
    for pv, ov, allowed in DELIBERATE_PKHEX:
        if a[0] == pv and b[0] == ov and codes and all(
                c.partition(":")[0] == "Nickname" and c.partition(":")[2] in allowed for c in codes):
            return True
    return False


def load(path, allowed):
    out = {}
    for number, line in enumerate(open(path, encoding="utf-8"), 1):
        parts = line.rstrip("\n").split(" ", 2)
        if len(parts) != 3 or not parts[0] or parts[1] not in allowed or not parts[2].strip():
            raise ValueError(f"{path}:{number}: malformed verdict record")
        head, verdict, detail = parts
        if ";" not in detail or (verdict == "VALID" and detail.partition(";")[2].strip()):
            raise ValueError(f"{path}:{number}: malformed explanation for {verdict}")
        if head in out:
            raise ValueError(f"{path}:{number}: duplicate record id {head}")
        out[head] = (verdict, detail)
    return out


def corpus_record(parts):
    if len(parts) not in (6, 7) or parts[2] not in ("0", "1", "8"):
        raise ValueError("wrong corpus fields or language")
    fields = parts[3:6]
    if any(not re.fullmatch(r"(?:[0-9a-fA-F]{2})+", x) for x in fields):
        raise ValueError("invalid hexadecimal corpus field")
    body, ot, nick = map(bytes.fromhex, fields)
    name_length = 6 if parts[2] == "1" else 11
    if len(body) not in (44, 48) or len(ot) != name_length or len(nick) != name_length:
        raise ValueError("wrong body or name length")
    if len(body) == 48:
        if len(parts) != 7 or parts[6] not in ("0", "1"):
            raise ValueError("wrong PK2 egg flag")
    elif len(parts) != 6 or parts[2] == "8":
        raise ValueError("wrong PK1 fields")
    return parts[2], body, parts[6] if len(parts) == 7 else "0"


def deliberate_record(a, b, record):
    """Require the actual record and sole complaint to fit a documented exception."""
    language, body, egg = record
    if len(body) != 48 or egg != "0":
        return False
    if a[0] == "INVALID" and b[0] == "VALID" and "GS Ball Celebi in Japanese Crystal" in b[1]:
        caught = int.from_bytes(body[29:31], "big")
        codes = a[1].partition(";")[2].split()
        return (language == "1" and body[0] == 251 and body[31] >= 30 and
                caught & 127 == 14 and (caught >> 8) & 63 == 30 and caught >> 14 in (1, 2, 3) and
                bool(codes) and all(c in ("Encounter:EncInvalid", "Fateful:FatefulGiftMissing") for c in codes))
    if a[0] == "VALID" and b[0] == "INVALID":
        match = re.fullmatch(r"\s*\| A level-100 Pokemon has exactly (\d+) EXP", b[1].partition(";")[2])
        return bool(match and body[31] == 100 and int.from_bytes(body[8:11], "big") < int(match[1]))
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("corpus")
    ap.add_argument("pkhex")
    ap.add_argument("pokebank")
    ap.add_argument("--show", type=int, default=8)
    args = ap.parse_args()
    kinds = {}
    records = {}
    for number, line in enumerate(open(args.corpus), 1):
        parts = line.split()
        if len(parts) not in (6, 7):
            raise ValueError(f"{args.corpus}:{number}: malformed corpus record")
        if parts[0] in kinds:
            raise ValueError(f"{args.corpus}:{number}: duplicate corpus id {parts[0]}")
        records[parts[0]] = corpus_record(parts)
        kinds[parts[0]] = parts[1]
    if not kinds:
        raise ValueError("empty corpus cannot establish agreement")
    pk = load(args.pkhex, {"VALID", "INVALID"})
    pb = load(args.pokebank, {"VALID", "INVALID", "UNCERTAIN"})
    missing = [k for k in kinds if k not in pk or k not in pb]
    extra = sorted((set(pk) | set(pb)) - set(kinds))
    table = collections.defaultdict(collections.Counter)
    diffs = collections.defaultdict(list)
    for key, kind in kinds.items():
        a, b = pk.get(key), pb.get(key)
        if not a or not b:
            table[kind]["missing"] += 1
            continue
        table[kind][(a[0], b[0])] += 1
        if b[0] == "UNCERTAIN":
            # Every complaint must be explained. A known exception followed by an
            # unrelated definitive problem must not hide that problem.
            complaints = [p.strip() for p in b[1].partition(";")[2].split("|") if p.strip()]
            known = bool(complaints) and all(any(k in p for k in UNCERTAIN_REASONS) for p in complaints)
            diffs[("uncertain", a[0]) if known else ("unexplained",)].append((key, kind, a[1], b[1]))
        elif a[0] != b[0]:
            deliberate = deliberate_record(a, b, records[key]) or deliberate_pkhex(a, b)
            if deliberate:
                table[kind]["deliberate"] += 1
                table[kind][(a[0], b[0])] -= 1
            diffs[("deliberate", a[0], b[0]) if deliberate else (a[0], b[0])].append((key, kind, a[1], b[1]))
    total = collections.Counter()
    print(f"{'kind':18} {'both valid':>10} {'both invalid':>13} {'PKHeX only valid':>17} {'ours only valid':>16}"
          f" {'uncertain':>10} {'deliberate':>11}")
    for kind in sorted(table):
        c = table[kind]
        total.update(c)
        print(f"{kind:18} {c[('VALID', 'VALID')]:10} {c[('INVALID', 'INVALID')]:13} "
              f"{c[('VALID', 'INVALID')]:17} {c[('INVALID', 'VALID')]:16} "
              f"{c[('VALID', 'UNCERTAIN')] + c[('INVALID', 'UNCERTAIN')]:10} {c['deliberate']:11}")
    agree = total[("VALID", "VALID")] + total[("INVALID", "INVALID")]
    uncertain = total[("VALID", "UNCERTAIN")] + total[("INVALID", "UNCERTAIN")]
    deliberate = total["deliberate"]
    differing = total[("VALID", "INVALID")] + total[("INVALID", "VALID")]
    n = sum(v for k, v in total.items() if k != "missing")
    decided = n - uncertain - deliberate
    print(f"\nmatching legal {total[('VALID', 'VALID')]}, matching invalid {total[('INVALID', 'INVALID')]}, "
          f"differing {differing}, deliberately uncertain {uncertain}, deliberately different {deliberate}")
    print(f"agreement {agree}/{decided} decided records = {100.0 * agree / max(decided, 1):.2f}%  "
          f"(PKHeX-only-valid {total[('VALID', 'INVALID')]}, ours-only-valid {total[('INVALID', 'VALID')]})")
    print(f"uncertain {uncertain}/{n} (PKHeX VALID {total[('VALID', 'UNCERTAIN')]}, "
          f"PKHeX INVALID {total[('INVALID', 'UNCERTAIN')]})")
    for direction, items in diffs.items():
        label = (f"Uncertain, PKHeX {direction[1]}" if direction[0] == "uncertain" else
                 f"Deliberately different, PKHeX {direction[1]}, PokeBank {direction[2]}"
                 if direction[0] == "deliberate" else
                 "UNCERTAIN without a known reason" if direction == ("unexplained",) else
                 "PKHeX VALID, PokeBank INVALID" if direction == ("VALID", "INVALID") else
                 "PKHeX INVALID, PokeBank VALID")
        print(f"\n== {label}: {len(items)}")
        for key, kind, a, b in items[:args.show]:
            print(f"  #{key} [{kind}]\n     PKHeX:    {a}\n     PokeBank: {b}")
    if missing or extra:
        print(f"\n== Corpus accounting: {len(missing)} records without both verdicts, "
              f"{len(extra)} verdicts for unknown records (first: {(missing + extra)[:5]})")
    print(f"\naccounted for {n}/{len(kinds)} corpus records")
    raise SystemExit(1 if missing or extra or any(k[0] not in ("uncertain", "deliberate") for k in diffs) else 0)


if __name__ == "__main__":
    try:
        main()
    except ValueError as exc:
        raise SystemExit(f"Corpus integrity error: {exc}")
