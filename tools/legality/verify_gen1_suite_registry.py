#!/usr/bin/env python3
"""Audit exact-head Gen I native source-suite registration, not legality parity."""
from __future__ import annotations
import argparse
from pathlib import Path
import re
import shlex
import tempfile

FOCUSED = Path('tests/test_legality_gen1_coordinator_smoke.sh')
L3 = Path('tests/test_legality_l3_run_all.sh')
REGISTRIES = ((FOCUSED, 'suites', 22), (L3, 'SUITES', 29))
MANDATORY = {
    'tests/test_legality_l1_gen1_pret_slots.cpp',
    'tests/test_legality_l1_gen1_pret_trade_metadata.cpp',
    'tests/test_legality_l3_gen1_all_species_joint.cpp',
    'tests/test_legality_l3_gen1_original_all_machines.cpp',
    'tests/test_legality_l3_gen1_all_species_machine_chronology.cpp',
    'tests/test_legality_gen1_editor_original_machines_subset.cpp',
    'tests/fixtures/legality_l4_gen1_original_native.cpp',
    'tests/fixtures/legality_l4_gen1_raw_record_witness.cpp',
    'tests/fixtures/legality_l5_gen1_rby_vector_integrity.cpp',
}


class RegistryError(ValueError):
    pass


def require(ok: bool, message: str) -> None:
    if not ok:
        raise RegistryError(message)


def extract(source: str, array: str) -> list[str]:
    matches = list(re.finditer(r'(?m)^\s*' + re.escape(array) + r'=\((.*?)^\s*\)',
                               source, re.S | re.M))
    require(len(matches) == 1, f'{array}: expected one bounded test array, got {len(matches)}')
    raw = matches[0].group(1)
    require(not re.search(r'\$|\x60|\\|\(\)|[{}]', raw),
            f'{array}: no command substitution, escaped or computed suite names allowed')
    items = shlex.split(raw, comments=True, posix=True)
    require(len(items) == len(set(items)), f'{array}: duplicate test registration')
    return items


def verify(root: Path) -> dict[str, int]:
    sets: dict[str, set[str]] = {}
    for relative, array, minimum in REGISTRIES:
        script = root / relative
        require(script.is_file(), f'Missing native runner: {relative}')
        text = script.read_text(encoding='utf-8')
        entries = extract(text, array)
        require(len(entries) >= minimum,
                f'{relative}: lost test registration: {len(entries)} < {minimum}')
        sources: set[str] = set()
        for name in entries:
            file = name if name.endswith('.cpp') else 'tests/' + name + '.cpp'
            require(file.startswith('tests/') and re.fullmatch(r'tests/[A-Za-z0-9_/]+\.cpp', file) is not None,
                    f'{relative}: malformed test path: {file}')
            require((root / file).is_file(), f'{relative}: missing source: {file}')
            sources.add(file)
        require(len(sources) == len(entries), f'{relative}: aliases/duplicate source paths')
        require('-Werror' in text and '-fsanitize=address,undefined' in text,
                f'{relative}: missing warnings-as-errors or ASan/UBSan gate')
        require('-O2' in text and '-O1' in text,
                f'{relative}: missing native normal/sanitize build modes')
        require('set -euo pipefail' in text, f'{relative}: failures may be ignored')
        sets[array] = sources
    require(MANDATORY.issubset(sets['suites']),
            'focused RBY smoke lost required original-source/QA category: '
            + ', '.join(sorted(MANDATORY - sets['suites'])))
    require(sets['suites'].intersection(sets['SUITES']) >= {
        'tests/test_legality_l3_gen1_all_species_joint.cpp',
        'tests/test_legality_l3_gen1_all_species_machine_chronology.cpp',
        'tests/test_legality_l3_gen1_original_all_machines.cpp',
    }, 'full L3 native gate lost all-species Gen I corroboration')
    return {'focused': len(sets['suites']), 'l3': len(sets['SUITES']),
            'distinct_sources': len(sets['suites'] | sets['SUITES'])}


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix='pbnx-gen1-reg-') as tmp:
        root = Path(tmp)
        (root / 'tests/fixtures').mkdir(parents=True)
        focused = [f'tests/fixture_{i:02d}.cpp' for i in range(22)]
        focused = sorted(MANDATORY) + focused[:13]
        other = [f'tests/l3_native_{i:02d}.cpp' for i in range(29)]
        other[:3] = sorted({
            'tests/test_legality_l3_gen1_all_species_joint.cpp',
            'tests/test_legality_l3_gen1_original_all_machines.cpp',
            'tests/test_legality_l3_gen1_all_species_machine_chronology.cpp',
        })
        for item in focused + other:
            p = root / item
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text('// dummy fixture\n')
        def script(array: str, entries: list[str], nested=False) -> str:
            paths = [x.removeprefix('tests/').removesuffix('.cpp') if nested else x for x in entries]
            return ('#!/bin/bash\nset -euo pipefail\n'
                    + f'{array}=(\n' + ''.join(f'    {name}\n' for name in paths)
                    + ')\nflags=(-O2 -O1 -Werror -fsanitize=address,undefined)\n')
        (root / FOCUSED).write_text(script('suites', focused))
        (root / L3).write_text(script('SUITES', other, nested=True))
        assert verify(root) == {'focused': 22, 'l3': 29, 'distinct_sources': 48}
        src = root / FOCUSED
        original = src.read_text()
        def bad(replacement: str, diagnostic: str) -> None:
            src.write_text(replacement)
            try:
                verify(root)
                raise AssertionError(f'mutation escaped fail closed: {diagnostic}')
            except RegistryError as exc:
                assert diagnostic in str(exc), (diagnostic, str(exc))
            finally:
                src.write_text(original)
        bad(original.replace(focused[0], focused[1]), 'duplicate')
        bad(original.replace(focused[0] + '\n', ''), 'lost test registration')
        bad(original.replace(focused[0], 'tests/not_present.cpp'), 'missing source')
        bad(original.replace('-fsanitize=address,undefined', ''), 'ASan/UBSan')
        bad(original.replace('set -euo pipefail', 'set -eu'), 'failures may be ignored')
        bad(original.replace(focused[0], '$(printf injected)'), 'command substitution')
        src.write_text(original.replace('tests/test_legality_l3_gen1_original_all_machines.cpp',
                                        'tests/l3_native_28.cpp'))
        try:
            verify(root)
            raise AssertionError('required original RBY category removal escaped')
        except RegistryError as exc:
            assert 'required original-source' in str(exc)
    print('Gen I exact-head suite registry self-test: positive, duplicates, drops, '
          'missing files, sanitizer, fail-fast, injection and mandatory categories: PASS')


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo-root', type=Path, default=Path('.'))
    ap.add_argument('--self-test', action='store_true')
    args = ap.parse_args()
    if args.self_test:
        self_test()
    else:
        summary = verify(args.repo_root)
        print('Gen I exact-head suite registry PASS: ' + ', '.join(
            f'{key}={value}' for key, value in summary.items()))


if __name__ == '__main__':
    main()
