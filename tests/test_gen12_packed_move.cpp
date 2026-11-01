#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "tests/fixtures/gsc_pokemon_fixture.h"
#include "UI/NarrowPokemonActionMenu.h"

#include <fstream>
#include <iterator>
#include <string_view>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace G1 = PokeVault::Integration::Gen1;
namespace G2 = PokeVault::Integration::Gen2;

namespace Gen1Fixture {

uint8_t checksum(const std::vector<uint8_t>& bytes, std::size_t start, std::size_t length) {
    unsigned sum = 0;
    for (std::size_t i = start; i < start + length; ++i) sum += bytes[i];
    return static_cast<uint8_t>((~sum) & 0xFF);
}

std::size_t bank(std::size_t box) {
    return (box < 6 ? 0x4000 : 0x6000) + (box % 6) * 0x462;
}

void repair(std::vector<uint8_t>& bytes) {
    bytes[0x3523] = checksum(bytes, 0x2598, 0xF8B);
    if (bytes[0x284C] & 0x80) {
        for (std::size_t base : {std::size_t(0x4000), std::size_t(0x6000)}) {
            bytes[base + 0x1A4C] = checksum(bytes, base, 0x1A4C);
            for (std::size_t i = 0; i < 6; ++i)
                bytes[base + 0x1A4D + i] = checksum(bytes, base + i * 0x462, 0x462);
        }
    }
}

std::vector<uint8_t> make(G1::SourceGame game) {
    std::vector<uint8_t> bytes(0x8000, 0);
    bytes[0x2598] = 0x91;
    bytes[0x2599] = 0x84;
    bytes[0x259A] = 0x83;
    bytes[0x259B] = 0x50;
    bytes[0x2605] = 0x12;
    bytes[0x2606] = 0x34;
    bytes[0x25CA] = bytes[0x27E7] = bytes[0x2F2D] = 0xFF;
    bytes[0x284C] = 0x82;
    bytes[0x29C3] = game == G1::SourceGame::Yellow ? 0x54 : 0x99;
    for (std::size_t box = 0; box < 12; ++box) bytes[bank(box) + 1] = 0xFF;

    constexpr std::size_t current = 0x30C0;
    const std::size_t body = current + 22;
    bytes[current] = 1;
    bytes[current + 1] = 0x54;
    bytes[current + 2] = 0xFF;
    bytes[body] = 0x54;
    bytes[body + 2] = 20;
    bytes[body + 3] = 5;
    bytes[body + 5] = bytes[body + 6] = 23;
    bytes[body + 7] = 190;
    bytes[body + 8] = 33;
    bytes[body + 12] = 0x12;
    bytes[body + 13] = 0x34;
    bytes[body + 16] = 125;
    bytes[body + 27] = 0x88;
    bytes[body + 28] = 0x88;
    bytes[body + 29] = 35;

    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(current + 682), 11, 0x50);
    bytes[current + 682] = 0x91;
    bytes[current + 683] = 0x84;
    bytes[current + 684] = 0x83;
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(current + 902), 11, 0x50);
    bytes[current + 902] = 0x8F;
    bytes[current + 903] = 0x88;
    bytes[current + 904] = 0x8A;
    bytes[current + 905] = 0x80;

    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(current), 0x462,
                bytes.begin() + static_cast<std::ptrdiff_t>(bank(2)));
    repair(bytes);
    return bytes;
}

} // namespace Gen1Fixture

std::vector<uint8_t> staged(const G1::StagedPokemonEditor& editor) {
    return {editor.stagedBytes().begin(), editor.stagedBytes().end()};
}
std::vector<uint8_t> staged(const G2::StagedEditor& editor) {
    return {editor.stagedBytes().begin(), editor.stagedBytes().end()};
}

void assertOriginal(const G1::StagedPokemonEditor& editor, const std::vector<uint8_t>& raw) {
    assert(editor.originalBytes().size() == raw.size());
    assert(std::equal(editor.originalBytes().begin(), editor.originalBytes().end(), raw.begin()));
}
void assertOriginal(const G2::StagedEditor& editor, const std::vector<uint8_t>& raw) {
    assert(editor.originalBytes().size() == raw.size());
    assert(std::equal(editor.originalBytes().begin(), editor.originalBytes().end(), raw.begin()));
}

