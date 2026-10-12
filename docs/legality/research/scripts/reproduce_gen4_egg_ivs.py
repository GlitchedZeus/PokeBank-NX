#!/usr/bin/env python3
"""Independent Platinum/HGSS inheritance-stage model vs unchanged retail functions.
No PID, initial-IV creation, parent authenticity, or whole egg legality claim.
"""
import ctypes,hashlib,itertools,json,pathlib,re,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[1];S=pathlib.Path(sys.argv[1]).resolve()
manifest=json.loads((R/'evidence/repositories.json').read_text())
items={0:None,294:0,289:1,290:2,293:3,291:4,292:5}
itemtext=(S/'pokeheartgold/include/constants/items.h').read_text()
for name,value in [('WEIGHT',294),('BRACER',289),('BELT',290),('ANKLET',293),('LENS',291),('BAND',292)]:
 assert re.search(r'#define ITEM_POWER_'+name+r'\s+'+str(value)+r'\b',itemtext)
def function(source,name):
 m=re.search(r'static (?:void|BOOL) '+name+r'\([^;]*?\)\s*\{',source);assert m,name
 pos=m.end();depth=1
 while depth:depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
 return source[m.start():pos]
def model(seed,power0,power1,platinum=False):
 state=seed;draws=[];available=list(range(6));selected=[];donors=[]
 def draw():
  nonlocal state
  state=(state*0x41c64e6d+0x6073)&0xffffffff;draws.append(state>>16);return draws[-1]
 powers=[items[power0],items[power1]]
 if not platinum and any(x is not None for x in powers):
  donor=(0 if draw()%2 else 1) if all(x is not None for x in powers) else (0 if powers[0] is not None else 1)
  selected.append(powers[donor]);donors.append(donor);available.remove(powers[donor])
 start=len(selected)
 for i in range(start,3):
  index=draw()%(6-i);selected.append(available[index])
  if platinum:
   # Preserve the fixed-array stale tail exactly, matching Emerald-style removal.
   available[i]=255;compacted=[x for x in available if x!=255];available[:len(compacted)]=compacted
  else:available.pop(index)
 for i in range(start,3):donors.append(draw()%2)
 initial=[(seed+i*3)%32 for i in range(6)]
 parents=[[(seed*3+i*5+7)%32 for i in range(6)],[(seed*7+i*11+13)%32 for i in range(6)]]
 child=initial.copy()
 for stat,donor in zip(selected,donors):child[stat]=parents[donor][stat]
 return dict(entry_seed=f'{seed:08X}',held_items=[power0,power1],draws=draws,draw_count=len(draws),selected_stats=selected,donor_slots=donors,initial_ivs=initial,parents_slot_order=parents,child_ivs=child,final_seed=f'{state:08X}')
