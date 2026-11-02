import pathlib,subprocess,json,hashlib,tarfile,io,sys
ROOT=pathlib.Path.cwd();OUT=ROOT/'outputs/archival-20261102-5fea7345';REPO='GlitchedZeus/PokeBank-NX';RID=431141822
class HashedReader:
 def __init__(self,f):self.f=f;self.h=hashlib.sha256();self.count=0
 def read(self,n=-1):
  b=self.f.read(n);self.h.update(b);self.count+=len(b);return b

def remote_info():
 return json.loads(subprocess.check_output(['gh','api',f'repos/{REPO}/releases/{RID}']))
def verify_asset(a,expected,outpath):
 assert a['state']=='uploaded' and a['size']==expected['bytes'] and a['digest']=='sha256:'+expected['sha256'],a['name']
 p=subprocess.Popen(['gh','api','--allow-escape-sequences','-H','Accept: application/octet-stream',f'repos/{REPO}/releases/assets/{a["id"]}'],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 reader=HashedReader(p.stdout);verified={};manifest=None;blobindex=None;members=0;payload_names=0;selected={};core='PART_B-RECONSTITUTED' in a['name'];bulk=a['name'].startswith('legality-storage-evidence-')
 if a['name'].endswith('.tar.gz'):
  with tarfile.open(fileobj=reader,mode='r|gz') as t:
   for m in t:
    if m.isdir():continue
    assert not m.name.startswith('/') and '..' not in pathlib.PurePosixPath(m.name).parts,m.name
    if m.islnk():
     assert m.linkname in verified,m.name;size,sha=verified[m.linkname]
    elif m.isfile():
     stream=t.extractfile(m);h=hashlib.sha256();size=0;chunks=[];keep=(m.name in ('MANIFEST.json','BLOB_INDEX.json') or (core and m.name.endswith(('FINAL_REPORT.md','FINAL_EXACT_HEAD_CI.json','NAME38_REPLAY.json'))))
     while b:=stream.read(1024*1024):
      h.update(b);size+=len(b)
      if keep:chunks.append(b)
     sha=h.hexdigest()
     if m.name=='MANIFEST.json':
      raw=b''.join(chunks);manifest=json.loads(raw)
      if core:assert hashlib.sha256(raw).hexdigest()=='4e79740c2a2d02676d8e70bd5b3798b1243437994aa5b0c7b49c3b03211ff937'
     elif m.name=='BLOB_INDEX.json':blobindex=json.loads(b''.join(chunks))
     elif keep:selected[m.name]=b''.join(chunks)
    else:raise ValueError('Unsupported archive entry '+m.name)
    verified[m.name]=(size,sha);members+=1
    if core and m.name!='MANIFEST.json':
     row=manifest['files'][m.name];assert row['bytes']==size and row['sha256']==sha,m.name;payload_names+=1
    if bulk and m.name.startswith('blobs/'):
     assert m.name=='blobs/'+sha and blobindex['blobs'][sha]['bytes']==size,m.name;payload_names+=1
  if core:assert payload_names==len(manifest['files'])==889
  if bulk:assert payload_names==len(blobindex['blobs'])==expected['blob_count']
 else:
  while reader.read(1024*1024):pass
 while reader.read(1024*1024):pass
 err=p.stderr.read();assert p.wait()==0,err.decode(errors='replace');assert reader.count==a['size'] and reader.h.hexdigest()==expected['sha256'],a['name']
 recovered=OUT/'download-recovered-selected'/a['name']
 for name,data in selected.items():
  path=recovered/name;path.parent.mkdir(parents=True,exist_ok=True)
  with path.open('xb') as f:f.write(data)
 result={'asset':a['name'],'asset_id':a['id'],'download_url':a['browser_download_url'],'remote_size':a['size'],'remote_digest':a['digest'],'downloaded_bytes':reader.count,'sha256':reader.h.hexdigest(),'remote_digest_matches_local':True,'download_and_sha256_verified':True,'archive_members_recovered_and_rehashed':members,'payload_names_verified':payload_names,'selected_files_restored':list(selected),'all_content_recovered_without_unchecked_extraction':True}
 with outpath.open('x') as f:json.dump(result,f,indent=2)
 print(json.dumps(result),flush=True)

manifest_path=OUT/'ARCHIVE_MANIFEST.v1.json';expected={}
if manifest_path.exists():
 manifest=json.loads(manifest_path.read_text());expected.update({x['name']:x for x in manifest['assets']})
supp=OUT/'SUPPLEMENT_ASSET.v1.json'
if supp.exists():
 data=json.loads(supp.read_text());expected[data['asset']['name']]=data['asset'];expected[data['asset']['name'].replace('.tar.gz','-http1.tar.gz')]=data['asset']
extra=OUT/'SOURCE_CORPUS_SUPPLEMENT_ASSET.v1.json'
if extra.exists():
 data=json.loads(extra.read_text());expected[data['asset']['name']]=data['asset']
seal=json.loads((OUT/'PART_B_RECONSTITUTED_VERIFIED.json').read_text());expected[pathlib.Path(seal['file']).name]={'bytes':seal['bytes'],'sha256':seal['sha256']}
for name in ['FINAL_REPORT.md','legality-major-2144a682-CI_LOGS.tar.gz']:
 path=ROOT/('outputs/legality-major-20261101/FINAL_REPORT.md' if name.endswith('.md') else 'outputs/'+name);expected[name]={'bytes':path.stat().st_size,'sha256':hashlib.file_digest(path.open('rb'),'sha256').hexdigest()}
info=remote_info();remote={x['name']:x for x in info['assets']};names=sys.argv[1:] or list(expected)
for name in names:
 out=OUT/('REMOTE_VERIFIED_'+name+'.json')
 if out.exists():print('Already verified',name,flush=True);continue
 assert name in remote,'Asset not yet uploaded: '+name
 verify_asset(remote[name],expected[name],out)
