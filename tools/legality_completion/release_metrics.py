#!/usr/bin/env python3
"""Strict release holdout accounting; retain errors and all raw disagreements.

Reference VALID includes Fishy. Unknown never becomes either binary verdict.
This measures synthetic corpus agreement, not rule completeness or authenticity.
"""
import argparse, collections, csv, hashlib, json, pathlib

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def read(root, group):
    reference=root/f'{group}-reference.txt'
    production=root/f'{group}-production.txt'
    left=reference.read_text().splitlines(); right=production.read_text().splitlines()
    assert left and len(left)==len(right), (group,len(left),len(right))
    seen=set()
    for line,native in zip(left,right):
        a=line.split();b=native.split()
        assert a[0]==b[0] and a[0] not in seen, (group,a[0],b[0])
        seen.add(a[0])
        gen=5 if group.startswith('gen5') else int(group[-1]) if group!='gen34' else (3 if len(a[2])==160 else 4)
        verdict=a[3] if group=='gen34' else a[2] if group.startswith('gen5') else a[1]
        actual={'VALID':'NOPROBLEMS','UNCERTAIN':'INCOMPLETE'}.get(b[1],b[1])
        assert verdict in ('VALID','INVALID'), (group,line)
        assert actual in ('NOPROBLEMS','INVALID','INCOMPLETE','LOADFAIL'), (group,native)
        fishy=bool('~' in line.partition(';')[2])
        yield {'group':group,'generation':gen,'id':a[0],'reference':verdict,'nx':actual,
               'fishy':fishy,'misc_error':'Misc:Error' in line,
               'game':a[1] if group=='gen34' else a[6] if group.startswith('gen5') else 'no source save title',
               'reference_line':line,'production_line':native}

def metric(rows):
    cross=collections.Counter((r['reference'],r['nx']) for r in rows)
    agree=cross['VALID','NOPROBLEMS']+cross['INVALID','INVALID']
    return {'records':len(rows),'raw_agreement':agree,
        'raw_agreement_percent':100*agree/len(rows) if rows else None,
        'raw_false_invalid':cross['VALID','INVALID'],
        'raw_false_clean':cross['INVALID','NOPROBLEMS'],
        'incomplete':sum(r['nx']=='INCOMPLETE' for r in rows),
        'loadfail':sum(r['nx']=='LOADFAIL' for r in rows),
        'reference_fishy_all':sum(r['fishy'] for r in rows),
        'reference_fishy_valid':sum(r['fishy'] and r['reference']=='VALID' for r in rows),
        'reference_misc_error':sum(r['misc_error'] for r in rows),
        'cross_table':{f'{p}/{n}':v for (p,n),v in sorted(cross.items())},
        'fishy_cross_table':{f'{p}/{n}':v for (p,n),v in sorted(collections.Counter((r['reference'],r['nx']) for r in rows if r['fishy']).items())}}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('root',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path,required=True)
    p.add_argument('--groups',nargs='+',choices=['gen1','gen2','gen34','gen5'],default=['gen1','gen2','gen34','gen5'],help='Explicit measured streams; missing requested files remain errors.')
    a=p.parse_args();groups=a.groups;rows=[r for group in groups for r in read(a.root,group)]
    report={'policy':'Fresh generated records, including generated INVALID, retained without exclusions. Raw binary agreement is not historical accuracy or completeness. Deliberate disagreements remain in raw counts. Fishy is exposed in every reference stream. LOADFAIL remains in denominator.',
        'reference':'bfe89c74a3f5265805e364b79bc04ad38ebbc5ea','groups':groups.copy(),'generations':{},'files':{}}
    for gen in range(1,6):
        subset=[r for r in rows if r['generation']==gen]
        if not subset:continue
        report['generations'][str(gen)]={**metric(subset),'by_game_or_origin':{g:metric([r for r in subset if r['game']==g]) for g in sorted({r['game'] for r in subset})}}
    report['total']=metric(rows)
    extra=[]
    if (a.root/'gen5-correlated-reference.txt').exists():
        groups.append('gen5-correlated');extra=list(read(a.root,'gen5-correlated'))
        report['correlated_gen5']=metric(extra)
    for group in groups:
        for side in ['reference','production']:
            path=a.root/f'{group}-{side}.txt';report['files'][path.name]={'sha256':digest(path),'bytes':path.stat().st_size}
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    with a.output.with_suffix('.differences.csv').open('w') as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader()
        w.writerows(r for r in rows+extra if r['nx']!=('INVALID' if r['reference']=='INVALID' else 'NOPROBLEMS'))
    for gen,info in report['generations'].items():
        print(gen,{k:v for k,v in info.items() if k!='by_game_or_origin'})
