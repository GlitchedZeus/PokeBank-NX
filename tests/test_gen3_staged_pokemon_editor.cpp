#include "Integration/Gen3/Gen3HeldItems.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "Integration/Gen3/Gen3ExportTransaction.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <unistd.h>
#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include "Pokemon/Experience.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Names/PokemonNames.h"
#include "Names/ItemNames.h"
#include "Names/MoveNames.h"
#include "Enums/Ball.h"
#include "Names/LocationDisplayNames.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen3;

namespace {

enum class Family { RS, Emerald, FRLG };
using LogicalSectors =
    std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}

uint8_t origin(SourceGame game) {
    switch (game) {
        case SourceGame::SapphireGBA: return 1;
        case SourceGame::RubyGBA: return 2;
        case SourceGame::EmeraldGBA: return 3;
        case SourceGame::FireRedGBA: return 4;
        case SourceGame::LeafGreenGBA: return 5;
    }
    return 0;
}

const char* exactSourceGameId(SourceGame game) {
    switch (game) {
        case SourceGame::RubyGBA: return "ruby_gba";
        case SourceGame::SapphireGBA: return "sapphire_gba";
        case SourceGame::EmeraldGBA: return "emerald_gba";
        case SourceGame::FireRedGBA: return "firered_gba";
        case SourceGame::LeafGreenGBA: return "leafgreen_gba";
    }
    return "";
}

// When set, samplePokemon (and so fixture()) stores the sample as an Egg.
bool gSampleIsEgg = false;
// The sample's held item (13 = Ether by default).
uint16_t gSampleHeldItem = 13;

std::array<uint8_t,80> samplePokemon(SourceGame game) {
    std::array<uint8_t,80> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(25);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"PIKACHU");
    p.setHeldItem(gSampleHeldItem);
    p.setFriendship(70);
    p.setOriginGame(origin(game));
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setLevel(20);
    p.setMove(0, 33); // Tackle
    p.setMovePPUps(0, 1);
    p.setMovePP(0, 40);
    for (int i = 0; i < 6; ++i) {
        p.setIV(i, static_cast<uint8_t>(10 + i));
        p.setEV(i, static_cast<uint8_t>(20 + i));
    }
    if (gSampleIsEgg) p.setEgg(true);
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::array<uint8_t,80> out{};
    std::memcpy(out.data(), encrypted, out.size());
    delete[] encrypted;
    return out;
}

LogicalSectors makeLogical(Family family, SourceGame game) {
    LogicalSectors logical{};
    if (family == Family::FRLG) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, key);
        // FRLG money is XOR-obfuscated with the full security key. Encode zero money.
        write32(logical[1], 0x0290, key);
    } else if (family == Family::Emerald) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        // Emerald money is XOR-obfuscated with the same full security key. Encode zero money.
        write32(logical[1], 0x0490, key);
    } else {
        write32(logical[0], 0xAC, 0);
        write32(logical[1], 0x0490, 0);
    }
    write16(logical[0], 0x0A, 12345);
    write16(logical[0], 0x0C, 54321);

    // Pinned PKHeX SaveBlock3LargeRS/E/FRLG badge flags (not an assumed
    // byte at the start of the event array). These straddle logical chunks.
    const size_t flags=family==Family::FRLG?0xee0:family==Family::Emerald?0x1270:0x1220;
    const unsigned firstBadge=family==Family::FRLG?0x820:family==Family::Emerald?0x867:0x807;
    for(unsigned i=0;i<8;++i)if((0xa5u>>i)&1u) {
        const auto position=flags+(firstBadge+i)/8;
        logical[1+position/Detail::kSectorDataSize][position%Detail::kSectorDataSize]|=
            static_cast<uint8_t>(1u<<((firstBadge+i)%8));
    }

    const auto mon = samplePokemon(game);
    const std::size_t logicalOffset = 4;
    for (std::size_t i = 0; i < mon.size(); ++i) {
        const std::size_t position = logicalOffset + i;
        const std::size_t sector = 5 + position / Detail::kSectorDataSize;
        const std::size_t offset = position % Detail::kSectorDataSize;
        logical[sector][offset] = mon[i];
    }
    return logical;
}

void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter,
               uint8_t rotation, Family family, SourceGame game) {
    auto logical = makeLogical(family, game);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset =
            Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(),
                  save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(
            std::span<const uint8_t>(save).subspan(
                offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}

std::vector<uint8_t> fixture(Family family, SourceGame game) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, game);
    writeSlot(save, 1, 9, 5, family, game);
    return save;
}

