#!/usr/bin/env python3
"""Swallowed reference exceptions cannot certify legality agreement."""
import importlib.util
from pathlib import Path
import unittest
spec=importlib.util.spec_from_file_location("release_metrics",Path(__file__).resolve().parents[1]/"tools/legality_completion/release_metrics.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class ReferenceErrors(unittest.TestCase):
    def row(self,ref,nx,error=False):
        return dict(reference=ref,nx=nx,misc_error=error,fishy=False)
    def test_errors_stay_in_raw_but_not_successes(self):
        rows=[self.row("VALID","NOPROBLEMS"),self.row("INVALID","INVALID",True),self.row("INVALID","NOPROBLEMS",True),self.row("INVALID","INCOMPLETE"),self.row("INVALID","NOPROBLEMS")]
        r=m.metric(rows)
        self.assertEqual((r['records'],r['raw_agreement'],r['raw_false_clean']),(5,2,2))
        self.assertEqual((r['reference_success_records'],r['reference_success_agreement'],r['reference_success_false_clean']),(3,1,1))
        self.assertEqual(r['reference_error_raw_agreements'],1)
        self.assertEqual(r['incomplete'],1)
    def test_no_successful_oracle_results(self):
        r=m.metric([self.row("INVALID","INVALID",True)])
        self.assertEqual(r['reference_success_records'],0)
        self.assertIsNone(r['reference_success_agreement_percent'])
if __name__=='__main__':unittest.main()
