#!/usr/bin/env python3
"""Read-only exact upstream PKHeX.Core C# -> checked-in Gen V/VI TM/HM table audit.

Usage:
 python3 tools/legality/verify_l3_gen56_pkhex_machines.py \
   --pkhex-core /path/to/PKHeX.Core

Require the pinned original owner-supplied ZIP/extracted snapshot (or identical
source hashes). This is a SOURCE PARITY check, not a running .NET host oracle or
retail Pokémon legality test. Preserves upstream GPL-3.0-or-later notices.
"""
from __future__ import annotations
import argparse
import hashlib
import re
from pathlib import Path

UPSTREAM_COMMIT = '66ef5983a5d349efa2dd615f2926a1b1ee97306e'
FILES = {
 'PersonalInfo/Info/PersonalInfo5BW.cs': '1c32b63ac1091fd76313a51b84a5d6306772feaee41cbe81ae805d8853362b1f',
 'Legality/LearnSource/Sources/LearnSource6XY.cs': '9333724e0fdf30775f62e540a5203c9a6a9fca3b39164b4c3dd1d084202b7f40',
 'Legality/LearnSource/Sources/LearnSource6AO.cs': '973b29f2b526d7b281ebac4d118a45d8e802becaed55f2ff461a8beff84bada5',
 'Legality/LearnSource/Sources/LearnSource5BW.cs': '2502d9c7b97df5c2ebfa869cb1a3bb1b3df2dbcfbcda221ab089bcb1cf235426',
 'Legality/LearnSource/Sources/LearnSource5B2W2.cs': '7e2a2f1b3906aea13050e33b7888ce23f20e5befcad74e8daa39f3148f6e5869',
}


def extract_cs_machine(text: str) -> list[int]:
    m = re.search(r'public static ReadOnlySpan<ushort> MachineMoves\s*=>\s*\[(.*?)\];',
                  text, re.S)
    if not m:
        raise ValueError('missing PKHeX MachineMoves source literal')
    body = re.sub(r'//[^\n]*', '', m.group(1))
    if not re.fullmatch(r'[\d,\s]*', body):
        raise ValueError('unexpected source expression in MachineMoves')
    return [int(v) for v in re.findall(r'\d+', body)]


def extract_cpp_machine(text: str, name: str) -> list[int]:
    m = re.search(rf'inline constexpr std::array<std::uint16_t,\d+> k{name}\s*=\s*\{{\{{(.*?)\}}\}};',
                  text, re.S)
    if not m:
        raise ValueError(f'missing native k{name} table')
    body = re.sub(r'//[^\n]*', '', m.group(1))
    if not re.fullmatch(r'[\d,\s]*', body):
        raise ValueError(f'bad native k{name} literal')
    return [int(v) for v in re.findall(r'\d+', body)]


def verify(core: Path, header: Path) -> None:
    sources = {}
    for path, expected_sha in FILES.items():
        raw = (core / path).read_bytes()
        actual_sha = hashlib.sha256(raw).hexdigest()
        if actual_sha != expected_sha:
            raise ValueError(f'SOURCE HASH MISMATCH: {path}: {actual_sha} != {expected_sha}')
        sources[path] = raw.decode('utf-8')
    cpp = header.read_text(encoding='utf-8')
    expected = (
        ('Gen5', 'PersonalInfo/Info/PersonalInfo5BW.cs', 101),
        ('XY', 'Legality/LearnSource/Sources/LearnSource6XY.cs', 105),
        ('ORAS', 'Legality/LearnSource/Sources/LearnSource6AO.cs', 107),
    )
    for table_name, path, size in expected:
        original = extract_cs_machine(sources[path])
        native = extract_cpp_machine(cpp, table_name)
        if len(original) != size or native != original:
            raise ValueError(f'{table_name}: original {len(original)}, native {len(native)}; content differs')
        print(f'{table_name}: {size}/{size} exact pinned machine indices match')
    bw = sources['Legality/LearnSource/Sources/LearnSource5BW.cs']
    b2w2 = sources['Legality/LearnSource/Sources/LearnSource5B2W2.cs']
    if 'index != 94; // TM95 not available in this game' not in bw or \
       'return info.GetIsLearnTM(index);' not in b2w2:
        raise ValueError('BW/B2W2 TM95 source gate changed; review required')
    if 'number==95 && (g==Game::Black||g==Game::White)' not in cpp:
        raise ValueError('BW-only TM95 exclusion missing in native adapter')
    print('BW TM95 exclusion and B2W2 allowance source checks PASS')
    print(f'PKHeX L3 Gen V/VI source audit PASS: pinned upstream {UPSTREAM_COMMIT}')


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--pkhex-core', type=Path, required=True)
    p.add_argument('--header', type=Path, default=Path('include/Legality/Gen56PkhexMachineSources.h'))
    args = p.parse_args()
    verify(args.pkhex_core, args.header)


if __name__ == '__main__':
    main()
