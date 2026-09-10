#!/usr/bin/env python3
"""Compare every file of an NRO's embedded RomFS against the intended tree."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

def verify(nro, root, application_source):
    data = nro.read_bytes()
    assert data[16:20] == b'NRO0', 'missing NRO0'
    aset = struct.unpack_from('<I', data, 24)[0]
    assert data[aset:aset+4] == b'ASET', 'missing ASET'
    offset, size = struct.unpack_from('<QQ', data, aset+40)
    start = aset + offset
    assert start + size <= len(data), 'RomFS out of bounds'
    rom = memoryview(data)[start:start+size]
    header = struct.unpack_from('<10Q', rom, 0)
    assert header[0] == 80, 'unexpected RomFS header'
    dm, ds, fm, fs, payload = header[3], header[4], header[7], header[8], header[9]
    assert dm+ds <= size and fm+fs <= size and payload <= size
    seen_dirs, seen_files, files = set(), set(), {}
    end = 0xffffffff
    def directory(entry, parent):
        assert entry not in seen_dirs and entry + 24 <= ds, 'invalid directory link'
        seen_dirs.add(entry)
        _, sibling, child, file_entry, _, n = struct.unpack_from('<6I', rom, dm+entry)
        assert entry+24+n <= ds
        name = bytes(rom[dm+entry+24:dm+entry+24+n]).decode('utf-8')
        assert '/' not in name and name not in ('.', '..')
        path = parent / name
        while file_entry != end:
            assert file_entry not in seen_files and file_entry+32 <= fs, 'invalid file link'
            seen_files.add(file_entry)
            _, nxt, off, length, _, nn = struct.unpack_from('<IIQQII', rom, fm+file_entry)
            assert file_entry+32+nn <= fs and payload+off+length <= size
            filename = bytes(rom[fm+file_entry+32:fm+file_entry+32+nn]).decode('utf-8')
            assert filename and '/' not in filename and filename not in ('.', '..')
            key = (path / filename).as_posix()
            assert key not in files, 'duplicate filename'
            body = bytes(rom[payload+off:payload+off+length])
            intended = root / key
            assert intended.is_file(), 'unexpected embedded file: '+key
            assert body == intended.read_bytes(), 'byte mismatch: '+key
            files[key] = hashlib.sha256(body).hexdigest()
            file_entry = nxt
        while child != end:
            child = directory(child, path)
        return sibling
    directory(0, Path())
    expected = {p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
    assert set(files) == expected, 'missing files: '+str(sorted(expected-set(files)))
    assert len(files) == len(expected), 'embedded file count differs from intended tree'
    short_source = application_source[:8].encode('ascii')
    assert short_source in data, 'application source identity absent'
    return {'result':'PASS', 'file_count':len(files), 'application_source':application_source, 'nro_size':len(data), 'nro_sha256':hashlib.sha256(data).hexdigest()}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('nro', type=Path)
    parser.add_argument('romfs', type=Path)
    parser.add_argument('--application-source', required=True,
                        help='full application-source SHA whose short form must be embedded')
    args = parser.parse_args()
    print(json.dumps(verify(args.nro, args.romfs, args.application_source), indent=2))
