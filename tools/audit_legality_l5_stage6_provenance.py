#!/usr/bin/env python3
"""L5 Stage 6: fail-closed original-history, 247-card and permission auditing.

This is an evidence *integrity* checker, not an authentication authority.
Never imports third-party event/save binaries, and never deems a Pokemon Legal.
"""
from __future__ import annotations
import argparse
from collections import Counter
import copy
import csv
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'docs/legality/research/l5'
PIN={
 'Q07_WC4_EVENT_PROVENANCE_JOIN.json':'8261720c96cede640bff89aa091ea0ac5f1e4029',
 'Q08_REDISTRIBUTION_GATE.json':'7cd3d0a227dc9b008f7c304581a2d7e8d85f06e9',
 'Q09_FIXTURE_PROVENANCE.csv':'7b05973a077d044f46884719d41a91fe64372690',
 'Q10_PAIRED_ORIGINAL_SOURCE_REQUESTS.json':'4376cd1dfc2fe53ae42a4bcfa1c6fb4720b4818c',
}
SHA256=re.compile(r'^[0-9a-f]{64}$')
SCHEMA='L5_STAGE6_ORIGINAL_CHAIN_OF_CUSTODY_V1'

def blob(raw:bytes)->str:
 return hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()

def pin_and_load()->tuple[dict,dict,list[dict],dict]:
 found={}
 for name,expected in PIN.items():
  p=DATA/name
  if not p.is_file():raise RuntimeError('full repository source missing: '+str(p))
  raw=p.read_bytes();sha=blob(raw)
  if sha!=expected:raise RuntimeError(f'original manifest drift: {name}: {sha} != {expected}')
  found[name]=raw
 with (DATA/'Q09_FIXTURE_PROVENANCE.csv').open(encoding='utf8',newline='') as f:
  rows=list(csv.DictReader(f))
 return (json.loads(found['Q07_WC4_EVENT_PROVENANCE_JOIN.json']),
         json.loads(found['Q08_REDISTRIBUTION_GATE.json']),rows,
         json.loads(found['Q10_PAIRED_ORIGINAL_SOURCE_REQUESTS.json']))

def require(ok:bool,reason:str)->None:
 if not ok:raise ValueError(reason)

