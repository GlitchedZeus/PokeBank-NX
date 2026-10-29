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
      "repo_rows":[{"source":name,"repository":"synthetic://example", "revision":"synthetic",
                    "source_license_claim":"review required",
                    "proposed_usage":rights.SOURCE_SPECIFIC_USAGE.get(name,rights.DEFAULT_USAGE),
                    "legal_basis_or_grant":"not documented in pinned registry",
                    "legal_review_complete":False,
                    **{name:"blocked_pending_review" for name in rights.PERMISSIONS}}
                   for name in sorted(rights.EXPECTED_SOURCES)],
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

    def test_all_frozen_sources_required_and_no_payload_injection(self):
        b=copy.deepcopy(BASE)
        b["repo_rows"].pop(3)
        with self.assertRaisesRegex(ValueError,"source count"):
            check(b)
        b=copy.deepcopy(BASE)
        b["repo_rows"][0]["binary_payload_hex"]="deadbeef"
        with self.assertRaisesRegex(ValueError,"schema/payload injection"):
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

    def test_usage_is_pinned_per_source(self):
        index={row["source"]:i for i,row in enumerate(BASE["repo_rows"])}
        for name in rights.EXPECTED_SOURCES:
            for usage in ("unrestricted",rights.DEFAULT_USAGE,
                          *rights.SOURCE_SPECIFIC_USAGE.values()):
                if usage==rights.SOURCE_SPECIFIC_USAGE.get(name,rights.DEFAULT_USAGE):
                    continue
                with self.subTest(source=name,usage=usage):
                    b=copy.deepcopy(BASE)
                    b["repo_rows"][index[name]]["proposed_usage"]=usage
                    with self.assertRaisesRegex(ValueError,"usage changed"):
                        check(b)
        b=copy.deepcopy(BASE)
        b["repo_rows"][index["pokediamond"]]["legal_basis_or_grant"]="granted"
        with self.assertRaisesRegex(ValueError,"usage changed"):
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
