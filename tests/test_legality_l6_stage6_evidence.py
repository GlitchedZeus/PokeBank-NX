"""Reject catalog promotion, missing coverage, and unearned execution receipts."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("stage6", ROOT / "tools/audit_l6_stage6_evidence.py")
AUDIT = importlib.util.module_from_spec(spec)
spec.loader.exec_module(AUDIT)


class QualificationTests(unittest.TestCase):
    def setUp(self):
        self.cells = json.loads((ROOT / AUDIT.Q17).read_text())
        self.events = json.loads((ROOT / AUDIT.Q07).read_text())

    def test_real_catalog_has_all_native_inputs(self):
        self.assertEqual(len(AUDIT.catalog_input(self.cells, self.events).splitlines()), 583)

    def test_duplicate_cell_cannot_fill_coverage(self):
        self.cells["rows"][1] = self.cells["rows"][0]
        with self.assertRaisesRegex(ValueError, "identities"):
            AUDIT.catalog_input(self.cells, self.events)

    def test_source_model_cannot_claim_runtime_comparison(self):
        self.cells["rows"][0]["pkhex_reference_differential_executed"] = True
        with self.assertRaisesRegex(ValueError, "promotion"):
            AUDIT.catalog_input(self.cells, self.events)

    def test_false_string_is_not_false_boolean(self):
        self.cells["rows"][0]["exact_game_specific_runtime_path_executed"] = "false"
        with self.assertRaises(ValueError):
            AUDIT.catalog_input(self.cells, self.events)

    def test_duplicate_event_index_fails(self):
        self.events["rows"][1]["template_index"] = 0
        with self.assertRaisesRegex(ValueError, "index"):
            AUDIT.catalog_input(self.cells, self.events)

    def test_archive_hash_alone_cannot_upgrade_receipt(self):
        self.events["rows"][0]["original_archive_sha256"] = "a" * 64
        with self.assertRaisesRegex(ValueError, "provenance"):
            AUDIT.catalog_input(self.cells, self.events)

    def test_reference_revision_change_requires_review(self):
        self.events["source"]["pkhex_revision"] = "a" * 40
        with self.assertRaisesRegex(ValueError, "revision"):
            AUDIT.catalog_input(self.cells, self.events)

    def test_redistribution_requires_separate_review(self):
        self.events["copying_binaries_permitted"] = True
        with self.assertRaisesRegex(ValueError, "redistribution"):
            AUDIT.catalog_input(self.cells, self.events)

    def test_pinned_dirty_run_never_compiles(self):
        with mock.patch.object(AUDIT, "git", side_effect=["a" * 40, "b" * 40, " M changed.cpp"]), \
             mock.patch.object(AUDIT.subprocess, "run") as launch:
            with self.assertRaisesRegex(RuntimeError, "clean checkout"):
                AUDIT.run("g++", False, "a" * 40, "b" * 40)
            launch.assert_not_called()

    def test_failed_execution_preserves_machine_readable_receipt(self):
        for error in (FileNotFoundError("compiler"), subprocess.TimeoutExpired("compiler", 1),
                      subprocess.CalledProcessError(3, "native")):
            with self.subTest(error=type(error).__name__):
                output = io.StringIO()
                with mock.patch.object(sys, "argv", ["stage6", "--normal"]), \
                     mock.patch.object(AUDIT, "run", side_effect=error), \
                     contextlib.redirect_stdout(output):
                    self.assertEqual(AUDIT.main(), 1)
                report = json.loads(output.getvalue())
                self.assertFalse(report["passed"])
                self.assertEqual(report["runs"][0]["status"], "FAIL")


if __name__ == "__main__":
    unittest.main(verbosity=2)
