#!/usr/bin/env python3
"""Build the Stage 3 cross-lane contract suite against the production analyzer."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCES = [
    "tests/fixtures/legality_stage3_cross_lane_contracts.cpp", "src/Legality/Legality.cpp",
    "src/Integration/Gen1/Gen1MoveCompatibility.cpp", "src/Integration/Gen2/Gen2MoveCompatibility.cpp",
    "src/Pokemon/Pokemon3FRLG.cpp", "src/Pokemon/Pokemon1ReadOnly.cpp", "src/Pokemon/Pokemon2ReadOnly.cpp",
    "src/Pokemon/PersonalRecord4.cpp", "src/Pokemon/PersonalInfo4DP.cpp", "src/Pokemon/PersonalInfo4PT.cpp",
    "src/Pokemon/PersonalInfo4HGSS.cpp", "src/Integration/Gen2/Gen2PersonalData.cpp",
    "src/Encryption/Encryption3FRLG.cpp", "src/Pokemon/BaseStatsGen89.cpp",
    "src/Pokemon/PersonalInfoTable.cpp", "src/Pokemon/Experience.cpp", "src/Pokemon/AbilityInfo.cpp",
    "src/Pokemon/FormInfo.cpp", "src/Pokemon/LearnsetTable.cpp", "src/Names/MoveNames.cpp",
    "src/Names/ItemNames.cpp", "src/Names/SpeciesNames.cpp", "src/Names/NatureNames.cpp",
    "src/Names/AbilityNames.cpp", "src/Utils/StringHelpers.cpp", "src/Utils/HelperUtilities.cpp",
    "src/Pokemon/Pokemon4ReadOnly.cpp", "src/Pokemon/Pokemon4ReadOnlyView.cpp", "src/Encryption/Encryption4.cpp",
]

def run(sanitize: bool) -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="pbnx-stage3-cross-lane-") as directory:
        executable = Path(directory) / "cross_lane"
        command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   "-Wno-unused-parameter", "-Iinclude", *SOURCES]
        # Keep this broad production link graph quick enough for a local
        # audit turn; optimization is not part of this correctness contract.
        command += (["-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                    if sanitize else ["-O0"])
        command += ["-o", str(executable)]
        subprocess.run(command, cwd=ROOT, check=True)
        result = subprocess.run([str(executable)], cwd=ROOT, text=True, capture_output=True,
                                check=True, env={"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                                                 "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    expected = "Stage 3 cross-lane contracts: 145 PASS"
    if expected not in result.stdout:
        raise RuntimeError("unexpected Stage 3 output: " + result.stdout + result.stderr)
    return {"status": "PASS", "sanitized": sanitize, "checks": 145,
            "actual_production_analyzer_executed": True, "stdout": result.stdout.strip()}

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--normal", action="store_true")
    mode.add_argument("--sanitize", action="store_true")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    reports = [run(False)] if args.normal else [run(True)] if args.sanitize else [run(False), run(True)]
    output = json.dumps({"schema": "POKEBANK_NX_STAGE3_CROSS_LANE_V1", "runs": reports},
                        indent=2, sort_keys=True) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")

if __name__ == "__main__":
    main()
