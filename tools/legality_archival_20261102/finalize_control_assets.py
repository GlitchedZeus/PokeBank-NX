import pathlib,json,subprocess,time,hashlib,urllib.request,re
P=pathlib.Path('outputs/archival-20261102-5fea7345').resolve();start=time.time()
while True:
 log=(P/'PUBLISH_VERIFIED_RESULTS_V3.log').read_text()
 if 'FINAL_MANIFEST_AND_CLEANUP_DRY_RUN_PASS_NO_REMOVAL' in log:break
 if 'Traceback' in log:raise SystemExit('Publication/inventory phase failed; inspect preserved log')
 if time.time()-start>3600:raise SystemExit('Still waiting for verified publication; no cleanup executed')
 time.sleep(20)
subprocess.run(['python3',str(P/'generate_public_report.py')],check=True)
files=['ARCHIVE_MANIFEST.v2.json.gz','VERIFICATION_REPORT.v2.json','CLEANUP_INVENTORY.v2.json.gz','RECOVERY_GUIDE.v6.md','PUBLIC_ARCHIVAL_REPORT.v2.md','SAFE_CLEANUP_COMMANDS.v2.txt','restore_evidence_v4.py','cleanup_guard_v4.py','EXCLUSIONS_AND_LOCAL_RETENTION.v1.json','BULK_PUBLIC_UPLOAD_CLEARANCE.json','SOURCE_CORPUS_SUPPLEMENT_ASSET.v1.json','REMOTE_GIT_RECOVERY_PROOF.v1.json','FRESH_CLONE_SOURCE_PIN_VERIFICATION.json','REMOTE_WITNESS_RECOVERY_PROOF.json','ORPHAN_BACKUP_REFS.json','GEN12_SNAPSHOT_REMOTE_VERIFIED.json']
info=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/releases/431141822']));assert not info['draft'];sums={a['name']:a['digest'].split(':')[1] for a in info['assets'] if a['state']=='uploaded'}
for name in files:
 f=P/name;sums[name]=hashlib.file_digest(f.open('rb'),'sha256').hexdigest()
with (P/'SHA256SUMS.v2.txt').open('x') as f:
 for name,sha in sorted(sums.items()):f.write(sha+'  '+name+'\n')
files.append('SHA256SUMS.v2.txt')
subprocess.run(['gh','release','upload','evidence/gen1-5-2144a682-20261102','--repo','GlitchedZeus/PokeBank-NX',*[str(P/n) for n in files]],check=True)
print('FINAL_CONTROL_ASSETS_UPLOADED',flush=True)
subprocess.run(['python3',str(P/'verify_control_assets.py'),*files],check=True)
# Confirm public access for every payload archive after publication without another huge transfer.
info=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/releases/431141822']));required=json.loads((P/'VERIFICATION_REPORT.v2.json').read_text())['required_assets'];checks=[]
for a in info['assets']:
 if a['name'] not in required:continue
 h=subprocess.run(['curl','-fsSLI','--connect-timeout','20','--max-time','120',a['browser_download_url']],capture_output=True,text=True);h.check_returncode();lengths=re.findall(r'(?im)^content-length:\s*(\d+)\s*$',h.stdout);assert not lengths or int(lengths[-1])==a['size'],a['name']
 checks.append({'name':a['name'],'asset_id':a['id'],'public_download_url':a['browser_download_url'],'public_head_success':True,'server_size':a['size'],'digest':a['digest'],'full_download_previously_verified':True})
control={n:json.loads((P/('REMOTE_VERIFIED_'+n+'.json')).read_text()) for n in files}
result={'schema':1,'public_release':info['html_url'],'engine_tag':'evidence/gen1-5-2144a682-20261102','payload_public_access':checks,'control_assets_independent_public_downloads':control,'all_payload_archives_fully_downloaded_and_recovered_before_publication':True,'cleanup_performed':False}
with (P/'PUBLIC_RETRIEVAL_VERIFIED.v1.json').open('x') as f:json.dump(result,f,indent=2)
subprocess.run(['gh','release','upload','evidence/gen1-5-2144a682-20261102','--repo','GlitchedZeus/PokeBank-NX',str(P/'PUBLIC_RETRIEVAL_VERIFIED.v1.json')],check=True)
subprocess.run(['python3',str(P/'verify_control_assets.py'),'PUBLIC_RETRIEVAL_VERIFIED.v1.json'],check=True)
print('ALL_FINAL_CONTROL_ASSETS_AND_PUBLIC_RETRIEVAL_VERIFIED',flush=True)
