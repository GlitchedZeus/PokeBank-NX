#!/usr/bin/env python3
"""Fail-closed original PKHeX snapshot/source and shared Product PK5 blob pin.

Never extracts archive members or ships copyrighted bytes. The pinned archive
must be passed by the user/CI independently; --product-only only verifies the
unchanged PR #300 native reader, and is NOT a PKHeX oracle run.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import stat
import zipfile

PRODUCT_SHA = {
    'include/Encryption/Encryption5.h': '68296b970114d11e98ecd87ce69f5b8a61ccef52',
    'include/Pokemon/Pokemon5ReadOnly.h': '27f75159178a70b7b78939d86f50b0364d4482ce',
}

def git_blob_sha(contents: bytes) -> str:
    return hashlib.sha1(b'blob ' + str(len(contents)).encode('ascii') + b'\0' + contents).hexdigest()

def verify_product(root: Path) -> dict:
    verified = {}
    for relative, expected in PRODUCT_SHA.items():
        actual = git_blob_sha((root / relative).read_bytes())
        if actual != expected:
            raise ValueError(f'Product PR #300 blob drift: {relative} expected {expected}, got {actual}')
        verified[relative] = actual
    return verified

def file_sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as reader:
        for chunk in iter(lambda: reader.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()

def safe_member_name(name: str) -> bool:
    parts = PurePosixPath(name).parts
    return (bool(parts) and not name.startswith('/') and '\\' not in name
            and all(part not in ('', '.', '..') for part in name.split('/')))

def verify_archive(archive: Path, manifest: dict) -> dict:
    expected = manifest['archive_sha256']
    if not re.fullmatch(r'[0-9a-f]{64}', expected):
        raise ValueError('Malformed archive SHA-256 pin')
    actual = file_sha(archive)
    if actual != expected:
        raise ValueError(f'PKHeX archive SHA-256 mismatch: expected {expected}, got {actual}')
    results = {}
    with zipfile.ZipFile(archive) as bundle:
        infos = bundle.infolist()
        names = set()
        for entry in infos:
            if not safe_member_name(entry.filename.rstrip('/')):
                raise ValueError(f'Unsafe archive member: {entry.filename!r}')
            if entry.filename in names:
                raise ValueError(f'Duplicate archive member: {entry.filename!r}')
            names.add(entry.filename)
            if stat.S_IFMT(entry.external_attr >> 16) == stat.S_IFLNK:
                raise ValueError(f'Archive symlink forbidden: {entry.filename!r}')
        for item in manifest['files']:
            expected_path = item['path']
            expected_digest = item['sha256']
            if not safe_member_name(expected_path) or not re.fullmatch(r'[0-9a-f]{64}', expected_digest):
                raise ValueError('Malformed PKHeX source-file pin')
            matches = [entry for entry in infos if entry.filename == expected_path
                       or (entry.filename.endswith('/' + expected_path)
                           and len(PurePosixPath(entry.filename).parts) == len(PurePosixPath(expected_path).parts) + 1)]
            if len(matches) != 1:
                raise ValueError(f'Pinned source must have exactly one member: {expected_path}')
            if matches[0].file_size > 16 * 1024 * 1024:
                raise ValueError(f'Pinned source unexpectedly large: {expected_path}')
            digest = hashlib.sha256()
            with bundle.open(matches[0]) as member:
                for chunk in iter(lambda: member.read(65536), b''):
                    digest.update(chunk)
            if digest.hexdigest() != expected_digest:
                raise ValueError(f'Pinned PKHeX source mismatch: {expected_path}')
            results[expected_path] = expected_digest
    return results

def verify_source_tree(root: Path, manifest: dict) -> dict:
    """Reject public revisions that differ from the owner's archived source files."""
    results = {}
    for item in manifest['files']:
        path, expected = item['path'], item['sha256']
        if not safe_member_name(path) or not re.fullmatch(r'[0-9a-f]{64}', expected):
            raise ValueError('Malformed pinned source path/digest')
        actual = file_sha(root / path)
        if actual != expected:
            raise ValueError(f'Public PKHeX checkout NOT equivalent to owner archive: {path}')
        results[path] = actual
    return results

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=Path('.'))
    parser.add_argument('--manifest', type=Path, default=Path('docs/legality/research/pkhex/PKHEX_USER_ARCHIVE_SOURCE_MAP.json'))
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--upstream-source-root', type=Path)
    parser.add_argument('--product-only', action='store_true')
    args = parser.parse_args(argv)
    if (args.product_only and (args.archive or args.upstream_source_root)) or (args.archive and args.upstream_source_root):
        parser.error('select exactly one of --product-only, --archive or --upstream-source-root')
    if not args.product_only and args.archive is None and args.upstream_source_root is None:
        parser.error('PKHeX archive or pinned upstream source tree required')
    product = verify_product(args.repo_root)
    sources = {}
    if args.archive is not None or args.upstream_source_root is not None:
        manifest = json.loads((args.repo_root / args.manifest).read_text())
        sources = verify_archive(args.archive, manifest) if args.archive else verify_source_tree(args.upstream_source_root, manifest)
    print(json.dumps({'product_reader_blobs_verified': product,
                      'pkhex_archive_verified': args.archive is not None,
                      'pkhex_upstream_source_verified': args.upstream_source_root is not None,
                      'pkhex_source_files_verified': len(sources)}, sort_keys=True))
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
