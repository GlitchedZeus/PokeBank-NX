#!/usr/bin/env python3
"""L5 Stage 7 final fail-closed 336-cell and 38-Invalid-site audit.

Checks pinned archived Q01/Q17/Q18 data AND pinned current coordinator
Legality.cpp. An archived green synthetic CI result never certifies
current full analyzer runtime, original history, or PKHeX parity.
"""
from __future__ import annotations
import argparse
import copy
import csv
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'docs/legality/research/l5'
FILES={
 'Q01_FAMILY_GAME_MATRIX.csv':'c28210b889baf10f2fae8902c874b1c67934b8ee',
 'Q17_GAME_FAMILY_PARITY_336.json':'ba31eece267abc10c5d75774d2d8adf3a36503f1',
 'Q17_INTEGRATION_NATIVE_EVIDENCE_336.json':'5c269b60d6be9c9da02f0cb3fd1773d3b8b64d8b',
 'Q18_INTEGRATED_INVALID_37_SITE_AUDIT.json':'a8c4cff07742e5394e5b8db1ea993320ae4fe3f9',
 'Q18_RELEASE_GATES_TRANCHE04.json':'f0c344fcf36a482f8a3147e2da2200b9b6414bca',
}
COORDINATOR_HEAD='ad9c1c859f94c0321a5d17489c72b0e7dbb9f440'
COORDINATOR_SOURCE_BLOB='4ac8a0dc0badb8211eb105811eb3fa9f928e36e4'
HISTORICAL_INTEGRATION_HEAD='c91b53e84ef75669049d159616894ec68cd4b0dd'
HISTORICAL_SOURCE_BLOB='a81cf6b878da50cc85837edf2c8c37ea11357d8a'
GROUPS={'RB':1,'Y':1,'GS':2,'C':2,'R':3,'S':3,'E':3,'FR':3,'LG':3,
        'COLO':3,'XD':3,'D':4,'P':4,'Pt':4,'HG':4,'SS':4}
EXPECTED={(g,i) for g in GROUPS for i in range(1,22)}
INVALID_ADD=re.compile(r'\badd\s*\(\s*r\s*,\s*Severity::Invalid\b')

def require(ok:bool,label:str)->None:
    if not ok:raise ValueError(label)

def blob(path:Path)->str:
    raw=path.read_bytes()
    return hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()

def verify(path:Path,expected:str)->None:
    require(path.is_file(),'SOURCE_MISSING:'+str(path))
    got=blob(path)
    require(got==expected,f'SOURCE_DRIFT:{path}:{got}!={expected}')

def load_originals()->tuple[list[dict],dict,dict,dict,dict]:
    for name,sha in FILES.items():verify(DATA/name,sha)
    with (DATA/'Q01_FAMILY_GAME_MATRIX.csv').open(encoding='utf8',newline='') as inp:
        q01=list(csv.DictReader(inp))
    other=[json.loads((DATA/name).read_text(encoding='utf8')) for name in list(FILES)[1:]]
    return q01,*other

def index(rows:list[dict],group:str,label:str)->dict[tuple[str,int],dict]:
    require(isinstance(rows,list) and len(rows)==336,label+'_COUNT')
    out={}
    for r in rows:
        require(isinstance(r,dict),label+'_MALFORMED')
        try:
            key=(r[group],int(r['family_id']))
        except (TypeError,ValueError,KeyError) as exc:
            raise ValueError(label+'_KEY_MALFORMED') from exc
        require(key in EXPECTED,label+'_OUT_OF_SCOPE')
        require(key not in out,label+'_DUPLICATE')
        require(int(r['generation'])==GROUPS[key[0]],label+'_GEN_DRIFT')
        out[key]=r
    require(set(out)==EXPECTED,label+'_KEY_SET')
    return out

def scan_invalid(src:str)->list[dict]:
    return [{'line':number,'source_line':line.strip(),
             'scope':'REQUIRES_EXACT_ORIGINAL_HISTORY_OR_FORMAT_PROOF',
             'whole_pokemon_verdict_certified':False}
            for number,line in enumerate(src.splitlines(),start=1) if INVALID_ADD.search(line)]

