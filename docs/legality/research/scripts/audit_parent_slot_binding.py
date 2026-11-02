#!/usr/bin/env python3
"""Check named-parent integration against retail daycare-slot IV vectors, research only."""
import io,json,pathlib,subprocess,sys,tarfile,tempfile
R=pathlib.Path(__file__).resolve().parents[1];NX=R.parents[2]
REV='fe3e3d054ef7d42c31df3e772e8370e2c7587c8a'
# Regenerate the 24 independent original-C/Python source-stage comparisons first.
subprocess.run([sys.executable,str(R/'scripts/reproduce_inheritance.py'),sys.argv[1]],check=True)
vectors=json.loads((R/'evidence/inheritance-vectors.json').read_text())
games={'pokeemerald':'emerald_gba','pokeruby':'ruby_gba','pokefirered':'firered_gba'}
program=r'''
#include "Legality/Gen3JointEggCandidateEvidence.h"
#include <iostream>
namespace H=Legality::Gen3JointEggCandidateEvidence;
namespace G=Legality::Gen3EggIVInheritanceStage;
int main(){
 unsigned gameIndex,seed,end,swap; std::array<uint8_t,6> observed{};
 const char* games[]={"emerald_gba","ruby_gba","firered_gba"};
 while(std::cin>>gameIndex>>seed>>end>>swap){
  for(auto& v:observed){unsigned n;std::cin>>n;v=n;}
  H::Input in{};auto& m=in.moveHistory;
  m.game=games[gameIndex];m.species=1;m.fatherId=101;m.motherId=202;
  m.snapshotsKnown=true;m.fatherSpecies=1;m.motherSpecies=1;
  m.pairProof=Legality::Gen34SameParentEggHistory::PairProof::IndependentlyCompatible;
  m.base={33,45,0,0};m.father=m.base;m.mother=m.base;m.observed=m.base;
  m.lightBallKnown=true;
  std::array<uint8_t,6> slot0{},slot1{};slot0.fill(30);slot1.fill(31);
  in.father={101,true,swap?slot1:slot0};in.mother={202,true,swap?slot0:slot1};
  in.sharedParentStageIndependentlyVerified=true;
  in.moveParentStageToken=in.ivParentStageToken=1234;
  in.independentlyKnownFinalIVStageState=true;in.finalIVStageRNGState=end;
  in.observedUnhatchedIVs=observed;
  auto direct=G::replay(m.game,seed,in.initialChildIVs,slot0,slot1,true);
  auto joint=H::evaluate(in);
  std::cout<<(direct.matches(observed))<<' '
   <<(joint.status==H::Status::KnownCandidateConsistent)<<' '
   <<(joint.status==H::Status::IVCandidateContradicted)<<'\n';
 }
}
'''
cases=[]; inputlines=[]
for v in vectors:
 for swap in (0,1):
  inputlines.append(' '.join(map(str,[list(games).index(v['game_source']),int(v['seed_before_inherit'],16),int(v['final_seed'],16),swap]+v['final_ivs_hp_atk_def_spe_spa_spd'])))
  cases.append({'game':games[v['game_source']],'seed':v['seed_before_inherit'],'final_seed':v['final_seed'],'father_daycare_slot':swap,'mother_daycare_slot':1-swap,'slot0_ivs':[30]*6,'slot1_ivs':[31]*6,'observed_ivs':v['final_ivs_hp_atk_def_spe_spa_spd'],'scope':'conditional synthetic IV stage with caller-attested parent move hypothesis; not native specimen'})
with tempfile.TemporaryDirectory(prefix='nx-parent-order-') as tmp:
 tmp=pathlib.Path(tmp)
 data=subprocess.check_output(['git','-C',str(NX),'archive',REV,'include'])
 with tarfile.open(fileobj=io.BytesIO(data)) as archive:archive.extractall(tmp,filter='data')
 cpp=tmp/'probe.cpp';cpp.write_text(program);exe=tmp/'probe'
 subprocess.run(['c++','-std=c++17','-O2','-I'+str(tmp/'include'),str(cpp),'-o',str(exe)],check=True)
 lines=subprocess.check_output([str(exe)],input='\n'.join(inputlines)+'\n',text=True).splitlines()
assert len(lines)==len(cases)
for case,line in zip(cases,lines):
 direct,consistent,contradicted=map(int,line.split());assert direct
 case.update(stage_matches_retail=bool(direct),joint_consistent=bool(consistent),joint_iv_contradicted=bool(contradicted))
 assert consistent if case['father_daycare_slot']==0 else contradicted
result={'engine_revision':REV,'cases':cases,'stage_agreements':len(cases),'correct_joint_matches':sum(c['joint_consistent'] for c in cases),'incorrect_candidate_rejections':sum(c['joint_iv_contradicted'] for c in cases),'finding':'Joint helper always binds father to retail daycare slot 0; input contains no actual daycare-slot mapping. Candidate-local discrepancy, no global Invalid claim.'}
(R/'evidence/parent-slot-binding-audit.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS stage comparisons:',len(cases),'; confirmed incorrect joint candidate rejections:',result['incorrect_candidate_rejections'])