std::unique_ptr<G1::StagedPokemonEditor> makeGen1Editor(const std::vector<uint8_t>& raw,
                                                        G1::SourceGame game) {
    const auto parsed = G1::parse(raw, game);
    assert(parsed);
    std::string error;
    auto editor = G1::StagedPokemonEditor::create(*parsed.save, error);
    assert(editor && error.empty());
    return editor;
}

void addGen1(G1::StagedPokemonEditor& editor, std::size_t box, const std::string& nickname,
             uint16_t species = 1) {
    std::string error;
    const auto slot = editor.appendSlot(box, error);
    assert(slot);
    G1::BoxPokemonCreate create;
    create.species = species;
    create.nickname = nickname;
    create.otName = "RED";
    assert(editor.stageAdd(box, *slot, create, error));
}

void setupGen1Five(G1::StagedPokemonEditor& editor) {
    addGen1(editor, 2, "B", 1);
    addGen1(editor, 2, "C", 4);
    addGen1(editor, 2, "D", 7);
    addGen1(editor, 2, "E", 10);
}

std::vector<std::string> gen1Order(G1::StagedPokemonEditor& editor, std::size_t box) {
    std::vector<std::string> result;
    std::string error;
    for (std::size_t slot = 0; slot < 20; ++slot) {
        auto pokemon = editor.boxedPokemon(box, slot, error);
        assert(error.empty());
        if (!pokemon) break;
        result.push_back(pokemon->nickname);
    }
    return result;
}

void assertGen1Packed(G1::StagedPokemonEditor& editor) {
    std::string error;
    for (std::size_t box = 0; box < 12; ++box) {
        bool seenEmpty = false;
        for (std::size_t slot = 0; slot < 20; ++slot) {
            const auto pokemon = editor.boxedPokemon(box, slot, error);
            assert(error.empty());
            if (!pokemon) seenEmpty = true;
            else assert(!seenEmpty);
        }
    }
}

std::size_t gen1Total(G1::StagedPokemonEditor& editor) {
    std::size_t total = 0;
    std::string error;
    for (std::size_t box = 0; box < 12; ++box)
        for (std::size_t slot = 0; slot < 20; ++slot)
            if (editor.boxedPokemon(box, slot, error)) ++total;
    return total;
}

void runGen1(G1::SourceGame game) {
    const auto raw = Gen1Fixture::make(game);
    std::string error;

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto before = editor->boxedPokemon(2, 1, error);
        assert(before);
        const auto total = gen1Total(*editor);
        assert(editor->beginPackedMove(2, 1, error));
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"PIKA", "C", "D", "E"}));
        std::size_t placed = 99;
        assert(editor->placePackedMove(2, 3, placed, error) && placed == 3);
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"PIKA", "C", "D", "B", "E"}));
        const auto moved = editor->boxedPokemon(2, 3, error);
        assert(moved && moved->rawBody == before->rawBody &&
               moved->nickname == before->nickname &&
               moved->originalTrainer == before->originalTrainer);
        assert(gen1Total(*editor) == total);
        assertGen1Packed(*editor);
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        assert(editor->beginPackedMove(2, 3, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(2, 1, placed, error) && placed == 1);
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"PIKA", "D", "B", "C", "E"}));
        assertGen1Packed(*editor);
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        assert(editor->beginPackedMove(2, 0, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(2, 19, placed, error) && placed == 4);
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"B", "C", "D", "E", "PIKA"}));
        assertGen1Packed(*editor);
    }

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        assert(editor->beginPackedMove(2, 4, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(2, 0, placed, error) && placed == 0);
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"E", "PIKA", "B", "C", "D"}));
        assertGen1Packed(*editor);
    }

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto before = staged(*editor);
        assert(editor->beginPackedMove(2, 2, error));
        assert(editor->packedMoveActive());
        assert(editor->cancelPackedMove(error));
        assert(!editor->packedMoveActive());
        assert(staged(*editor) == before);
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"PIKA", "B", "C", "D", "E"}));
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto total = gen1Total(*editor);
        const auto before = editor->boxedPokemon(2, 2, error);
        assert(before && editor->beginPackedMove(2, 2, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(11, 0, placed, error) && placed == 0);
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"PIKA", "B", "D", "E"}));
        assert((gen1Order(*editor, 11) == std::vector<std::string>{"C"}));
        const auto moved = editor->boxedPokemon(11, 0, error);
        assert(moved && moved->rawBody == before->rawBody);
        assert(gen1Total(*editor) == total);
        assertGen1Packed(*editor);
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        for (int i = 0; i < 20; ++i) addGen1(*editor, 11, "F", 25);
        const auto beforePickup = staged(*editor);
        assert(editor->beginPackedMove(2, 2, error));
        const auto afterPickup = staged(*editor);
        std::size_t placed = 99;
        assert(!editor->placePackedMove(11, 0, placed, error));
        assert(error == "This Generation I box is full");
        assert(editor->packedMoveActive());
        assert(staged(*editor) == afterPickup);
        assert(editor->cancelPackedMove(error));
        assert(staged(*editor) == beforePickup);
        assertOriginal(*editor, raw);

        const auto addSlot = editor->appendSlot(10, error);
        assert(addSlot && *addSlot == 0);
        G1::BoxPokemonCreate create;
        create.nickname = "NEW";
        create.otName = "RED";
        assert(editor->stageAdd(10, *addSlot, create, error));
    }
}

