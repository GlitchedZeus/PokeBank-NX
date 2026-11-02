#!/usr/bin/env python3
"""Adjudicate unchanged older-reference stricter rows with official26.10.18.

Original comparison streams and denominators are untouched. This is a second
reference-version measurement, never a native-original-game proof or ID allowlist.
Analysis errors cannot establish a reference-version change.
"""
import argparse, hashlib, json, os, pathlib, subprocess, sys
from functools import cache
PIN='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'


@cache
def rejected_in_release(report):
    data=json.loads(pathlib.Path(report).read_text())
    if data['reference']!=PIN:raise ValueError('Unexpected current reference')
    return {row['sha256'] for row in data['rows'] if row['release']=='INVALID' and not row['analysis_error']}


def changed_in_release(hexdata):
    report=os.environ.get('CURRENT_RELEASE_REPLAY_REPORT')
    if not report:return False
    return hashlib.sha256(bytes.fromhex(hexdata)).hexdigest() in rejected_in_release(report)


def main():
    p=argparse.ArgumentParser();p.add_argument('--group',choices=['gen34','gen5'],required=True)
    p.add_argument('--joined',type=pathlib.Path,required=True);p.add_argument('--source',type=pathlib.Path,required=True)
    p.add_argument('--work',type=pathlib.Path,required=True);a=p.parse_args()
    assert subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()==PIN
    repo=pathlib.Path(__file__).resolve().parents[2];a.work.mkdir(parents=True,exist_ok=True)
    assert not subprocess.check_output(['git','-C',str(a.source),'status','--porcelain','--untracked-files=no','--','PKHeX.Core'],text=True).strip(),'Reference source has tracked edits'
    rows=[]
    original_lines=a.joined.read_text().splitlines()
    for line in original_lines:
        left,right=line.split('\t');l=left.split();r=right.split()
        assert l[0]==r[0]
        if l[3 if a.group=='gen34' else 2]=='VALID' and r[1]=='INVALID':
            rows.append({'id':l[0],'legacy_line':left,'production_line':right,
                'hex':l[2 if a.group=='gen34' else 1]})
    # Empty replay still has an explicit report and leaves the full legacy stream intact.
    if rows:
        name='PKHeXGen34Oracle' if a.group=='gen34' else 'PKHeXGen5Oracle'
        tool='pkhex_gen34_diff' if a.group=='gen34' else 'pkhex_gen5_diff'
        project=repo/'tools'/tool/'oracle'/f'{name}.csproj';binary=a.work/'oracle'
        with (a.work/'build.log').open('w') as log:
            subprocess.run(['dotnet','build',str(project),'-c','Release',f'-p:PKHEX_ROOT={a.source.resolve()}',f'-p:ArtifactsPath={(a.work/"artifacts").resolve()}','-o',str(binary.resolve())],stdout=log,stderr=subprocess.STDOUT,check=True)
        input=a.work/'input.txt';input.write_text('\n'.join(r['legacy_line'] for r in rows)+'\n')
        with (a.work/'reference.txt').open('w') as output:
            subprocess.run(['dotnet',str((binary/f'{name}.dll').resolve()),'replay',str(input.resolve())],stdout=output,check=True)
        replay=(a.work/'reference.txt').read_text().splitlines();assert len(replay)==len(rows)
        for row,line in zip(rows,replay):
            fields=line.split();assert fields[0]==row['id']
            assert fields[2 if a.group=='gen34' else 1]==row['hex'],'Replay changed original bytes'
            row['release']=fields[3 if a.group=='gen34' else 2]
            row['analysis_error']='Misc:Error' in line
            row['reference_line']=line
    for row in rows:row['sha256']=hashlib.sha256(bytes.fromhex(row.pop('hex'))).hexdigest()
    report={'reference':PIN,'reference_source_clean':True,'older_record_count':len(original_lines),
        'dotnet_sdk':subprocess.check_output(['dotnet','--version'],text=True).strip(),
        'oracle_sources':{str(path.relative_to(repo)):hashlib.sha256(path.read_bytes()).hexdigest() for path in (project.parent.glob('*.cs') if rows else [])},
        'reference_assembly_sha256':hashlib.sha256((binary/'PKHeX.Core.dll').read_bytes()).hexdigest() if rows else None,
        'scope':'Unchanged-byte secondary-reference adjudication of all older VALID/native INVALID rows. Does not alter raw counts, prove native history, or establish full applicable rule coverage.',
        'older_joined_sha256':hashlib.sha256(a.joined.read_bytes()).hexdigest(),'rows':rows}
    (a.work/'REPORT.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Secondary official-release replay:',len(rows),'retained stricter rows;',sum(r['release']=='INVALID' and not r['analysis_error'] for r in rows),'reference-version changes')

if __name__=='__main__':main()
