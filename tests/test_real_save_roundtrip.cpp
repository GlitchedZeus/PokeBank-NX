// Opt-in real-save round trip (owner Oct 28, Feature D; adapted from PKSE 56a0667's host harness,
// without its on-console SD-writing self test).
//
// POKEBANK_REAL_SAVES=<folder> runs every *.sav in that folder (and its saves/ subfolder) through the
// generation's own staged editor with NO edits and requires the finalized bytes to equal the file
// exactly. Saves are never committed; without the variable the test reports NOT TESTED / NO FIXTURE
// and passes. Per file:
//   PASS     the unedited round trip is byte-identical;
//   REFUSED  the editor declined the save (the reason is printed, e.g. Japanese Gen I/II stay
//            read-only); that is evidence, not a failure;
//   FAIL     the parser rejected a save the name says it supports, or the bytes changed.
// The game is taken from the file name (red, blue, yellow, gold, silver, crystal, ruby, sapphire,
// emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white,
// black2, white2); a file whose name names no game is skipped.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace G1 = PokeVault::Integration::Gen1;
namespace G2 = PokeVault::Integration::Gen2;
namespace G3 = PokeVault::Integration::Gen3;
namespace G4 = PokeVault::Integration::Gen4;
namespace G5 = PokeVault::Integration::Gen5;

namespace {

enum class Outcome { Pass, Refused, Fail, Skipped };
struct Result { Outcome outcome; std::string detail; };

std::vector<uint8_t> readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
bool has(const std::string& name, const char* word) { return name.find(word) != std::string::npos; }

Result compare(const std::vector<uint8_t>& out, const std::vector<uint8_t>& in, const std::string& error) {
    if (out.empty()) return {Outcome::Refused, "finalize declined: " + error};
    if (out.size() != in.size()) return {Outcome::Fail, "size changed"};
    for (std::size_t i = 0; i < in.size(); ++i)
        if (out[i] != in[i]) return {Outcome::Fail, "byte offset " + std::to_string(i) + " changed"};
    return {Outcome::Pass, "byte-identical"};
}

Result gen1(const std::vector<uint8_t>& raw, G1::SourceGame game) {
    const auto parsed = G1::parse(raw, game);
    if (!parsed) return {Outcome::Fail, std::string("parser refused: ") + G1::errorName(parsed.error)};
    std::string error;
    auto editor = G1::StagedPokemonEditor::create(*parsed.save, error);
    if (!editor) return {Outcome::Refused, error + " (" + G1::regionName(parsed.save->metadata().region) + ")"};
    return compare(editor->finalizedBytes(error), raw, error);
}
Result gen2(const std::vector<uint8_t>& raw, G2::SourceGame game) {
    const auto parsed = G2::parse(raw, game);
    if (!parsed) return {Outcome::Fail, "parser refused: " + parsed.detail};
    std::string error;
    auto editor = G2::StagedEditor::create(*parsed.save, error);
    if (!editor) return {Outcome::Refused, error + " (" + G2::regionName(parsed.save->metadata().region) + ")"};
    return compare(editor->finalizedBytes(error), raw, error);
}
Result gen3(const std::vector<uint8_t>& raw, G3::SourceGame game) {
    std::string error;
    auto editor = G3::StagedPokemonEditor::create(raw, game, error);
    if (!editor) return {Outcome::Fail, "editor refused: " + error};
    return compare(editor->finalizedBytes(error), raw, error);
}
Result gen4(const std::vector<uint8_t>& raw, G4::Layout layout, const char* id) {
    std::string error;
    auto editor = G4::Gen4StagedPokemonEditor::create(raw, layout, id, &error);
    if (!editor) return {Outcome::Fail, "editor refused: " + error};
    return compare(editor->finalizedBytes(&error), raw, error);
}
Result gen5(const std::vector<uint8_t>& raw, const char* id) {
    std::string error;
    // Gen V builds an export candidate only from pending changes; an unedited workspace has none,
    // so the evidence here is the strict parse of the real file. When both copies are valid but
    // differ, the app asks the user to choose (by design); the runner then checks each copy.
    if (G5::Gen5ReadOnlySave::parse(raw, id, &error)) return {Outcome::Pass, "strict parse (no unedited export path in Gen V)"};
    const std::string automatic = error;
    std::string primaryError, backupError;
    const bool primary = G5::Gen5ReadOnlySave::parse(raw, id, &primaryError, G5::SaveCopySelection::Primary).has_value();
    const bool backup = G5::Gen5ReadOnlySave::parse(raw, id, &backupError, G5::SaveCopySelection::Backup).has_value();
    if (primary || backup)
        return {Outcome::Pass, std::string("strict parse of the chosen copy (primary ") + (primary ? "ok" : "refused") +
                                   ", backup " + (backup ? "ok" : "refused") + "; automatic: " + automatic + ")"};
    return {Outcome::Fail, "parser refused: " + automatic + " / primary: " + primaryError + " / backup: " + backupError};
}

Result run(const fs::path& file) {
    const std::string n = lower(file.filename().string());
    const auto raw = readFile(file);
    // Order matters: "black2" before "black", "heartgold" before "gold", "leafgreen" before "green".
    if (has(n, "black2") || has(n, "black 2")) return gen5(raw, "black2_nds");
    if (has(n, "white2") || has(n, "white 2")) return gen5(raw, "white2_nds");
    if (has(n, "black")) return gen5(raw, "black_nds");
    if (has(n, "white")) return gen5(raw, "white_nds");
    if (has(n, "heartgold")) return gen4(raw, G4::Layout::HeartGoldSoulSilver, "heartgold_nds");
    if (has(n, "soulsilver")) return gen4(raw, G4::Layout::HeartGoldSoulSilver, "soulsilver_nds");
    if (has(n, "platinum")) return gen4(raw, G4::Layout::Platinum, "platinum_nds");
    if (has(n, "diamond")) return gen4(raw, G4::Layout::DiamondPearl, "diamond_nds");
    if (has(n, "pearl")) return gen4(raw, G4::Layout::DiamondPearl, "pearl_nds");
    if (has(n, "firered")) return gen3(raw, G3::SourceGame::FireRedGBA);
    if (has(n, "leafgreen")) return gen3(raw, G3::SourceGame::LeafGreenGBA);
    if (has(n, "emerald")) return gen3(raw, G3::SourceGame::EmeraldGBA);
    if (has(n, "sapphire")) return gen3(raw, G3::SourceGame::SapphireGBA);
    if (has(n, "ruby")) return gen3(raw, G3::SourceGame::RubyGBA);
    if (has(n, "crystal")) return gen2(raw, G2::SourceGame::Crystal);
    if (has(n, "silver")) return gen2(raw, G2::SourceGame::Silver);
    if (has(n, "gold")) return gen2(raw, G2::SourceGame::Gold);
    if (has(n, "yellow")) return gen1(raw, G1::SourceGame::Yellow);
    if (has(n, "blue")) return gen1(raw, G1::SourceGame::Blue);
    if (has(n, "red")) return gen1(raw, G1::SourceGame::Red);
    return {Outcome::Skipped, "no game named in the file name"};
}

}  // namespace