std::unique_ptr<G2::StagedEditor> makeGen2Editor(const std::vector<uint8_t>& raw,
                                                 G2::SourceGame game) {
    const auto parsed = G2::parse(raw, game);
    assert(parsed);
    std::string error;
    auto editor = G2::StagedEditor::create(*parsed.save, error);
    assert(editor && error.empty());
    return editor;
}

void addGen2(G2::StagedEditor& editor, std::size_t box, const std::string& nickname,
             uint16_t species = 25) {
    G2::BoxPokemonCreate create;
    create.species = species;
    create.level = 15;
    create.nickname = nickname;
    create.otName = "ASH";
    std::size_t slot = 99;
    std::string error;
    assert(editor.stageAddBoxPokemon(box, create, slot, error));
}

void setupGen2Five(G2::StagedEditor& editor) {
    addGen2(editor, 0, "B");
    addGen2(editor, 0, "C");
    addGen2(editor, 0, "D");
    addGen2(editor, 0, "E");
}

std::vector<std::string> gen2Order(G2::StagedEditor& editor, std::size_t box) {
    std::vector<std::string> result;
    for (std::size_t slot = 0; slot < editor.metadata().boxCapacity; ++slot) {
        std::string error;
        auto pokemon = editor.boxedPokemon(box, slot, error);
        if (!pokemon) break;
        result.push_back(pokemon->nickname);
    }
    return result;
}

void assertGen2Packed(G2::StagedEditor& editor) {
    for (std::size_t box = 0; box < editor.metadata().boxCount; ++box) {
        bool seenEmpty = false;
        for (std::size_t slot = 0; slot < editor.metadata().boxCapacity; ++slot) {
            std::string error;
            const auto pokemon = editor.boxedPokemon(box, slot, error);
            if (!pokemon) seenEmpty = true;
            else assert(!seenEmpty);
        }
    }
}

std::size_t gen2Total(G2::StagedEditor& editor) {
    std::size_t total = 0;
    for (std::size_t box = 0; box < editor.metadata().boxCount; ++box)
        for (std::size_t slot = 0; slot < editor.metadata().boxCapacity; ++slot) {
            std::string error;
            if (editor.boxedPokemon(box, slot, error)) ++total;
        }
    return total;
}

