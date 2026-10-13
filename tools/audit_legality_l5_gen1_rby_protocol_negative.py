#!/usr/bin/env python3
"""L5 adversarial differential *protocol* QA.

All three observation streams are SYNTHETIC in this test.  Passing this
suite does not attest any PKHeX binary run or original Red/Blue/Yellow save.
It ensures the production-output comparison fails closed on swapped cases,
altered native fields, false retail provenance, and source-context loss.
"""
from __future__ import annotations
from copy import deepcopy
import json
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parent))
import audit_legality_l5_gen1_rby_live_diff as audit

ROOT=Path(__file__).resolve().parent.parent
VECTORS=ROOT/"tests/fixtures/legality_l5_gen1_rby_oracle_cases.json"
STAT_FIELDS=("stat_exp_hp","stat_exp_attack","stat_exp_defense",
             "stat_exp_speed","stat_exp_special")

def simulated_inputs():
    cases=audit.load_vectors(VECTORS)
    pkhex,accepted,coordinator={},{},{}
    for v in cases:
        base={"schema":audit.SCHEMA,"id":v["id"],"source":v["source"],
              "species":v["species"],"move":v["move"],"dv16":v["dv16"],
              "level":v["level"],"experience":v["experience"],
              "original_retail_history_authenticated":False,
              "ability_invalid":False,"gender_invalid":False,
              **audit.dv_values(v["dv16"]),
              **{k:v[k] for k in STAT_FIELDS}}
        pkhex[v["id"]]={
            **base,"engine":"PKHEX_CORE",
            "execution_mode":"actual_original_pkhex_core",
            "pkhex_revision":audit.PIN,"original_source":"RBY_VERSION_UNKNOWN",
            "pk1_ability":-1,"pk1_gender":2,"parsed":True,
        }
        accepted[v["id"]]={
            **base,"engine":"POKEBANK_NX_ANALYZER",
            "execution_mode":"actual_production_pk1","nx_variant":"baseline",
            "ability_invalid":v["source"]=="",
            "gender_invalid":v["source"]=="" and v["species"]==1,
            "move_domain_invalid":v["move"]==200,"modern_ev_invalid":False,
            "source_conflict_warning":v["source"]=="platinum_nds",
        }
        coordinator[v["id"]]={
            **accepted[v["id"]],"nx_variant":"coordinator",
            "ability_invalid":False,"gender_invalid":False,
        }
    return cases,pkhex,accepted,coordinator