void runGame(SourceGame game, Family family) {
    { // Real 100-byte Party records in the selected rotating slot, for all five titles.
        auto source=fixture(family,game);
        const auto selected=Detail::validateSlot(source,1);assert(selected.valid);
        const auto sector=selected.logicalSectorOffsets[1];
        const auto body=sector+Detail::partyDataOffset(game);
        source[sector+Detail::partyCountOffset(game)]=1;
        const auto boxed=samplePokemon(game);
        std::copy(boxed.begin(),boxed.end(),source.begin()+body);
        Pokemon::Pokemon3FRLG native(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(source.data()+body),100));
        native.recalculateStats();native.setStatHPCurrent(native.statHPMax()-7);
        auto* encoded=Encryption::encryptArray3FRLG(native.getData());
        std::memcpy(source.data()+body,encoded,100);delete[] encoded;
        source[body+0x50]=8;source[body+0x55]=0xA5; // status and mail id
        write16(source,sector+0xFF6,Detail::sectorChecksum(std::span<const uint8_t>(source).subspan(sector,Detail::kChunkLengths[1])));
        const auto immutable=source;
        std::string error;auto editor=StagedPokemonEditor::create(source,game,error);assert(editor);
        const auto before=editor->partyPokemon(0,error);assert(before);
        {
            auto reviewed=StagedPokemonEditor::create(source,game,error);assert(reviewed);
            BoxPokemonEdit request;request.nature=static_cast<uint8_t>((before->nature+1)%25);
            // Round 11 FRLG: an exact-game native location (FR/LG Kanto 89 Viridian City, R/S/E
            // Hoenn 2 Dewford Town); deliberately updated from 2 for every game.
            const uint16_t nativeMet=family==Family::FRLG?89:2;
            request.language=3;request.ball=2;request.metLocation=nativeMet;request.metLevel=19;
            request.friendship=99;request.pokerus=0x13;request.otName="WILL";
            assert(reviewed->stagePartyPokemonEdit(0,request,error));
            const auto expectedLocation=Names::getMetLocationDisplayName(before->originGame,nativeMet);
            size_t named=0;
            for(const auto& row:reviewed->pendingChanges()) {
                assert(row.beforeValue!=row.afterValue && row.party);
                assert(row.label.find("Pokemon edit")==std::string::npos);
                if(row.label=="Party 1 Nature") {assert(row.afterValue==Names::getNatureName(*request.nature));++named;}
                if(row.label=="Party 1 Language") {assert(row.afterValue=="French");++named;}
                if(row.label=="Party 1 Ball") {assert(row.afterValue=="Ultra Ball");++named;}
                if(row.label=="Party 1 Met Location") {assert(row.afterValue==expectedLocation);++named;}
                if(row.label=="Party 1 OT Name") {assert(row.afterValue=="WILL");++named;}
                if(row.label=="Party 1 Pokérus") {assert(row.afterValue.find("Infected")!=std::string::npos);++named;}
            }
            assert(named==6 && reviewed->originalBytes()==immutable && source==immutable);
        }
        {
            auto unusual=source;unusual[body+0x54]=77;
            unusual[body+0x56]=3;unusual[body+0x57]=0;
            unusual[body+0x58]=0;unusual[body+0x59]=0;
            write16(unusual,sector+0xFF6,Detail::sectorChecksum(std::span<const uint8_t>(unusual).subspan(sector,Detail::kChunkLengths[1])));
            auto metadataOnly=StagedPokemonEditor::create(unusual,game,error);assert(metadataOnly);
            BoxPokemonEdit friendship;friendship.friendship=99;
            assert(metadataOnly->stagePartyPokemonEdit(0,friendship,error));
            const auto bytes=metadataOnly->finalizedBytes(error);assert(!bytes.empty());
            assert(std::equal(unusual.begin()+body+80,unusual.begin()+body+100,bytes.begin()+body+80));
        }
        BoxPokemonEdit edit;edit.level=50;edit.nickname="PARTY";edit.ivs=std::array<uint8_t,6>{31,30,29,28,27,26};
        edit.moves=std::array<uint16_t,4>{85,0,0,0};
        edit.pp=std::array<uint8_t,4>{15,0,0,0};edit.ppUps=std::array<uint8_t,4>{0,0,0,0};
        const auto preview=editor->previewPartyEdit(0,edit,error);assert(preview && editor->stagedBytes()==immutable);
        assert(editor->stagePartyPokemonEdit(0,edit,error));
        bool reviewedMove=false;
        for(const auto& row:editor->pendingChanges())if(row.party && row.label=="Party 1 Move 1") {
            assert(row.afterValue=="Thunderbolt");reviewedMove=true;
        }
        assert(reviewedMove);
        const auto after=editor->partyPokemon(0,error);assert(after && after->level==50 && after->nickname=="PARTY");
        const auto candidate=editor->finalizedBytes(error);assert(!candidate.empty());
        Pokemon::Pokemon3FRLG reload(std::span<const std::byte>(reinterpret_cast<const std::byte*>(candidate.data()+body),100));
        assert(reload.checksumValid() && reload.getData()[0x54]==std::byte{50});
        assert(reload.getData()[0x50]==std::byte{8} && reload.getData()[0x55]==std::byte{0xA5});
        assert(reload.statHPCurrent()==native.statHPCurrent());
        assert((std::array<uint16_t,6>{reload.statHPMax(),reload.statATK(),reload.statDEF(),reload.statSPE(),reload.statSPA(),reload.statSPD()})==after->calculatedStats);
        for(size_t i=0;i<source.size();++i)
            if(source[i]!=candidate[i])assert((i>=body && i<body+100) || i==sector+0xFF6 || i==sector+0xFF7);
        edit.level=0;const auto staged=editor->stagedBytes();
        assert(!editor->stagePartyPokemonEdit(0,edit,error) && editor->stagedBytes()==staged);
        assert(!editor->stagePartyPokemonEdit(1,{},error) && editor->stagedBytes()==staged);
        assert(source==immutable && editor->originalBytes()==immutable);
        editor->discard();assert(editor->stagedBytes()==immutable && !editor->hasPendingChanges());
    }
    auto source = fixture(family, game);
    const auto immutableSource = source;
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    if (!editor || !error.empty()) {
        std::cerr << "Gen III staged editor create failed: game="
                  << static_cast<unsigned>(game) << " family="
                  << static_cast<unsigned>(family) << " error=" << error << "\\n";
    }
    assert(editor && error.empty());
    assert(editor->trainer().badges==0xa5);
    assert(editor->boxCount() == 14);
    assert(editor->boxCapacity() == 30);
    assert(StagedPokemonEditor::pidCorrelatedEditingEnabled());
    assert(!StagedPokemonEditor::moveSelectionEditingEnabled());

    // October 24 R2-REVIEWMOVE: actual Create -> Pikachu -> Safari Ball -> Keep.
    // Exercise BOTH caught-Ball and Held Item choices; their numeric IDs must
    // never be confused. Reparse the native encrypted record and sector checksums.
    assert(std::string(Names::getItemNameG3(21))=="Hyper Potion");
    // Owner Oct 25: PKHeX ItemStorage3RS.Unreleased (Safari Ball 5, Berry
    // Juice 44) are never released held items, so they are refused below.
    for(const uint16_t held : {uint16_t{0},uint16_t{14},uint16_t{21}}) {
        for(uint8_t ball=1;ball<=12;++ball) {
            auto createEditor=StagedPokemonEditor::create(source,game,error); assert(createEditor);
            PokeBank::UIModel::Gen3SharedEditor::Session draft;
            draft.beginCreate(createEditor->trainer(),createEditor->originGame());
            assert(draft.working.species==25);
            draft.working.heldItem=held;draft.working.ball=ball;
            if(!draft.add(*createEditor,0,1,error)) { std::cerr<<"Create ball="<<int(ball)<<" held="<<held<<": "<<error<<"\n";assert(false); }
            const auto created=createEditor->boxedPokemon(0,1,error);
            assert(created && created->heldItem==held && created->ball==ball);
            const auto bytes=createEditor->finalizedBytes(error);assert(!bytes.empty());
            const auto reopened=StagedPokemonEditor::create(bytes,game,error);assert(reopened);
            const auto persisted=reopened->boxedPokemon(0,1,error);
            assert(persisted && persisted->heldItem==held && persisted->ball==ball);
            assert(createEditor->originalBytes()==immutableSource && source==immutableSource);
            createEditor->discard();assert(createEditor->stagedBytes()==immutableSource);
        }
    }
    assert(nativeHeldItemSupported(5) && nativeHeldItemSupported(44));
    assert(!releasedHeldItem(5) && !releasedHeldItem(44));
    assert(releasedHeldItem(0) && releasedHeldItem(4) && releasedHeldItem(43));
    for(uint16_t invalid : {uint16_t{5},uint16_t{44},uint16_t{259},uint16_t{339},uint16_t{377},uint16_t{65535}}) {
        auto rejected=StagedPokemonEditor::create(source,game,error);assert(rejected);
        BoxPokemonCreate bad;bad.heldItem=invalid;
        assert(!rejected->stageAddBoxPokemon(0,1,bad,error));
        assert(rejected->stagedBytes()==immutableSource && rejected->pendingChanges().empty());
    }
    for(uint8_t invalid : {uint8_t{0},uint8_t{13},uint8_t{255}}) {
        auto rejected=StagedPokemonEditor::create(source,game,error);assert(rejected);
        BoxPokemonCreate bad;bad.ball=invalid;
        assert(!rejected->stageAddBoxPokemon(0,1,bad,error));
        assert(rejected->stagedBytes()==immutableSource && rejected->pendingChanges().empty());
    }
    auto before = editor->boxedPokemon(0, 0, error);
    assert(before && error.empty());
    // Left-panel edits round-trip for every exact Gen III game without source writes.
    {
        auto identity = StagedPokemonEditor::create(source, game, error); assert(identity);
        BoxPokemonEdit edit; edit.nickname = "BLUE"; edit.level = 50;
        edit.otName = "WILL"; edit.tid = 65535;
        assert(identity->stageBoxPokemonEdit(0, 0, edit, error));
        const auto after = identity->boxedPokemon(0, 0, error); assert(after);
        assert(after->nickname == "BLUE" && after->level == 50 && after->otName == "WILL" && after->tid == 65535);
        assert(after->pid == before->pid && after->sid == before->sid);
        assert(after->shiny == ((after->tid ^ after->sid ^ (after->pid & 65535) ^ (after->pid >> 16)) < 8));
        assert(identity->originalBytes() == immutableSource && source == immutableSource);
        identity->discard(); assert(identity->stagedBytes() == immutableSource);
        BoxPokemonCreate create; create.nickname = "DRAFT"; create.level = 50;
        create.otName = "BLUE"; create.tid = 0;
        assert(identity->stageAddBoxPokemon(0, 1, create, error));
        const auto added = identity->boxedPokemon(0, 1, error); assert(added);
        assert(added->nickname == "DRAFT" && added->level == 50 && added->otName == "BLUE" && added->tid == 0);
        assert(identity->originalBytes() == immutableSource);
        // CG74 (owner Oct 25 P3): Review lists the created record's values, not only the species.
        {
            const auto& rows = identity->pendingChanges();
            const auto row = [&](const std::string& name) -> const StagedPokemonChange* {
                for (const auto& r : rows)
                    if (r.label == "Box 1 / Slot 2 / " + name) return &r;
                return nullptr;
            };
            // The shared Gen I-V Create Review shape (Gen12CreateReview.h).
            assert(row("Add Pok\xC3\xA9mon") && !row("Add Pok\xC3\xA9mon")->createdDetail &&
                   row("Add Pok\xC3\xA9mon")->beforeValue == "Empty" &&
                   row("Add Pok\xC3\xA9mon")->afterValue == "Pikachu Lv 50");
            for (const char* name : {"Nickname", "Ball", "OT / TID / SID", "Met", "Nature", "Gender", "Ability",
                                     "Language"})
                assert(row(name) && row(name)->createdDetail && row(name)->beforeValue.empty() &&
                       !row(name)->afterValue.empty());
            assert(row("Ball")->afterValue == Enums::getBallName(4));
            assert(row("Nickname")->afterValue == "DRAFT");
            assert(row("OT / TID / SID")->afterValue == "BLUE / 0 / " + std::to_string(added->sid));
            // Empty rows and Create defaults add no noise (this draft has no moves).
            for (const char* name : {"Moves", "Held Item", "Shiny", "Pokérus", "Friendship", "IVs", "EVs"})
                assert(!row(name) || (std::string(name) == "Shiny" && added->shiny));
        }
        identity->discard(); assert(identity->stagedBytes() == immutableSource);
        {
            BoxPokemonCreate full = create;
            full.moves = {85, 98, 0, 0};
            full.pp = {15, 30, 0, 0};
            full.heldItem = 13;
            full.ivs = {31, 0, 0, 0, 0, 1};
            full.pokerus = 0x21;
            full.friendship = 255;
            assert(identity->stageAddBoxPokemon(0, 1, full, error));
            std::vector<std::string> details;
            for (const auto& r : identity->pendingChanges())
                if (r.createdDetail) details.push_back(r.label.substr(std::string("Box 1 / Slot 2 / ").size()) + "=" + r.afterValue);
            const auto has = [&](const std::string& entry) {
                return std::find(details.begin(), details.end(), entry) != details.end();
            };
            assert(has("Moves=" + std::string(Names::getMoveName(85)) + ", " + Names::getMoveName(98)));
            assert(has("Held Item=" + std::string(Names::getItemNameG3(13))));
            assert(has("IVs=HP 31, SpD 1"));
            assert(has("Pokérus=Infected (33)") && has("Friendship=255"));
            identity->discard(); assert(identity->stagedBytes() == immutableSource);
        }
        // A blank English nickname defaults to the upper-case retail species name.
        BoxPokemonCreate unnamed; unnamed.species = 25; unnamed.level = 5; unnamed.language = 2;
        unnamed.otName = "BLUE";
        assert(identity->stageAddBoxPokemon(0, 1, unnamed, error));
        assert(identity->boxedPokemon(0, 1, error)->nickname == "PIKACHU");
        identity->discard();
        // M1 D2: a blank nickname is the record language's own default name, formatted as PKHeX
        // SpeciesName.GetSpeciesNameGeneration(species, language, 3) does: Japanese as-is, other
        // languages upper case, French E/I accents stripped, Farfetch'd with a straight apostrophe.
        struct LocalizedDefault { uint8_t language; uint16_t species; const char* name; };
        for (const auto& want : {LocalizedDefault{1, 1, "フシギダネ"}, LocalizedDefault{2, 1, "BULBASAUR"},
                                 LocalizedDefault{3, 1, "BULBIZARRE"}, LocalizedDefault{5, 1, "BISASAM"},
                                 LocalizedDefault{4, 1, "BULBASAUR"}, LocalizedDefault{7, 1, "BULBASAUR"},
                                 LocalizedDefault{3, 4, "SALAMECHE"}, LocalizedDefault{2, 83, "FARFETCH'D"},
                                 LocalizedDefault{1, 25, "ピカチュウ"}}) {
            BoxPokemonCreate localized; localized.species = want.species; localized.level = 5;
            localized.language = want.language; localized.otName = want.language == 1 ? "サトシ" : "BLUE";
            if (!identity->stageAddBoxPokemon(0, 1, localized, error))
                std::cerr << "D2 create lang=" << int(want.language) << " species=" << want.species << ": " << error << "\n";
            assert(identity->boxedPokemon(0, 1, error)->nickname == want.name);
            identity->discard();
        }
        // M1 D1: nickname and language edited together encode in the NEW language; a language-only
        // change renames a default name to the new language's default and keeps a custom name
        // only if it can be stored unchanged, otherwise it is refused with nothing written.
        {
            BoxPokemonCreate bulba; bulba.species = 1; bulba.level = 5; bulba.language = 2; bulba.otName = "BLUE";
            assert(identity->stageAddBoxPokemon(0, 1, bulba, error));
            const auto named = [&] { return identity->boxedPokemon(0, 1, error); };
            BoxPokemonEdit toFrench; toFrench.language = 3;
            assert(identity->stageBoxPokemonEdit(0, 1, toFrench, error));
            assert(named()->nickname == "BULBIZARRE" && named()->language == 3);
            BoxPokemonEdit toJapanese; toJapanese.language = 1;
            assert(identity->stageBoxPokemonEdit(0, 1, toJapanese, error));
            assert(named()->nickname == "フシギダネ" && named()->language == 1);
            BoxPokemonEdit both; both.nickname = "Müller"; both.language = 5;
            assert(identity->stageBoxPokemonEdit(0, 1, both, error));
            assert(named()->nickname == "Müller" && named()->language == 5);
            BoxPokemonEdit toSpanish; toSpanish.language = 7;   // custom name kept, same table
            assert(identity->stageBoxPokemonEdit(0, 1, toSpanish, error));
            assert(named()->nickname == "Müller" && named()->language == 7);
            const auto beforeRefusal = identity->stagedBytes();
            BoxPokemonEdit cannotKeep; cannotKeep.language = 1;  // 'Müller' has no Japanese spelling
            assert(!identity->stageBoxPokemonEdit(0, 1, cannotKeep, error) && !error.empty());
            assert(identity->stagedBytes() == beforeRefusal);
            BoxPokemonEdit kana; kana.nickname = "ボブ"; kana.language = 1;
            assert(identity->stageBoxPokemonEdit(0, 1, kana, error));
            assert(named()->nickname == "ボブ" && named()->language == 1);
            BoxPokemonEdit backToEnglish; backToEnglish.nickname = "Émile"; backToEnglish.language = 2;
            assert(identity->stageBoxPokemonEdit(0, 1, backToEnglish, error));
            assert(named()->nickname == "Émile" && named()->language == 2);
            identity->discard();
        }
        // Japanese records: 5-character nickname and OT (PKHeX PK3); a 6th character is refused.
        BoxPokemonCreate japanese; japanese.species = 25; japanese.level = 5; japanese.language = 1;
        japanese.otName = "サトシ";
        japanese.nickname = "ピカチュウ";
        assert(identity->stageAddBoxPokemon(0, 1, japanese, error));
        identity->discard();
        japanese.nickname = "ピカチュウダ";
        assert(!identity->stageAddBoxPokemon(0, 1, japanese, error));
        japanese.nickname = "ピカ"; japanese.otName = "サトシサトシ";
        assert(!identity->stageAddBoxPokemon(0, 1, japanese, error));
        assert(identity->stagedBytes() == immutableSource);
        // Pokerus strain 0 with days left is never written (Create or Edit); real states are.
        BoxPokemonCreate sick; sick.species = 25; sick.level = 5; sick.language = 2; sick.otName = "BLUE";
        sick.pokerus = 0x0F;
        assert(!identity->stageAddBoxPokemon(0, 1, sick, error));
        BoxPokemonEdit pokerus; pokerus.pokerus = 0x0F;
        assert(!identity->stageBoxPokemonEdit(0, 0, pokerus, error));
        pokerus.pokerus = 0x21;
        assert(identity->stageBoxPokemonEdit(0, 0, pokerus, error));
        identity->discard();
        error.clear();
        create.otName = "TOOLONGOT";
        assert(!identity->stageAddBoxPokemon(0, 1, create, error));
        assert(identity->stagedBytes() == immutableSource);
        error.clear();
    }
    assert(before->species == 25);
    assert(before->level == 20);
    assert(before->moves[0] == 33);
    assert(before->pp[0] == 40);
    const uint32_t pid = before->pid;
    const uint16_t tid = before->tid;
    const uint16_t sid = before->sid;
    const uint8_t nature = before->nature;
    const bool shiny = before->shiny;

    // Shared species-change policy regression at the real staged serializer boundary.
    // R/S/E use Poochyena and FR/LG use Pidgey so every exact game exercises a
    // populated encounter minimum rather than a generation-only constant.
    const uint16_t replacementSpecies =
        family == Family::FRLG ? uint16_t{16} : uint16_t{261};
    const auto replacementLevel =
        PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(
            exactSourceGameId(game), replacementSpecies);
    assert(replacementLevel == 2);
    const auto stagedBeforeSpeciesPolicy = editor->stagedBytes();
    BoxPokemonEdit speciesPolicyEdit;
    speciesPolicyEdit.species = replacementSpecies;
    speciesPolicyEdit.level = replacementLevel;
    auto speciesPreview = editor->previewEdit(0, 0, speciesPolicyEdit, error);
    assert(speciesPreview && error.empty());
    assert(speciesPreview->species == replacementSpecies);
    assert(speciesPreview->level == replacementLevel);
    assert(speciesPreview->level != before->level);
    assert(speciesPreview->experience ==
           Pokemon::getExpForLevel(
               replacementLevel, Pokemon::getGrowthRate(replacementSpecies)));
    assert(speciesPreview->calculatedStats != before->calculatedStats);
    assert(editor->stagedBytes() == stagedBeforeSpeciesPolicy);
    assert(editor->originalBytes() == immutableSource && source == immutableSource);

    assert(editor->stageBoxPokemonEdit(0, 0, speciesPolicyEdit, error));
    auto speciesChanged = editor->boxedPokemon(0, 0, error);
    assert(speciesChanged && speciesChanged->species == replacementSpecies);
    assert(speciesChanged->level == replacementLevel);
    assert(speciesChanged->experience == speciesPreview->experience);
    assert(speciesChanged->calculatedStats == speciesPreview->calculatedStats);
    assert(editor->originalBytes() == immutableSource && source == immutableSource);

    std::array<uint16_t,6> levelOneStats{};
    for (uint8_t manual : {uint8_t(1), uint8_t(50), uint8_t(100)}) {
        BoxPokemonEdit manualLevel;
        manualLevel.level = manual;
        assert(editor->stageBoxPokemonEdit(0, 0, manualLevel, error));
        speciesChanged = editor->boxedPokemon(0, 0, error);
        assert(speciesChanged && speciesChanged->level == manual);
        assert(speciesChanged->experience ==
               Pokemon::getExpForLevel(
                   manual, Pokemon::getGrowthRate(replacementSpecies)));
        if (manual == 1) levelOneStats = speciesChanged->calculatedStats;
        if (manual == 100) assert(speciesChanged->calculatedStats != levelOneStats);
        assert(editor->originalBytes() == immutableSource && source == immutableSource);
    }

    // Cancel/discard restores the staged workspace only; the source remains byte-identical.
    editor->discard();
    assert(editor->stagedBytes() == immutableSource);
    assert(editor->originalBytes() == immutableSource && source == immutableSource);
    before = editor->boxedPokemon(0, 0, error);
    assert(before && before->species == 25 && before->level == 20);

    for (const uint16_t unreleased : {uint16_t{5}, uint16_t{44}}) {
        BoxPokemonEdit bad; bad.heldItem = unreleased;
        assert(!editor->stageBoxPokemonEdit(0, 0, bad, error));
        assert(editor->stagedBytes() == immutableSource);
    }
    error.clear();

    BoxPokemonEdit edit;
    edit.nickname = "SPARKY";
    edit.level = 25;
    edit.friendship = 200;
    edit.heldItem = 14;
    edit.ivs = std::array<uint8_t,6>{{31,30,29,28,27,26}};
    edit.evs = std::array<uint8_t,6>{{100,100,100,100,55,55}};
    edit.ppUps = std::array<uint8_t,4>{{2,0,0,0}};
    edit.pp = std::array<uint8_t,4>{{45,0,0,0}};
    edit.language = 2;
    edit.ball = 4;
    edit.metLevel = 12;
    // Round 11 FRLG: exact-game native ids (deliberately updated from 2, a Hoenn id, for FR/LG).
    edit.metLocation = family == Family::FRLG ? 89 : 2;
    assert(editor->stageBoxPokemonEdit(0, 0, edit, error));

    auto after = editor->boxedPokemon(0, 0, error);
    assert(after && error.empty());
    assert(after->nickname == "SPARKY");
    assert(after->level == 25);
    assert(after->friendship == 200);
    assert(after->heldItem == 14);
    assert(after->ivs == *edit.ivs);
    assert(after->evs == *edit.evs);
    assert(after->ppUps[0] == 2);
    assert(after->pp[0] == 45);
    assert(after->pid == pid);
    assert(after->tid == tid);
    assert(after->sid == sid);
    assert(after->nature == nature);
    assert(after->shiny == shiny);

    // Reject unsafe/out-of-domain values atomically.
    const auto stagedBeforeReject = editor->stagedBytes();
    BoxPokemonEdit badEv;
    badEv.evs = std::array<uint8_t,6>{{100,100,100,100,100,100}};
    assert(!editor->stageBoxPokemonEdit(0, 0, badEv, error));
    assert(editor->stagedBytes() == stagedBeforeReject);
    BoxPokemonEdit badItem;
    badItem.heldItem = 260; // key item, never a held-item picker choice
    assert(!editor->stageBoxPokemonEdit(0, 0, badItem, error));
    assert(editor->stagedBytes() == stagedBeforeReject);

    // Sparse-native clone: source stays occupied, exact destination slot receives identical PK3.
    assert(editor->stageCloneBoxPokemon(0, 0, 0, 1, error));
    auto clone = editor->boxedPokemon(0, 1, error);
    assert(clone && clone->encryptedBytes == after->encryptedBytes);

    // Create is explicit and writes the exact selected empty slot without packed compaction.
    BoxPokemonCreate create;
    create.species = 133;
    create.level = 10;
    create.experience = Pokemon::getExpForLevel(10, Pokemon::getGrowthRate(133));
    create.nickname = "EEVEE";
    create.heldItem = 13;
    create.moves = {33, 45, 0, 0};
    create.pp = {35, 40, 0, 0};
    create.ppUps = {0, 1, 0, 0};
    create.ivs = {31, 30, 29, 28, 27, 26};
    create.evs = {100, 90, 80, 70, 60, 50};
    create.language = 2;
    create.friendship = 88;
    create.pokerus = 0x21;
    create.ball = 4;
    // Round 11 FRLG (owner Extra 2): FireRed / LeafGreen Create writes a Kanto id. Deliberately
    // updated from 1 (Oldale Town, a Hoenn id the old test pinned for FR/LG) to 101 (Route 1).
    create.metLocation = family == Family::FRLG ? 101 : 57;
    create.metLevel = 25;
    create.abilityNumber = 1;
    assert(editor->stageAddBoxPokemon(0, 2, create, error));
    auto created = editor->boxedPokemon(0, 2, error);
    assert(created && created->species == 133 && created->level == 10);
    assert(created->experience == *create.experience);
    assert(created->originGame == origin(game));
    assert(created->heldItem == create.heldItem);
    assert(created->moves == create.moves && created->pp == create.pp && created->ppUps == create.ppUps);
    assert(created->ivs == create.ivs && created->evs == create.evs);
    assert(created->language == create.language && created->friendship == create.friendship);
    assert(created->pokerus == create.pokerus && created->ball == create.ball);
    assert(created->metLocation == create.metLocation && created->metLevel == create.metLevel);
    {
        // Round 11 FRLG: the stored met location of a created record is native to the save's own
        // game (Names::isGen3NativeMetLocation): FR/LG never write a Hoenn id, R/S/E never a Kanto
        // id. A foreign id is refused atomically; 0 stays the unchosen draft value.
        const bool frlg = family == Family::FRLG;
        const auto stagedBefore = editor->stagedBytes();
        const auto changesBefore = editor->pendingChanges().size();
        for (const uint16_t foreign : frlg ? std::vector<uint16_t>{1, 16, 57, 85, 197, 212}
                                           : std::vector<uint16_t>{88, 101, 141, 187, 196}) {
            BoxPokemonCreate bad = create; bad.metLocation = foreign;
            std::string why;
            assert(!editor->stageAddBoxPokemon(0, 5, bad, why));
            assert(why.find("met location") != std::string::npos);
            assert(editor->stagedBytes() == stagedBefore && editor->pendingChanges().size() == changesBefore);
        }
        if (game != SourceGame::EmeraldGBA) {
            BoxPokemonCreate bad = create; bad.metLocation = 200;   // Emerald-only Birth Island
            std::string why;
            assert(!editor->stageAddBoxPokemon(0, 5, bad, why) && editor->stagedBytes() == stagedBefore);
        }
        // Markers (in-game trade / fateful encounter) are native to every Gen III game; a traded
        // Rayquaza created in FireRed with a Kanto location stages and reads back the Kanto id.
        auto probe = StagedPokemonEditor::create(editor->stagedBytes(), game, error); assert(probe);
        BoxPokemonCreate rayquaza; rayquaza.species = 384; rayquaza.level = 70; rayquaza.metLevel = 70;
        rayquaza.metLocation = frlg ? 141 : 85;    // Cerulean Cave / Sky Pillar
        rayquaza.moves = {33, 0, 0, 0}; rayquaza.pp = {35, 0, 0, 0};
        assert(probe->stageAddBoxPokemon(0, 5, rayquaza, error));
        const auto readBack = probe->boxedPokemon(0, 5, error);
        assert(readBack && readBack->species == 384 && readBack->metLocation == rayquaza.metLocation);
        assert(readBack->originGame == origin(game));
        assert(Names::isGen3NativeMetLocation(readBack->originGame, readBack->metLocation));
        for (const uint16_t marker : {254, 255}) {
            auto markerProbe = StagedPokemonEditor::create(editor->stagedBytes(), game, error); assert(markerProbe);
            BoxPokemonCreate traded = rayquaza; traded.metLocation = marker;
            assert(markerProbe->stageAddBoxPokemon(0, 5, traded, error));
            assert(markerProbe->boxedPokemon(0, 5, error)->metLocation == marker);
        }
        // Edit: a changed met location must be native to the RECORD's origin game; the stored
        // value itself (even a legacy foreign id) is always kept unchanged.
        auto editProbe = StagedPokemonEditor::create(editor->stagedBytes(), game, error); assert(editProbe);
        const auto record = editProbe->boxedPokemon(0, 2, error); assert(record);
        BoxPokemonEdit foreignEdit; foreignEdit.metLocation = frlg ? 16 : 101;
        assert(!editProbe->stageBoxPokemonEdit(0, 2, foreignEdit, error));
        assert(editProbe->stagedBytes() == editor->stagedBytes());
        BoxPokemonEdit keepEdit; keepEdit.metLocation = record->metLocation; keepEdit.nickname = "KEPT";
        assert(editProbe->stageBoxPokemonEdit(0, 2, keepEdit, error));
        assert(editProbe->boxedPokemon(0, 2, error)->metLocation == record->metLocation);
        assert(editProbe->originalBytes() == editor->stagedBytes());
    }
    assert(created->abilityNumber == 1);
    assert(!created->shiny);

    if (game == SourceGame::RubyGBA) {
        // PID-linked edits use the real Gen III correlation rules and survive strict reparse.
        BoxPokemonCreate correlatedCreate;
        correlatedCreate.species = 280; // Ralts: variable gender + two native abilities.
        correlatedCreate.level = 10;
        correlatedCreate.experience = Pokemon::getExpForLevel(10, Pokemon::getGrowthRate(280));
        correlatedCreate.metLevel = 10;
        correlatedCreate.nickname = "RALTS";
        correlatedCreate.nature = 3;
        correlatedCreate.gender = 1;
        correlatedCreate.shiny = true;
        correlatedCreate.abilityNumber = 2;
        const auto sourceBeforeCorrelated = editor->originalBytes();
        auto correlatedPreview = editor->previewCreate(0, 3, correlatedCreate, error);
        assert(correlatedPreview && error.empty());
        assert(correlatedPreview->nature == 3);
        assert(correlatedPreview->gender == 1);
        assert(correlatedPreview->shiny);
        assert(correlatedPreview->abilityNumber == 2);
        assert(editor->stageAddBoxPokemon(0, 3, correlatedCreate, error));
        auto correlated = editor->boxedPokemon(0, 3, error);
        assert(correlated && correlated->nature == 3 && correlated->gender == 1);
        assert(correlated->shiny && correlated->abilityNumber == 2);
        assert(correlated->tid == editor->trainer().tid16 && correlated->sid == editor->trainer().sid16);
        assert(editor->originalBytes() == sourceBeforeCorrelated);

        const auto correlatedPid = correlated->pid;
        BoxPokemonEdit correlatedEdit;
        correlatedEdit.nature = 7;
        correlatedEdit.gender = 0;
        correlatedEdit.shiny = false;
        correlatedEdit.abilityNumber = 1;
        auto correlatedEditPreview = editor->previewEdit(0, 3, correlatedEdit, error);
        assert(correlatedEditPreview && error.empty());
        assert(correlatedEditPreview->nature == 7 && correlatedEditPreview->gender == 0);
        assert(!correlatedEditPreview->shiny && correlatedEditPreview->abilityNumber == 1);
        assert(correlatedEditPreview->tid == correlated->tid && correlatedEditPreview->sid == correlated->sid);
        assert(correlatedEditPreview->pid != correlatedPid);
        // The UI radar consumes calculatedStats from this exact preview; a Nature edit
        // must therefore refresh the visible battle-stat shape before Keep is pressed.
        assert(correlatedEditPreview->calculatedStats != correlated->calculatedStats);
        assert(editor->stageBoxPokemonEdit(0, 3, correlatedEdit, error));
        correlated = editor->boxedPokemon(0, 3, error);
        assert(correlated && correlated->encryptedBytes == correlatedEditPreview->encryptedBytes);

        // Unown's letter is the PID-derived Gen III form: the staged editor reaches it through a
        // PID re-roll that keeps Nature / Shiny, and refuses a letter for any other species.
        {
            BoxPokemonEdit unown;
            unown.species = 201;
            unown.form = 7; // H
            auto unownPreview = editor->previewEdit(0, 3, unown, error);
            assert(unownPreview && error.empty() && unownPreview->form == 7);
            assert(unownPreview->nature == correlated->nature && unownPreview->shiny == correlated->shiny);
            BoxPokemonEdit wrongSpecies;
            wrongSpecies.form = 3;
            assert(!editor->previewEdit(0, 3, wrongSpecies, error));
        }

        const auto beforeImpossible = editor->stagedBytes();
        BoxPokemonEdit impossibleGender;
        impossibleGender.species = 81; // Magnemite is genderless.
        impossibleGender.gender = 0;
        assert(!editor->stageBoxPokemonEdit(0, 3, impossibleGender, error));
        assert(editor->stagedBytes() == beforeImpossible);
        BoxPokemonEdit impossibleAbility;
        impossibleAbility.species = 25; // Pikachu has one Gen III ability.
        impossibleAbility.abilityNumber = 2;
        assert(!editor->stageBoxPokemonEdit(0, 3, impossibleAbility, error));
        assert(editor->stagedBytes() == beforeImpossible);
    }

    // Release is staged sparse removal; neighboring slots do not shift.
    const auto cloneBytes = clone->encryptedBytes;
    assert(editor->stageReleaseBoxPokemon(0, 0, error));
    assert(!editor->boxedPokemon(0, 0, error));
    auto neighbor = editor->boxedPokemon(0, 1, error);
    assert(neighbor && neighbor->encryptedBytes == cloneBytes);

    const auto finalized = editor->finalizedBytes(error);
    assert(!finalized.empty() && error.empty());
    assert(source == immutableSource);
    const auto selected = Detail::validateSlot(finalized, 1);
    assert(selected.valid && selected.counter == 9);
    assert(Detail::familyMatches(game, Detail::detectFamily(finalized, selected)));

    editor->discard();
    assert(editor->stagedBytes() == immutableSource);
    assert(!editor->hasPendingChanges());

    // Hardware regression: exact sparse slots, transactional carry, no overwrite/compaction.
    assert(editor->stageCloneBoxPokemon(0, 0, 0, 7, error));
    assert(editor->stageCloneBoxPokemon(0, 0, 0, 2, error));
    const auto beforeCarry = editor->stagedBytes();
    const auto changesBeforeCarry = editor->pendingChanges().size();
    const auto originalMon = editor->boxedPokemon(0, 0, error)->encryptedBytes;
    assert(!editor->beginSparseMove(0, {0,0}, error));
    assert(!editor->beginSparseMove(0, {0,30}, error));
    assert(editor->stagedBytes() == beforeCarry);
    assert(editor->beginSparseMove(0, {7,0}, error));
    assert(!editor->boxedPokemon(0, 0, error));
    assert(!editor->boxedPokemon(0, 7, error));
    assert(editor->boxedPokemon(0, 2, error)->encryptedBytes == originalMon);
    const auto carried = editor->stagedBytes();
    assert(!editor->placeSparseMove(0, 2, error)); // occupied target, whole group rejected
    assert(!editor->placeSparseMove(0, 5, error)); // row wrap
    assert(!editor->placeSparseMove(0, 29, error)); // bottom edge
    assert(!editor->stageReleaseBoxPokemon(0, 2, error));
    assert(editor->finalizedBytes(error).empty());
    assert(editor->stagedBytes() == carried);
    assert(editor->cancelSparseMove(error));
    assert(editor->stagedBytes() == beforeCarry);
    assert(editor->pendingChanges().size() == changesBeforeCarry);
    assert(editor->beginSparseMove(0, {0}, error));
    assert(editor->placeSparseMove(0, 5, error));
    assert(!editor->boxedPokemon(0, 0, error));
    assert(editor->boxedPokemon(0, 5, error)->encryptedBytes == originalMon);
    {
        // One Review row per Move, in player words (S04 follow-up, same as Gen IV/V).
        const auto& move = editor->pendingChanges().back();
        assert(move.label == "Move Pokemon" && move.beforeValue == "1 Pokemon" &&
               move.afterValue == "Box 1 / Slot 6");
    }
    assert(editor->boxedPokemon(0, 2, error)->encryptedBytes == originalMon);
    assert(editor->beginSparseMove(0, {7,2}, error));
    // First row-major source is 2; slot 7 is one row down, one column left.
    assert(!editor->placeSparseMove(1, 0, error));
    assert(editor->placeSparseMove(1, 2, error));
    assert(editor->boxedPokemon(1, 2, error)->encryptedBytes == originalMon);
    assert(editor->boxedPokemon(1, 7, error)->encryptedBytes == originalMon);
    assert(!editor->boxedPokemon(1, 3, error)); // shape hole preserved
    assert(!editor->boxedPokemon(0, 2, error));
    assert(!editor->boxedPokemon(0, 7, error));
    assert(editor->boxedPokemon(0, 5, error)->encryptedBytes == originalMon);
    assert(editor->originalBytes() == immutableSource && source == immutableSource);
    assert(!editor->finalizedBytes(error).empty());

    // Preview follows exact Create/Edit serialization, including safe deterministic PID,
    // native stats, gender/ability and origin, without touching staged bytes or pending changes.
    const auto beforePreview = editor->stagedBytes();
    const auto pendingBeforePreview = editor->pendingChanges().size();
    auto preview = editor->previewCreate(13, 29, create, error);
    assert(preview && preview->calculatedStats[0] > 0 && preview->originGame == origin(game));
    assert(editor->stagedBytes() == beforePreview);
    assert(editor->pendingChanges().size() == pendingBeforePreview);
    assert(editor->stageAddBoxPokemon(13, 29, create, error));
    assert(editor->boxedPokemon(13, 29, error)->encryptedBytes == preview->encryptedBytes);
    const auto beforeEditPreview = editor->stagedBytes();
    BoxPokemonEdit previewEdit;
    previewEdit.level = 45;
    auto editPreview = editor->previewEdit(13, 29, previewEdit, error);
    assert(editPreview && editPreview->calculatedStats != preview->calculatedStats);
    assert(editPreview->pid == preview->pid && editPreview->nature == preview->nature);
    assert(editPreview->gender == preview->gender && editPreview->shiny == preview->shiny);
    assert(editPreview->ability == preview->ability && editPreview->tid == preview->tid && editPreview->sid == preview->sid);
    assert(editor->stagedBytes() == beforeEditPreview);
    assert(editor->stageBoxPokemonEdit(13, 29, previewEdit, error));
    assert(editor->boxedPokemon(13, 29, error)->encryptedBytes == editPreview->encryptedBytes);
    editor->discard();

    auto rtcSource=source;
    const std::array<uint8_t,7> rtcFooter{{0x52,0x54,0x43,0x01,0x02,0x03,0x04}};
    rtcSource.insert(rtcSource.end(),rtcFooter.begin(),rtcFooter.end());
    auto rtcEditor=StagedPokemonEditor::create(rtcSource,game,error);
    assert(rtcEditor&&error.empty());
    const auto rtcFinal=rtcEditor->finalizedBytes(error);
    assert(rtcFinal==rtcSource);
    assert(rtcEditor->stageAddBoxPokemon(0,2,create,error));
    const auto rtcEdited=rtcEditor->finalizedBytes(error);
    assert(rtcEdited.size()==rtcSource.size());
    assert(rtcEdited!=rtcSource);
    assert(std::equal(rtcFooter.begin(),rtcFooter.end(),
                      rtcEdited.end()-static_cast<std::ptrdiff_t>(rtcFooter.size())));

    // Wrong family must fail closed.
    const SourceGame mismatch =
        family == Family::FRLG ? SourceGame::RubyGBA : SourceGame::FireRedGBA;
    assert(!StagedPokemonEditor::create(source, mismatch, error));

    // A single corrupt rotating save slot must remain recoverable from the other valid slot.
    auto oneBroken = source;
    oneBroken[Detail::kSlotBases[1] + 0xFF6] ^= 0xFF;
    assert(StagedPokemonEditor::create(oneBroken, game, error));

    // With both rotating save slots checksum-invalid there is no safe recovery candidate.
    auto broken = source;
    broken[Detail::kSlotBases[0] + 0xFF6] ^= 0xFF;
    broken[Detail::kSlotBases[1] + 0xFF6] ^= 0xFF;
    assert(!StagedPokemonEditor::create(broken, game, error));
}

} // namespace