def validate(q07:dict,q08:dict,q09:list[dict],q10:dict)->dict:
 require(q07.get('schema')=='L5_Q07_GEN4_247_EVENT_ARCHIVE_JOIN_V1','WC4_SCHEMA')
 require(q07.get('catalog_id')=='Q07','WC4_CATALOG')
 source=q07.get('source',{});gallery=q07.get('gallery',{})
 require(source.get('engine_revision')=='d29eada5c087e201e00c749703f1a2de3ddfadeb','SOURCE_COMMIT')
 require(source.get('header_path')=='include/Legality/Gen4EventTemplateData.inc','SOURCE_PATH')
 require(source.get('blob_sha')=='6dd1fea4a9ba3669f957fbd4e70d076cb0f85529','SOURCE_BLOB')
 require(source.get('projection_records')==247,'SOURCE_COUNT')
 require(source.get('pkhex_revision')=='6501f0ab46e8f8ca048539dbaf8cae8cb104e722','PKHEX_SOURCE')
 require(gallery.get('repo')=='projectpokemon/EventsGallery' and
         gallery.get('revision')=='154d81be88453f6f78ec1d6d86e85fe0f2f5c240','GALLERY_SOURCE')
 require(q07.get('copying_binaries_permitted') is False,'ORIGINAL_BINARY_IMPORT')
 require(q07.get('defaultGrade')=='unknown','DEFAULT_GRADE')
 data=q07.get('rows')
 require(isinstance(data,list) and len(data)==247,'NOT_247_ROWS')
 keys=set(); tuples=set();card_ids=[]
 for i,row in enumerate(data):
  require(isinstance(row,dict),'MALFORMED_ROW')
  require(type(row.get('template_index')) is int and row['template_index']==i,'INDEX_ORDER_OR_DUPLICATE')
  require(type(row.get('source_line')) is int and row['source_line']==i+6,'SOURCE_LINE')
  parts=[]
  for field in ('species','tid','sid','card_id'):
   v=row.get(field)
   require(type(v) is int and 0<=v<=65535,'NATIVE_ID_DOMAIN:'+field)
   parts.append(v)
  species,tid,sid,card=parts
  require(1<=species<=493,'NATIVE_SPECIES_DOMAIN')
  exact=f'wc4_6501f0ab:{i}:{species}:{tid}:{sid}:{card}'
  require(row.get('identity_key')==exact,'IDENTITY_SUBSTITUTION')
  key=(i,species,tid,sid,card)
  require(key not in tuples and exact not in keys,'DUPLICATE_WC4_TUPLE')
  keys.add(exact);tuples.add(key);card_ids.append(card)
  require(row.get('source_projection')=='accepted PKHeX wc4.pkl converted resource, structural-only','FALSE_ORIGINAL_SOURCE')
  require(row.get('archive_path') is None,'UNATTESTED_ARCHIVE_PATH')
  require(row.get('original_archive_sha256') is None,'UNATTESTED_ORIGINAL_HASH')
  require(row.get('archive_provenance')=='unverified','UNAUTHENTICATED_ARCHIVE_PROMOTION')
  for f in ('original_payload_moves','original_payload_ribbons','original_raw_trainer_bytes',
            'distribution_language_window','original_release_receipt'):
   require(row.get(f)=='unknown','ORIGINAL_WITNESS_PROMOTION:'+f)
  require(row.get('license_redistribution_clearance')=='not established','UNVERIFIED_PERMISSION')
  require(row.get('verdict_policy')=='Unknown until a specific source-backed original release is authenticated','FALSE_LEGALITY_VERDICT')
  require(type(row.get('native_game_code')) is int and type(row.get('native_language_code')) is int,'ORIGINAL_GAME_LANGUAGE')
 frequency=Counter(card_ids); repeated=sum(v for v in frequency.values() if v>1)
 metrics=q07.get('metrics',{})
 for name,value in {'templates':247,'unique_card_ids':len(frequency),
                    'repeated_card_id_templates':repeated,
                    'duplicate_cards':sum(v>1 for v in frequency.values()),
                    'original_card_receipts_verified':0,
                    'archive_item_sha256_verified':0}.items():
  require(metrics.get(name)==value,'WC4_METRIC_DRIFT:'+name)
 require(len(frequency)==70 and repeated==219 and sum(v>1 for v in frequency.values())==42,
         'WC4_CARD_COLLISION_DISTRIBUTION')
 require(q08.get('schema')=='L5_Q08_REDISTRIBUTION_GATE_V1','LICENSE_SCHEMA')
 require(q08.get('not_legal_opinion') is True,'LICENSE_SCOPE')
 require(q08.get('project_license_observed')=='AGPL-3.0','LICENSE_PROJECT')
 require(q08.get('pkhex_reference_license_observed')=='GPL-3.0','LICENSE_REFERENCE')
 dispositions=q08.get('dispositions',{})
 for k in ('third_party_code_import','bulk_retail_table_import','distribution_ROM_or_event_payload'):
  require(dispositions.get(k)=='BLOCKED','UNAUTHORIZED_REDISTRIBUTION:'+k)
 require(dispositions.get('native_original_save_from_user')=='consent_and_data_protection_required','CONSENT_GATE')
 repo_rows=q08.get('repo_rows',[])
 require(len(repo_rows)==14 and len({r.get('repository') for r in repo_rows})==14,'REFERENCE_REPOS')
 for r in repo_rows:
  require(r.get('legal_review_complete') is False and not r.get('legal_basis_or_grant') in (None,''),
          'FALSE_LICENSE_REVIEW')
  for k in ('copy_source_code','import_original_retail_tables','redistribute_event_or_pokemon_payloads'):
   require(r.get(k)=='blocked_pending_review','COPY_PERMISSION_ESCALATION')
 require(len(q09)==12 and len({r.get('fixture_id') for r in q09})==12,'SYNTHETIC_FIXTURE_REGISTER')
 require(all(r.get('missing_original_stage') and r.get('provenance_grade') for r in q09),'FIXTURE_ORIGINAL_CLAIM')
 require(q10.get('schema')=='L5_Q10_AUTHENTICATED_PAIRED_SOURCE_REQUEST_V1','HISTORY_SCHEMA')
 reqs=q10.get('source_witness_requests',[])
 require(q10.get('scenarios')==22 and len(reqs)==22,'REQUIRED_HISTORY_22')
 require(len({r.get('request_id') for r in reqs})==22,'HISTORY_ID_DUPLICATES')
 for r in reqs:
  require(r.get('source_exhaustiveness')=='unverified','ORIGINAL_HISTORY_ENUMERATION')
  require(r.get('original_paired_fixture_digest') is None and
          r.get('original_data_consent') is None,'UNATTESTED_ORIGINAL_PAIR')
  require(r.get('independently_approved_source') is False,'UNATTESTED_APPROVAL')
  require(r.get('source_state')=='missing_original_history','MISSING_HISTORY_ESCALATION')
  require(r.get('expected_verdict_without_external_stage')=='Incomplete/Unknown','UNSUPPORTED_INVALID_VERDICT')
 for k in ('verified_native_original_pairs','verified_distribution_redeem_receipts','license_ready_archives'):
  require(q10.get(k)==0,'FALSE_ORIGINAL_CERTIFICATE:'+k)
 return {'schema':SCHEMA,'integrity':'PASS_ORIGINAL_MANIFEST_SOURCE_PINNED',
         'event_template_rows_indexed':247,'unique_template_identities':247,
         'unique_card_ids':70,'ambiguous_card_id_templates':219,
         'archive_original_payload_sha256_authenticated':0,
         'authenticated_original_distribution_receipts':0,
         'historical_source_requests_missing':22,'source_registry_reference_count':14,
         'original_pair_verified':0,'redistribution_permission':'BLOCKED',
         'whole_pokemon_legality':'NOT_EVALUATED','release_gate':'BLOCKED'}

