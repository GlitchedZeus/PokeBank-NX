#!/usr/bin/env python3
from pathlib import Path

p = Path('tests/test_game_hub_contract.py')
text = p.read_text(encoding='utf-8')
old = '''require("Quick Games is selection/navigation only" in drawer_update and
        "refreshHubSelectionFromCache();" in drawer_update and
        "refreshHubPreview();" not in drawer_update,
        "Quick Games A must close from cached state without synchronous save/launch discovery")'''
new = '''require("refreshHubSelectionFromCache();" in drawer_update and
        "refreshHubPreview(false);" in drawer_update and
        "refreshHubPreview();" not in drawer_update,
        "Quick Games A must hydrate selected save presentation without launch discovery")'''
if text.count(old) != 1:
    raise SystemExit(f'tests/test_game_hub_contract.py: expected one Quick Games contract, found {text.count(old)}')
p.write_text(text.replace(old, new, 1), encoding='utf-8')
