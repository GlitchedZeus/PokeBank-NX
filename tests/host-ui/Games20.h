#pragma once
// Round 17 autonomous QA: the 20 Gen I-V releases and their QA-matrix saves
// (build-host/games20/<game id>.sav, written by test_qa_matrix --dump-fixtures), opened through the
// production path (ImmutableSnapshot::read -> openBackupTrainer -> TrainerViewScreen) for the
// host UI suites tests/test_ui_games20_*.cpp. Synthetic saves only.
#include "Games/GameIdentity.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Safety/SourceMutationPolicy.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/TrainerViewScreen.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace Games20 {
namespace fsx = std::filesystem;
using SK = PokeVault::Safety::SourceKind;

struct GameSpec { const char* id; const char* title; int generation; const char* emulator; SK kind; };
inline const GameSpec kGames[20] = {
    {"red_gb", "Pokemon Red", 1, "RetroArch", SK::RetroArchLegacy},
    {"blue_gb", "Pokemon Blue", 1, "RetroArch", SK::RetroArchLegacy},
    {"yellow_gb", "Pokemon Yellow", 1, "RetroArch", SK::RetroArchLegacy},
    {"gold_gbc", "Pokemon Gold", 2, "RetroArch", SK::RetroArchLegacy},
    {"silver_gbc", "Pokemon Silver", 2, "RetroArch", SK::RetroArchLegacy},
    {"crystal_gbc", "Pokemon Crystal", 2, "RetroArch", SK::RetroArchLegacy},
    {"ruby_gba", "Pokemon Ruby", 3, "mGBA", SK::RetroArchLegacy},
    {"sapphire_gba", "Pokemon Sapphire", 3, "mGBA", SK::RetroArchLegacy},
    {"emerald_gba", "Pokemon Emerald", 3, "mGBA", SK::RetroArchLegacy},
    {"firered_gba", "Pokemon FireRed", 3, "mGBA", SK::RetroArchLegacy},
    {"leafgreen_gba", "Pokemon LeafGreen", 3, "mGBA", SK::RetroArchLegacy},
    {"diamond_nds", "Pokemon Diamond", 4, "DraStic", SK::ExternalLegacy},
    {"pearl_nds", "Pokemon Pearl", 4, "DraStic", SK::ExternalLegacy},
    {"platinum_nds", "Pokemon Platinum", 4, "DraStic", SK::ExternalLegacy},
    {"heartgold_nds", "Pokemon HeartGold", 4, "DraStic", SK::ExternalLegacy},
    {"soulsilver_nds", "Pokemon SoulSilver", 4, "DraStic", SK::ExternalLegacy},
    {"black_nds", "Pokemon Black", 5, "DraStic", SK::ExternalLegacy},
    {"white_nds", "Pokemon White", 5, "DraStic", SK::ExternalLegacy},
    {"black2_nds", "Pokemon Black 2", 5, "DraStic", SK::ExternalLegacy},
    {"white2_nds", "Pokemon White 2", 5, "DraStic", SK::ExternalLegacy},
};

inline std::vector<uint8_t> readAll(const fsx::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
inline uint64_t fnv(const std::vector<uint8_t>& b) {
    uint64_t h = 1469598103934665603ull;
    for (uint8_t c : b) { h ^= c; h *= 1099511628211ull; }
    return h;
}
inline bool validUtf8(const std::string& s) {
    size_t i = 0;
    while (i < s.size()) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
        if (n == 0 || i + n > s.size()) return false;
        for (size_t k = 1; k < n; ++k)
            if ((static_cast<unsigned char>(s[i + k]) >> 6) != 2) return false;
        i += n;
    }
    return true;
}


// One opened save: the trainer and a screen on it. The source file must outlive it (Gen IV re-reads
// the container by path).
struct Opened {
    std::unique_ptr<Trainer::Trainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
    std::string error;
    explicit operator bool() const noexcept { return static_cast<bool>(screen); }
};
inline Opened open(const GameSpec& game, const fsx::path& source) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    Opened out;
    S::File file;
    if (!S::read(source.string(), source.parent_path().string(), {}, file, out.error)) return out;
    out.trainer = PokeVault::Legacy::openBackupTrainer(file, game.id, out.error);
    if (!out.trainer) return out;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, game.title, source.string(), 0, AccountUid{},
                                                         game.kind, game.id, game.emulator);
    return out;
}
// Copies the fixture of `game` into <root>/saves/<id>/<id>.sav and returns that path ("" if missing).
inline fsx::path install(const GameSpec& game, const fsx::path& fixtures, const fsx::path& root,
                         std::vector<uint8_t>* bytesOut = nullptr) {
    const auto bytes = readAll(fixtures / (std::string(game.id) + ".sav"));
    if (bytes.empty()) return {};
    const fsx::path dir = root / "saves" / game.id;
    fsx::create_directories(dir);
    const fsx::path source = dir / (std::string(game.id) + ".sav");
    std::ofstream out(source, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (bytesOut) *bytesOut = bytes;
    return source;
}
} // namespace Games20
