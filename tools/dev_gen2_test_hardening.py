from pathlib import Path

p = Path('tests/test_gsc_pokemon_editor.cpp')
s = p.read_text()

# Keep the deterministic fixture readable under the project's -Werror policy.
s = s.replace(
    "    if(c==' ')return 0x7F;if(c=='-')return 0xE3;if(c=='!')return 0xE7;\n",
    "    if(c==' ')return 0x7F;\n    if(c=='-')return 0xE3;\n    if(c=='!')return 0xE7;\n")

needle = '    BoxPokemonEdit bad;bad.nickname="ABCDEFGHIJK";assert(!editor->stageBoxPokemonEdit(0,0,bad,error));\n'
extra = '''    BoxPokemonEdit bad;bad.nickname="ABCDEFGHIJK";assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.species=252;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
'''
if 'bad.species=252' not in s:
    if needle not in s: raise SystemExit('invalid-edit insertion marker missing')
    s = s.replace(needle, extra, 1)

needle = '    bad={};bad.level=0;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));\n'
extra = '''    bad={};bad.level=0;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.level=101;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
'''
if 'bad.level=101' not in s:
    if needle not in s: raise SystemExit('level upper-bound insertion marker missing')
    s = s.replace(needle, extra, 1)

needle = '''    BoxPokemonEdit genderGlyph;genderGlyph.nickname="NIDORAN\\xE2\\x99\\x80";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    auto glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\\xE2\\x99\\x80");
'''
extra = needle + '''    genderGlyph.nickname="NIDORAN\\xE2\\x99\\x82";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\\xE2\\x99\\x82");
'''
if 'genderGlyph.nickname="NIDORAN\\xE2\\x99\\x82"' not in s:
    if needle not in s: raise SystemExit('gender-glyph insertion marker missing')
    s = s.replace(needle, extra, 1)

p.write_text(s)

p = Path('tests/test_machine_display.cpp')
s = p.read_text()
needle = '''    assert(Names::getTMMove(GameVersion::FRLG, 289) == 264);
    assert(Names::getTMMove(GameVersion::FRLG, 289) != Names::getTMMove(GameVersion::GSC, 191));
'''
extra = needle + '''    auto frLeaf = Names::getMachineDescriptor(GameVersion::FR, 289);
    assert(frLeaf && frLeaf.kind == MachineKind::TM && frLeaf.number == 1 && frLeaf.moveId == 264);
    auto swLeaf = Names::getMachineDescriptor(GameVersion::SW, 1145);
    assert(swLeaf && swLeaf.kind == MachineKind::TR && swLeaf.number == 15 && swLeaf.moveId == 126);
'''
if 'auto frLeaf =' not in s:
    if needle not in s: raise SystemExit('leaf-game insertion marker missing')
    s = s.replace(needle, extra, 1)
p.write_text(s)
