"""Transport/integrity controls; these synthetic streams do not test legality."""
import hashlib
import importlib.util
import json
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('retained_reference', ROOT / 'tools/legality_completion/verify_reference_artifacts.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class RetainedReference(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = pathlib.Path(self.directory.name)
        secondary = self.root / 'current-release'
        (secondary / 'oracle').mkdir(parents=True)
        assembly = b'synthetic assembly identity control, not PKHeX'
        (secondary / 'oracle/PKHeX.Core.dll').write_bytes(assembly)
        self.hex = '00' * 136
        original = f'case {self.hex} VALID synthetic native egg B ;'
        native = 'case INVALID ; synthetic transport control'
        replay = f'case {self.hex} INVALID synthetic native egg B ; Egg:synthetic'
        joined = original + '\t' + native + '\n'
        for name, text in [('pkhex.txt', original), ('pokebank.txt', native), ('joined.txt', joined.rstrip('\n'))]:
            (self.root / name).write_text(text + '\n')
        (secondary / 'input.txt').write_text(original + '\n')
        (secondary / 'reference.txt').write_text(replay + '\n')
        self.report = dict(reference=module.PIN, reference_source_clean=True,
            older_record_count=1, older_joined_sha256=hashlib.sha256(joined.encode()).hexdigest(),
            reference_assembly_sha256=hashlib.sha256(assembly).hexdigest(),
            rows=[dict(id='case', legacy_line=original, production_line=native,
                       sha256=hashlib.sha256(bytes(136)).hexdigest(), reference_line=replay,
                       release='INVALID', analysis_error=False)])
        self.write_report()

    def write_report(self):
        (self.root / 'current-release/REPORT.json').write_text(json.dumps(self.report))

    def test_complete(self):
        module.verify(self.root, 'gen5')

    def test_omitted_difficult_row_fails(self):
        self.report['rows'] = []
        self.write_report()
        with self.assertRaises(AssertionError): module.verify(self.root, 'gen5')

    def test_changed_input_byte_fails(self):
        p = self.root / 'current-release/reference.txt'
        p.write_text(p.read_text().replace('case ' + self.hex, 'case 01' + self.hex[2:]))
        with self.assertRaises(AssertionError): module.verify(self.root, 'gen5')

    def test_corrupted_standalone_raw_export_fails(self):
        # Real CI artifact12831481595 has '(' in one raw hex string where its
        # joined snapshot has '8'. Preserve both, reject the inconsistency.
        p = self.root / 'pkhex.txt'
        p.write_text(p.read_text().replace('case 00', 'case 0(', 1))
        with self.assertRaises(AssertionError): module.verify(self.root, 'gen5')

    def test_non_stricter_bad_hex_is_never_skipped(self):
        original = 'case 0(' + self.hex[2:] + ' VALID synthetic native egg B ;'
        native = 'case NOPROBLEMS ;'
        joined = original + '\t' + native + '\n'
        (self.root / 'pkhex.txt').write_text(original + '\n')
        (self.root / 'pokebank.txt').write_text(native + '\n')
        (self.root / 'joined.txt').write_text(joined)
        self.report['rows'] = []
        self.report['older_joined_sha256'] = hashlib.sha256(joined.encode()).hexdigest()
        self.write_report()
        with self.assertRaises(ValueError): module.verify(self.root, 'gen5')

    def test_wrong_assembly_fails(self):
        (self.root / 'current-release/oracle/PKHeX.Core.dll').write_bytes(b'wrong assembly')
        with self.assertRaises(AssertionError): module.verify(self.root, 'gen5')

    def test_missing_report_fails(self):
        (self.root / 'current-release/REPORT.json').unlink()
        with self.assertRaises(FileNotFoundError): module.verify(self.root, 'gen5')


if __name__ == '__main__': unittest.main()