void runGen2(const L& layoutFixture, G2::SourceGame game) {
    const auto raw = fixture(layoutFixture);
    std::string error;

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        const auto before = editor->boxedPokemon(0, 1, error);
        assert(before);
        const auto total = gen2Total(*editor);
        assert(editor->beginPackedMove(0, 1, error));
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"PIKA", "C", "D", "E"}));
        std::size_t placed = 99;
        assert(editor->placePackedMove(0, 3, placed, error) && placed == 3);
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"PIKA", "C", "D", "B", "E"}));
        const auto moved = editor->boxedPokemon(0, 3, error);
        assert(moved && moved->rawBody == before->rawBody &&
               moved->nickname == before->nickname &&
               moved->originalTrainer == before->originalTrainer);
        assert(gen2Total(*editor) == total);
        assertGen2Packed(*editor);
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        assert(editor->beginPackedMove(0, 3, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(0, 1, placed, error) && placed == 1);
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"PIKA", "D", "B", "C", "E"}));
        assertGen2Packed(*editor);
    }

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        assert(editor->beginPackedMove(0, 0, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(0, 19, placed, error) && placed == 4);
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"B", "C", "D", "E", "PIKA"}));
        assertGen2Packed(*editor);
    }

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        assert(editor->beginPackedMove(0, 4, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(0, 0, placed, error) && placed == 0);
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"E", "PIKA", "B", "C", "D"}));
        assertGen2Packed(*editor);
    }

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        const auto before = staged(*editor);
        assert(editor->beginPackedMove(0, 2, error));
        assert(editor->cancelPackedMove(error));
        assert(staged(*editor) == before);
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"PIKA", "B", "C", "D", "E"}));
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        const auto total = gen2Total(*editor);
        const auto before = editor->boxedPokemon(0, 2, error);
        assert(before && editor->beginPackedMove(0, 2, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(2, 0, placed, error) && placed == 0);
        assert((gen2Order(*editor, 0) == std::vector<std::string>{"PIKA", "B", "D", "E"}));
        assert((gen2Order(*editor, 2) == std::vector<std::string>{"C"}));
        const auto moved = editor->boxedPokemon(2, 0, error);
        assert(moved && moved->rawBody == before->rawBody);
        assert(gen2Total(*editor) == total);
        assertGen2Packed(*editor);
        assertOriginal(*editor, raw);
    }

    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        for (int i = 1; i < 20; ++i) addGen2(*editor, 1, "F");
        const auto beforePickup = staged(*editor);
        assert(editor->beginPackedMove(0, 2, error));
        const auto afterPickup = staged(*editor);
        std::size_t placed = 99;
        assert(!editor->placePackedMove(1, 0, placed, error));
        assert(error == "This Generation II box is full");
        assert(editor->packedMoveActive());
        assert(staged(*editor) == afterPickup);
        assert(editor->cancelPackedMove(error));
        assert(staged(*editor) == beforePickup);
        assertOriginal(*editor, raw);

        G2::BoxPokemonCreate create;
        create.species = 25;
        create.level = 5;
        create.nickname = "NEW";
        create.otName = "ASH";
        std::size_t newSlot = 99;
        assert(editor->stageAddBoxPokemon(2, create, newSlot, error) && newSlot == 0);
    }
}

std::string readText(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), {}};
}

// R2-CHECK-009: Gen I Box Actions lists Move (between Clone and Release, the shared E02 order)
// where the Boxes-view carry exists, and Move starts that carry with the Pokemon picked up:
// A places it (Round 11 H23; Y before) as ONE verified staged move, B leaves the source untouched.
void runGen1BoxActionsMove(G1::SourceGame game) {
    namespace Gen1 = PokeBank::UIModel::Gen1Editor;
    namespace Menu = PokeBank::UIModel::NarrowPokemonMenu;
    const auto labels = [](const Gen1::ActionSet& set) {
        std::vector<std::string> out;
        const auto model = Menu::gen1(set);
        for (int i = 0; i < model.count; ++i) out.emplace_back(model.rows[static_cast<std::size_t>(i)].label);
        return out;
    };
    const std::string legality = Menu::kOriginLegalityLabel;
    assert((labels(Gen1::actionsForSlot(true, false, true)) ==
            std::vector<std::string>{"View", "Edit", "Clone", "Move", "Release", legality, "Cancel"}));
    assert((labels(Gen1::actionsForSlot(true, true, true)) ==
            std::vector<std::string>{"View", "Edit", "Clone", "Move", "Release", legality,
                                     "Review Pending Changes", "Cancel"}));
    // Read-only Storage pane (boxMove false) and empty slots: no Move.
    assert((labels(Gen1::actionsForSlot(true, false, false)) ==
            std::vector<std::string>{"View", "Edit", "Clone", "Release", legality, "Cancel"}));
    assert((labels(Gen1::actionsForSlot(false, false, true)) == std::vector<std::string>{"Add Pokemon", "Cancel"}));

    const auto raw = Gen1Fixture::make(game);
    std::string error;
    {   // Actions -> Move (pickup), then Y: one staged move of the same record.
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto before = editor->boxedPokemon(2, 1, error);
        const auto stagedBefore = staged(*editor);
        assert(before && editor->beginPackedMove(2, 1, error));
        std::size_t placed = 99;
        assert(editor->placePackedMove(2, 3, placed, error) && placed == 3);
        const auto moved = editor->boxedPokemon(2, 3, error);
        assert(moved && moved->rawBody == before->rawBody && moved->nickname == before->nickname);
        assert(staged(*editor) != stagedBefore);  // the move is staged in the working copy
        assert((gen1Order(*editor, 2) == std::vector<std::string>{"PIKA", "C", "D", "B", "E"}));
        assertGen1Packed(*editor);
        assertOriginal(*editor, raw);
    }
    {   // Actions -> Move (pickup), then B: the staged box is exactly as before the pickup.
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto stagedBefore = staged(*editor);
        const auto orderBefore = gen1Order(*editor, 2);
        assert(editor->beginPackedMove(2, 1, error));
        assert(editor->cancelPackedMove(error));
        assert(staged(*editor) == stagedBefore && gen1Order(*editor, 2) == orderBefore);
        assertOriginal(*editor, raw);
    }

    // Production routing: both live Gen I action dispatchers send Move into the SAME Boxes carry
    // (ClassicPackedMove::beginGen1FromAction -> beginGen1), and every sheet / hit-test / dispatch
    // builds its rows with the same boxMove flag so rows, touch ids and actions line up.
    const auto overlay = readText("src/UI/ClassicPackedMoveOverlay.inc");
    assert(overlay.find("bool beginGen1FromAction(TrainerViewScreen& screen, int box, int slot) {") != std::string::npos);
    assert(overlay.find("screen.selectedMode != TrainerViewScreen::ViewMode::Boxes") != std::string::npos);
    assert(overlay.find("beginGen1(screen, state, box, slot);") != std::string::npos);
    for (const char* path : {"src/UI/ClassicReleaseActionFix.inc", "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc"}) {
        const auto source = readText(path);
        assert(source.find("ClassicPackedMove::beginGen1FromAction(screen, box, slot)") != std::string::npos);
    }
    const auto release = readText("src/UI/ClassicReleaseActionFix.inc");
    std::size_t flagged = 0;
    for (auto at = release.find("boxMoveAvailable(screen)"); at != std::string::npos;
         at = release.find("boxMoveAvailable(screen)", at + 1)) ++flagged;
    assert(flagged == 2); // input and draw
    const auto ux = readText("src/UI/Gen1PokemonEditorOverlayUX.inc");
    assert(ux.find("screen.selectedMode == TrainerViewScreen::ViewMode::Boxes;") != std::string::npos);
}

