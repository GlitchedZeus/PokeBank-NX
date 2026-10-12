#!/usr/bin/env python3
"""Regression checks for the audit-owned integration gate manifest."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "legality_integration_audit", ROOT / "tools" / "run_legality_integration_audit.py")
assert SPEC is not None and SPEC.loader is not None
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class IntegrationAuditRunnerTests(unittest.TestCase):
    def test_complete_manifest_resolves_all_declared_inputs(self) -> None:
        self.assertEqual(AUDIT.validate_manifest("all"), [])

    def test_l5_json_vector_is_a_declared_audit_input(self) -> None:
        vector = AUDIT.fixture_path("legality_l5_native_ev_vectors")
        self.assertEqual(vector.name, "legality_l5_native_ev_vectors.json")
        self.assertTrue(vector.is_file())

    def test_plan_covers_normal_and_sanitized_regressions(self) -> None:
        gates = AUDIT.plan("audit", "a" * 40)
        self.assertEqual([gate.name for gate in gates], [
            "stage1-l2-baseline", "stage2-native", "stage2-native-sanitize",
            "stage3-cross-lane", "stage3-cross-lane-sanitize",
            "stage4-native", "stage4-native-sanitize"])
        self.assertTrue(all(gate.environment["ASAN_OPTIONS"].startswith("detect_leaks=0")
                            for gate in gates))
        self.assertEqual(len(AUDIT.plan("l5", "b" * 40)), 11)


if __name__ == "__main__":
    unittest.main(verbosity=2)
