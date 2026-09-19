#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
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
    assert(!StagedPokemonEditor::pidCorrelatedEditingEnabled());
    assert(!StagedPokemonEditor::moveSelectionEditingEnabled());

    auto before = editor->boxedPokemon(0, 0, error);
    assert(before && error.empty());
    assert(before->species == 25);
    assert(before->level == 20);
    assert(before->moves[0] == 33);
    assert(before->pp[0] == 40);
    const uint32_t pid = before->pid;
    const uint16_t tid = before->tid;
    const uint16_t sid = before->sid;
    const uint8_t nature = before->nature;
    const bool shiny = before->shiny;

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
    create.nickname = "EEVEE";
    assert(editor->stageAddBoxPokemon(0, 2, create, error));
    auto created = editor->boxedPokemon(0, 2, error);
    assert(created && created->species == 133 && created->level == 10);
    assert(created->originGame == origin(game));
    assert((created->moves == std::array<uint16_t,4>{}));
    assert(!created->shiny);

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
