"""Independent fail-closed tests for the live GB comparison protocol."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

COMPARE = Path(__file__).resolve().parents[1] / 'compare.py'
spec = importlib.util.spec_from_file_location('gb_compare', COMPARE)
compare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(compare)


def corpus(body=None, language='0', egg='0'):
    body = body or bytes(48)
    names = '50' * (6 if language == '1' else 11)
    return f'1 fixture {language} {body.hex()} {names} {names} {egg}\n'


class ProtocolTests(unittest.TestCase):
    def run_comparison(self, records, pk, nx):
        with tempfile.TemporaryDirectory() as temp:
            files = [Path(temp) / x for x in ('corpus', 'pk', 'nx')]
            for path, content in zip(files, (records, pk, nx)):
                path.write_text(content)
            return subprocess.run([sys.executable, str(COMPARE), *map(str, files)],
                                  capture_output=True, text=True)

    def test_fail_closed(self):
        valid = '1 VALID origin ;\n'
        for records, pk, nx in (
            ('', '', ''),
            (corpus() * 2, valid, valid),
            (corpus(), valid * 2, valid),
            (corpus(), valid, valid * 2),
            (corpus(), '', valid),
            (corpus(), valid, ''),
            (corpus(), valid + '2 VALID origin ;\n', valid),
            (corpus(), '1 ERROR origin ;\n', '1 ERROR origin ;\n'),
            (corpus(), valid, '1 SKIPPED origin ;\n'),
            (corpus(), valid, '1 VALID origin ; bad problem\n'),
            (corpus(), '1 INVALID\n', valid),
            (corpus(), valid, valid + '\n'),
            ('1 broken\n', valid, valid),
            (corpus(bytes(43)), valid, valid),
            (corpus(language='3'), valid, valid),
            (corpus(egg='2'), valid, valid),
            (corpus().replace('50' * 11, '50', 1), valid, valid),
            (corpus().replace('00' * 48, '0z' * 48), valid, valid),
            (corpus(), valid, '1 UNCERTAIN origin ; | was only distributed at events | impossible move\n'),
            (corpus(), valid, '1 UNCERTAIN was only distributed at events ;\n'),
            (corpus(), '1 INVALID origin ; Nickname:WordFilterFlaggedPatternSuffix\n', valid),
            (corpus(), '1 INVALID origin ; Move:WordFilterFlaggedPattern\n Move:Duplicate\n', valid),
            (corpus(), '1 INVALID origin ; Encounter:EncInvalid\n',
             '1 VALID GS Ball Celebi in Japanese Crystal ;\n'),
            (corpus(), valid, '1 INVALID origin ; | A level-100 Pokemon has exactly 1000000 EXP\n'),
        ):
            with self.subTest(records=records[:20], pk=pk, nx=nx):
                result = self.run_comparison(records, pk, nx)
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_valid_controls(self):
        for verdict in ('VALID', 'INVALID'):
            result = self.run_comparison(corpus(), f'1 {verdict} origin ;\n', f'1 {verdict} origin ;\n')
            self.assertEqual(result.returncode, 0, result.stderr)
        result = self.run_comparison(corpus(), '1 INVALID origin ; Nickname:WordFilterFlaggedPattern\n',
                                     '1 VALID origin ;\n')
        self.assertEqual(result.returncode, 0, result.stderr)
        result = self.run_comparison(corpus(), '1 INVALID origin ; Encounter:EncInvalid\n',
                                     '1 UNCERTAIN origin ; | was only distributed at events\n')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_exact_gsball_exception(self):
        body = bytearray(48)
        body[0], body[31] = 251, 30
        body[29:31] = (0x5e0e).to_bytes(2, 'big')
        pk = '1 INVALID EncounterInvalid ; Encounter:EncInvalid Fateful:FatefulGiftMissing\n'
        nx = '1 VALID GS Ball Celebi in Japanese Crystal (level 30) ;\n'
        self.assertEqual(self.run_comparison(corpus(body, '1'), pk, nx).returncode, 0)
        self.assertNotEqual(self.run_comparison(corpus(body, '0'), pk, nx).returncode, 0)
        pk = pk.rstrip() + ' Move1:Unobtainable\n'
        self.assertNotEqual(self.run_comparison(corpus(body, '1'), pk, nx).returncode, 0)

    def test_level100_exception_requires_only_that_problem(self):
        body = bytearray(48)
        body[0], body[31] = 1, 100
        pk = '1 VALID origin ;\n'
        nx = '1 INVALID origin ; | A level-100 Pokemon has exactly 1059860 EXP\n'
        self.assertEqual(self.run_comparison(corpus(body), pk, nx).returncode, 0)
        self.assertNotEqual(self.run_comparison(corpus(body), pk, nx.rstrip() + ' | duplicate move\n').returncode, 0)


if __name__ == '__main__':
    unittest.main()
