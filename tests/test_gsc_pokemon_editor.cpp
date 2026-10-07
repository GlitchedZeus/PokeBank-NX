#include "fixtures/gsc_pokemon_fixture.h"

namespace {
void runFamily(const L& l,SourceGame game){
    auto raw=fixture(l,true);const auto source=raw;
    auto parsed=parse(raw,game);assert(parsed);
    // The accepted read parser intentionally keeps the raw PokemonRecord gender unresolved.
    // The staged semantic layer must derive gender from species + Attack DV without changing it.
    assert(parsed.save->boxes()[0].slots[0]);
    assert(parsed.save->boxes()[0].slots[0]->gender==2);
    std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor&&error.empty());
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::BoxPokemon));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonEditing));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonCreation));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonClone));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::PokemonShinyToggle));
    assert(editor->capabilities().supports(PokeVault::SaveEdit::Capability::MachineMoveNames));
    assert(!editor->capabilities().supports(PokeVault::SaveEdit::Capability::PartyPokemon));

    auto p=editor->boxedPokemon(0,0,error);assert(p&&p->species==25&&p->nickname=="PIKA");
    assert(p->gender==static_cast<uint8_t>(genderFromAttackDV(p->species,p->dvs[1])));
    assert(p->shiny);assert(StagedEditor::isShinyDVs({7,10,10,10}));
    assert(StagedEditor::derivedHPDV({7,10,10,10})==8);
    assert(raw==source);assert(std::equal(parsed.save->sourceBytes().begin(),parsed.save->sourceBytes().end(),source.begin()));

    // Per-field semantic round-trip regression matrix.
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

    BoxPokemonEdit edit;edit.species=26;edit.nickname="SPARKY";edit.level=20;edit.heldItem=1;
    edit.moves=std::array<uint8_t,4>{84,85,0,0};
    edit.dvs=std::array<uint8_t,4>{9,8,7,6};
    edit.statExperience=std::array<uint16_t,5>{100,200,300,400,500};
    edit.otName="RED";edit.trainerId=4321;edit.friendship=200;edit.pokerus=0x21;edit.caughtData=0x1234;
    assert(editor->stageBoxPokemonEdit(0,0,edit,error));
    auto changed=editor->boxedPokemon(0,0,error);assert(changed);
    assert(changed->species==26&&changed->nickname=="SPARKY"&&changed->level==20&&changed->heldItem==1);
    assert((changed->moves==std::array<uint8_t,4>{84,85,0,0}));
    assert(changed->pp[0]==StagedEditor::gen2MoveBasePP(84));
    assert(changed->pp[1]==StagedEditor::gen2MoveBasePP(85));
    assert(changed->dvs[0]==StagedEditor::derivedHPDV({9,8,7,6}));
    assert(changed->gender==static_cast<uint8_t>(genderFromAttackDV(changed->species,changed->dvs[1])));
    assert(changed->originalTrainer=="RED"&&changed->trainerId==4321&&changed->friendship==200);
    const auto* personal=personalRecord(26);assert(personal);
    assert(changed->experience==Pokemon::getExpForLevel(20,personal->experienceGrowth));
    assert(Pokemon::getLevelFromExp(changed->experience,personal->experienceGrowth)==20);

    // Invalid/malformed edits reject atomically.
    const auto preReject=std::vector<uint8_t>(editor->stagedBytes().begin(),editor->stagedBytes().end());
    BoxPokemonEdit bad;bad.nickname="ABCDEFGHIJK";assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.species=252;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.nickname="BAD@";assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.level=0;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.level=101;assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.heldItem=6;assert(!editor->stageBoxPokemonEdit(0,0,bad,error)); // TERU-SAMA
    bad={};bad.moves=std::array<uint8_t,4>{252,0,0,0};assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    bad={};bad.dvs=std::array<uint8_t,4>{16,1,1,1};assert(!editor->stageBoxPokemonEdit(0,0,bad,error));
    assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),preReject.begin()));

    // Exact 10-character international nickname is accepted.
    BoxPokemonEdit maxName;maxName.nickname="ABCDEFGHIJ";assert(editor->stageBoxPokemonEdit(0,0,maxName,error));
    // Explicit PP/PP-Up validation.
    BoxPokemonEdit ppGood;ppGood.ppUps=std::array<uint8_t,4>{1,0,0,0};
    ppGood.pp=std::array<uint8_t,4>{static_cast<uint8_t>(StagedEditor::gen2MoveBasePP(84)*6/5),
                                     StagedEditor::gen2MoveBasePP(85),0,0};
    assert(editor->stageBoxPokemonEdit(0,0,ppGood,error));
    BoxPokemonEdit ppBad;ppBad.pp=std::array<uint8_t,4>{63,63,0,0};assert(!editor->stageBoxPokemonEdit(0,0,ppBad,error));

    // Shiny -> non-shiny preserves Attack DV/gender; non-shiny -> shiny uses real DV rule.
    auto beforeToggle=editor->boxedPokemon(0,0,error);assert(beforeToggle);
    assert(editor->stageBoxPokemonShiny(0,0,false,error));
    auto nonShiny=editor->boxedPokemon(0,0,error);assert(nonShiny&&!nonShiny->shiny);
    const auto genderBefore=nonShiny->gender;
    assert(editor->stageBoxPokemonShiny(0,0,true,error));
    auto shinyAgain=editor->boxedPokemon(0,0,error);assert(shinyAgain&&shinyAgain->shiny&&shinyAgain->gender==genderBefore);

    // A female 12.5%-female species with Attack DV 1 cannot preserve female while becoming shiny.
    BoxPokemonEdit female;female.species=1;female.dvs=std::array<uint8_t,4>{1,8,8,8};
    assert(editor->stageBoxPokemonEdit(1,0,female,error));
    auto femaleMon=editor->boxedPokemon(1,0,error);assert(femaleMon&&femaleMon->gender==1);
    const auto femaleBytes=std::vector<uint8_t>(editor->stagedBytes().begin(),editor->stagedBytes().end());
    assert(!editor->stageBoxPokemonShiny(1,0,true,error));assert(error.find("gender")!=std::string::npos);
    assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),femaleBytes.begin()));

    // Clone appends without touching source.
    auto sourceBeforeClone=editor->boxedPokemon(0,0,error);assert(sourceBeforeClone);
    size_t cloneSlot=99;assert(editor->stageCloneBoxPokemon(0,0,2,cloneSlot,error));assert(cloneSlot==0);
    auto clone=editor->boxedPokemon(2,0,error);assert(clone&&clone->nickname==sourceBeforeClone->nickname);
    auto sourceAfterClone=editor->boxedPokemon(0,0,error);assert(sourceAfterClone&&sourceAfterClone->nickname==sourceBeforeClone->nickname);

    // New boxed Pokemon is complete, level/EXP consistent, default OT/TID comes from save.
    BoxPokemonCreate created;created.species=158;created.level=5;created.nickname="TOTODILE";
    created.moves={10,43,0,0};created.dvs={8,8,8,8};created.friendship=70;
    size_t addSlot=99;assert(editor->stageAddBoxPokemon(3,created,addSlot,error));assert(addSlot==0);
    auto added=editor->boxedPokemon(3,0,error);assert(added&&added->species==158&&added->nickname=="TOTODILE");
    assert(added->originalTrainer=="ASH"&&added->trainerId==0x1234&&!added->shiny);
    const auto* createdPersonal=personalRecord(158);assert(createdPersonal);
    assert(added->experience==Pokemon::getExpForLevel(5,createdPersonal->experienceGrowth));
    assert((added->statExperience==std::array<uint16_t,5>{0,0,0,0,0}));

    BoxPokemonCreate defaultNamed=created;defaultNamed.species=25;defaultNamed.nickname.clear();
    size_t defaultSlot=99;assert(editor->stageAddBoxPokemon(5,defaultNamed,defaultSlot,error));
    auto defaultMon=editor->boxedPokemon(5,0,error);assert(defaultMon&&defaultMon->nickname=="Pikachu");
    BoxPokemonEdit genderGlyph;genderGlyph.nickname="NIDORAN\xE2\x99\x80";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    auto glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\xE2\x99\x80");
    genderGlyph.nickname="NIDORAN\xE2\x99\x82";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\xE2\x99\x82");

    // Full box refuses add/clone and never overwrites.
    auto fullRaw=fixture(l);fullBox(fullRaw,4);checksum(fullRaw,l);
    auto fullParsed=parse(fullRaw,game);assert(fullParsed);auto fullEditor=StagedEditor::create(*fullParsed.save,error);assert(fullEditor);
    const auto fullBefore=std::vector<uint8_t>(fullEditor->stagedBytes().begin(),fullEditor->stagedBytes().end());
    size_t noSlot=99;assert(!fullEditor->stageAddBoxPokemon(4,created,noSlot,error));assert(error.find("full")!=std::string::npos);
    assert(!fullEditor->stageCloneBoxPokemon(0,0,4,noSlot,error));
    assert(std::equal(fullEditor->stagedBytes().begin(),fullEditor->stagedBytes().end(),fullBefore.begin()));

    // Final export: checksum pair, RTC preservation, strict reload, semantic values, counts/terminators.
    assert(!editor->pendingChanges().empty());
    auto out=editor->finalizedBytes(error);assert(!out.empty()&&error.empty());
    assert(out.size()==source.size());assert(le16(out,l.sum)==le16(out,l.sum2));
    assert(std::equal(out.begin()+0x8000,out.end(),source.begin()+0x8000));
    auto reload=parse(out,game);assert(reload);
    assert(reload.save->boxes()[2].slots[0]&&reload.save->boxes()[3].slots[0]);
    assert(out[boxStart(2)]==1&&out[boxStart(2)+2]==0xFF);
    assert(out[boxStart(3)]==1&&out[boxStart(3)+2]==0xFF);
    assert(source==raw); // caller source remains sacred throughout staged editing/export.

    // Isolated non-current boxed edit changes only that stored box after comparing normalized exports.
    auto baseRaw=fixture(l);auto baseParsed=parse(baseRaw,game);assert(baseParsed);
    auto baseEditor=StagedEditor::create(*baseParsed.save,error);assert(baseEditor);
    auto normalized=baseEditor->finalizedBytes(error);assert(!normalized.empty());
    BoxPokemonEdit isolated;isolated.nickname="ONLYBOX";assert(baseEditor->stageBoxPokemonEdit(1,0,isolated,error));
    auto isolatedOut=baseEditor->finalizedBytes(error);assert(!isolatedOut.empty());
    assert(differsOnlyInRange(normalized,isolatedOut,boxStart(1),boxStart(1)+listLen(20,32,11)));

    editor->discard();assert(!editor->hasPendingChanges());
    assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),source.begin()));
}
}

int main(){
    runFamily(GS,SourceGame::Gold);
    runFamily(GS,SourceGame::Silver);
    runFamily(C,SourceGame::Crystal);
    std::cout<<"Generation II boxed Pokemon staged editor tests: PASS\n";
}
