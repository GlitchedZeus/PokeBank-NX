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
REQUIRED_FOCUSED = frozenset({
    'tests/fixtures/legality_l4_gen1_individual_chain.cpp',
    'tests/fixtures/legality_l4_gen1_npc_metadata.cpp',
    'tests/fixtures/legality_l4_gen1_original_native.cpp',
    'tests/fixtures/legality_l4_gen1_preflight.cpp',
    'tests/fixtures/legality_l4_gen1_raw_record_witness.cpp',
    'tests/fixtures/legality_l5_gen1_rby_vector_integrity.cpp',
    'tests/test_legality_gen1_editor_original_machines_subset.cpp',
    'tests/test_legality_l1_gen1_pret_evolution.cpp',
    'tests/test_legality_l1_gen1_pret_fishing_roll.cpp',
    'tests/test_legality_l1_gen1_pret_gift_event.cpp',
    'tests/test_legality_l1_gen1_pret_prizes.cpp',
    'tests/test_legality_l1_gen1_pret_rod_init.cpp',
    'tests/test_legality_l1_gen1_pret_slot_weights.cpp',
    'tests/test_legality_l1_gen1_pret_slots.cpp',
    'tests/test_legality_l1_gen1_pret_statics_trades.cpp',
    'tests/test_legality_l1_gen1_pret_trade_metadata.cpp',
    'tests/test_legality_l1_gen1_pret_wild_roll.cpp',
    'tests/test_legality_l3_gen1_all_species_joint.cpp',
    'tests/test_legality_l3_gen1_all_species_machine_chronology.cpp',
    'tests/test_legality_l3_gen1_original_all_machines.cpp',
    'tests/test_legality_l3_gen1_rby_original_joint.cpp',
    'tests/test_legality_l3_gen1_retail_catalog.cpp',
})

REQUIRED_L3 = frozenset({
    'tests/test_legality_gen1_editor_original_machines_subset.cpp',
    'tests/test_legality_l3_gba_full_machine_classification.cpp',
    'tests/test_legality_l3_gba_original_full_tables.cpp',
    'tests/test_legality_l3_gba_original_machine_tables.cpp',
    'tests/test_legality_l3_gba_relearner.cpp',
    'tests/test_legality_l3_gba_starter_evolution.cpp',
    'tests/test_legality_l3_gba_versioned_levelup.cpp',
    'tests/test_legality_l3_gen12_joint_chronology.cpp',
    'tests/test_legality_l3_gen12_multistage.cpp',
    'tests/test_legality_l3_gen12_native_snapshots.cpp',
    'tests/test_legality_l3_gen1_all_species_joint.cpp',
    'tests/test_legality_l3_gen1_all_species_machine_chronology.cpp',
    'tests/test_legality_l3_gen1_original_all_machines.cpp',
    'tests/test_legality_l3_gen1_rby_original_joint.cpp',
    'tests/test_legality_l3_gen1_retail_catalog.cpp',
    'tests/test_legality_l3_gen3_joint_core.cpp',
    'tests/test_legality_l3_gen3_joint_native.cpp',
    'tests/test_legality_l3_joint_evolution_machine.cpp',
    'tests/test_legality_l3_joint_evolution_retail.cpp',
    'tests/test_legality_l3_machine_methods.cpp',
    'tests/test_legality_l3_move_chronology.cpp',
    'tests/test_legality_l3_original_source_digest.cpp',
    'tests/test_legality_l3_original_source_fingerprints.cpp',
    'tests/test_legality_l3_palpark_joint_stage.cpp',
    'tests/test_legality_l3_palpark_native_joint.cpp',
    'tests/test_legality_l3_palpark_native_retail.cpp',
    'tests/test_legality_l3_palpark_platinum_source_joint.cpp',
    'tests/test_legality_l3_platinum_native_sources.cpp',
    'tests/test_legality_l3_shedinja_copy.cpp',
})


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
        require('-UNDEBUG' in text, f'{relative}: active assertions (-UNDEBUG) missing')
        require('-Werror' in text and '-fsanitize=address,undefined' in text,
                f'{relative}: missing warnings-as-errors or ASan/UBSan gate')
        require('-O2' in text and '-O1' in text,
                f'{relative}: missing native normal/sanitize build modes')
        require('set -euo pipefail' in text, f'{relative}: failures may be ignored')
        baseline = REQUIRED_FOCUSED if array == 'suites' else REQUIRED_L3
        require(baseline.issubset(sources),
                f'{relative}: lost required baseline suite(s): '
                + ', '.join(sorted(baseline - sources)))
        sets[array] = sources
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
        focused = sorted(REQUIRED_FOCUSED)
        other = sorted(REQUIRED_L3)
        for item in focused + other:
            p = root / item
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text('// dummy fixture\n')
        def script(array: str, entries: list[str], nested=False) -> str:
            paths = [x.removeprefix('tests/').removesuffix('.cpp') if nested else x for x in entries]
            return ('#!/bin/bash\nset -euo pipefail\n'
                    + f'{array}=(\n' + ''.join(f'    {name}\n' for name in paths)
                    + ')\nflags=(-O2 -O1 -UNDEBUG -Werror -fsanitize=address,undefined)\n')
        (root / FOCUSED).write_text(script('suites', focused))
        (root / L3).write_text(script('SUITES', other, nested=True))
        assert verify(root) == {'focused': 22, 'l3': 29, 'distinct_sources': len(set(focused) | set(other))}
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
        bad(original.replace('-UNDEBUG', ''), 'active assertions')
        bad(original.replace(focused[0], '$(printf injected)'), 'command substitution')
        replacement = root / 'tests/registry_replacement.cpp'
        replacement.write_text('// injected but valid source name\\n')
        bad(original.replace('tests/test_legality_l1_gen1_pret_wild_roll.cpp',
                             'tests/registry_replacement.cpp'), 'lost required baseline')
    print('Gen I exact-head suite registry self-test: full-baseline preservation, duplicates, drops, '
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
