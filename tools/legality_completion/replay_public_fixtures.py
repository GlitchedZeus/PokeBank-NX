#!/usr/bin/env python3
"""Replay every native Gen I-V fixture in an exact official source checkout.

Published test fixtures have no independently authenticated acquisition custody.
CK3/XK3 conversions are labelled derivatives. Parse/analysis errors stay in counts.
"""
import argparse, collections, hashlib, json, pathlib, re, subprocess

def run(command, **kwargs):
    return subprocess.check_output([str(x) for x in command],text=True,timeout=60,**kwargs)

def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=pathlib.Path,required=True)
    p.add_argument('--cache',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    assert run(['git','-C',a.source,'rev-parse','HEAD']).strip()=='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'
    root=a.source/'Tests/PKHeX.Core.Tests/Legality'
    paths=sorted(p for p in root.rglob('*') if p.suffix.lower() in {'.pk1','.pk2','.pk3','.pk4','.pk5','.ck3','.xk3'}
                 and re.search(r'/Generation [1-5](?:/| )',p.as_posix()))
    rows=[];inputs=collections.defaultdict(list)
    for index,path in enumerate(paths):
        rel=path.relative_to(a.source).as_posix();raw=path.read_bytes()
        row={'id':f'public-{index:03d}','source_path':rel,'source_blob':run(['git','-C',a.source,'rev-parse',f'HEAD:{rel}']).strip(),
             'original_sha256':hashlib.sha256(raw).hexdigest(),'original_bytes':len(raw),
             'custody':'published PKHeX test file; original-game acquisition not authenticated'}
        try:
            entity=json.loads(run([a.cache/'dotnet/dotnet',a.cache/'gl-export/GlobalLinkCatalog.dll','read-fixture',path]))
            row.update(entity);gen=entity['format'];i=row['id'];h=entity['hex'];ref='VALID' if entity['referenceValid'] else 'INVALID'
            row['conversion']='GameCube-to-PK3 derivative' if entity['sourceType'] in ('CK3','XK3') else 'decoded native record'
            if gen<=2:
                # The existing GB driver protocol takes a Japanese-layout bit
                # and lower-case hex; Korean is inferred from the raw OT bytes.
                line=f"{i} public {int(entity['language']==1)} {h.lower()} {entity['ot'].lower()} {entity['nickname'].lower()}"+(f" {int(entity['egg'])}" if gen==2 else '')
                driver=a.cache/'gb_diff';group=f'gen{gen}'
            elif gen<=4:
                cart=entity['version'] if entity['version'] in ('R','S','E','FR','LG','D','P','Pt','HG','SS') else ('R' if gen==3 else 'D')
                row['host_cart_context']=cart;row['host_cart_custody']='assumed from origin; source save not supplied'
                line=f'{i} {cart} {h} {ref}';driver=a.cache/'gen34_diff';group='gen34'
            else:
                line=f'{i} {h} {ref} public reference native B';driver=a.cache/'gen5_diff';group='gen5'
            native=run([driver],input=line+'\n').strip();assert native.split()[0]==i
            row['nx']={'VALID':'NOPROBLEMS','UNCERTAIN':'INCOMPLETE'}.get(native.split()[1],native.split()[1])
            row['production_line']=native;inputs[group].append(line)
        except Exception as exc:
            row['error']=str(exc);row['nx']='ERROR'
        rows.append(row)
    for group,lines in inputs.items():(a.output/f'{group}-input.txt').write_text('\n'.join(lines)+'\n')
    summary={str(gen):dict(collections.Counter(('VALID' if r.get('referenceValid') else 'INVALID')+'/'+r['nx']
        if 'referenceValid' in r else 'ERROR' for r in rows if r.get('format')==gen)) for gen in range(1,6)}
    result={'policy':'No selected file excluded after enumeration; errors remain in denominator. Not an authenticated real-world sample or independent oracle. Native source save identity unavailable.',
            'selected':len(paths),'processed':len(rows),'errors':sum('error' in r for r in rows),'by_generation':summary,'records':rows}
    (a.output/'MANIFEST.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='records'},indent=2))

if __name__=='__main__':main()
