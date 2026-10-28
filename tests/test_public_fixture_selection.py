#!/usr/bin/env python3
"""Guard the complete pinned native corpus, including Illegal/General folders."""
import importlib.util,json,pathlib,tempfile
repo=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('public_replay',repo/'tools/legality_completion/replay_public_fixtures.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
manifest=json.loads((repo/'docs/legality/hardening-20261027/PUBLIC_FIXTURES.json').read_text())
assert manifest['selected']==105 and manifest['processed']==105 and manifest['errors']==0
paths={r['source_path'].removeprefix('Tests/PKHeX.Core.Tests/Legality/') for r in manifest['records']}
assert len(paths)==105 and any(p.startswith('Illegal/') for p in paths) and any(p.startswith('Legal/General/') for p in paths)
with tempfile.TemporaryDirectory() as tmp:
    root=pathlib.Path(tmp)
    for rel in paths:
        path=root/rel;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'fixture selection only')
    # Misleading parent names must not redefine the native format scope.
    extra=root/'Legal/Generation 9/native.PK3';extra.parent.mkdir(parents=True);extra.write_bytes(b'')
    modern=root/'Illegal/Misc/outside.pk6';modern.write_bytes(b'')
    (root/'not-a-record.pk1').mkdir()
    selected={p.relative_to(root).as_posix() for p in module.native_fixture_paths(root)}
    assert selected==paths|{'Legal/Generation 9/native.PK3'}
print('Public fixture selection: all105 pinned native paths, Illegal/General census and directory-independent scope PASS')
