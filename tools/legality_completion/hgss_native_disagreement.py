#!/usr/bin/env python3
"""Conservative native wild-level absence, separate from raw counts/oracle verdicts.

All original evolution predecessors and all Safari areas/times/bonus slots are
unioned. Membership cannot prove a capture; absence can prove this necessary rule.
Erased transfer met levels are bounded by the transfer level, never reconstructed.
"""
import json, pathlib

INDEX=pathlib.Path(__file__).resolve().parents[2]/'docs/legality/campaign-20261028/HGSS_NATIVE_LEVEL_INDEX.json'
DATA=json.loads(INDEX.read_text())
ENCOUNTER='No matching Gen IV encounter for this species, level, location, ball and PID/IV'
TILE='Encounter ground tile type does not match the encounter'


def necessary_level_proof(hexdata,enc,transferred=False):
    b=bytes.fromhex(hexdata)
    if len(b)!=136 or b[0x5f] not in (7,8):return None
    u16=lambda i:int.from_bytes(b[i:i+2],'little')
    if u16(4)!=0 or (sum(u16(i) for i in range(8,136,2))&65535)!=u16(6):return None
    if u16(0x7e) or int.from_bytes(b[0x38:0x3c],'little')&(1<<30):return None
    kind,_,origin=enc.partition(':')
    if kind!='EncounterSlot4' or not origin.isdigit():return None
    species=u16(8);origins=DATA['possible_origin_species'].get(str(species),[])
    if int(origin) not in origins:return None
    level=b[0x84]&127;location=u16(0x80) if transferred else (u16(0x46) or u16(0x80))
    if transferred:
        if location!=30001:return None
        ball=b[0x83]
    else:
        if u16(0x44):return None
        ball=max(b[0x83],b[0x86])
    if ball==5 and (transferred or location==202):
        possible=sorted({lv for s in origins for lv in DATA['safari'].get(str(s),[])})
        basis='native-safari-level-absence'
    elif not transferred and ball!=5 and location!=202 and set(origins)<={129,130}:
        possible=sorted({lv for s in origins for lv in DATA['ordinary_magikarp_family'].get(f'{b[0x5f]}/{location}/{s}',[])})
        basis='native-magikarp-family-wild-level-absence'
    else:return None
    # In a PK5 the transfer writer overwrites met level. Any earlier acquisition
    # at or below it remains possible; a discarded old location proves nothing.
    if (any(lv<=level for lv in possible) if transferred else level in possible):return None
    return {'basis':basis,'native_commit':DATA['native_commit'],'species':species,
        'possible_origin_species':origins,'version':b[0x5f],'location':location,
        'stored_met_level':level,'transferred':transferred,'native_possible_levels':possible,
        'scope':'Necessary native wild-level absence. No RNG sufficiency or acquisition custody claim.'}


def proof(row):
    if row['p']!='VALID' or row['ours']!='INVALID':return None
    return necessary_level_proof(row['hex'],row['enc'])
