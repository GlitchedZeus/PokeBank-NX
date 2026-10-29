"""Transport controls only; toy payloads assert no Pokémon legality verdict."""
import hashlib
import io
import pathlib
import sys
import tarfile
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/legality_completion'))
from package_reference_evidence import capture,extract_checked


class EvidencePackage(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=pathlib.Path(self.temp.name);self.build=self.root/'build';self.build.mkdir()
        (self.build/'reference-heads.txt').write_text('synthetic-head\n')
        self.run=self.build/'diff-base-1';self.run.mkdir()
        (self.run/'pkhex.txt').write_bytes(b'toy original raw\n')
        (self.run/'joined.txt').write_bytes(b'toy original joined\n')
        self.archive=self.root/'evidence.tar.gz';self.result=capture(self.build,self.archive)

    def test_exact_capture_survives_later_loose_file_change(self):
        (self.run/'pkhex.txt').write_bytes(b'toy conflicting later loose file\n')
        dest=self.root/'extracted'
        extract_checked(self.archive,dest,self.result['archive_sha256'])
        self.assertEqual((dest/'diff-base-1/pkhex.txt').read_bytes(),b'toy original raw\n')
        self.assertEqual((self.run/'pkhex.txt').read_bytes(),b'toy conflicting later loose file\n')

    def test_wrong_attested_archive_hash_rejected(self):
        with self.assertRaises(AssertionError):extract_checked(self.archive,self.root/'out','0'*64)

    def test_member_change_without_manifest_update_rejected(self):
        other=self.root/'changed.tar.gz'
        with tarfile.open(self.archive,'r:gz') as src,tarfile.open(other,'w:gz') as dst:
            for member in src.getmembers():
                data=src.extractfile(member).read()
                if member.name.endswith('pkhex.txt'):data=b'changed after capture\n'
                info=tarfile.TarInfo(member.name);info.size=len(data);dst.addfile(info,io.BytesIO(data))
        with self.assertRaises(AssertionError):extract_checked(other,self.root/'out')

    def test_path_escape_rejected(self):
        other=self.root/'escape.tar.gz'
        with tarfile.open(other,'w:gz') as tar:
            info=tarfile.TarInfo('../escape');info.size=1;tar.addfile(info,io.BytesIO(b'x'))
        with self.assertRaises(AssertionError):extract_checked(other,self.root/'out')
        self.assertFalse((self.root/'escape').exists())

    def test_missing_case_file_rejected(self):
        other=self.root/'omitted.tar.gz'
        with tarfile.open(self.archive,'r:gz') as src,tarfile.open(other,'w:gz') as dst:
            for member in src.getmembers():
                if member.name.endswith('pkhex.txt'):continue
                dst.addfile(member,src.extractfile(member))
        with self.assertRaises(AssertionError):extract_checked(other,self.root/'out')


if __name__=='__main__':unittest.main()
