#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "UI/Gen1PokemonPresentation.h"
#include "Integration/Gen1/Gen1Shiny.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace PokeVault::Integration::Gen1;

namespace {
uint8_t checksum(const std::vector<uint8_t>& bytes, size_t start, size_t count) {
    unsigned sum = 0;
    for (size_t i = start; i < start + count; ++i) sum += bytes[i];
    return static_cast<uint8_t>((~sum) & 0xFF);
}

size_t bank(size_t box) { return (box < 6 ? 0x4000 : 0x6000) + (box % 6) * 0x462; }

void repair(std::vector<uint8_t>& bytes) {
    bytes[0x3523] = checksum(bytes, 0x2598, 0xF8B);
    for (size_t base : {size_t(0x4000), size_t(0x6000)}) {
        bytes[base + 0x1A4C] = checksum(bytes, base, 0x1A4C);
        for (size_t i = 0; i < 6; ++i)
            bytes[base + 0x1A4D + i] = checksum(bytes, base + i * 0x462, 0x462);
    }
}

std::vector<uint8_t> fixture() {
    std::vector<uint8_t> bytes(0x8000, 0);
    bytes[0x2598] = 0x91; bytes[0x2599] = 0x84; bytes[0x259A] = 0x83; bytes[0x259B] = 0x50; // RED
    bytes[0x2605] = 0x12; bytes[0x2606] = 0x34;
    bytes[0x25CA] = bytes[0x27E7] = bytes[0x2F2D] = 0xFF;
    bytes[0x284C] = 0x82; // boxes initialized, current box 2
    bytes[0x29C3] = 0x99;
    for (size_t b = 0; b < 12; ++b) bytes[bank(b) + 1] = 0xFF;

    const size_t box = 0x30C0, body = box + 22;
    bytes[box] = 1; bytes[box + 1] = 0x54; bytes[box + 2] = 0xFF; // Pikachu
    bytes[body] = 0x54; bytes[body + 1] = 0; bytes[body + 2] = 20; bytes[body + 3] = 5;
    bytes[body + 5] = bytes[body + 6] = 23; bytes[body + 7] = 190;
    bytes[body + 8] = 56; // deliberately weird/incompatible Hydro Pump source move
    bytes[body + 12] = 0x12; bytes[body + 13] = 0x34; bytes[body + 16] = 125;
    bytes[body + 27] = 0x88; bytes[body + 28] = 0x88;
    bytes[body + 29] = 5; // PP byte for the weird move; must survive unrelated edits exactly.
    std::fill_n(bytes.begin() + box + 682, 11, 0x50);
    bytes[box + 682] = 0x91; bytes[box + 683] = 0x84; bytes[box + 684] = 0x83;
    std::fill_n(bytes.begin() + box + 902, 11, 0x50);
    bytes[box + 902] = 0x8F; bytes[box + 903] = 0x88; bytes[box + 904] = 0x8A; bytes[box + 905] = 0x80;
    std::copy_n(bytes.begin() + box, 0x462, bytes.begin() + bank(2));
    repair(bytes);
    return bytes;
}

std::array<uint8_t,4> stored(const PokemonRecord& p) {
    return {p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
}
} // namespace

int main() {
    auto raw = fixture();
    const auto parsed = parse(raw, SourceGame::Red);
    assert(parsed && parsed.save);
    std::string error;
    auto editor = StagedPokemonEditor::create(*parsed.save, error);
    assert(editor && error.empty());

    // Preservation boundary: an existing hacked/unusual move is visible and remains byte-truthful
    // when an unrelated field is edited. Compatibility filtering applies only to new/replacement picks.
    const auto source = editor->boxedPokemon(2, 0, error);
    assert(source && source->moves[0] == 56);
    assert(!MoveCompatibility::canLearnMove(SourceGame::Red, source->species, source->moves[0]));
    const auto sourceMoves = source->moves;
    const auto sourcePP = source->pp;
    const auto sourcePPUps = source->ppUps;
    BoxPokemonEdit nicknameOnly;
    nicknameOnly.nickname = "Sparky";
    assert(editor->stageEdit(2, 0, nicknameOnly, error));
    const auto renamed = editor->boxedPokemon(2, 0, error);
    assert(renamed);
    assert(renamed->moves == sourceMoves);
    assert(renamed->pp == sourcePP);
    assert(renamed->ppUps == sourcePPUps);

    // New shiny Charizard uses only real Gen I DVs. No shiny flag or extension is serialized.
    BoxPokemonCreate create;
    create.species = 6;
    create.level = 36;
    create.nickname = "Charizard";
    create.otName = "RED";
    create.moves = {53,0,0,0}; // Flamethrower is compatible in Red.
    create.pp = {15,0,0,0};
    create.ppUps = {0,0,0,0};
    create.dvs = ShinyDVs::makeShiny({8,8,8,8});
    const auto shinyDVs = create.dvs;
    assert(ShinyDVs::isShiny(shinyDVs));
    assert(!MoveCompatibility::firstIncompatible(SourceGame::Red, create.species, create.moves));
    assert(editor->stageAdd(2, 1, create, error));
    const auto staged = editor->boxedPokemon(2, 1, error);
    assert(staged && staged->species == 6);
    const auto draftPresentation = PokeBank::UIModel::presentGen1Pokemon(
        create.species, create.level, create.dvs, create.statExperience);
    const auto stagedPresentation = PokeBank::UIModel::presentGen1Pokemon(*staged);
    assert(draftPresentation.shiny && stagedPresentation.shiny);
    assert(draftPresentation.battleStats == stagedPresentation.battleStats);
    assert(stored(*staged) == shinyDVs);
    assert(ShinyDVs::isShiny(stored(*staged)));

    // Exercise the actual serialization boundary: staged bytes -> finalized save -> strict normal parser.
    const auto finalized = editor->finalizedBytes(error);
    assert(!finalized.empty() && error.empty());
    const auto reparsed = parse(finalized, SourceGame::Red);
    assert(reparsed && reparsed.save);
    const auto reparsedShiny = reparsed.save->boxes()[2].slots[1];
    assert(reparsedShiny);
    assert(PokeBank::UIModel::presentGen1Pokemon(*reparsedShiny).battleStats == draftPresentation.battleStats);
    assert(stored(*reparsedShiny) == shinyDVs);
    assert(ShinyDVs::isShiny(stored(*reparsedShiny)));

    // Same-save Clone preserves the exact four stored DVs and therefore the derived shiny identity.
    // There is no normalization/randomization and the source staged record remains identical.
    assert(editor->stageClone(2, 1, 2, 2, error));
    const auto clone = editor->boxedPokemon(2, 2, error);
    const auto originalStaged = editor->boxedPokemon(2, 1, error);
    assert(clone && originalStaged);
    assert(stored(*originalStaged) == shinyDVs);
    assert(stored(*clone) == shinyDVs);
    assert(ShinyDVs::isShiny(stored(*clone)));
    assert(clone->dvs[0] == StagedPokemonEditor::derivedHPDV(shinyDVs));

    // A known incompatible new draft is rejected by the normal Create/Edit compatibility model.
    // The staged core remains preservation-first for already-existing unusual data.
    const std::array<uint8_t,4> badMoves{53,56,0,0};
    const auto bad = MoveCompatibility::firstIncompatible(SourceGame::Red, 6, badMoves);
    assert(bad && *bad == 1);

    // The immutable source object is byte-identical throughout edit/add/clone/finalize operations.
    assert(std::equal(parsed.save->sourceBytes().begin(), parsed.save->sourceBytes().end(), raw.begin()));

    std::cout << "Gen I Cleanup #3 staged semantics: PASS (strict shiny round-trip/clone, weird-source preservation)\n";
    return 0;
}
