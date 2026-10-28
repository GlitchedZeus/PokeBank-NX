#!/usr/bin/env python3
"""Typed HGSS ordinary-encounter audit from pinned native data and swarm gates.

This does not infer swarm availability from a nonzero placeholder species.
Headbutt, contest, Safari, event and RNG conditions are separate obligations.
"""
import argparse, collections, json, pathlib, re


def constants(path, prefix):
    return {n:int(v) for n,v in re.findall(r'#define\s+('+prefix+r'\w+)\s+(\d+)\b',path.read_text())}


def native_rows(root):
    ids=constants(root/'include/constants/map_sections.h','MAPSEC_')
    species=constants(root/'include/constants/species.h','SPECIES_')
    headers=(root/'src/data/map_headers.h').read_text()
    banks=collections.defaultdict(set);maps={}
    for name,body in re.findall(r'\[(MAP_\w+)\]\s*=\s*\{(.*?)\n\s*\},',headers,re.S):
        a=re.search(r'\.wildEncounterBank\s*=\s*ENCDATA_(\w+)',body)
        b=re.search(r'\.mapsec\s*=\s*(MAPSEC_\w+)',body)
        if a and b:
            banks[a[1]].add(ids[b[1]]);maps[name]=a[1]
    swarm_source=(root/'src/unk_02097F6C.c').read_text().split('static const u16 sSwarmMapLUT',1)[1].split('};',1)[0]
    swarms={maps[n]:int(v) for n,v in re.findall(r'\{\s*(MAP_\w+),\s*(\d+)\s*\}',swarm_source)}
    assert len(swarms)==20
    raw=json.loads((root/'files/fielddata/encountdata/gs_enc_data.json').read_text())['encounters']
    expected=set();unmapped=[]
    def select(v,version):
        if isinstance(v,dict):
            key='HEARTGOLD' if version==7 else 'SOULSILVER'
            if key in v:return select(v[key],version)
            return {k:select(x,version) for k,x in v.items()}
        if isinstance(v,list):return [select(x,version) for x in v]
        return v
    def sp(v,version):
        v=select(v,version)
        return {species[x] for x in v.values()} if isinstance(v,dict) else {species[v]}
    def levels(mon,version):
        x=select(mon['level'],version)
        return (x['min'],x['max']) if isinstance(x,dict) else (x,x)
    for r in raw:
        bank=r['map']
        if bank not in banks:
            unmapped.append(bank);continue
        for version in (7,8):
            for loc in banks[bank]:
                for typ,key in [(0,'land'),(1,'surf'),(5,'rock_smash')]:
                    d=r[key];rate=select(d['rate'],version)
                    if not rate:continue
                    for slot,mon in enumerate(d['mons']):
                        lo,hi=levels(mon,version);possibilities=sp(mon['species'],version)
                        if typ==0:
                            if swarms.get(bank)==0 and slot in (0,1):possibilities|=sp(r['landSwarm'],version)
                            if slot in (2,3,4,5):
                                for radio in ['hoenn','sinnoh']:
                                    if radio in r:possibilities|=sp(r[radio][0 if slot<4 else 1],version)
                        if typ==1 and swarms.get(bank)==1 and slot==0:possibilities|=sp(r['surfSwarm'],version)
                        for spec in possibilities:
                            if spec:expected.add((version,loc,typ,rate,spec,lo,hi,slot))
                for typ,key in [(2,'old_rod'),(3,'good_rod'),(4,'super_rod')]:
                    d=r['fishing'][key];rate=select(d['rate'],version)
                    if not rate:continue
                    for slot,mon in enumerate(d['mons']):
                        lo,hi=levels(mon,version);possibilities=sp(mon['species'],version)
                        if (typ,slot) in [(3,3),(4,1)] and 'nightFish' in r:possibilities|=sp(r['nightFish'],version)
                        # Native night replacement happens before the swarm replacement.
                        if swarms.get(bank)==2 and slot in {2:[2],3:[0,2,3],4:[0,1,2,3,4]}[typ]:possibilities|=sp(r['fishSwarm'],version)
                        for spec in possibilities:
                            if spec:expected.add((version,loc,typ,rate,spec,lo,hi,slot))
    return expected,{'swarm_banks':swarms,'unmapped_banks':unmapped}


def published_rows(areas):
    return {(a['version'],a['location'],a['type'],a['rate'],q['species'],q['min'],q['max'],q['slot'])
        for a in areas if a['version'] in (7,8) and a['type'] in (0,1,2,3,4,5) for q in a['slots']}


def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True)
    p.add_argument('--catalog',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True)
    a=p.parse_args();expected,metadata=native_rows(a.native)
    actual=published_rows(json.loads(a.catalog.read_text()))
    missing=expected-actual;extra=actual-expected
    report={'scope':'Ordinary land/surf/rod/rock-smash typed rows only; not RNG, Safari, Headbutt, events or acquisition authentication.',
        'expected':len(expected),'actual':len(actual),'missing':sorted(missing),'extra':sorted(extra),**metadata}
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print('Native HGSS ordinary catalog:',len(expected),'native,',len(actual),'catalog,',len(missing),'missing,',len(extra),'extra')
    if missing or extra:
        print('missing examples',sorted(missing)[:8]);print('extra examples',sorted(extra)[:8])
        raise SystemExit(1)

if __name__=='__main__':main()
