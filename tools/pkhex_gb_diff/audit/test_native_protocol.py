#!/usr/bin/env python3
"""Run malformed records through the actual C++/C# harness executables.

The caller can supply an ASan/UBSan binary. Exit 2 and an integrity diagnostic
are required; a crash, silent skip, or sanitizer diagnosis is a test failure.
"""
import subprocess
import sys
from pathlib import Path


def main():
    command = sys.argv[1:]
    if not command:
        raise SystemExit('usage: test_native_protocol.py COMMAND [ARG...]')
    fixture = Path(__file__).resolve().parents[3] / 'tests/audit/gen12_adversarial.corpus'
    good = fixture.read_text().splitlines()[14] + '\n'  # coherent PK2 control
    parts = good.split()
    bad = []
    for index, value in ((2, '7'), (3, '01'), (3, '0'), (3, 'zz'*48), (3, '0z'*48),
                         (4, '50'), (5, '50'), (6, '2')):
        fields = list(parts); fields[index] = value
        bad.append(' '.join(fields)+'\n')
    bad.extend(['missing fields\n', good+'\n', good+good, good.rstrip()+' EXTRA\n'])
    result = subprocess.run(command, input=good, text=True, capture_output=True)
    if result.returncode or ' VALID ' not in result.stdout:
        raise SystemExit(f'control failed: {result.stderr} {result.stdout}')
    for payload in bad:
        result = subprocess.run(command, input=payload, text=True, capture_output=True)
        if result.returncode != 2 or 'Corpus integrity error:' not in result.stderr or 'Sanitizer' in result.stderr:
            raise SystemExit(f'malformed protocol FAIL: {payload!r}\n{result.returncode}: {result.stderr}')
    print(f'Native protocol: control + {len(bad)} malformed inputs PASS')


if __name__ == '__main__':
    main()
