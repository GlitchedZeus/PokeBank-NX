#!/usr/bin/env python3
"""Add independent source explanations without changing raw verdicts/counts."""
import argparse
import collections
import json
import pathlib
from release_metrics import read
from hgss_native_disagreement import necessary_level_proof
from bw2_shared_order import necessary_order_proof


def report(root,groups):
    counts=collections.defaultdict(collections.Counter)
    rows=[]
    for group in groups:
        for row in read(root,group):
            if row['reference']!='VALID' or row['nx']!='INVALID':continue
            a=row['reference_line'].split()
            proof=None
            if group=='gen34':proof=necessary_level_proof(a[2],a[4])
            elif group=='gen5':
                proof=necessary_level_proof(a[1],a[3],transferred=True)
                if proof is None:proof=necessary_order_proof(a[1],a[3])
            basis=proof['basis'] if proof else 'unresolved'
            counts[str(row['generation'])][basis]+=1
            rows.append({**row,'native_proof':proof,'classification':basis})
    return dict(policy='Every row remains in raw false Invalid and denominator. These necessary-condition source explanations are reported separately; no case IDs, oracle validity edits or denominator exclusions.',
                reference='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea',
                raw_stricter_breakdown={g:dict(c) for g,c in counts.items()},rows=rows)


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('root',type=pathlib.Path)
    p.add_argument('--groups',nargs='+',default=['gen1','gen2','gen34','gen5'])
    p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    data=report(a.root,a.groups);a.output.write_text(json.dumps(data,indent=2)+'\n')
    print(json.dumps(data['raw_stricter_breakdown'],sort_keys=True))
