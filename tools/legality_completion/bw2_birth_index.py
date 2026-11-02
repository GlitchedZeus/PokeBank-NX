#!/usr/bin/env python3
"""Reproduce necessary-order input tables from clean pinned official resources.

Mechanism evidence is the separately executed B2W2 reconstruction. These tables
are released PKHeX data, not an independently acquired complete original ROM.
All machine entries are included to overapproximate possible alternate sources.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
PIN = 'bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'
OUTPUT = ROOT / 'docs/legality/campaign-20261028/BW2_SHARED_ORDER_INDEX.json'
RESOURCES = ('PKHeX.Core/Resources/byte/levelup/lvlmove_b2w2.pkl',
             'PKHeX.Core/Resources/byte/eggmove/eggmove_bw.pkl',
             'PKHeX.Core/Resources/byte/personal/personal_b2w2')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_index(source, exported, native):
    pins = json.loads((ROOT / 'docs/legality/campaign-20261028/BW2_BREEDING_SOURCE_PINS.json').read_text())
    for pin in pins['files']:
        assert sha(native / pin['path']) == pin['sha256'], pin['path']
    data = json.loads(exported.read_text())
    assert [row['species'] for row in data['rows']] == list(range(1, 650))
    pidgey = json.loads((native / 'res/pokemon/pidgey/data.json').read_text())['learnset']
    normalize = lambda name: name.removeprefix('MOVE_').replace('_', '').lower().replace('faintattack', 'feintattack')
    expected = [(level, normalize(move)) for level, move in pidgey['by_level']]
    assert expected == list(zip(data['pidgey_levels'], map(normalize, data['pidgey_names'])))
    names = {normalize(name): index for index, name in enumerate(data['move_names'])}
    row = data['rows'][15]
    assert set(row['egg']) == {names[normalize(name)] for name in pidgey['egg_moves']}
    native_machines = {data['machine_table'][int(item[2:]) - 1 + (95 if item.startswith('HM') else 0)] for item in pidgey['by_tm']}
    assert set(row['machines']) == native_machines
    return dict(schema=1, reference=PIN, mechanism_reference=pins['commit'],
                mechanism_blob=next(r['git_blob_sha1'] for r in pins['files'] if r['path']=='src/ov036/day_care.c'),
                policy='Necessary-condition superset only; all649 base-form rows. No specimen IDs. Published US/EU B2W2 mechanism; data from clean released resources. Native Pidgey level/egg/machine row independently cross-checked. This is not complete regional/ancestry authentication.',
                resources={name:sha(source / name) for name in RESOURCES},
                exporter_sha256=sha(ROOT / 'tools/legality_completion/breeding_reference_trace/Program.cs'),
                entries={str(row['species']):{k:v for k,v in row.items() if k!='species'} for row in data['rows']})


def main():
    p=argparse.ArgumentParser();p.add_argument('--source',type=pathlib.Path,required=True)
    p.add_argument('--native',type=pathlib.Path,required=True);p.add_argument('--work',type=pathlib.Path,required=True)
    p.add_argument('--assembly',type=pathlib.Path);p.add_argument('--exported',type=pathlib.Path)
    p.add_argument('--check',action='store_true');a=p.parse_args();a.work.mkdir(parents=True,exist_ok=True)
    assert subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()==PIN
    assert not subprocess.check_output(['git','-C',str(a.source),'status','--porcelain','--untracked-files=no','--','PKHeX.Core'],text=True).strip()
    exported=a.exported
    if exported is None:
        assembly=a.assembly
        if assembly is None:
            binary=a.work/'core'
            subprocess.run(['dotnet','build',str(a.source/'PKHeX.Core/PKHeX.Core.csproj'),'-c','Release',f'-p:ArtifactsPath={(a.work/"core-artifacts").resolve()}','-o',str(binary.resolve())],check=True)
            assembly=binary/'PKHeX.Core.dll'
        binary=a.work/'exporter'
        subprocess.run(['dotnet','build',str(ROOT/'tools/legality_completion/breeding_reference_trace/Trace.csproj'),'-c','Release',f'-p:PKHEX_ASSEMBLY={assembly.resolve()}',f'-p:ArtifactsPath={(a.work/"exporter-artifacts").resolve()}','-o',str(binary.resolve())],check=True)
        exported=a.work/'exported.json'
        with exported.open('w') as out:subprocess.run(['dotnet',str((binary/'Trace.dll').resolve()),'bw2-index'],stdout=out,check=True)
    result=build_index(a.source,exported,a.native)
    text=json.dumps(result,separators=(',',':'))+'\n'
    if a.check:assert OUTPUT.read_text()==text,'BW2 source-order index differs'
    else:OUTPUT.write_text(text)
    print('BW2 necessary source-order index:649 rows; native Pidgey row equality PASS; SHA256',hashlib.sha256(text.encode()).hexdigest())


if __name__=='__main__':main()
