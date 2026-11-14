// Round 17 autonomous QA: random-input ("monkey") walks through the REAL TrainerViewScreen
// update / draw for all 20 Gen I-V releases (Red, Blue, Yellow, Gold, Silver, Crystal, Ruby,
// Sapphire, Emerald, FireRed, LeafGreen, Diamond, Pearl, Platinum, HeartGold, SoulSilver, Black,
// White, Black 2, White 2), on the exact per-version saves the QA matrix builds
// (test_qa_matrix --dump-fixtures). Each walk is a seeded sequence of controller buttons, repeated
// and rapid presses, touch taps on drawn text and on random points, drags, and random prompt
// answers (numbers clamped as the device prompt clamps them; names in ASCII, kana, Hangul, symbols,
// empty and over-long). It is built with ASan/UBSan in the sanitize variant.
//
// What every walk proves (per game, per seed):
//   I1  no crash, hang, sanitizer report or uncaught exception in update() or draw();
//   I2  the source save on disk and the in-memory source snapshot stay byte-identical;
//   I3  from wherever the walk ended, B (answering the unsaved-changes question with Discard)
//       always leaves the save: no screen traps the player;
//   I4  every save file the walk exported strictly reparses as the SAME game, and the source is
//       never among the files it wrote;
//   I5  every string drawn is valid UTF-8 (no torn name ever reaches the renderer).
// A failure prints the game, seed and the tail of the action trace; MONKEY_GAME / MONKEY_SEED /
// MONKEY_STEPS / MONKEY_TRACE=1 replay it exactly.
//
// Evidence class: SYNTHETIC saves, host harness. It does not show Switch rendering, controller
// firmware, touch calibration or docked mode.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Games20.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

#ifndef GAMES20_FIXTURE_DIR
#define GAMES20_FIXTURE_DIR "build-host/games20"
#endif

namespace fsx = std::filesystem;
using SK = PokeVault::Safety::SourceKind;

namespace {

int failures = 0;
long checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}

using Games20::GameSpec;
using Games20::kGames;
using Games20::readAll;
using Games20::fnv;
using Games20::validUtf8;

const u64 kButtons[] = {HidNpadButton_A, HidNpadButton_B, HidNpadButton_X, HidNpadButton_Y, HidNpadButton_L,
                        HidNpadButton_R, HidNpadButton_ZL, HidNpadButton_ZR, HidNpadButton_Plus, HidNpadButton_Minus,
                        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right,
                        HidNpadButton_StickLUp, HidNpadButton_StickLDown, HidNpadButton_StickLLeft,
                        HidNpadButton_StickLRight};
const char* kButtonNames[] = {"A", "B", "X", "Y", "L", "R", "ZL", "ZR", "+", "-", "Up", "Down", "Left", "Right",
                              "StickUp", "StickDown", "StickLeft", "StickRight"};
// Weighted: A and directions most (they reach deeper screens), B often (it must always be safe).
const int kWeights[] = {22, 12, 6, 6, 4, 4, 2, 3, 4, 3, 9, 12, 7, 9, 2, 2, 1, 1};

const char* kNames[] = {"", "A", "RED", "PIKACHU", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", "a b", "  ", "\xE3\x83\x94\xE3\x82\xAB",
                        "\xED\x94\xBC\xEC\xB9\xB4\xEC\xB8\x84", "\xE2\x99\x82\xE2\x99\x80", "A\xC3\x97" "B", "N\xC3\x89O", "0", "!?.,-",
                        "\xF0\x9F\x98\x80", "\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A\xE3\x81\x8B\xE3\x81\x8D\xE3\x81\x8F",
                        "Mr. Mime", "x\ty", "'\"\\", "\xEF\xBC\xA1\xEF\xBC\xA2"};

struct Walk {
    const GameSpec& game;
    uint32_t seed;
    int steps;
    bool trace;
    fsx::path sourcePath;
    std::vector<uint8_t> original;
    std::vector<std::string> log;
    std::mt19937 rng;

