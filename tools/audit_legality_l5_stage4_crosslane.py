#!/usr/bin/env python3
"""L5 stage 4: source-pinned, cross-lane ACTUAL C++ helper regression.

Runs exact bytes from four existing PR branches, via separate checkout roots.
Tests are L5-only files; never mutate any GitHub branch or original save.
A missing or divergent source blob BLOCKS (not PASS); normal AND sanitizer
executions of ALL four helpers required. Optional --clang adds another mode.

On a full L5 checkout (with four separate sibling lane checkouts):
 python3 tools/audit_legality_l5_stage4_crosslane.py --l1-root ../L1 \
  --l2-root ../L2 --l3-root ../L3 --l4-root ../L4 --clang
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
FILES={
 'L1': {'Gen4HgssOriginalTreeStage.h':'9c767ede360d503baa5831db3ce9c6eff04c9441'},
 'L2': {'Gen4DiamondPearlEggIVStage.h':'6b1d2a7bc2299fe7780deba849caf033fa2ff0f5'},
 'L3': {'Gen14CandidateMoveGbaStarterTables.h':'c60d5594d92bae65b8e4d1f46e2dbede27227a40',
        'Gen14CandidateMoveGbaNinjaTables.h':'ed5510aedd2440bfd7d6bd6e29650d5d2ebbe4dd'},
 'L4': {'Gen4EventClockReceiptEvidence.h':'69506bbc1b75355991a591415c64312311949e00'},
}
FIXTURES={
 'L1': ('legality_l5_stage4_l1_tree_adversarial.cpp',635893),
 'L2': ('legality_l5_stage4_l2_egg_iv_adversarial.cpp',327683),
 'L3': ('legality_l5_stage4_l3_move_source_adversarial.cpp',885145),
 'L4': ('legality_l5_stage4_l4_clock_adversarial.cpp',828165),
}
FIXTURE_BLOBS={
 'L1':'d21f366d1a0f54b31557ba4b96447a617d66787e',
 'L2':'a553326572f742cabbe5ec4bc8baeeea1f85c5aa',
 'L3':'59442ec6de0add4fdd1fea98aa98221ce5f65e72',
 'L4':'61c4f16a94bf53ef11d455ae1ec44cb421f2210e',
}
EXPECTED_TOTAL=sum(c for _,c in FIXTURES.values())

def blob(file:Path)->str:
 data=file.read_bytes()
 return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()

def exact_file(path:Path,sha:str)->None:
 if not path.is_file():raise FileNotFoundError(f'Exact source file missing: {path}')
 actual=blob(path)
 if actual!=sha:raise RuntimeError(f'Git blob mismatch {path}: expected {sha}; got {actual}')

def source_overlay(roots:dict[str,Path],overlay:Path)->None:
 dest=overlay/'Legality';dest.mkdir(parents=True)
 for lane,files in FILES.items():
  for name,sha in files.items():
   source=roots[lane]/'include'/'Legality'/name
   exact_file(source,sha)
   # Copy pinned bytes into temp overlay, never alter/check out sibling lane.
   target=dest/name
   target.write_bytes(source.read_bytes())
   exact_file(target,sha)

def checked_stdout(text:str,lane:str)->int:
 pattern=rf'^L5 Stage4 actual {lane} .+ PASS (\d+) assertions; .+$'
 match=re.fullmatch(pattern,text.strip())
 if not match:raise RuntimeError('No exact source-helper attestation for '+lane+': '+text)
 count=int(match.group(1))
 if count!=FIXTURES[lane][1]:raise RuntimeError(f'{lane} count drift: {count}')
 return count

def run(roots:dict[str,Path],clang:bool)->dict:
 evidence=[]
 with tempfile.TemporaryDirectory(prefix='l5-stage4-actual-') as temp:
  tmp=Path(temp);inc=tmp/'include';source_overlay(roots,inc)
  for lane,(name,count) in FIXTURES.items():
   source=ROOT/'tests'/'fixtures'/name
   exact_file(source,FIXTURE_BLOBS[lane])
   for mode in (('g++',False),('g++',True),*([('clang++',False)] if clang else [])):
    compiler,sanitized=mode
    executable=tmp/f"{lane}-{compiler}-{'san' if sanitized else 'opt'}"
    flags=['-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror']
    flags+=(['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitized else ['-O2'])
    subprocess.run([compiler,*flags,'-I',str(inc),str(source),'-o',str(executable)],
                   check=True,capture_output=True,text=True)
    env=os.environ.copy()
    if sanitized:
     env['ASAN_OPTIONS']='detect_leaks=0:halt_on_error=1'
     env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
    proc=subprocess.run([str(executable)],check=True,capture_output=True,text=True,env=env)
    if checked_stdout(proc.stdout,lane)!=count:raise AssertionError(lane)
    evidence.append({'lane':lane,'compiler':compiler,'asan_ubsan':sanitized,
                     'executed':True,'assertions':count,'status':'PASS'})
 for lane in FILES:
  if sum(e['lane']==lane and e['asan_ubsan'] for e in evidence)!=1:
   raise RuntimeError(f'Sanitizer gate missing for {lane}')
  if sum(e['lane']==lane and not e['asan_ubsan'] and e['compiler']=='g++' for e in evidence)!=1:
   raise RuntimeError(f'Normal gate missing for {lane}')
 return {'stage':'L5_STAGE4_ACTUAL_CROSSLANE','status':'PASS_SOURCE_PINNED_HELPERS',
         'source_github_blob_pins_verified':True,'actual_helper_headers_compiled':True,
         'normal_and_asan_ubsan_required':True,'per_mode_total_assertions':EXPECTED_TOTAL,
         'authentic_retail_encounter_histories':0,'global_invalid_conclusions':0,
         'full_integration_analyzer_executed':False,'sibling_sources_modified':False,
         'runs':evidence}

def self_test()->None:
 assert EXPECTED_TOTAL==2676886
 for lane,(_,count) in FIXTURES.items():
  example=f'L5 Stage4 actual {lane} helper PASS {count} assertions; attested source-scoped controls'
  assert checked_stdout(example,lane)==count
  for invalid in (example.replace(str(count),str(count+1)),example.replace('PASS','UNKNOWN')):
   try:checked_stdout(invalid,lane)
   except RuntimeError:continue
   raise AssertionError('Attestation false positive')
 print('L5 Stage4 source-pinned runner self-test PASS (12 positive/negative checks)')

def main()->None:
 ap=argparse.ArgumentParser(description=__doc__)
 for lane in FILES:ap.add_argument(f'--{lane.lower()}-root',type=Path)
 ap.add_argument('--clang',action='store_true')
 ap.add_argument('--self-test',action='store_true')
 ap.add_argument('--json-out',type=Path)
 args=ap.parse_args()
 if args.self_test:self_test();return
 roots={lane:getattr(args,lane.lower()+'_root') or ROOT for lane in FILES}
 report=run(roots,args.clang)
 output=json.dumps(report,sort_keys=True,indent=2)+'\n'
 print(output,end='')
 if args.json_out:args.json_out.write_text(output,encoding='utf-8')
if __name__=='__main__':main()