def verdict_grade(matrix:dict,old:dict,new:dict)->dict:
    applicable=matrix['local_applicability']=='potentially_applicable'
    require(matrix['local_applicability'] in ('potentially_applicable','not_native_in_this_format'),
            'BAD_APPLICABILITY')
    require(old['local_applicability']==matrix['local_applicability'],'APPLICABILITY_CHANGED')
    require(old['family_name']==new['family_name']==matrix['family_name'],'FAMILY_DRIFT')
    require(old['accepted_analyzer_current_sha']=='d29eada5c087e201e00c749703f1a2de3ddfadeb',
            'ACCEPTED_SOURCE_DRIFT')
    require(old['parity_grade']=='UNVERIFIED_BEHAVIORAL_PARITY','FABRICATED_HISTORIC_PARITY')
    require(old['real_original_specimen_with_complete_game_history'] is False,
            'FALSE_ORIGINAL_HISTORY')
    require(old['exact_game_specific_runtime_path_executed'] is False,
            'FALSE_PER_GAME_RUNTIME')
    require(old['pkhex_reference_differential_executed'] is False,
            'FALSE_PKHEX_RUNTIME')
    require(old['fully_closed_candidate_set'] is False,'FALSE_CANDIDATE_CLOSURE')
    require(old['cell_passing_native_tests']==0,'FAKE_CELL_TEST_CREDIT')
    require(new['original_retail_history_proven'] is False,'FABRICATED_ORIGINAL')
    require(new['pkhex_exact_game_full_parity_proven'] is False,'FABRICATED_PKHEX_PARITY')
    require(new['game_source_original_method_oracle'] is None,'FABRICATED_METHOD_ORACLE')
    require(new['coverage_grade'] in ('RESEARCH_ONLY_OR_UNTESTED',
        'CONFIRMED_SEMANTIC_DEFECT','PARTIAL_FORMAT_NATIVE_EXECUTION'),'UNKNOWN_GRADE')
    if not applicable:
        require(old['reference_cell']=='not_applicable_native_fields','FALSE_NOT_NATIVE_REFERENCE')
    if new['source_free_false_invalid_in_production_source']:
        require(new['source_contract_reproduced'] is True and
                new['coverage_grade']=='CONFIRMED_SEMANTIC_DEFECT','FALSE_SOURCE_DEFECT')
    return {'game_group':matrix['game_group_id'],'generation':GROUPS[matrix['game_group_id']],
            'family_id':int(matrix['family_id']),'family_name':matrix['family_name'],
            'native_applicability':matrix['local_applicability'],
            'historical_source_grade':new['coverage_grade'],
            'full_pkhex_retail_parity':'BLOCKED_NOT_EXECUTED',
            'current_coordinator_runtime':'NOT_ATTESTED_BY_ARCHIVED_MANIFEST',
            'original_retail_history':'MISSING',
            'certified':False}

