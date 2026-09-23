#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include "Pokemon/Experience.h"
#include "Pokemon/Pokemon3FRLG.h"
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
    p.setHeldItem(13);
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
    assert(editor->boxCount() == 14);
    assert(editor->boxCapacity() == 30);
    assert(StagedPokemonEditor::pidCorrelatedEditingEnabled());
    assert(!StagedPokemonEditor::moveSelectionEditingEnabled());

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
        identity->discard(); assert(identity->stagedBytes() == immutableSource);
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
    edit.metLocation = 2;
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
    create.metLocation = family == Family::FRLG ? 1 : 57;
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

int main() {
    runGame(SourceGame::RubyGBA, Family::RS);
    runGame(SourceGame::SapphireGBA, Family::RS);
    runGame(SourceGame::EmeraldGBA, Family::Emerald);
    runGame(SourceGame::FireRedGBA, Family::FRLG);
    runGame(SourceGame::LeafGreenGBA, Family::FRLG);
    std::cout << "Generation III staged boxed Pokemon editor: PASS\n";
}
