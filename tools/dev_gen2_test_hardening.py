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

# Permanent one-field-at-a-time semantic round-trip matrix. Each edit starts from a fresh
# strict fixture so a compound edit cannot mask a field-specific serializer/reparse defect.
marker = '    // Per-field semantic round-trip regression matrix.\n'
if marker not in s:
    anchor = '    assert(raw==source);assert(std::equal(parsed.save->sourceBytes().begin(),parsed.save->sourceBytes().end(),source.begin()));\n\n'
    if anchor not in s: raise SystemExit('per-field insertion marker missing')
    block = r'''    // Per-field semantic round-trip regression matrix.
    auto assertSamePokemon = [](const PokemonRecord& a, const PokemonRecord& b) {
        assert(a.species==b.species&&a.heldItem==b.heldItem&&a.moves==b.moves);
        assert(a.trainerId==b.trainerId&&a.experience==b.experience&&a.statExperience==b.statExperience);
        assert(a.dvs==b.dvs&&a.pp==b.pp&&a.ppUps==b.ppUps);
        assert(a.friendship==b.friendship&&a.pokerus==b.pokerus&&a.caughtData==b.caughtData);
        assert(a.level==b.level&&a.originalTrainer==b.originalTrainer&&a.nickname==b.nickname);
        assert(a.isEgg==b.isEgg&&a.shiny==b.shiny&&a.gender==b.gender);
    };
    auto singleFieldRoundTrip = [&](const BoxPokemonEdit& oneEdit, auto verify) {
        auto oneRaw=fixture(l,true);const auto oneSource=oneRaw;
        auto oneParsed=parse(oneRaw,game);assert(oneParsed);
        std::string oneError;
        auto oneEditor=StagedEditor::create(*oneParsed.save,oneError);assert(oneEditor&&oneError.empty());
        auto oneBefore=oneEditor->boxedPokemon(0,0,oneError);assert(oneBefore);
        assert(oneEditor->stageBoxPokemonEdit(0,0,oneEdit,oneError));
        auto oneAfter=oneEditor->boxedPokemon(0,0,oneError);assert(oneAfter);
        verify(*oneBefore,*oneAfter);
        auto oneOut=oneEditor->finalizedBytes(oneError);assert(!oneOut.empty()&&oneError.empty());
        auto oneReload=parse(oneOut,game);assert(oneReload);
        auto oneReloadEditor=StagedEditor::create(*oneReload.save,oneError);assert(oneReloadEditor&&oneError.empty());
        auto oneReparsed=oneReloadEditor->boxedPokemon(0,0,oneError);assert(oneReparsed);
        verify(*oneBefore,*oneReparsed);
        assert(oneRaw==oneSource);
        assert(std::equal(oneParsed.save->sourceBytes().begin(),oneParsed.save->sourceBytes().end(),oneSource.begin()));
    };
    {
        auto unchangedRaw=fixture(l,true);const auto unchangedSource=unchangedRaw;
        auto unchangedParsed=parse(unchangedRaw,game);assert(unchangedParsed);
        std::string unchangedError;
        auto unchangedEditor=StagedEditor::create(*unchangedParsed.save,unchangedError);assert(unchangedEditor);
        auto unchangedBefore=unchangedEditor->boxedPokemon(0,0,unchangedError);assert(unchangedBefore);
        auto unchangedOut=unchangedEditor->finalizedBytes(unchangedError);assert(!unchangedOut.empty());
        auto unchangedReload=parse(unchangedOut,game);assert(unchangedReload);
        auto unchangedReloadEditor=StagedEditor::create(*unchangedReload.save,unchangedError);assert(unchangedReloadEditor);
        auto unchangedAfter=unchangedReloadEditor->boxedPokemon(0,0,unchangedError);assert(unchangedAfter);
        assertSamePokemon(*unchangedBefore,*unchangedAfter);
        assert(unchangedRaw==unchangedSource);
    }
    {
        BoxPokemonEdit one;one.species=26;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){assert(after.species==26);});
    }
    {
        BoxPokemonEdit one;one.nickname="FIELD";
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){assert(after.nickname=="FIELD");});
    }
    {
        BoxPokemonEdit one;one.level=20;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){
            const auto* pi=personalRecord(after.species);assert(pi);assert(after.level==20);
            assert(after.experience==Pokemon::getExpForLevel(20,pi->experienceGrowth));
        });
    }
    {
        BoxPokemonEdit one;one.heldItem=kPotionItemId;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){assert(after.heldItem==kPotionItemId);});
    }
    {
        BoxPokemonEdit one;one.moves=std::array<uint8_t,4>{84,45,0,0};
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){
            assert((after.moves==std::array<uint8_t,4>{84,45,0,0}));
            assert(after.pp[0]==StagedEditor::gen2MoveBasePP(84));
        });
    }
    {
        BoxPokemonEdit one;one.ppUps=std::array<uint8_t,4>{1,0,0,0};
        one.pp=std::array<uint8_t,4>{42,40,0,0};
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){
            assert(after.ppUps[0]==1&&after.pp[0]==42);
        });
    }
    {
        BoxPokemonEdit one;one.dvs=std::array<uint8_t,4>{9,8,7,6};
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){
            assert((std::array<uint8_t,4>{after.dvs[1],after.dvs[2],after.dvs[3],after.dvs[4]}==
                    std::array<uint8_t,4>{9,8,7,6}));
            assert(after.dvs[0]==StagedEditor::derivedHPDV({9,8,7,6}));
            assert(after.gender==static_cast<uint8_t>(genderFromAttackDV(after.species,after.dvs[1])));
        });
    }
    {
        BoxPokemonEdit one;one.statExperience=std::array<uint16_t,5>{100,200,300,400,500};
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){
            assert((after.statExperience==std::array<uint16_t,5>{100,200,300,400,500}));
        });
    }
    {
        BoxPokemonEdit one;one.otName="RED";one.trainerId=4321;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){
            assert(after.originalTrainer=="RED"&&after.trainerId==4321);
        });
    }
    {
        BoxPokemonEdit one;one.friendship=200;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){assert(after.friendship==200);});
    }
    {
        BoxPokemonEdit one;one.pokerus=0x21;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){assert(after.pokerus==0x21);});
    }
    {
        BoxPokemonEdit one;one.caughtData=0x1234;
        singleFieldRoundTrip(one,[](const PokemonRecord&,const PokemonRecord& after){assert(after.caughtData==0x1234);});
    }

'''
    s = s.replace(anchor, anchor + block, 1)

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
