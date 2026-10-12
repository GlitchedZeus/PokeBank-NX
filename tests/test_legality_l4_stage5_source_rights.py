import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("rights",ROOT/"tools/legality_l4_stage5_source_rights.py")
rights=importlib.util.module_from_spec(spec)
spec.loader.exec_module(rights)

BASE={"schema":"L5_Q08_REDISTRIBUTION_GATE_V1","catalog_id":"Q08",
      "not_legal_opinion":True,"project_license_observed":"AGPL-3.0",
      "pkhex_reference_license_observed":"GPL-3.0",
      "repo_rows":[{"source":f"synthetic_source_{i}","legal_review_complete":False,
                    **{name:"blocked_pending_review" for name in rights.PERMISSIONS}}
                   for i in range(14)],
      "dispositions":{**{p:"BLOCKED" for p in rights.DISPOSITIONS},
                      "third_party_research_reference":"allowed_as_unredistributed_reference",
                      "native_original_save_from_user":"consent_and_data_protection_required"}}

def check(b=BASE):return rights.verify_rights(json.dumps(b))

class SourceRights(unittest.TestCase):
    def test_research_only(self):
        self.assertEqual(check()["research_rows"],14)
        self.assertEqual(check()["retail_event_payloads_permitted"],0)

    def test_every_source_every_permission(self):
        for row in range(14):
            for name in rights.PERMISSIONS:
                with self.subTest(row=row,name=name):
                    b=copy.deepcopy(BASE)
                    b["repo_rows"][row][name]="allowed"
                    with self.assertRaisesRegex(ValueError,"no longer blocked"):
                        check(b)

    def test_deleted_or_duplicate_source(self):
        b=copy.deepcopy(BASE);b["repo_rows"][1]["source"]=b["repo_rows"][0]["source"]
        with self.assertRaisesRegex(ValueError,"duplicate"):
            check(b)
        b=copy.deepcopy(BASE);b["repo_rows"]=[]
        with self.assertRaisesRegex(ValueError,"no independently"):
            check(b)

    def test_original_rights_change(self):
        b=copy.deepcopy(BASE);b["repo_rows"][3]["legal_review_complete"]=True
        with self.assertRaisesRegex(ValueError,"legal review"):
            check(b)

    def test_all_global_dispositions(self):
        for name in rights.DISPOSITIONS:
            b=copy.deepcopy(BASE);b["dispositions"][name]="allowed"
            with self.assertRaisesRegex(ValueError,"must remain blocked"):
                check(b)

    def test_cannot_promote_research_only(self):
        b=copy.deepcopy(BASE)
        b["dispositions"]["third_party_research_reference"]="embedded"
        with self.assertRaisesRegex(ValueError,"research-only"):
            check(b)
        b=copy.deepcopy(BASE)
        b["dispositions"]["native_original_save_from_user"]="unrestricted"
        with self.assertRaisesRegex(ValueError,"user-source"):
            check(b)

    def test_license_change(self):
        b=copy.deepcopy(BASE);b["pkhex_reference_license_observed"]="unknown"
        with self.assertRaisesRegex(ValueError,"license context"):
            check(b)

    def test_schema_and_advice_gates(self):
        b=copy.deepcopy(BASE);b["schema"]="Q08v2"
        with self.assertRaisesRegex(ValueError,"schema"):
            check(b)
        b=copy.deepcopy(BASE);b["not_legal_opinion"]=False
        with self.assertRaisesRegex(ValueError,"legal advice"):
            check(b)

if __name__=="__main__":unittest.main()
