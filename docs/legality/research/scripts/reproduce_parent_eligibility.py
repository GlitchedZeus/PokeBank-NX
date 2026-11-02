#!/usr/bin/env python3
"""Host unchanged Emerald eligibility and role functions with explicit accessor stubs.
Abstract category vectors test control flow, not a full species dataset or native records.
"""
import ctypes,hashlib,itertools,json,pathlib,re,subprocess,sys,tempfile
R=pathlib.Path(__file__).resolve().parents[1];S=pathlib.Path(sys.argv[1]).resolve()/'pokeemerald'
manifest=json.loads((R/'evidence/repositories.json').read_text());rev=manifest['pokeemerald']['revision']
assert subprocess.check_output(['git','-C',str(S),'rev-parse','HEAD'],text=True).strip()==rev
source=(S/'src/daycare.c').read_text()
def fn(name,source=source):
 m=re.search(r'static (?:bool8|u8|u16|BOOL) '+name+r'\([^;]*?\)\s*\{',source);assert m,name
 p=m.end();depth=1
 while depth:depth+=(source[p]=='{')-(source[p]=='}');p+=1
 return source[m.start():p]
stub=r'''
#include <stdint.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;typedef uint8_t bool8;
#define DAYCARE_MON_COUNT 2
#define EGG_GROUPS_PER_MON 2
#define TRUE 1
#define FALSE 0
#define MON_DATA_SPECIES 0
#define MON_DATA_OT_ID 1
#define MON_DATA_PERSONALITY 2
#define MON_FEMALE 254
#define MON_GENDERLESS 255
#define EGG_GROUP_DITTO 13
#define EGG_GROUP_NO_EGGS_DISCOVERED 15
#define PARENTS_INCOMPATIBLE 0
#define PARENTS_LOW_COMPATIBILITY 20
#define PARENTS_MED_COMPATIBILITY 50
#define PARENTS_MAX_COMPATIBILITY 70
#define EGG_GENDER_MALE 0x8000
// Local stub labels, not serialized species IDs.
#define SPECIES_DITTO 40
#define SPECIES_NIDORAN_F 41
#define SPECIES_NIDORAN_M 42
#define SPECIES_ILLUMISE 43
#define SPECIES_VOLBEAT 44
struct BoxPokemon {u16 species;u32 ot;u32 gender;};
struct DayCare {struct {struct BoxPokemon mon;} mons[2];u32 offspringPersonality;};
static struct {u16 eggGroups[2];} gSpeciesInfo[64];
static u32 GetBoxMonData(struct BoxPokemon* p,int f){return f==0?p->species:f==1?p->ot:p->gender;}
static u32 GetGenderFromSpeciesAndPersonality(u16 species,u32 pid){return pid;}
static u32 GetBoxMonGender(struct BoxPokemon* p){return p->gender;}
// Role algorithm receives a pre-resolved baby lineage in this harness.
static u16 GetEggSpecies(u16 species){return species;}
'''
wrapper=r'''
unsigned score(unsigned a,unsigned b,unsigned ga,unsigned gb,unsigned idDiff,unsigned a0,unsigned a1,unsigned b0,unsigned b1){
 struct DayCare d={0};d.mons[0].mon=(struct BoxPokemon){a,100,ga};
 d.mons[1].mon=(struct BoxPokemon){b,idDiff?200:100,gb};
 gSpeciesInfo[a].eggGroups[0]=a0;gSpeciesInfo[a].eggGroups[1]=a1;
 gSpeciesInfo[b].eggGroups[0]=b0;gSpeciesInfo[b].eggGroups[1]=b1;
 return GetDaycareCompatibilityScore(&d);
}
unsigned roles(unsigned a,unsigned b,unsigned ga,unsigned gb,unsigned pid){
 struct DayCare d={0};d.mons[0].mon=(struct BoxPokemon){a,0,ga};d.mons[1].mon=(struct BoxPokemon){b,0,gb};d.offspringPersonality=pid;
 u8 slots[2]={255,255};unsigned baby=DetermineEggSpeciesAndParentSlots(&d,slots);
 return baby | ((unsigned)slots[0]<<8) | ((unsigned)slots[1]<<16);
}
'''
profiles=[(1,1),(1,2),(2,3),(3,4),(4,4),(5,5),(13,13),(15,15),(2,5)]
def expected(a,b,ga,gb,diff):
 ag=profiles[a//2];bg=profiles[b//2]
 if 15 in (ag[0],bg[0]) or ag[0]==bg[0]==13:return 0
 if 13 in (ag[0],bg[0]):return 50 if diff else 20
 if ga==gb or 255 in (ga,gb) or not set(ag)&set(bg):return 0
 return (70 if diff else 50) if a==b else (50 if diff else 20)
counts={};digest=hashlib.sha256();rolecases=[]
with tempfile.TemporaryDirectory(prefix='nx-parent-eligibility-') as tmp:
 p=pathlib.Path(tmp)/'probe.c';lib=p.with_suffix('.so')
 p.write_text(stub+fn('EggGroupsOverlap')+fn('GetDaycareCompatibilityScore')+fn('DetermineEggSpeciesAndParentSlots')+wrapper)
 subprocess.run(['cc','-std=c99','-O2','-shared','-fPIC',str(p),'-o',str(lib)],check=True)
 api=ctypes.CDLL(str(lib));api.score.argtypes=[ctypes.c_uint]*9;api.score.restype=ctypes.c_uint;api.roles.argtypes=[ctypes.c_uint]*5;api.roles.restype=ctypes.c_uint
 for a,b,ga,gb,diff in itertools.product(range(18),range(18),(0,254,255),(0,254,255),(0,1)):
  actual=api.score(a+1,b+1,ga,gb,diff,*profiles[a//2],*profiles[b//2]);want=expected(a,b,ga,gb,diff)
  assert actual==want,(a,b,ga,gb,diff,actual,want)
  counts[str(actual)]=counts.get(str(actual),0)+1;digest.update(f'{a},{b},{ga},{gb},{diff},{actual}\n'.encode())
 # Valid role shapes, pre-resolved lineage labels. Ditto + non-Ditto or ordinary opposite genders.
 for label,a,b,ga,gb in [('female_plus_ditto',20,40,254,255),('male_plus_ditto',20,40,0,255),('genderless_plus_ditto',20,40,255,255),('ordinary_pair',20,21,254,0),('nidoran_f_plus_ditto',41,40,254,255),('illumise_plus_ditto',43,40,254,255)]:
  for reversed_order,pid in itertools.product((False,True),(0,32768)):
   aa,bb,gga,ggb=(b,a,gb,ga) if reversed_order else (a,b,ga,gb)
   value=api.roles(aa,bb,gga,ggb,pid);baby=value&255;mother=(value>>8)&255;father=(value>>16)&255
   originalfemale=1 if reversed_order else 0
   wantfather=originalfemale if label in ('male_plus_ditto','genderless_plus_ditto') else 1-originalfemale
   wantbaby=42 if a==41 and pid else 44 if a==43 and pid else a
   assert (baby,mother,father)==(wantbaby,1-wantfather,wantfather)
   rolecases.append(dict(case=label,reversed_deposit_order=reversed_order,pid_bit15=bool(pid),baby_stub_label=baby,move_mother_slot=mother,move_father_slot=father))
# Independently execute the equivalent Platinum and HGSS eligibility functions too.
additional=[]
for repo,path,overlap,scorefn in [
 ('pokeheartgold','src/get_egg.c','EggGroupsHaveMatch','ComputeCompatibilityBetweenBoxMons'),
 ('pokeplatinum','src/overlay005/daycare.c','Egg_AreEggGroupsCompatible','BoxMon_GetPairDaycareCompatibilityScore')]:
 folder=S.parent/repo
 pin=manifest[repo]['revision']
 assert subprocess.check_output(['git','-C',str(folder),'rev-parse','HEAD'],text=True).strip()==pin
 text=(folder/path).read_text()
 extra_stub=r'''
#include <stddef.h>
typedef struct BoxPokemon BoxPokemon;typedef int BOOL;
#define NUM_DAYCARE_MONS 2
#define MAX_EGG_GROUPS 2
#define EGG_GROUP_UNDISCOVERED 15
#define GENDER_NONE 255
#define BASE_EGG_GROUP_1 0
#define BASE_EGG_GROUP_2 1
#define SPECIES_DATA_EGG_GROUP_1 0
#define SPECIES_DATA_EGG_GROUP_2 1
#define GetBoxMonData(p,f,unused) GetBoxMonData(p,f)
#define BoxPokemon_GetValue(p,f,unused) GetBoxMonData(p,f,NULL)
#define GetGenderBySpeciesAndPersonality(s,p) GetGenderFromSpeciesAndPersonality(s,p)
#define Pokemon_GetGenderOf(s,p) GetGenderFromSpeciesAndPersonality(s,p)
#define GetMonBaseStat(s,f) (gSpeciesInfo[s].eggGroups[f])
#define SpeciesData_GetSpeciesValue(s,f) (gSpeciesInfo[s].eggGroups[f])
'''
 scorewrap=wrapper.split('unsigned roles(',1)[0].replace('return GetDaycareCompatibilityScore(&d);','BoxPokemon *parents[2]={&d.mons[0].mon,&d.mons[1].mon};return '+scorefn+'(parents);')
 with tempfile.TemporaryDirectory(prefix='nx-parent-gen4-') as tmp:
  p=pathlib.Path(tmp)/'probe.c';lib=p.with_suffix('.so')
  p.write_text(stub+extra_stub+fn(overlap,text)+fn(scorefn,text)+scorewrap)
  subprocess.run(['cc','-std=c99','-O2','-shared','-fPIC',str(p),'-o',str(lib)],check=True)
  api=ctypes.CDLL(str(lib));api.score.argtypes=[ctypes.c_uint]*9;api.score.restype=ctypes.c_uint
  compared=0
  for a,b,ga,gb,diff in itertools.product(range(18),range(18),(0,254,255),(0,254,255),(0,1)):
   actual=api.score(a+1,b+1,ga,gb,diff,*profiles[a//2],*profiles[b//2])
   assert actual==expected(a,b,ga,gb,diff),(repo,a,b,ga,gb,diff,actual)
   compared+=1
 additional.append(dict(repository='pret/'+repo,revision=pin,source_path=path,source_sha256=hashlib.sha256(text.encode()).hexdigest(),eligibility_cases=compared,scope='same abstract eligibility control-flow inputs; no role function executed'))
result={'repository':'pret/pokeemerald','revision':rev,'source_path':'src/daycare.c','source_sha256':hashlib.sha256(source.encode()).hexdigest(),'eligibility_cases':sum(counts.values()),'additional_retail_checks':additional,'total_eligibility_comparisons':sum(counts.values())+sum(x['eligibility_cases'] for x in additional),'scores':counts,'ordered_case_digest_sha256':digest.hexdigest(),'role_cases':rolecases,'scope':'Unchanged original retail functions vs independent model; accessor/gender/lineage stubs supplied explicitly. Abstract categories are not proven species/PID pairs. No native specimen or egg rate simulation.'}
(R/'evidence/parent-eligibility-vectors.json').write_text(json.dumps(result,indent=2)+'\n')
print(f'PASS: {result["total_eligibility_comparisons"]} eligibility comparisons across Emerald/Platinum/HGSS and {len(rolecases)} Emerald donor-role cases')