stub=r'''
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define GF_ASSERT(x) assert(x)
#define NUM_STATS 6
#define STAT_MAX 6
#define NUM_INHERITED_IVS 3
#define NUM_DAYCARE_MONS 2
#define STAT_HP 0
#define STAT_ATK 1
#define STAT_DEF 2
#define STAT_SPEED 3
#define STAT_SPATK 4
#define STAT_SPDEF 5
#define STAT_ATTACK 1
#define STAT_DEFENSE 2
#define STAT_SPECIAL_ATTACK 4
#define STAT_SPECIAL_DEFENSE 5
#define MON_DATA_HP_IV 0
#define MON_DATA_ATK_IV 1
#define MON_DATA_DEF_IV 2
#define MON_DATA_SPEED_IV 3
#define MON_DATA_SPATK_IV 4
#define MON_DATA_SPDEF_IV 5
#define MON_DATA_HELD_ITEM 6
#define ITEM_POWER_WEIGHT 294
#define ITEM_POWER_BRACER 289
#define ITEM_POWER_BELT 290
#define ITEM_POWER_ANKLET 293
#define ITEM_POWER_LENS 291
#define ITEM_POWER_BAND 292
typedef struct {u8 ivs[6];u16 item;} BoxPokemon;
typedef struct {u8 ivs[6];} Pokemon;
typedef struct {BoxPokemon parents[2];} Daycare;
static uint32_t state,calls;
static u16 LCRandom(void){state=state*0x41c64e6du+0x6073u;calls++;return state>>16;}
#define LCRNG_Next LCRandom
static BoxPokemon* Daycare_GetBoxMonI(Daycare* d,unsigned i){return &d->parents[i];}
#define Daycare_GetBoxMon Daycare_GetBoxMonI
static u16 GetBoxMonData(BoxPokemon* p,unsigned f,void* unused){return f==6?p->item:p->ivs[f];}
#define BoxPokemon_GetValue GetBoxMonData
static void SetMonData(Pokemon* p,unsigned f,const void* value){p->ivs[f]=*(const u8*)value;}
#define Pokemon_SetValue SetMonData
'''
wrapper=r'''
void probe(uint32_t seed,unsigned item0,unsigned item1,uint32_t* out){
 state=seed;calls=0;Daycare d={0};Pokemon e={0};
 for(unsigned i=0;i<6;i++){
  e.ivs[i]=(seed+i*3)%32;
  d.parents[0].ivs[i]=(seed*3+i*5+7)%32;
  d.parents[1].ivs[i]=(seed*7+i*11+13)%32;
 }
 d.parents[0].item=item0;d.parents[1].item=item1;
 RUN_INHERIT(&e,&d);
 for(unsigned i=0;i<6;i++)out[i]=e.ivs[i];out[6]=state;out[7]=calls;
}
'''
records=[];summaries=[];seeds=list(range(256))+[65535,0x12345678,0xffffffff]
for repo,path,names in [('pokeheartgold','src/get_egg.c',['SetStatIdToPowerItemStat','Daycare_TryGetForcedInheritedIV','_IVList_Remove','InheritIVs']),('pokeplatinum','src/overlay005/daycare.c',['RemoveIVIndexFromList','Egg_InheritIVs'])]:
 pin=manifest[repo]['revision'];folder=S/repo
 assert subprocess.check_output(['git','-C',str(folder),'rev-parse','HEAD'],text=True).strip()==pin
 text=(folder/path).read_text();digest=hashlib.sha256();count=0;draw_counts={};repeat_cases=0
 with tempfile.TemporaryDirectory(prefix='nx-gen4-egg-') as tmp:
  p=pathlib.Path(tmp)/'probe.c';lib=p.with_suffix('.so')
  p.write_text(stub+'\n'.join(function(text,name) for name in names)+wrapper.replace('RUN_INHERIT',names[-1]))
  subprocess.run(['cc','-std=c99','-O2','-shared','-fPIC',str(p),'-o',str(lib)],check=True)
  api=ctypes.CDLL(str(lib));api.probe.argtypes=[ctypes.c_uint32,ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint32)]
  for seed,item0,item1 in itertools.product(seeds,items,items):
   e=model(seed,item0,item1,repo=='pokeplatinum');a=(ctypes.c_uint32*8)();api.probe(seed,item0,item1,a)
   assert list(a[:6])==e['child_ivs'] and a[6]==int(e['final_seed'],16) and a[7]==e['draw_count'],(repo,e,list(a))
   rewound=a[6]
   for _ in range(e['draw_count']):rewound=((rewound-0x6073)*0xeeb9eb65)&0xffffffff
   assert rewound==seed
   if repo=='pokeheartgold':assert len(set(e['selected_stats']))==3
   repeat_cases+=len(set(e['selected_stats']))<3
   count+=1;draw_counts[str(e['draw_count'])]=draw_counts.get(str(e['draw_count']),0)+1
   digest.update(json.dumps(e,sort_keys=True,separators=(',',':')).encode())
   # Full item-pair matrix at seed 0 plus five branch controls at four other seeds.
   if seed==0 or (seed in (1,2,0x12345678,0xffffffff) and (item0,item1) in ((0,0),(294,0),(0,294),(294,294),(294,289))):records.append({'game':repo,**e,'verified':'unchanged retail C vs independent Python'})
 summaries.append(dict(repository='pret/'+repo,revision=pin,path=path,source_sha256=hashlib.sha256(text.encode()).hexdigest(),comparisons=count,known_branch_rewind_checks=count,draw_counts=draw_counts,repeated_stat_cases=repeat_cases,case_digest_sha256=digest.hexdigest()))
result={'scope':'synthetic inheritance stage; physical daycare slots; varied stub parent/initial IVs; no PID or native legitimacy assertion','summaries':summaries,'item_to_stat':items,'stat_order':['HP','Atk','Def','Spe','SpA','SpD'],'fixtures':records}
(R/'evidence/gen4-egg-iv-vectors.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'comparisons':sum(s['comparisons'] for s in summaries),'stored_fixtures':len(records),'summaries':summaries}))