std::vector<uint8_t> slurpFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

// Verified app-owned working-copy export for every exact Gen III game: strict reparse before and
// after writing, exact original backup, manifest provenance, fault cleanup, source immutability.
void exportGame(SourceGame game, Family family) {
    namespace fs = std::filesystem;
    using PokeVault::Integration::Common::ExportFailurePoint;
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-g3-export-" + std::to_string(::getpid()));
    fs::remove_all(root);
    auto source = fixture(family, game);
    const auto immutableSource = source;
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    assert(editor && error.empty());

    PokeVault::Integration::Gen3::Gen3ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(exactSourceGameId(game)) + "_20261014-120000";
    request.sourcePath = "sdmc:/retroarch/saves/fixture.srm";
    request.timestamp = "20261014-120000";
    auto none = PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request);
    assert(!none.success && none.error.find("No pending") != std::string::npos);

    BoxPokemonEdit edit; edit.nickname = "EXPORT"; edit.level = 42;
    assert(editor->stageBoxPokemonEdit(0, 0, edit, error));
    for (auto point : {ExportFailurePoint::AfterBackupWrite, ExportFailurePoint::AfterEditedWrite,
                       ExportFailurePoint::AfterReadbackVerify, ExportFailurePoint::AfterManifestWrite}) {
        auto faulty = request;
        faulty.failurePoint = point;
        assert(!PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, faulty).success);
        assert(fs::is_empty(root));
    }
    auto exported = PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request);
    assert(exported.success && exported.error.empty());
    const fs::path dir(exported.directory);
    assert(slurpFile(dir / "original_backup.sav") == immutableSource);
    const auto edited = slurpFile(dir / "edited.sav");
    assert(edited == editor->stagedBytes() && edited != immutableSource);
    auto reparsed = parse(edited, game);
    assert(reparsed);
    auto reopened = StagedPokemonEditor::create(edited, game, error);
    assert(reopened);
    const auto mon = reopened->boxedPokemon(0, 0, error);
    assert(mon && mon->nickname == "EXPORT" && mon->level == 42);
    std::ifstream manifestIn(dir / "EDIT_MANIFEST.txt");
    const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
    assert(manifest.find(std::string("GAME_ID=") + exactSourceGameId(game)) != std::string::npos);
    assert(manifest.find("SOURCE_SHA256=" + exported.originalSha256) != std::string::npos);
    assert(manifest.find("- Box 1 slot 1 ") != std::string::npos);

    // Bag edits live in a separate staged workspace; one export carries both, merged by sector.
    auto bag = StagedInventoryEditor::create(source, game, error);
    assert(bag && error.empty());
    assert(bag->stageInventoryQuantity(InventoryPouch::Items, 13, 7, error));
    auto combined = PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request, bag.get());
    assert(combined.success && combined.directory != exported.directory);
    const auto both = slurpFile(fs::path(combined.directory) / "edited.sav");
    auto bothParsed = parse(both, game);
    assert(bothParsed);
    bool sawPotion = false;
    for (const auto& pouch : bothParsed.save->inventory())
        for (const auto& item : pouch.items)
            if (pouch.pouch == InventoryPouch::Items && item.itemId == 13 && item.count == 7) sawPotion = true;
    assert(sawPotion);
    auto bothReopened = StagedPokemonEditor::create(both, game, error);
    const auto bothMon = bothReopened ? bothReopened->boxedPokemon(0, 0, error) : std::nullopt;
    assert(bothMon && bothMon->nickname == "EXPORT");

    // A Bag-only export works too; a workspace opened from a different save is refused.
    editor->discard();
    assert(PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request, bag.get()).success);
    auto other = fixture(family, game);
    other[0x10] ^= 1;  // different original bytes (outside any validated sector payload use)
    auto foreign = StagedInventoryEditor::create(other, game, error);
    if (foreign && foreign->stageInventoryQuantity(InventoryPouch::Items, 13, 2, error))
        assert(!PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request, foreign.get()).success);
    error.clear();
    assert(source == immutableSource);
    fs::remove_all(root);
}

