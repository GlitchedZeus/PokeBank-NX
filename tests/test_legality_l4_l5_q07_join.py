#!/usr/bin/env python3
"""Synthetic L4×L5 Q07 join tests; source receipts stay unverified."""
import csv
import importlib.util
import io
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("l4_l5", ROOT / "tools/legality_l4_l5_q07_join.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
DATA = b"// frozen L4 pinned C++ projection for synthetic join\n"
HEAD = ["projection_index", "species", "tid", "sid", "card_id", "version",
        "language", "archive_grade", "record_digest_available", "release_attested"]
L4 = [dict(projection_index=str(i), species=str(4+i), tid=str(7200+i),
           sid=str(500+i), card_id="67", version="7", language="1",
           archive_grade="UNVERIFIED", record_digest_available="NO",
           release_attested="NO") for i in range(2)]
BASE = {
    "source": {"pkhex_revision": "6501f0ab46e8f8ca048539dbaf8cae8cb104e722",
               "projection_records": 2, "blob_sha": module.git_blob_sha(DATA)},
    "metrics": {"templates": 2, "unique_card_ids": 1, "repeated_card_id_templates": 2,
                "duplicate_cards": 1, "original_card_receipts_verified": 0,
                "archive_item_sha256_verified": 0},
    "rows": []
}
for i, a in enumerate(L4):
    BASE["rows"].append({"template_index": i,
                         "identity_key": f"wc4_6501f0ab:{i}:{a['species']}:{a['tid']}:{a['sid']}:{a['card_id']}",
                         "species": int(a["species"]), "tid": int(a["tid"]), "sid": int(a["sid"]),
                         "card_id": 67, "native_game_code": 7, "native_language_code": 1,
                         "archive_path": None, "original_archive_sha256": None,
                         "archive_provenance": "unverified", "original_release_receipt": "unknown",
                         "license_redistribution_clearance": "not established",
                         "original_payload_moves": "unknown", "original_payload_ribbons": "unknown",
                         "original_raw_trainer_bytes": "unknown",
                         "distribution_language_window": "unknown"})


def ctext(rows):
    out = io.StringIO()
    w = csv.DictWriter(out, fieldnames=HEAD)
    w.writeheader()
    w.writerows(rows)
    return out.getvalue()


def check(data=None, rows=None, cpp=DATA):
    return module.verify(ctext(L4 if rows is None else rows),
                         json.dumps(BASE if data is None else data), cpp,
                         expected_count=2, expected_distinct=1, expected_reused=2)


class JoinTests(unittest.TestCase):
    def test_agreement(self):
        self.assertEqual(check()["matching_composite_keys"], 2)
        self.assertTrue(check()["original_event_provenance_still_unknown"])

    def test_changed_trainer(self):
        x = json.loads(json.dumps(BASE)); x["rows"][0]["sid"] = 0
        with self.assertRaisesRegex(ValueError, "sid"):
            check(x)

    def test_changed_language(self):
        x = json.loads(json.dumps(BASE)); x["rows"][0]["native_language_code"] = 2
        with self.assertRaisesRegex(ValueError, "language"):
            check(x)

    def test_nonunique_or_altered_index(self):
        x = json.loads(json.dumps(BASE)); x["rows"][1]["identity_key"] = x["rows"][0]["identity_key"]
        with self.assertRaisesRegex(ValueError, "composite"):
            check(x)

    def test_stale_template_blob(self):
        with self.assertRaisesRegex(ValueError, "Git blob"):
            check(cpp=DATA+b"stale")

    def test_unverified_original_claim(self):
        x = json.loads(json.dumps(BASE)); x["rows"][0]["archive_path"] = "fake.pcd"
        with self.assertRaisesRegex(ValueError, "original receipt"):
            check(x)

    def test_fabricated_original_digest(self):
        x = json.loads(json.dumps(BASE)); x["metrics"]["archive_item_sha256_verified"] = 1
        with self.assertRaisesRegex(ValueError, "authenticated originals"):
            check(x)

    def test_wrong_count(self):
        x = json.loads(json.dumps(BASE)); x["metrics"]["duplicate_cards"] = 0
        with self.assertRaisesRegex(ValueError, "metrics"):
            check(x)

    def test_wrong_resource_pin(self):
        x = json.loads(json.dumps(BASE)); x["source"]["pkhex_revision"] = "0"*40
        with self.assertRaisesRegex(ValueError, "revision"):
            check(x)


if __name__ == "__main__":
    unittest.main()
