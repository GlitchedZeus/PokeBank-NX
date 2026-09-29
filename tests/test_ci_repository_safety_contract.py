#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"
RETIRED_IMPORT = WORKFLOWS / "import-pkse.yml"

def fail(message: str) -> None:
    raise SystemExit(message)

if RETIRED_IMPORT.exists():
    fail("AUDIT-001: retired destructive PKSE import workflow must not be tracked")

for path in sorted(list(WORKFLOWS.glob("*.yml")) + list(WORKFLOWS.glob("*.yaml"))):
    text = path.read_text(encoding="utf-8")

    writable = bool(re.search(r"(?m)^\s*contents:\s*write\s*$", text))
    root_overlay = bool(re.search(
        r"(?m)^\s*git\s+checkout\s+[^\n]+\s+--\s+\.\s*$", text
    ))
    direct_main_push = bool(re.search(
        r"(?m)^\s*git\s+push\s+origin\s+HEAD:main(?:\s|$)", text
    ))
    mutable_pkse_master = (
        "github.com/kiasta/PKSE.git" in text and
        bool(re.search(r"(?m)^\s*git\s+fetch\s+pkse\s+master(?:\s|$)", text))
    )

    if writable and root_overlay and direct_main_push:
        fail(
            f"AUDIT-001: {path.relative_to(ROOT)} can overlay a tree onto the repository "
            "and push it directly to main with contents: write"
        )
    if writable and mutable_pkse_master and direct_main_push:
        fail(
            f"AUDIT-001: {path.relative_to(ROOT)} can import mutable PKSE master "
            "and push it directly to main"
        )


NATIVE_GATE = WORKFLOWS / "native-pr-build.yml"
if not NATIVE_GATE.is_file():
    fail("AUDIT-002: unfiltered native PR gate is missing")

native_text = NATIVE_GATE.read_text(encoding="utf-8")
trigger_block = native_text.split("permissions:", 1)[0]
if not re.search(r"(?m)^\s*pull_request:\s*$", trigger_block):
    fail("AUDIT-002: native PR gate must run on pull_request")
if re.search(r"(?m)^\s+branches:\s*$", trigger_block):
    fail("AUDIT-002: native PR gate must not be branch-filtered")
if re.search(r"(?m)^\s+paths:\s*$", trigger_block):
    fail("AUDIT-002: native PR gate must not be path-filtered")
for required in (
    "devkitpro/devkita64:",
    "make -j1",
    "test -s PokeBankNX.elf",
    "test -s PokeBankNX.nro",
):
    if required not in native_text:
        fail(f"AUDIT-002: native PR gate is missing required compile/link contract: {required}")


HOST_WORKFLOW = WORKFLOWS / "host-tests.yml"
if not HOST_WORKFLOW.is_file():
    fail("AUDIT-006: host test workflow is missing")

host_text = HOST_WORKFLOW.read_text(encoding="utf-8")
if not re.search(r"(?m)^\s*ASAN_OPTIONS:\s*detect_leaks=1\s*$", host_text):
    fail("AUDIT-006: unrestricted CI must enable LeakSanitizer")
if 'ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"' not in host_text:
    fail("AUDIT-006: focused sanitizer regression must inherit the CI leak policy")

make_text = (ROOT / "Makefile.host.base").read_text(encoding="utf-8")
if "ASAN_OPTIONS ?= detect_leaks=0" not in make_text:
    fail("AUDIT-006: constrained local default must remain explicit")
if 'ASAN_OPTIONS="$(ASAN_OPTIONS)" ./$test_bin' not in make_text:
    fail("AUDIT-006: sanitizer loop must use the configured ASAN_OPTIONS value")


PINNED_DEVKITA64 = "devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282"
for workflow_name in ("native-pr-build.yml", "product-ui-native.yml"):
    workflow = WORKFLOWS / workflow_name
    if not workflow.is_file():
        fail(f"AUDIT-003: required native workflow missing: {workflow_name}")
    workflow_text = workflow.read_text(encoding="utf-8")
    if "devkitpro/devkita64:latest" in workflow_text:
        fail(f"AUDIT-003: {workflow_name} still uses mutable devkita64:latest")
    if PINNED_DEVKITA64 not in workflow_text:
        fail(f"AUDIT-003: {workflow_name} does not use the tested immutable devkitA64 digest")


standalone_text = (ROOT / "docs" / "STANDALONE_RUNTIME.md").read_text(encoding="utf-8")
if "RetroArch is not invoked as a helper" in standalone_text:
    fail("AUDIT-041: standalone runtime docs still deny the supported RetroArch launch path")
for required in ("READ-ONLY SAVE SOURCE", "OPTIONAL USER-INVOKED GAME-LAUNCH TARGET",
                 "launching does not change that boundary"):
    if required not in standalone_text:
        fail(f"AUDIT-041: standalone runtime classification missing: {required}")

print("CI repository safety contracts: PASS")
