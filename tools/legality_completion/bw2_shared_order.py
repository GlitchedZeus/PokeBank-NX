#!/usr/bin/env python3
"""Independent necessary shared-order proof; never alters raw reference validity."""
import functools
import hashlib
import json
import pathlib

ROOT=pathlib.Path(__file__).resolve().parents[2]
INDEX_SHA256='af6c37f285b684e4ff2c58ded52e1f438541ac489d8bc331f1e06307ba24357f'


@functools.cache
def index():
    raw=(ROOT/'docs/legality/campaign-20261028/BW2_SHARED_ORDER_INDEX.json').read_bytes()
    assert hashlib.sha256(raw).hexdigest()==INDEX_SHA256
    data=json.loads(raw)
    assert data['reference']=='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'
    assert data['mechanism_reference']=='9cb65bafa3e0198e6951a2fda6cc1c3470c67419'
    assert data['mechanism_blob']=='17ab01ff83b334e153ebf6b989bb7c8b8bf2fd09'
    assert data['exporter_sha256']==hashlib.sha256((ROOT/'tools/legality_completion/breeding_reference_trace/Program.cs').read_bytes()).hexdigest()
    assert sorted(map(int,data['entries']))==list(range(1,650))
    return data['entries']


def necessary_order_proof(hexdata,encounter):
    if not encounter.startswith('EncounterEgg5:'):return None
    try:raw=bytes.fromhex(hexdata)
    except ValueError:return None
    if len(raw) not in (136,220):return None
    u16=lambda at:int.from_bytes(raw[at:at+2],'little')
    if raw[0x17]!=2 or raw[0x5f] not in (22,23) or not (int.from_bytes(raw[0x38:0x3c],'little')&(1<<30)):return None
    if raw[0x40]&1 or raw[0x40]>>3 or u16(0x7e)!=60002 or u16(0x80)!=0:return None
    if sum(u16(at) for at in range(8,136,2))&65535!=u16(6):return None
    entry=index().get(str(u16(8)))
    if entry is None:return None
    alternatives=set(entry['initial'])|set(entry['egg'])|set(entry['machines'])
    if u16(8)==172:alternatives.add(344)
    required=[]
    for slot in range(4):
        move=u16(0x28+2*slot)
        if not move:break
        if move in entry['levels'] and move not in alternatives:required.append(move)
    # Any initial/egg/TM alternative was allowed above. The remaining moves
    # can only be appended by the child-learnset traversal. Even extra unseen
    # parental moves cannot reverse their relative order.
    remaining=iter(entry['levels'])
    for move in required:
        if not any(candidate==move for candidate in remaining):
            return {'basis':'native-bw2-shared-learnset-order','species':u16(8),
                    'required_shared_order':required,'native_level_order':entry['levels'],
                    'sha256':hashlib.sha256(raw).hexdigest()}
    return None
