#!/usr/bin/env python3
"""Broken-path controls for Round 4 source guards that follow shared presenters."""
from dataclasses import replace
from pathlib import Path
import test_qa_cross_gen_contract as qa
root=Path(__file__).resolve().parent.parent
sources,_=qa.load_sources(root)

def check(items,cid):
    return next(r for r in qa.contracts(items) if r.cid.startswith(cid)).violations

def mutate(path,old,new):
    found=False
    result=[]
    for source in sources:
        if source.rel==path:
            assert old in source.code,(path,old)
            source=replace(source,code=source.code.replace(old,new));found=True
        result.append(source)
    assert found,path
    return result

baseline={r.cid[:4]:r.violations for r in qa.contracts(sources)}
for cid in ('CG08','CG33','CG35','CG40','CG41','CG42','CG45','CG46','CG48','CG49','CG53','CG54'):
    assert not baseline[cid],(cid,baseline[cid])
assert check(mutate('src/UI/SharedPokemonEditorSurface.inc',', 20, refilter)',', 20, nullptr)'), 'CG40')
assert check(mutate('src/UI/Gen2UnifiedPokemonWorkspace.inc','Selector::nextOtGender(','Selector::missingGender('),'CG41')
assert check(mutate('src/UI/SharedPokemonEditorSurface.inc','SharedScrollbar::kTouchSlop + 4','SharedScrollbar::kTouchSlop - 1'),'CG42')
assert check(mutate('src/UI/Gen4SharedPokemonSurface.inc',
    'std::string(Names::getSpeciesName(p.species())) + " #" + std::to_string(p.species())',
    'std::to_string(p.species()) + " - " + Names::getSpeciesName(p.species())'),'CG45')
assert check(mutate('src/UI/Gen4SharedPokemonSurface.inc','"Unavailable ability"','"Ability Slot " + std::to_string(value + 1)'),'CG46')
assert check(mutate('src/UI/Gen5SharedReadOnlySurface.inc','PokeBank::UIModel::SharedEditorPresentation::legalityLabel(\n                PokeBank::UIModel::SharedEditorPresentation::LegalityState::Incomplete)','"PASS"'),'CG48')
assert check(mutate('include/UI/SharedEditorChrome.h','drawOriginLegalityPanel(','missingOriginPanel('),'CG49')
assert check(mutate('include/UI/SharedEditorChrome.h','drawNavBar(fb,{{"B","Back"}});',';'),'CG08')
assert check(mutate('src/UI/Gen4SharedPokemonSurface.inc','SharedEditorChrome::drawReviewPanel(','SharedEditorChrome::missingReviewPanel('),'CG35')
assert check(mutate('include/UI/SharedReviewPresentation.h','out.push("X","Export Copy")','out.push("X","Wrong export")'),'CG35')
assert check(mutate('include/UI/SharedEditorChrome.h','beginModal(buttons);',';'),'CG35')
assert check(mutate('src/UI/Gen4SharedPokemonSurface.inc','Surface::openContextualMetLocationPicker(',
    'Surface::missingContextualPicker('),'CG53')
assert check(mutate('src/UI/Gen5SharedReadOnlySurface.inc','state.pickerMetContext.detail(value,',
    'std::to_string(p->metLevel()) + ignored(value,'),'CG53')
assert check(mutate('include/Integration/Encounter/EncounterContext45.h',
    'for (const auto& entry : kWild45)','for (const auto& entry : missingData)'),'CG53')
assert check(mutate('src/UI/SharedPokemonEditorSurface.inc',
    'state.pickerMetContext.choices(state.pickerOtherHistories)',
    'state.pickerMetContext.choices(false)'),'CG53')
assert check(mutate('src/UI/TrainerViewScreenBase.inc','ClassicPartyActions::open(','ClassicPartyActions::missingOpen('),'CG33')
assert check(mutate('src/UI/TrainerViewScreenCompositeOverlay.cpp','ClassicPartyActions::handle(*this',
    'ClassicPartyActions::missingHandle(*this'),'CG33')
assert check(mutate('src/UI/ClassicPartyActions.inc','actionsForSlot(true,{false,false,true},false)',
    'actionsForSlot(false,{false,false,false},false)'),'CG33')
assert check(mutate('include/UI/Gen2PokemonSession.h','editor.stagePartyPokemonEdit(',
    'editor.stageBoxPokemonEdit('),'CG33')
assert check(mutate('include/Integration/Gen5/Gen5SharedEditorBridge.h',
    'return session.stage(NativeField::Language,stat,value,error);',
    'return session.stage(NativeField::Friendship,stat,value,error);'),'CG54')
print('Round 4 shared source contracts: real paths pass; twenty broken-path controls fail PASS')
