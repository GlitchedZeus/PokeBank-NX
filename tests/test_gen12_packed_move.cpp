#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "tests/fixtures/gsc_pokemon_fixture.h"

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

int main() {
    for (const auto game : {G1::SourceGame::Red, G1::SourceGame::Blue, G1::SourceGame::Yellow})
        runGen1(game);

    runGen2(GS, G2::SourceGame::Gold);
    runGen2(C, G2::SourceGame::Crystal);

    std::cout << "Gen I/II packed single Y move tests passed\n";
    return 0;
}
