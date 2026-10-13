#!/usr/bin/env python3
"""Hermetic positive/negative pinned-archive contract regression."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import zipfile

SOURCE = Path(__file__).resolve().parents[1] / 'tools' / 'legality_pkhex_source_pin.py'
spec = importlib.util.spec_from_file_location('pin', SOURCE)
pin = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pin)

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    good = root / 'good.zip'
    with zipfile.ZipFile(good, 'w') as output:
        output.writestr('PKHeX-master/PKHeX.Core/PKM/PK5.cs', b'original source bytes')
    manifest = {
        'archive_sha256': pin.file_sha(good),
        'files': [{'path': 'PKHeX.Core/PKM/PK5.cs',
                   'sha256': hashlib.sha256(b'original source bytes').hexdigest()}],
    }
    assert len(pin.verify_archive(good, manifest)) == 1
    source_tree = root / 'PKHeX-sources'
    source_file = source_tree / 'PKHeX.Core/PKM/PK5.cs'
    source_file.parent.mkdir(parents=True)
    source_file.write_bytes(b'original source bytes')
    assert len(pin.verify_source_tree(source_tree, manifest)) == 1
    source_file.write_bytes(b'altered source')
    try:
        pin.verify_source_tree(source_tree, manifest)
        raise AssertionError('wrong upstream must be rejected')
    except ValueError:
        pass
    bad_manifest = {'archive_sha256': manifest['archive_sha256'],
                    'files': [{'path': 'PKHeX.Core/PKM/PK5.cs', 'sha256': '0'*64}]}
    try:
        pin.verify_archive(good, bad_manifest)
        raise AssertionError('a modified source pin must fail closed')
    except ValueError:
        pass
    with zipfile.ZipFile(good, 'a') as output:
        output.writestr('../evil', b'bad')
    manifest['archive_sha256'] = pin.file_sha(good)
    try:
        pin.verify_archive(good, manifest)
        raise AssertionError('path traversal member must fail closed')
    except ValueError:
        pass
    try:
        pin.verify_archive(good, {'archive_sha256': '0'*64, 'files': manifest['files']})
        raise AssertionError('different archive digest must fail closed')
    except ValueError:
        pass
    assert pin.git_blob_sha(b'hi\n') == '45b983be36b73c0788dc9cbcb76cbb80fc7bb057'
    assert not pin.safe_member_name('../evil')
    assert not pin.safe_member_name('bad\\name')
    assert pin.safe_member_name('PKHeX.Core/PKM/PK5.cs')
print('PKHeX source pin test: archive digest, exact file, traversal, mismatch, Git blob PASS')
