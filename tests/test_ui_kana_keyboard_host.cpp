// The in-app keyboard's Japanese name pages (owner Oct 28), driven through the REAL
// UI::Dialogs::KeyboardState::update / drawKeyboard with scripted buttons:
//   * a japaneseKana request opens on Hiragana; every key row fits the card (<= 22 half keys);
//   * typing, ゛ ゜ 小, the mode key (あ -> ア -> １２３ -> あ), the half key, ZL / ZR page cycling and
//     the full-width space all produce text Utils::Gen12Kana::encode accepts;
//   * the 5-character cap holds; a modifier with no result in the font changes nothing;
//   * an ordinary request still opens on Letters and ZL / ZR still cycle Letters / Symbols / Accents.
#include "UI/Dialogs/KeyboardDialog.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "Utils/Gen12KanaText.h"
#include "HostUi.h"

#include <iostream>
#include <string>
#include <vector>

namespace D = UI::Dialogs;
namespace K = Utils::Gen12Kana;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

struct Board {
    D::KeyboardState kb;
    UI::TouchInput touch;
    UI::PKSEFramebuffer fb;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        kb.update(pad, touch);
        HostUi::texts.clear();
        D::drawKeyboard(kb, fb);
    }
    void press(u64 b) { frame(b); frame(); }
    // Move the selection to the key with this label, then press A.
    bool typeKey(const std::string& label) {
        for (int i = 0; i < static_cast<int>(kb.keys.size()); ++i) {
            if (kb.keys[i].lowerLabel != label) continue;
            kb.selectedKeyIndex = i;
            press(HidNpadButton_A);
            return true;
        }
        return false;
    }
    bool pressAction(D::KeyboardKeyAction action) {
        for (int i = 0; i < static_cast<int>(kb.keys.size()); ++i) {
            if (kb.keys[i].action != action) continue;
            kb.selectedKeyIndex = i;
            press(HidNpadButton_A);
            return true;
        }
        return false;
    }
};

Utils::KeyboardRequest japanese() {
    Utils::KeyboardRequest r;
    r.header = "Nickname";
    r.maxChars = 5;
    r.japaneseKana = true;
    return r;
}

bool encodes(const std::string& text) {
    std::vector<uint8_t> out;
    return K::encode(text, 5, out) == K::Result::Ok;
}
}  // namespace

