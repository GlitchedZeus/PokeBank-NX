#!/usr/bin/env python3
"""Unit tests for the frozen WC4 projection integrity/anti-false-proof guard."""
from __future__ import annotations

import csv
import importlib.util
import io
import unittest
from pathlib import Path

MODULE = Path(__file__).resolve().parents[1] / "tools/legality_l4_projection_audit.py"
spec = importlib.util.spec_from_file_location("l4_projection_audit", MODULE)
audit_module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(audit_module)
HEADERS = list(audit_module.REQUIRED)
ROWS = [
    {"projection_index": "0", "species": "4", "tid": "7200", "sid": "58364",
     "pid": "0x0f199b50u", "met_level": "40", "met_location": "3053",
     "ball": "16", "form": "0", "gender": "0", "language": "1",
     "version": "7", "ot_gender": "0", "fateful": "true", "card_id": "67",
     "same_card_id_rows": "2", "archive_grade": "UNVERIFIED",
     "initial_moves_origin": "UNAVAILABLE", "ribbon_origin": "UNAVAILABLE",
     "raw_text_origin": "UNAVAILABLE", "release_attested": "NO", "record_digest_available": "NO"},
    {"projection_index": "1", "species": "25", "tid": "7208", "sid": "27372",
     "pid": "0x0a2b1e97u", "met_level": "50", "met_location": "3060",
     "ball": "16", "form": "0", "gender": "0", "language": "1",
     "version": "10", "ot_gender": "0", "fateful": "true", "card_id": "67",
     "same_card_id_rows": "2", "archive_grade": "UNVERIFIED",
     "initial_moves_origin": "UNAVAILABLE", "ribbon_origin": "UNAVAILABLE",
     "raw_text_origin": "UNAVAILABLE", "release_attested": "NO", "record_digest_available": "NO"},
]
INC = """inline constexpr EventTemplate kGen4EventTemplates[] = {
    {4,7200,58364,0x0f199b50u,40,3053,16,0,0,1,7,0,true,67},
    {25,7208,27372,0x0a2b1e97u,50,3060,16,0,0,1,10,0,true,67},
};"""


def make_csv(rows):
    output = io.StringIO()
    writer = csv.DictWriter(output, fieldnames=HEADERS)
    writer.writeheader()
    writer.writerows(rows)
    return output.getvalue()


def run(rows=None, inc=INC):
    return audit_module.audit(make_csv(ROWS if rows is None else rows), inc,
                              expected_count=2, expected_distinct=1, expected_reused=2)


class L4AuditTests(unittest.TestCase):
    def test_known_projection(self):
        self.assertEqual(run()["reused_card_id_rows"], 2)
        self.assertEqual(run()["original_archives_authenticated"], 0)

    def test_misordered_or_duplicate_projection_index(self):
        rows = [dict(x) for x in ROWS]
        rows[1]["projection_index"] = "0"
        with self.assertRaisesRegex(ValueError, "projection index"):
            run(rows)

    def test_wrong_source_identity_same_reused_card_id(self):
        rows = [dict(x) for x in ROWS]
        rows[1]["species"] = "4"
        with self.assertRaisesRegex(ValueError, "species changed"):
            run(rows)

    def test_wrong_pid_is_not_a_same_card_match(self):
        rows = [dict(x) for x in ROWS]
        rows[0]["pid"] = "0x1u"
        with self.assertRaisesRegex(ValueError, "pid changed"):
            run(rows)

    def test_wrong_collision_count(self):
        rows = [dict(x) for x in ROWS]
        rows[0]["same_card_id_rows"] = "1"
        with self.assertRaisesRegex(ValueError, "same_card_id_rows"):
            run(rows)

    def test_unverified_archive_cannot_be_promoted(self):
        for name, value in [("archive_grade", "OriginalPCD"),
                            ("initial_moves_origin", "ORIGINAL"),
                            ("release_attested", "YES"),
                            ("record_digest_available", "YES")]:
            with self.subTest(name=name):
                rows = [dict(x) for x in ROWS]
                rows[0][name] = value
                with self.assertRaisesRegex(ValueError, "promoted"):
                    run(rows)

    def test_partial_dataset(self):
        with self.assertRaisesRegex(ValueError, "expected 2"):
            run(ROWS[:1])

    def test_changed_generated_projection(self):
        with self.assertRaisesRegex(ValueError, "met_location changed"):
            run(inc=INC.replace("40,3053,16", "40,53,16"))

    def test_malformed_cpp_template(self):
        with self.assertRaisesRegex(ValueError, "template #0"):
            run(inc=INC.replace("true,67", "true"))


if __name__ == "__main__":
    unittest.main()
