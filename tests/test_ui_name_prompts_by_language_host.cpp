// Round 15 keyboard lane, through the REAL TrainerViewScreen (tests/host-ui harness): the owner's
// Round 13 R14 FAIL was "the nicknames and the name at the top left changes but thats it, no
// keyboard changes or anything" -- changing a record's language changed its stored name but the
// keyboard stayed Latin. Here the record's language is set first, then Party -> Edit -> Nickname /
// Original Trainer is opened on the real screen, and the keyboard the shared prompt shows (the
// promptName stub opens the real UI::Dialogs::KeyboardState with the same request) must be:
//   * Gen IV / Gen V Korean record   -> opens on Hangul, cap 5;
//   * Gen IV / Gen V Japanese record -> opens on hiragana, cap 5, ABC page full width;
//   * Gen III Japanese record        -> opens on hiragana (GEN3_JP), cap 5;
//   * English records                -> Latin pages, Gen V accents live (no longer ASCII-only);
// and a name typed there in the record's script passes the field the editor opened it with. (The
// writer round trips -- store, export, strict reparse -- are tests/test_text_field_capability_matrix.)
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5NativeText.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "UI/Dialogs/KeyboardDialog.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
namespace D = UI::Dialogs;
using View = UI::TrainerViewScreen::ViewMode;
using SK = PokeVault::Safety::SourceKind;

std::vector<std::shared_ptr<void>> keepAlive;
template <class T>
T* keep(std::unique_ptr<T> p) {
    T* raw = p.get();
    keepAlive.push_back(std::shared_ptr<void>(std::move(p)));
    return raw;
}

struct Driver {
    explicit Driver(UI::TrainerViewScreen& s) : screen(s) {}
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void press(u64 button) { frame(button); frame(); }
};

UI::TrainerViewScreen* screenFor(Trainer::Trainer& trainer, const std::string& title, const std::string& id,
                                 SK kind) {
    return keep(std::make_unique<UI::TrainerViewScreen>(trainer, title, "/host/" + id + ".sav", 0, AccountUid{},
                                                        kind, id, "DraStic"));
}

// Party slot 1 -> A (actions) -> Down -> A (Edit), then walk Details with Down until the prompt
// whose header is `header` opens; returns that prompt, or nullptr.
const HostUi::NamePrompt* openNamePrompt(Driver& d, const std::string& header, const std::string& answer = {}) {
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Party;
    d.screen.selectedItemIndex = 0;
    d.screen.selectedPartyIndex = 0;
    d.frame();
    d.frame();
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    for (int step = 0; step < 8; ++step) {
        d.press(HidNpadButton_Down);
        const std::size_t before = HostUi::namePrompts.size();
        if (!answer.empty()) HostUi::textAnswers.push_back({true, answer});
        HostUi::numbers.clear();
        d.press(HidNpadButton_A);
        if (HostUi::namePrompts.size() > before) {
            if (HostUi::namePrompts.back().header == header) return &HostUi::namePrompts.back();
        } else if (!answer.empty() && !HostUi::textAnswers.empty()) {
            HostUi::textAnswers.pop_back();   // that row was not a name row
        }
    }
    return nullptr;
}

bool has(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

// ---- Gen IV ---------------------------------------------------------------------------------
UI::TrainerViewScreen* openPlatinum(uint8_t language) {
    namespace G4 = PokeVault::Integration::Gen4;
    auto raw = qasynth::gen4::save(qasynth::gen4::Kind::Platinum);
    qasynth::gen4::forge(raw);
    std::string error;
    auto editor = G4::Gen4StagedPokemonEditor::create(raw, G4::Layout::Platinum, "platinum_nds", &error);
    if (!editor) return nullptr;
    auto mon = editor->editablePartyPokemon(0, &error);
    if (!mon || !mon->setLanguage(language) || !editor->commitPartyPokemon(0, *mon, &error)) return nullptr;
    auto bytes = std::make_shared<std::vector<uint8_t>>(editor->finalizedBytes(&error));
    keepAlive.push_back(bytes);
    auto parsed = G4::Gen4ReadOnlySave::parse(*bytes, G4::Layout::Platinum, "platinum_nds", &error);
    if (!parsed) return nullptr;
    auto kept = std::make_shared<G4::Gen4ReadOnlySave>(*parsed);
    keepAlive.push_back(kept);
    auto* trainer = keep(PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*kept, "platinum_nds", error));
    return trainer ? screenFor(*trainer, "Pokemon Platinum", "platinum_nds", SK::ExternalLegacy) : nullptr;
}

// ---- Gen V ----------------------------------------------------------------------------------
UI::TrainerViewScreen* openBlack(uint8_t language) {
    namespace G5 = PokeVault::Integration::Gen5;
    const auto input = HostUiFixtures::gen5Fixture();
    std::string error;
    auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    if (!parsed) return nullptr;
    G5::Gen5StagedPokemonWorkspace ws(*parsed);
    if (!ws.stageParty(0, G5::StagedPokemon5Record::Field::Language, 0, language, &error)) return nullptr;
    const auto candidate = G5::buildStagedSaveCandidate(ws, &error);
    if (!candidate) return nullptr;
    auto bytes = std::make_shared<std::vector<uint8_t>>(candidate->bytes);
    keepAlive.push_back(bytes);
    auto again = G5::Gen5ReadOnlySave::parse(*bytes, "black_nds", &error, G5::SaveCopySelection::Primary);
    if (!again) return nullptr;
    auto* trainer = keep(PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*again, "black_nds", error));
    return trainer ? screenFor(*trainer, "Pokemon Black", "black_nds", SK::ExternalLegacy) : nullptr;
}

