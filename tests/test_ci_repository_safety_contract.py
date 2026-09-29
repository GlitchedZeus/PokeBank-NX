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

print("CI repository safety contract: PASS")