def validate(q01:list[dict],base:dict,integrated:dict,risk:dict,gates:dict,
             source:str)->dict:
    require(base.get('schema')=='L5_Q17_336_CELL_EVIDENCE_SCOREBOARD_V1','BASE_SCHEMA')
    require(integrated.get('schema')=='L5_Q17_INTEGRATION_INDEPENDENT_336_V1',
            'INTEGRATED_SCHEMA')
    require(integrated.get('integration_head')==HISTORICAL_INTEGRATION_HEAD,'HISTORICAL_HEAD')
    require(integrated.get('l5_original_scoreboard_blob')==FILES['Q17_GAME_FAMILY_PARITY_336.json'],
            'HISTORICAL_SCOREBOARD_SHA')
    require(integrated.get('total_game_family_cells')==336,'TOTAL_CELLS')
    a=index(q01,'game_group_id','MATRIX')
    b=index(base.get('rows'),'group','BASE')
    c=index(integrated.get('rows'),'game_group','INTEGRATED')
    rows=[]
    for key in sorted(EXPECTED):rows.append(verdict_grade(a[key],b[key],c[key]))
    require(sum(x['native_applicability']=='potentially_applicable' for x in rows)==310,
            'NATIVE_SCOPE_COUNT')
    require(sum(x['native_applicability']=='not_native_in_this_format' for x in rows)==26,
            'NOT_NATIVE_COUNT')
    scores=base.get('scores',{})
    require(scores.get('cells')==336 and scores.get('groups')==16 and
            scores.get('families')==21 and scores.get('not_native')==26 and
            scores.get('potentially_applicable')==310,'SCORE_TAMPER')
    for key in ('pkhex_full_parity_cleared_cells','authenticated_retail_history_cells',
                'full_native_method_per_game_tests_passed'):
        require(scores.get(key)==0,'FALSE_SCORE_CERTIFICATE:'+key)
    for key in ('exact_game_retail_native_parity_complete',
                'original_attested_event_archive_items','original_paired_save_histories_verified'):
        require(integrated.get(key)==0,'FALSE_INTEGRATED_PARITY:'+key)
    require(risk.get('schema')=='L5_INTEGRATED_INVALID_37_SITE_AUDIT_V1','AUDIT_SCHEMA')
    anchor=risk.get('anchor',{})
    require(anchor.get('sha')==HISTORICAL_INTEGRATION_HEAD and
            anchor.get('git_blob_sha')==HISTORICAL_SOURCE_BLOB,'HISTORICAL_AUDIT_SOURCE')
    sites=risk.get('rows',[])
    require(risk.get('direct_sites')==37 and len(sites)==37,'ARCHIVE_37')
    require(len({s.get('site_id') for s in sites})==37,'DUPLICATE_OLD_SITE')
    for site in sites:
        require(site.get('branch_head')==HISTORICAL_INTEGRATION_HEAD and
                site.get('analyzer_git_blob')==HISTORICAL_SOURCE_BLOB,
                'FALSE_CURRENT_AUDIT')
        require(site.get('retail_original_histories_authenticated') is False,
                'FABRICATED_SITE_ORIGINAL')
    require(risk.get('dynamic_severity_add_calls_observed')==0,'UNTRACKED_DYNAMIC_ADDS')
    require(gates.get('release_ready') is False,'FALSE_RELEASE_READY')
    require(gates.get('proven_original_whole_pokemon_bad_verdict_cases')==0,
            'UNVERIFIED_FALSE_INVALID_CASES')
    gs=gates.get('gate_rows',[])
    require(len(gs)==14 and len({x.get('id') for x in gs})==14,'GATE_COUNT')
    require(all(x.get('state')!='READY' for x in gs),'UNJUSTIFIED_READY_GATE')
    found=scan_invalid(source)
    require(len(found)==38,'CURRENT_INVALID_SITE_COUNT_DRIFT')
    require(all(x['line']>0 for x in found),'INVALID_SITE_LINE')
    # 37 archived entries are pinned to c91b, the current source is ad9c.
    # Neither a positional pairing nor a matching count would renew provenance.
    return {'schema':'L5_STAGE7_FINAL_FAIL_CLOSED_DASHBOARD_V1',
            'release_state':'BLOCKED','manifest_structure':'PASS_SOURCE_PINNED',
            'current_coordinator_head':COORDINATOR_HEAD,
            'historical_integration_head':HISTORICAL_INTEGRATION_HEAD,
            'historical_invalid_sites':37,'current_coordinator_direct_invalid_sites':38,
            'current_invalid_site_remapping':'BLOCKED_REQUIRES_FRESH_SITE_BY_SITE_AUDIT',
            'site_certified_original_route_verdicts':0,
            'full_pkhex_retail_parity_cells':0,
            'authenticated_original_history_cells':0,
            'exact_game_retail_native_parity_certified':0,
            'native_applicable':310,'not_native':26,
            'prior_14_release_gates':[{'id':x['id'],'state':x['state']} for x in gs],
            'cell_rows':rows,'current_invalid_emissions':found,
            'blockers':['ARCHIVED_CI_NOT_CURRENT_HEAD',
                        'CURRENT_INVALID_38_VS_ARCHIVED_37',
                        'NO_EXECUTED_FULL_COORDINATOR_ANALYZER',
                        'NO_EXECUTABLE_GAME_SPECIFIC_PKHEX_ORACLE',
                        'NO_PAIRED_ORIGINAL_GAME_HISTORIES',
                        'NO_ATTESTED_ORIGINAL_EVENT_ARCHIVES',
                        'NO_AUTHORIZED_THIRD_PARTY_PAYLOAD_REUSE']}


FROZEN_SNAPSHOT='Q21_STAGE7_FROZEN_DASHBOARD_336_38.json'
FROZEN_BLOB='15f4ba61ca069d87f6cd7a7c65e9da3fa044d9fb'
def verify_frozen_snapshot(report:dict)->None:
    """Cross-check the separate read-only GitHub source observation."""
    file=DATA/FROZEN_SNAPSHOT
    verify(file,FROZEN_BLOB)
    frozen=json.loads(file.read_text(encoding='utf8'))
    require(frozen.get('schema')=='L5_STAGE7_FROZEN_SOURCE_OBSERVATION_336_38_V1',
            'FROZEN_SCHEMA')
    src=frozen.get('source_refs',{})
    require(src.get('current_coordinator')==COORDINATOR_HEAD and
            src.get('current_coordinator_cpp_git_blob')==COORDINATOR_SOURCE_BLOB,
            'FROZEN_COORDINATOR_DRIFT')
    require(frozen.get('release_state')=='BLOCKED','FROZEN_FALSE_RELEASE')
    metrics=frozen.get('metrics',{})
    require(metrics.get('total')==336 and
            metrics.get('potentially_applicable')==310 and
            metrics.get('not_native')==26 and
            metrics.get('certified_full_retail_pkhex_parity')==0,
            'FROZEN_SCORE_DRIFT')
    rows=frozen.get('cells',[])
    require(len(rows)==len(report['cell_rows'])==336,'FROZEN_ROWS')
    indexed={(r['game_group'],r['family_id']):r for r in rows}
    require(len(indexed)==336,'FROZEN_DUPLICATE_CELL')
    for i,now in enumerate(report['cell_rows']):
        observed=indexed.get((now['game_group'],now['family_id']),{})
        for key in ('game_group','generation','family_id','family_name',
                    'native_applicability','historical_source_grade'):
            require(observed.get(key)==now.get(key),f'FROZEN_CELL_DRIFT:{i}:{key}')
        require(observed.get('certified') is False,'FROZEN_FAKE_CERTIFICATE')
    require(frozen.get('current_invalid_lexical_site_lines')==
            [r['line'] for r in report['current_invalid_emissions']],
            'FROZEN_INVALID_SITE_DRIFT')

