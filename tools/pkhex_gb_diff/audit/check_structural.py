#!/usr/bin/env python3
"""Compare independent structural expectations, without differential exemptions.

Exit 1 means a checker blessed an independently contradictory native record.
An expected audit finding is still a FAIL; this runner must not be presented as
passing production acceptance until the relevant production checks are fixed.
"""
import argparse
import importlib.util
import json
from pathlib import Path

COMPARE = Path(__file__).resolve().parents[1] / 'compare.py'
spec = importlib.util.spec_from_file_location('gb_compare', COMPARE)
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('manifest', type=Path)
    parser.add_argument('pkhex', type=Path)
    parser.add_argument('nx', type=Path)
    parser.add_argument('--json-out', type=Path)
    args = parser.parse_args()
    rows = json.loads(args.manifest.read_text())['cases']
    expected = {row['id']: row['expected'] for row in rows}
    if len(expected) != len(rows) or not rows:
        raise ValueError('empty or duplicate expectation IDs')
    pk = compare.load(args.pkhex, {'VALID', 'INVALID'})
    nx = compare.load(args.nx, {'VALID', 'INVALID', 'UNCERTAIN'})
    if set(expected) != set(pk) or set(expected) != set(nx):
        raise ValueError('missing or extra checker records')
    results = [dict(row, pkhex=pk[row['id']][0], nx=nx[row['id']][0],
                    pkhex_detail=pk[row['id']][1], nx_detail=nx[row['id']][1]) for row in rows]
    summary = dict(schema='GB_INDEPENDENT_STRUCTURAL_RESULTS_V1', count=len(rows),
                   pkhex_disagreements=sum(r['pkhex'] != r['expected'] for r in results),
                   nx_disagreements=sum(r['nx'] != r['expected'] for r in results), cases=results)
    if args.json_out:
        args.json_out.write_text(json.dumps(summary, indent=2)+'\n')
    print(f"{len(rows)} structural fixtures: NX {summary['nx_disagreements']} disagreements; "
          f"PKHeX {summary['pkhex_disagreements']} disagreements")
    raise SystemExit(1 if summary['nx_disagreements'] or summary['pkhex_disagreements'] else 0)


if __name__ == '__main__':
    main()
