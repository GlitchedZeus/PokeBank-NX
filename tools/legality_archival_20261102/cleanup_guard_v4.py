#!/usr/bin/env python3
"""Validate a frozen cleanup inventory; remove files ONLY with explicit --execute.
The archival session runs validation only. No recursive directory removal.
"""
import argparse,pathlib,json,hashlib,subprocess,sys,stat
p=argparse.ArgumentParser();p.add_argument('inventory',type=pathlib.Path);p.add_argument('--execute',action='store_true');a=p.parse_args();d=json.loads(a.inventory.read_text());root=pathlib.Path(d['project_root']).resolve();assert root.name=='g-p-6a9518ebef448191bfcc640287c6cb43';assert d['all_required_remote_assets_verified'] and d['source_recovery_proof_passed'];errors=[];files=[]
# Validate every path and hash BEFORE any possible removal. New or changed data is retained.
for row in d['files']:
 f=pathlib.Path(row['path']);rel=f.relative_to(root);assert rel.parts[0] in ('outputs','.cache') and 'sources' not in rel.parts and '.git' not in rel.parts
 if not f.exists():continue
 if f.resolve()!=f.absolute():errors.append(str(f)+' (symlink in path)');continue
 if f.is_symlink() or not f.is_file() or f.stat().st_size!=row['bytes'] or hashlib.file_digest(f.open('rb'),'sha256').hexdigest()!=row['sha256']:errors.append(str(f));continue
 files.append(f)
if errors:raise SystemExit('ABORT: changed/unexpected paths; nothing removed: '+repr(errors[:10]))
# The exact frozen verified assets must still exist at the expected digests.
if a.execute:
 info=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/releases/'+str(d['release_id'])]));remote={x['name']:x for x in info['assets']}
 for x in d['required_assets']:
  y=remote.get(x['name']);assert y and y['state']=='uploaded' and y['size']==x['bytes'] and y['digest']=='sha256:'+x['sha256'],x['name']
 assert info['tag_name']==d['evidence_tag'] and info['prerelease'] and not info['draft']
 tag=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/git/ref/tags/'+d['evidence_tag']]));assert tag['object']['sha']==d['engine_anchor']
 print('Explicit --execute supplied: removing only validated frozen files; directories, symlinks, source trees and Git metadata remain.')
 pinned={x['path']:x['sha256'] for x in d['files']}
 for f in files:
  # Recheck immediately before removal; never unlink a replacement symlink.
  s=f.lstat();assert stat.S_ISREG(s.st_mode);assert hashlib.file_digest(f.open('rb'),'sha256').hexdigest()==pinned[str(f)]
  f.unlink()
else:print('DRY RUN ONLY: verified',len(files),'unchanged files. No files removed. Eligible allocated bytes:',d['eligible_allocated_bytes'])
