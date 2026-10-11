#!/usr/bin/env python3
"""Find a deterministic Method K nature-loop witness beyond the 128-search window.
Usage: python reproduce_nature_window.py /path/to/research-sources
Synthetic RNG-stage evidence only; does not claim full encounter legality.
"""
import json,pathlib,subprocess,sys,tempfile
S=pathlib.Path(sys.argv[1]).resolve();R=pathlib.Path(__file__).resolve().parents[1];NX=R.parents[2]
# Refuse silent source drift against the recorded audit revisions.
manifest=json.loads((pathlib.Path(__file__).resolve().parents[1]/'evidence/repositories.json').read_text())
for repo in ['pokeheartgold']:
    actual=subprocess.check_output(['git','-C',str(S/repo),'rev-parse','HEAD'],text=True).strip()
    assert actual==manifest[repo]['revision'], ('source revision mismatch',repo,actual)
def nxt(s):return (s*0x41c64e6d+0x6073)&0xffffffff
def prev(s):return ((s-0x6073)*pow(0x41c64e6d,-1,2**32))&0xffffffff
def reverse(s,n,bound):
 for i in range(bound):
  hi=s>>16;s=prev(s);lo=s>>16
  if ((hi<<16)|lo)%25==n:return i
  s=prev(s)
 return -1
for seed in range(100000):
 nature_seed=nxt(seed);nature=(nature_seed>>16)%25;s=nature_seed; rejected=[]
 while True:
  before=s;s=nxt(s);lo=s>>16;s=nxt(s);pid=((s>>16)<<16)|lo
  if pid%25==nature:break
  rejected.append(pid)
 if len(rejected)>=128 and reverse(before,nature,128)==-1:break
else:raise AssertionError('no deterministic witness')
ivseed=nxt(s);iv1=(ivseed>>16)&32767;ivseed=nxt(ivseed);iv2=(ivseed>>16)&32767
retail=(S/'pokeheartgold/src/pokemon.c').read_text(); start=retail.index('void CreateMonWithNature(');end=retail.index('\n}',start)+2;func=retail[start:end]
code=r'''
#include <cstdint>
#include <cstdio>
#include "Legality/Gen4LeadFrameEvidence.h"
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;
#define TRUE 1
struct Pokemon {u32 pid;};
static u32 state;
u32 LCRandom(){state=state*0x41c64e6d+0x6073;return state>>16;}
u8 GetNatureFromPersonality(u32 p){return p%25;}
void CreateMon(Pokemon*m,int,int,int,int,int pid,int,int){m->pid=(u32)pid;}
'''+func+f'''
int main(){{state={nature_seed}u;Pokemon p{{}};CreateMonWithNature(&p,19,5,32,{nature});
std::printf("%u %u %d\\n",p.pid,state,Legality::Gen4LeadFrame::reversalWindow({before}u,{nature}));}}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=pathlib.Path(tmp)/'a.cpp';p.write_text(code);exe=p.with_suffix('')
 subprocess.run(['g++','-std=c++17','-O0','-I'+str(NX/'include'),str(p),'-o',str(exe)],check=True)
 observed=list(map(int,subprocess.check_output([str(exe)],text=True).split()))
 assert observed==[pid,s,-1],observed
out={'scope':'synthetic nature/PID/IV-stage witness; no native or fully matched encounter claim','seed_before_nature':f'{seed:08X}','seed_after_nature':f'{nature_seed:08X}','nature':nature,'rejected_pid_count':len(rejected),'rejected_pids':[f'{p:08X}' for p in rejected],'seed_before_retained_pid':f'{before:08X}','retained_pid':f'{pid:08X}','iv_words':[iv1,iv2],'bounded_reversal_result':-1,'extended_reversal_result':reverse(before,nature,100000),'independent_verification':'PASS: original HGSS CreateMonWithNature hosted with RNG stub agrees with independent Python; accepted NX helper returns -1'}
(R/'evidence/nature-window-vector.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({k:v for k,v in out.items() if k!='rejected_pids'},indent=2))