// Round 11 H32 (skeptic, scratchpad probe): the Gen I carry transaction owns the staged image from
// pickup to place / cancel. Before the fix stageEdit / stageAdd / stageClone / stageRemove were all
// accepted mid-carry and cancelPackedMove then silently threw those changes away; discard() left the
// carry active, so a later cancel brought discarded changes back.
void runGen1CarryGuards(G1::SourceGame game) {
    const auto raw = Gen1Fixture::make(game);
    std::string error;
    {   // Every public box mutation is refused while carrying; nothing changes; cancel restores.
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto beforePickup = staged(*editor);
        const auto changesBefore = editor->pendingChanges().size();
        assert(editor->beginPackedMove(2, 1, error));
        const auto carrying = staged(*editor);
        G1::BoxPokemonEdit edit;
        edit.nickname = "LOST";
        G1::BoxPokemonCreate create;
        create.species = 7;
        create.otName = "RED";
        assert(!editor->stageEdit(2, 0, edit, error) && !error.empty());
        assert(!editor->stageAdd(3, 0, create, error));
        assert(!editor->stageClone(2, 0, 3, 0, error));
        assert(!editor->stageRemove(2, 0, error));
        assert(!editor->revertPokemon(2, 0, error));
        assert(staged(*editor) == carrying && editor->packedMoveActive());
        assert(editor->cancelPackedMove(error) && staged(*editor) == beforePickup);
        assert(editor->pendingChanges().size() == changesBefore && gen1Order(*editor, 3).empty());
        // Idle again: the same calls work (pickup and Release paths are unchanged).
        assert(editor->stageClone(2, 0, 3, 0, error) && gen1Order(*editor, 3).size() == 1);
        assert(editor->stageRemove(3, 0, error) && gen1Order(*editor, 3).empty());
        assertOriginal(*editor, raw);
    }
    {   // Clone onto an occupied slot (inside the list) or a non-contiguous slot: refused, bytes unchanged.
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        const auto before = staged(*editor);
        assert(!editor->stageClone(2, 0, 2, 1, error));   // occupied (count 5)
        assert(!editor->stageClone(2, 0, 2, 7, error));   // past the next free slot
        assert(staged(*editor) == before);
        assert(editor->stageClone(2, 0, 2, 5, error) && gen1Order(*editor, 2).size() == 6);
    }
    {   // Discard during a carry ends it: a later cancel or place cannot restore discarded changes.
        auto editor = makeGen1Editor(raw, game);
        setupGen1Five(*editor);
        assert(editor->beginPackedMove(2, 1, error));
        editor->discard();
        assert(!editor->packedMoveActive());
        assert(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), raw.begin(), raw.end()));
        assert(editor->cancelPackedMove(error));
        std::size_t placed = 0;
        assert(!editor->placePackedMove(2, 0, placed, error));
        assert(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), raw.begin(), raw.end()));
        assert(editor->pendingChanges().empty());
        // The editor is usable again: a new pickup works.
        assert(editor->beginPackedMove(2, 0, error) && editor->cancelPackedMove(error));
        assertOriginal(*editor, raw);
    }
}