// Phase 4: Box and Bag edits exported SEPARATELY through the real transaction. A Box export may
// only change active-slot PC-storage sectors (logical ids 5..13: payload and their checksum); a
// Bag export may only change active-slot non-storage sectors. The inactive slot, every other
// sector and the tail of the 128 KiB image stay byte-identical; Party (logical sector 1) is never
// written by a Box export. A foreign Bag workspace, racing destinations and a mid-publication
// failure are refused without residue; malformed images never open a working copy.
void phase4Game(SourceGame game, Family family) {
    namespace fs = std::filesystem;
    using PokeVault::Integration::Common::ExportFailurePoint;
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-g3-phase4-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    auto entries = [&] {
        return static_cast<size_t>(std::distance(fs::directory_iterator(root), fs::directory_iterator()));
    };
    const auto source = fixture(family, game);
    const auto parsed = parse(source, game);
    assert(parsed);
    const uint8_t active = parsed.save->metadata().activeSlot;
    const auto slot = Detail::validateSlot(source, active);
    assert(slot.valid);
    // Logical sector id owning a byte of the image, or -1 outside the active slot.
    auto sectorOf = [&](size_t i) -> int {
        for (size_t id = 0; id < Detail::kSectorCount; ++id) {
            const size_t begin = slot.logicalSectorOffsets[id];
            if (i >= begin && i < begin + Detail::kSectorSize) return static_cast<int>(id);
        }
        return -1;
    };
    auto checkFootprint = [&](const std::vector<uint8_t>& edited, bool storageOnly) {
        assert(edited.size() == source.size());
        bool changed = false;
        for (size_t i = 0; i < edited.size(); ++i) {
            if (edited[i] == source[i]) continue;
            const int id = sectorOf(i);
            assert(id >= 0); // never the inactive slot or the image tail
            assert(storageOnly ? id >= 5 : id < 5);
            changed = true;
        }
        assert(changed);
    };
    PokeVault::Integration::Gen3::Gen3ExportRequest request;
    request.rootDirectory = root.string();
    request.sourcePath = "sdmc:/retroarch/saves/fixture.srm";
    request.timestamp = "20261015-120000";
    std::string error;

    {   // Box only
        auto editor = StagedPokemonEditor::create(source, game, error);
        assert(editor);
        BoxPokemonEdit edit; edit.level = 33;
        assert(editor->stageBoxPokemonEdit(0, 0, edit, error));
        request.directoryStem = "box";
        for (auto point : {ExportFailurePoint::AfterFirstPublishedFile,
                           ExportFailurePoint::DestinationAppearsAsEmptyDirectory,
                           ExportFailurePoint::DestinationAppearsAsFile}) {
            auto faulty = request;
            faulty.failurePoint = point;
            assert(!PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, faulty).success);
            if (point != ExportFailurePoint::AfterFirstPublishedFile) {
                assert(entries() == 1 && fs::exists(root / "box") && !fs::is_directory(root / "box/edited.sav"));
                fs::remove(root / "box");
            }
            assert(entries() == 0);
        }
        auto exported = PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request);
        assert(exported.success);
        const auto edited = slurpFile(fs::path(exported.directory) / "edited.sav");
        checkFootprint(edited, true);
        auto reopened = parse(edited, game);
        assert(reopened && reopened.save->party().size() == parsed.save->party().size());
        fs::remove_all(exported.directory);
    }
    {   // Bag only
        auto editor = StagedPokemonEditor::create(source, game, error);
        auto bag = StagedInventoryEditor::create(source, game, error);
        assert(editor && bag && bag->stageInventoryQuantity(InventoryPouch::Items, 13, 5, error));
        request.directoryStem = "bag";
        auto exported = PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request, bag.get());
        assert(exported.success);
        checkFootprint(slurpFile(fs::path(exported.directory) / "edited.sav"), false);
        fs::remove_all(exported.directory);

        // A Bag workspace from a different, fully valid save (another save counter) is refused.
        std::vector<uint8_t> other(Detail::kSaveSize, 0);
        writeSlot(other, 0, 11, 2, family, game);
        writeSlot(other, 1, 9, 5, family, game);
        auto foreign = StagedInventoryEditor::create(other, game, error);
        assert(foreign && foreign->stageInventoryQuantity(InventoryPouch::Items, 13, 2, error));
        request.directoryStem = "foreign";
        auto refused = PokeVault::Integration::Gen3::publishVerifiedGen3Export(*editor, request, foreign.get());
        assert(!refused.success && refused.error.find("different saves") != std::string::npos);
        assert(entries() == 0);
    }
    {   // Malformed images
        auto truncated = source; truncated.resize(source.size() - 1);
        assert(!StagedPokemonEditor::create(truncated, game, error));
        auto oversized = source; oversized.push_back(0);
        assert(!StagedPokemonEditor::create(oversized, game, error));
        assert(!StagedPokemonEditor::create(std::vector<uint8_t>{}, game, error));
        auto broken = source;
        for (uint8_t s = 0; s < 2; ++s)  // both copies' storage sector payloads corrupted
            broken[Detail::validateSlot(source, s).logicalSectorOffsets[5] + 0x10] ^= 0x5A;
        assert(!StagedPokemonEditor::create(broken, game, error));
    }
    fs::remove_all(root);
}