    std::unique_ptr<Trainer::Trainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    int reopened = 0;

    Walk(const GameSpec& g, uint32_t s, int n, bool t, fsx::path src, std::vector<uint8_t> bytes)
        : game(g), seed(s), steps(n), trace(t), sourcePath(std::move(src)), original(std::move(bytes)), rng(s) {}

    std::string where() const {
        return std::string(game.id) + " seed " + std::to_string(seed);
    }
    void note(const std::string& action) {
        log.push_back(action);
        if (trace) std::cerr << "  [" << game.id << " " << seed << " #" << log.size() << "] " << action << '\n';
    }
    void dumpTail() const {
        std::cerr << "  replay: MONKEY_GAME=" << game.id << " MONKEY_SEED=" << seed << " MONKEY_STEPS=" << steps
                  << " MONKEY_TRACE=1\n  last actions:";
        for (size_t i = log.size() > 30 ? log.size() - 30 : 0; i < log.size(); ++i) std::cerr << ' ' << log[i] << ';';
        std::cerr << '\n';
    }
    bool open() {
        namespace S = PokeVault::Source::ImmutableSnapshot;
        screen.reset();
        trainer.reset();
        std::string error;
        S::File file;
        if (!S::read(sourcePath.string(), sourcePath.parent_path().string(), {}, file, error)) {
            check(false, where() + ": the source could not be read: " + error);
            return false;
        }
        trainer = PokeVault::Legacy::openBackupTrainer(file, game.id, error);
        if (!trainer) {
            check(false, where() + ": the save does not open as " + game.id + ": " + error);
            return false;
        }
        screen = std::make_unique<UI::TrainerViewScreen>(*trainer, game.title, sourcePath.string(), 0, AccountUid{},
                                                         game.kind, game.id, game.emulator);
        ++reopened;
        return true;
    }
    void frame(u64 down = 0, u64 held = 0) {
        PadState pad{down, held | down, 0};
        touch.update();
        screen->update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen->draw(fb);
    }
    void answers() {
        // Whatever the next action opens gets a random answer (or a cancel).
        HostUi::numbers.clear();
        HostUi::textAnswers.clear();
        for (int i = 0; i < 3; ++i) {
            const int kind = static_cast<int>(rng() % 10);
            int value = 0;
            if (kind < 3) value = static_cast<int>(rng() % 5);
            else if (kind < 6) value = static_cast<int>(rng() % 1000);
            else if (kind < 8) value = static_cast<int>(rng() % 70000);
            else value = static_cast<int>(rng());
            HostUi::numbers.push_back({rng() % 4 != 0, value});
            HostUi::textAnswers.push_back({rng() % 4 != 0, kNames[rng() % std::size(kNames)]});
        }
    }
    u64 pickButton(std::string& name) {
        int total = 0;
        for (int w : kWeights) total += w;
        int roll = static_cast<int>(rng() % static_cast<unsigned>(total));
        for (size_t i = 0; i < std::size(kButtons); ++i) {
            roll -= kWeights[i];
            if (roll < 0) { name = kButtonNames[i]; return kButtons[i]; }
        }
        name = "A";
        return HidNpadButton_A;
    }
    void tap(u32 x, u32 y) {
        HostUi::touch = {true, x, y}; frame();
        HostUi::touch = {true, x, y}; frame();
        HostUi::touch = {false, x, y}; frame();
        frame();
    }
    void step() {
        answers();
        const unsigned roll = rng() % 100;
        if (roll < 68) {
            std::string name;
            const u64 b = pickButton(name);
            note(name);
            frame(b);
            frame();
        } else if (roll < 74) {
            // Rapid repeat: the same button on consecutive frames with no release between.
            std::string name;
            const u64 b = pickButton(name);
            const int n = 2 + static_cast<int>(rng() % 6);
            note(name + "x" + std::to_string(n));
            for (int i = 0; i < n; ++i) frame(b);
            frame();
        } else if (roll < 78) {
            // Held repeat (auto-repeat paths): down once, then held for a while.
            std::string name;
            const u64 b = pickButton(name);
            const int n = 5 + static_cast<int>(rng() % 40);
            note("hold " + name + " " + std::to_string(n));
            frame(b);
            for (int i = 0; i < n; ++i) frame(0, b);
            frame();
        } else if (roll < 81) {
            // Two buttons in the same frame.
            std::string a, b;
            const u64 x = pickButton(a), y = pickButton(b);
            note(a + "+" + b);
            frame(x | y);
            frame();
        } else if (roll < 92 && !HostUi::visibleText.empty()) {
            const auto t = HostUi::visibleText[rng() % HostUi::visibleText.size()];
            const u32 x = static_cast<u32>(std::clamp(t.x + 4, 0, 1279)), y = static_cast<u32>(std::clamp(t.y + 4, 0, 719));
            note("tap \"" + t.text.substr(0, 16) + "\"@" + std::to_string(x) + "," + std::to_string(y));
            tap(x, y);
        } else if (roll < 96) {
            const u32 x = rng() % 1280, y = rng() % 720;
            note("tap@" + std::to_string(x) + "," + std::to_string(y));
            tap(x, y);
        } else {
            // Drag between two random points (carry / scroll / multi-select gestures).
            const int x0 = static_cast<int>(rng() % 1280), y0 = static_cast<int>(rng() % 720);
            const int x1 = static_cast<int>(rng() % 1280), y1 = static_cast<int>(rng() % 720);
            note("drag " + std::to_string(x0) + "," + std::to_string(y0) + "->" + std::to_string(x1) + "," + std::to_string(y1));
            const int n = 3 + static_cast<int>(rng() % 6);
            for (int i = 0; i <= n; ++i) {
                HostUi::touch = {true, static_cast<u32>(x0 + (x1 - x0) * i / n), static_cast<u32>(y0 + (y1 - y0) * i / n)};
                frame();
            }
            HostUi::touch = {false, static_cast<u32>(x1), static_cast<u32>(y1)};
            frame();
            frame();
        }
    }
    bool sourceIntact(const std::string& when) {
        const auto now = readAll(sourcePath);
        const bool same = now == original;
        check(same, where() + ": the source save changed on disk (" + when + ", action #" + std::to_string(log.size()) + ")");
        if (!same) dumpTail();
        return same;
    }
    void textsValid() {
        for (const auto& t : HostUi::texts)
            if (!validUtf8(t)) {
                check(false, where() + ": invalid UTF-8 reached the renderer after action #" + std::to_string(log.size()));
                dumpTail();
                return;
            }
        ++checks;
    }
    bool drawn(const std::string& needle) const {
        return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                           [&](const std::string& t) { return t.find(needle) != std::string::npos; });
    }
    // I3: B always gets out. The unsaved-changes question is answered with its own Discard button.
    bool escape() {
        HostUi::numbers.clear();
        HostUi::textAnswers.clear();
        HostUi::touch = {};
        bool tappedLast = false;
        for (int i = 0; i < 120; ++i) {
            if (screen->goBack) return true;
            u64 b = HidNpadButton_B;
            // A question is recognised by its own Discard button: a "Y" (or "A") glyph drawn right
            // before a label starting with "Discard", or the inline "A: Discard & Home" /
            // "Y: Discard & Home" line. Elsewhere Y is an ordinary action (Random DVs, Move): B.
            for (size_t k = 0; k + 1 < HostUi::texts.size(); ++k)
                if (HostUi::texts[k + 1].rfind("Discard", 0) == 0 && (HostUi::texts[k] == "Y" || HostUi::texts[k] == "A")) {
                    b = HostUi::texts[k] == "Y" ? HidNpadButton_Y : HidNpadButton_A;
                    break;
                }
            if (drawn("A: Discard & Home")) b = HidNpadButton_A;
            else if (drawn("Y: Discard")) b = HidNpadButton_Y;
            // A list-style question ("Save bank changes?  Save & Exit / Discard changes / Cancel")
            // has no glyph: tap its Discard option whenever one is visible.
            if (b == HidNpadButton_B && !tappedLast) {
                const auto option = std::find_if(HostUi::visibleText.begin(), HostUi::visibleText.end(),
                                                 [](const HostUi::TextRect& t) { return t.text.rfind("Discard", 0) == 0; });
                if (option != HostUi::visibleText.end()) {
                    if (trace) std::cerr << "  [escape " << i << "] tap \"" << option->text << "\"\n";
                    tap(static_cast<u32>(std::clamp(option->x + 4, 0, 1279)), static_cast<u32>(std::clamp(option->y + 4, 0, 719)));
                    tappedLast = true;   // a Discard label that is not a button: B next time
                    continue;
                }
            }
            if (trace) {
                std::cerr << "  [escape " << i << "] press " << (b == HidNpadButton_B ? "B" : b == HidNpadButton_Y ? "Y" : "A") << " on:";
                for (size_t k = 0; k < HostUi::texts.size() && k < 60; ++k) std::cerr << " | " << HostUi::texts[k];
                std::cerr << '\n';
            }
            tappedLast = false;
            frame(b);
            frame();
        }
        return screen->goBack;
    }
    void run() {
        if (!open()) return;
        frame();
        const int failuresAtStart = failures;
        for (int i = 0; i < steps && failures == failuresAtStart; ++i) {
            step();
            if (trace)
                std::cerr << "      -> mode=" << static_cast<int>(screen->selectedMode) << " detail=" << screen->detailViewActive
                          << " bankExitQ=" << screen->storageExitConfirmActive << " sheet=" << screen->actionSheet.isOpen()
                          << " emptySlotMenu=" << drawn("Empty Slot") << " status=\"" << screen->storageStatus << "\"\n";
            textsValid();
            if (screen->goBack) {
                note("(left the save; reopened)");
                if (!open()) return;
                frame();
            }
            if (i % 40 == 39 && !sourceIntact("mid-walk")) return;
        }
        if (failures != failuresAtStart) { dumpTail(); return; }
        const bool out = escape();
        check(out, where() + ": B never leaves the save from where the walk ended (the player is trapped)");
        if (!out) {
            std::cerr << "  screen when trapped:";
            for (size_t i = 0; i < HostUi::texts.size() && i < 40; ++i) std::cerr << " | " << HostUi::texts[i];
            std::cerr << '\n';
            dumpTail();
        }
        sourceIntact("end of walk");
    }
};

