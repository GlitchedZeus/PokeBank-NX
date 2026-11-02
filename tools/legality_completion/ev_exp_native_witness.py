#!/usr/bin/env python3
"""Run the unchanged native HGSS EV award body and EXP split in a small harness.
Source hashes are checked before extraction. Stubs supply a legal party/item/
encounter configuration, not a replacement EV algorithm. This is a source-code
reproduction, not hardware evidence or an authenticated cartridge specimen.
"""
import argparse,csv,hashlib,json,os,pathlib,re,subprocess,urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--source',type=pathlib.Path,required=True);p.add_argument('--work',type=pathlib.Path,required=True);p.add_argument('--compiler',default='g++');p.add_argument('--sanitize',action='store_true');p.add_argument('--fetch',action='store_true');a=p.parse_args()
manifest=json.loads((ROOT/'docs/legality/hardening-20261027/NATIVE_EV_SOURCE_PINS.json').read_text())
for row in manifest['files']:
 f=a.source/row['path']
 if a.fetch and not f.exists():f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(urllib.request.urlopen(row['url']).read())
 data=f.read_bytes();assert hashlib.sha256(data).hexdigest()==row['sha256'],row['path']
 assert hashlib.sha1(f'blob {len(data)}\0'.encode()+data).hexdigest()==row['git_blob'],row['path']
text=(a.source/'src/battle/battle_command.c').read_text()
start=text.index('static void BattleScript_CalcEffortValues(Party *party, int partySlot, u32 species, u32 form) {')
end=text.index('\nenum {',start);body=text[start:end]
start=text.index('        totalExp = GetMonBaseStat(')
end=text.index('    } else {\n        BattleScriptIncrementPointer(ctx, adrs);',start)
exp=text[start:end]
items=(a.source/'include/constants/items.h').read_text()
constants='\n'.join(l for l in items.splitlines() if re.match(r'#define (?:HOLD_EFFECT_(?:LVLUP_\w+_EV_UP|EVS_UP_SPEED_DOWN)|ITEM_POWER_BRACER)\b',l))
with (a.source/'files/itemtool/itemdata/item_data.csv').open() as f:bracer=next(r for r in csv.DictReader(f) if r['item']=='ITEM_POWER_BRACER')
assert bracer['holdEffect']=='HOLD_EFFECT_LVLUP_ATK_EV_UP' and bracer['holdEffectParam']=='4'
enc=json.loads((a.source/'files/fielddata/encountdata/gs_enc_data.json').read_text())
def has_hoothoot(node):
 if isinstance(node,dict):
  if node.get('level')==2 and 'SPECIES_HOOTHOOT' in str(node.get('species')):return True
  return any(has_hoothoot(v) for v in node.values())
 if isinstance(node,list):return any(has_hoothoot(v) for v in node)
 return False
assert has_hoothoot(enc),'native level2 Hoothoot source missing'
# Hoothoot HGSS base EXP58 and HP yield1, independently retained in the pinned
# personal resource and native encounter source; no other stat yield.
scaffold=r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;
constexpr int MAX_EV_SUM=510,MAX_EV_PER_STAT=255,HEAP_ID_BATTLE=0;
enum {STAT_HP,STAT_ATK,STAT_DEF,STAT_SPEED,STAT_SPATK,STAT_SPDEF,STAT_ACC};
enum {BASE_HP_YIELD,BASE_ATK_YIELD,BASE_DEF_YIELD,BASE_SPEED_YIELD,BASE_SPATK_YIELD,BASE_SPDEF_YIELD,BASE_EXP_YIELD};
enum {MON_DATA_HP_EV=0,MON_DATA_HELD_ITEM=20,ITEMATTR_HOLD_EFFECT=0,ITEMATTR_HOLD_EFFECT_PARAM=1};
struct Pokemon {std::array<u8,6> evs{100,100,0,0,0,0};};struct Party {Pokemon mon;};
struct BaseStats {std::array<int,6> yield{1,0,0,0,0,0};};
Pokemon* Party_GetMonByIndex(Party* p,int i){assert(i==0);return &p->mon;}
BaseStats* AllocAndLoadMonPersonal_HandleAlternateForm(u32 species,u32 form,int){assert(species==163&&form==0);return new BaseStats;}
int GetMonData(Pokemon* p,int field,void*){return field==MON_DATA_HELD_ITEM?ITEM_POWER_BRACER:p->evs.at(field);}
int GetItemAttr(u16 item,int field,int){assert(item==ITEM_POWER_BRACER);return field==ITEMATTR_HOLD_EFFECT?HOLD_EFFECT_LVLUP_ATK_EV_UP:4;}
int GetPersonalAttr(BaseStats* p,int field){return p->yield.at(field);}
int MaskOfFlagNo(int slot){return 1<<slot;}
bool Party_MaskMonsWithPokerus(Party*,int){return false;}
void SetMonData(Pokemon* p,int field,const u8* value){p->evs.at(field)=*value;}
void FreeMonPersonal(BaseStats* p){delete p;}
int GetMonBaseStat(u32 species,int field){assert(species==163&&field==BASE_EXP_YIELD);return 58;}
struct Battler {u32 species=163;u16 level=2;};struct Context {Battler battleMons[1];int battlerIdFainted=0;u16 gainedExp=0,partyGainedExp=0;};
'''
main='''
int main(){Context context;auto*ctx=&context;int expMonsCnt=6,expShareMonsCnt=5;u16 totalExp=0;
'''+exp+'''
assert(ctx->gainedExp==1);Party party;BattleScript_CalcEffortValues(&party,0,163,0);
assert((party.mon.evs==std::array<u8,6>{101,104,0,0,0,0}));
std::cout<<"Native HGSS: six participants/five EXP Shares, Power Bracer; one battle grants HP+1 and ATK+4 for1 EXP PASS\\n";
}
'''
a.work.mkdir(parents=True,exist_ok=True);src=a.work/'native-ev-witness.cpp';src.write_text(constants+'\n'+scaffold+'\n'+body+main)
flags=['-std=c++20','-O1','-g','-Wall','-Wextra','-Werror']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie']
exe=a.work/'native-ev-witness';subprocess.run([a.compiler,*flags,str(src),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