int main() {
    // A PK3 egg stores language=Japanese as a placeholder, so its OT charset comes from the
    // containing save. clone() must carry that non-byte context or a Japanese OT becomes Latin.
    {
        std::array<uint8_t, 80> blank{};
        Pokemon::Pokemon3FRLG japaneseEgg(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
        japaneseEgg.setSaveLanguage(1);
        japaneseEgg.setOTName(u"ア");
        japaneseEgg.setEgg(true);
        assert(japaneseEgg.otName() == u"ア");
        auto cloned = japaneseEgg.clone();
        assert(cloned && cloned->otName() == u"ア");
    }

    {
        const auto raw=samplePokemon(SourceGame::FireRedGBA);
        Pokemon::Pokemon3FRLG egg(std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(raw.data()),raw.size()));
        egg.setEgg(true);
        assert(egg.isEgg());
        assert((egg.rd8(0x13)&0x04)!=0);
        assert(egg.language()==1);
        assert(egg.nickname()==u"\u30bf\u30de\u30b4");
        // Upstream PKSE 9a9a731 / PKHeX PK3.Nickname: while the egg bit is set the name stays
        // the placeholder, whatever is written.
        egg.setNickname(u"BOB");
        assert(egg.nickname()==u"\u30bf\u30de\u30b4");
        assert(egg.language()==1);
        egg.setEgg(false);
        assert(!egg.isEgg());
        assert((egg.rd8(0x13)&0x04)==0);
        egg.setNickname(u"\u30dc\u30d6");
        assert(egg.nickname()==u"\u30dc\u30d6");
    }

    // Phase 3 #7: a Pokemon that already holds an item that was never released as a held item
    // (Safari Ball 5 / Berry Juice 44, PKHeX ItemStorage3RS.Unreleased) keeps it through an
    // unrelated edit on the real Edit -> Keep route; only choosing it anew is refused.
    for (const uint16_t unreleased : {uint16_t{5}, uint16_t{44}}) {
        gSampleHeldItem = unreleased;
        const auto source = fixture(Family::FRLG, SourceGame::FireRedGBA);
        gSampleHeldItem = 13;
        std::string error;
        auto editor = StagedPokemonEditor::create(source, SourceGame::FireRedGBA, error);
        assert(editor && error.empty());
        const auto record = editor->boxedPokemon(0, 0, error);
        assert(record && record->heldItem == unreleased);
        PokeBank::UIModel::Gen3SharedEditor::Session draft;
        draft.begin(*record, PokeBank::UIModel::Gen3SharedEditor::Mode::Edit);
        draft.working.nickname = "SAFARI";
        assert(draft.keep(*editor, 0, 0, error));
        const auto bytes = editor->finalizedBytes(error);
        assert(!bytes.empty());
        auto reopened = StagedPokemonEditor::create(bytes, SourceGame::FireRedGBA, error);
        assert(reopened);
        const auto kept = reopened->boxedPokemon(0, 0, error);
        assert(kept && kept->nickname == "SAFARI" && kept->heldItem == unreleased);
        // Choosing it as a new held item is still refused, and refusal changes nothing.
        auto picker = StagedPokemonEditor::create(source, SourceGame::FireRedGBA, error);
        BoxPokemonEdit pick;
        pick.heldItem = unreleased;
        assert(!picker->stageBoxPokemonEdit(0, 0, pick, error) && picker->stagedBytes() == source);
    }

    // A staged edit may not rename an Egg or change its language; the save stays unchanged.
    for (const auto& [game, family] : {std::pair{SourceGame::FireRedGBA, Family::FRLG},
                                       std::pair{SourceGame::EmeraldGBA, Family::Emerald}}) {
        gSampleIsEgg = true;
        const auto source = fixture(family, game);
        gSampleIsEgg = false;
        std::string error;
        auto editor = StagedPokemonEditor::create(source, game, error);
        assert(editor && error.empty());
        const auto before = editor->stagedBytes();

        BoxPokemonEdit rename;
        rename.nickname = "BOB";
        assert(!editor->stageBoxPokemonEdit(0, 0, rename, error));
        assert(error.find("Egg") != std::string::npos);
        assert(editor->stagedBytes() == before && !editor->hasPendingChanges());

        BoxPokemonEdit relanguage;
        relanguage.language = 2;
        error.clear();
        assert(!editor->stageBoxPokemonEdit(0, 0, relanguage, error));
        assert(error.find("Egg") != std::string::npos);
        assert(editor->stagedBytes() == before && !editor->hasPendingChanges());

        // Restating the Egg's own name and language alongside another field is not a change.
        BoxPokemonEdit unchangedName;
        unchangedName.nickname = "\xE3\x82\xBF\xE3\x83\x9E\xE3\x82\xB4";  // タマゴ in UTF-8
        unchangedName.language = 1;
        unchangedName.friendship = 120;
        error.clear();
        assert(editor->stageBoxPokemonEdit(0, 0, unchangedName, error));
        assert(error.empty());
    }
    runGame(SourceGame::RubyGBA, Family::RS);
    runGame(SourceGame::SapphireGBA, Family::RS);
    runGame(SourceGame::EmeraldGBA, Family::Emerald);
    runGame(SourceGame::FireRedGBA, Family::FRLG);
    runGame(SourceGame::LeafGreenGBA, Family::FRLG);
    exportGame(SourceGame::RubyGBA, Family::RS);
    exportGame(SourceGame::SapphireGBA, Family::RS);
    exportGame(SourceGame::EmeraldGBA, Family::Emerald);
    exportGame(SourceGame::FireRedGBA, Family::FRLG);
    exportGame(SourceGame::LeafGreenGBA, Family::FRLG);
    phase4Game(SourceGame::RubyGBA, Family::RS);
    phase4Game(SourceGame::SapphireGBA, Family::RS);
    phase4Game(SourceGame::EmeraldGBA, Family::Emerald);
    phase4Game(SourceGame::FireRedGBA, Family::FRLG);
    phase4Game(SourceGame::LeafGreenGBA, Family::FRLG);
    std::cout << "Generation III staged boxed Pokemon editor: PASS\n";
}
