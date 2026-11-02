// Round 14 P0-5 (SHARED_UX_REQUIREMENTS.md): the console's System keyboard is an input source, not
// an exemption. Driven through the REAL UI::Dialogs::KeyboardState with the System key pressed and
// a scripted swkbd answer (HostUi::textAnswers): an answer that breaks the prompt's own rules (the
// character cap, the kana font of a Japanese Gen I / II prompt, malformed UTF-8, or the field's
// TextFieldCapability check) is refused with a reason and the previous text is kept; a valid answer
// is accepted as before. With a field rule, OK refuses text the field cannot store.
#include "UI/Dialogs/KeyboardDialog.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "Text/TextFieldCapability.h"
#include "HostUi.h"

#include <iostream>
#include <string>

namespace D = UI::Dialogs;
namespace T = PokeBank::Text;

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
    bool pressAction(D::KeyboardKeyAction action) {
        for (int i = 0; i < static_cast<int>(kb.keys.size()); ++i) {
            if (kb.keys[i].action != action) continue;
            kb.selectedKeyIndex = i;
            press(HidNpadButton_A);
            return true;
        }
        return false;
    }
    // The System key, answered by the scripted console keyboard.
    void system(const std::string& answer) {
        HostUi::textAnswers.push_back({true, answer});
        check(pressAction(D::KeyboardKeyAction::SystemKeyboard), "System key present");
        frame();   // the release the hand-off waits for
    }
};

Utils::KeyboardRequest request(int maxChars) {
    Utils::KeyboardRequest r;
    r.header = "OT";
    r.initialText = "RED";
    r.maxChars = maxChars;
    return r;
}
}  // namespace

int main() {
    {   // Over the character cap: refused, previous text kept, a reason shown.
        Board b;
        b.kb.open(request(7), 1280, 720);
        b.frame();
        b.system("ABCDEFGHIJ");
        check(!b.kb.finished && b.kb.text == "RED", "over-long System answer refused, text kept");
        check(!b.kb.refusalReason.empty() && b.kb.refusedFrames > 0, "refusal reason shown");
        b.system("BLUE");
        check(b.kb.finished && b.kb.accepted && b.kb.result().text == "BLUE", "valid System answer accepted");
    }
    {   // Malformed UTF-8 from the applet is never accepted.
        Board b;
        b.kb.open(request(7), 1280, 720);
        b.frame();
        b.system(std::string("A\xFF" "B"));
        check(!b.kb.finished && b.kb.text == "RED", "malformed System answer refused");
    }
    {   // A Japanese Gen I / II kana prompt: Latin from swkbd is not in the kana font.
        Board b;
        auto r = request(5);
        r.initialText = "";
        r.japaneseKana = true;
        b.kb.open(r, 1280, 720);
        b.frame();
        b.system("ABC");
        check(!b.kb.finished && b.kb.text.empty(), "Latin System answer refused on a kana prompt");
        b.system("\xE3\x82\xA2\xE3\x82\xA4");   // アイ
        check(b.kb.finished && b.kb.accepted, "kana System answer accepted on a kana prompt");
    }
    {   // A field rule (Gen III English OT): "A×B" refused by System and by OK; "BOB" accepted.
        const T::Gen3NameField field(T::FieldKind::OriginalTrainer, 2, 7, 7);
        Board b;
        auto r = request(7);
        r.field = &field;
        b.kb.open(r, 1280, 720);
        b.frame();
        b.system("A\xC3\x97" "B");
        check(!b.kb.finished && b.kb.text == "RED", "System answer the field cannot store is refused");
        check(b.kb.refusalReason == T::reason(T::Refusal::UnsupportedGlyph), "field refusal reason shown");
        b.kb.text = "A\xC3\x97" "B";   // as if typed on the Symbols page
        b.kb.caretOffset = b.kb.text.size();
        b.press(HidNpadButton_Plus);
        check(!b.kb.finished, "OK refuses text the field cannot store");
        b.kb.text = "BOB";
        b.kb.caretOffset = b.kb.text.size();
        b.press(HidNpadButton_Plus);
        check(b.kb.finished && b.kb.accepted && b.kb.result().text == "BOB", "OK accepts storable text");
    }
    {   // No field rule: OK behaves exactly as before.
        Board b;
        b.kb.open(request(7), 1280, 720);
        b.frame();
        b.press(HidNpadButton_Plus);
        check(b.kb.finished && b.kb.accepted && b.kb.result().text == "RED", "plain OK unchanged");
    }
    if (failures) {
        std::cerr << failures << " System keyboard validation failure(s)\n";
        return 1;
    }
    std::cout << "System keyboard answers pass the prompt's own rules before acceptance: PASS\n";
    return 0;
}
