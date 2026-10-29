#!/usr/bin/env python3
"""Remove only the native contest bank's inactive ordinary-grass projection.

An exact whole-bank fingerprint preserves ordinary National Park grass, all
dedicated contest slots and every other encounter. No specimen IDs are used.
"""
import argparse
import collections
import hashlib
import json
import pathlib
import re
import urllib.request
from hgss_native_catalog import constants
from hgss_swarm_repair import parse_array, render, PROJECT, CORRECTION as ROD_NOTE
from breeding_native_witness import body

PINS=PROJECT/'docs/legality/campaign-20261028/HGSS_CONTEST_SOURCE_PINS.json'
NOTE='// Native HGSS contest-bank correction: tools/legality_completion/hgss_contest_repair.py; HGSS_CONTEST_SOURCE_PINS.json.\n'


def annotate(text):
    if NOTE in text:return text
    return text.replace(ROD_NOTE,ROD_NOTE+NOTE,1) if ROD_NOTE in text else NOTE+text


def checked_native(root,fetch=False):
    pins=json.loads(PINS.read_text())
    for row in pins['files']:
        path=root/row['path']
        if fetch and not path.exists():
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(urllib.request.urlopen(row['url'],timeout=30).read())
        data=path.read_bytes()
        assert hashlib.sha256(data).hexdigest()==row['sha256'],row['path']
        assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==row['git_blob_sha1'],row['path']
    # Static cross-check of the pinned entry/dispatch chain, not a ROM/gameplay
    # replay. The ordinary map clears the contest flag; entry initializes the
    # contest state and flag before its gate scripts warp to the separate bank.
    flags=body((root/'src/sys_flags.c').read_text(),'BOOL Save_VarsFlags_CheckBugContestFlag(')
    assert 'CheckScriptFlag(state, FLAG_UNK_996)' in flags
    scripts=root/'files/fielddata/script/scr_seq'
    common=(scripts/'scr_seq_0151.s').read_text()
    sequence=['SetVar VAR_UNK_4118, 1','BugContestAction 0, VAR_TEMP_x4000','SetFlag FLAG_UNK_996']
    assert [common.index(x) for x in sequence]==sorted(common.index(x) for x in sequence)
    assert 'ClearFlag FLAG_UNK_996' in (scripts/'scr_seq_0025_D22R0101.s').read_text()
    for name in ['scr_seq_0242_R35R0201.s','scr_seq_0245_R36R0201.s']:
        gate=(scripts/name).read_text()
        assert 'Compare VAR_UNK_4118, 1' in gate and 'Warp MAP_NATIONAL_PARK_BUG_CATCHING_CONTEST' in gate
    encounter=(root/'src/field/encounter_check.c').read_text()
    assert 'else if (bugContestFlag)' in encounter
    inner=body(encounter,'static BOOL FieldSystem_GenerateBugContestEncounter_Internal(')
    assert 'BugContest_GetEncounterSlot(FieldSystem_BugContest_Get(fieldSystem)' in inner
    assert 'encData' not in inner and 'encSlots' not in inner
    return pins


def native_bank(root):
    headers=(root/'src/data/map_headers.h').read_text()
    name='MAP_NATIONAL_PARK_BUG_CATCHING_CONTEST'
    body=re.search(r'\['+name+r'\]\s*=\s*\{(.*?)\n\s*\},',headers,re.S)[1]
    bank=re.search(r'\.wildEncounterBank\s*=\s*ENCDATA_(\w+)',body)[1]
    aliases=[name for name,entry in re.findall(r'\[(MAP_\w+)\]\s*=\s*\{(.*?)\n\s*\},',headers,re.S)
             if re.search(r'\.wildEncounterBank\s*=\s*ENCDATA_'+bank+r'\b',entry)]
    assert aliases==[name],aliases
    section=re.search(r'\.mapsec\s*=\s*(MAPSEC_\w+)',body)[1]
    location=constants(root/'include/constants/map_sections.h','MAPSEC_')[section]
    raw=json.loads((root/'files/fielddata/encountdata/gs_enc_data.json').read_text())['encounters']
    data=next(row for row in raw if row['map']==bank)
    species=constants(root/'include/constants/species.h','SPECIES_')
    fingerprints={}
    for version,label in [(7,'HEARTGOLD'),(8,'SOULSILVER')]:
        fingerprint=[]
        for time in ('morn','day','nite'):
            for slot,mon in enumerate(data['land']['mons']):
                value=mon['species'][time]
                if isinstance(value,dict):value=value[label]
                fingerprint.append((species[value],0,mon['level'],mon['level'],slot))
        fingerprints[version]=collections.Counter(fingerprint)
    return bank,location,data['land']['rate'],fingerprints


def repaired(source,root):
    bank,location,rate,fingerprints=native_bank(root)
    areas=parse_array(source,'Area4 kAreas[]');slots=parse_array(source,'Slot4 kSlots[]')
    kept_areas=[];kept_slots=[];removed=[]
    for area in areas:
        version,loc,typ,frequency,tile,start,count=area
        rows=slots[start:start+count]
        signature=collections.Counter((row[0],row[1],row[2],row[3],row[4]) for row in rows)
        if version in fingerprints and loc==location and typ==0 and frequency==rate and tile==4 and signature==fingerprints[version]:
            removed.append(dict(native_bank=bank,area=area,slots=rows))
            continue
        kept_areas.append([version,loc,typ,frequency,tile,len(kept_slots),count]);kept_slots.extend(rows)
    assert len(removed) in (0,2),len(removed)
    if removed:assert sorted(row['area'][0] for row in removed)==[7,8]
    result=source
    for signature,rows in [('Area4 kAreas[]',kept_areas),('Slot4 kSlots[]',kept_slots)]:
        pattern=r'const '+re.escape(signature)+r'\s*=\s*\{.*?\n\};'
        result,count=re.subn(pattern,lambda _:render(signature,rows),result,count=1,flags=re.S);assert count==1
    return annotate(result),removed,len(kept_areas),len(kept_slots)


def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True)
    p.add_argument('--fetch',action='store_true');p.add_argument('--apply',action='store_true')
    p.add_argument('--source',type=pathlib.Path,default=PROJECT/'src/Legality/Gen4LegalityData.cpp')
    p.add_argument('--header',type=pathlib.Path,default=PROJECT/'include/Legality/Gen4LegalityData.h')
    p.add_argument('--report',type=pathlib.Path,required=True);a=p.parse_args()
    pins=checked_native(a.native,a.fetch);original=a.source.read_text()
    result,removed,areas,slots=repaired(original,a.native)
    report=dict(native_commit=pins['commit'],scope=pins['scope'],removed=removed,
                before_sha256=hashlib.sha256(original.encode()).hexdigest(),
                candidate_sha256=hashlib.sha256(result.encode()).hexdigest(),areas=areas,slots=slots)
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    if a.apply:
        a.source.write_text(result);header=a.header.read_text()
        for name,count in [('kAreaCount',areas),('kSlotCount',slots)]:
            header,n=re.subn(r'('+name+r'\s*=\s*)\d+',lambda m:m[1]+str(count),header,count=1);assert n==1
        a.header.write_text(annotate(header))
    else:
        assert not removed,'Contest-only ordinary bank still present'
        for name,count in [('kAreaCount',areas),('kSlotCount',slots)]:
            assert int(re.search(name+r'\s*=\s*(\d+)',a.header.read_text())[1])==count
    print(f'Native contest-bank repair:{len(removed)} inactive areas removed;{areas} areas,{slots} slots retained PASS')


if __name__=='__main__':main()