def canonical(value:object)->bytes:
 return json.dumps(value,sort_keys=True,ensure_ascii=True,separators=(',',':')).encode('utf8')

def receipt_chain(packet:dict,external_head:str|None=None)->dict:
 """Hash-chain integrity ONLY; externally trusted signer/consent never inferred."""
 require(packet.get('schema')=='L5_STAGE6_OFF_REPO_RECEIPT_CHAIN_V1','CHAIN_SCHEMA')
 entries=packet.get('entries')
 require(isinstance(entries,list) and 1<=len(entries)<=64,'CHAIN_LENGTH')
 prior='0'*64;identities=set()
 for i,row in enumerate(entries):
  require(type(row) is dict,'CHAIN_ENTRY')
  require(type(row.get('sequence')) is int and row['sequence']==i+1,'CHAIN_SEQUENCE')
  require(type(row.get('source_identity')) is str and bool(row['source_identity']),'CHAIN_SOURCE')
  require(type(row.get('original_route_identity')) is str and bool(row['original_route_identity']),'CHAIN_ROUTE')
  identity=(row['source_identity'],row['original_route_identity'],row['sequence'])
  require(identity not in identities,'CHAIN_ID_DUPLICATE');identities.add(identity)
  require(row.get('previous_sha256')==prior,'CHAIN_PARENT_MUTATION')
  digest=row.get('content_sha256')
  require(type(digest) is str and bool(SHA256.fullmatch(digest)),'CHAIN_DIGEST_DOMAIN')
  expected=hashlib.sha256(canonical({k:v for k,v in row.items() if k!='entry_sha256'})).hexdigest()
  require(row.get('entry_sha256')==expected,'CHAIN_CONTENT_TAMPER')
  prior=expected
 require(packet.get('head_sha256')==prior,'CHAIN_HEAD_SUBSTITUTION')
 if external_head is not None:
  require(external_head==prior,'CHAIN_EXTERNAL_HEAD_MISMATCH')
 return {'cryptographic_record_linkage':'INTEGRITY_ONLY',
         'locally_observed_entries':len(entries),
         'external_anchor_matched':external_head is not None,
         'original_owner_consent':'NOT_INDEPENDENTLY_VERIFIED',
         'original_archive_payload':'NOT_INDEPENDENTLY_AUTHENTICATED',
         'release_gate':'BLOCKED','redistribution':'BLOCKED'}