def selftest()->None:
    m={'game_group_id':'RB','family_id':'1','family_name':'Structural','generation':'1',
       'local_applicability':'potentially_applicable'}
    b={'local_applicability':m['local_applicability'],'family_name':'Structural',
       'accepted_analyzer_current_sha':'d29eada5c087e201e00c749703f1a2de3ddfadeb',
       'parity_grade':'UNVERIFIED_BEHAVIORAL_PARITY',
       'real_original_specimen_with_complete_game_history':False,
       'exact_game_specific_runtime_path_executed':False,
       'pkhex_reference_differential_executed':False,
       'fully_closed_candidate_set':False,'cell_passing_native_tests':0}
    c={'family_name':'Structural','original_retail_history_proven':False,
       'pkhex_exact_game_full_parity_proven':False,'game_source_original_method_oracle':None,
       'coverage_grade':'RESEARCH_ONLY_OR_UNTESTED',
       'source_free_false_invalid_in_production_source':False,
       'source_contract_reproduced':False}
    assert not verdict_grade(m,b,c)['certified']
    corruptions=[
      (1,'real_original_specimen_with_complete_game_history',True),
      (1,'exact_game_specific_runtime_path_executed',True),
      (1,'pkhex_reference_differential_executed',True),
      (1,'fully_closed_candidate_set',True),
      (1,'cell_passing_native_tests',1),
      (1,'parity_grade','VERIFIED_FULL_PKHEX_PARITY'),
      (1,'accepted_analyzer_current_sha','0'*40),
      (2,'original_retail_history_proven',True),
      (2,'pkhex_exact_game_full_parity_proven',True),
      (2,'game_source_original_method_oracle','fabricated'),
      (2,'coverage_grade','FULL_PARITY'),
      (2,'source_free_false_invalid_in_production_source',True),
      (0,'local_applicability','not_native_in_this_format'),
      (0,'family_name','MISMATCH'),
    ]
    for idx,k,v in corruptions:
        group=[copy.deepcopy(m),copy.deepcopy(b),copy.deepcopy(c)]
        group[idx][k]=v
        try:verdict_grade(*group)
        except ValueError:pass
        else:raise AssertionError('fabricated evidence passed: '+k)
    s='add(r, Severity::Invalid, "a");\n// add(r, Severity::Invalid, "fake");\n'
    # Scanner intentionally counts lexical matches even inside comments;
    # do not interpret its 38 observed call sites as semantic execution.
    assert len(scan_invalid(s))==2
    assert len(scan_invalid('add(r,Severity::Warning, "a");'))==0
    print('L5 Stage7 fail-closed cell/original-source self-test PASS: 14 forged promotions rejected; 3 parser checks')

def main()->int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--coordinator-root',type=Path)
    p.add_argument('--current-coordinator-head',default=COORDINATOR_HEAD)
    p.add_argument('--self-test',action='store_true')
    p.add_argument('--verify-manifests',action='store_true')
    p.add_argument('--json-out',type=Path)
    p.add_argument('--csv-out',type=Path)
    args=p.parse_args()
    if args.self_test:selftest();return 0
    require(args.current_coordinator_head==COORDINATOR_HEAD,
            'COORDINATOR_MOVED_REPIN_AND_REAUDIT')
    require(args.coordinator_root is not None,
            'NO_FULL_CURRENT_COORDINATOR_CHECKOUT_BLOCKED')
    src=args.coordinator_root/'src/Legality/Legality.cpp'
    verify(src,COORDINATOR_SOURCE_BLOB)
    result=validate(*load_originals(),src.read_text(encoding='utf8'))
    verify_frozen_snapshot(result)
    payload=json.dumps(result,indent=2,sort_keys=True)+'\n'
    print(payload,end='')
    if args.json_out:args.json_out.write_text(payload,encoding='utf8')
    if args.csv_out:
        with args.csv_out.open('w',encoding='utf8',newline='') as fp:
            writer=csv.DictWriter(fp,fieldnames=list(result['cell_rows'][0]));writer.writeheader()
            writer.writerows(result['cell_rows'])
    if args.verify_manifests:return 0 # integrity only, never release approval
    return 2 # release blocking is intentional
if __name__=='__main__':raise SystemExit(main())
