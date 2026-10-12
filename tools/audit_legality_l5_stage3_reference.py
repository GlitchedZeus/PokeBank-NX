#!/usr/bin/env python3
"""L5 Stage 3 independently executed, method-scoped PKHeX rule differential.

Compiles the independent C++ fixture under optimized GCC and GCC ASan+UBSan,
optionally Clang. Read-only, synthetic, never whole-Pokemon or retail parity.
Optional --pkhex-root / --nx-root verify exact inspected source blobs on disk.
The reference source is NOT copied into the generated binary.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests/fixtures/legality_l5_reference_differential.cpp"
PINNED_PKHEX = {
    "PKHeX.Core/Legality/Verifiers/EffortValueVerifier.cs":
        "86c9bc6ea0361821df86e1473788c228ce7506a0",
    "PKHeX.Core/PKM/Util/EffortValues.cs":
        "5904496c4e60df3eb421bdf4c387f9df08f130ad",
    "PKHeX.Core/PKM/PK2.cs":
        "fe6fea5c5a2ff82f2ace7ba9af289348c16a0df0",
    "PKHeX.Core/PKM/PK1.cs":
        "a4b692ae7045afb9d11ea3a3b6e25b94c0c4de8b",
    "PKHeX.Core/PKM/Shared/GBPKM.cs":
        "4e09e9f209c58c53de4437be1a876be57faf9ab0",
    "PKHeX.Core/PKM/Shared/G3PKM.cs":
        "1042c3edef50b4f2d16b164c56ef2a3461befc9f",
    "PKHeX.Core/PKM/Shared/G4PKM.cs":
        "c42aad87ec573ee59377db28a110a6403163de0c",
}
PINNED_NX = {
    "src/Legality/Legality.cpp": "2ee1624de1520630c54d69599834c8e29d589ffb",
}
OUTPUT = re.compile(
    r"^L5 Stage3 PINNED reference DIFFERENTIAL PASS: "
    r"(?P<total>[0-9]+) compared; agreement=(?P<agree>[0-9]+), "
    r"source-scope gaps=(?P<gaps>[0-9]+), "
    r"unresolved histories=(?P<unknown>[0-9]+); "
    r"assertions=(?P<assertions>[0-9]+) "
    r"\(method scoped, synthetic, no live PKHeX/full-analyzer execution\)$"
)
EXPECTED = dict(total=655450, agree=131860, gaps=392746,
                unknown=130844, assertions=655458)


def git_blob(file: Path) -> str:
    b = file.read_bytes()
    return hashlib.sha1(b"blob " + str(len(b)).encode("ascii") + b"\0" + b).hexdigest()


def check_blobs(root: Path, expected: dict[str, str]) -> None:
    for rel, sha in expected.items():
        file = root / rel
        if not file.is_file():
            raise FileNotFoundError("Required pinned source missing: " + str(file))
        actual = git_blob(file)
        if actual != sha:
            raise RuntimeError(f"Source drift in {rel}: {actual} != pinned {sha}")


def parse_native_output(stdout: str) -> dict[str, int]:
    match = OUTPUT.fullmatch(stdout.strip())
    if not match:
        raise RuntimeError("Native fixture did not produce its expected attestation")
    numbers = {key: int(value) for key, value in match.groupdict().items()}
    if numbers != EXPECTED:
        raise RuntimeError(f"Unexpected native reference counts: {numbers} != {EXPECTED}")
    if sum(numbers[name] for name in ("agree", "gaps", "unknown")) != numbers["total"]:
        raise RuntimeError("Differential classification accounting did not balance")
    return numbers


def run_native(compiler: str, sanitize: bool, exe: Path) -> dict:
    cxx = shutil.which(compiler)
    if cxx is None:
        raise RuntimeError(f"{compiler} not installed, cannot claim PASS")
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    if sanitize:
        flags += ["-O1", "-g", "-fsanitize=address,undefined",
                  "-fno-omit-frame-pointer"]
    else:
        flags += ["-O2"]
    subprocess.run([cxx, *flags, str(FIXTURE), "-o", str(exe)],
                   check=True, capture_output=True, text=True, cwd=ROOT)
    env = os.environ.copy()
    if sanitize:
        env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    result = subprocess.run([str(exe)], check=True, text=True,
                            capture_output=True, cwd=ROOT, env=env)
    return {"compiler": compiler, "sanitized": sanitize, "executed": True,
            "evidence": parse_native_output(result.stdout)}


def self_test() -> None:
    template = ("L5 Stage3 PINNED reference DIFFERENTIAL PASS: 655450 "
                "compared; agreement=131860, source-scope gaps=392746, "
                "unresolved histories=130844; assertions=655458 "
                "(method scoped, synthetic, no live PKHeX/full-analyzer execution)")
    assert parse_native_output(template) == EXPECTED
    for corrupted in (
        template.replace("655450", "655451"),
        template.replace("392746", "392745"),
        template + "\nFAKE",
        template.replace("PASS", "PENDING"),
    ):
        try:
            parse_native_output(corrupted)
        except RuntimeError:
            continue
        raise AssertionError("Invalid native evidence falsely accepted")
    print("L5 Stage3 differential runner self-test PASS (5 inputs)")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--pkhex-root", type=Path)
    ap.add_argument("--nx-root", type=Path)
    ap.add_argument("--require-both-pins", action="store_true")
    ap.add_argument("--clang", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--json-out", type=Path)
    args = ap.parse_args()
    if args.self_test:
        self_test()
        return
    if args.require_both_pins and (args.pkhex_root is None or args.nx_root is None):
        ap.error("--require-both-pins requires --pkhex-root and --nx-root")
    if not FIXTURE.is_file():
        raise FileNotFoundError(FIXTURE)
    if args.pkhex_root is not None:
        check_blobs(args.pkhex_root, PINNED_PKHEX)
    if args.nx_root is not None:
        check_blobs(args.nx_root, PINNED_NX)
    runs = []
    with tempfile.TemporaryDirectory(prefix="l5-stage3-diff-") as tmp:
        where = Path(tmp)
        runs.append(run_native("g++", False, where / "diff-gcc"))
        runs.append(run_native("g++", True, where / "diff-sanitized"))
        if args.clang:
            runs.append(run_native("clang++", False, where / "diff-clang"))
    evidence = {
        "status": "PASS_SYNTHETIC_METHOD_SCOPED",
        "runs": runs,
        "pkhex_revision": "6501f0ab46e8f8ca048539dbaf8cae8cb104e722",
        "pkhex_exact_source_pins_verified": args.pkhex_root is not None,
        "nx_exact_source_pins_verified": args.nx_root is not None,
        "production_analyzer_executed": False,
        "pkhex_executable_executed": False,
        "retail_authenticated_originals": 0,
        "pkhex_whole_pokemon_parity_certificates": 0,
        "historical_unknown_is_invalid": False,
        "release_ready": False,
    }
    encoded = json.dumps(evidence, indent=2, sort_keys=True) + "\n"
    print(encoded, end="")
    if args.json_out:
        args.json_out.write_text(encoded, encoding="utf-8")


if __name__ == "__main__":
    main()