def synthetic()->tuple[dict,dict,list[dict],dict]:
 cards=[0]*137+[i for i in range(1,42) for _ in range(2)]+list(range(42,70))
 assert len(cards)==247
 rows=[]
 for i,card in enumerate(cards):
  species=i%493+1;tid=(7000+i)%65536;sid=(9000+i*3)%65536
  rows.append({'template_index':i,'source_line':i+6,
   'identity_key':f'wc4_6501f0ab:{i}:{species}:{tid}:{sid}:{card}',
   'species':species,'tid':tid,'sid':sid,'card_id':card,
   'native_game_code':7,'native_language_code':1,
   'source_projection':'accepted PKHeX wc4.pkl converted resource, structural-only',
   'archive_path':None,'original_archive_sha256':None,'archive_provenance':'unverified',
   'original_payload_moves':'unknown','original_payload_ribbons':'unknown',
   'original_raw_trainer_bytes':'unknown','distribution_language_window':'unknown',
   'original_release_receipt':'unknown',
   'license_redistribution_clearance':'not established',
   'verdict_policy':'Unknown until a specific source-backed original release is authenticated'})
 q7={'schema':'L5_Q07_GEN4_247_EVENT_ARCHIVE_JOIN_V1','catalog_id':'Q07',
     'source':{'engine_revision':'d29eada5c087e201e00c749703f1a2de3ddfadeb',
       'header_path':'include/Legality/Gen4EventTemplateData.inc',
       'blob_sha':'6dd1fea4a9ba3669f957fbd4e70d076cb0f85529',
       'projection_records':247,'pkhex_revision':'6501f0ab46e8f8ca048539dbaf8cae8cb104e722'},
     'gallery':{'repo':'projectpokemon/EventsGallery',
       'revision':'154d81be88453f6f78ec1d6d86e85fe0f2f5c240'},
     'rows':rows,'defaultGrade':'unknown','copying_binaries_permitted':False,
     'metrics':{'templates':247,'unique_card_ids':70,'repeated_card_id_templates':219,
                'duplicate_cards':42,'original_card_receipts_verified':0,
                'archive_item_sha256_verified':0}}
 repos=[]
 for i in range(14):
  repos.append({'repository':f'original-src-{i}','legal_review_complete':False,
    'legal_basis_or_grant':'not documented in pinned registry',
    'copy_source_code':'blocked_pending_review',
    'import_original_retail_tables':'blocked_pending_review',
    'redistribute_event_or_pokemon_payloads':'blocked_pending_review'})
 q8={'schema':'L5_Q08_REDISTRIBUTION_GATE_V1','not_legal_opinion':True,
     'project_license_observed':'AGPL-3.0','pkhex_reference_license_observed':'GPL-3.0',
     'repo_rows':repos,'dispositions':{
      'third_party_code_import':'BLOCKED',
      'bulk_retail_table_import':'BLOCKED',
      'distribution_ROM_or_event_payload':'BLOCKED',
      'native_original_save_from_user':'consent_and_data_protection_required'}}
 q9=[{'fixture_id':f'SYN-{i}','missing_original_stage':'not original',
      'provenance_grade':'synthetic'} for i in range(12)]
 requests=[{'request_id':f'SYN-{i}','source_exhaustiveness':'unverified',
    'original_paired_fixture_digest':None,'original_data_consent':None,
    'independently_approved_source':False,'source_state':'missing_original_history',
    'expected_verdict_without_external_stage':'Incomplete/Unknown'} for i in range(22)]
 q10={'schema':'L5_Q10_AUTHENTICATED_PAIRED_SOURCE_REQUEST_V1',
      'source_witness_requests':requests,'scenarios':22,
      'verified_native_original_pairs':0,'verified_distribution_redeem_receipts':0,
      'license_ready_archives':0}
 return q7,q8,q9,q10

def make_chain()->dict:
 rows=[];prev='0'*64
 for i in range(4):
  row={'sequence':i+1,'source_identity':'synthetic-save',
       'original_route_identity':'synthetic-route',
       'previous_sha256':prev,
       'content_sha256':hashlib.sha256(f'synthetic:{i}'.encode()).hexdigest()}
  prev=row['entry_sha256']=hashlib.sha256(canonical(row)).hexdigest()
  rows.append(row)
 return {'schema':'L5_STAGE6_OFF_REPO_RECEIPT_CHAIN_V1','entries':rows,'head_sha256':prev}

