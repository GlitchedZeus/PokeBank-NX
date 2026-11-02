#!/usr/bin/env python3
"""Verify retained raw streams and every secondary-reference row, without waivers."""
import argparse
import hashlib
import json
import pathlib

PIN = 'bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'


def verify(root, group):
    reference = (root / 'pkhex.txt').read_text().splitlines()
    production = (root / 'pokebank.txt').read_text().splitlines()
    joined_path = root / 'joined.txt'
    joined = joined_path.read_text().splitlines()
    assert reference and len(reference) == len(production) == len(joined), 'Missing raw rows'
    assert joined == [a + '\t' + b for a, b in zip(reference, production)], 'Joined bytes changed'
    index = 3 if group == 'gen34' else 2
    hex_index = 2 if group == 'gen34' else 1
    expected = {}
    seen = set()
    for left, right in zip(reference, production):
        a, b = left.split(), right.split()
        assert a[0] == b[0] and a[0] not in seen, 'Missing or duplicate identity'
        seen.add(a[0])
        payload = bytes.fromhex(a[hex_index])
        assert len(payload) in ((80, 100, 136, 236) if group == 'gen34' else (136, 220)), 'Malformed raw record size'
        if a[index] == 'VALID' and b[1] == 'INVALID':
            expected[a[0]] = (left, right, hashlib.sha256(payload).hexdigest())
    secondary = root / 'current-release'
    report = json.loads((secondary / 'REPORT.json').read_text())
    assert report['reference'] == PIN and report['reference_source_clean']
    assert report['older_record_count'] == len(reference)
    assert report['older_joined_sha256'] == hashlib.sha256(joined_path.read_bytes()).hexdigest()
    rows = report['rows']
    assert len(rows) == len(expected), 'A raw stricter case was omitted from replay'
    assert len({row['id'] for row in rows}) == len(rows)
    if rows:
        inputs = (secondary / 'input.txt').read_text().splitlines()
        outputs = (secondary / 'reference.txt').read_text().splitlines()
        assert len(rows) == len(inputs) == len(outputs)
        for row, original, replay in zip(rows, inputs, outputs):
            left, right, digest = expected[row['id']]
            assert row['legacy_line'] == original == left
            assert row['production_line'] == right and row['sha256'] == digest
            assert row['reference_line'] == replay
            a = replay.split()
            assert a[0] == row['id'] and row['release'] == a[index]
            assert hashlib.sha256(bytes.fromhex(a[hex_index])).hexdigest() == digest
            assert row['analysis_error'] == ('Misc:Error' in replay)
        assembly = secondary / 'oracle/PKHeX.Core.dll'
        assert hashlib.sha256(assembly.read_bytes()).hexdigest() == report['reference_assembly_sha256']
    print(f'{root.name}: {len(reference)} unchanged raw rows, {len(rows)} complete secondary replays PASS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('group', choices=['gen34', 'gen5'])
    parser.add_argument('runs', nargs='+', type=pathlib.Path)
    args = parser.parse_args()
    for run in args.runs:
        verify(run, args.group)
