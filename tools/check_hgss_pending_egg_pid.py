#!/usr/bin/env python3
"""Compare implementation with unchanged pinned HGSS functions, using synthetic streams.
Usage: python tools/check_hgss_pending_egg_pid.py /path/to/pokeheartgold
Retail function bodies live only in temporary compiler files, not this repository.
"""
import ctypes,itertools,pathlib,re,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1];S=pathlib.Path(sys.argv[1]).resolve()
PIN='9d8b7591f09b65804da2fb2dfd56f320633e0d36'
assert subprocess.check_output(['git','-C',str(S),'rev-parse','HEAD'],text=True).strip()==PIN
source=subprocess.check_output(['git','-C',str(S),'show',PIN+':src/get_egg.c'],text=True)
def function(name):
 m=re.search(r'static (?:int|void) '+name+r'\([^;]*?\)\s*\{',source);assert m
 pos=m.end();depth=1
 while depth:depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
 return source[m.start():pos]
stub=r'''
#include <cstdint>
#include <cassert>
#include <cstddef>
#include "Legality/Gen4HGSSPendingEggPid.h"
using u8=uint8_t;using u32=uint32_t;
#define NUM_DAYCARE_MONS 2
#define ITEM_EVERSTONE 229
#define MON_DATA_HELD_ITEM 0
#define MON_DATA_PERSONALITY 1
struct BoxPokemon {uint32_t item,pid;};
struct Daycare {BoxPokemon parents[2];uint32_t pending;};
static uint32_t state,lcCount,mtCount,mtSize;static const uint32_t* stream;
static void Daycare_GetBothBoxMonsPtr(Daycare* d,BoxPokemon** p){p[0]=&d->parents[0];p[1]=&d->parents[1];}
static BoxPokemon* Daycare_GetBoxMonI(Daycare* d,int i){return &d->parents[i];}
static uint32_t GetBoxMonData(BoxPokemon* p,int field,void*){return field?p->pid:p->item;}
static uint32_t GetNatureFromPersonality(uint32_t pid){return pid%25;}
static uint16_t LCRandom(){++lcCount;state=state*0x41c64e6du+0x6073u;return state>>16;}
static uint32_t MTRandom(){assert(mtCount<mtSize);return stream[mtCount++];}
static void Save_Daycare_SetEggPID(Daycare* d,uint32_t pid){d->pending=pid;}
'''
wrapper=r'''
extern "C" void probe(uint32_t seed,uint32_t item0,uint32_t item1,uint32_t pid0,uint32_t pid1,
 const uint32_t* values,uint32_t size,uint32_t* out){
 Daycare d{{{item0,pid0},{item1,pid1}},0};state=seed;lcCount=mtCount=0;stream=values;mtSize=size;
 GenerateEggPID(&d);
 namespace H=Legality::Gen4HGSSPendingEggPid;
 H::Input in{};in.game="heartgold_nds";in.parentItemsAndOrderVerified=true;
 in.heldItems={static_cast<uint16_t>(item0),static_cast<uint16_t>(item1)};
 in.parentPidKnown={true,true};in.parentPid={pid0,pid1};
 in.creationStageVerified=true;in.parentStageToken=in.mtStageToken=1;
 in.lcEntryStateKnown=true;in.lcEntryState=seed;
 in.consecutiveMTOutputsVerified=in.exactConsumedMTCountVerified=true;
 in.mtOutputs={values,mtCount};auto r=H::replay(in);
 out[0]=d.pending;out[1]=state;out[2]=mtCount;out[3]=lcCount;
 out[4]=r.pendingPid;out[5]=r.lcFinalState;out[6]=r.mtConsumed;out[7]=r.lcConsumed;
 out[8]=r.status==H::Status::ReplayedPendingPid;out[9]=r.retailRetryLimitReached;
}
'''
seeds=[0,1,2,42,0x12345678,0xffffffff]
# Directly target each side of the exact 0x7FFF proc boundary.
seeds += [(((roll<<16)-0x6073)*0xeeb9eb65)&0xffffffff for roll in (32766,32767,32768)]
count=0;fallbacks=0
with tempfile.TemporaryDirectory(prefix='hgss-pid-source-') as tmp:
 p=pathlib.Path(tmp)/'probe.cpp';lib=p.with_suffix('.so')
 p.write_text(stub+function('Daycare_EverstoneCheck')+function('GenerateEggPID')+wrapper)
 subprocess.run(['c++','-std=c++20','-shared','-fPIC','-O2','-I'+str(ROOT/'include'),str(p),'-o',str(lib)],check=True)
 api=ctypes.CDLL(str(lib));api.probe.argtypes=[ctypes.c_uint32]*5+[ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)]
 for seed,items,nature in itertools.product(seeds,[(0,0),(229,0),(0,229),(229,229)],range(25)):
  values=(ctypes.c_uint32*101)(*range(101));out=(ctypes.c_uint32*10)()
  api.probe(seed,*items,nature,(nature+7)%25,values,len(values),out)
  assert list(out[:4])==list(out[4:8]) and out[8],(seed,items,nature,list(out))
  count+=1
 # Full retail loop endpoint: zero is rejected even for nature zero;
 # retained 2401st value may therefore fail the inherited nature condition.
 for tail in (0,1,25):
  vals=[0]*2400+[tail];values=(ctypes.c_uint32*len(vals))(*vals);out=(ctypes.c_uint32*10)()
  api.probe(0,229,0,0,7,values,len(values),out)
  assert list(out[:4])==list(out[4:8]) and out[8] and out[2]==2401,list(out)
  assert bool(out[9])==(tail!=25);count+=1;fallbacks+=bool(out[9])
print(f'PASS: {count} retail/implementation comparisons, including {fallbacks} exhausted-retail-loop fallbacks; synthetic MT/accessor inputs')