class IntegrityTests(unittest.TestCase):
    def setUp(self):
        self.cases,self.pkhex,self.accepted,self.coordinator=simulated_inputs()
        self.ident=self.cases[0]["id"]

    def evaluate(self,pkhex=None,accepted=None,coordinator=None,cases=None):
        return audit.evaluate(cases if cases is not None else self.cases,
               pkhex if pkhex is not None else self.pkhex,
               accepted if accepted is not None else self.accepted,
               coordinator if coordinator is not None else self.coordinator)

    def test_complete_synthetic_protocol_controls(self):
        result=self.evaluate()
        self.assertEqual(result["vectors"],72)
        self.assertFalse(result["full_gen1_legal_parity_certified"])
        self.assertEqual(result["accepted_baseline_non_native_false_invalid_counts"],
                         {"ability_invalid":12,"gender_invalid":6})
        self.assertEqual(result["coordinator_non_native_defect_disagreements"],0)
        for source,counts in result["defect_disagreements_by_source_context"].items():
            with self.subTest(source=source):
                self.assertEqual(counts["vectors"],12)
                self.assertEqual(counts["coordinator_f3"],0)
                self.assertEqual(counts["coordinator_f4"],0)

    def test_every_observation_field_tamper_is_rejected(self):
        fields={
            "species":81 if self.cases[0]["species"]==1 else 1,
            "move":250,"dv16":1234,"source":"forged_green_gb",
            "level":37,"experience":4,
            "dv_hp":200,"dv_attack":200,"dv_defense":200,
            "dv_speed":200,"dv_special":200,
            "stat_exp_hp":2,"stat_exp_attack":2,"stat_exp_defense":2,
            "stat_exp_speed":2,"stat_exp_special":2,
        }
        for stream_name in ("pkhex","accepted","coordinator"):
            for field,value in fields.items():
                with self.subTest(stream=stream_name,field=field):
                    streams={n:deepcopy(getattr(self,n)) for n in
                             ("pkhex","accepted","coordinator")}
                    streams[stream_name][self.ident][field]=value
                    with self.assertRaises(ValueError):
                        self.evaluate(**streams)

    def test_non_native_coordinator_f3_f4_regression_fails(self):
        for field in ("ability_invalid","gender_invalid"):
            broken=deepcopy(self.coordinator)
            broken[self.ident][field]=True
            with self.subTest(field=field),self.assertRaises(ValueError):
                self.evaluate(coordinator=broken)

    def test_native_false_negatives_and_gen1_ev_misread_fail(self):
        for field in ("move_domain_invalid","modern_ev_invalid"):
            broken=deepcopy(self.coordinator)
            broken[self.ident][field]=True
            with self.subTest(field=field),self.assertRaises(ValueError):
                self.evaluate(coordinator=broken)

    def test_wrong_implementation_cannot_pose_as_coordinator(self):
        swapped=deepcopy(self.coordinator)
        swapped[self.ident]["nx_variant"]="baseline"
        with self.assertRaisesRegex(ValueError,"EXECUTION_SWAP"):
            self.evaluate(coordinator=swapped)

    def test_original_pkhex_must_really_complete(self):
        for field,value in (("parsed",False),("pk1_ability",0),
                            ("gender_invalid",True),("pk1_gender",3)):
            broken=deepcopy(self.pkhex)
            broken[self.ident][field]=value
            with self.subTest(field=field),self.assertRaises(ValueError):
                self.evaluate(pkhex=broken)

    def test_truncated_evidence_cannot_certify_parity(self):
        broken=deepcopy(self.coordinator)
        del broken[self.ident]
        with self.assertRaisesRegex(ValueError,"ID_OR_VECTOR_MISMATCH"):
            self.evaluate(coordinator=broken)

    def test_jsonl_variant_pin_retail_and_duplicate_key_guards(self):
        with TemporaryDirectory() as td:
            path=Path(td)/"observation.jsonl"
            good=list(self.coordinator.values())
            path.write_text("\n".join(json.dumps(row) for row in good)+"\n")
            self.assertEqual(len(audit.read_jsonl(
                path,"POKEBANK_NX_ANALYZER","coordinator")),72)
            for field,value,diagnostic in (
                ("nx_variant","baseline","EXECUTION_VARIANT_MISMATCH"),
                ("original_retail_history_authenticated",True,"ORIGINAL_FABRICATION"),
                ("execution_mode","source_modeled","OBS_NOT_LIVE"),
            ):
                broken=deepcopy(good)
                broken[0][field]=value
                path.write_text("\n".join(json.dumps(row) for row in broken)+"\n")
                with self.subTest(field=field),self.assertRaisesRegex(
                      ValueError,diagnostic):
                    audit.read_jsonl(path,"POKEBANK_NX_ANALYZER","coordinator")
            first=json.dumps(good[0])
            attack=first[:-1]+',"engine":"FORGED"}'
            path.write_text(attack+"\n"+"\n".join(json.dumps(r) for r in good[1:]))
            with self.assertRaisesRegex(ValueError,"DUPLICATE_JSON_KEY"):
                audit.read_jsonl(path,"POKEBANK_NX_ANALYZER","coordinator")
            forged=deepcopy(list(self.pkhex.values()))
            forged[0]["pkhex_revision"]="unverified"
            path.write_text("\n".join(json.dumps(r) for r in forged))
            with self.assertRaisesRegex(ValueError,"ORACLE_REVISION"):
                audit.read_jsonl(path,"PKHEX_CORE")

    def test_vector_cartesian_replacement_detected(self):
        with TemporaryDirectory() as td:
            file=Path(td)/"cases.json"
            data=json.loads(VECTORS.read_text())
            data["cases"][0]["source"]=data["cases"][1]["source"]
            data["cases"][0]["id"]="different_unique_id"
            file.write_text(json.dumps(data))
            with self.assertRaises(ValueError):
                audit.load_vectors(file)

    def test_vector_json_duplicate_key_detected(self):
        with TemporaryDirectory() as td:
            file=Path(td)/"cases.json"
            raw=VECTORS.read_text()
            file.write_text('{"schema":"FAKE",'+raw.lstrip()[1:])
            with self.assertRaisesRegex(ValueError,"DUPLICATE_JSON_KEY"):
                audit.load_vectors(file)

if __name__=="__main__":
    unittest.main(verbosity=2)
