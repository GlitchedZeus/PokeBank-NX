#!/usr/bin/env python3
"""Strict observed-corpus metrics. Unknown never counts as either Valid or Invalid.
No exception allow-list; source-backed adjudication is a separate review obligation.
"""
import argparse, collections, csv, hashlib, json, pathlib

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def streams(root,gen):
    oracle=(root/'pkhex.txt').read_text().splitlines(); native=(root/'pokebank.txt').read_text().splitlines()
    assert len(oracle)==len(native) and oracle, 'Missing/unequal verdict streams'
    fishy_exposed=(root/'METADATA.json').exists() and json.loads((root/'METADATA.json').read_text()).get('oracle_exposes_fishy',False)
    seen=set()
    for left,right in zip(oracle,native):
        a=left.partition(' ;')[0].split(); b=right.partition(' ;')[0].split()
        assert a[0]==b[0] and a[0] not in seen; seen.add(a[0])
        p=a[1] if gen<3 else a[3] if gen<5 else a[2]
        n={'VALID':'NOPROBLEMS','UNCERTAIN':'INCOMPLETE'}.get(b[1],b[1])
        assert p in ('VALID','INVALID') and n in ('NOPROBLEMS','INCOMPLETE','INVALID','LOADFAIL')
        game='native-format-no-source-title' if gen<3 else a[1] if gen<5 else a[6]
        fishy=None if gen<3 else '~' in left.partition(';')[2] if gen<5 or fishy_exposed else None
        yield dict(id=a[0],game=game,p=p,n=n,fishy=fishy,oracle=left,native=right)

def metric(rows):
    c=collections.Counter((r['p'],r['n']) for r in rows)
    invalid=sum(v for (p,n),v in c.items() if p=='INVALID');valid=len(rows)-invalid
    def ratio(n,d): return {'numerator':n,'denominator':d,'percent':100*n/d if d else None}
    binary=sum(r['n']==('INVALID' if r['p']=='INVALID' else 'NOPROBLEMS') for r in rows)
    all_fishy=all(r['fishy'] is not None for r in rows)
    clean=[r for r in rows if r['p']=='VALID' and not r['fishy']] if all_fishy else []
    return {'records':len(rows),'cross_table':{p+'/'+n:v for (p,n),v in sorted(c.items())},
      'invalid_detection':ratio(c['INVALID','INVALID'],invalid),
      'valid_certification_including_fishy':ratio(c['VALID','NOPROBLEMS'],valid),
      'valid_non_fishy_certification':ratio(sum(r['n']=='NOPROBLEMS' for r in clean),len(clean)) if all_fishy else None,
      'binary_exact_verdict_agreement':ratio(binary,len(rows)),
      'three_state_agreement':ratio(sum(r['n']==('INVALID' if r['p']=='INVALID' else 'INCOMPLETE' if r['fishy'] else 'NOPROBLEMS') for r in rows),len(rows)) if all_fishy else None,
      'incomplete':sum(r['n']=='INCOMPLETE' for r in rows),'raw_false_invalid':c['VALID','INVALID'],
      'raw_false_clean':c['INVALID','NOPROBLEMS'],'loadfail':sum(r['n']=='LOADFAIL' for r in rows),
      'unresolved_or_differing':len(rows)-binary,
      'rule_coverage':None,'production_wired_rule_coverage':None,'historical_provenance':None,'original_source_evidence':None}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('output',type=pathlib.Path);p.add_argument('corpora',nargs='+',help='GEN:LABEL:DIRECTORY');a=p.parse_args()
    report={'policy':'Observed fixed-corpus rates only, never semantic completeness. No exception removed. Gen I/II source-title applicability unmeasured. Gen V game is record origin, not save identity. Older Gen V and GB oracle did not expose Fishy, so non-Fishy metrics remain unmeasured. Synthetic specimens do not authenticate historical events.','corpora':{}}
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with a.output.with_suffix('.mismatches.csv').open('w') as f:
        w=csv.writer(f);w.writerow(['corpus','record_id','game_or_origin','pkhex','nx','reason'])
        for spec in a.corpora:
            gen,label,path=spec.split(':',2);gen=int(gen);root=pathlib.Path(path)
            rs=list(streams(root,gen));groups={r['game'] for r in rs}
            report['corpora'][label]={'generation':gen,'files':{name:{'sha256':digest(root/name),'bytes':(root/name).stat().st_size} for name in ['pkhex.txt','pokebank.txt']},'all':metric(rs),'by_game_or_origin':{g:metric([r for r in rs if r['game']==g]) for g in sorted(groups)}}
            for r in rs:
                if r['n']!=('INVALID' if r['p']=='INVALID' else 'NOPROBLEMS'):
                    w.writerow([label,r['id'],r['game'],r['p'],r['n'],r['native'].partition(';')[2].strip()])
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    for label,r in report['corpora'].items(): print(label,json.dumps(r['all']))
