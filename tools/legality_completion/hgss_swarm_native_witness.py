#!/usr/bin/env python3
"""Compile unchanged HGSS swarm predicate/slot-replacement bodies, not an oracle patch.

The harness supplies isolated save/map state. This establishes gate and slot
semantics only, not a complete battle, native capture receipt or hardware run.
"""
import argparse, hashlib, pathlib, subprocess
from hgss_swarm_repair import checked_native


def body(source, name, return_type='void'):
    signature='static '+return_type+' '+name+'('
    start=source.index(signature)
    # Skip the declaration, taking the actual definition at the start of a line.
    while source.find(';',start)<source.find('{',start):start=source.index(signature,start+1)
    brace=source.index('{',start);depth=1;end=brace+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True);p.add_argument('--fetch',action='store_true')
    p.add_argument('--work',type=pathlib.Path,required=True);p.add_argument('--compiler',default='g++');p.add_argument('--sanitize',action='store_true');a=p.parse_args()
    pins=checked_native(a.native,a.fetch);a.work.mkdir(parents=True,exist_ok=True)
    original=(a.native/'src/unk_02097F6C.c').read_text();gate=original[original.index('#define SWARM_MAP_COUNT'):original.index('void GetSwarmInfoFromRand')]
    encounter=(a.native/'src/field/encounter_check.c').read_text()
    fishing=body(encounter,'EncSlots_Update_FishingSwarm')
    prefix=r'''#include <cassert>
#include <cstdint>
#include <iostream>
#include "constants/maps.h"
#include "constants/species.h"
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using BOOL=int;
#define TRUE 1
#define FALSE 0
#define NELEMS(x) (sizeof(x)/sizeof((x)[0]))
#define GF_ASSERT(x) assert(x)
#define ROD_TYPE_OLD 0
#define ROD_TYPE_GOOD 1
#define ROD_TYPE_SUPER 2
'''
    stubs=r'''
struct RoamerSaveData {};
struct Location {u16 mapId;};
struct FieldSystem {void* saveData;Location* location;};
struct EncounterData {u16 fishSwarm;};
struct EncounterSlot {u16 species;};
static RoamerSaveData save;
static u32 outbreakRandom;
static BOOL active=TRUE;
static RoamerSaveData* Save_Roamers_Get(void*) {return &save;}
static BOOL RoamerSave_OutbreakActive(RoamerSaveData*) {return active;}
static u32 Roamers_GetRand(RoamerSaveData*,int) {return outbreakRandom;}
'''
    harness=r'''
int main() {
 uint64_t cases=0;
 for(u32 random=0;random<20;++random)for(u32 map=0;map<65536;++map)for(u16 type=0;type<3;++type) {
  const bool expected=map==sSwarmMapLUT[random][0]&&type==sSwarmMapLUT[random][1];
  assert(bool(sub_02097F6C(random,u16(map),type))==expected);++cases;
 }
 struct NativeSwarm {u16 map;u32 random;u16 species;};
 const NativeSwarm native[]={{MAP_ROUTE_12,3,SPECIES_RELICANTH},{MAP_ROUTE_32,6,SPECIES_QWILFISH},{MAP_ROUTE_44,12,SPECIES_REMORAID},{MAP_VIOLET,19,SPECIES_BARBOACH}};
 for(const auto& target:native)for(u8 rod=0;rod<3;++rod) {
  Location loc{target.map};FieldSystem field{nullptr,&loc};EncounterData data{target.species};EncounterSlot slots[5]{};
  for(u16 i=0;i<5;++i)slots[i].species=u16(10+i);
  outbreakRandom=target.random;active=TRUE;EncSlots_Update_FishingSwarm(&field,rod,&data,slots);
  for(u16 i=0;i<5;++i) {
   const bool changed=rod==0?i==2:rod==1?(i==0||i==2||i==3):true;
   assert(slots[i].species==(changed?target.species:u16(10+i)));
  }
 }
 for(u16 map:{u16(MAP_NEW_BARK),u16(MAP_ROUTE_34)})for(u32 random=0;random<20;++random)for(u8 rod=0;rod<3;++rod) {
  Location loc{map};FieldSystem field{nullptr,&loc};EncounterData data{SPECIES_MAGIKARP};EncounterSlot slots[5]{};
  for(u16 i=0;i<5;++i)slots[i].species=u16(10+i);
  outbreakRandom=random;active=TRUE;EncSlots_Update_FishingSwarm(&field,rod,&data,slots);
  for(u16 i=0;i<5;++i)assert(slots[i].species==u16(10+i));
 }
 std::cout<<"Original HGSS swarm bodies: "<<cases<<" exhaustive map/type gates; four native fishing outbreaks; allfive Super Rod slots; New Bark/Route34 phantom defaults excluded PASS\n";
}
'''
    text=prefix+gate+stubs+fishing+harness;source=a.work/'hgss_swarm_native.cpp';source.write_text(text);binary=a.work/'hgss_swarm_native'
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-O1','-g']
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run([a.compiler,*flags,'-I'+str((a.native/'include').resolve()),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary.resolve())],check=True)
    print('Native source',pins['commit'],'compiled-body SHA256',hashlib.sha256(text.encode()).hexdigest())
    # These unchanged dispatch bodies show that Safari battles never consume
    # the ordinary bank supplied by the caller. The callees are isolated probes;
    # this is a dispatch witness, not a whole capture or RNG simulation.
    dispatch='\n'.join(body(encounter,n,'BOOL') for n in (
        'FieldSystem_GenerateLandSafariEncounter',
        'FieldSystem_GenerateSurfingEncounter',
        'FieldSystem_GenerateFishingEncounter'))
    dispatch_prefix=r'''#include <cassert>
#include <iostream>
using BOOL=int;using u8=unsigned char;
struct FieldSystem {};struct Pokemon {};struct BattleSetup {};struct EncounterData {};
struct EncounterSlot {};struct EncounterGenState {};
constexpr int ROD_TYPE_NONE=-1,ENCOUNTER_TYPE_LAND=0,ENCOUNTER_TYPE_SURFING=1,ENCOUNTER_TYPE_FISHING=2,BATTLER_ENEMY=1;
static int selected=-1,selectedType=-1,selectedRod=-2;
static BOOL FieldSystem_GenerateSafariEncounter(FieldSystem*,Pokemon*,int rod,EncounterGenState*,u8 type,int,BattleSetup*) {selected=1;selectedType=type;selectedRod=rod;return 1;}
static BOOL FieldSystem_GenerateRegularEncounter(Pokemon*,int rod,EncounterGenState*,EncounterSlot*,u8 type,int,BattleSetup*) {selected=0;selectedType=type;selectedRod=rod;return 1;}
'''
    dispatch_test=r'''
int main() {
 assert(FieldSystem_GenerateLandSafariEncounter(nullptr,nullptr,nullptr,nullptr,nullptr,nullptr));
 assert(selected==1&&selectedType==ENCOUNTER_TYPE_LAND&&selectedRod==ROD_TYPE_NONE);
 for(BOOL safari:{0,1}) {
  assert(FieldSystem_GenerateSurfingEncounter(nullptr,nullptr,nullptr,nullptr,nullptr,safari));
  assert(selected==safari&&selectedType==ENCOUNTER_TYPE_SURFING&&selectedRod==ROD_TYPE_NONE);
  for(int rod=0;rod<3;++rod) {
   assert(FieldSystem_GenerateFishingEncounter(nullptr,nullptr,nullptr,nullptr,nullptr,rod,safari));
   assert(selected==safari&&selectedType==ENCOUNTER_TYPE_FISHING&&selectedRod==rod);
  }
 }
 std::cout<<"Original HGSS Safari dispatch: land, surf, all rods select Safari handler; ordinary controls PASS\n";
}
'''
    text=dispatch_prefix+dispatch+dispatch_test;source=a.work/'hgss_safari_dispatch.cpp';source.write_text(text);binary=a.work/'hgss_safari_dispatch'
    subprocess.run([a.compiler,*flags,str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary.resolve())],check=True)
    print('Native Safari dispatch compiled-body SHA256',hashlib.sha256(text.encode()).hexdigest())

if __name__=='__main__':main()
