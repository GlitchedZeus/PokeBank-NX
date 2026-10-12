#!/usr/bin/env python3
"""Read-only source inventory; emits hashes/counts, not copied third-party datasets."""
import csv,hashlib,json,pathlib,re,struct,subprocess,sys
from urllib.parse import quote
S=pathlib.Path(sys.argv[1]).resolve(); R=pathlib.Path(__file__).resolve().parents[1]; NX=R.parents[2]
def sha(p):return subprocess.check_output(['git','-C',str(p),'rev-parse','HEAD'],text=True).strip()
def write(name,data): (R/name).write_text(json.dumps(data,indent=2)+'\n')
repos={p.name:{'repository':('kwsch/PKHeX' if p.name=='PKHeX' else 'projectpokemon/EventsGallery' if p.name=='EventsGallery' else 'pret/'+p.name),'revision':sha(p),'license':('GPL-3.0 reference only; dataset provenance requires review' if p.name=='PKHeX' else 'LICENSE REVIEW REQUIRED; no blanket reuse permission established')} for p in S.iterdir() if (p/'.git').exists()}
repos['PokeBank-NX']={'repository':'GlitchedZeus/PokeBank-NX','revision':'fecfbca9b0323578fd423f5d90486ddb2d0da72e','license':'Existing project license; no licensing change by audit'}
assert repos['PKHeX']['revision']=='6501f0ab46e8f8ca048539dbaf8cae8cb104e722', 'Wrong PKHeX revision'
write('evidence/repositories.json',repos)
paths={
'RED-LEVEL':('pokered','data/pokemon/evos_moves.asm','EvosMovesPointerTable','A'),
'YELLOW-LEVEL':('pokeyellow','data/pokemon/evos_moves.asm','EvosMovesPointerTable','A'),
'GOLD-BREED':('pokegold','engine/pokemon/breeding.asm','InitEggMoves','A'),
'PK-LICENSE':('PKHeX','LICENSE','Version 3','B'),
'NX-LICENSE':('PokeBank-NX','LICENSE','Version 3','B'),
'EV-README':('EventsGallery','README.md','placeholder','B'),
'EV-ALTERED':('EventsGallery','PKHeX Legality/README.md','altered','B'),
'NX-ANALYZE':('PokeBank-NX','src/Legality/Legality.cpp','Report analyze', 'B'),
'NX-EGGMOVE':('PokeBank-NX','include/Legality/Gen34EggMoveEvidence.h','classify','B'),
'NX-MOVE3':('PokeBank-NX','include/Integration/Gen3/Gen3LearnsetTable.h','Availability classify','B'),
'NX-MOVE4':('PokeBank-NX','include/Integration/Gen4/Gen4MoveCompatibility.h','Availability classify','B'),
'NX-EVENT4':('PokeBank-NX','include/Legality/Gen4EventTemplate.h','struct EventTemplate','B'),
'NX-FIELDS':('PokeBank-NX','include/Pokemon/Pokemon.h','ribbonNational','B'),
'NX-WILD4':('PokeBank-NX','include/Legality/Gen4WildEncounter.h','munchlaxTreeIndices','B'),
'NX-TRANSFER':('PokeBank-NX','include/Legality/Gen4TransferEvidence.h','namespace','B'),
'HG-PID':('pokeheartgold','src/pokemon.c','CreateMonWithNature','A'),
'HG-ENCOUNTER':('pokeheartgold','src/field/encounter_check.c','getWildMonNature','A'),
'PK-METHODK':('PKHeX','PKHeX.Core/Legality/RNG/ClassicEra/Gen4/GenerateMethodK.cs','SetRandomK','B'),
'PK-METHODJ':('PKHeX','PKHeX.Core/Legality/RNG/ClassicEra/Gen4/GenerateMethodJ.cs','SetRandomJ','B'),
'NX-RNG4':('PokeBank-NX','include/Legality/Gen4LeadFrameEvidence.h','namespace','B'),
'PK-HISTORY':('PKHeX','PKHeX.Core/Legality/LearnSource/Verify/LearnVerifierHistory.cs','MarkAndIterate','B'),
'PK-BREED':('PKHeX','PKHeX.Core/Legality/Breeding.cs','IsValidSpeciesBit34','B'),
'PK-TRASH':('PKHeX','PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs','VerifyTrashBytesPalPark','B'),
'PK-RIBBON':('PKHeX','PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierUnique4.cs','AddMissingContest4','B'),
'PK-RIBRULE':('PKHeX','PKHeX.Core/Legality/Verifiers/Ribbons/RibbonRules.cs','IsAllowedContest4','B'),
'PK-METHOD':('PKHeX','PKHeX.Core/Legality/RNG/MethodFinder.cs','GetLCRNGMatch','B'),
'PK-LEARNREADER':('PKHeX','PKHeX.Core/Legality/Learnset/LearnsetReader.cs','ReadLearnset16','B'),
'PK-BIN':('PKHeX','PKHeX.Core/Legality/Assets/BinLinkerAccessor16.cs','GetEntry','B'),
'E-DAYCARE':('pokeemerald','src/daycare.c','InheritIVs','A'),
'E-EVO':('pokeemerald','src/evolution_scene.c','CreateShedinja','A'),
'E-LEVEL':('pokeemerald','src/data/pokemon/level_up_learnsets.h','sNinjaskLevelUpLearnset','A'),
'E-RNG':('pokeemerald','include/random.h','ISO_RANDOMIZE1','A'),
'RS-DAYCARE':('pokeruby','src/daycare.c','InheritIVs','A'),
'FR-DAYCARE':('pokefirered','src/daycare.c','InheritIVs','A'),
'C-BREED':('pokecrystal','engine/pokemon/breeding.asm','InitEggMoves','A'),
'C-DAYCARE':('pokecrystal','engine/events/daycare.asm','.GotDVs','A'),
'C-TIMECAPSULE':('pokecrystal','engine/link/time_capsule.asm','', 'A'),
'PT-DAYCARE':('pokeplatinum','src/overlay005/daycare.c','Egg_InheritIVs','A'),
'HG-DAYCARE':('pokeheartgold','src/get_egg.c','InheritIVs','A'),
'HG-LANGUAGE':('pokeheartgold','src/daycare.c','Save_Daycare_MasudaCheck','A'),
}
for n in range(1,5): paths['PK-GROUP'+str(n)]=('PKHeX',f'PKHeX.Core/Legality/LearnSource/Group/LearnGroup{n}.cs','public bool Check','B')
for n in range(2,5): paths['PK-EGGMOVE'+str(n)]=('PKHeX',f'PKHeX.Core/Legality/Moves/Breeding/MoveBreed{n}.cs','Validate','B')
for tag in ['1RB','1YW','2GS','2C','2Stadium','3RS','3E','3FR','3LG','4DP','4Pt','4HGSS']:
 paths['PK-SOURCE'+tag]=('PKHeX',f'PKHeX.Core/Legality/LearnSource/Sources/LearnSource{tag}.cs','GetCanLearn','B')
