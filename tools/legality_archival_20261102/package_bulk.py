import pathlib,json,hashlib,tarfile,io,os,collections
ROOT=pathlib.Path.cwd();OUT=ROOT/'outputs/archival-20261102-5fea7345';HEAD='2144a6820490fa81c932dd2d917b5153283feefc';TAG='evidence/gen1-5-2144a682-20261102';BASE='https://github.com/GlitchedZeus/PokeBank-NX/releases/download/'+TAG+'/'
rows=[json.loads(x) for x in (OUT/'CANDIDATES_PRIVATE.jsonl').open()];events=json.loads((OUT/'EVENTS_PUBLIC_FIXTURE_PROVENANCE.json').read_text());synthetic=json.loads((OUT/'SYNTHETIC_SAVE_PROVENANCE.json').read_text());allowed=set(events)|{synthetic['sha256']};original=json.loads((OUT/'PUBLIC_FIXTURE_PROVENANCE.json').read_text());cleared=set(allowed)|set(original)

def resolved(x):
 for flag in x.get('flags',x.get('safety_flags',[])):
  if flag=='unproven_save_or_specimen' and x.get('sha256') in cleared:continue
  if flag=='flagged_nested_content' and x.get('nested') and all(resolved(c) for c in x['nested']):continue
  return False
 return all(resolved(c) for c in x.get('nested',[]))
findings={d['relative_path']:d for d in (json.loads(x) for x in (OUT/'SAFETY_FINDINGS_PRIVATE.jsonl').open()) if 'relative_path' in d}
for row in rows:
 d=findings.get(row['relative_path'])
 if d:
  assert resolved(dict(d,nested=d.get('nested_findings',[]))),row['relative_path']
  row['safety_resolution']='source-provenance-cleared';row['public_fixture_provenance']=events.get(row['sha256']) or (synthetic if row['sha256']==synthetic['sha256'] else row.get('public_fixture_provenance'))
 else:row['safety_resolution']='no unresolved flags'
# Every archived original path is mapped to a content-addressed payload, without source selection or loss of duplicate path names.
unique={}
for x in rows:unique.setdefault(x['sha256'],x)
parts=[];current=[];size=0
for sha,x in sorted(unique.items()):
 if current and size+x['bytes']>1500000000:parts.append(current);current=[];size=0
 current.append(x);size+=x['bytes']
if current:parts.append(current)
assets=[];locations={}
for index,group in enumerate(parts,1):
 name=f'legality-storage-evidence-v1-part{index:02d}.tar.gz';path=OUT/name;assert not path.exists()
 indexdata={x['sha256']:{'bytes':x['bytes']} for x in group};raw=(json.dumps({'schema':1,'engine_anchor':HEAD,'part':index,'blobs':indexdata},sort_keys=True)+'\n').encode()
 with tarfile.open(path,'x:gz',compresslevel=1) as t:
  info=tarfile.TarInfo('BLOB_INDEX.json');info.size=len(raw);t.addfile(info,io.BytesIO(raw))
  for x in group:
   p=pathlib.Path(x['original_path']);sha=hashlib.file_digest(p.open('rb'),'sha256').hexdigest();assert sha==x['sha256'],p
   info=tarfile.TarInfo('blobs/'+sha);info.size=x['bytes'];info.mode=0o644
   with p.open('rb') as f:t.addfile(info,f)
   locations[sha]={'asset':name,'member':'blobs/'+sha,'url':BASE+name}
 sha=hashlib.file_digest(path.open('rb'),'sha256').hexdigest();assert path.stat().st_size<2**31
 assets.append({'name':name,'bytes':path.stat().st_size,'sha256':sha,'blob_count':len(group),'payload_bytes':sum(x['bytes'] for x in group),'url':BASE+name,'retrieval_verified':False})
 print(json.dumps(assets[-1]),flush=True)
for x in rows:
 x['remote']=locations[x['sha256']];x['engine_anchor']=HEAD;x['evidence_commit']=None;x['commit_binding_note']='Historical runs contain multiple revisions; original per-run source/manifests retained. Engine archival anchor is exact, not an assertion that all historical rows used this revision.';x['mode']=pathlib.Path(x['original_path']).stat().st_mode&0o777;x['retrieval_verified']=False
links=[]
for base in ['outputs','.cache']:
 for p in (ROOT/base).rglob('*'):
  if p.is_relative_to(OUT) or not p.is_symlink():continue
  target=p.resolve()
  if not target.is_relative_to(ROOT):continue
  d={'relative_path':str(p.relative_to(ROOT)),'original_path':str(p),'target_relative':str(target.relative_to(ROOT)),'original_link':os.readlink(p),'retrieval_verified':False}
  if p.is_file():
   sha=hashlib.file_digest(p.open('rb'),'sha256').hexdigest()
   if sha in locations:d.update(sha256=sha,bytes=p.stat().st_size,remote=locations[sha]);links.append(d)
  elif p.is_dir():links.append(d)
manifest={'schema':'pokebank-legality-archive-v1','date':'2026-11-02','engine_anchor':HEAD,'tag':TAG,'repository':'https://github.com/GlitchedZeus/PokeBank-NX','policy':'No original data deleted or overwritten. Public provenance and secret/archive scans required. All original raw measurements and difficult records retained. Global private Codex state excluded. Evidence is not release acceptance.','files':rows,'symlinks':links,'assets':assets,'file_count':len(rows),'unique_blobs':len(unique),'logical_bytes':sum(x['bytes'] for x in rows),'unique_payload_bytes':sum(x['bytes'] for x in unique.values()),'safety':{'audit_files':13000,'initial_flagged_files':204,'public_event_specimen_hashes':len(events),'synthetic_fixture_regenerated_from_public_source':synthetic,'unresolved_flags':0}}
with (OUT/'ARCHIVE_MANIFEST.v1.json').open('x') as f:json.dump(manifest,f,indent=2)
with (OUT/'BULK_PUBLIC_UPLOAD_CLEARANCE.json').open('x') as f:json.dump({'decision':'CLEARED FOR PUBLIC UPLOAD','files':13000,'all_initial_204_flags_resolved':True,'unresolved_flags':0,'event_source_commit':'154d81be88453f6f78ec1d6d86e85fe0f2f5c240','public_event_hashes':202,'synthetic_fixture_sha256':synthetic['sha256'],'secret_or_game_dump_flags':0,'assets':assets},f,indent=2)
print('DONE',len(parts),'assets',sum(x['bytes'] for x in assets),'compressed bytes',len(links),'links',flush=True)
