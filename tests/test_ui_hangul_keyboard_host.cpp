// Round 11 H27: the in-app keyboard's Korean page, driven through the REAL
// UI::Dialogs::KeyboardState::update / drawKeyboard with scripted buttons:
//   * a koreanHangul request opens on Hangul; every key row fits the card (<= 22 half keys);
//   * typing jamo composes syllables (피카츄), Shift gives tense consonants, a final consonant moves
//     to the next syllable, and the result is text the Gen IV writer's table stores;
//   * composition only continues the keyboard's own run: initial text, a caret move or Backspace
//     ends it, so existing names are never recomposed;
//   * the character cap holds when a final would split into a new syllable;
//   * Letters / Symbols carry a 한 key back; ZL / ZR walk Hangul -> Letters -> Symbols -> Accents;
//   * an ordinary request has no Hangul page and is unchanged.
#include "UI/Dialogs/KeyboardDialog.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "Utils/Gen4Text.h"
#include "Utils/HangulComposer.h"
#include "Utils/StringHelpers.h"
#include "HostUi.h"

#include <iostream>
#include <string>
#include <vector>

namespace D = UI::Dialogs;

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
    bool typeKey(const std::string& label) {
        for (int i = 0; i < static_cast<int>(kb.keys.size()); ++i) {
            if (kb.keys[i].lowerLabel != label) continue;
            kb.selectedKeyIndex = i;
            press(HidNpadButton_A);
            return true;
        }
        return false;
    }
    void typeKeys(const std::vector<std::string>& labels) {
        for (const auto& l : labels) check(typeKey(l), "key " + l);
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
    bool hasAction(D::KeyboardKeyAction action) const {
        for (const auto& k : kb.keys) if (k.action == action) return true;
        return false;
    }
};

Utils::KeyboardRequest korean(int maxChars = 10, const std::string& initial = {}) {
    Utils::KeyboardRequest r;
    r.header = "Nickname";
    r.maxChars = maxChars;
    r.initialText = initial;
    r.koreanHangul = true;
    return r;
}

bool storableGen4(const std::string& text) {
    return Utils::gen4CanEncode(Utils::utf8ToUtf16(text));
}
}  // namespace

