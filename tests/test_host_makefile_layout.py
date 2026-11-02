#!/usr/bin/env python3
"""Guard against host test Makefile sprawl or accidental deletion during consolidation."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HOST = ROOT / "build-system" / "host"
GROUPS = {
    "core.mk": ["base"],
    "classic.mk": ["gsc", "editor", "cleanup3", "standalone", "shared-editor", "hardware-fixes"],
    "generations.mk": ["gen4", "gen5"],
    "sources.mk": ["source-instances", "product-ui", "exports", "home-providers"],
    "regressions.mk": ["gen14", "qa", "item-quantity-policy", "pickers", "reroll-mechanics", "species-change-moves", "round4-r", "round4-codex"],
    "ui.mk": ["host-ui", "review-entry", "pkse-upstream", "multilingual"],
    "pokemon-files.mk": ["pkse-october", "pkse-october-pid", "pkfile-gen12", "pkfile-gen3", "pkfile-gen4", "pkfile-gen5"],
    "localization.mk": ["pokemon-file-ui", "recovery-view", "jp-gen12", "jp-gen1", "jp-gen2"],
    "inventory.mk": ["bag-top-up", "pokemon-file-menus", "machine"],
    "deps.mk": ["deps"],
}

root_makefiles = sorted(path.name for path in ROOT.glob("Makefile*") if path.is_file())
assert root_makefiles == ["Makefile", "Makefile.host"], root_makefiles

index = (ROOT / "Makefile.host").read_text(encoding="utf-8")
includes = re.findall(r"(?m)^include ([^\s]+)\s*$", index)
expected_includes = [f"build-system/host/{group}" for group in GROUPS]
assert includes == expected_includes, (includes, expected_includes)

seen = []
for group, fragments in GROUPS.items():
    source = (HOST / group).read_text(encoding="utf-8")
    names = re.findall(r"(?m)^# ---- BEGIN original Makefile\.host\.([^ ]+) ----$", source)
    ends = re.findall(r"(?m)^# ---- END original Makefile\.host\.([^ ]+) ----$", source)
    assert names == fragments == ends, (group, names, ends, fragments)
    for name in fragments:
        begin = f"# ---- BEGIN original Makefile.host.{name} ----\n"
        end = f"\n# ---- END original Makefile.host.{name} ----"
        body = source.split(begin, 1)[1].split(end, 1)[0]
        assert len(body.strip()) >= 100, f"{group}: empty/short source for {name}"
    seen.extend(names)
assert len(seen) == len(set(seen)) == 40, f"Expected all 40 host fragments once, got {len(seen)}"
assert "HOST_TESTS" in (HOST / "core.mk").read_text(encoding="utf-8")
assert "test_gen5_pk5_readonly" in (HOST / "generations.mk").read_text(encoding="utf-8")
assert "test_machine_display" in (HOST / "inventory.mk").read_text(encoding="utf-8")
assert "override CXX" in (HOST / "deps.mk").read_text(encoding="utf-8")

classic = (HOST / "standalone" / "classic_inventory.mk").read_text(encoding="utf-8")
create = (HOST / "standalone" / "gen5-create.mk").read_text(encoding="utf-8")
assert "host-classic-inventory" in classic
assert "test_gen5_native_create.cpp" in create
assert "standalone/" not in index, "Standalones must not accidentally join the shared host suite"
print("Host Makefile layout: PASS (2 root, 40 fragments in 10 ordered modules, 2 standalone)")
