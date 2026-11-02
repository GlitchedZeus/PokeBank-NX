#!/usr/bin/env python3
"""Native necessary wild-level evidence; never changes an oracle verdict.

Safari levels union every area/time/base/bonus, ignoring block/day conditions.
Ordinary levels union all native times, radio, night and actual swarm gates.
These are deliberately broad sets: membership does not prove a capture/frame.
"""
import argparse, collections, hashlib, json, pathlib, urllib.request
from hgss_native_catalog import constants, native_rows
from hgss_swarm_repair import checked_native, PROJECT

INDEX=PROJECT/'docs/legality/campaign-20261028/HGSS_NATIVE_LEVEL_INDEX.json'
SAFARI_PINS=PROJECT/'docs/legality/campaign-20261028/HGSS_SAFARI_SOURCE_PINS.json'


def generate(root,fetch=False):
    pins=checked_native(root,fetch)
    safari_pins=json.loads(SAFARI_PINS.read_text())
    assert safari_pins['commit']==pins['commit']
    for row in safari_pins['files']:
        p=root/row['path']
        if fetch and not p.exists():
            p.parent.mkdir(parents=True,exist_ok=True)
            p.write_bytes(urllib.request.urlopen(row['url'],timeout=30).read())
        b=p.read_bytes()
        assert hashlib.sha256(b).hexdigest()==row['sha256']
        assert hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()==row['git_blob_sha1']
    species=constants(root/'include/constants/species.h','SPECIES_')
    safari=collections.defaultdict(set)
    def walk(x):
        if isinstance(x,dict):
            if 'species' in x and 'level' in x:
                safari[str(species[x['species']])].add(x['level'])
            for v in x.values():walk(v)
        elif isinstance(x,list):
            for v in x:walk(v)
    walk(json.loads((root/'files/arc/safari_enc.json').read_text()))
    # Ignore evolution conditions/levels: this overapproximates every possible
    # pre-evolution and cannot reject a history because a narrower route was missed.
    previous={str(s):{s} for s in set(species.values()) if 0<s<=493}
    evolutions=json.loads((root/'files/poketool/personal/evo.json').read_text())['evoTable']
    changed=True
    while changed:
        changed=False
        for row in evolutions:
            base=species[row['baseSpecies']]
            for evo in row['evos']:
                target=species[evo['target']]
                before=len(previous[str(target)])
                previous[str(target)]|=previous[str(base)]
                changed|=len(previous[str(target)])!=before
    ordinary=collections.defaultdict(set)
    rows,_=native_rows(root)
    for version,location,typ,rate,species,lo,hi,slot in rows:
        # Only this non-Safari species is used for absence proofs. Its native
        # wild sources are ordinary water/rod slots, not headbutt/contest.
        if species in (129,130) and location!=202:
            ordinary[f'{version}/{location}/{species}'].update(range(lo,hi+1))
    return {'native_commit':pins['commit'],
        'scope':'Necessary native wild levels only. Safari union ignores all block/time/day conditions; ordinary Magikarp union ignores RNG/rod/lead constraints. No positive encounter or historical authentication proof.',
        'source_pins':['HGSS_SWARM_SOURCE_PINS.json','HGSS_SAFARI_SOURCE_PINS.json'],
        'safari':{k:sorted(v) for k,v in sorted(safari.items(),key=lambda x:int(x[0]))},
        'possible_origin_species':{k:sorted(v) for k,v in sorted(previous.items(),key=lambda x:int(x[0]))},
        'ordinary_magikarp_family':{k:sorted(v) for k,v in sorted(ordinary.items())}}


def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True)
    p.add_argument('--fetch',action='store_true');p.add_argument('--check',action='store_true');a=p.parse_args()
    text=json.dumps(generate(a.native,a.fetch),indent=2)+'\n'
    if a.check:
        assert INDEX.read_text()==text,'native level index differs from pinned original data'
    else:INDEX.write_text(text)
    print('Native HGSS wild level index:',hashlib.sha256(text.encode()).hexdigest(),'PASS')

if __name__=='__main__':main()
