#!/usr/bin/env python3
"""L5 independent production Invalid-site release gate.

Use on a FULL coordinator checkout; this file does not modify production:
 python3 tools/audit_legality_l5_invalid_site_gate.py \
   --source src/Legality/Legality.cpp --expected-invalid 37 --require-f3-f4

The count is *lexical* proof of all direct callsites, not encounter completeness,
historical legality, or authenticated PKHeX parity.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

NON_CODE = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|'
    r"'(?:\\.|[^'\\])*'",
    re.DOTALL,
)
ADD = re.compile(r"\badd\s*\(\s*r\s*,")
EXPLICIT = re.compile(
    r"\badd\s*\(\s*r\s*,\s*(Severity::(?:Info|Warning|Invalid))\s*,"
)
ABILITY = re.compile(
    r"if\s*\(\s*nativeFormatGeneration\s*!=\s*1\s*&&\s*"
    r"nativeFormatGeneration\s*!=\s*2\s*&&\s*"
    r"\(\s*!sourceProfile\s*\|\|\s*exactGeneration\s*>=\s*3\s*\)\s*\)",
    re.DOTALL,
)
GENDER = re.compile(
    r"if\s*\(\s*nativeFormatGeneration\s*!=\s*1\s*&&\s*"
    r"\(\s*!sourceProfile\s*\|\|\s*exactGeneration\s*>=\s*2\s*\)\s*\)",
    re.DOTALL,
)


def strip_literals_and_comments(source: str) -> str:
    """Keep offsets/line breaks so diagnostic sites refer to original lines."""
    return NON_CODE.sub(lambda m: re.sub(r"[^\n]", " ", m.group()), source)


def scan(source: str) -> dict:
    code = strip_literals_and_comments(source)
    starts = [m.start() for m in ADD.finditer(code)]
    explicit = list(EXPLICIT.finditer(code))
    invalid = [m for m in explicit if m.group(1) == "Severity::Invalid"]
    known = {m.start() for m in explicit}
    return {
        "sha256": hashlib.sha256(source.encode("utf-8")).hexdigest(),
        "total_add_calls": len(starts),
        "explicit_add_calls": len(explicit),
        "nonexplicit_severity_lines": [
            code.count("\n", 0, start) + 1 for start in starts if start not in known
        ],
        "invalid_count": len(invalid),
        "invalid_lines": [code.count("\n", 0, m.start()) + 1 for m in invalid],
        "f3_native_ability_guard": bool(ABILITY.search(code)),
        "f4_native_gender_guard": bool(GENDER.search(code)),
    }


def self_test() -> None:
    text = r"""
// add(r, Severity::Invalid, "fake");
const char *s = "add(r, Severity::Invalid, \"fake\")";
/* add(r, Severity::Invalid, "fake"); */
add(r, Severity::Invalid, "real");
add(r, Severity::Info, "real");
add(r, variableSeverity, "unknown");
"""
    got = scan(text)
    assert got["invalid_count"] == 1, got
    assert got["total_add_calls"] == 3, got
    assert got["explicit_add_calls"] == 2, got
    assert len(got["nonexplicit_severity_lines"]) == 1, got
    assert got["invalid_lines"] == [5], got
    print("L5 scanner self-test PASS: comments/literals excluded, dynamic severity trapped")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--expected-invalid", type=int)
    parser.add_argument("--require-f3-f4", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        if args.source is None:
            return 0
    if args.source is None or args.expected_invalid is None:
        parser.error("--source and --expected-invalid are required without --self-test")
    source = args.source.read_text(encoding="utf-8")
    result = scan(source)
    errors = []
    if result["invalid_count"] != args.expected_invalid:
        errors.append("direct Invalid site count changed; re-audit EVERY new/removed site")
    if result["nonexplicit_severity_lines"]:
        errors.append("add(r,...) with nonexplicit severity; audit dynamic Invalid paths")
    if args.require_f3_f4:
        if not result["f3_native_ability_guard"]:
            errors.append("F3 native PK1/PK2 ability capability guard missing/changed")
        if not result["f4_native_gender_guard"]:
            errors.append("F4 native PK1 absent-gender capability guard missing/changed")
    result["passed"] = not errors
    result["errors"] = errors
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main())
