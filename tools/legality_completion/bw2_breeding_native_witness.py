#!/usr/bin/env python3
"""Replay pinned published B2W2 move/role bodies with isolated data accessors.

The project reports matching C for US/EU B2/W2. This does not rebuild a ROM,
authenticate acquisition, or certify native BW or complete parental ancestry.
"""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[2]
PINS = ROOT / 'docs/legality/campaign-20261028/BW2_BREEDING_SOURCE_PINS.json'


def body(source, signature):
    start = source.index(signature)
    while source.find(';', start) < source.find('{', start):
        start = source.index(signature, start + 1)
    at = source.index('{', start)
    depth, end = 1, at + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def typedef(source, name):
    end = source.index('} ' + name + ';') + len('} ' + name + ';')
    return source[source.rfind('typedef struct {', 0, end):end]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--native', type=pathlib.Path, required=True)
    parser.add_argument('--work', type=pathlib.Path, required=True)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--fetch', action='store_true')
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    pins = json.loads(PINS.read_text())
    for row in pins['files']:
        path = args.native / row['path']
        if args.fetch and not path.exists():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(urllib.request.urlopen(row['url'], timeout=30).read())
        data = path.read_bytes()
        assert hashlib.sha256(data).hexdigest() == row['sha256']
        assert hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest() == row['git_blob_sha1']
    source = (args.native / 'src/ov036/day_care.c').read_text()
    breeding = body(source, 'static void breeding(')
    calls = ['genderSort(', 'setGenderlessFirst(', 'levelMoves(', 'inheritEggMoves(',
             'inheritLvlUpMoves(', 'inheritTmMoves(', 'pichuLightBallHandler(']
    assert [breeding.index(call) for call in calls] == sorted(breeding.index(call) for call in calls)
    # The complete source's only parent-pointer mutations are in this swap;
    # named role sorting happens before the move phases in the original caller.
    assert len(re.findall(r'parents->parents\[[^\]]+\]\s*=', source)) == 2
    prefix = r'''#include "Legality/BreedingBirthMoves.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
namespace Native {
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using BOOL=bool;using HeapID=u32;
constexpr BOOL TRUE=true,FALSE=false;
constexpr int PKM_PARAM_MOVE1=0,PKM_PARAM_MOVE2=1,PKM_PARAM_MOVE3=2,PKM_PARAM_MOVE4=3;
constexpr int PKM_PARAM_SEX=-1,PKM_PARAM_SPECIES=-2,PKM_PARAM_ITEM=-3;
constexpr int GENDER_MALE=0,GENDER_FEMALE=1,GENDER_UNKNOWN=2;
constexpr int SPECIES_DITTO=132,SPECIES_PICHU=172,SPECIES_GENESECT=649;
constexpr int ITEM_TM01=0,ITEM_LAST=95,ITEM_LIGHT_BALL=236,MOVE_VOLT_TACKLE=344;
constexpr int PERSONAL_MACHINES_1=0,PERSONAL_MACHINES_2=1,PERSONAL_MACHINES_3=2,PERSONAL_MACHINES_4=3;
constexpr int LEARNSET_MAX=64;
struct PartyPkm {u32 species=1,sex=0,item=0;std::array<u32,4> moves{};};
struct LearnsetEntry {u32 level=0,move=0;};
struct PersonalData {std::array<u32,4> flags{};};
static Legality::Breeding::BirthMovesData current;
static size_t initialCount=0;
'''
    structures = '\n'.join(typedef(source, name) for name in ('DayCareMoveParamTable', 'EggParents', 'EggData'))
    tables = '\n'.join(re.search(r'static const DayCareMoveParamTable ' + name + r'\s*=.*?;', source, re.S)[0]
                       for name in ('sEggMoveParams', 'sLevelMoveParams', 'sTmMoveParams'))
    stubs = r'''
static u32 PokeParty_GetParam(PartyPkm* p,int field,void*) {
 if(field==PKM_PARAM_SEX)return p->sex;
 if(field==PKM_PARAM_SPECIES)return p->species;
 if(field==PKM_PARAM_ITEM)return p->item;
 assert(field>=0&&field<4);return p->moves[field];
}
static void PML_LearnsetLvUpLoad(u32,u32,LearnsetEntry* result) {
 for(size_t i=0;i<current.levels.size();++i)result[i]={i<initialCount?1u:2u,current.levels[i]};
 result[current.levels.size()]={0,65535};
}
static BOOL LearnsetEntry_IsEnd(LearnsetEntry e){return e.move==65535;}
static u32 LearnsetEntry_GetLevel(LearnsetEntry e){return e.level;}
static u32 LearnsetEntry_GetMove(LearnsetEntry e){return e.move;}
static u16* GFL_ArcSysReadHeapNew(int archive,u32,HeapID){assert(archive==124);auto* p=new u16[current.eggs.size()+1];p[0]=current.eggs.size();std::copy(current.eggs.begin(),current.eggs.end(),p+1);return p;}
static void GFL_HeapFree(u16* p){delete[] p;}
static BOOL PML_ItemIsTMHM(u16 item){return item<current.machines.size();}
static u32 PML_ItemGetTMBitMask(u16 item){return item;}
static u32 PML_ItemGetTMMoveID(int item){return size_t(item)<current.machines.size()?current.machines[item]:0;}
static PersonalData* PML_PersonalLoad(u16,u16,HeapID){auto* p=new PersonalData;for(size_t i=0;i<current.machines.size();++i)p->flags[i/32]|=u32{1}<<(i%32);return p;}
static void PML_PersonalFree(PersonalData* p){delete p;}
static u32 PML_PersonalGetParam(PersonalData* p,int field){return p->flags[field];}
'''
    functions = '\n'.join(body(source, signature) for signature in (
        'static void swapDaycarePKMPointers(', 'static void genderSort(', 'static void setGenderlessFirst(',
        'static void PushEggPkmMove(', 'static BOOL canPkmLearnTMItemNumber(', 'static void levelMoves(',
        'static void inheritEggMoves(', 'static void inheritLvlUpMoves(', 'static void inheritTmMoves(',
        'static void pichuLightBallHandler('))
    harness = r'''
}
int main(){
 using namespace Legality::Breeding;
 uint32_t random=0x9135726u;
 const auto next=[&](){random^=random<<13;random^=random>>17;random^=random<<5;return random;};
 for(size_t trial=0;trial<20001;++trial){
  BirthMovesData data;std::vector<uint16_t> pool;
  for(uint16_t i=1;i<=12;++i)pool.push_back(i);
  const auto shuffle=[&](){for(size_t i=pool.size();i>1;--i)std::swap(pool[i-1],pool[next()%i]);};
  shuffle();size_t initial=1+next()%4;data.initial.assign(pool.begin(),pool.begin()+initial);
  data.levels=pool; // Actual child learnset order; not paternal slot order.
  shuffle();std::vector<uint16_t> dad(pool.begin(),pool.begin()+1+next()%4);
  shuffle();std::vector<uint16_t> mom(pool.begin(),pool.begin()+1+next()%4);
  for(uint16_t i=1;i<=12;++i){if(next()&1)data.eggs.push_back(i);if(next()&1)data.machines.push_back(i);}
  const unsigned kind=trial%4;Native::PartyPkm first,second;
  first.species=1;first.sex=kind==2?Native::GENDER_FEMALE:kind==3?Native::GENDER_UNKNOWN:Native::GENDER_MALE;
  second.species=kind?132:2;second.sex=kind?Native::GENDER_UNKNOWN:Native::GENDER_FEMALE;
  if(kind)mom={144};
  if(trial==0){data={{1,2},{1,2,30,20},{},{},false};initial=2;dad={20,30};mom={20,30};}
  if(trial==20000){data=PIDGEY_DATA;initial=1;dad={17,97};mom={97,17};first.species=second.species=16;}
  const bool pichu=trial%13==0,lightBall=pichu&&trial%26==0;
  data.voltTackle=pichu;
  if(lightBall)(trial%52==0?first:second).item=Native::ITEM_LIGHT_BALL;
  std::copy(dad.begin(),dad.end(),first.moves.begin());std::copy(mom.begin(),mom.end(),second.moves.begin());
  Native::current=data;Native::initialCount=initial;
  Native::EggParents parents{{&first,&second}};if(next()&1)std::swap(parents.parents[0],parents.parents[1]);
  Native::genderSort(&parents);Native::setGenderlessFirst(&parents);
  assert(parents.parents[0]==(kind==2?&second:&first));
  Native::EggData egg{};egg.level=1;egg.species=trial==20000?16:pichu?Native::SPECIES_PICHU:1;
  Native::levelMoves(&egg);Native::inheritEggMoves(&parents,&egg,0);
  Native::inheritLvlUpMoves(&parents,&egg);Native::inheritTmMoves(&parents,&egg,0);
  Native::pichuLightBallHandler(&parents,&egg);
  const auto model=inheritBirthMoves(data,5,kind==2?mom:dad,kind==2?dad:mom,lightBall);
  std::array<uint16_t,4> expected{},actual{};std::copy(model.begin(),model.end(),expected.begin());
  for(size_t i=0;i<4;++i)actual[i]=egg.moves[i];
  if(trial==20000)assert((actual==std::array<uint16_t,4>{33,97,17,0}));
  if(actual!=expected){std::cerr<<"Native BW2 move/order mismatch trial "<<trial<<" native=";for(auto m:actual)std::cerr<<m<<',';std::cerr<<" model=";for(auto m:expected)std::cerr<<m<<',';std::cerr<<'\n';return 1;}
 }
 std::cout<<"Published BW2 bodies:19999 random cases,child-learnset shared order,role permutations,Light Ball,explicit order control and cross-checked Pidgey birth PASS\n";
}
'''
    from bw2_shared_order import index
    pidgey = index()['16']
    vector = lambda values: '{' + ','.join(map(str, values)) + '}'
    data = '{' + ','.join(vector(pidgey[key]) for key in ('initial','levels','egg','machines')) + ',false}'
    harness = harness.replace('PIDGEY_DATA', data)
    generated = prefix + structures + '\n' + tables + stubs + functions + harness
    cpp = args.work / 'native_bw2_birth.cpp'
    cpp.write_text(generated)
    flags = ['-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
             '-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-sign-compare']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    exe = args.work / 'native_bw2_birth'
    subprocess.run([args.compiler, *flags, '-I' + str(ROOT / 'include'), str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe.resolve())], check=True)
    print('Published source', pins['commit'], 'compiled body witness SHA256', hashlib.sha256(generated.encode()).hexdigest())


if __name__ == '__main__':
    main()
