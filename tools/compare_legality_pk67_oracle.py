#!/usr/bin/env python3
"""Fail-closed native PK6/PK7 vs original unmodified PKHeX.Core oracle.

An original .NET 10 PKHeX host must have produced --oracle. This Python code
merely checks two independent result streams; it is NOT itself the oracle.
"""
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

FAMILIES = frozenset(('PK6', 'PK7'))
ENCODINGS = frozenset(('encrypted', 'plain'))
COLUMNS = 9
STATES = frozenset(('Parsed', 'InvalidSanity', 'InvalidChecksum',
                    'InvalidSpecies', 'InvalidNature', 'InvalidEVTotal',
                    'InvalidEVPerStat', 'EmptySlot'))


def compare(vectors: Path, oracle: Path) -> dict:
    expected = {}
    counts = Counter()
    for row in vectors.read_text(encoding='utf-8').splitlines():
        fields = row.split('\t')
        if len(fields) != COLUMNS:
            raise ValueError('invalid number of native vector fields')
        label, family, encoding, raw, status, species, nature, ev, pid = fields
        if label in expected or not label or family not in FAMILIES or encoding not in ENCODINGS:
            raise ValueError('duplicate or unsupported vector metadata')
        if status not in STATES or (encoding == 'plain' and status != 'Parsed'):
            raise ValueError('unsupported status/encoding')
        blob = bytes.fromhex(raw)
        if len(blob) not in (232, 260):
            raise ValueError('unexpected native record length')
        expected[label] = (family, encoding, blob, status, int(species),
                           int(nature), int(ev), int(pid))
        counts[(family, status)] += 1
    if len(expected) != 1152:
        raise ValueError(f'incomplete native vector set: {len(expected)}')
    if any(counts[(family, status)] != (128 if status == 'Parsed' else 64)
           for family in FAMILIES for status in STATES):
        raise ValueError(f'incomplete native evidence cohorts: {dict(counts)}')
    seen = set()
    for row in oracle.read_text(encoding='utf-8').splitlines():
        data = json.loads(row)
        label = data['id']
        if label not in expected or label in seen:
            raise ValueError('unknown/duplicate oracle result')
        seen.add(label)
        family, encoding, raw, status, species, nature, ev, pid = expected[label]
        if data['family'] != family or data['encoding'] != encoding:
            raise ValueError('oracle generation or ciphertext mode mismatch')
        if data['nativeStatus'] != status:
            raise ValueError('native cohort altered by oracle')
        if data['rawSha256'] != hashlib.sha256(raw).hexdigest():
            raise ValueError('oracle raw-data digest mismatch')
        if data.get('synthetic') is not True or data.get('sourceHistoryAuthenticated') is not False:
            raise ValueError('cannot promote synthetic history to retail proof')
        if status == 'Parsed':
            if not data['checksumValid'] or data['sanity'] != 0:
                raise ValueError('original PKHeX parsed/checksum mismatch')
            actual = (data['species'], data['nature'], data['evTotal'], data['pid'])
            if actual != (species, nature, ev, pid):
                raise ValueError('native/original PKHeX field discrepancy: ' + label)
            if data.get('originalPKHeX') is None:
                raise ValueError('original PKHeX LegalityAnalysis was not executed')
        elif status == 'InvalidSanity':
            if data['sanity'] == 0:
                raise ValueError('oracle sanity disagreement')
        elif status == 'InvalidChecksum':
            if data['checksumValid']:
                raise ValueError('oracle checksum disagreement')
        else:
            if not data['checksumValid'] or data['sanity'] != 0:
                raise ValueError('malformed native fields lack coherent framing')
            if status == 'InvalidSpecies' and data['species'] <= (721 if family == 'PK6' else 807):
                raise ValueError('species cap disagreement')
            if status == 'InvalidNature' and data['nature'] <= 24:
                raise ValueError('nature cap disagreement')
            if status == 'InvalidEVTotal' and data['evTotal'] <= 510:
                raise ValueError('EV sum disagreement')
            if status == 'InvalidEVPerStat' and data['maxEV'] <= 252:
                raise ValueError('per-stat EV disagreement')
            if status == 'EmptySlot' and data['species'] != 0:
                raise ValueError('empty slot disagreement')
    if seen != set(expected):
        raise ValueError('missing original PKHeX oracle result')
    return {'differential': 'PASS', 'nativeVectors': len(expected),
            'originalOracle': 'PKHeX.Core PK6 and PK7 (.NET 10)',
            'authenticatedRetailParity': False}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--vectors', required=True, type=Path)
    parser.add_argument('--oracle', required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(compare(args.vectors, args.oracle), sort_keys=True))


if __name__ == '__main__':
    main()
