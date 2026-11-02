import hashlib,json,pathlib,subprocess,tarfile,io
root=pathlib.Path.cwd();repo=root/'PokeBank-NX-legality-completion';out=root/'outputs';head=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()
assert head=='2144a6820490fa81c932dd2d917b5153283feefc'
assert not subprocess.check_output(['git','-C',str(repo),'status','--porcelain'],text=True).strip()
ci=json.loads((out/'legality-major-20261101/published-2144/FINAL_EXACT_HEAD_CI.json').read_text())
assert ci['head']==head and ci['workflow_success']==ci['workflow_count']==24 and ci['job_success']==ci['job_count']
package=out/'archival-20261102-5fea7345/legality-major-2144a682-PART_B-RECONSTITUTED.tar.gz';assert not package.exists(),'Never overwrite an evidence seal'
entries={}
def add_tree(prefix,path):
 for file in sorted(path.rglob('*')):
  if file.is_file():entries[prefix+'/'+file.relative_to(path).as_posix()]=file.resolve()
add_tree('local-continuation',out/'legality-major-20261101');add_tree('urgent-ci-repair',out/'legality-ci-build-fix-20261031')
for label,path in [('native-hgss',root/'.cache/legality-major-20261101/native-hgss'),('native-bw2',root/'.cache/legality-major-20261101/native-bw2'),('native-emerald',root/'.cache/legality-major-20261101/native-emerald'),('native-wings',root/'.cache/legality-major-20261031/native-wings')]:add_tree('original-source/'+label,path)
for group in ['gen34','gen5']:
 for label,prefix in [('current','ctx_'),('ancestry',''),('cap','rng_')]:entries['binaries/'+label+'/'+group+'_diff']=root/'.cache/legality-major-20261101'/(prefix+group+'_diff')
 for file in (root/f'.cache/legality-hardening-20261027/oracle261018-{group}').glob('*'):
  if file.is_file():entries['official-oracle/'+group+'/'+file.name]=file
entries['binaries/green-3f/gen5_diff']=root/'.cache/legality-ci-build-fix-build/gen5_diff';entries['binaries/frozen-ac64/gen34_diff']=root/'outputs/legality-major-20261031/fresh-independent-final2/gen34_diff'
manifest={'published_head':head,'policy':'All local raw measurements, initial failures, source overlays, official references, diagnostic errors, native evidence and exact-head CI records retained. Duplicate content may use tar hard links; every original name, byte count and SHA256 remains. Samples overlap and must not be counted as unique specimens. Earlier PartA is immutable and separate. No fixture/verdict/denominator selection.','files':{}}
for name,path in entries.items():
 data=path.read_bytes();manifest['files'][name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'archive_mode':493 if path.stat().st_mode&0o111 else 420}
raw=(json.dumps(manifest,indent=2)+'\n').encode();assert hashlib.sha256(raw).hexdigest()=='4e79740c2a2d02676d8e70bd5b3798b1243437994aa5b0c7b49c3b03211ff937', 'Original manifest differs; halt';first={}
with tarfile.open(package,'w:gz',compresslevel=6) as tar:
 info=tarfile.TarInfo('MANIFEST.json');info.size=len(raw);tar.addfile(info,io.BytesIO(raw))
 for name,path in entries.items():
  row=manifest['files'][name];data=path.read_bytes();assert len(data)==row['bytes'] and hashlib.sha256(data).hexdigest()==row['sha256'],name
  info=tarfile.TarInfo(name);info.mode=row['archive_mode'];key=(row['sha256'],row['bytes'],row['archive_mode'])
  if key in first:info.type=tarfile.LNKTYPE;info.linkname=first[key];tar.addfile(info)
  else:info.size=len(data);tar.addfile(info,io.BytesIO(data));first[key]=name
print('SEALED',package.stat().st_size,len(entries),'payload names',flush=True)
verified={}
with tarfile.open(package,'r:gz') as tar:
 assert json.load(tar.extractfile('MANIFEST.json'))==manifest
 for member in tar:
  if member.name=='MANIFEST.json':continue
  row=manifest['files'][member.name]
  if member.islnk():observed=verified[member.linkname]
  else:
   data=tar.extractfile(member).read();observed=(len(data),hashlib.sha256(data).hexdigest())
  assert observed==(row['bytes'],row['sha256']),member.name;verified[member.name]=observed
assert len(verified)==len(entries)
sha=hashlib.sha256(package.read_bytes()).hexdigest();result={'file':str(package),'bytes':package.stat().st_size,'sha256':sha,'payload_names':len(entries),'distinct_payloads_rehashed':len(first),'manifest_sha256':hashlib.sha256(raw).hexdigest(),'all_archive_name_hash_bindings_verified':True,'published_head':head}
(out/'archival-20261102-5fea7345/PART_B_RECONSTITUTED_VERIFIED.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
