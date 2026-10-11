#!/usr/bin/env python3
"""Research only: compare independently written IV model against retail C functions.
No ROM, save, production source or third-party source is written into this repo.
Usage: python reproduce_inheritance.py /absolute/path/to/research-sources
"""
import json, pathlib, subprocess, sys, tempfile
ROOT=pathlib.Path(sys.argv[1]).resolve()
OUT=pathlib.Path(__file__).resolve().parents[1]/'evidence'

def function(text,name):
    import re
    match=re.search(r'static void '+name+r'\([^;]*?\)\s*\{',text)
    if not match: raise ValueError(name)
    start=match.start(); pos=match.end(); depth=1
    while depth:
        depth += (text[pos]=='{')-(text[pos]=='}'); pos+=1
    return text[start:pos]

def model(seed,emerald):
    state=seed; draws=[]
    for _ in range(6):
        state=(state*0x41c64e6d+0x6073)&0xffffffff
        draws.append(state>>16)
    # Retail array retains a stale tail when compacted; do not use list.pop.
    available=list(range(6)); chosen=[]
    for i in range(3):
        chosen.append(available[draws[i]%(6-i)])
        available[i if emerald else chosen[-1]]=255
        compact=[v for v in available if v!=255]
        available[:len(compact)]=compact
    parents=[d%2 for d in draws[3:]]
    ivs=[0]*6
    for stat,parent in zip(chosen,parents): ivs[stat]=31 if parent else 30
    return {'seed_before_inherit':f'{seed:08X}','draws':draws,'selected_stats':chosen,
            'parents':parents,'final_ivs_hp_atk_def_spe_spa_spd':ivs,'final_seed':f'{state:08X}'}

stub=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t u8; typedef int32_t s32;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define NELEMS(a) ARRAY_COUNT(a)
#define NUM_STATS 6
#define INHERITED_IV_COUNT 3
#define DAYCARE_MON_COUNT 2
#define MON_DATA_HP_IV 0
#define MON_DATA_ATK_IV 1
#define MON_DATA_DEF_IV 2
#define MON_DATA_SPEED_IV 3
#define MON_DATA_SPATK_IV 4
#define MON_DATA_SPDEF_IV 5
struct BoxPokemon { u8 ivs[6]; };
struct Pokemon { u8 ivs[6]; };
struct DayCare { struct { struct BoxPokemon mon; } mons[2]; };
static uint32_t state;
static uint16_t Random(void){ state=state*0x41c64e6du+0x6073u; return state>>16; }
static u8 GetBoxMonData(void *p,int field){return ((struct BoxPokemon*)p)->ivs[field];}
static void SetMonData(struct Pokemon *p,int field,const void *v){p->ivs[field]=*(const u8*)v;}
'''
main=r'''
int main(int argc,char**argv){
 state=(uint32_t)strtoul(argv[1],0,0);
 struct DayCare d={0}; struct Pokemon e={0};
 for(int i=0;i<6;i++){d.mons[0].mon.ivs[i]=30;d.mons[1].mon.ivs[i]=31;}
 InheritIVs(&e,&d);
 for(int i=0;i<6;i++)printf("%u ",e.ivs[i]);
 printf("%u\n",state);return 0;
}
'''
result=[]
with tempfile.TemporaryDirectory(prefix='nx-inheritance-') as tmp:
    for repo in ['pokeemerald','pokeruby','pokefirered']:
        source=(ROOT/repo/'src/daycare.c').read_text()
        body=stub+function(source,'RemoveIVIndexFromList')+function(source,'InheritIVs')+main
        c=pathlib.Path(tmp)/(repo+'.c'); exe=c.with_suffix('');c.write_text(body)
        subprocess.run(['cc','-std=c99','-O0',str(c),'-o',str(exe)],check=True)
        for seed in [0,1,2,3,42,65535,0x12345678,0xffffffff]:
            expect=model(seed,repo=='pokeemerald')
            actual=list(map(int,subprocess.check_output([str(exe),str(seed)],text=True).split()))
            assert actual[:6]==expect['final_ivs_hp_atk_def_spe_spa_spd'],(repo,seed,actual,expect)
            assert actual[6]==int(expect['final_seed'],16)
            result.append({'game_source':repo,**expect,'independent_check':'PASS: unchanged extracted retail C vs independent Python',
                'scope':'synthetic IV-inheritance-stage vector; no PID, native PK3 or encounter legitimacy claim'})
OUT.mkdir(exist_ok=True)
(OUT/'inheritance-vectors.json').write_text(json.dumps(result,indent=2)+'\n')
print(f'PASS: {len(result)} source/model comparisons; retail default BUGFIX undefined')
