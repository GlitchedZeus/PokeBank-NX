#!/usr/bin/env python3
"""Cheap CI check that the GitHub recovery recipe has not drifted apart."""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STATE_PATH = ROOT / "recovery" / "RECOVERY_STATE.json"
GENERATOR = ROOT / "tools" / "gen_hdsprites.py"
RECOVER = ROOT / "tools" / "recover_workspace.py"
PACKER = ROOT / "tools" / "pack_recovery_snapshot.py"
CONTRACT = ROOT / "docs" / "RECOVERY_CONTRACT.md"
SNAPSHOT_README = ROOT / "recovery" / "assets_snapshot" / "README.md"


def fail(message: str) -> None:
    print(f"RECOVERY CONTRACT: FAIL — {message}")
    raise SystemExit(1)


def main() -> int:
    for path in (STATE_PATH, GENERATOR, RECOVER, PACKER, CONTRACT, SNAPSHOT_README):
        if not path.is_file():
            fail(f"missing tracked recovery file: {path.relative_to(ROOT)}")

    try:
        state = json.loads(STATE_PATH.read_text(encoding="utf-8"))
    except Exception as exc:
        fail(f"invalid recovery state JSON: {exc}")

    text = GENERATOR.read_text(encoding="utf-8")
    match = re.search(r'^PINNED_REF\s*=\s*[\"\']([^\"\']+)[\"\']', text, re.MULTILINE)
    if not match:
        fail("gen_hdsprites.py has no PINNED_REF")

    generator_ref = match.group(1)
    state_ref = state.get("pokemon_hd", {}).get("pinned_pokeapi_sprites_ref")
    if generator_ref != state_ref:
        fail(f"sprite pin mismatch: generator={generator_ref}, recovery state={state_ref}")

    if state.get("pokemon_hd", {}).get("expected_png_count") != 3260:
        fail("expected HD render count is not the verified 3260 baseline")
    if state.get("pokemon_hd", {}).get("expected_base_species") != 1025:
        fail("expected base-species count is not 1025")
    if state.get("type_icons", {}).get("expected_count") != 18:
        fail("expected type-icon count is not 18")
    if len(state.get("fonts", {}).get("expected", [])) != 3:
        fail("expected font count is not 3")

    snapshot = state.get("full_romfs_snapshot", {})
    if snapshot.get("manifest") != "recovery/assets_snapshot/manifest.json":
        fail("full RomFS snapshot manifest path drifted")
    if snapshot.get("packer") != "tools/pack_recovery_snapshot.py":
        fail("full RomFS snapshot packer path drifted")

    for name in state.get("game_cards", {}).get("expected", []):
        source = ROOT / state["game_cards"]["tracked_source"] / name
        if not source.is_file():
            fail(f"tracked game-card source missing: {source.relative_to(ROOT)}")

    if state.get("recovery_command") != "python3 tools/recover_workspace.py":
        fail("unexpected recovery command in recovery state")

    print("RECOVERY CONTRACT: PASS")
    print(f"sprite ref: {generator_ref}")
    print("verified baseline: 3260 HD / 1025 base / 18 types / 3 fonts")
    print(f"snapshot status: {snapshot.get('status', 'unknown')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
