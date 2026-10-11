#!/usr/bin/env python3
"""Compare pending NX Emerald table with pinned PKHeX, without copying either dataset."""
import hashlib,json,pathlib,re,struct,subprocess,sys
R=pathlib.Path(__file__).resolve().parents[1]; NX=R.parents[2]
S=pathlib.Path(sys.argv[1]).resolve()/'PKHeX'
rev='6501f0ab46e8f8ca048539dbaf8cae8cb104e722'
assert subprocess.check_output(['git','-C',str(S),'rev-parse','HEAD'],text=True).strip()==rev
pending='b12f4225ae6af9eeb4aa4db9766adc10d69bc51b'
path='include/Legality/Gen3EmeraldLevelUpData.inc'
s=subprocess.check_output(['git','-C',str(NX),'show',pending+':'+path])
def arr(name):return list(map(int,re.findall(r'\d+',s.decode().split(name+'{{',1)[1].split('}};',1)[0])))
off=arr('kEmeraldLevelOffsets'); packed=arr('kEmeraldPackedLevelMoves')
p=S/'PKHeX.Core/Resources/byte/levelup/lvlmove_e.pkl'; b=p.read_bytes()
assert len(off)==388 and off[-1]==len(packed)==3947
mismatch=[]; total=0
for species in range(1,387):
 actual=[(v&511,v>>9) for v in packed[off[species]:off[species+1]]]
 a,z=struct.unpack_from('<HH',b,4+species*2); data=b[a:z]; n=len(data)//3
 expected=list(zip(struct.unpack_from('<'+'H'*n,data),data[2*n:]))
 total+=len(actual)
 if actual!=expected:mismatch.append({'species':species,'nx_count':len(actual),'reference_count':len(expected),'nx_only':sorted(set(actual)-set(expected)),'reference_only':sorted(set(expected)-set(actual)),'order_equal':actual==expected})
result={'nx_revision':pending,'nx_path':path,'nx_sha256':hashlib.sha256(s).hexdigest(),'pkhex_revision':rev,'reference_path':str(p.relative_to(S)),'reference_sha256':hashlib.sha256(b).hexdigest(),'species_compared':386,'ordered_pairs_compared':total,'mismatches':mismatch,'scope':'Cross-source consistency, not independent retail execution or full acquisition feasibility; reference provenance may overlap.'}
(R/'evidence/emerald-levelup-crosscheck.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
