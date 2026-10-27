#!/usr/bin/env python3
"""Source-indexed audit inventory. Source occurrence is NEVER counted as rule completion.
Semantic applicability/evidence must be adjudicated before a rule-coverage denominator
can be certified. All 20 title rows remain in the inventory, including unknown cases.
"""
import argparse,hashlib,json,pathlib,re,subprocess
PINS={'pinned':'6501f0ab46e8f8ca048539dbaf8cae8cb104e722','current':'6042efa241a9925ce57e645329ea7f976deabdd3'}
GAMES={1:['red','blue','yellow'],2:['gold','silver','crystal'],3:['ruby','sapphire','emerald','firered','leafgreen'],4:['diamond','pearl','platinum','heartgold','soulsilver'],5:['black','white','black2','white2']}
CATEGORIES={
'encounter-candidates':('Encounter','encounter'), 'wild-slots':('Encounter','encounter'), 'fishing-safari-headbutt':('Encounter','encounter'),
'static-gifts':('Encounter','encounter'),'ingame-trades':('Encounter','encounter'),'historical-events':('Event','eventGift'),
'mystery-gift-payloads':('Mystery','eventGift'),'breeding-eggs':('Breeding','eggBreeding'),'evolution':('Evolution','moves'),
'levelup-chronology':('Learn','moves'),'machine-tutor-moves':('Learn','moves'),'joint-moves':('Move','moves'),
'pid-iv-rng':('PID','pidRng'),'ivs-dvs':('IndividualValue','internal'),'gender':('Gender','internal'),
'shiny':('PID','internal'),'abilities':('Ability','internal'),'ev-stat-exp':('Effort','internal'),'level-exp-daycare':('Level','internal'),
'ot-trainer-id':('Trainer','internal'),'nicknames-encoding':('Nickname','internal'),'korean-japanese-regions':('Language','internal'),
'forms':('Form','internal'),'items-catchrate':('Item','internal'),'balls':('Ball','internal'),'pp-slots':('MovePP','moves'),
'contest-ribbons':('Ribbon','internal'),'met-locations-dates':('Misc','internal'),'pokerus-fateful-flags':('Misc','internal'),
'event-ot-nickname-language':('Event','eventGift'),'transfer-alternative-history':('Transfer','transfer'),
'gamecube-ancestry':('CXD','transfer'),'stadium-ranch-wii':('Encounter','encounter'),'source-save-context':('SlotType','structure')}
def sha(root):return subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()
def collect(root,pin):
 if sha(root)!=pin:raise ValueError('source pin mismatch')
 rows=[];files={}
 for p in sorted((root/'PKHeX.Core/Legality').rglob('*.cs')):
  rel=p.relative_to(root).as_posix();data=p.read_text(encoding='utf-8-sig');h=hashlib.sha256(p.read_bytes()).hexdigest();files[rel]=h
  method=None
  for i,line in enumerate(data.splitlines(),1):
   # Explicit source sites, not a claim one line == one independent semantic rule.
   m=re.search(r'(?:public|private|protected|internal)\s+(?:static\s+)?(?:override\s+)?[\w<>?\[\]]+\s+(\w+)\s*\(',line)
   if m:method=m[1]
   if re.search(r'\b(GetInvalid|GetValid|GetFishy|GetResult|CheckResult|MatchRating|Severity\.(?:Invalid|Fishy))\b',line) and not line.lstrip().startswith(('//','///','using')):
    key=rel+':'+str(i)
    rows.append({'id':key,'path':rel,'symbol_context':method,'line':i,'line_sha256':hashlib.sha256(line.encode()).hexdigest(),'source_sha256':h,
      'url':f'https://github.com/kwsch/PKHeX/blob/{pin}/{rel}#L{i}',
      'generation_applicability':'Needs semantic review','applicable_games':None,'nx_implementation':None,'production_connected':None,
      'completeness':'Missing audit','original_game_source':None,'positive_tests':None,'negative_tests':None,'differential_results':None,'remaining_disagreement':'Not yet adjudicated; never counted as complete'})
 return rows,files

def generate(pinned,current,repo):
 snapshots={}
 for label,root in [('pinned',pinned),('current',current)]:
  assert sha(root)==PINS[label], f'{label}: checkout does not match declared reference pin'
  rows,files=collect(root,PINS[label]);snapshots[label]={'commit':PINS[label],'source_sites':rows,'files':files}
 a=snapshots['pinned']['files'];b=snapshots['current']['files'];drift={'added':sorted(b.keys()-a.keys()),'removed':sorted(a.keys()-b.keys()),'changed':sorted(k for k in a.keys()&b.keys() if a[k]!=b[k])}
 source_groups={category:[r['id'] for r in snapshots['pinned']['source_sites'] if search.casefold() in r['path'].casefold()] for category,(search,area) in CATEGORIES.items()}
 coverage=[]
 for gen,games in GAMES.items():
  for game in games:
   for category,(search,area) in CATEGORIES.items():
    path='src/Legality/'+('GBLegality.cpp' if gen<3 else f'Gen{gen}Legality.cpp')
    coverage.append({'generation':gen,'game':game,'category':category,'pkhex_revision':PINS['pinned'],'pkhex_source_group':category,'pkhex_latest_revision':PINS['current'],
     'original_game_source':None,'nx_file':path,'nx_function':'checkGen'+str(gen) if gen<3 else 'Analyzer','report_area':area,
     'production_connected':None,'native_analyzer_dispatch_exists':gen<5,'production_evidence':'Legality.cpp native branch; per-rule trace still required' if gen<5 else 'Engine/Gen5App adapter only; current app does not call it',
     'completeness':'Partial','positive_tests':None,'negative_tests':None,'differential_results':None,'remaining_disagreement':'Per-rule applicability, test evidence and historical-source audit incomplete'})
 return {'schema':1,'engine_base':sha(repo),'definition':'Source-site inventory plus semantic area/title crosswalk; neither is a certified applicable-rule denominator.',
 'percentage_policy':'Unmeasured until applicability and executed per-rule evidence are audited. No source occurrence is Complete. Unknown/Fishy/Incomplete are retained.',
 'games':GAMES,'regional_variants':['Japanese Red/Green/Blue/Yellow','Japanese G/S/C','Korean Gold/Silver','international language variants'],
 'pkhex_source_groups':source_groups,'snapshots':snapshots,'upstream_drift':drift,'coverage':coverage,'metrics':{str(g):{'rule_coverage':None,'production_wired_rule_coverage':None,'invalid_detection':None,'valid_certification':None,'exact_verdict_agreement':None,'historical_provenance':None,'original_source_evidence':None,'unresolved_rule_count':None} for g in GAMES}}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('pinned',type=pathlib.Path);p.add_argument('current',type=pathlib.Path);p.add_argument('output',type=pathlib.Path)
 p.add_argument('--current-commit',default=PINS['current']);a=p.parse_args();PINS['current']=a.current_commit
 a.output.write_text(json.dumps(generate(a.pinned,a.current,pathlib.Path(__file__).resolve().parents[2]),indent=2)+'\n')
