// Japanese Gen I / II name prompts (owner Oct 28): on a Japanese save the Nickname / OT / Trainer
// name prompts open the kana keyboard (Utils::promptJapaneseName, capped at 5 characters); every
// other save keeps the ordinary prompt. Linked against the host-UI archive; the prompts are the
// recording stubs in tests/host-ui/HostUiStubs.cpp.
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "UI/ClassicJapaneseNames.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
uint8_t diff8(std::span<const uint8_t> s) {
    uint8_t v = 0xFF;
    for (uint8_t b : s) v = static_cast<uint8_t>(v - b);
    return v;
}
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
}  // namespace

int main() {
    namespace G1 = PokeVault::Integration::Gen1;
    namespace Names = UI::ClassicJapaneseNames;

    // Minimal Japanese Red (the same layout facts as test_rby_readonly_bridge's Japanese case).
    std::vector<uint8_t> jp(0x8000, 0);
    std::fill(jp.begin() + 0x2598, jp.begin() + 0x2598 + 6, 0x50);
    jp[0x2ED6] = 0xFF; jp[0x302E] = 0xFF; jp[0x25C5] = 0xFF; jp[0x27DD] = 0xFF;
    jp[0x3594] = diff8(std::span<const uint8_t>(jp).subspan(0x2598, 0x0FFC));
    const auto parsed = G1::parse(jp, G1::SourceGame::Red);
    check(static_cast<bool>(parsed), "Japanese Red parses");
    if (!parsed) return 1;
    std::string error;
    auto rby = PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error);
    check(rby && rby->japaneseLayout(), "Japanese Red trainer opens");
    if (!rby) return 1;
    check(Names::japaneseSave(*rby, "red_gb"), "a Japanese Gen I save is recognised");

    HostUi::textAnswers.push_back({true, "ピカ"});
    const auto kana = Names::prompt(*rby, "red_gb", "Generation I Nickname", "1-10 supported Gen I characters", "", 10);
    check(HostUi::japanesePromptCount == 1 && HostUi::lastJapaneseMaxChars == 5,
          "Japanese save: Nickname opens the kana prompt capped at 5 (cap " +
              std::to_string(HostUi::lastJapaneseMaxChars) + ")");
    check(kana.accepted && kana.text == "ピカ", "the kana answer comes back unchanged");

    // A non-Japanese save keeps the ordinary prompt.
    const auto input = HostUiFixtures::gen5Fixture();
    namespace G5 = PokeVault::Integration::Gen5;
    const auto g5 = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto black = g5 ? PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*g5, "black_nds", error) : nullptr;
    check(black != nullptr, "Gen V fixture opens");
    if (black) {
        check(!Names::japaneseSave(*black, "black_nds"), "a non-Gen I/II save is not treated as Japanese Gen I/II");
        const int before = HostUi::promptCount;
        HostUi::textAnswers.push_back({true, "ASH"});
        const auto latin = Names::prompt(*black, "black_nds", "Nickname", "guide", "", 10);
        check(HostUi::japanesePromptCount == 1 && HostUi::promptCount == before + 1 && latin.text == "ASH",
              "other saves keep the ordinary prompt");
    }

    // Wiring: every Gen I / II Nickname and OT prompt goes through the helper; the Trainer page
    // switches on nameJapanese.
    for (const char* file : {"src/UI/Gen1PokemonEditorOverlayFoundation.inc", "src/UI/Gen1PokemonEditorOverlayUX.inc",
                             "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
                             "src/UI/Gen2PokemonEditorFoundation.inc", "src/UI/Gen2UnifiedPokemonWorkspace.inc"}) {
        const auto src = read(file);
        for (const char* old : {"Utils::promptText(\"Generation I Nickname\"", "Utils::promptText(\"Generation I OT\"",
                                "Utils::promptText(\"Gen II Nickname\"", "Utils::promptText(\"Gen II OT Name\"",
                                "Utils::promptText(\"Nickname\"", "Utils::promptText(\"OT Name\""})
            check(src.find(old) == std::string::npos, std::string(file) + " still prompts directly: " + old);
    }
    const auto session = read("include/UI/ClassicTrainerSessionInput.h");
    check(session.find("fields.nameJapanese\n            ? Utils::promptJapaneseName(\"Trainer Name\"") != std::string::npos,
          "Trainer page name prompt switches to kana on Japanese saves");
    check(read("src/UI/Gen1PokemonEditorOverlayFoundation.inc").find("fields.nameJapanese = UI::ClassicJapaneseNames::japaneseSave(screen.trainer, screen.sourceGameId);") != std::string::npos &&
              read("src/UI/Gen2TrainerSessionOverlay.inc").find("fields.nameJapanese = UI::ClassicJapaneseNames::japaneseSave(screen.trainer, screen.sourceGameId);") != std::string::npos,
          "Gen I and Gen II Trainer pages set nameJapanese from the save");

    if (failures) { std::cerr << failures << " Japanese name prompt check(s) failed\n"; return 1; }
    std::cout << "Japanese Gen I/II name prompts: kana keyboard on Japanese saves, ordinary elsewhere PASS\n";
    return 0;
}
