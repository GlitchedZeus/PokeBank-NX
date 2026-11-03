// Round 15 keyboard lane (owner: "audit EVERY text-entry surface across Gen I-IX. Implement shared
// Japanese/Korean/Latin input with native encoding validation. No ASCII-only keyboard when the
// underlying game supports more. Never truncate, corrupt or silently replace characters.").
//
// One matrix: every Gen I-V save-native name field (Text/NameFields.h makeNameField) x the
// languages its records / saves come in, through the REAL shared keyboard
// (UI::Dialogs::KeyboardState opened with the field, exactly as Utils::promptName does):
//   * the pages offered (start page and the ZL / ZR cycle) are the record's language's script(s);
//   * EVERY key that is live on EVERY offered page types a character the field stores, and every
//     disabled key one it does not (glyphsAllowed is the writer's own table);
//   * the cap is the game's (PKHeX PK1-PK5 / Legal.GetMaxLength*);
//   * live validation while typing: a disabled key types nothing and says why, the cap holds, a
//     seeded unstorable name shows its reason before OK and OK refuses it, Hangul composition is
//     re-checked after every jamo;
//   * round trips through the real writers: store -> export -> strict reparse -> equal, for Gen IV
//     Japanese / Korean records, Gen V Japanese / Korean / accented records and trainer name, and
//     Gen III Japanese records (kana + full-width Latin). Never a prefix, never a substitute.
#include "fuzz_synthetic_saves.h"
#include "HostUi.h"

#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5NativeText.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5Trainer.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Text/NameFields.h"
#include "UI/Dialogs/KeyboardDialog.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace {

