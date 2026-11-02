#!/usr/bin/env python3
"""Remove native-unreachable HGSS rod swarm placeholders, preserving native slots.

The full native-data projection has separate map-geometry/Safari obligations.
Only an absent Magikarp Good/Super Rod row is adjudicated by this repair.
The production analyzer continues to use its ordinary typed encounter search.
"""
import argparse, hashlib, json, pathlib, re, urllib.request
from hgss_native_catalog import native_rows

PROJECT=pathlib.Path(__file__).resolve().parents[2]
PINS=PROJECT/'docs/legality/campaign-20261028/HGSS_SWARM_SOURCE_PINS.json'
CORRECTION='// Native HGSS rod-swarm correction: tools/legality_completion/hgss_swarm_repair.py; source pins in docs/legality/campaign-20261028/HGSS_SWARM_SOURCE_PINS.json.\n'


def annotate(source):
    if CORRECTION in source:
        return source
    return CORRECTION+source


def checked_native(root, fetch):
    pins=json.loads(PINS.read_text())
    for row in pins['files']:
        path=root/row['path']
        if fetch and not path.exists():
            data=urllib.request.urlopen(row['url'],timeout=30).read()
            path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        data=path.read_bytes()
        assert hashlib.sha256(data).hexdigest()==row['sha256'],path
        assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==row['git_blob_sha1'],path
    return pins


def parse_array(source, signature):
    prefix,tail=source.split('const '+signature,1)
    before,tail=tail.split('= {',1);body,after=tail.split('\n};',1)
    rows=[[int(x.strip().rstrip('uUlL'),0) for x in m[1:-1].split(',')]
          for m in re.findall(r'\{[^{}]+\}',body)]
    return rows


def render(signature, rows):
    def one(row):return '{'+', '.join(str(x)+('u' if signature.startswith('Area4') and i==4 else '') for i,x in enumerate(row))+'}'
    return 'const '+signature+' = {\n'+''.join('    '+', '.join(one(r) for r in rows[i:i+4])+',\n' for i in range(0,len(rows),4))+'};'


def repaired(source, expected):
    areas=parse_array(source,'Area4 kAreas[]');slots=parse_array(source,'Slot4 kSlots[]')
    out_areas=[];out_slots=[];removed=[]
    for area in areas:
        v,loc,t,rate,tile,start,n=area;kept=[]
        for q in slots[start:start+n]:
            projection=(v,loc,t,rate,q[0],q[2],q[3],q[4])
            if v in (7,8) and t<6 and projection not in expected:
                # A new, unreviewed native mismatch must not be silently deleted.
                assert t in (3,4) and q[0]==129,projection
                removed.append({'area':area,'slot':q,'projection':projection})
            else:kept.append(q)
        assert kept,area
        out_areas.append([v,loc,t,rate,tile,len(out_slots),len(kept)]);out_slots.extend(kept)
    result=source
    for signature,rows in [('Area4 kAreas[]',out_areas),('Slot4 kSlots[]',out_slots)]:
        pattern=r'const '+re.escape(signature)+r'\s*=\s*\{.*?\n\};'
        result,count=re.subn(pattern,lambda _:render(signature,rows),result,count=1,flags=re.S);assert count==1
    return annotate(result),removed,len(out_slots)


def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=pathlib.Path,required=True);p.add_argument('--fetch',action='store_true')
    p.add_argument('--source',type=pathlib.Path,default=PROJECT/'src/Legality/Gen4LegalityData.cpp')
    p.add_argument('--header',type=pathlib.Path,default=PROJECT/'include/Legality/Gen4LegalityData.h')
    p.add_argument('--report',type=pathlib.Path,required=True);p.add_argument('--apply',action='store_true')
    p.add_argument('--removed-rows',type=pathlib.Path,help='also write the removed rows the PKHeX oracles read (tools/pkhex_gen34_diff/hgss_placeholders.py)');a=p.parse_args()
    pins=checked_native(a.native,a.fetch);expected,meta=native_rows(a.native)
    original=a.source.read_text();candidate,removed,count=repaired(original,expected)
    report={'native_commit':pins['commit'],'source_pins':str(PINS.relative_to(PROJECT)),
        'rule':'HGSS rod placeholders cannot become encounters unless the native map/type swarm gate or an ordinary/night slot actually supplies that species.',
        'scope':'Deletes only native-unreachable Magikarp Good/Super Rod rows; preserves other catalog entries and native swarm slots. No full encounter/RNG or historical authentication claim.',
        'before_sha256':hashlib.sha256(original.encode()).hexdigest(),'candidate_sha256':hashlib.sha256(candidate.encode()).hexdigest(),
        'removed':removed,'after_slot_count':count,'native_projection_metadata':meta}
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    if a.removed_rows:
        rows=sorted({(r['area'][0],r['area'][1],r['area'][2],r['slot'][2],r['slot'][3]) for r in removed})
        head=json.dumps({'native_commit':pins['commit'],'species':129,'columns':['version','location','slot_type','level_min','level_max']})
        a.removed_rows.write_text(head[:-1]+', "rows": [\n'+',\n'.join('  '+json.dumps(list(r)) for r in rows)+'\n]}\n')
    if a.apply:
        a.source.write_text(candidate)
        h=a.header.read_text();h,n=re.subn(r'(kSlotCount\s*=\s*)\d+',lambda m:m[1]+str(count),h,count=1);assert n==1;a.header.write_text(annotate(h))
    elif removed:
        raise SystemExit(f'FAIL: {len(removed)} native-unreachable rod variants remain')
    else:
        h=a.header.read_text();assert int(re.search(r'kSlotCount\s*=\s*(\d+)',h)[1])==count
    print(f'Native HGSS swarm catalog: {len(removed)} unreachable variants removed; {count} retained slots PASS')

if __name__=='__main__':main()
