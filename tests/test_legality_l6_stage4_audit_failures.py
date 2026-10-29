#!/usr/bin/env python3
"""L6 Stage 4 audit failure-path regressions (no source save access)."""
from __future__ import annotations
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "audit_l6_stage4", ROOT / "tools/run_legality_integration_audit.py")
assert spec and spec.loader
AUDIT = importlib.util.module_from_spec(spec)
spec.loader.exec_module(AUDIT)

class FailurePathTests(unittest.TestCase):
    def gate(self, *args: str):
        return AUDIT.Gate("adversarial", list(args), {}, None)

    def test_nonzero_is_never_pass(self):
        with contextlib.redirect_stdout(io.StringIO()):
            outcome = AUDIT.run_one(self.gate(sys.executable, "-c", "raise SystemExit(34)"), 5)
        self.assertEqual(outcome["returncode"], 34)
        self.assertFalse(outcome["passed"])
        self.assertFalse(outcome["timed_out"])

    def test_success_records_real_exit(self):
        with contextlib.redirect_stdout(io.StringIO()):
            outcome = AUDIT.run_one(self.gate(sys.executable, "-c", "print(123)"), 5)
        self.assertEqual(outcome["returncode"], 0)
        self.assertTrue(outcome["passed"])

    def test_timeout_with_raw_bytes_is_recorded_not_exception(self):
        code = "import os,time;os.write(1,b'\\xff\\x00partial');time.sleep(4)"
        with contextlib.redirect_stdout(io.StringIO()):
            outcome = AUDIT.run_one(self.gate(sys.executable, "-c", code), 1)
        self.assertTrue(outcome["timed_out"])
        self.assertIsNone(outcome["returncode"])
        self.assertFalse(outcome["passed"])

    def test_spawn_error_produces_failure_receipt(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            result = AUDIT.run_one(self.gate("/path/that/does/not/exist/l6"), 5)
        self.assertFalse(result["passed"])
        self.assertIsNone(result["returncode"])
        self.assertIn("FileNotFoundError", result["launch_error"])
        self.assertIn("FileNotFoundError", err.getvalue())

    def test_dry_run_cannot_claim_executed_tests(self):
        def fakegit(*parts: str):
            if parts[-1] == "HEAD": return "a" * 40
            if parts[-1] == "HEAD^{tree}": return "b" * 40
            if parts[-1] == "--untracked-files=no": return ""
            return "audit/l6"
        stdout = io.StringIO()
        with mock.patch.object(AUDIT, "git", side_effect=fakegit), \
             mock.patch.object(AUDIT, "validate_manifest", return_value=[]), \
             mock.patch.object(AUDIT, "version", return_value="synthetic version"), \
             mock.patch.object(sys, "argv", ["audit", "--dry-run", "--suite", "audit"]), \
             contextlib.redirect_stdout(stdout):
            code = AUDIT.main()
        report = json.loads(stdout.getvalue())
        self.assertEqual(code, 0) # successful manifest preflight only
        self.assertFalse(report["passed"])
        self.assertTrue(report["dry_run_valid"])
        self.assertEqual(report["commands"], [])

if __name__ == "__main__":
    unittest.main(verbosity=2)
