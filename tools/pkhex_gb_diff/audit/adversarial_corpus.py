#!/usr/bin/env python3
"""Deterministic raw records that the stochastic generators normalize away.

This is an independent structural expectation, not a PKHeX-derived verdict:
normal party/box records cannot acquire duplicate moves via the move-learning
routines, and party levels must agree with EXP outside documented transient
Day Care storage. PKHeX itself derives CurrentLevel from EXP and can ignore a
contradictory stored level, so agreement alone does not satisfy these fixtures.

Sources: pinned pret/pokered engine/pokemon/learn_move.asm and
pret/pokecrystal engine/pokemon/learn.asm, engine/pokemon/experience.asm,
engine/pokemon/move_mon.asm. See docs/audits/GEN12_INDEPENDENT_20261013.md.
No saves or ROMs are read, written, or distributed.
"""
import argparse
from itertools import combinations
import json
from pathlib import Path


def exp(level):
    # Bulbasaur's original Medium Slow growth formula, levels >= 2.
    return 6 * level ** 3 // 5 - 15 * level ** 2 + 100 * level - 140


def record(gen, moves, level=50, experience=None):
    body = bytearray(44 if gen == 1 else 48)
    xp = exp(level) if experience is None else experience
    if gen == 1:
        body[0], body[5], body[6], body[7] = 153, 22, 3, 45
        body[8:12] = bytes(moves)
        body[12:14] = (12345).to_bytes(2, 'big')
        body[14:17] = xp.to_bytes(3, 'big')
        body[27:29] = bytes([0x55, 0x55])
        body[29:33] = bytes([0] * 4)  # spent PP is legal for every adult move
        body[3], body[33] = level, level
    else:
        body[0] = 1
        body[2:6] = bytes(moves)
        body[6:8] = (12345).to_bytes(2, 'big')
        body[8:11] = xp.to_bytes(3, 'big')
        body[21:23] = bytes([0x55, 0x55])
        body[27], body[31] = 70, level
    # Populate coherent party stats too, so controls do not depend on the
    # checkers ignoring zero-filled party caches. Stat Experience is zero;
    # 0x5555 DVs give HP DV 15 and the other four DVs 5.
    hp = (45 + 15) * 2 * level // 100 + level + 10
    stats = [hp, (49+5)*2*level//100+5, (49+5)*2*level//100+5,
             (45+5)*2*level//100+5, (65+5)*2*level//100+5]
    if gen == 1:
        body[1:3] = hp.to_bytes(2, 'big')
        start = 34
    else:
        body[34:36] = hp.to_bytes(2, 'big')
        stats.append(stats[-1])  # Gen II Bulbasaur's Special Defense is also 65.
        start = 36
    for index, value in enumerate(stats):
        body[start+2*index:start+2*index+2] = value.to_bytes(2, 'big')
    ot = '868e8b8350505050505050'
    nickname = '8c8e8d5050505050505050'
    return f'0 {body.hex()} {ot} {nickname}' + (' 0' if gen == 2 else '')


def cases():
    rows = []
    for gen in (1, 2):
        rows.append((f'g{gen}-control', gen, 'VALID', record(gen, [33, 45, 15, 36]),
                     'Four distinct attainable moves with coherent level/EXP.'))
        for first, second in combinations(range(4), 2):
            moves = [33, 45, 15, 36]
            moves[second] = moves[first]
            rows.append((f'g{gen}-duplicate-{first}-{second}', gen, 'INVALID', record(gen, moves),
                         'A nonzero move occurs twice in an ordinary native record.'))
        for level, xp in ((5, 0), (50, 0), (50, exp(49)), (50, exp(51)),
                          (99, exp(100)), (100, exp(100)-1), (100, exp(100)+1)):
            rows.append((f'g{gen}-level-{level}-exp-{xp}', gen, 'INVALID', record(gen, [33, 45, 15, 36], level, xp),
                         'Stored party level contradicts the original growth formula.'))
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--manifest', type=Path)
    args = parser.parse_args()
    manifest = []
    for ident, gen, expected, fields, reason in cases():
        print(f'{ident} adversarial-g{gen} {fields}')
        manifest.append(dict(id=ident, generation=gen, expected=expected, reason=reason))
    if args.manifest:
        args.manifest.write_text(json.dumps({'schema': 'GB_INDEPENDENT_STRUCTURAL_V1', 'cases': manifest}, indent=2)+'\n')


if __name__ == '__main__':
    main()
