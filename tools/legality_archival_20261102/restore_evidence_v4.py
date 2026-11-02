#!/usr/bin/env python3
"""Restore only verified content-addressed evidence into a NEW directory. No overwrites."""
import argparse,json,pathlib,hashlib,tarfile,os,shutil
p=argparse.ArgumentParser();p.add_argument('manifest',type=pathlib.Path);p.add_argument('assets',type=pathlib.Path);p.add_argument('destination',type=pathlib.Path);p.add_argument('--prefix',action='append',default=[]);p.add_argument('--source-checkout',type=pathlib.Path);a=p.parse_args();m=json.loads(a.manifest.read_text());dst=a.destination.resolve();dst.mkdir(parents=True,exist_ok=False)
if a.source_checkout:
 import subprocess
 assert subprocess.check_output(['git','-C',str(a.source_checkout),'rev-parse','HEAD'],text=True).strip()==m['engine_anchor'],'Source checkout differs from evidence anchor'
selected=[x for x in m['files'] if not a.prefix or any(x['relative_path'].startswith(s) for s in a.prefix)];selected_links=[x for x in m.get('symlinks',[]) if not a.prefix or any(x['relative_path'].startswith(v) for v in a.prefix)];wanted={x['sha256'] for x in selected}|{x['sha256'] for x in selected_links if 'sha256' in x};needed_assets={x['remote']['asset'] for x in selected+selected_links if 'remote' in x};store=dst/'.blobs';store.mkdir()
for asset in m['assets']:
 if asset['name'] not in needed_assets:continue
 f=a.assets/asset['name'];assert hashlib.file_digest(f.open('rb'),'sha256').hexdigest()==asset['sha256'],asset['name']
 with tarfile.open(f) as t:
  for member in t:
   if not member.isfile() or not member.name.startswith('blobs/'):continue
   sha=member.name.split('/',1)[1]
   if sha not in wanted:continue
   out=store/sha
   with out.open('xb') as w:shutil.copyfileobj(t.extractfile(member),w)
   assert hashlib.file_digest(out.open('rb'),'sha256').hexdigest()==sha,sha
for row in selected:
 rel=pathlib.PurePosixPath(row['relative_path']);assert not rel.is_absolute() and '..' not in rel.parts
 f=dst/rel;f.parent.mkdir(parents=True,exist_ok=True)
 with (store/row['sha256']).open('rb') as r,f.open('xb') as w:shutil.copyfileobj(r,w)
 f.chmod(row['mode']);assert f.stat().st_size==row['bytes']
for row in selected_links:
 if a.prefix and not any(row['relative_path'].startswith(s) for s in a.prefix):continue
 f=dst/row['relative_path'];target=dst/row['target_relative'];assert f.is_relative_to(dst) and target.is_relative_to(dst)
 if not target.exists() and a.source_checkout and row['target_relative'].startswith('PokeBank-NX-legality-completion/'):
  target=a.source_checkout.resolve()/pathlib.PurePosixPath(row['target_relative']).relative_to('PokeBank-NX-legality-completion')
 if target.is_file() and row.get('sha256') and hashlib.file_digest(target.open('rb'),'sha256').hexdigest()!=row['sha256']:target=dst/'__unmatched_source_alias_target__'
 if target.exists():
  f.parent.mkdir(parents=True,exist_ok=True);f.symlink_to(os.path.relpath(target,f.parent),target_is_directory=target.is_dir())
 elif row.get('sha256') and (store/row['sha256']).exists():
  f.parent.mkdir(parents=True,exist_ok=True)
  with (store/row['sha256']).open('rb') as r,f.open('xb') as w:shutil.copyfileobj(r,w)
  f.chmod(next((x['mode'] for x in m['files'] if x['sha256']==row['sha256']),0o644))
 else:print('Alias retained in manifest; target not restored:',row['relative_path'])
print(f'Restored and rehashed {len(selected)} original paths in {dst}. No pre-existing path overwritten.')