def selftest()->None:
 original=synthetic();res=validate(*original)
 assert res['release_gate']=='BLOCKED' and res['event_template_rows_indexed']==247
 controls=[
  (0,lambda x:x['rows'].pop()),(0,lambda x:x['rows'][12].update(template_index=11)),
  (0,lambda x:x['rows'][12].update(source_line=250)),
  (0,lambda x:x['rows'][12].update(card_id=33)),
  (0,lambda x:x['rows'][12].update(identity_key='card-only:0')),
  (0,lambda x:x['rows'][12].update(archive_path='FAKE.wc4')),
  (0,lambda x:x['rows'][12].update(original_archive_sha256='0'*64)),
  (0,lambda x:x['rows'][12].update(archive_provenance='original_authenticated')),
  (0,lambda x:x['rows'][12].update(original_payload_moves='attested')),
  (0,lambda x:x['rows'][12].update(license_redistribution_clearance='yes')),
  (0,lambda x:x.update(copying_binaries_permitted=True)),
  (0,lambda x:x['metrics'].update(archive_item_sha256_verified=1)),
  (0,lambda x:x['metrics'].update(unique_card_ids=247)),
  (1,lambda x:x['dispositions'].update(distribution_ROM_or_event_payload='ALLOWED')),
  (1,lambda x:x['repo_rows'][1].update(legal_review_complete=True)),
  (1,lambda x:x['repo_rows'][1].update(copy_source_code='allowed')),
  (1,lambda x:x['repo_rows'].pop()),
  (2,lambda x:x[0].update(missing_original_stage='')),
  (3,lambda x:x.update(verified_native_original_pairs=1)),
  (3,lambda x:x['source_witness_requests'][1].update(original_data_consent=True)),
  (3,lambda x:x['source_witness_requests'][2].update(source_exhaustiveness='complete')),
  (3,lambda x:x['source_witness_requests'].pop()),
 ]
 for idx,mutation in controls:
  args=copy.deepcopy(original);mutation(args[idx])
  try:validate(*args)
  except ValueError:pass
  else:raise AssertionError(f'unsafe original promotion {idx} accepted')
 packet=make_chain();assert receipt_chain(packet)['release_gate']=='BLOCKED'
 assert receipt_chain(packet,packet['head_sha256'])['external_anchor_matched']
 for mutation in (
  lambda x:x['entries'][2].update(sequence=4),
  lambda x:x['entries'][2].update(content_sha256='0'*64),
  lambda x:x['entries'][2].update(previous_sha256='0'*64),
  lambda x:x['entries'][2].update(original_route_identity='altered'),
  lambda x:x.update(head_sha256='0'*64),
  lambda x:x['entries'].append(copy.deepcopy(x['entries'][-1])),
 ):
  q=copy.deepcopy(packet);mutation(q)
  try:receipt_chain(q)
  except ValueError:pass
  else:raise AssertionError('tampered synthetic SHA256 chain accepted')
 try:receipt_chain(packet,'0'*64)
 except ValueError:pass
 else:raise AssertionError('forged external trust anchor accepted')
 print('L5 Stage6 manifest/chain-of-custody self-test PASS: 31 unsafe promotions/tamper controls rejected, no original data')

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--self-test',action='store_true')
 parser.add_argument('--receipt-chain',type=Path)
 parser.add_argument('--trusted-external-head')
 parser.add_argument('--json-out',type=Path)
 args=parser.parse_args()
 if args.self_test:selftest();return
 report=validate(*pin_and_load())
 report['source_pins_verified']=PIN
 if args.receipt_chain:
  report['off_repo_chain']=receipt_chain(json.loads(args.receipt_chain.read_text()),args.trusted_external_head)
 elif args.trusted_external_head:
  parser.error('--trusted-external-head requires --receipt-chain')
 text=json.dumps(report,sort_keys=True,indent=2)+'\n'
 print(text,end='')
 if args.json_out:args.json_out.write_text(text,encoding='utf8')
if __name__=='__main__':main()