int main() {
    Board b;
    b.kb.open(korean(), 1280, 720);
    b.frame();
    check(b.kb.page == D::KeyboardPage::Hangul, "a Korean name prompt opens on Hangul");
    for (const auto& key : b.kb.keys)
        check(key.keyX >= b.kb.cardX && key.keyX + key.keyWidth <= b.kb.cardX + b.kb.cardWidth,
              "key inside the card: " + key.lowerLabel);
    check(b.kb.cardWidth <= 1280 - 32, "card fits the screen");
    for (const auto& key : b.kb.keys)
        if (key.action == D::KeyboardKeyAction::Type)
            check(Utils::Hangul::isJamoKey(Utils::Hangul::decodeOne(key.lowerLabel)) &&
                  Utils::Hangul::isJamoKey(Utils::Hangul::decodeOne(key.upperLabel)),
                  "every Hangul key is a jamo: " + key.lowerLabel);
    check(b.hasAction(D::KeyboardKeyAction::Shift) && b.hasAction(D::KeyboardKeyAction::Backspace) &&
          b.hasAction(D::KeyboardKeyAction::ShowLetters) && b.hasAction(D::KeyboardKeyAction::ShowSymbols) &&
          b.hasAction(D::KeyboardKeyAction::SystemKeyboard) && b.hasAction(D::KeyboardKeyAction::Accept),
          "Hangul page carries Shift, Backspace, ABC, ?123, System and OK");

    // 피카츄 (Pikachu): ㅍ ㅣ ㅋ ㅏ ㅊ ㅠ.
    b.typeKeys({"ㅍ", "ㅣ"});
    check(b.kb.text == "피", "ㅍ + ㅣ -> 피 (got " + b.kb.text + ")");
    b.typeKey("ㅋ");
    check(b.kb.text == "핔", "피 + ㅋ -> 핔, ㅋ as the final (got " + b.kb.text + ")");
    b.typeKeys({"ㅏ", "ㅊ", "ㅠ"});
    check(b.kb.text == "피카츄" && storableGen4(b.kb.text), "typed 피카츄, storable in Gen IV (got " + b.kb.text + ")");
    check(b.kb.caretOffset == b.kb.text.size(), "caret after the composed text");

    // Shift: tense consonant, one-shot.
    Board s;
    s.kb.open(korean(), 1280, 720);
    s.frame();
    s.press(HidNpadButton_X);
    s.typeKeys({"ㄱ"});                       // lower label ㄱ, shifted -> ㄲ
    s.typeKeys({"ㅗ", "ㅂ", "ㅜ", "ㄱ", "ㅣ"});   // 부 + ㄱ -> 북, + ㅣ -> 부기
    check(s.kb.text == "꼬부기" && storableGen4(s.kb.text), "Shift ㄱ -> ㄲ, 꼬부기 (got " + s.kb.text + ")");
    // Backspace removes the whole syllable and ends the run: ㅣ after 부 stays a jamo.
    s.pressAction(D::KeyboardKeyAction::Backspace);
    check(s.kb.text == "꼬부", "Backspace removes a whole syllable (got " + s.kb.text + ")");
    s.typeKey("ㅣ");
    check(s.kb.text == "꼬부ㅣ", "after Backspace a vowel does not recompose (got " + s.kb.text + ")");

    // Initial text is never recomposed; a caret move ends the run.
    Board i;
    i.kb.open(korean(10, "가"), 1280, 720);
    i.frame();
    i.typeKey("ㄱ");
    check(i.kb.text == "가ㄱ", "initial text is not recomposed (got " + i.kb.text + ")");
    i.press(HidNpadButton_L);
    i.press(HidNpadButton_R);
    i.typeKey("ㅏ");
    check(i.kb.text == "가ㄱㅏ", "a caret move ends the composing run (got " + i.kb.text + ")");

    // Cap: at 2 characters, 각 + ㅏ would become 가가 + ... = 3; refused, text unchanged.
    Board c;
    c.kb.open(korean(2), 1280, 720);
    c.frame();
    c.typeKeys({"ㄴ", "ㅏ", "ㄱ", "ㅏ", "ㄱ"});
    check(c.kb.text == "나각", "two syllables at the cap (got " + c.kb.text + ")");
    c.typeKey("ㅏ");
    check(c.kb.text == "나각" && c.kb.refusedFrames > 0, "a split past the cap is refused (got " + c.kb.text + ")");

    // Page keys and ZL / ZR.
    Board z;
    z.kb.open(korean(), 1280, 720);
    z.frame();
    z.press(HidNpadButton_ZR);
    check(z.kb.page == D::KeyboardPage::Letters, "ZR: Hangul -> Letters");
    check(z.hasAction(D::KeyboardKeyAction::ShowHangul) && !z.hasAction(D::KeyboardKeyAction::ShowAccents),
          "a Korean Letters page leads back to Hangul");
    z.typeKey("a");
    z.press(HidNpadButton_ZR);
    check(z.kb.page == D::KeyboardPage::Symbols && z.hasAction(D::KeyboardKeyAction::ShowHangul), "ZR: -> Symbols");
    z.press(HidNpadButton_ZR);
    check(z.kb.page == D::KeyboardPage::Accents, "ZR: -> Accents");
    z.press(HidNpadButton_ZR);
    check(z.kb.page == D::KeyboardPage::Hangul, "ZR: -> Hangul again");
    z.press(HidNpadButton_ZL);
    check(z.kb.page == D::KeyboardPage::Accents, "ZL walks back");
    z.press(HidNpadButton_ZL);
    z.press(HidNpadButton_ZL);
    check(z.kb.page == D::KeyboardPage::Letters, "ZL -> Letters");
    check(z.pressAction(D::KeyboardKeyAction::ShowHangul) && z.kb.page == D::KeyboardPage::Hangul, "한 key -> Hangul");
    z.typeKey("ㅏ");
    check(z.kb.text == "aㅏ", "a jamo after Latin text is appended (got " + z.kb.text + ")");

    // An ordinary prompt is unchanged: no Hangul page, Accents key, ZR order.
    Board l;
    Utils::KeyboardRequest plain;
    plain.header = "Nickname";
    plain.maxChars = 10;
    l.kb.open(plain, 1280, 720);
    l.frame();
    check(l.kb.page == D::KeyboardPage::Letters, "ordinary prompts still open on Letters");
    check(!l.hasAction(D::KeyboardKeyAction::ShowHangul) && l.hasAction(D::KeyboardKeyAction::ShowAccents),
          "ordinary Letters page keeps its Accents key");
    for (int n = 0; n < 3; ++n) {
        l.press(HidNpadButton_ZR);
        check(l.kb.page != D::KeyboardPage::Hangul, "ordinary ZR never reaches Hangul");
    }
    check(l.kb.page == D::KeyboardPage::Letters, "ordinary ZR cycles Letters / Symbols / Accents");

    if (failures) { std::cerr << failures << " Hangul keyboard check(s) failed\n"; return 1; }
    std::cout << "Hangul keyboard (real KeyboardState): composition, run boundaries, cap and page keys PASS\n";
    return 0;
}
