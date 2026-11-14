import hashlib, importlib.util, json, pathlib, random, struct, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('probe',ROOT/'tools/format_probe.py');probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)
class ResearchTests(unittest.TestCase):
    def test_bdsp_integrity_structural_only(self):
        # Structurally synthetic, deliberately NOT a playable save or positive retail fixture.
        for size,version in probe.BDSP.items():
            with self.subTest(size=size):
                b=bytearray(size);struct.pack_into('<I',b,0,version)
                b[probe.HASH_OFFSET:probe.HASH_OFFSET+16]=hashlib.md5(b).digest()
                original=bytes(b);r=probe.bdsp_integrity(b)
                self.assertTrue(r['md5_matches']);self.assertTrue(r['revision_matches_length'])
                self.assertEqual(bytes(b),original);self.assertFalse(r['export_authorized'])
                b[-1]^=1;self.assertFalse(probe.bdsp_integrity(b)['md5_matches'])
                self.assertFalse(probe.bdsp_integrity(b[:-1])['candidate'])
                b=bytearray(original);b[0]^=1;self.assertFalse(probe.bdsp_integrity(b)['revision_matches_length'])
    def test_record_geometry(self):
        rng=random.Random(609)
        for record,sizes in probe.RECORDS.items():
            for stride in sizes:
                for _ in range(250):
                    count=rng.randrange(1,1001);offset=rng.randrange(0,4096);end=offset+count*stride
                    self.assertEqual(probe.checked_range(end,offset,count,stride),(offset,end))
                    self.assertIsNone(probe.checked_range(end-1,offset,count,stride))
        for args in [(0,0,0,0),(5,-1,1,1),(5,0,-1,1),(5,6,0,1),(5,0,10**100,1)]:self.assertIsNone(probe.checked_range(*args))
    def test_pinned_source_tables(self):
        m=json.loads((ROOT/'metadata/pkhex-index.json').read_text())
        self.assertEqual(m['commit'],'641c26ba100225f3605e5a44b61b3b16e15a3774')
        expected={'6XY':55,'6AO':58,'7SM':37,'7USUM':39,'7b':21}
        sizes={'6XY':0x65600,'6AO':0x76000,'7SM':0x6BE00,'7USUM':0x6CC00,'7b':0xB8800}
        for family,table in m['block_tables'].items():
            blocks=table['blocks'];self.assertEqual(len(blocks),expected[family]);last=0
            for b in blocks:
                self.assertGreaterEqual(b['offset'],last);last=b['offset']+b['length']
                self.assertLessEqual(last,sizes[family]-0x200)
        for s in m['sources'].values():self.assertEqual(len(s['sha256']),64);self.assertIn(m['commit'],s['url'])
    def test_transfer_guards(self):
        graph=json.loads((ROOT/'metadata/transfer-policy.json').read_text())
        edges={(r['from'],r['to']) for r in graph['edges']}
        self.assertNotIn(('HOME','Bank'),edges);self.assertNotIn(('ZA','SV'),edges)
        self.assertNotIn(('Gen6','SWSH'),edges)
        self.assertTrue(graph['history_required']);self.assertFalse(graph['cross_game_move_enabled'])
if __name__=='__main__':unittest.main(verbosity=2)
