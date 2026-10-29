#!/usr/bin/env python3
"""Compare the forward birth builder with unchanged original HGSS C bodies.

Only move-slot insertion/inheritance is exercised; species/gender/PID/IV/event
and acquisition receipt checks are separate. No complete Gen V native replay claim.
"""
import argparse,hashlib,json,pathlib,subprocess,urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[2]
PINS=ROOT/'docs/legality/campaign-20261028/HGSS_BREEDING_SOURCE_PINS.json'

def body(source,signature):
    start=source.index(signature)
    while source.find(';',start)<source.find('{',start):start=source.index(signature,start+1)
    at=source.index('{',start);depth=1;end=at+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True);p.add_argument('--fetch',action='store_true');p.add_argument('--work',type=pathlib.Path,required=True);p.add_argument('--compiler',default='g++');p.add_argument('--sanitize',action='store_true');a=p.parse_args();a.work.mkdir(parents=True,exist_ok=True)
    pins=json.loads(PINS.read_text())
    for r in pins['files']:
        f=a.native/r['path']
        if a.fetch and not f.exists():f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(urllib.request.urlopen(r['url'],timeout=30).read())
        b=f.read_bytes();assert hashlib.sha256(b).hexdigest()==r['sha256'];assert hashlib.sha1(b'blob '+str(len(b)).encode()+b'\0'+b).hexdigest()==r['git_blob_sha1']
    source=(a.native/'src/get_egg.c').read_text();inherit=body(source,'static void InheritMoves(')
    struct=source[source.index('struct EggMoveSearch {'):source.index('struct EggMoveSearch {')+source[source.index('struct EggMoveSearch {'):].index('};')+2]
    append=body((a.native/'src/pokemon.c').read_text(),'u32 TryAppendBoxMonMove(')
    prefix=r'''#include "Legality/BreedingBirthMoves.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
namespace Native {
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using BOOL=bool;
constexpr int MAX_MON_MOVES=4,MAX_EGG_MOVES=16,HEAP_ID_FIELD1=1,NUM_TMHMS=100,ITEM_TM01=0;
constexpr int MON_DATA_SPECIES=100,MON_DATA_FORM=101,MON_DATA_MOVE1=0;
constexpr u32 MOVE_APPEND_FULL=65535,MOVE_APPEND_KNOWN=65534;
constexpr u16 MOVE_NONE=0;
struct BoxPokemon {std::array<u16,4> moves{};};
struct Pokemon {BoxPokemon box;};
static Legality::Breeding::BirthMovesData current;
static BOOL AcquireBoxMonLock(BoxPokemon*){return true;}
static void ReleaseBoxMonLock(BoxPokemon*,BOOL){}
static u16 GetBoxMonData(BoxPokemon* p,int field,void*){return p->moves[field];}
static u16 GetMonData(Pokemon*,int field,void*){return field==MON_DATA_SPECIES?1:0;}
static void BoxMonSetMoveInSlot(BoxPokemon* p,u16 move,u8 slot){p->moves[slot]=move;}
'''
    stubs=r'''
static u32 TryAppendMonMove(Pokemon* p,u16 move){return TryAppendBoxMonMove(&p->box,move);}
static void DeleteMonFirstMoveAndAppend(Pokemon* p,u16 move){for(int i=0;i<3;++i)p->box.moves[i]=p->box.moves[i+1];p->box.moves[3]=move;}
struct Allocation {u8* p;template<class T>operator T*()const{return reinterpret_cast<T*>(p);}};
static Allocation Heap_Alloc(int,size_t n){return {new u8[n]};}
static void Heap_Free(void* p){delete[] static_cast<u8*>(p);}
static void MI_CpuClearFast(void* p,size_t n){std::memset(p,0,n);}
static u16 Species_LoadLearnsetTable(u16,u16,u16* p){std::copy(current.levels.begin(),current.levels.end(),p);return current.levels.size();}
static u16 LoadEggMoves(Pokemon*,u16* p){std::copy(current.eggs.begin(),current.eggs.end(),p);return current.eggs.size();}
static u16 TMHMGetMove(int i){return size_t(i)<current.machines.size()?current.machines[i]:0;}
static BOOL GetTMHMCompatBySpeciesAndForm(u16,u16,int i){return size_t(i)<current.machines.size();}
'''
    harness=r'''
}
int main(){
 using namespace Legality::Breeding;
 uint32_t random=0x8135726u;
 const auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
 for(size_t trial=0;trial<20000;++trial){
  BirthMovesData data;
  std::vector<uint16_t> pool;for(uint16_t i=1;i<=12;++i)pool.push_back(i);
  const auto shuffle=[&](){for(size_t i=pool.size();i>1;--i)std::swap(pool[i-1],pool[next()%i]);};
  shuffle();data.initial.assign(pool.begin(),pool.begin()+1+next()%4);
  shuffle();std::vector<uint16_t> dad(pool.begin(),pool.begin()+1+next()%4);
  shuffle();std::vector<uint16_t> mom(pool.begin(),pool.begin()+1+next()%4);
  for(uint16_t i=1;i<=12;++i){if(next()&1)data.levels.push_back(i);if(next()&1)data.eggs.push_back(i);if(next()&1)data.machines.push_back(i);}
  Native::current=data;Native::Pokemon egg;Native::BoxPokemon father,mother;
  std::copy(data.initial.begin(),data.initial.end(),egg.box.moves.begin());std::copy(dad.begin(),dad.end(),father.moves.begin());std::copy(mom.begin(),mom.end(),mother.moves.begin());
  Native::InheritMoves(&egg,&father,&mother);
  const auto model=inheritBirthMoves(data,4,dad,mom);std::array<uint16_t,4> expected{};std::copy(model.begin(),model.end(),expected.begin());
  assert(egg.box.moves==expected);
 }
 // Known moves are skipped, not removed and appended again. This matters when
 // four required parental moves leave no fifth slot for an invented filler.
 BirthMovesData data{{111,1,45},{516,1,45},{118},{},false};
 assert((inheritBirthMoves(data,5,{118,516,1,45},{516,1,45})==std::vector<uint16_t>{1,45,118,516}));
 data={{84,204},{},{},{},true};assert((inheritBirthMoves(data,5,{}, {},true)==std::vector<uint16_t>{84,204,344}));
 std::cout<<"Original HGSS birth bodies:20000 independent parent/order/pool comparisons; duplicate-slot and Light Ball controls PASS\n";
}
'''
    text=prefix+append+stubs+struct+'\n'+inherit+harness;cpp=a.work/'native_birth.cpp';cpp.write_text(text);exe=a.work/'native_birth'
    flags=['-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-parameter']
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run([a.compiler,*flags,'-I'+str(ROOT/'include'),str(cpp),'-o',str(exe)],check=True);subprocess.run([str(exe.resolve())],check=True)
    print('Native source',pins['commit'],'compiled witness SHA256',hashlib.sha256(text.encode()).hexdigest())
if __name__=='__main__':main()
