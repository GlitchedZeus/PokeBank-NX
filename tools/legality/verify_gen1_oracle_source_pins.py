#!/usr/bin/env python3
"""Fail closed on stale or duplicate local source blobs in the Gen I oracle CI.

The existing workflow pins external accepted PKSM/PKHeX checkout revisions,
and its own native analyzer and fixture blobs. This checker independently
validates every LOCAL blob pin at the actual checked-out coordinator SHA.
External source pins remain verified by the original CI steps.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import tempfile

PIN = re.compile(
    r'^\s*test\s+"\$\(git hash-object ([^\s)"\\]+)\)"\s*=\s*([a-f0-9]{40})\s*$'
)
COORDINATOR_PREFIX = "_l5_gen1_coordinator/"
EXTERNAL_PREFIXES = ("_l5_gen1_accepted/", "_l5_gen1_pkhex/")


class PinError(RuntimeError):
    pass


def verify(workflow: Path, root: Path, min_local_pins: int = 8) -> tuple[int, int]:
    seen: dict[str, int] = {}
    local_count = 0
    external_count = 0
    for line_number, line in enumerate(
        workflow.read_text(encoding="utf-8").splitlines(), 1
    ):
        match = PIN.fullmatch(line)
        if match is None:
            continue
        source, expected = match.groups()
        if source in seen:
            raise PinError(
                f"duplicate source SHA pin for {source}: "
                f"lines {seen[source]} and {line_number}"
            )
        seen[source] = line_number
        if source.startswith(EXTERNAL_PREFIXES):
            external_count += 1
            continue
        if source.startswith(COORDINATOR_PREFIX):
            relative = source[len(COORDINATOR_PREFIX):]
        else:
            if source.startswith("_l5_"):
                raise PinError(
                    f"unknown/exempted source checkout at line {line_number}: {source}"
                )
            relative = source
        relative_path = Path(relative)
        if relative_path.is_absolute() or ".." in relative_path.parts:
            raise PinError(f"unsafe source SHA path at line {line_number}: {source}")
        path = root / relative_path
        if not path.is_file():
            raise PinError(
                f"missing pinned original Gen I source at line {line_number}: {source}"
            )
        actual = subprocess.check_output(
            ["git", "hash-object", str(path)], text=True
        ).strip()
        if actual != expected:
            raise PinError(
                f"stale original Gen I SHA pin at line {line_number}: {source}: "
                f"workflow={expected}, exact-checkout={actual}"
            )
        local_count += 1
    if local_count < min_local_pins:
        raise PinError(
            f"too few local source pins ({local_count} < {min_local_pins}); gate truncated?"
        )
    return local_count, external_count


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="pbnx-gen1-pin-") as tmp:
        root = Path(tmp)
        (root / "src").mkdir()
        (root / "tests").mkdir()
        (root / "src/file.cpp").write_text("// original\n")
        (root / "tests/fixture.cpp").write_text("int main(){}\n")
        h1 = subprocess.check_output(
            ["git", "hash-object", str(root / "src/file.cpp")], text=True
        ).strip()
        h2 = subprocess.check_output(
            ["git", "hash-object", str(root / "tests/fixture.cpp")], text=True
        ).strip()
        line1 = (
            f'          test "$(git hash-object _l5_gen1_coordinator/src/file.cpp)" = {h1}'
        )
        line2 = f'          test "$(git hash-object tests/fixture.cpp)" = {h2}'
        external = (
            f'          test "$(git hash-object _l5_gen1_pkhex/PKHeX.Core/PKM/PK1.cs)" = {h1}'
        )
        workflow = root / "workflow.yml"
        workflow.write_text("\n".join([line1, line2, external]) + "\n")
        assert verify(workflow, root, 2) == (2, 1)
        workflow.write_text("\n".join([line1, line1, line2]) + "\n")
        try:
            verify(workflow, root, 2)
            raise AssertionError("duplicate source pins must be rejected")
        except PinError as exc:
            assert "duplicate" in str(exc)
        workflow.write_text("\n".join([line1, line2]) + "\n")
        (root / "tests/fixture.cpp").write_text("// changed\n")
        try:
            verify(workflow, root, 2)
            raise AssertionError("stale source pin must be rejected")
        except PinError as exc:
            assert "stale" in str(exc)
        workflow.write_text("\n".join([line1, external]) + "\n")
        try:
            verify(workflow, root, 2)
            raise AssertionError("incomplete source gate must be rejected")
        except PinError as exc:
            assert "too few" in str(exc)
        workflow.write_text("\n".join([line1, line2]) + "\n")
        (root / "tests/fixture.cpp").unlink()
        try:
            verify(workflow, root, 2)
            raise AssertionError("missing pinned source must be rejected")
        except PinError as exc:
            assert "missing" in str(exc)
    print(
        "Gen I original oracle pin checker self-tests: "
        "normal, duplicate, stale, missing, truncated: PASS"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--workflow", type=Path,
        default=Path(".github/workflows/legality-gen1-exact-pkhex-oracle.yml")
    )
    parser.add_argument("--repo-root", type=Path, default=Path("."))
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    local, external = verify(args.workflow, args.repo_root)
    print(
        f"Gen I original PKHeX oracle SHA preflight: {local} local sources match, "
        f"{external} external pins still enforced by workflow: PASS"
    )


if __name__ == "__main__":
    try:
        main()
    except PinError as exc:
        raise SystemExit(str(exc)) from exc
