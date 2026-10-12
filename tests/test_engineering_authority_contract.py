#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

authority = read("docs/ENGINEERING_AUTHORITY.md")
for required in (
    "GitHub is authoritative",
    "PR #92",
    "PR #101",
    "143c5e5c341d4f85af30e013808a37d6719560fe",
    "Cross-game True Move and source injection remain locked",
):
    assert required in authority, required

historical = {
    "docs/CODEX_SESSION.md": "HISTORICAL SESSION DISCIPLINE",
    "docs/NEXT_CODEX_PROMPT.md": "HISTORICAL PROMPT",
    "docs/SESSION_RUNBOOK.md": "HISTORICAL RUNBOOK",
}
for path, marker in historical.items():
    text = read(path)
    assert marker in text
    assert "docs/ENGINEERING_AUTHORITY.md" in text

for path in (
    "docs/PROJECT_RESOURCE_INDEX.md",
    "docs/RESEARCH_CURRENT_INDEX.md",
    "docs/NEXT_SESSION_PLAN.md",
    "docs/RECOVERY_CONTRACT.md",
    "docs/V1_ROADMAP.md",
    "docs/UPSTREAM_AUDIT.md",
    "docs/RETROARCH_SOURCE_NAMING.md",
    "docs/UI_FLOW.md",
    "docs/UI_STYLE_GUIDE.md",
    "docs/UI_OWNERSHIP_STATUS.md",
):
    assert "docs/ENGINEERING_AUTHORITY.md" in read(path), path

assert "DO NOT EXECUTE AS CURRENT WORK" in read("docs/NEXT_CODEX_PROMPT.md")
assert "Recovery is branch-agnostic" in read("docs/RECOVERY_CONTRACT.md")

print("engineering authority/docs routing contract: PASS")
