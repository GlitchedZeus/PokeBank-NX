#!/usr/bin/env python3
"""Validate research structure and all pinned registry bytes from local source checkouts."""
import csv,hashlib,json,pathlib,re,subprocess,sys
R=pathlib.Path(__file__).resolve().parents[1]; NX=R.parents[2]; S=pathlib.Path(sys.argv[1]).resolve()
registry=json.loads((R/'SOURCE_REGISTRY.json').read_text()); ids=[x['id'] for x in registry]
assert len(ids)==len(set(ids))
for item in registry:
 repo=NX if item['repository']=='GlitchedZeus/PokeBank-NX' else S/item['repository'].split('/')[-1]
 data=subprocess.check_output(['git','-C',str(repo),'show',item['revision']+':'+item['source_path']])
 assert hashlib.sha256(data).hexdigest()==item['sha256'],item['id']
 assert item['url'].startswith('https://github.com/'+item['repository']+'/blob/'+item['revision']+'/')
for p in R.rglob('*.json'):json.loads(p.read_text())
counts={}
for p in R.glob('*.csv'):
 with p.open(newline='') as f:
  reader=csv.DictReader(f); rows=list(reader)
  assert all(None not in row and all(v is not None for v in row.values()) for row in rows),p
  counts[p.name]=len(rows)
links=0
for p in R.rglob('*.md'):
 for target in re.findall(r'\]\(([^)]+)\)',p.read_text()):
  if '://' in target or target.startswith('#'):continue
  path=target.split('#')[0]
  assert (p.parent/path).exists(),(p,target)
  links+=1
changed=subprocess.check_output(['git','-C',str(NX),'diff','--name-only','fecfbca9b0323578fd423f5d90486ddb2d0da72e'],text=True).splitlines()
assert all(p.startswith('docs/legality/research/') for p in changed),changed
print(json.dumps({'status':'PASS','registry_hashes_verified':len(registry),'csv_rows':counts,'local_links_checked':links,'tracked_changes_research_only':True}))
