#!/usr/bin/env python3
"""L5 Stage 4: exact-PKHeX-source-modeled vs real NX Time Capsule helper.

This does NOT execute a PKHeX verifier or Legality::analyze by default.
An optional paired live-adapter protocol runs BOTH actual external programs.
It never upgrades synthetic method-level tests to full 336-cell parity.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = 'tests/fixtures/legality_l5_stage4_pinned_oracle.cpp'
ORIGINAL_NX = 'include/Legality/Gen12TimeCapsuleEvidence.h'
NX_SHA = '4be5eda0dac2f43e886024d2febc07d9639abc18'
FIXTURE_SHA = '1e840be5a733b12457aef029c0ceea3978679ea4'
PKHEX_COMMIT = '6501f0ab46e8f8ca048539dbaf8cae8cb104e722'
PKHEX_SOURCES = {
  'PKHeX.Core/Legality/Verifiers/EffortValueVerifier.cs': '86c9bc6ea0361821df86e1473788c228ce7506a0',
  'PKHeX.Core/PKM/Util/EffortValues.cs': '5904496c4e60df3eb421bdf4c387f9df08f130ad',
  'PKHeX.Core/Legality/Restrictions/GBRestrictions.cs': '4e741e87568012384c50d245ba5c5953b95f2605',
}
NX_ANALYZER_SHA = 'b1d02d6620e8fffca2cf71874b786d44108e193f'
COUNTS = dict(comparisons=1638410,clean=128871,both_invalid=137178,
              reference_only=1238477,nx_only=3054,incomplete=127938,
              suspected=3054,history_checks=720885,domain=65285,assertions=4390400,
              runtime_pkhex=0,runtime_analyzer=0)
PATTERN = re.compile(r'^L5 Stage4 source-modeled pinned EV and true NX Time Capsule: '
  + r' '.join(k+r'=(?P<'+k+r'>\d+)' for k in COUNTS) +r'\n?$')


def gitblob(file:Path)->str:
    b=file.read_bytes()
    return hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()


def verify(file:Path,expected:str)->str:
    if not file.is_file():raise RuntimeError(f'missing required source: {file}')
    actual=gitblob(file)
    if actual!=expected:raise RuntimeError(f'exact source drift in {file}: {actual} != {expected}')
    return actual


def parse(stdout:str)->dict[str,int]:
    match=PATTERN.fullmatch(stdout)
    if not match:raise RuntimeError('not a valid original oracle attestation: '+repr(stdout[-300:]))
    numbers={k:int(v) for k,v in match.groupdict().items()}
    if numbers!=COUNTS:raise RuntimeError(f'oracle counts drift: {numbers!r}')
    expected=sum(numbers[k] for k in ('clean','both_invalid','reference_only','nx_only','incomplete','suspected'))
    # 162 hypothetical Gen I ancestry families have an unresolved Korean
    # restriction: those add historical_unknown once beyond EV comparisons.
    if expected-numbers['comparisons']!=162:
        raise RuntimeError('EV/historical-gap severity accounting not conserved')
    if numbers['runtime_pkhex'] or numbers['runtime_analyzer']:
        raise RuntimeError('source model asserted live executable success')
    return numbers


def build_and_run(cxx:str,sanitize:bool,where:Path,timeout:int)->dict:
    compiler=shutil.which(cxx)
    if compiler is None:raise RuntimeError(f'missing compiler {cxx}')
    executable=where/(cxx.replace('+','p')+('-san' if sanitize else '-normal'))
    opts=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'] if sanitize else ['-O2']
    cmd=[compiler,'-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror',
         *opts,'-Iinclude',FIXTURE,'-o',str(executable)]
    subprocess.run(cmd,cwd=ROOT,check=True,capture_output=True,text=True,timeout=timeout)
    env=os.environ.copy()
    env.update(ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    result=subprocess.run([str(executable)],cwd=ROOT,check=True,env=env,
                          text=True,capture_output=True,timeout=timeout)
    return {'compiler':cxx,'asan_ubsan':sanitize,'tests_executed':True,
            'assertions':parse(result.stdout)['assertions'],'attestation':result.stdout.strip()}


def canonical(case:dict)->bytes:
    return json.dumps(case,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode()


def parse_response(result:dict,engine:str,case:dict)->str:
    if not isinstance(result,dict) or result.get('schema')!='L5_STAGE4_LIVE_OBSERVATION_V1':
        raise RuntimeError('missing live adapter schema')
    if result.get('engine')!=engine or result.get('case_id')!=case['case_id']:
        raise RuntimeError('wrong engine or case identity')
    if result.get('case_sha256')!=hashlib.sha256(canonical(case)).hexdigest():
        raise RuntimeError('live observation input digest mismatch')
    if result.get('execution_mode')!='direct_runtime':
        raise RuntimeError('source-modeled/canned observation supplied as runtime')
    if result.get('verdict') not in ('Invalid','Incomplete','NoProblemsFound'):
        raise RuntimeError('invalid runtime verifier verdict')
    if not isinstance(result.get('check_ids'),list) or not all(isinstance(x,str) for x in result['check_ids']):
        raise RuntimeError('missing/check_ids type mismatch')
    return result['verdict']


def live_adapter(executable:Path,engine:str,case:dict,timeout:int)->str:
    if not executable.is_file():raise RuntimeError(f'live adapter executable unavailable: {executable}')
    proc=subprocess.run([str(executable),'--l5-stage4-json'],input=canonical(case).decode()+'\n',
                        capture_output=True,text=True,timeout=timeout,check=True)
    try:body=json.loads(proc.stdout)
    except json.JSONDecodeError as err:raise RuntimeError('live adapter did not return JSON') from err
    return parse_response(body,engine,case)


def runtime_compare(pkhex:Path,nx:Path,cases:Path,timeout:int)->dict:
    data=json.loads(cases.read_text(encoding='utf-8'))
    if data.get('schema')!='L5_STAGE4_SYNTHETIC_RUNTIME_VECTORS_V1':
        raise RuntimeError('incompatible runtime-vector schema')
    entries=data.get('cases')
    if not isinstance(entries,list) or not entries or len(entries)>64:
        raise RuntimeError('runtime vector count must be 1..64')
    seen=set();matrix=[]
    for entry in entries:
        if not isinstance(entry,dict) or not isinstance(entry.get('case_id'),str) or not entry['case_id']:
            raise RuntimeError('invalid case identity')
        if entry['case_id'] in seen:raise RuntimeError('duplicate case id')
        seen.add(entry['case_id'])
        p=live_adapter(pkhex,'PKHEX_CORE',entry,timeout)
        n=live_adapter(nx,'POKEBANK_NX_ANALYZER',entry,timeout)
        # Never translate NoProblemsFound into certified Legal: original
        # source provenance may still be missing for a synthetic specimen.
        matrix.append({'id':entry['case_id'],'pkhex_verdict':p,'nx_verdict':n,
                       'agreement':p==n,'historical_source_attested':False})
    return {'executed_live_observations':len(matrix),'runtime_protocol':'direct_two_executables',
            'pkhex_executable_sha256':hashlib.sha256(pkhex.read_bytes()).hexdigest(),
            'nx_executable_sha256':hashlib.sha256(nx.read_bytes()).hexdigest(),
            'observations':matrix,'authenticated_original_histories':0,
            'full_pkhex_parity_certified':False}


def selftest()->None:
    text='L5 Stage4 source-modeled pinned EV and true NX Time Capsule: '+\
        ' '.join(f'{key}={value}' for key,value in COUNTS.items())+'\n'
    assert parse(text)==COUNTS
    for corrupted in (text.replace('4390400','4390401'),
                      text.replace('runtime_pkhex=0','runtime_pkhex=1'),
                      text.replace('incomplete=127938','incomplete=0'),text+'FAKE',
                      text.replace('source-modeled','runtime-verified')):
        try:parse(corrupted)
        except RuntimeError:pass
        else:raise AssertionError('corrupted attestation accepted')
    case={'case_id':'negative','generation':2,'egg':True}
    valid={'schema':'L5_STAGE4_LIVE_OBSERVATION_V1','engine':'PKHEX_CORE',
           'case_id':'negative','case_sha256':hashlib.sha256(canonical(case)).hexdigest(),
           'execution_mode':'direct_runtime','verdict':'Incomplete','check_ids':[]}
    assert parse_response(valid,'PKHEX_CORE',case)=='Incomplete'
    for changed in ({**valid,'engine':'POKEBANK_NX_ANALYZER'},
                    {**valid,'execution_mode':'source_modeled'},
                    {**valid,'case_sha256':'0'*64},
                    {**valid,'verdict':'Legal'},
                    {**valid,'check_ids':'fabricated'},
                    {**valid,'case_id':'other'}):
        try:parse_response(changed,'PKHEX_CORE',case)
        except RuntimeError:pass
        else:raise AssertionError('invalid runtime receipt accepted')
    print('L5 Stage4 runner: 13 attestation/live-receipt controls (11 rejected, 2 accepted) PASS')


def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--self-test',action='store_true')
    ap.add_argument('--pkhex-root',type=Path)
    ap.add_argument('--native-model-root',type=Path)
    ap.add_argument('--runtime-pkhex-adapter',type=Path)
    ap.add_argument('--runtime-nx-adapter',type=Path)
    ap.add_argument('--runtime-cases',type=Path)
    ap.add_argument('--require-runtime',action='store_true')
    ap.add_argument('--timeout',type=int,default=300)
    ap.add_argument('--json-out',type=Path)
    args=ap.parse_args()
    if args.self_test:selftest();return 0
    if args.timeout<1:ap.error('timeout must be >0')
    pins={FIXTURE:verify(ROOT/FIXTURE,FIXTURE_SHA),
          ORIGINAL_NX:verify(ROOT/ORIGINAL_NX,NX_SHA)}
    if args.native_model_root:
        pins['accepted_nx_legality_cpp']=verify(
          args.native_model_root/'src/Legality/Legality.cpp',NX_ANALYZER_SHA)
    if args.pkhex_root:
        for path,sha in PKHEX_SOURCES.items():
            pins[path]=verify(args.pkhex_root/path,sha)
    with tempfile.TemporaryDirectory(prefix='l5-stage4-oracle-') as temp:
        rows=[build_and_run(cc,san,Path(temp),args.timeout)
              for cc in ('g++','clang++') for san in (False,True)]
    run_args=(args.runtime_pkhex_adapter,args.runtime_nx_adapter,args.runtime_cases)
    if args.require_runtime and not all(run_args):
        raise RuntimeError('runtime differential required but adapters/vectors unavailable: BLOCKED')
    if any(run_args) and not all(run_args):
        raise RuntimeError('partial runtime adapters cannot satisfy the live gate')
    live=runtime_compare(*run_args,args.timeout) if all(run_args) else None
    report={'schema':'L5_STAGE4_PINNED_DIFFERENTIAL_GATE_V1',
            'pkhex_revision':PKHEX_COMMIT,'exact_sources_verified':pins,
            'pkhex_sources_locally_verified':bool(args.pkhex_root),
            'native_accepted_analyzer_source_locally_verified':bool(args.native_model_root),
            'real_production_time_capsule_helper_executed':True,
            'source_modeled_ev_comparisons_per_binary':COUNTS['comparisons'],
            'live_pkhex_runtime_executed':bool(live),
            'live_nx_legality_analyzer_executed':bool(live),
            'live_runtime_evidence':live,'normal_and_sanitizer_runs':rows,
            'original_retail_receipts_authenticated':0,'certified_full_pkhex_parity_cells':0,
            'release_gate':'BLOCKED_NO_DIRECT_RUNTIME_OR_ORIGINALS' if not live else 'BLOCKED_MISSING_ORIGINAL_RETAIL_HISTORIES'}
    output=json.dumps(report,indent=2,sort_keys=True)+'\n'
    print(output,end='')
    if args.json_out:args.json_out.write_text(output,encoding='utf-8')
    return 0

if __name__=='__main__':raise SystemExit(main())
