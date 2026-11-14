#!/usr/bin/env python3
"""Re-run unmodified existing Gen VI–VIII gates in an isolated lane checkout.
Does not check out, reset, patch or merge any repository. Build outputs are the
only mutations, under that checkout's build-research directory.
"""
import argparse,json,pathlib,subprocess,time
PIN='2f065ee2b0768d01bf621f969aef01120c6231e3'
p=argparse.ArgumentParser();p.add_argument('lane',type=pathlib.Path);p.add_argument('logs',type=pathlib.Path);a=p.parse_args();root=a.lane.resolve();a.logs.mkdir(parents=True,exist_ok=True)
sha=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()
if sha!=PIN:raise SystemExit('Refusing unpinned audit lane '+sha)
results=[]
for compiler,label in [('g++','gcc'),('clang++','clang')]:
 for sanitizer in [False,True]:
    mode='sanitize' if sanitizer else 'test';name=label+'-'+mode;log=a.logs/(name+'.log')
    cmd=['make','-f','Makefile.host',*[f'gen{g}-host-{mode}' for g in [6,7,8]],'CXX='+compiler,'HOST_BUILD=build-research/'+name,'ASAN_OPTIONS=detect_leaks=1']
    start=time.monotonic()
    with log.open('w') as f:r=subprocess.run(cmd,cwd=root,stdout=f,stderr=subprocess.STDOUT)
    results.append({'source_sha':sha,'command':cmd,'exit':r.returncode,'seconds':time.monotonic()-start,'log':str(log)})
    print(name,r.returncode,flush=True)
(a.logs/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(x['exit'] for x in results))
