// R2-TRAINER: Gen I staged Trainer Name / Money (Gen1StagedTrainerEditor). Synthetic R/B/Y saves
// only. Layout (PKHeX SAV1Offsets / PKSM-Core Sav1, mirrored by Gen1ReadOnlySave.cpp): player name
// 0x2598 (International 11 bytes, Japanese 6), money International 0x25F3 / Japanese 0x25EE as
// 3-byte packed BCD (pret/pokered wPlayerMoney, max 999999), main checksum International 0x3523
// over 0x2598..0x3522 / Japanese 0x3594 over 0x2598..0x3593.
#include "Integration/Gen1/Gen1ExportTransaction.h"
#include "Integration/Gen1/Gen1StagedTrainerEditor.h"
#include "UI/ClassicTrainerSession.h"

#include <cassert>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>
#include <unistd.h>

using namespace PokeVault::Integration::Gen1;
namespace fs = std::filesystem;

namespace {
uint8_t diff8(std::span<const uint8_t> bytes) {
    uint8_t value = 0xFF;
    for (uint8_t b : bytes) value = static_cast<uint8_t>(value - b);
    return value;
}

// International R/B/Y: player "RED", money 0, one boxed Pikachu (enough for the staged Pokemon
// workspace that the Gen I export goes through).
std::vector<uint8_t> international(SourceGame game) {
    std::vector<uint8_t> v(kRawSaveSize, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50; // RED
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 2;                                   // boxes not initialized
    v[0x29C3] = game == SourceGame::Yellow ? 0x54 : 0x99;
    const std::size_t box = 0x30C0, body = box + 22;
    v[box] = 1; v[box + 1] = 0x54; v[box + 2] = 0xFF;
    v[body] = 0x54; v[body + 2] = 20; v[body + 3] = 5;
    v[body + 5] = v[body + 6] = 23; v[body + 7] = 190; v[body + 8] = 33;
    v[body + 12] = 0x12; v[body + 13] = 0x34; v[body + 16] = 125;
    v[body + 27] = 0x88; v[body + 28] = 0x88; v[body + 29] = 35;
    std::fill_n(v.begin() + box + 682, 11, 0x50); v[box + 682] = 0x91; v[box + 683] = 0x84; v[box + 684] = 0x83;
    std::fill_n(v.begin() + box + 902, 11, 0x50);
    v[box + 902] = 0x8F; v[box + 903] = 0x88; v[box + 904] = 0x8A; v[box + 905] = 0x80;
    v[0x3523] = diff8(std::span<const uint8_t>(v).subspan(0x2598, 0x0F8B));
    return v;
}

// Japanese R/G/B/Y layout with empty lists (as in test_rby_staged_inventory).
std::vector<uint8_t> japanese(SourceGame game) {
    std::vector<uint8_t> v(kRawSaveSize, 0);
    v[0x2598] = 0x50;
    v[0x2ED5 + 1] = 0xFF; v[0x302D + 1] = 0xFF;
    v[0x2842] = 2;
    v[0x29B9] = game == SourceGame::Yellow ? 0x54 : 0x99;
    v[0x2712] = game == SourceGame::Yellow ? 90 : 0;
    v[0x25C4 + 1] = 0xFF; v[0x27DC + 1] = 0xFF;
    v[0x25EE] = 0x00; v[0x25EF] = 0x31; v[0x25F0] = 0x41; // 3141
    v[0x3594] = diff8(std::span<const uint8_t>(v).subspan(0x2598, 0x0FFC));
    return v;
}

std::vector<uint8_t> readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string readText(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void bcd() {
    std::array<uint8_t, 3> out{};
    assert(encodeMoneyBCD3(0, out) && out == (std::array<uint8_t, 3>{0x00, 0x00, 0x00}));
    assert(encodeMoneyBCD3(123456, out) && out == (std::array<uint8_t, 3>{0x12, 0x34, 0x56}));
    assert(encodeMoneyBCD3(999999, out) && out == (std::array<uint8_t, 3>{0x99, 0x99, 0x99}));
    assert(!encodeMoneyBCD3(1000000, out));
    for (uint32_t m : {0u, 7u, 3141u, 500000u, 999999u}) {
        uint32_t back = 1;
        assert(encodeMoneyBCD3(m, out) && decodeMoneyBCD3(std::span<const uint8_t, 3>(out), back) && back == m);
    }
    const std::array<uint8_t, 3> invalid{0x1A, 0x00, 0x00};
    uint32_t ignored = 0;
    assert(!decodeMoneyBCD3(std::span<const uint8_t, 3>(invalid), ignored));
}

void internationalGame(SourceGame game) {
    const auto raw = international(game);
    const auto source = raw;
    auto parsed = parse(raw, game);
    assert(parsed);
    std::string error;
    auto trainer = StagedTrainerEditor::create(*parsed.save, error);
    assert(trainer && error.empty());
    assert(trainer->trainerName() == "RED" && trainer->money() == 0 && trainer->nameEditable());
    assert(!trainer->hasPendingChanges());

    // Name: native glyphs, at most 7 characters, written into the 11-byte field.
    assert(!trainer->stageTrainerName("TOOLONGX", error));
    assert(!trainer->stageTrainerName("A&B", error));
    assert(!trainer->stageTrainerName("", error));
    assert(trainer->stageTrainerName("ASH", error));
    const auto staged = trainer->stagedBytes();
    assert(staged[0x2598] == 0x80 && staged[0x2599] == 0x92 && staged[0x259A] == 0x87 && staged[0x259B] == 0x50);

    // Money: packed BCD at 0x25F3, bounds 0..999999.
    assert(!trainer->stageMoney(1000000, error) && trainer->money() == 0);
    assert(trainer->stageMoney(123456, error));
    assert(trainer->stagedBytes()[0x25F3] == 0x12 && trainer->stagedBytes()[0x25F4] == 0x34 &&
           trainer->stagedBytes()[0x25F5] == 0x56);
    assert(trainer->stageMoney(999999, error));

    // Save-specific checksum is recomputed; strict reparse sees the staged values.
    const auto out = trainer->finalize(error);
    assert(!out.empty() && error.empty());
    assert(out[0x3523] == diff8(std::span<const uint8_t>(out).subspan(0x2598, 0x0F8B)));
    auto reopened = parse(out, game);
    assert(reopened && reopened.save->trainer().name == "ASH" && reopened.save->trainer().money == 999999);
    // Only the name, money and checksum bytes changed.
    for (std::size_t i = 0; i < out.size(); ++i) {
        const bool allowed = (i >= 0x2598 && i < 0x2598 + 11) || (i >= 0x25F3 && i < 0x25F6) || i == 0x3523;
        if (!allowed) assert(out[i] == source[i]);
    }

    // Review lines (shared export change list) and Discard.
    const auto changes = trainer->pendingChanges();
    assert(changes.size() == 2 && changes[0].label == "Trainer Name" && changes[0].afterValue == "ASH" &&
           changes[1].label == "Money" && changes[1].afterValue == "$999999");
    auto pokemon = StagedPokemonEditor::create(*parsed.save, error);
    assert(pokemon && error.empty() && !pokemon->hasPendingChanges());
    const auto lines = gen1ExportChangeLines(*pokemon, nullptr, trainer.get());
    assert(lines.size() == 2 && lines[0] == "Trainer Name: RED -> ASH" && lines[1] == "Money: $0 -> $999999");

    // Export: a Trainer-only change exports through the verified Gen I transaction and reopens.
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-gen1-trainer-" + std::to_string(static_cast<int>(game)) + "-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    const fs::path sourcePath = root / "source.srm";
    { std::ofstream f(sourcePath, std::ios::binary); f.write(reinterpret_cast<const char*>(source.data()), static_cast<std::streamsize>(source.size())); }
    Gen1ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = "trainer_export";
    request.sourcePath = sourcePath.string();
    request.timestamp = "20261017T000000Z";
    const auto result = publishVerifiedGen1Export(*pokemon, request, nullptr, trainer.get());
    assert(result.success && result.error.empty());
    std::vector<uint8_t> edited;
    for (const auto& entry : fs::directory_iterator(result.directory))
        if (entry.path().extension() == ".srm" && entry.path().filename().string().find("original") == std::string::npos)
            edited = readFile(entry.path());
    auto exported = parse(edited, game);
    assert(exported && exported.save->trainer().name == "ASH" && exported.save->trainer().money == 999999);
    assert(readFile(sourcePath) == source); // the source save is never written
    fs::remove_all(root);

    trainer->discard();
    assert(!trainer->hasPendingChanges() && trainer->trainerName() == "RED" && trainer->money() == 0);
    assert(std::equal(trainer->stagedBytes().begin(), trainer->stagedBytes().end(), source.begin()));
    assert(raw == source);
}

void japaneseLayout(SourceGame game) {
    const auto raw = japanese(game);
    auto parsed = parse(raw, game);
    assert(parsed && parsed.save->metadata().region == RegionLayout::Japanese);
    std::string error;
    auto trainer = StagedTrainerEditor::create(*parsed.save, error);
    assert(trainer && trainer->money() == 3141 && trainer->nameMax() == 5);
    // No kana encoder: Japanese names stay read-only, honestly refused.
    assert(!trainer->nameEditable() && !trainer->stageTrainerName("ASH", error) && !error.empty());
    assert(trainer->stageMoney(271828, error));
    assert(trainer->stagedBytes()[0x25EE] == 0x27 && trainer->stagedBytes()[0x25EF] == 0x18 &&
           trainer->stagedBytes()[0x25F0] == 0x28);
    const auto out = trainer->finalize(error);
    assert(!out.empty() && out[0x3594] == diff8(std::span<const uint8_t>(out).subspan(0x2598, 0x0FFC)));
    auto reopened = parse(out, game);
    assert(reopened && reopened.save->trainer().money == 271828);
}

void trainerPage() {
    namespace T = PokeBank::UIModel::ClassicTrainerSession;
    // The Trainer footer offers A: Edit only for editable Name / Money (every generation).
    static_assert(std::string_view(T::trainerPageInstructions(true)).find("A: Edit") != std::string_view::npos);
    static_assert(std::string_view(T::trainerPageInstructions(false)).find("A: Edit") == std::string_view::npos);
    const auto base = readText("src/UI/TrainerViewScreenBase.inc");
    assert(base.find("ClassicTrainerSession::trainerPageInstructions(") != std::string::npos);
    assert(base.find("instructions = \"Up/Down: Select  |  A: Edit  |  B: Back  |  +: Exit App\";") == std::string::npos);
    // Gen I and Gen II stage through the one shared Trainer session flow.
    const auto gen1 = readText("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    assert(gen1.find("if (handleGen1TrainerSession(screen, down, held, stickX, stickY, touch)) return true;") != std::string::npos);
    assert(gen1.find("UI::ClassicTrainerSession::handle(") != std::string::npos);
    const auto gen2 = readText("src/UI/Gen2TrainerSessionOverlay.inc");
    assert(gen2.find("UI::ClassicTrainerSession::handle(") != std::string::npos);
    assert(gen2.find("editor->stageTrainerEdit(name, money, error)") != std::string::npos); // Gen II regression
    // Discard restores the shown Trainer values; Review and Export include the Trainer workspace.
    const auto ux = readText("src/UI/Gen1PokemonEditorOverlayUX.inc");
    assert(ux.find("trainer->stagedInventory(), trainer->stagedTrainer())") != std::string::npos);
    assert(ux.find("screen.trainer.trainerName = e->trainerName();") != std::string::npos);
}
} // namespace

int main() {
    bcd();
    for (auto game : {SourceGame::Red, SourceGame::Blue, SourceGame::Yellow}) {
        internationalGame(game);
        japaneseLayout(game);
    }
    trainerPage();
    std::cout << "Gen I staged Trainer Name / Money (R/B/Y, International + Japanese) and Trainer page: PASS\n";
    return 0;
}
