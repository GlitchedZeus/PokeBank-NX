#!/usr/bin/env python3
"""Unit checks for the fail-closed L6 Stage 2 matrix definition."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("l6_stage2_matrix", ROOT / "tools" / "audit_l6_stage2_matrix.py")
assert SPEC is not None and SPEC.loader is not None
MATRIX = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MATRIX)


class Stage2MatrixTests(unittest.TestCase):
    def test_receipted_native_program_totals(self) -> None:
        self.assertEqual({name: len(lane.programs) for name, lane in MATRIX.LANES.items()}, {
            "l1": 31, "l2": 17, "l3": 19, "l4": 28, "l5": 8, "audit": 3})
        self.assertEqual(sum(len(lane.programs) for lane in MATRIX.LANES.values()), 106)

    def test_current_checkout_is_honestly_incomplete(self) -> None:
        report = MATRIX.preflight()
        self.assertFalse(report["ready"])
        self.assertEqual(report["lanes"]["l1"]["present_sources"], 13)
        self.assertEqual(report["lanes"]["l2"]["present_sources"], 11)
        self.assertEqual(report["lanes"]["l3"]["present_sources"], 7)
        self.assertEqual(report["lanes"]["l4"]["present_sources"], 15)
        self.assertEqual(sum(len(report["lanes"][name]["missing_sources"])
                             for name in ("l1", "l2", "l3", "l4")), 49)
        self.assertEqual(
            {row["path"] for row in report["lanes"]["l5"]["source_pin_mismatches"]},
            {"include/Pokemon/Pokemon3FRLG.h", "src/Pokemon/Pokemon4ReadOnly.cpp"})

    def test_execution_plan_propagates_compiler_and_identity(self) -> None:
        head = "a" * 40
        tree = "b" * 40
        plan = MATRIX.execution_plan("clang++", head, tree)
        self.assertEqual([name for name, _, _ in plan],
                         ["l1", "l2", "l3", "l4-normal", "l4-sanitize", "l5", "audit"])
        for _, _, environment in plan:
            self.assertEqual(environment["CXX"], "clang++")
        for name, command, _ in plan:
            if name in {"l5", "audit"}:
                self.assertIn("--compiler", command)
                self.assertIn("clang++", command)
                self.assertIn(head, command)
                self.assertIn(tree, command)


if __name__ == "__main__":
    unittest.main(verbosity=2)
