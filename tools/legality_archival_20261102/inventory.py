import os,stat,json,collections,pathlib,time,subprocess
base=pathlib.Path('/home/user/.codex');project=pathlib.Path.cwd();out=project/'outputs/archival-20261102-5fea7345';rows=[];errors=[];dirs=collections.defaultdict(lambda:[0,0,0]);seen=set();repos=[]
def category(p):
 s=str(p);parts=p.parts;name=p.name.lower()
 if any(x in parts for x in ['sessions','archived_sessions','attachments','sqlite','backups']) or name in ['auth.json','config.toml','history.jsonl','hooks.json','credentials','.env'] or any(x in name for x in ['token','secret','credential']):return 'sensitive/private material'
 if '.git' in parts or name=='.git':return 'essential Git metadata'
 if project not in p.parents:return 'private/unrelated or global material—retain'
 if 'sources' in parts:return 'essential synced read-only reference'
 if any(x in s.lower() for x in ['fuzz_corpus','fuzz-corpus','crash-','timeout-','leak-','oom-']):return 'irreplaceable evidence'
 if 'outputs' in parts:return 'irreplaceable evidence'
 if p.suffix.lower() in ['.sav','.srm','.gba','.gbc','.gb','.nds','.3ds','.cia','.nsp','.xci','.p12','.pem','.key']:return 'sensitive/private or copyrighted—requires provenance'
 if p.suffix.lower() in ['.cpp','.c','.h','.hpp','.py','.sh','.md','.mk','.cs','.json','.yml','.yaml','.patch','.diff','.txt','.csv','.pk1','.pk2','.pk3','.pk4','.pk5','.ck3','.xk3']:return 'essential source/research/fixture—requires provenance'
 if p.suffix.lower() in ['.o','.obj','.a','.so','.dll','.pdb','.exe','.d','.pyc','.nro','.nso','.elf']:return 'recoverable generated data/compiler cache'
 if any(x in parts for x in ['build','obj','bin','artifacts','node_modules','.nuget']):return 'recoverable generated data/compiler cache'
 return 'unclassified—retain pending audit'
for root,sub,files in os.walk(base,followlinks=False):
 r=pathlib.Path(root)
 if r==out:sub[:]=[];continue
 if '.git' in sub or '.git' in files:repos.append(str(r))
 for name in files+ [x for x in sub if (r/x).is_symlink()]:
  p=r/name
  try:
   s=p.lstat();key=(s.st_dev,s.st_ino);unique=key not in seen;seen.add(key);allocated=s.st_blocks*512 if unique else 0
   row={'path':str(p),'bytes':s.st_size,'allocated_bytes_unique_inode':allocated,'kind':'symlink' if stat.S_ISLNK(s.st_mode) else 'file','classification':category(p)};rows.append(row)
   par=p.parent
   while par==base or base in par.parents:
    dirs[str(par)][0]+=s.st_size;dirs[str(par)][1]+=allocated;dirs[str(par)][2]+=1
    if par==base:break
    par=par.parent
  except OSError as e:errors.append({'path':str(p),'error':type(e).__name__})
with (out/'ALL_FILES_PRIVATE.jsonl').open('x') as f:
 for row in rows:f.write(json.dumps(row)+'\n')
summary={'scope':str(base),'project':str(project),'metadata_only':True,'file_count':len(rows),'logical_bytes':sum(x['bytes'] for x in rows),'unique_allocated_bytes':sum(x['allocated_bytes_unique_inode'] for x in rows),'classification_allocated_bytes':dict(collections.Counter()),'largest_directories':sorted([{'path':p,'logical_bytes':v[0],'allocated_bytes':v[1],'files':v[2]} for p,v in dirs.items()],key=lambda x:-x['allocated_bytes'])[:150],'largest_files':sorted(rows,key=lambda x:-x['allocated_bytes_unique_inode'])[:100],'repositories':sorted(set(repos)),'errors':errors}
for x in rows:summary['classification_allocated_bytes'][x['classification']]=summary['classification_allocated_bytes'].get(x['classification'],0)+x['allocated_bytes_unique_inode']
(out/'SIZE_REPORT_PRIVATE.json').write_text(json.dumps(summary,indent=2)+'\n');print({k:v for k,v in summary.items() if k not in ['largest_directories','largest_files','repositories','errors']});print('Repositories',len(summary['repositories']),'errors',len(errors))