// Round 11 H32: the same transaction rule for Gen II. Every public staging entry point refuses while
// a Pokemon is carried (the skeptic found the trainer name / money cache left stale by a cancel, which
// then blocked finalize), cancel restores the cached trainer name and money with the bytes, and
// discard() ends the carry.
template <class L>
void runGen2CarryGuards(const L& layoutFixture, G2::SourceGame game) {
    const auto raw = fixture(layoutFixture);
    std::string error;
    {
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        const auto beforePickup = staged(*editor);
        const auto changesBefore = editor->pendingChanges().size();
        const std::string name = editor->trainerName();
        const uint32_t money = editor->money();
        assert(editor->beginPackedMove(0, 1, error));
        const auto carrying = staged(*editor);
        G2::BoxPokemonEdit edit;
        edit.nickname = "LOST";
        G2::BoxPokemonCreate create;
        create.species = 152;
        create.level = 5;
        create.otName = "ASH";
        std::size_t slot = 99;
        assert(!editor->stageBoxPokemonEdit(0, 0, edit, error) && !error.empty());
        assert(!editor->stageBoxPokemonShiny(0, 0, true, error));
        assert(!editor->stageCloneBoxPokemon(0, 0, 2, slot, error));
        assert(!editor->stageAddBoxPokemon(2, create, slot, error));
        assert(!editor->stageTrainerName("LOST", error));
        assert(!editor->stageMoney(1234, error));
        assert(!editor->stageTrainerEdit("LOST", 1234, error));
        assert(!editor->stagePlayTime({1, 2, 3}, error));
        assert(!editor->stageTrainerGender(1, error));
        assert(!editor->stageItemQuantity(G2::InventoryPocket::Items, 18, 3, error));
        assert(!editor->stageInventoryQuantity(G2::InventoryPocket::Items, 18, 3, error));
        assert(!editor->stageInventorySlotQuantity(G2::InventoryPocket::Items, 0, 18, 3, error));
        assert(!editor->stageReleaseBoxPokemon(0, 0, error));
        assert(staged(*editor) == carrying && editor->packedMoveActive());
        assert(editor->trainerName() == name && editor->money() == money);
        assert(editor->cancelPackedMove(error) && staged(*editor) == beforePickup);
        assert(editor->pendingChanges().size() == changesBefore);
        assert(editor->trainerName() == name && editor->money() == money);
        assert(!editor->finalizedBytes(error).empty() && error.empty());
        // Idle again: trainer edits and Release work.
        assert(editor->stageMoney(1234, error) && editor->money() == 1234);
        assert(editor->stageReleaseBoxPokemon(0, 4, error));
        assertOriginal(*editor, raw);
    }
    {   // Discard during a carry ends it; nothing discarded can come back.
        auto editor = makeGen2Editor(raw, game);
        setupGen2Five(*editor);
        assert(editor->stageMoney(4321, error));
        assert(editor->beginPackedMove(0, 1, error));
        editor->discard();
        assert(!editor->packedMoveActive());
        assert(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), raw.begin(), raw.end()));
        assert(editor->cancelPackedMove(error));
        std::size_t placed = 0;
        assert(!editor->placePackedMove(0, 0, placed, error));
        assert(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), raw.begin(), raw.end()));
        assert(editor->pendingChanges().empty() && editor->money() != 4321);
        assertOriginal(*editor, raw);
    }
}

int main() {
    for (const auto game : {G1::SourceGame::Red, G1::SourceGame::Blue, G1::SourceGame::Yellow}) {
        runGen1(game);
        runGen1BoxActionsMove(game);
        runGen1CarryGuards(game);
    }

    runGen2(GS, G2::SourceGame::Gold);
    runGen2(C, G2::SourceGame::Crystal);
    runGen2CarryGuards(GS, G2::SourceGame::Gold);
    runGen2CarryGuards(C, G2::SourceGame::Crystal);

    std::cout << "Gen I/II packed single Y move tests passed\n";
    return 0;
}
