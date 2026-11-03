// Japanese Gen I / II name prompts (owner Oct 28): on a Japanese save the Nickname / OT / Trainer
// name prompts open the kana keyboard capped at 5 characters; every other save keeps the Latin
// pages. Round 15 (owner: "audit EVERY text-entry surface ... one shared keyboard"; SHARED_UX P1-2 /
// P1-5): the prompts now go through the ONE name prompt, Utils::promptName with the field's
// capability, instead of the separate promptJapaneseName -- so this test checks the page the real
// keyboard opens on (recorded by the promptName stub) rather than which helper was called.
// Linked against the host-UI archive; the prompts are the recording stubs in HostUiStubs.cpp.
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "UI/ClassicJapaneseNames.h"
#include "UI/Dialogs/KeyboardDialog.h"

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

    namespace D = UI::Dialogs;
    namespace T = PokeBank::Text;
    HostUi::textAnswers.push_back({true, "ピカ"});
    const auto kana = Names::promptName(*rby, "red_gb", T::FieldKind::Nickname, "Generation I Nickname", "");
    check(HostUi::namePrompts.size() == 1 && HostUi::namePrompts.back().maxChars == 5 &&
              HostUi::namePrompts.back().startPage == static_cast<int>(D::KeyboardPage::HiraganaA),
          "Japanese save: Nickname opens the kana keyboard capped at 5");
    check(kana.accepted && kana.text == "ピカ", "the kana answer comes back unchanged");

    // A non-Japanese save keeps the ordinary prompt.
    const auto input = HostUiFixtures::gen5Fixture();
    namespace G5 = PokeVault::Integration::Gen5;
    const auto g5 = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto black = g5 ? PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*g5, "black_nds", error) : nullptr;
    check(black != nullptr, "Gen V fixture opens");
    if (black) {
        check(!Names::japaneseSave(*black, "black_nds"), "a non-Gen I/II save is not treated as Japanese Gen I/II");
        HostUi::textAnswers.push_back({true, "ASH"});
        const auto latin = Names::promptName(*black, "black_nds", T::FieldKind::Nickname, "Nickname", "");
        check(HostUi::namePrompts.size() == 2 &&
                  HostUi::namePrompts.back().startPage == static_cast<int>(D::KeyboardPage::Letters) &&
                  HostUi::namePrompts.back().maxChars == 10 && latin.text == "ASH",
              "other saves keep the Latin pages");
    }

    // Wiring: every Gen I / II Nickname and OT prompt goes through the helper; the Trainer page
    // takes the save's trainer-name field (kana on a Japanese save) and opens Utils::promptName.
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
    check(session.find("Utils::promptName(*fields.nameField, \"Trainer Name\", name)") != std::string::npos,
          "Trainer page name prompt is the shared name prompt with the save's field");
    const std::string trainerField =
        "fields.nameField = UI::ClassicJapaneseNames::field(screen.trainer, screen.sourceGameId,";
    check(read("src/UI/Gen1PokemonEditorOverlayFoundation.inc").find(trainerField) != std::string::npos &&
              read("src/UI/Gen2TrainerSessionOverlay.inc").find(trainerField) != std::string::npos,
          "Gen I and Gen II Trainer pages take the trainer-name field from the save");
    // The Japanese trainer-name field is the kana one, 5 characters.
    const auto trainerName = Names::field(*rby, "red_gb", T::FieldKind::TrainerName);
    check(trainerName->maxCharacters() == 5 && trainerName->keyboard().kana && !trainerName->keyboard().latin,
          "Japanese Gen I trainer name: kana pages, 5 characters, no Latin page");

    if (failures) { std::cerr << failures << " Japanese name prompt check(s) failed\n"; return 1; }
    std::cout << "Japanese Gen I/II name prompts: kana keyboard on Japanese saves, ordinary elsewhere PASS\n";
    return 0;
}
