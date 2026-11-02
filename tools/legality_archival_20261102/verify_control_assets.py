import pathlib,subprocess,json,hashlib,tarfile,io,gzip,sys
p=pathlib.Path('outputs/archival-20261102-5fea7345').resolve();info=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/releases/431141822']));remote={x['name']:x for x in info['assets']}
for name in sys.argv[1:]:
 out=p/('REMOTE_VERIFIED_'+name+'.json')
 if out.exists():continue
 f=p/name;expected=hashlib.file_digest(f.open('rb'),'sha256').hexdigest();a=remote[name];assert a['digest']=='sha256:'+expected and a['size']==f.stat().st_size and a['state']=='uploaded'
 if info['draft']:raw=subprocess.check_output(['gh','api','--allow-escape-sequences','-H','Accept: application/octet-stream','repos/GlitchedZeus/PokeBank-NX/releases/assets/'+str(a['id'])])
 else:raw=subprocess.check_output(['curl','-fsSL','--connect-timeout','20','--max-time','120',a['browser_download_url']])
 assert len(raw)==f.stat().st_size and hashlib.sha256(raw).hexdigest()==expected
 members=0
 if name.endswith('.tar.gz'):
  with tarfile.open(fileobj=io.BytesIO(raw)) as t:
   for m in t:
    if m.isfile():t.extractfile(m).read();members+=1
 elif name.endswith('.json.gz'):json.loads(gzip.decompress(raw))
 elif name.endswith('.json'):json.loads(raw)
 d={'asset':name,'asset_id':a['id'],'download_url':a['browser_download_url'],'sha256':expected,'downloaded_bytes':len(raw),'download_and_sha256_verified':True,'remote_digest_matches_local':True,'archive_members_recovered_and_rehashed':members,'public_unauthenticated_download':not info['draft']}
 with out.open('x') as g:json.dump(d,g,indent=2)
 print(name,len(raw),'SHA256+download verified',flush=True)
