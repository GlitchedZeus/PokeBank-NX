#!/usr/bin/env python3
"""Comparator selftest with MOCK oracle rows. NOT evidence of PKHeX parity."""
import hashlib
import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from compare_legality_pk67_oracle import compare


def must_fail(vectors, oracle, expected_message):
    try:
        compare(vectors, oracle)
    except (ValueError, KeyError, AssertionError) as error:
        assert expected_message in str(error), error
    else:
        raise AssertionError('a corrupted oracle cohort passed')


def main(vectors):
    lines = vectors.read_text().splitlines()
    assert len(lines) == 1152
    fake = []
    for line in lines:
        ident, family, encoding, rawhex, status, species, nature, ev, pid = line.split('\t')
        valid = status not in ('InvalidChecksum',)
        fake.append(dict(
            id=ident, family=family, encoding=encoding,
            rawSha256=hashlib.sha256(bytes.fromhex(rawhex)).hexdigest(),
            nativeStatus=status, sanity=1 if status=='InvalidSanity' else 0,
            checksumValid=valid, species=int(species), nature=int(nature),
            evTotal=int(ev), maxEV=253 if status=='InvalidEVPerStat' else 252,
            pid=int(pid), originalPKHeX={'parsed':True} if status=='Parsed' else None,
            synthetic=True, sourceHistoryAuthenticated=False))
        if status == 'InvalidSpecies': fake[-1]['species'] = 722 if family == 'PK6' else 808
        if status == 'InvalidNature': fake[-1]['nature'] = 25
        if status == 'InvalidEVTotal': fake[-1]['evTotal'] = 511
        if status == 'EmptySlot': fake[-1]['species'] = 0
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / 'fake.jsonl'
        def write(rows):
            output.write_text('\n'.join(json.dumps(x) for x in rows) + '\n')
        write(fake)
        assert compare(vectors, output)['nativeVectors'] == 1152
        damaged = [dict(x) for x in fake]
        damaged[0]['rawSha256'] = '0' * 64
        write(damaged)
        must_fail(vectors, output, 'digest mismatch')
        damaged = [dict(x) for x in fake]
        damaged[0]['sourceHistoryAuthenticated'] = True
        write(damaged)
        must_fail(vectors, output, 'retail proof')
        damaged = [dict(x) for x in fake]
        damaged[0]['family'] = 'PK7' if damaged[0]['family'] == 'PK6' else 'PK6'
        write(damaged)
        must_fail(vectors, output, 'generation')
        damaged = [dict(x) for x in fake]
        damaged[0]['checksumValid'] = False
        write(damaged)
        must_fail(vectors, output, 'parsed/checksum')
        write(fake[:-1]); must_fail(vectors, output, 'missing original')
    print('PK6/PK7 comparator selftests (MOCK rows only): good fixture and five rejection conditions PASS')


if __name__ == '__main__':
    main(Path(sys.argv[1]))
