import pathlib,json,hashlib,gzip,datetime,subprocess,collections,os
ROOT=pathlib.Path.cwd();OUT=ROOT/'outputs/archival-20261102-5fea7345';HEAD='2144a6820490fa81c932dd2d917b5153283feefc';TAG='evidence/gen1-5-2144a682-20261102';REPO='GlitchedZeus/PokeBank-NX';RID=431141822
m=json.loads((OUT/'ARCHIVE_MANIFEST.v1.json').read_text());s=json.loads((OUT/'SUPPLEMENT_ASSET.v1.json').read_text());m['assets'].append(s['asset']);m['files']+=s['files'];extra=json.loads((OUT/'SOURCE_CORPUS_SUPPLEMENT_ASSET.v1.json').read_text());m['assets'].append(extra['asset']);m['files']+=extra['files'];verification={}
for f in OUT.glob('REMOTE_VERIFIED_*.json'):
 d=json.loads(f.read_text());verification[d['asset']]=d
required=[x['name'] for x in m['assets']]+['legality-major-2144a682-PART_B-RECONSTITUTED.tar.gz','legality-major-2144a682-CI_LOGS.tar.gz','FINAL_REPORT.md','legality-storage-evidence-v1-part09-http1.tar.gz']
assert all(n in verification and verification[n]['download_and_sha256_verified'] for n in required),'A required remote download remains unverified'
info=json.loads(subprocess.check_output(['gh','api',f'repos/{REPO}/releases/{RID}']));assert info['tag_name']==TAG and info['prerelease'] and not info['draft'];remote={x['name']:x for x in info['assets']}
for n in required:
 a=remote[n];v=verification[n];assert a['state']=='uploaded' and a['digest']=='sha256:'+v['sha256'] and a['size']==v['downloaded_bytes']
 v['download_url']=a['browser_download_url'];v['public_release_metadata_rechecked']=True
for asset in m['assets']:
 a=remote[asset['name']];v=verification[asset['name']];asset.update(url=a['browser_download_url'],retrieval_verified=True,asset_id=a['id'],remote_digest=a['digest'],verification=v)
byasset={x['name']:x for x in m['assets']};bysha={}
for x in m['files']:
 x['retrieval_verified']=True;x['remote']['url']=byasset[x['remote']['asset']]['url'];x['engine_anchor']=HEAD;x.setdefault('reproducible',False);x['corresponding_commit']=HEAD;x['commit_binding_note']='Exact archival campaign anchor. Historical production/reference revisions are separately preserved in the original per-run manifests; this is not a relabelling of those experiments.'
 bysha[x['sha256']]=x
for x in m['symlinks']:
 if 'remote' in x:x['retrieval_verified']=True;x['remote']['url']=byasset[x['remote']['asset']]['url'];x['status']='Target payload bytes independently downloaded and rehashed'
 else:x['retrieval_verified']=False;x['status']='Directory/source/dependency alias recorded; recreate with exact Git checkout or documented SDK install. Original local alias is retained and not in cleanup.'
m.update(schema='pokebank-legality-archive-v2',file_count=len(m['files']),unique_blobs=len(bysha),logical_bytes=sum(x['bytes'] for x in m['files']),unique_payload_bytes=sum(x['bytes'] for x in bysha.values()),release_url=info['html_url'],all_archived_regular_files_remote_retrieval_verified=True,private_backup_status='No authorized private destination supplied. Global/private state, unreviewed Git metadata, synced sources and excluded items remain local and UNSAFE TO DELETE.',known_missing_original_compressed_archives=['legality-major-2144a682-PART_B.tar.gz','legality-repair-eabe41f3-20261029.tar.gz','legality-repair-eabe41f3-ci-20261029.tar.gz'])
# Verify old archive payload coverage against durable downloaded blob hashes, not filenames alone.
coverage=[]
for name in ['legality-repair-eabe41f3-20261029.manifest.json','legality-repair-eabe41f3-ci-20261029.manifest.json']:
 old=json.loads((ROOT/'outputs'/name).read_text());missing=[p for p,x in old['files'].items() if x['sha256'] not in bysha];assert not missing,missing
 coverage.append({'original_manifest':name,'payload_names':len(old['files']),'all_payload_hashes_in_download_verified_assets':True,'original_compressed_bytes_recovered':False,'engine_head':old['engine_head']})
m['older_missing_seal_payload_recovery']=coverage
with (OUT/'ARCHIVE_MANIFEST.v2.json').open('x') as f:json.dump(m,f,indent=2)
with (OUT/'ARCHIVE_MANIFEST.v2.json.gz').open('xb') as f:
 with gzip.GzipFile(fileobj=f,mode='wb',filename='',mtime=0,compresslevel=6) as z:z.write((OUT/'ARCHIVE_MANIFEST.v2.json').read_bytes())
