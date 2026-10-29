#!/usr/bin/env python3
"""Separate, conservative adjudication ledger; never rewrites raw metrics.

Reuses unchanged GB predicates/native constructions. Records not established by
those checks remain unresolved. A policy difference is not automatically proof.
"""
import argparse, collections, hashlib, json, pathlib, subprocess, sys
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'pkhex_gb_diff'))
import compare
from compare_dual import classify
from native_chain import NativeChain
import release_metrics

def pin(path):return subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()

def main():
    p=argparse.ArgumentParser();p.add_argument('root',type=pathlib.Path);p.add_argument('--cache',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
    assert pin(a.cache/'pkhex261018')=='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'
    assert pin(a.cache/'pokered')=='af519899719f0754965776faac0e836a3b906e6d'
    assert pin(a.cache/'pokecrystal')=='3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6'
    context=compare.Context.load(a.cache/'pkhex261018',a.cache/'pokered')
    native=NativeChain(a.cache/'pokered',a.cache/'pokecrystal',a.cache/'pkhex261018')
    result={'policy':'Every raw discrepancy remains in METRICS. Existing policy predicates are separate from evidence-backed native proof. No new broad exception or record-ID whitelist. Original acquisition custody is unauthenticated.','generations':{},'rows':[]}
    for gen in (1,2):
        group=f'gen{gen}';records=compare.load_corpus(a.root/f'{group}-input.txt',context)
        oracle=compare.load_verdicts(a.root/f'{group}-reference.txt',('VALID','INVALID'),records,'reference')
        nx=compare.load_verdicts(a.root/f'{group}-production.txt',('VALID','INVALID','UNCERTAIN'),records,'NX')
        proofs={l.split()[0]:l for l in (a.root/f'{group}-proofs.txt').read_text().splitlines()}
        counts=collections.Counter()
        for row in release_metrics.read(a.root,group):
            if row['nx']==('INVALID' if row['reference']=='INVALID' else 'NOPROBLEMS'):continue
            rid=row['id'];kind,reason=classify(records[rid],oracle[rid],nx[rid],oracle[rid],context,native,proofs.get(rid,''))
            category=kind+':'+reason
            # Separate strict source proof from an older policy label. Korean
            # interoperability, in particular, is not authenticated here.
            evidence=('native-level/EXP invariant; existing source review' if reason=='level-exp'
                      else 'constructive native parent route; not event custody' if reason=='constructive-original-game-parent-ancestry'
                      else 'Japanese GS Ball native availability; individual receipt unverified' if reason=='gs-ball-celebi'
                      else 'existing scoped policy; evidence limits still apply' if kind=='deliberate'
                      else 'unresolved evidence' if kind=='uncertain' else 'not adjudicated')
            counts[category]+=1
            result['rows'].append({**row,'classification':category,'evidence':evidence})
        result['generations'][str(gen)]=dict(counts)
    # Recompute the original coupled-setter contradiction from actual fields,
    # requiring this to be the sole Invalid complaint. No case IDs enter it.
    counts=collections.Counter()
    for row in release_metrics.read(a.root,'gen34'):
        if row['nx']==('INVALID' if row['reference']=='INVALID' else 'NOPROBLEMS'):continue
        kind='unresolved'
        if row['generation']==4 and row['reference']=='VALID' and row['nx']=='INVALID':
            b=bytes.fromhex(row['reference_line'].split()[2]);u16=lambda i:int.from_bytes(b[i:i+2],'little')
            ext=u16(0x46);dp=u16(0x80);expected=ext if ext<=111 or 2000<=ext<=2010 or 3000<=ext<=3076 else 3002
            complaint=row['production_line'].partition(';')[2].split(' | W:')[0].strip()
            known=(1<=ext<=1999 or 2000<=ext<=3076)
            if known and ext!=55 and u16(0x44)==0 and u16(0x7e)==0 and dp!=expected and complaint==f'D/P met location field holds {dp} where Pt/HG/SS write {expected} for met location {ext}':
                kind='native-coupled-met-setter-contradiction'
        counts[str(row['generation'])+'/'+kind]+=1
        result['rows'].append({**row,'classification':kind})
    result['gen34']=dict(counts)
    for group in ['gen5','gen5-correlated']:
        if not (a.root/f'{group}-reference.txt').exists():continue
        result[group]=dict(collections.Counter(row['reference_line'].partition(';')[2].strip() for row in release_metrics.read(a.root,group)
            if row['reference']=='INVALID' and row['nx']=='NOPROBLEMS'))
    result['implementation_sha256']={path.name:hashlib.sha256(path.read_bytes()).hexdigest() for path in [pathlib.Path(compare.__file__),pathlib.Path(sys.modules['compare_dual'].__file__),pathlib.Path(sys.modules['native_chain'].__file__)]}
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2))

if __name__=='__main__':main()
