#!/usr/bin/env python3
"""Keep the G4-01 backend read-only and unreachable from mutation/device routing."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
source = (root / 'src/Integration/Gen4/Gen4ReadOnlySave.cpp').read_text()
entity = (root / 'src/Pokemon/Pokemon4ReadOnly.cpp').read_text()
assigned = (root / 'src/Integration/Gen4/Gen4AssignedSource.cpp').read_text()
for name, text in [('container', source), ('entity', entity), ('assigned source', assigned)]:
    assert not re.search(r'\b(fwrite|pwrite|write|rename|remove|unlink|serialize|encryptArray4)\s*\(', text), name
assert '"rb"' in assigned and not re.search(r'"(?:w|a|r\+)[b+]*"', assigned)
for path in ['src/UI/UI.cpp', 'src/Save/GetSaveFileContents.cpp']:
    assert 'Gen4' not in (root / path).read_text(), f'G4-01 is not device-routed: {path}'
route = (root / 'src/Conversion/RouteEvidence.cpp').read_text()
assert re.search(r'bool routeEnabledForTrueMove\([^)]*\) noexcept\s*\{\s*return false;\s*\}', route)
policy = (root / 'include/Safety/WritePolicy.h').read_text()
assert re.search(r'LIVE_SAVE_WRITES_ENABLED\s*=\s*false', policy)
print('G4-01 read-only/source/UI/True Move safety contract PASS')