# Prepare exact file-only removal list; never include source worktrees/Git/global state or unverified payloads.
plan=json.loads((OUT/'CLEANUP_CANDIDATES_PREVERIFICATION.v3.json').read_text());rows=plan['files'];paths={x['path'] for x in rows};seen={(pathlib.Path(x['path']).stat().st_dev,pathlib.Path(x['path']).stat().st_ino) for x in rows if pathlib.Path(x['path']).exists()}
for n in required:
 f=OUT/n
 if not f.exists():continue
 a=remote[n];sha=hashlib.file_digest(f.open('rb'),'sha256').hexdigest();assert a['digest']=='sha256:'+sha
 if str(f) in paths:continue
 st=f.stat();key=(st.st_dev,st.st_ino);rows.append({'path':str(f),'bytes':st.st_size,'sha256':sha,'allocated_bytes_unique_inode':0 if key in seen else st.st_blocks*512,'basis':'local archival staging copy; exact remote asset independently downloaded and recovered','asset':n});seen.add(key);paths.add(str(f))
for f in (OUT/'downloads-verified').glob('*'):
 if not f.is_file() or f.name not in remote:continue
 a=remote[f.name];sha=hashlib.file_digest(f.open('rb'),'sha256').hexdigest();assert a['digest']=='sha256:'+sha
 if str(f) in paths:continue
 st=f.stat();key=(st.st_dev,st.st_ino);rows.append({'path':str(f),'bytes':st.st_size,'sha256':sha,'allocated_bytes_unique_inode':0 if key in seen else st.st_blocks*512,'basis':'verified duplicate remote download','asset':f.name});seen.add(key)
plan.update(schema='pokebank-verified-cleanup-v2',status='Prepared for review; NO CLEANUP EXECUTED',files=rows,eligible_allocated_bytes=sum(x['allocated_bytes_unique_inode'] for x in rows),all_required_remote_assets_verified=True,source_recovery_proof_passed=True,release_id=RID,evidence_tag=TAG,engine_anchor=HEAD,required_assets=[{'name':n,'bytes':remote[n]['size'],'sha256':remote[n]['digest'].split(':')[1]} for n in required],warning='Allocated file blocks, excluding retained directory blocks; actual filesystem free-space gain may differ. File-only commands preserve all directories and symlinks. Changed/missing bytes never justify deleting a replacement.')
with (OUT/'CLEANUP_INVENTORY.v2.json').open('x') as f:json.dump(plan,f,indent=2)
with (OUT/'CLEANUP_INVENTORY.v2.json.gz').open('xb') as f:
 with gzip.GzipFile(fileobj=f,mode='wb',filename='',mtime=0,compresslevel=6) as z:z.write((OUT/'CLEANUP_INVENTORY.v2.json').read_bytes())
report={'schema':2,'date_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'engine_anchor':HEAD,'release_id':RID,'release_url':info['html_url'],'archived_regular_original_paths':len(m['files']),'unique_download_verified_blobs':len(bysha),'required_assets':{n:verification[n] for n in required},'all_required_archives_downloaded_and_payloads_recovered':True,'older_missing_seal_payload_recovery':coverage,'part_b_original_manifest_sha256':'4e79740c2a2d02676d8e70bd5b3798b1243437994aa5b0c7b49c3b03211ff937','part_b_original_gzip_sha_not_recovered':True,'original_gen12_snapshot':json.loads((OUT/'GEN12_SNAPSHOT_REMOTE_VERIFIED.json').read_text()),'orphan_reflog_backup':json.loads((OUT/'ORPHAN_REMOTE_REFS_VERIFIED.json').read_text()),'source_recovery':json.loads((OUT/'recovery-proof/RECOVERY_NORMAL_REGRESSION_RESULTS.json').read_text()),'remote_witness_recovery':json.loads((OUT/'REMOTE_WITNESS_RECOVERY_PROOF.json').read_text()),'initial_audited_allocated_bytes':27232681984,'eligible_cleanup_allocated_bytes':plan['eligible_allocated_bytes'],'cleanup_performed':False,'unarchived_retained':['Global Codex private configuration/authentication/conversations/history','Synced read-only sources and unrelated projects','Original source trees/Git metadata, including 6747 unreviewed unreferenced commit objects','SDK/public source dependency caches and unique fuzz/source-uncertain paths','Downloads NRO and external transaction test remnants','Private archival inventories'],'limitations':['No original compressed bytes claimed for three missing seals; all named payloads recovered separately.','No acquired-specimen authenticity or full title/device integration acceptance.','No new legality rule implementation; known 21 Gen IV raw false-Invalid disagreements, 344 Gen V reference errors and Incomplete outcomes remain.']}
with (OUT/'VERIFICATION_REPORT.v2.json').open('x') as f:json.dump(report,f,indent=2)
print('FINAL_MANIFEST_READY',len(m['files']),'paths',len(bysha),'unique blobs','cleanup',plan['eligible_allocated_bytes']/1e9,'GB',flush=True)
