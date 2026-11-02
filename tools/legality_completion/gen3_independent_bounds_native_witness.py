#!/usr/bin/env python3
"""Verify native Gen III local bounds independently of encounter/RNG history.

The unchanged Emerald PP function and masks run against every published native
move PP and packed PP-Up byte. EV constants/writer guards are source-checked;
this is not a full native EV-history or RNG proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import urllib.request


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--fetch', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    pins = json.loads((root / 'docs/legality/major-20261101/GEN3_RNG_INDEPENDENT_SOURCE_PINS.json').read_text())
    for row in pins['files']:
        path = args.source / row['path']
        if args.fetch and not path.exists():
            url = 'https://raw.githubusercontent.com/' + pins['repository'] + '/' + pins['commit'] + '/' + row['path']
            data = urllib.request.urlopen(url, timeout=30).read()
            assert hashlib.sha256(data).hexdigest() == row['sha256']
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        data = path.read_bytes()
        assert hashlib.sha256(data).hexdigest() == row['sha256']
        assert hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() == row['git_blob']
    source = (args.source / 'src/pokemon.c').read_text()
    signature = 'u8 CalculatePPWithBonus(u16 move, u8 ppBonuses, u8 moveIndex)'
    start = source.index(signature)
    body = source[start:source.index('\n}', start) + 2]
    masks = '\n'.join(line for line in source.splitlines() if line.startswith('#define PP_UP_SHIFTS(val)') or line.startswith('const u8 gPPUpGetMask['))
    assert len(masks.splitlines()) == 2
    constants = (args.source / 'include/constants/pokemon.h').read_text()
    assert re.search(r'^#define MAX_PER_STAT_EVS 255$', constants, re.M)
    assert re.search(r'^#define MAX_TOTAL_EVS 510$', constants, re.M)
    ev_body = source[source.index('void MonGainEVs('):source.index('void MonGainEVs(') + 4200]
    assert 'if (totalEVs >= MAX_TOTAL_EVS)' in ev_body
    assert 'if (totalEVs + (s16)evIncrease > MAX_TOTAL_EVS)' in ev_body
    assert 'if (evs[i] + (s16)evIncrease > MAX_PER_STAT_EVS)' in ev_body
    move_ids = {name: int(value) for name, value in re.findall(r'^#define (MOVE_\w+)\s+(\d+)\b', (args.source / 'include/constants/moves.h').read_text(), re.M)}
    move_pp = {}
    for name, entry in re.findall(r'\[(MOVE_\w+)\]\s*=\s*\{(.*?)\n\s*\}', (args.source / 'src/data/battle_moves.h').read_text(), re.S):
        if name in move_ids and move_ids[name] <= 354:
            pp = re.search(r'\.pp\s*=\s*(\d+)', entry)
            assert pp, name
            move_pp[move_ids[name]] = int(pp[1])
    assert set(move_pp) == set(range(355))
    assert move_ids['MOVE_HYPER_BEAM'] == 63 and move_pp[63] == 5
    args.work.mkdir(parents=True, exist_ok=True)
    cpp = args.work / 'native_local_bounds.cpp'
    prefix = '#include "Legality/Gen3LegalityData.h"\n#include <cassert>\n#include <cstdint>\n#include <iostream>\nusing u8=uint8_t;using u16=uint16_t;\nconstexpr unsigned MAX_MON_MOVES=4;\nstruct BattleMove {u8 pp;};\nconst BattleMove gBattleMoves[355]={'
    prefix += ','.join('{' + str(move_pp[i]) + '}' for i in range(355)) + '};\n'
    cpp.write_text(prefix + masks + '\n' + body + '\n' + MAIN)
    binary = args.work / 'native_local_bounds'
    flags = ['-fsanitize=address,undefined', '-fno-sanitize-recover=undefined', '-fno-omit-frame-pointer'] if args.sanitize else []
    subprocess.run([args.compiler, '-std=c++20', '-O1' if args.sanitize else '-O2', '-Wall', '-Wextra', '-Werror', *flags, '-I' + str(root / 'include'), str(cpp), str(root / 'src/Legality/Gen3LegalityData.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


MAIN = r'''int main() {
    for (unsigned move=0;move<=354;++move) {
        assert(gBattleMoves[move].pp==Legality::Gen3Data::kMovePP[move]);
        for (unsigned packed=0;packed<256;++packed)
            for (unsigned slot=0;slot<4;++slot) {
                const unsigned ups=(packed>>(2*slot))&3;
                const unsigned maximum=unsigned(Legality::Gen3Data::kMovePP[move])*(5+ups)/5;
                assert(CalculatePPWithBonus(move,packed,slot)==maximum);
            }
    }
    assert(CalculatePPWithBonus(63,3,0)==8);
    std::cout<<"Native Emerald PP table/function/masks and EV510 writer bounds PASS; no encounter/RNG certification inferred\n";
}
'''

if __name__ == '__main__':
    main()