// I4: everything under the private SD root that is not the source and has a save's size must be a
// valid save of the same game.
void exportsValid(const GameSpec& game, const fsx::path& root, const fsx::path& source, size_t saveSize,
                  std::set<std::string>& seen, long& exportsChecked) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    std::error_code ec;
    for (auto it = fsx::recursive_directory_iterator(root, ec); !ec && it != fsx::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        const auto path = it->path();
        if (path == source || !seen.insert(path.string()).second) continue;
        if (it->file_size(ec) != saveSize) continue;
        const std::string name = path.filename().string();
        if (name.find(".tmp") != std::string::npos) {
            check(false, std::string(game.id) + ": a temporary export file was left behind: " + path.string());
            continue;
        }
        std::string error;
        S::File file;
        const bool read = S::read(path.string(), path.parent_path().string(), {}, file, error);
        auto trainer = read ? PokeVault::Legacy::openBackupTrainer(file, game.id, error) : nullptr;
        check(trainer != nullptr, std::string(game.id) + ": exported file " + path.string() + " does not reparse as " + game.id + ": " + error);
        ++exportsChecked;
    }
}

} // namespace

int main(int argc, char** argv) {
    const fsx::path fixtures = fsx::absolute(argc > 1 ? argv[1] : GAMES20_FIXTURE_DIR);
    const auto env = [](const char* name, long fallback) {
        const char* v = std::getenv(name);
        return v && *v ? std::atol(v) : fallback;
    };
    const long seeds = env("MONKEY_SEEDS", 2);
    const long steps = env("MONKEY_STEPS", 600);
    const long oneSeed = env("MONKEY_SEED", -1);
    const bool trace = std::getenv("MONKEY_TRACE") != nullptr;
    const std::string only = std::getenv("MONKEY_GAME") ? std::getenv("MONKEY_GAME") : "";

    char pattern[] = "/tmp/pokebank-games20-monkey-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    if (!temp) { std::cerr << "mkdtemp failed\n"; return EXIT_FAILURE; }
    const fsx::path root = temp, home = fsx::current_path();
    fsx::current_path(root);   // sdmc:/... (exports, config) resolves inside the private directory
    fsx::create_directories("sdmc:");   // the SD root always exists on the device
    HostUi::clampNumbersLikeDevice = true;

    int gamesRun = 0;
    long walks = 0, actions = 0, exportsChecked = 0, reopened = 0;
    for (const auto& game : kGames) {
        if (!only.empty() && only != game.id) continue;
        const auto bytes = readAll(fixtures / (std::string(game.id) + ".sav"));
        if (bytes.empty()) {
            check(false, std::string(game.id) + ": fixture missing in " + fixtures.string() +
                             " (run build-host/qa-gcc/plain/test_qa_matrix --dump-fixtures)");
            continue;
        }
        check(PokeVault::Games::findGame(game.id) != nullptr, std::string(game.id) + " is a known game id");
        const fsx::path dir = root / "saves" / game.id;
        fsx::create_directories(dir);
        const fsx::path source = dir / (std::string(game.id) + ".sav");
        { std::ofstream out(source, std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); }
        ++gamesRun;
        std::set<std::string> seen;
        const int failuresBeforeGame = failures;
        // One report per game is enough to act on; the other games still run (MONKEY_STOP=1 stops).
        for (long s = 0; s < (oneSeed >= 0 ? 1 : seeds) && failures == failuresBeforeGame; ++s) {
            const uint32_t seed = oneSeed >= 0 ? static_cast<uint32_t>(oneSeed)
                                               : static_cast<uint32_t>(1000 * (gamesRun) + s + 1);
            Walk walk(game, seed, static_cast<int>(steps), trace, source, bytes);
            walk.run();
            ++walks;
            actions += static_cast<long>(walk.log.size());
            reopened += walk.reopened;
            exportsValid(game, root / "sdmc:", source, bytes.size(), seen, exportsChecked);
            check(fnv(readAll(source)) == fnv(bytes), std::string(game.id) + ": source hash unchanged after seed " + std::to_string(seed));
        }
        if (failures && std::getenv("MONKEY_STOP")) break;
    }
    if (only.empty() && oneSeed < 0) check(gamesRun == 20, "all 20 Gen I-V releases ran (" + std::to_string(gamesRun) + ")");

    fsx::current_path(home);
    std::error_code ignored;
    if (!failures || !std::getenv("MONKEY_KEEP")) fsx::remove_all(root, ignored);
    if (failures) {
        std::cerr << failures << " failure(s) in " << checks << " checks\n";
        return EXIT_FAILURE;
    }
    std::cout << "Games-20 monkey (real TrainerViewScreen, " << gamesRun << " releases x " << walks / std::max(1, gamesRun)
              << " seeded walks, " << actions << " random actions, " << reopened << " save opens, " << exportsChecked
              << " exported saves reparsed; no crash, source byte-identical, B always exits; " << checks
              << " checks): PASS\n";
    return EXIT_SUCCESS;
}
