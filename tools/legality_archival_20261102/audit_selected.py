import pathlib,subprocess,json,hashlib,tarfile,zipfile,tempfile,os,re,collections,time
ROOT=pathlib.Path.cwd(); OUT=pathlib.Path(__import__('sys').argv[1]).resolve();OUT.mkdir(exist_ok=False)
ROMS={'.gba','.gbc','.gb','.nds','.3ds','.cia','.nsp','.xci','.iso','.gcm','.wud','.wad','.rvz','.cso','.rom'}
SAVES={'.sav','.srm','.dsv','.gci','.raw','.pk1','.pk2','.pk3','.pk4','.pk5','.pk6','.pk7','.pk8','.pk9','.ck3','.xk3','.ek3','.ek4','.ek5','.bk4'}
PATTERNS={
 'github_token':re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{36,255}|github_pat_[A-Za-z0-9_]{60,255})'),
 'openai_key':re.compile(rb'sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{40,250}'),
 'aws_access_key':re.compile(rb'\b(?:AKIA|ASIA)[A-Z0-9]{16}\b'),
 'private_key':re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----'),
 'credential_url':re.compile(rb'https?://[^\s/"<>]{2,80}:[^\s/@"<>]{6,120}@'),
 'auth_json':re.compile(rb'"(?:access_token|refresh_token|OPENAI_API_KEY)"\s*:\s*"[^"\s]{12,}')}
public={}; memo={}; summary=collections.Counter(); findinglog=(OUT/'SAFETY_FINDINGS_PRIVATE.jsonl').open('x'); records=(OUT/'CANDIDATES_PRIVATE.jsonl').open('x'); nested=(OUT/'ARCHIVE_CONTENTS_AUDIT_PRIVATE.jsonl').open('x')
# Hash only public repository fixture blobs, never infer save provenance from filenames.
for rel in ['PokeBank-NX-legality-completion','.cache/legality-hardening-20261027/pkhex261018','.cache/legality-hardening-20261027/pkhex6501','research-sources/PKHeX']:
 p=ROOT/rel; head=subprocess.check_output(['git','-C',str(p),'rev-parse','HEAD'],text=True).strip()
 tree=subprocess.check_output(['git','-C',str(p),'ls-tree','-rz','HEAD'])
 for entry in tree.split(b'\0'):
  if not entry:continue
  meta,name=entry.split(b'\t',1); mode,typ,blob=meta.split(); name=os.fsdecode(name)
  if typ!=b'blob' or pathlib.Path(name).suffix.lower() not in SAVES:continue
  data=subprocess.check_output(['git','-C',str(p),'cat-file','blob',blob.decode()]);sha=hashlib.sha256(data).hexdigest()
  public[sha]={'repository':rel,'commit':head,'path':name,'git_blob':blob.decode()}
(OUT/'PUBLIC_FIXTURE_PROVENANCE.json').write_text(json.dumps(public,indent=2))

def audit_stream(f,name,size,depth=0):
 h=hashlib.sha256(); flags=set(); tail=b''; ext=pathlib.Path(name).suffix.lower(); isarc=name.lower().endswith(('.tar.gz','.tgz','.tar','.zip','.tar.xz','.tar.bz2'))
 spool=tempfile.SpooledTemporaryFile(max_size=8*1024*1024,dir=OUT) if isarc else None
 while chunk:=f.read(1024*1024):
  h.update(chunk);block=tail+chunk
  for label,pat in PATTERNS.items():
   if pat.search(block):flags.add(label)
  tail=block[-512:]
  if spool:spool.write(chunk)
 sha=h.hexdigest(); result={'sha256':sha,'bytes':size,'flags':sorted(flags),'nested':[]}
 if ext in ROMS:result['flags'].append('game_dump_extension')
 if ext in SAVES and sha not in public:result['flags'].append('unproven_save_or_specimen')
 base=pathlib.PurePosixPath(name).name.lower()
 if base in ('auth.json','credentials','credentials.json','.env') or 'rollout-' in base or base in ('history.jsonl','session_index.jsonl'):
  result['flags'].append('private_filename')
 if isarc:
  if sha in memo:
   result['nested']=memo[sha]['nested'];result['flags']=sorted(set(result['flags']+memo[sha]['flags']))
  elif depth>=6:result['flags'].append('archive_depth_not_audited')
  else:
   spool.seek(0)
   try:
    if name.lower().endswith('.zip'):
     with zipfile.ZipFile(spool) as z:
      for m in z.infolist():
       if m.is_dir():continue
       with z.open(m) as c: child=audit_stream(c,m.filename,m.file_size,depth+1)
       child['member']=m.filename;result['nested'].append(child)
    else:
     with tarfile.open(fileobj=spool,mode='r:*') as t:
      seen={}
      for m in t:
       if m.isdir():continue
       if m.isfile():
        with t.extractfile(m) as c:child=audit_stream(c,m.name,m.size,depth+1)
        seen[m.name]=child
       elif m.islnk() and m.linkname in seen:child=dict(seen[m.linkname])
       elif m.issym():child={'bytes':0,'sha256':None,'flags':[],'nested':[],'symlink':m.linkname}
       else:child={'bytes':m.size,'sha256':None,'flags':['unresolved_archive_member'],'nested':[]}
       if m.name.startswith('/') or '..' in pathlib.PurePosixPath(m.name).parts:child['flags'].append('unsafe_member_path')
       child=dict(child,member=m.name);result['nested'].append(child)
    if any(c['flags'] for c in result['nested']):result['flags'].append('flagged_nested_content')
   except Exception as e: result['flags'].append('archive_read_failure:'+type(e).__name__)
   memo[sha]=result.copy()
   nested.write(json.dumps({'archive_sha256':sha,'original_name':name,'members':result['nested']})+'\n');nested.flush()
  spool.close()
 return result

def candidates():
 for name in __import__('sys').argv[2:]:
  yield pathlib.Path(name).resolve(),'irreplaceable evidence',False

count=0
for p,kind,repro in candidates():
 try:
  with p.open('rb') as f:d=audit_stream(f,str(p.relative_to(ROOT)),p.stat().st_size)
  flags=d['flags']; record={'original_path':str(p),'relative_path':str(p.relative_to(ROOT)),'bytes':d['bytes'],'sha256':d['sha256'],'classification':kind,'reproducible':repro,'safety_flags':flags,'public_fixture_provenance':public.get(d['sha256'])}
  records.write(json.dumps(record)+'\n');records.flush();summary['files']+=1;summary['bytes']+=d['bytes'];summary['flagged_files']+=bool(flags)
  if flags:
   findinglog.write(json.dumps(dict(record,nested_findings=d['nested']))+'\n');findinglog.flush()
  count+=1
  if count%100==0:print(json.dumps(dict(summary,current=str(p.relative_to(ROOT)))),flush=True)
 except Exception as e:
  findinglog.write(json.dumps({'original_path':str(p),'error':type(e).__name__+':'+str(e)})+'\n');findinglog.flush();summary['errors']+=1
(OUT/'AUDIT_SUMMARY_PRIVATE.json').write_text(json.dumps(summary,indent=2));print('DONE',dict(summary),flush=True)
