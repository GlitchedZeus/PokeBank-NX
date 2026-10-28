#!/usr/bin/env python3
"""Negative controls for the reviewed backup/carry source-contract adaptations."""
from dataclasses import replace
from pathlib import Path
import test_qa_cross_gen_contract as qa

root = Path(__file__).resolve().parent.parent
sources, _ = qa.load_sources(root)

def result(items, cid):
    return next(r for r in qa.round4_p0_contracts(items) if r.cid.startswith(cid))

def mutated(path, old, new):
    output = []
    found = False
    for src in sources:
        if src.rel == path:
            assert old in src.code, (path, old)
            output.append(replace(src, code=src.code.replace(old, new)))
            found = True
        else:
            output.append(src)
    assert found
    return output

assert not result(sources, 'CG59').violations
assert not result(sources, 'CG60').violations
assert not result(sources, 'CG56').violations
assert result(mutated('src/UI/SaveSelectScreen.cpp', 'SB::createBackup(', 'SB::missingCopy('), 'CG59').violations
assert result(mutated('src/UI/UI.cpp', 'const std::string backupPath = openWorkingCopy(',
                      'const std::string backupPath = backupSaveData('), 'CG60').violations
assert result(mutated('src/Legacy/GSCReadOnlyTrainer.cpp', 'stagedEditor_->presentationBytes(error)',
                      'stagedEditor_->finalizedBytes(error)'), 'CG56').violations
assert result(mutated('src/Integration/Gen2/Gen2StagedEditor.cpp', 'packedMove_.active',
                      'false'), 'CG56').violations
print('Round 4 source contracts: real backup/carry paths pass; four broken-path controls fail PASS')