registry=[]
for id,(repo,path,symbol,grade) in paths.items():
 f=(NX if repo=='PokeBank-NX' else S/repo)/path
 if repo=='EventsGallery':
  data=subprocess.check_output(['git','-C',str(S/repo),'show','HEAD:'+path])
 else:
  assert f.exists(), f
  data=f.read_bytes()
  lines=data.decode().splitlines(); hits=[i+1 for i,s in enumerate(lines) if symbol and symbol in s]
 meta=repos[repo]
 registry.append({'id':id,**meta,'url':f"https://github.com/{meta['repository']}/blob/{meta['revision']}/{quote(path)}",'source_path':path,'symbol':symbol,'matching_lines':hits,'line_range':[1,len(lines)],'sha256':hashlib.sha256(data).hexdigest(),'retrieved_date':'2026-10-11','evidence_grade':grade,'uncertainty':'Grade A identifies retail-source evidence, not a verified complete native specimen; see packet-specific validation limits.'})
# Preserve separately pinned live-reconciliation records when refreshing the baseline inventory.
existing=R/'SOURCE_REGISTRY.json'
if existing.exists():
 ids={r['id'] for r in registry}
 registry.extend(r for r in json.loads(existing.read_text()) if r['id'] not in ids)
write('SOURCE_REGISTRY.json',registry)
# All 11 Gen I-IV level-up resources, decoded only for counts and selected facts.
resources=[]; samples=[]
for tag in ['rb','y','gs','c','rs','e','fr','lg','dp','pt','hgss']:
 p=S/'PKHeX'/f'PKHeX.Core/Resources/byte/levelup/lvlmove_{tag}.pkl'; b=p.read_bytes(); n=struct.unpack_from('<H',b,2)[0]; total=0
 for species in range(n):
  a,z=struct.unpack_from('<HH',b,4+species*2); d=b[a:z]; assert len(d)%3==0
  k=len(d)//3; total+=k
  if species in (172,290,291,292):
   moves=struct.unpack_from('<'+'H'*k,d) if k else []
   for m,lv in zip(moves,d[k*2:]): samples.append({'game_resource':tag,'species':species,'move':m,'level':lv,'source':'PKHeX@'+repos['PKHeX']['revision'],'scope':'reference fact; not specimen'})
 resources.append({'path':str(p.relative_to(S/'PKHeX')),'sha256':hashlib.sha256(b).hexdigest(),'bytes':len(b),'entries':n,'level_move_pairs':total,'license_status':'REFERENCE ONLY; LICENSE REVIEW REQUIRED'})
write('evidence/levelup-resource-manifest.json',resources)
write('evidence/selected-levelup-facts.json',samples)
# Decode accepted packed wild tables to aggregate categories. No completeness inference.
f=NX/'include/Legality/Gen4WildEncounterData.inc'; s=f.read_text().split('kPackedGen4WildEncounters[] = {',1)[1].split('};',1)[0]
counts={}; examples={}
for h in re.findall(r'0x([0-9a-fA-F]+)ULL',s):
 v=int(h,16); game=(v>>43)&7; method=(v>>31)&15; key=(game,method)
 counts[key]=counts.get(key,0)+1
 examples.setdefault(key,{'game':['D','P','Pt','HG','SS'][game],'method_id':method,'species':v&511,'location':(v>>9)&255,'min_level':(v>>17)&127,'max_level':(v>>24)&127,'slot':(v>>46)&15,'rate':(v>>50)&255,'conditions':'Other conditions not reconstructed by this inventory','confidence':'B; accepted packed table'})
write('evidence/encounter-category-inventory.json',[dict(examples[k],rows=counts[k]) for k in sorted(counts)])
print('PASS:',len(registry),'source records;',len(resources),'level-up resources;',sum(counts.values()),'Gen IV packed rows;',len(counts),'game/method categories')