// ---- Gen III --------------------------------------------------------------------------------
UI::TrainerViewScreen* openFireRed(uint8_t language) {
    namespace G3 = PokeVault::Integration::Gen3;
    std::string error;
    const auto base = HostUiFixtures::gen3Fixture(0);
    auto editor = G3::StagedPokemonEditor::create(base, G3::SourceGame::FireRedGBA, error);
    if (!editor) return nullptr;
    G3::BoxPokemonEdit edit;
    edit.language = language;
    if (language == 1) edit.nickname = "ピカチュウ";
    if (!editor->stagePartyPokemonEdit(0, edit, error)) { std::cerr << "gen3 edit: " << error << '\n'; return nullptr; }
    auto bytes = std::make_shared<std::vector<uint8_t>>(editor->finalizedBytes(error));
    keepAlive.push_back(bytes);
    auto parsed = G3::parse(*bytes, G3::SourceGame::FireRedGBA);
    if (!parsed) return nullptr;
    auto* trainer = keep(PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error));
    return trainer ? screenFor(*trainer, "Pokemon FireRed", "firered_gba", SK::RetroArchLegacy) : nullptr;
}

void expectPrompt(const std::string& game, const HostUi::NamePrompt* p, D::KeyboardPage start, int cap,
                  const std::vector<std::string>& live, const std::vector<std::string>& disabled = {}) {
    check(p != nullptr, game + ": the name prompt opened from the real editor");
    if (!p) return;
    check(p->startPage == static_cast<int>(start),
          game + ": keyboard opens on page " + std::to_string(p->startPage) + ", expected " +
              std::to_string(static_cast<int>(start)));
    check(p->maxChars == cap, game + ": cap " + std::to_string(p->maxChars) + " != " + std::to_string(cap));
    for (const auto& k : live) check(has(p->enabledKeys, k), game + ": key " + k + " is live");
    for (const auto& k : disabled) check(has(p->disabledKeys, k), game + ": key " + k + " is disabled");
}
}  // namespace

int main() {
    using P = D::KeyboardPage;
    // Gen IV Korean / Japanese / English record.
    if (auto* s = openPlatinum(8)) {
        Driver d(*s);
        expectPrompt("Gen IV Korean nickname", openNamePrompt(d, "Nickname"), P::Hangul, 5, {"ㅍ", "ㅏ"});
        check(!HostUi::namePrompts.empty() &&
                  std::find(HostUi::namePrompts.back().pages.begin(), HostUi::namePrompts.back().pages.end(),
                            static_cast<int>(P::HiraganaA)) == HostUi::namePrompts.back().pages.end(),
              "Gen IV Korean: no kana pages");
    } else check(false, "Gen IV Korean fixture");
    if (auto* s = openPlatinum(8)) {
        Driver d(*s);
        expectPrompt("Gen IV Korean OT", openNamePrompt(d, "Original Trainer"), P::Hangul, 5, {"ㄱ"});
    }
    if (auto* s = openPlatinum(1)) {
        Driver d(*s);
        expectPrompt("Gen IV Japanese nickname", openNamePrompt(d, "Nickname", "ピカチュウ"), P::HiraganaA, 5,
                     {"あ", "か"});
        check(HostUi::namePrompts.back().answerPassedField, "Gen IV Japanese: ピカチュウ passes the field");
    } else check(false, "Gen IV Japanese fixture");
    if (auto* s = openPlatinum(2)) {
        Driver d(*s);
        expectPrompt("Gen IV English nickname", openNamePrompt(d, "Nickname"), P::Letters, 10, {"q", "m"});
    }
    // Gen V Korean / Japanese / English record.
    if (auto* s = openBlack(8)) {
        Driver d(*s);
        expectPrompt("Gen V Korean nickname", openNamePrompt(d, "Nickname"), P::Hangul, 5, {"ㅍ"});
    } else check(false, "Gen V Korean fixture");
    if (auto* s = openBlack(1)) {
        Driver d(*s);
        expectPrompt("Gen V Japanese nickname", openNamePrompt(d, "Nickname", "ピカ"), P::HiraganaA, 5, {"あ"});
        check(HostUi::namePrompts.back().answerPassedField, "Gen V Japanese: ピカ passes the field");
    } else check(false, "Gen V Japanese fixture");
    if (auto* s = openBlack(2)) {
        Driver d(*s);
        expectPrompt("Gen V English OT", openNamePrompt(d, "Original Trainer", "Chéren"), P::Letters, 7, {"q"});
        check(HostUi::namePrompts.back().answerPassedField, "Gen V English: Chéren passes the field (not ASCII-only)");
    }
    // Gen III Japanese / English record.
    if (auto* s = openFireRed(1)) {
        Driver d(*s);
        expectPrompt("Gen III Japanese nickname", openNamePrompt(d, "Generation III Nickname"), P::HiraganaA, 5,
                     {"あ"});
    } else check(false, "Gen III Japanese fixture");
    if (auto* s = openFireRed(2)) {
        Driver d(*s);
        expectPrompt("Gen III English nickname", openNamePrompt(d, "Generation III Nickname"), P::Letters, 10,
                     {"q"});
    }
    std::cout << HostUi::namePrompts.size() << " name prompts recorded\n";
    for (const auto& p : HostUi::namePrompts)
        std::cout << "  " << p.header << " page " << p.startPage << " cap " << p.maxChars << '\n';
    if (failures) {
        std::cerr << failures << " name-prompt-by-language failure(s)\n";
        return 1;
    }
    std::cout << "Real Gen III / IV / V editors open the shared keyboard on the record's script PASS\n";
    return 0;
}