int main() {
    Board b;
    b.kb.open(japanese(), 1280, 720);
    b.frame();
    check(b.kb.page == D::KeyboardPage::HiraganaA, "a Japanese name prompt opens on Hiragana");
    // Rows fit the card: the widest row defines the card, every key lies inside it.
    for (const auto& key : b.kb.keys)
        check(key.keyX >= b.kb.cardX && key.keyX + key.keyWidth <= b.kb.cardX + b.kb.cardWidth,
              "key inside the card: " + key.lowerLabel);
    check(b.kb.cardWidth <= 1280 - 32, "card fits the screen");

    // ピカチュウ: katakana page, ヒ + ゜, カ, チ, ユ + 小, ウ.
    check(b.pressAction(D::KeyboardKeyAction::KanaNextMode) && b.kb.page == D::KeyboardPage::KatakanaA, "mode key -> ア");
    check(b.typeKey("ヒ"), "ヒ key");
    check(b.kb.page == D::KeyboardPage::KatakanaA, "typing keeps the page");
    check(b.pressAction(D::KeyboardKeyAction::KanaOtherHalf) && b.kb.page == D::KeyboardPage::KatakanaB, "half key -> マ…");
    check(b.pressAction(D::KeyboardKeyAction::KanaHandakuten) && b.kb.text == "ピ", "゜ makes ピ (got " + b.kb.text + ")");
    b.pressAction(D::KeyboardKeyAction::KanaOtherHalf);
    b.typeKey("カ"); b.typeKey("チ");
    b.pressAction(D::KeyboardKeyAction::KanaOtherHalf);
    b.typeKey("ユ");
    check(b.pressAction(D::KeyboardKeyAction::KanaSmall) && b.kb.text == "ピカチュ", "小 makes ュ (got " + b.kb.text + ")");
    b.pressAction(D::KeyboardKeyAction::KanaOtherHalf);
    b.typeKey("ウ");
    check(b.kb.text == "ピカチュウ" && encodes(b.kb.text), "typed ピカチュウ encodes (got " + b.kb.text + ")");
    // Cap: a sixth character is refused.
    b.typeKey("ア");
    check(b.kb.text == "ピカチュウ" && b.kb.refusedFrames > 0, "the 5-character cap holds");
    // ゛ on ウ: ヴ is not in the font, nothing changes.
    b.pressAction(D::KeyboardKeyAction::KanaOtherHalf);
    const auto before = b.kb.text;
    b.pressAction(D::KeyboardKeyAction::KanaDakuten);
    check(b.kb.text == before, "゛ on ウ changes nothing (no ヴ in the font)");

    // Fresh prompt: hiragana, voiced, full-width space through Y, symbols page via the mode key.
    Board h;
    h.kb.open(japanese(), 1280, 720);
    h.frame();
    h.typeKey("か");
    h.pressAction(D::KeyboardKeyAction::KanaOtherHalf);
    h.pressAction(D::KeyboardKeyAction::KanaDakuten);
    check(h.kb.text == "が", "゛ makes が (got " + h.kb.text + ")");
    h.press(HidNpadButton_Y);
    check(h.kb.text == "が　", "Y types the full-width space");
    h.pressAction(D::KeyboardKeyAction::KanaNextMode);   // -> ア
    h.pressAction(D::KeyboardKeyAction::KanaNextMode);   // -> １２３
    check(h.kb.page == D::KeyboardPage::KanaSymbols, "mode key -> １２３");
    h.typeKey("１");
    h.pressAction(D::KeyboardKeyAction::KanaNextMode);
    check(h.kb.page == D::KeyboardPage::HiraganaA, "mode key -> あ again");
    check(h.kb.text == "が　１" && encodes(h.kb.text), "mixed text encodes (got " + h.kb.text + ")");

    // ZL / ZR walk the five kana pages and never reach Latin pages.
    Board z;
    z.kb.open(japanese(), 1280, 720);
    z.frame();
    const D::KeyboardPage order[] = {D::KeyboardPage::HiraganaB, D::KeyboardPage::KatakanaA, D::KeyboardPage::KatakanaB,
                                     D::KeyboardPage::KanaSymbols, D::KeyboardPage::HiraganaA};
    for (const auto expected : order) {
        z.press(HidNpadButton_ZR);
        check(z.kb.page == expected, "ZR walks the kana pages");
    }
    z.press(HidNpadButton_ZL);
    check(z.kb.page == D::KeyboardPage::KanaSymbols, "ZL walks back");
    // Every character key on every kana page encodes.
    for (int i = 0; i < 5; ++i) {
        for (const auto& key : z.kb.keys)
            if (key.action == D::KeyboardKeyAction::Type)
                check(encodes(key.lowerLabel), "kana key encodes: " + key.lowerLabel);
        z.press(HidNpadButton_ZR);
    }
    check(!z.kb.navHint().empty() && z.kb.navHint().find("ZL/ZR: Page") != std::string::npos, "kana nav hint");

    // An ordinary prompt is unchanged.
    Board l;
    Utils::KeyboardRequest plain;
    plain.header = "Nickname";
    plain.maxChars = 10;
    l.kb.open(plain, 1280, 720);
    l.frame();
    check(l.kb.page == D::KeyboardPage::Letters, "ordinary prompts still open on Letters");
    l.press(HidNpadButton_ZR);
    check(l.kb.page == D::KeyboardPage::Symbols, "ordinary ZR still goes to Symbols");
    l.press(HidNpadButton_Y);
    check(l.kb.text == " ", "ordinary Y still types a half-width space");

    if (failures) { std::cerr << failures << " kana keyboard check(s) failed\n"; return 1; }
    std::cout << "Kana keyboard (real KeyboardState): pages, modifiers, mode key, cap and Latin prompts PASS\n";
    return 0;
}