int failures = 0;
void expect(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

namespace D = UI::Dialogs;
namespace T = PokeBank::Text;
using Page = D::KeyboardPage;

const char* pageName(Page p) {
    switch (p) {
        case Page::Letters: return "ABC";
        case Page::Symbols: return "?123";
        case Page::Accents: return "accents";
        case Page::Digits: return "digits";
        case Page::HiraganaA: return "hiragana";
        case Page::HiraganaB: return "hiragana2";
        case Page::KatakanaA: return "katakana";
        case Page::KatakanaB: return "katakana2";
        case Page::KanaSymbols: return "kana123";
        case Page::Hangul: return "hangul";
    }
    return "?";
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
    void open(const T::TextFieldCapability& field, const std::string& initial = {}) {
        Utils::KeyboardRequest r;
        r.header = "Name";
        r.initialText = initial;
        r.maxChars = field.maxCharacters();
        r.field = &field;
        kb.open(r, 1280, 720);
        frame();
    }
    bool typeKey(const std::string& label) {
        for (int i = 0; i < static_cast<int>(kb.keys.size()); ++i) {
            if (kb.keys[i].action != D::KeyboardKeyAction::Type || kb.keys[i].lowerLabel != label) continue;
            kb.selectedKeyIndex = i;
            press(HidNpadButton_A);
            return true;
        }
        return false;
    }
    bool hasKey(const std::string& label, bool* enabled = nullptr) const {
        for (const auto& k : kb.keys)
            if (k.action == D::KeyboardKeyAction::Type && k.lowerLabel == label) {
                if (enabled) *enabled = k.lowerEnabled;
                return true;
            }
        return false;
    }
    void goTo(Page p) {
        for (int i = 0; i < 12 && kb.page != p; ++i) press(HidNpadButton_ZR);
    }
};

// ---- A. every field x language: pages, live keys, cap ------------------------------------
struct Row {
    std::string name;
    T::FieldContext context;
    std::size_t maxChars;
    Page start;
    std::vector<Page> pages;   // the ZL / ZR cycle, in order
};

const std::vector<Page> kKana = {Page::HiraganaA, Page::HiraganaB, Page::KatakanaA, Page::KatakanaB,
                                 Page::KanaSymbols};
std::vector<Page> join(std::vector<Page> a, const std::vector<Page>& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

std::vector<Row> rows() {
    using F = T::Format;
    using K = T::FieldKind;
    const std::vector<Page> latin = {Page::Letters, Page::Symbols, Page::Accents};
    const std::vector<Page> japanese = join(kKana, {Page::Letters, Page::Symbols});
    const std::vector<Page> korean = {Page::Hangul, Page::Letters, Page::Symbols, Page::Accents};
    std::vector<Row> out;
    for (const F f : {F::Gen1, F::Gen2}) {
        const std::string g = f == F::Gen1 ? "Gen I" : "Gen II";
        out.push_back({g + " intl nickname", {f, K::Nickname, 2, false}, 10, Page::Letters, latin});
        out.push_back({g + " intl OT", {f, K::OriginalTrainer, 2, false}, 7, Page::Letters, latin});
        out.push_back({g + " intl trainer", {f, K::TrainerName, 2, false}, 7, Page::Letters, latin});
        // Japanese Gen I / II: kana only (the font has no Latin); editing stays behind the gate.
        out.push_back({g + " JP nickname", {f, K::Nickname, 1, true}, 5, Page::HiraganaA, kKana});
        out.push_back({g + " JP OT", {f, K::OriginalTrainer, 1, true}, 5, Page::HiraganaA, kKana});
        out.push_back({g + " JP trainer", {f, K::TrainerName, 1, true}, 5, Page::HiraganaA, kKana});
    }
    for (const uint8_t lang : {uint8_t{2}, uint8_t{3}, uint8_t{5}}) {
        const std::string l = "Gen III lang" + std::to_string(lang);
        out.push_back({l + " nickname", {F::Gen3, K::Nickname, lang, false}, 10, Page::Letters, latin});
        out.push_back({l + " OT", {F::Gen3, K::OriginalTrainer, lang, false}, 7, Page::Letters, latin});
    }
    out.push_back({"Gen III intl trainer", {F::Gen3, K::TrainerName, 2, false}, 7, Page::Letters, latin});
    out.push_back({"Gen III JP nickname", {F::Gen3, K::Nickname, 1, false}, 5, Page::HiraganaA, japanese});
    out.push_back({"Gen III JP OT", {F::Gen3, K::OriginalTrainer, 1, false}, 5, Page::HiraganaA, japanese});
    for (const F f : {F::Gen4, F::Gen5}) {
        const std::string g = f == F::Gen4 ? "Gen IV" : "Gen V";
        out.push_back({g + " EN nickname", {f, K::Nickname, 2, false}, 10, Page::Letters, latin});
        out.push_back({g + " EN OT", {f, K::OriginalTrainer, 2, false}, 7, Page::Letters, latin});
        out.push_back({g + " FR nickname", {f, K::Nickname, 3, false}, 10, Page::Letters, latin});
        out.push_back({g + " JP nickname", {f, K::Nickname, 1, false}, 5, Page::HiraganaA, japanese});
        out.push_back({g + " JP OT", {f, K::OriginalTrainer, 1, false}, 5, Page::HiraganaA, japanese});
        out.push_back({g + " KO nickname", {f, K::Nickname, 8, false}, 5, Page::Hangul, korean});
        out.push_back({g + " KO OT", {f, K::OriginalTrainer, 8, false}, 5, Page::Hangul, korean});
        out.push_back({g + " EN trainer", {f, K::TrainerName, 2, false}, 7, Page::Letters, latin});
    }
    out.push_back({"Gen V JP trainer", {F::Gen5, K::TrainerName, 1, false}, 5, Page::HiraganaA, japanese});
    out.push_back({"Gen V KO trainer", {F::Gen5, K::TrainerName, 8, false}, 5, Page::Hangul, korean});
    return out;
}

void pagesAndKeys() {
    std::cout << std::left << std::setw(26) << "field" << std::setw(5) << "cap" << "pages (live keys / disabled keys)\n";
    for (const auto& row : rows()) {
        const auto field = T::makeNameField(row.context);
        expect(field->maxCharacters() == row.maxChars,
               row.name + ": cap " + std::to_string(field->maxCharacters()) + " != " + std::to_string(row.maxChars));
        Board b;
        b.open(*field);
        expect(b.kb.page == row.start, row.name + ": opens on " + pageName(b.kb.page) + ", not " + pageName(row.start));
        const auto cycle = b.kb.pageCycle();
        expect(cycle == row.pages, row.name + ": page cycle differs");
        expect(b.kb.request.maxChars == static_cast<int>(row.maxChars), row.name + ": keyboard cap is the field's");
        std::cout << std::setw(26) << row.name << std::setw(5) << row.maxChars;
        for (const Page p : cycle) {
            b.goTo(p);
            expect(b.kb.page == p, row.name + ": ZR reaches " + pageName(p));
            int live = 0, off = 0;
            for (const auto& key : b.kb.keys) {
                if (key.action != D::KeyboardKeyAction::Type) continue;
                // THE invariant: a key is live exactly when the field stores what it types.
                if (p != Page::Hangul) {
                    expect(key.lowerEnabled == field->glyphsAllowed(key.lowerLabel),
                           row.name + " " + pageName(p) + ": key " + key.lowerLabel + " liveness != field");
                    expect(key.upperEnabled == field->glyphsAllowed(key.upperLabel),
                           row.name + " " + pageName(p) + ": shifted key " + key.upperLabel + " liveness != field");
                }
                key.lowerEnabled ? ++live : ++off;
            }
            std::cout << pageName(p) << "(" << live << "/" << off << ") ";
        }
        std::cout << '\n';
        // No page outside the language's script: never Hangul for a non-Korean record, never kana
        // pages for an international Gen I / II / III record.
        const bool korean = row.context.language == 8;
        for (const Page p : cycle) {
            if (p == Page::Hangul) expect(korean, row.name + ": Hangul offered to a non-Korean record");
        }
    }
}

// Spot checks of what the matrix above means for the player.
void spotChecks() {
    using F = T::Format;
    using K = T::FieldKind;
    {
        // Gen III Japanese: ABC is full width and live (the cartridge stores Ａ-Ｚ); hiragana live.
        const auto field = T::makeNameField({F::Gen3, K::Nickname, 1, false});
        Board b;
        b.open(*field);
        bool on = false;
        expect(b.hasKey("あ", &on) && on, "Gen III JP: あ live");
        b.goTo(Page::Letters);
        expect(b.hasKey("ｑ", &on) && on, "Gen III JP: ABC page types full-width ｑ, live");
        expect(!b.hasKey("q"), "Gen III JP: no half-width key on the ABC page");
    }
    {
        // Gen III English: × is not in GEN3_EN, so its key is disabled and types nothing.
        const auto field = T::makeNameField({F::Gen3, K::OriginalTrainer, 2, false});
        Board b;
        b.open(*field);
        b.goTo(Page::Symbols);
        bool on = true;
        expect(b.hasKey("×", &on) && !on, "Gen III EN: × disabled");
        b.typeKey("1");
        b.typeKey("×");
        expect(b.kb.text == "1", "Gen III EN: the disabled × typed nothing (text " + b.kb.text + ")");
        expect(b.kb.refusalReason.find("×") != std::string::npos, "Gen III EN: the refusal names ×");
    }
    {
        // Gen I international: the encoder has no ♂, Gen II's does.
        bool on = true;
        const auto g1 = T::makeNameField({F::Gen1, K::Nickname, 2, false});
        Board b1;
        b1.open(*g1);
        b1.goTo(Page::Symbols);
        expect(b1.hasKey("♂", &on) && !on, "Gen I intl: ♂ disabled");
        const auto g2 = T::makeNameField({F::Gen2, K::Nickname, 2, false});
        Board b2;
        b2.open(*g2);
        b2.goTo(Page::Symbols);
        expect(b2.hasKey("♂", &on) && on, "Gen II intl: ♂ live");
    }
    {
        // Gen V English: accents are live now (no longer ASCII-only).
        const auto field = T::makeNameField({F::Gen5, K::Nickname, 2, false});
        Board b;
        b.open(*field);
        b.goTo(Page::Accents);
        bool on = false;
        expect(b.hasKey("é", &on) && on, "Gen V EN: é live");
        b.typeKey("é");
        expect(b.kb.text == "é" && b.kb.fieldOk, "Gen V EN: typed é, live verdict OK");
    }
}

// ---- B. live validation ---------------------------------------------------------------------
void liveValidation() {
    using F = T::Format;
    using K = T::FieldKind;
    {
        // A seeded name the field cannot store shows its reason before OK, and OK refuses it.
        const auto field = T::makeNameField({F::Gen3, K::OriginalTrainer, 2, false});
        Board b;
        b.open(*field, "A×B");
        expect(!b.kb.fieldOk && b.kb.fieldReason.find("×") != std::string::npos,
               "seeded A×B: the live verdict names × before OK (" + b.kb.fieldReason + ")");
        b.press(HidNpadButton_Plus);
        expect(!b.kb.finished, "seeded A×B: OK refused");
        b.press(HidNpadButton_B);   // deletes B
        b.press(HidNpadButton_B);   // deletes ×
        expect(b.kb.text == "A" && b.kb.fieldOk, "after deleting ×, the verdict clears (text " + b.kb.text + ")");
        b.press(HidNpadButton_Plus);
        expect(b.kb.finished && b.kb.accepted && b.kb.result().text == "A", "OK accepts the storable name");
    }
    {
        // The cap in the game's characters: a Japanese Gen IV nickname holds 5 kana.
        const auto field = T::makeNameField({F::Gen4, K::Nickname, 1, false});
        Board b;
        b.open(*field);
        for (const char* k : {"あ", "い", "う", "え", "お", "か"}) b.typeKey(k);
        expect(b.kb.text == "あいうえお", "Gen IV JP: the 6th kana is refused at the cap (text " + b.kb.text + ")");
        expect(b.kb.refusedFrames > 0, "Gen IV JP: the refusal shows");
        b.press(HidNpadButton_Plus);
        expect(b.kb.finished && b.kb.result().text == "あいうえお", "Gen IV JP: OK accepts 5 kana");
    }
    {
        // Gen I / II Japanese (gate closed for editing, keyboard ready): Space types the full-width
        // space, no Latin page exists.
        const auto field = T::makeNameField({F::Gen1, K::Nickname, 1, true});
        Board b;
        b.open(*field);
        b.typeKey("ひ");
        b.press(HidNpadButton_Y);
        expect(b.kb.text == "ひ　" && b.kb.fieldOk, "Gen I JP: ひ + full-width space (text " + b.kb.text + ")");
    }
    {
        // Hangul: composition re-checked after every jamo against the Gen 4 code page.
        const auto field = T::makeNameField({F::Gen4, K::Nickname, 8, false});
        Board b;
        b.open(*field);
        b.typeKey("ㅍ");
        b.typeKey("ㅣ");
        expect(b.kb.text == "피" && b.kb.fieldOk, "Gen IV KO: 피 storable");
        b.typeKey("ㅋ");
        const bool stored = field->glyphAllowed(U'핔');
        expect(b.kb.text == "핔" && b.kb.fieldOk == stored, "Gen IV KO: the live verdict follows the composed 핔");
        for (const char* k : {"ㅏ", "ㅊ", "ㅠ"}) b.typeKey(k);
        expect(b.kb.text == "피카츄" && b.kb.fieldOk, "Gen IV KO: 피카츄 storable (text " + b.kb.text + ")");
        b.press(HidNpadButton_Plus);
        expect(b.kb.finished && b.kb.result().text == "피카츄", "Gen IV KO: OK returns 피카츄");
    }
}

// ---- C. round trips through the real writers ------------------------------------------------
std::vector<uint8_t> gen3Save(uint8_t recordLanguage) {
    namespace D3 = PokeVault::Integration::Gen3::Detail;
    namespace G3 = PokeVault::Integration::Gen3;
    using Sectors = std::array<std::array<uint8_t, D3::kSectorSize>, D3::kSectorCount>;
    auto put16 = [](std::span<uint8_t> b, std::size_t o, uint16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(v >> 8); };
    auto put32 = [&](std::span<uint8_t> b, std::size_t o, uint32_t v) { put16(b, o, uint16_t(v)); put16(b, o + 2, uint16_t(v >> 16)); };
    Sectors logical{};
    constexpr uint32_t key = 0xA1B2C3D4;
    put32(logical[0], 0xAC, 1);
    put32(logical[0], 0xF20, key);
    put32(logical[1], 0x0290, key);
    put16(logical[0], 0x0A, 12345);
    put16(logical[0], 0x0C, 54321);
    // An international trainer name ("RED" + 0xFF padding): bytes 6-7 non-zero = international save.
    logical[0][0] = Utils::charToGen3(u'R', 2);
    logical[0][1] = Utils::charToGen3(u'E', 2);
    logical[0][2] = Utils::charToGen3(u'D', 2);
    for (std::size_t i = 3; i < 8; ++i) logical[0][i] = Utils::GEN3_TERMINATOR;
    std::vector<uint8_t> save(D3::kSaveSize, 0);
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const uint8_t rotation = slot == 0 ? 0 : 5;
        const uint32_t counter = slot == 0 ? 7 : 9;
        for (std::size_t physical = 0; physical < D3::kSectorCount; ++physical) {
            const uint16_t id = static_cast<uint16_t>((physical + rotation) % D3::kSectorCount);
            const std::size_t offset = D3::kSlotBases[slot] + physical * D3::kSectorSize;
            std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
            put16(save, offset + 0xFF4, id);
            put16(save, offset + 0xFF6, D3::sectorChecksum(std::span<const uint8_t>(save).subspan(offset, D3::kChunkLengths[id])));
            put32(save, offset + 0xFF8, D3::kSectorSignature);
            put32(save, offset + 0xFFC, counter);
        }
    }
    const auto game = G3::SourceGame::FireRedGBA;
    const auto active = D3::validateSlot(save, 1);
    const auto sector = active.logicalSectorOffsets[1];
    const auto body = sector + D3::partyDataOffset(game);
    save[sector + D3::partyCountOffset(game)] = 1;
    std::array<uint8_t, 100> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(0x12345678u); p.setTID16(12345); p.setSID16(54321); p.setSpecies(25); p.setLanguage(recordLanguage);
    p.setOTName(recordLanguage == 1 ? u"レッド" : u"RED");
    p.setNickname(recordLanguage == 1 ? u"ピカチュウ" : u"PIKACHU"); p.setFriendship(70); p.setOriginGame(4); p.setBall(4);
    p.setMetLevel(20); p.setMetLocation(1); p.setLevel(20); p.setMove(0, 33); p.setMovePP(0, 35);
    p.recalculateStats(); p.setStatHPCurrent(p.statHPMax());
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::memcpy(save.data() + body, encrypted, 100);
    delete[] encrypted;
    put16(save, sector + 0xFF6, D3::sectorChecksum(std::span<const uint8_t>(save).subspan(sector, D3::kChunkLengths[1])));
    return save;
}

void gen3RoundTrip() {
    namespace G3 = PokeVault::Integration::Gen3;
    const auto game = G3::SourceGame::FireRedGBA;
    struct Case { uint8_t language; bool nickname; std::string text; bool stored; };
    const std::vector<Case> cases = {
        {1, true, "ピカチュウ", true},       // kana
        {1, true, "ＰＩＫＡ", true},         // full-width Latin, as a Japanese cartridge stores it
        {1, true, "ピカ１２", true},         // full-width digits
        {1, false, "サトシ", true},
        {1, true, "ピカチュウだ", false},    // 6 > 5: refused, never cut
        {1, true, "피카", false},            // Hangul: no Gen III table has it
        {2, true, "Pikachu", true},
        {2, true, "PIKA×", false},           // × is not in GEN3_EN
    };
    for (const auto& c : cases) {
        const auto base = gen3Save(c.language);
        std::string error;
        auto editor = G3::StagedPokemonEditor::create(base, game, error);
        expect(editor != nullptr, "Gen III fixture opens: " + error);
        if (!editor) return;
        // The keyboard's own verdict for this record's field.
        const auto field = T::makeNameField({T::Format::Gen3,
            c.nickname ? T::FieldKind::Nickname : T::FieldKind::OriginalTrainer, c.language, false});
        expect(field->check(c.text).ok() == c.stored, "Gen III lang" + std::to_string(c.language) + " " + c.text +
               ": keyboard verdict");
        const auto before = editor->stagedBytes();
        G3::BoxPokemonEdit edit;
        (c.nickname ? edit.nickname : edit.otName) = c.text;
        const bool ok = editor->stagePartyPokemonEdit(0, edit, error);
        const std::string where = "Gen III lang" + std::to_string(c.language) + (c.nickname ? " nickname " : " OT ") + c.text;
        expect(ok == c.stored, where + ": writer verdict agrees with the keyboard (" + error + ")");
        if (!ok) {
            expect(editor->stagedBytes() == before, where + ": refused, nothing staged");
            continue;
        }
        const auto out = editor->finalizedBytes(error);
        auto re = G3::StagedPokemonEditor::create(out, game, error);
        const auto record = re ? re->partyPokemon(0, error) : std::nullopt;
        expect(record.has_value(), where + ": strict reparse");
        if (record) {
            const std::string back = c.nickname ? record->nickname : record->otName;
            expect(Utils::gen3FoldWidth(Utils::utf8ToUtf16(back)) == Utils::gen3FoldWidth(Utils::utf8ToUtf16(c.text)),
                   where + ": reparsed as " + back);
            expect(record->language == c.language, where + ": record language kept");
        }
    }
}

void gen4RoundTrip() {
    namespace G4 = PokeVault::Integration::Gen4;
    auto raw = qasynth::gen4::save(qasynth::gen4::Kind::Platinum);
    qasynth::gen4::forge(raw);
    struct Case { uint8_t language; bool nickname; std::string text; bool stored; };
    const std::vector<Case> cases = {
        {1, true, "ピカチュウ", true},
        {1, true, "ＰＩＫＡ", true},
        {1, false, "サトシ", true},
        {1, true, "ピカチュウだ", false},    // Japanese nickname: 5
        {8, true, "피카츄", true},
        {8, false, "지우", true},
        {8, true, "피카츄피카츄", false},    // Korean nickname: 5
        {2, true, "Pikachu", true},
        {2, true, "Nidoran♂", true},
    };
    for (const auto& c : cases) {
        std::string error;
        auto editor = G4::Gen4StagedPokemonEditor::create(raw, G4::Layout::Platinum, "platinum_nds", &error);
        expect(editor.has_value(), "Gen IV fixture opens: " + error);
        if (!editor) return;
        auto mon = editor->editablePartyPokemon(0, &error);
        expect(mon.has_value() && mon->setLanguage(c.language), "Gen IV language set");
        if (!mon) return;
        const std::string where = "Gen IV lang" + std::to_string(c.language) + (c.nickname ? " nickname " : " OT ") + c.text;
        // The keyboard the UI opens for this record (pages from its language, cap 5 for JP / KO).
        const auto field = T::makeNameField({T::Format::Gen4,
            c.nickname ? T::FieldKind::Nickname : T::FieldKind::OriginalTrainer, mon->language(), false});
        const bool keyboardOk = field->check(c.text).ok();
        expect(keyboardOk == c.stored, where + ": keyboard verdict");
        if (!keyboardOk) continue;   // the keyboard never hands this text to the writer
        const auto wide = Utils::utf8ToUtf16(c.text);
        const bool set = c.nickname ? mon->setNickname(wide) : mon->setOriginalTrainerName(wide);
        expect(set && editor->commitPartyPokemon(0, *mon, &error), where + ": writer stores it: " + error);
        const auto out = editor->finalizedBytes(&error);
        expect(!out.empty(), where + ": export: " + error);
        auto re = G4::Gen4StagedPokemonEditor::create(out, G4::Layout::Platinum, "platinum_nds", &error);
        const auto record = re ? re->partyPokemon(0, &error) : std::nullopt;
        expect(record.has_value(), where + ": strict reparse: " + error);
        if (record) {
            const auto back = Utils::utf16ToUtf8(c.nickname ? record->nickname() : record->originalTrainerName());
            expect(back == c.text, where + ": reparsed as " + back);
            expect(record->language() == c.language, where + ": language kept");
        }
    }
}

void gen5RoundTrip() {
    namespace G5 = PokeVault::Integration::Gen5;
    using Field = G5::StagedPokemon5Record::Field;
    auto raw = qasynth::gen5::save(G5::SaveFamily::BlackWhite, 21);
    qasynth::gen5::forge(raw);
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(raw, "black_nds", &error, G5::SaveCopySelection::Primary);
    expect(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return;
    struct Case { uint8_t language; bool nickname; std::string text; bool stored; };
    const std::vector<Case> cases = {
        {1, true, "ピカチュウ", true},
        {1, true, "ＰＩＫＡ", true},
        {1, false, "トウヤ", true},
        {8, true, "피카츄", true},
        {8, false, "지우", true},
        {2, true, "Pikaché", true},          // no longer ASCII-only
        {2, true, "Nidoran♂", true},         // ♂ stored half width (U+246D), read back as ♂
        {2, false, "Chéren", true},
        {2, true, "Pika\U0001F600", false},  // no font has it
        {2, true, "Pika漢", false},      // kanji: not in the Gen 4/5 name repertoire
    };
    for (const auto& c : cases) {
        G5::Gen5StagedPokemonWorkspace ws(*parsed);
        const std::string where = "Gen V lang" + std::to_string(c.language) + (c.nickname ? " nickname " : " OT ") + c.text;
        expect(ws.stageParty(0, Field::Language, 0, c.language, &error), where + ": language staged: " + error);
        const auto field = T::makeNameField({T::Format::Gen5,
            c.nickname ? T::FieldKind::Nickname : T::FieldKind::OriginalTrainer, c.language, false});
        expect(field->check(c.text).ok() == c.stored, where + ": keyboard verdict");
        error.clear();
        const bool ok = ws.stagePartyName(0, c.nickname, c.text, &error);
        expect(ok == c.stored, where + ": writer verdict agrees with the keyboard (" + error + ")");
        if (!ok) continue;
        const auto candidate = G5::buildStagedSaveCandidate(ws, &error);
        expect(candidate.has_value(), where + ": export: " + error);
        if (!candidate) continue;
        const auto re = G5::Gen5ReadOnlySave::parse(candidate->bytes, "black_nds", &error, G5::SaveCopySelection::Primary);
        const auto record = re ? re->partyPokemon(0) : std::nullopt;
        expect(record.has_value(), where + ": strict reparse");
        if (!record) continue;
        const auto back = G5::editableName5(record->decodedBytes(), c.nickname, record->language());
        expect(back && *back == c.text, where + ": reparsed as " + back.value_or("(none)"));
        if (c.text == "Nidoran♂") {
            const auto bytes = record->decodedBytes();
            expect(bytes[0x48 + 14] == 0x6D && bytes[0x48 + 15] == 0x24, "Gen V: ♂ stored as the half-width U+246D");
        }
    }
    // Trainer name: same rule, the save's language.
    for (const std::string name : {"Chéren", "Hilda♀", "トウヤ"}) {
        auto trainer = G5::Gen5StagedTrainer::create(*parsed, &error);
        expect(trainer.has_value(), "Gen V trainer opens: " + error);
        if (!trainer) return;
        expect(trainer->setName(name, &error), "Gen V trainer name " + name + " stored: " + error);
        expect(G5::trainerNameText5(trainer->current().rawName) == name, "Gen V trainer name " + name + " reads back");
        expect(G5::editableTrainerName5(trainer->current().rawName, trainer->current().language) == name,
               "Gen V trainer name " + name + " re-seeds the prompt unchanged");
    }
    auto trainer = G5::Gen5StagedTrainer::create(*parsed, &error);
    if (trainer) {
        expect(!trainer->setName("Pika\U0001F600", &error) && !trainer->dirty(), "Gen V trainer: emoji refused, nothing staged");
        expect(!trainer->setName("TooLongName", &error) && !trainer->dirty(), "Gen V trainer: 11 characters refused, never cut");
    }
}

}  // namespace

int main() {
    pagesAndKeys();
    spotChecks();
    liveValidation();
    gen3RoundTrip();
    gen4RoundTrip();
    gen5RoundTrip();
    if (failures) {
        std::cerr << failures << " text-field capability matrix failure(s)\n";
        return 1;
    }
    std::cout << "Gen I-V shared keyboard x field capability matrix (pages, live keys, live validation, "
                 "store -> export -> strict reparse) PASS\n";
    return 0;
}
