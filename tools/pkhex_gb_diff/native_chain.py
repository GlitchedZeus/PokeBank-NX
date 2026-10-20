#!/usr/bin/env python3
"""Constructive cross-check from pret assembly, independent of NX/PKHeX tables.

Only native, non-event witnesses are certified. A failed construction says nothing
about unrecorded distributions. See completion-session2/SOURCES.md for limits.
"""
import functools
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from verify_gb_legality_data import Pret, consts


class NativeChain:
    def __init__(self, red, crystal):
        self.dex = consts(crystal / "constants/pokemon_constants.asm")
        self.moves = consts(crystal / "constants/move_constants.asm")
        self.base, self.level, self.egg, self.evos = {}, {}, {}, {}
        for p in (crystal / "data/pokemon/base_stats").glob("*.asm"):
            text = p.read_text()
            species = self.dex[re.search(r"db (\w+)\s*;", text)[1]]
            groups = re.search(r"dn (EGG_\w+), (EGG_\w+)", text).groups()
            gender = re.search(r"db (GENDER_\w+)", text)[1]
            tm = re.search(r"tmhm(.*?); end", text, re.S)[1]
            self.base[species] = (gender, set(groups), {self.moves[x] for x in re.findall(r"[A-Z][A-Z0-9_]+", tm)})
        text = (crystal / "data/pokemon/evos_attacks.asm").read_text()
        pointers = re.findall(r"dw (\w+)EvosAttacks", (crystal / "data/pokemon/evos_attacks_pointers.asm").read_text())
        labels = {label: i for i, label in enumerate(pointers, 1)}
        for label, body in re.findall(r"^(\w+)EvosAttacks:\n(.*?)(?=^\w+EvosAttacks:|\Z)", text, re.M | re.S):
            if label not in labels:
                continue
            species = labels[label]
            self.level[species] = [(int(lv), self.moves[m]) for lv, m in re.findall(r"db (\d+), (\w+)", body)]
            self.evos[species] = [self.dex[line.split(",")[-1].strip()] for line in re.findall(r"db EVOLVE_[^;\n]+", body)]
        egg_text = (crystal / "data/pokemon/egg_moves.asm").read_text()
        egg_labels = {label: {self.moves[m] for m in re.findall(r"db (\w+)", body)} for label, body in re.findall(r"^(\w+):\n(.*?)(?=^\w+:|\Z)", egg_text, re.M | re.S)}
        pointers = re.findall(r"dw (\w+)", (crystal / "data/pokemon/egg_move_pointers.asm").read_text())
        self.egg = {i: egg_labels.get(label, set()) for i, label in enumerate(pointers, 1)}
        red = Pret(red)
        rb = red.base_stats()
        _, learn = red.evos_moves()
        self.old = {s: set(p["tms"] + p["moves"] + [m for _, m in learn[s]]) for s, p in rb.items()}
        self.previous = {to: s for s, tos in self.evos.items() for to in tos}
        self.fathers = {}
        for baby in self.base:
            if baby in self.previous:
                continue
            fathers = set()
            for mother, (gender, groups, _) in self.base.items():
                if self.root(mother) != (29 if baby == 32 else baby) or "EGG_NONE" in groups or mother == 132:
                    continue
                if gender != "GENDER_F100":
                    fathers.add(mother)
                if gender in ("GENDER_F0", "GENDER_UNKNOWN"):
                    continue
                for father, (sex, g, _) in self.base.items():
                    if father != 132 and "EGG_NONE" not in g and sex not in ("GENDER_F100", "GENDER_UNKNOWN") and groups & g:
                        fathers.add(father)
            self.fathers[baby] = fathers

    def root(self, species):
        while species in self.previous:
            species = self.previous[species]
        return species

    @functools.lru_cache(maxsize=2048)
    def states(self, moves):
        # Concrete sets of move IDs, rather than production's subset-bit encoding.
        query = frozenset(moves)
        reachable = {s: {frozenset()} for s in self.base}
        native = {s: query & (tm | {m for _, m in self.level[s]}) for s, (_, _, tm) in self.base.items()}
        native[235] = query - {0, 102, 118, 119, 120, 144, 153, 165, 214}  # native Sketch (Struggle cannot be sketched)
        old = {s: query & self.old.get(s, set()) for s in self.base}
        accept = {s: query & (self.egg.get(s, set()) | tm | {m for _, m in self.level[s]}) for s, (_, _, tm) in self.base.items()}
        changed = True
        while changed:
            changed = False
            for s in self.base:
                new = set(reachable[s])
                for state in tuple(reachable[s]):
                    new.add(state | native[s])
                    if all(m <= 165 for m in state):
                        new.add(state | old[s])
                for father in self.fathers.get(s, ()):
                    new.update(state & accept[s] for state in reachable[father])
                for state in tuple(new):
                    new.update(frozenset(m for j, m in enumerate(state) if mask & (1 << j)) for mask in range(1 << len(state)))
                if not new <= reachable[s]:
                    reachable[s] |= new
                    changed = True
                for to in self.evos[s]:
                    if not new <= reachable[to]:
                        reachable[to] |= new
                        changed = True
        return reachable

    def move_failure_matches(self, record, problem):
        match = re.fullmatch(r"(.+) cannot learn (.+) from this origin", problem)
        if not match:
            return False
        normalized = lambda value: re.sub(r"[^A-Z0-9]", "", value.upper())
        species = {normalized(name): dex for name, dex in self.dex.items()}
        moves = {normalized(name): move for name, move in self.moves.items()}
        stored = record.body[2:6] if record.format == 2 else record.body[8:12]
        return species.get(normalized(match[1])) == record.species and moves.get(normalized(match[2])) in stored

    def certifies(self, record, ctx):
        if record.format != 2 or record.korean:
            return False
        species = record.species
        # An independently constructible native path, not a claim about events.
        moves = set(record.body[2:6]) - {0}
        if record.egg:
            initial = [m for level, m in self.level[species] if level <= 5][-4:]
            moves -= set(initial)
        else:
            stage = species
            while True:
                moves -= {m for level, m in self.level[stage] if level <= record.level}
                moves -= self.base[stage][2]
                if stage not in self.previous:
                    break
                stage = self.previous[stage]
        baby = self.root(species)
        if not moves or not moves <= (self.egg.get(baby, set()) | self.base[baby][2] | {m for _, m in self.level[baby]}):
            return False
        known = self.states(tuple(sorted(moves)))
        return any(frozenset(moves) in known[f] for f in self.fathers.get(baby, ()))
