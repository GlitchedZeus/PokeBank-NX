#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> None:
    personal = load_module("gen_personal_contract", ROOT / "tools" / "gen_personal.py")
    learnsets = load_module("gen_learnsets_contract", ROOT / "tools" / "gen_learnsets.py")
    assets = load_module("check_device_assets_contract", ROOT / "tools" / "check_device_assets.py")

    personal_names = personal.load_species_names()
    learnset_names = learnsets.load_species_names()
    assert personal_names == learnset_names
    assert len(personal_names) > 1025
    assert personal_names[1] == "Bulbasaur"
    assert personal_names[25] == "Pikachu"

    gen1 = assets.gen1_species_names()
    assert gen1[1] == "Bulbasaur"
    assert gen1[25] == "Pikachu"
    assert gen1[1] != "Species 1"

    for path in (ROOT / "tools" / "gen_personal.py", ROOT / "tools" / "gen_learnsets.py"):
        text = path.read_text(encoding="utf-8")
        assert "SPECIES_NAMES_EN" in text
        assert "could not locate SPECIES_NAMES[] in" not in text

    print("generated species-name consumer contract: PASS")


if __name__ == "__main__":
    main()
