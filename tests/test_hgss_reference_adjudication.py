#!/usr/bin/env python3
"""Adversarial regression: source proof, never placeholder-membership exemption."""
import copy, hashlib, json, os, pathlib, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/pkhex_gen34_diff'))
sys.path.insert(0,str(ROOT/'tools/legality_completion'))
import hgss_placeholders as gate
from hgss_native_disagreement import necessary_level_proof, ENCOUNTER
from current_release_replay import changed_in_release, PIN

os.environ.pop('CURRENT_RELEASE_REPLAY_REPORT',None)
positive=0
for line in (ROOT/'tests/fixtures/gen15_session2/hgss_swarm.txt').read_text().splitlines():
    ident,game,verdict,hexdata=line.split()
    if verdict!='NOPROBLEMS':continue
    species=int.from_bytes(bytes.fromhex(hexdata)[8:10],'little')
    # Simulate an engine regression. Its rejected verdict must fail the gate,
    # although the old predicate could find a discarded overlapping placeholder.
    assert gate.native('HG',hexdata,f'EncounterSlot4:{species}',[ENCOUNTER]) is None,ident
    positive+=1
assert positive==7
ledger=json.loads((ROOT/'docs/legality/campaign-20261028/HGSS_FROZEN_REFERENCE_ADJUDICATION.json').read_text())
for row in ledger['rows']:
    a=row['reference_line'].split()
    assert necessary_level_proof(a[2],a[4]),row['id']
assert len(ledger['rows'])==91
# A modern reference error cannot become a deliberate disagreement; byte hashes
# bind actual unchanged replay results. No specimen ID affects the decision.
with tempfile.TemporaryDirectory() as tmp:
    path=pathlib.Path(tmp)/'REPORT.json'
    digest=hashlib.sha256(bytes.fromhex(a[2])).hexdigest()
    path.write_text(json.dumps({'reference':PIN,'rows':[{'sha256':digest,'release':'INVALID','analysis_error':True}]}))
    os.environ['CURRENT_RELEASE_REPLAY_REPORT']=str(path)
    assert not changed_in_release(a[2])
    path2=pathlib.Path(tmp)/'VALID.json'
    path2.write_text(json.dumps({'reference':PIN,'rows':[{'sha256':digest,'release':'VALID','analysis_error':False}]}))
    os.environ['CURRENT_RELEASE_REPLAY_REPORT']=str(path2)
    assert not changed_in_release(a[2])
    path3=pathlib.Path(tmp)/'INVALID.json'
    path3.write_text(json.dumps({'reference':PIN,'rows':[{'sha256':digest,'release':'INVALID','analysis_error':False}]}))
    os.environ['CURRENT_RELEASE_REPLAY_REPORT']=str(path3)
    assert changed_in_release(a[2])
    assert not changed_in_release('00'*136)
print('HGSS reference gate:7 genuine positives cannot be exempted;91 independent native level proofs; error/valid/byte-identity guards PASS')
