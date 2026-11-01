#!/usr/bin/env python3
"""Join frozen raw discrepancies to existing name diagnostics without changing verdicts.

The native reconstruction is diagnostic evidence, never a whole-record legality oracle.
"""
import argparse, collections, hashlib, json
from pathlib import Path
import release_metrics

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument('streams',type=Path);p.add_argument('--native-research',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixtures',type=Path,required=True);p.add_argument('--after-streams',type=Path)
    a=p.parse_args();research=json.loads(a.native_research.read_text());fields={r['id']:r for r in research['fields']}
    raw=list(release_metrics.read(a.streams,'gen5'));after=list(release_metrics.read(a.after_streams,'gen5')) if a.after_streams else []
    if after:
        assert len(raw)==len(after) and all(l['reference_line']==r['reference_line'] for l,r in zip(raw,after)), 'before/after reference input identity mismatch'
    by_id={r['id']:r for r in after}
    bad=[r for r in raw if r['reference']=='INVALID' and r['nx']=='NOPROBLEMS']
    rows=[]
    games={20:'White',21:'Black',22:'White 2',23:'Black 2'}
    for r in bad:
        parts=r['reference_line'].split();data=bytes.fromhex(parts[1]);assert len(data)==136
        f=fields[r['id']];assert f['lookbackHit'] and not f['currentOnlyHit']
        rows.append(dict(id=r['id'],sha256=hashlib.sha256(data).hexdigest(),format='PK5',bytes=len(data),stored_original_version=data[0x5f],native_origin_game=games.get(data[0x5f]),source_context=' '.join(parts[4:7]),destination_save_game='not supplied; stored version is origin, not receiving save',reference_rule=r['reference_line'].partition(';')[2].strip(),reference=r['reference'],production=r['nx'],production_line=r['production_line'],after_production_line=by_id.get(r['id'],{}).get('production_line'),after_verdict=by_id.get(r['id'],{}).get('nx'),field=f['role'],text=f['message'],pattern=f['pattern'],pattern_index=f['index'],root_cause='new Switch-era lookback word-filter pattern',gen5_current_filter_hit=f['currentOnlyHit'],native_bw2_reconstruction_blocked=f['native_filter_blocked'],native_bw2_ascii_keyboard_scope=f['ascii_keyboard_scope'],adjudication='Incomplete historical evidence; neither definitive Invalid nor a proven native superiority',native_reconstruction_limit='42/740 accented controls failed; published reconstruction not independently matched to a retail ROM; BW and transferred origins outside scoped BW2 keyboard evidence'))
    errors=[dict(id=r['id'],reference_line=r['reference_line'],production_line=r['production_line']) for r in raw if r['misc_error']]
    result=dict(reference='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea',policy='All original rows retained. Raw NOPROBLEMS discrepancies are not reclassified by this ledger. No case IDs influence production. Reference exceptions are separate from demonstrated legality defects.',metrics=release_metrics.metric(raw),false_clean_cases=rows,reference_errors=errors,after_metrics=release_metrics.metric(after) if after else None,native_research_status=research['status'],files={str(x):sha(x) for x in [a.streams/'gen5-reference.txt',a.streams/'gen5-production.txt',a.native_research]})
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2,ensure_ascii=True)+'\n')
    a.fixtures.parent.mkdir(parents=True,exist_ok=True);a.fixtures.write_text('# Frozen original raw false-clean records. Verdicts are baseline observations, not approved historical legality.\n'+''.join(f"{r['id']} NOPROBLEMS {r['reference_line'].split()[1]}\n" for r in bad))
    print(json.dumps(dict(false_clean=len(rows),reference_errors=len(errors),metrics=result['metrics'],source_games=dict(collections.Counter(r['native_origin_game'] or 'transferred' for r in rows))),indent=2))
if __name__=='__main__': main()
