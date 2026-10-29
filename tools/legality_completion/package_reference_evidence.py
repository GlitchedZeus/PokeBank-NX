#!/usr/bin/env python3
"""Seal every available reference stream, then verify that immutable capture.

The CI log records the verified archive hash. Packaging never edits a record or
changes validity. Failed/partial runs can be captured for forensics but cannot
pass verification against the complete expected run set.
"""
import argparse
import hashlib
import io
import json
import pathlib
import tarfile
import tempfile
from verify_reference_artifacts import verify,PIN


def sha(data):
    return hashlib.sha256(data).hexdigest()


def capture(build,archive):
    paths=[build/'reference-heads.txt']
    for run in sorted(build.glob('diff-*')):
        for pattern in ('pkhex.txt','pokebank.txt','joined.txt','base.txt','build.log','*generation-audit.*','oracle/*.dll',
                        'current-release/REPORT.json','current-release/input.txt',
                        'current-release/reference.txt','current-release/oracle/*.dll'):
            paths.extend(sorted(run.glob(pattern)))
    paths=sorted(set(p for p in paths if p.is_file()))
    assert build/'reference-heads.txt' in paths,'Missing source heads'
    entries={}
    archive.parent.mkdir(parents=True,exist_ok=True)
    with tarfile.open(archive,'w:gz',compresslevel=3) as tar:
        def add(name,data):
            info=tarfile.TarInfo(name);info.size=len(data);info.mode=0o644
            tar.addfile(info,io.BytesIO(data))
        for path in paths:
            name=path.relative_to(build).as_posix();data=path.read_bytes()
            entries[name]={'sha256':sha(data),'bytes':len(data)}
            add(name,data)
        manifest={'schema':1,'policy':'Exact available raw/reference/assembly bytes; no row filtering or validity edits. Completeness requires the separate expected-run verification.','files':entries}
        encoded=(json.dumps(manifest,indent=2)+'\n').encode()
        (build/'reference-files.json').write_bytes(encoded)
        add('reference-files.json',encoded)
    return {'archive_sha256':sha(archive.read_bytes()),'manifest_sha256':sha(encoded),'files':len(entries)}


def extract_checked(archive,destination,expected_sha=None):
    digest=sha(archive.read_bytes())
    if expected_sha:assert digest==expected_sha,'Archive differs from the CI-verified hash'
    assert not destination.exists(),'Extraction requires a new directory'
    with tarfile.open(archive,'r:gz') as tar:
        members=tar.getmembers();names=[m.name for m in members]
        assert len(names)==len(set(names)),'Duplicate archive member'
        for member in members:
            path=pathlib.PurePosixPath(member.name)
            assert member.isfile() and not path.is_absolute() and '..' not in path.parts
            assert str(path)==member.name and member.name not in ('','.','reference-files.json/'),member.name
        raw=tar.extractfile('reference-files.json').read()
        manifest=json.loads(raw);assert manifest['schema']==1
        assert set(names)==set(manifest['files'])|{'reference-files.json'},'Missing/unrecorded archive member'
        destination.mkdir(parents=True)
        for member in members:
            data=tar.extractfile(member).read()
            if member.name!='reference-files.json':
                entry=manifest['files'][member.name]
                assert len(data)==entry['bytes'] and sha(data)==entry['sha256'],member.name
            target=destination.joinpath(*pathlib.PurePosixPath(member.name).parts)
            target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    return {'archive_sha256':digest,'manifest_sha256':sha(raw),'files':len(manifest['files'])}


def main():
    p=argparse.ArgumentParser();p.add_argument('--build',type=pathlib.Path)
    p.add_argument('--archive',type=pathlib.Path,required=True);p.add_argument('--verify',action='store_true')
    p.add_argument('--expected-sha');p.add_argument('--head');p.add_argument('--group',choices=['gen34','gen5'])
    p.add_argument('--runs',nargs='+');p.add_argument('--extract',type=pathlib.Path);a=p.parse_args()
    if not a.verify:
        assert a.build is not None
        print(json.dumps(capture(a.build,a.archive),sort_keys=True));return
    with tempfile.TemporaryDirectory() as tmp:
        root=a.extract if a.extract is not None else pathlib.Path(tmp)/'capture'
        result=extract_checked(a.archive,root,a.expected_sha)
        if a.head:assert (root/'reference-heads.txt').read_text().splitlines()[0]==a.head,'Wrong engine head'
        if a.group:
            assert a.runs,'The complete expected run set is required'
            heads=(root/'reference-heads.txt').read_text().splitlines()
            assert len(heads)==3 and heads[1]=='6501f0ab46e8f8ca048539dbaf8cae8cb104e722' and heads[2]==PIN,'Wrong pinned reference heads'
            actual={p.name for p in root.glob('diff-*') if p.is_dir()}
            assert actual==set(a.runs),'Missing or unverified extra run'
            for run in a.runs:verify(root/run,a.group)
        result['sealed_verification']='PASS';print(json.dumps(result,sort_keys=True))


if __name__=='__main__':main()
