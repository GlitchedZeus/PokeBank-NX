#!/usr/bin/env python3
"""Compile and run Stage 2's bounded native Gen I-IV property suite.

The suite links the production save parsers, PK3 codec, and immutable PK4
adapter.  It uses only synthetic byte arrays and performs normal plus
ASan/UBSan runs without changing shared Makefiles or source saves.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEST = "tests/fixtures/legality_stage2_native_format_properties.cpp"
SOURCES = [
    TEST, "src/Encryption/Encryption3FRLG.cpp", "src/Encryption/Encryption4.cpp",
    "src/Pokemon/Pokemon3FRLG.cpp", "src/Pokemon/BaseStatsGen89.cpp",
    "src/Pokemon/PersonalInfoTable.cpp", "src/Pokemon/Experience.cpp",
    "src/Names/ItemNames.cpp", "src/Names/SpeciesNames.cpp", "src/Names/NatureNames.cpp",
    "src/Names/AbilityNames.cpp",
    "src/Pokemon/Pokemon4ReadOnly.cpp", "src/Pokemon/PersonalRecord4.cpp",
    "src/Pokemon/PersonalInfo4DP.cpp", "src/Pokemon/PersonalInfo4PT.cpp",
    "src/Pokemon/PersonalInfo4HGSS.cpp", "src/Integration/Gen1/Gen1ReadOnlySave.cpp",
    "src/Integration/Gen2/Gen2ReadOnlySave.cpp", "src/Utils/StringHelpers.cpp",
    "src/Utils/HelperUtilities.cpp",
]

def run(compiler: str, sanitize: bool) -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="pbnx-stage2-native-") as directory:
        executable = Path(directory) / "native_properties"
        command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   "-Wno-unused-parameter",
                   "-Iinclude", *SOURCES]
        # The production PK3 codec contains GCC-specific diagnostic pragmas,
        # and its shared implementation keeps one currently-unused helper.
        # These suppressions only make the independent Clang build portable;
        # they do not substitute or exclude any production source.
        if "clang" in Path(compiler).name:
            command += ["-Wno-unknown-warning-option", "-Wno-unused-function"]
        # The production PK3 graph is intentionally broad; O0 keeps the local
        # sanitizer build inside the bounded audit window without weakening
        # AddressSanitizer/UndefinedBehaviorSanitizer instrumentation.
        command += (["-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                    if sanitize else ["-O2"])
        command += ["-o", str(executable)]
        subprocess.run(command, cwd=ROOT, check=True)
        result = subprocess.run([str(executable)], cwd=ROOT, text=True,
                                capture_output=True, check=True,
                                env={"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                                     "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    expected = "Stage 2 native Gen I-IV boundary/property checks: 2139 PASS"
    if expected not in result.stdout:
        raise RuntimeError("unexpected property-suite output: " + result.stdout + result.stderr)
    return {"compiler": compiler, "sanitized": sanitize, "status": "PASS", "checks": 2139,
            "production_components": ["Gen1ReadOnlySave", "Gen2ReadOnlySave", "Encryption3FRLG",
                                      "Encryption4", "Pokemon3FRLG", "Pokemon4ReadOnly"],
            "stdout": result.stdout.strip()}

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--sanitize", action="store_true", help="run only ASan/UBSan")
    mode.add_argument("--normal", action="store_true", help="run only the optimized native build")
    parser.add_argument("--compiler", default="g++", help="C++ compiler executable (default: g++)")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    reports = ([run(args.compiler, True)] if args.sanitize else
               [run(args.compiler, False)] if args.normal else
               [run(args.compiler, False), run(args.compiler, True)])
    output = json.dumps({"schema": "POKEBANK_NX_STAGE2_NATIVE_FORMATS_V1", "runs": reports},
                        indent=2, sort_keys=True) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")

if __name__ == "__main__":
    main()