int main() {
    const char* dir = std::getenv("POKEBANK_REAL_SAVES");
    if (!dir) {
        std::cout << "Real-save round trip: NOT TESTED / NO FIXTURE (set POKEBANK_REAL_SAVES)\n";
        return 0;
    }
    std::vector<fs::path> files;
    for (const fs::path& base : {fs::path(dir), fs::path(dir) / "saves"}) {
        std::error_code ec;
        if (!fs::is_directory(base, ec)) continue;
        for (const auto& e : fs::directory_iterator(base, ec))
            if (e.is_regular_file() && lower(e.path().extension().string()) == ".sav") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());
    int pass = 0, refused = 0, fail = 0, skipped = 0;
    for (const auto& f : files) {
        const auto r = run(f);
        const char* tag = r.outcome == Outcome::Pass ? "PASS    " : r.outcome == Outcome::Refused ? "REFUSED "
                        : r.outcome == Outcome::Fail ? "FAIL    " : "SKIPPED ";
        std::cout << tag << f.filename().string() << ": " << r.detail << '\n';
        (r.outcome == Outcome::Pass ? pass : r.outcome == Outcome::Refused ? refused
                                           : r.outcome == Outcome::Fail ? fail : skipped)++;
    }
    std::cout << "Real-save round trip: " << pass << " pass, " << refused << " refused, " << fail << " fail, "
              << skipped << " skipped (" << files.size() << " files). Synthetic tests never stand in for these.\n";
    return fail ? 1 : 0;
}
