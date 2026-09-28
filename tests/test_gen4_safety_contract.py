#!/usr/bin/env python3
"""Keep visible G4-02 routing read-only while permitting only the presentation bridge."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
source = (root / 'src/Integration/Gen4/Gen4ReadOnlySave.cpp').read_text()
entity = (root / 'src/Pokemon/Pokemon4ReadOnly.cpp').read_text()
assigned = (root / 'src/Integration/Gen4/Gen4AssignedSource.cpp').read_text()
discovery = (root / 'src/Integration/Gen4/Gen4SourceDiscovery.cpp').read_text()
for name, text in [('container', source), ('entity', entity), ('assigned source', assigned),
                   ('source discovery/adapter', discovery)]:
    assert not re.search(r'\b(fwrite|pwrite|write|rename|remove|unlink|serialize|encryptArray4)\s*\(', text), name
assert '"rb"' in discovery
io_source = assigned + "\n" + discovery
assert not re.search(r'fopen\s*\([^,]+,\s*"(?:w|a|r\+)[b+]*"', io_source)
assert not re.search(r'freopen\s*\(', io_source)
ui = (root / 'src/UI/UI.cpp').read_text()
assert 'handleGen4View' in ui
assert 'SourceKind::ExternalLegacy' in ui
select = (root / 'src/UI/SaveSelectScreen.cpp').read_text()
assert 'Gen4AssignedFile' in select and 'discoverKnownSources' in select

# Gen IV game cards must never auto-open the remembered adapter source. A always enters a
# validated Save Instances chooser, whose rows retain provider identity for future emulator roots.
current_title = select[select.index('void SaveSelectScreen::selectCurrentTitle()'):
                       select.index('void SaveSelectScreen::selectCurrentLegacyInstance()')]
gen4_branch = current_title[current_title.index('SelectedSourceKind::Gen4AssignedFile'):]
gen4_branch = gen4_branch[:gen4_branch.index('selectedUserUid')]
assert 'discoverGen4Candidates();' in gen4_branch
assert 'openAssignedSource(' not in gen4_branch
assert 'titleSelected = true' not in gen4_branch

discover = select[select.index('void SaveSelectScreen::discoverGen4Candidates()'):
                  select.index('bool SaveSelectScreen::assignGen4Candidate')]
assert 'resolveFileForGame' in discover
assert 'inspectSourceFile' in discover
assert 'existing.sourceIdentity == candidate.sourceIdentity' in discover
assert 'a.modifiedTime > b.modifiedTime' in discover

instances = select[select.index('} else if (overlay == Overlay::Gen4Candidates && u) {'):
                   select.index('} else if (overlay == Overlay::Help)')]
assert 'NINTENDO DS / SAVE INSTANCES / READ ONLY' in instances
assert ' — Save Instances' in instances
assert 'candidate.sourceType' in instances
assert 'MOST RECENTLY MODIFIED' in instances
assert '{"A", "Open Read Only"}' in instances
assert '{"Y", "Source Setup"}' in instances
assert '{"X", "Refresh Saves"}' in instances
bridge = (root / 'src/Pokemon/Pokemon4ReadOnlyView.cpp').read_text()
assert not re.search(r'\b(fwrite|pwrite|rename|remove|unlink)\s*\(', bridge)
assert 'clone() const override { return nullptr; }' in (root / 'include/Pokemon/Pokemon4ReadOnlyView.h').read_text()
assert 'Gen4ReadOnlyTrainer' in (root / 'src/Legacy/Gen4ReadOnlyTrainer.cpp').read_text()
save_contents = (root / 'src/Save/GetSaveFileContents.cpp').read_text()
assert 'Gen4' not in save_contents, 'Gen IV external saves must not enter installed-save write routing'
route = (root / 'src/Conversion/RouteEvidence.cpp').read_text()
assert re.search(r'bool routeEnabledForTrueMove\([^)]*\) noexcept\s*\{\s*return false;\s*\}', route)
policy = (root / 'include/Safety/WritePolicy.h').read_text()
assert re.search(r'LIVE_SAVE_WRITES_ENABLED\s*=\s*false', policy)
print('G4-02 visible read-only source/UI/True Move safety contract PASS')
