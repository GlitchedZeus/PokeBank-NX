// Round 14 text safety (SHARED_UX_REQUIREMENTS.md P0-1 / P0-2 / P0-3): one table of strings x every
// writable name field of every Gen I-V format, driven through the REAL staged editors / workspaces
// the shared UI uses. For each cell the outcome must be exactly one of:
//   * ACCEPTED: export / strict reparse reads back the requested text (Gen III Latin width folded,
//     the same bytes drawn half- or full-width), or
//   * REFUSED: a message, and the staged bytes are unchanged.
// Never a different string: no prefix, no '?' substitute, no blank. Gen I / II are the
// international layouts; Japanese Gen I / II editing stays behind its gate (JapaneseEditingGate.h).
// Gen III Japanese / Egg records are covered in tests/test_gen3_staged_pokemon_editor.cpp
// (textSafetyGame). Synthetic saves only (tests/fuzz_synthetic_saves.h, gsc_pokemon_fixture.h).
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen1/Gen1StagedTrainerEditor.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen3/Gen3StagedTrainerEditor.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5NativeText.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5Trainer.h"
#include "Encryption/Encryption3FRLG.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Text/TextFieldCapability.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

int failures = 0;
void expect(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

struct Input { std::string label; std::string text; };

std::vector<Input> corpus(std::size_t maxChars) {
    std::vector<Input> rows = {
        {"ascii", "Red"},
        {"latin1", "\xC3\x89va"},                      // Éva
        {"times", "A\xC3\x97" "B"},                    // A×B
        {"ellipsis", "A\xE2\x80\xA6" "B"},             // A…B
        {"kana", "\xE3\x81\x82\xE3\x82\xA2"},          // あア
        {"hangul", "\xEA\xB0\x80\xEB\x82\x98"},        // 가나
        {"emoji", "A\xF0\x9F\x98\x80"},                // A😀 (non-BMP)
        {"malformed", "A\xFF" "B"},                    // invalid UTF-8
    };
    rows.push_back({"max", std::string("ABCDEFGHIJKL").substr(0, maxChars)});
    rows.push_back({"max+1", std::string("ABCDEFGHIJKLM").substr(0, maxChars + 1)});
    return rows;
}

std::u16string fold(const std::string& utf8) { return Utils::gen3FoldWidth(Utils::utf8ToUtf16(utf8)); }

// One cell of the matrix. `readBack` is empty when refused.
struct Cell {
    bool accepted = false;
    bool unchanged = true;   // staged bytes identical to before (checked only when refused)
    std::string error;
    std::string readBack;
};

std::map<std::string, std::map<std::string, char>> table;   // "format field" -> label -> A/R

void judge(const std::string& format, const std::string& field, const Input& in, const Cell& c,
           bool foldWidth = false) {
    const std::string where = format + " " + field + " [" + in.label + "]";
    table[format + " " + field][in.label] = c.accepted ? 'A' : 'R';
    if (c.accepted) {
        const bool same = foldWidth ? fold(c.readBack) == fold(in.text) : c.readBack == in.text;
        expect(same, where + ": accepted but read back as \"" + c.readBack + "\"");
    } else {
        expect(c.unchanged, where + ": refused but the staged bytes changed");
        expect(!c.error.empty(), where + ": refused without a message");
    }
    // Rules every format shares.
    if (in.label == "ascii" || in.label == "max") expect(c.accepted, where + ": plain ASCII refused: " + c.error);
    if (in.label == "max+1" || in.label == "emoji" || in.label == "malformed")
        expect(!c.accepted, where + ": must be refused");
}

// ------------------------------------------------------------------ Gen I (Red, international)
void gen1() {
    namespace G1 = PokeVault::Integration::Gen1;
    auto raw = qasynth::gen1::save();
    qasynth::gen1::forge(raw);
    auto parsed = G1::parse(raw, G1::SourceGame::Red);
    expect(static_cast<bool>(parsed), "Gen I fixture parses");
    if (!parsed) return;
    std::string error;
    std::vector<uint8_t> base;
    {
        auto editor = G1::StagedPokemonEditor::create(*parsed.save, error);
        G1::BoxPokemonCreate create;
        create.species = 25; create.level = 5; create.nickname = "PIKA"; create.otName = "RED";
        create.moves = {84, 0, 0, 0}; create.pp = {30, 0, 0, 0};
        const auto slot = editor ? editor->appendSlot(0, error) : std::nullopt;
        expect(editor && slot && editor->stageAdd(0, *slot, create, error), "Gen I fixture Pokemon added: " + error);
        if (!editor) return;
        base = editor->finalizedBytes(error);
        expect(!base.empty(), "Gen I fixture exports: " + error);
        if (base.empty()) return;
    }
    for (const bool nickname : {true, false}) {
        for (const auto& in : corpus(nickname ? 10 : 7)) {
            auto source = G1::parse(base, G1::SourceGame::Red);
            auto editor = G1::StagedPokemonEditor::create(*source.save, error);
            const std::vector<uint8_t> before(editor->stagedBytes().begin(), editor->stagedBytes().end());
            G1::BoxPokemonEdit edit;
            (nickname ? edit.nickname : edit.otName) = in.text;
            Cell c;
            error.clear();
            c.accepted = editor->stageEdit(0, 0, edit, error);
            c.error = error;
            c.unchanged = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()) == before;
            if (c.accepted) {
                const auto out = editor->finalizedBytes(error);
                auto re = G1::parse(out, G1::SourceGame::Red);
                auto reEditor = re ? G1::StagedPokemonEditor::create(*re.save, error) : nullptr;
                const auto record = reEditor ? reEditor->boxedPokemon(0, 0, error) : std::nullopt;
                expect(record.has_value(), "Gen I reparse");
                if (record) c.readBack = nickname ? record->nickname : record->originalTrainer;
            }
            judge("Gen I", nickname ? "nickname" : "OT", in, c);
        }
    }
    for (const auto& in : corpus(7)) {
        auto source = G1::parse(base, G1::SourceGame::Red);
        auto editor = G1::StagedTrainerEditor::create(*source.save, error);
        const std::vector<uint8_t> before(editor->stagedBytes().begin(), editor->stagedBytes().end());
        Cell c;
        error.clear();
        c.accepted = editor->stageTrainerName(in.text, error);
        c.error = error;
        c.unchanged = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()) == before;
        if (c.accepted) {
            auto re = G1::parse(editor->stagedBytes(), G1::SourceGame::Red);
            expect(static_cast<bool>(re), "Gen I trainer reparse");
            if (re) c.readBack = re.save->trainer().name;
        }
        judge("Gen I", "trainer", in, c);
    }
}

// ------------------------------------------------------------------ Gen II (Gold, international)
void gen2() {
    namespace G2 = PokeVault::Integration::Gen2;
    const auto base = fixture(GS);
    std::string error;
    for (const bool nickname : {true, false}) {
        for (const auto& in : corpus(nickname ? 10 : 7)) {
            auto source = G2::parse(base, G2::SourceGame::Gold);
            expect(static_cast<bool>(source), "Gen II fixture parses");
            if (!source) return;
            auto editor = G2::StagedEditor::create(*source.save, error);
            const std::vector<uint8_t> before(editor->stagedBytes().begin(), editor->stagedBytes().end());
            G2::BoxPokemonEdit edit;
            (nickname ? edit.nickname : edit.otName) = in.text;
            Cell c;
            error.clear();
            c.accepted = editor->stageBoxPokemonEdit(0, 0, edit, error);
            c.error = error;
            c.unchanged = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()) == before;
            if (c.accepted) {
                const auto out = editor->finalizedBytes(error);
                auto re = G2::parse(out, G2::SourceGame::Gold);
                auto reEditor = re ? G2::StagedEditor::create(*re.save, error) : nullptr;
                const auto record = reEditor ? reEditor->boxedPokemon(0, 0, error) : std::nullopt;
                expect(record.has_value(), "Gen II reparse");
                if (record) c.readBack = nickname ? record->nickname : record->originalTrainer;
            }
            judge("Gen II", nickname ? "nickname" : "OT", in, c);
        }
    }
    for (const auto& in : corpus(7)) {
        auto source = G2::parse(base, G2::SourceGame::Gold);
        auto editor = G2::StagedEditor::create(*source.save, error);
        const std::vector<uint8_t> before(editor->stagedBytes().begin(), editor->stagedBytes().end());
        Cell c;
        error.clear();
        c.accepted = editor->stageTrainerName(in.text, error);
        c.error = error;
        c.unchanged = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end()) == before;
        if (c.accepted) {
            const auto out = editor->finalizedBytes(error);
            auto re = G2::parse(out, G2::SourceGame::Gold);
            expect(static_cast<bool>(re), "Gen II trainer reparse");
            if (re) c.readBack = re.save->trainer().name;
        }
        judge("Gen II", "trainer", in, c);
    }
}

// ------------------------------------------------------------------ Gen III (FireRed, English)
std::vector<uint8_t> gen3Save() {
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
    p.setPID(0x12345678u); p.setTID16(12345); p.setSID16(54321); p.setSpecies(25); p.setLanguage(2);
    p.setOTName(u"RED"); p.setNickname(u"PIKACHU"); p.setFriendship(70); p.setOriginGame(4); p.setBall(4);
    p.setMetLevel(20); p.setMetLocation(1); p.setLevel(20); p.setMove(0, 33); p.setMovePP(0, 35);
    p.recalculateStats(); p.setStatHPCurrent(p.statHPMax());
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::memcpy(save.data() + body, encrypted, 100);
    delete[] encrypted;
    put16(save, sector + 0xFF6, D3::sectorChecksum(std::span<const uint8_t>(save).subspan(sector, D3::kChunkLengths[1])));
    return save;
}

void gen3() {
    namespace G3 = PokeVault::Integration::Gen3;
    const auto base = gen3Save();
    const auto game = G3::SourceGame::FireRedGBA;
    std::string error;
    for (const bool nickname : {true, false}) {
        for (const auto& in : corpus(nickname ? 10 : 7)) {
            auto editor = G3::StagedPokemonEditor::create(base, game, error);
            expect(editor != nullptr, "Gen III fixture opens: " + error);
            if (!editor) return;
            const auto before = editor->stagedBytes();
            G3::BoxPokemonEdit edit;
            (nickname ? edit.nickname : edit.otName) = in.text;
            Cell c;
            error.clear();
            c.accepted = editor->stagePartyPokemonEdit(0, edit, error);
            c.error = error;
            c.unchanged = editor->stagedBytes() == before;
            if (c.accepted) {
                const auto out = editor->finalizedBytes(error);
                auto re = G3::StagedPokemonEditor::create(out, game, error);
                const auto record = re ? re->partyPokemon(0, error) : std::nullopt;
                expect(record.has_value(), "Gen III reparse");
                if (record) c.readBack = nickname ? record->nickname : record->otName;
            }
            judge("Gen III", nickname ? "nickname" : "OT", in, c, true);
        }
    }
    for (const auto& in : corpus(7)) {
        auto editor = G3::Gen3StagedTrainerEditor::create(base, game, error);
        expect(editor != nullptr, "Gen III trainer editor opens: " + error);
        if (!editor) return;
        Cell c;
        error.clear();
        c.accepted = editor->stageTrainerName(in.text, error);
        c.error = error;
        auto applied = base;
        std::string applyError;
        const bool appliedOk = editor->applyTo(applied, applyError);
        c.unchanged = appliedOk && applied == base;
        if (c.accepted) {
            auto re = G3::StagedPokemonEditor::create(applied, game, error);
            expect(re != nullptr, "Gen III trainer reparse");
            if (re) c.readBack = re->trainer().name;
        }
        judge("Gen III", "trainer", in, c, true);
    }
}

// ------------------------------------------------------------------ Gen IV (Platinum, English)
void gen4() {
    namespace G4 = PokeVault::Integration::Gen4;
    auto raw = qasynth::gen4::save(qasynth::gen4::Kind::Platinum);
    qasynth::gen4::forge(raw);
    std::string error;
    for (const bool nickname : {true, false}) {
        for (const auto& in : corpus(nickname ? 10 : 7)) {
            auto editor = G4::Gen4StagedPokemonEditor::create(raw, G4::Layout::Platinum, "platinum_nds", &error);
            expect(editor.has_value(), "Gen IV fixture opens: " + error);
            if (!editor) return;
            const auto before = editor->stagedBytes();
            auto mon = editor->editablePartyPokemon(0, &error);
            expect(mon.has_value(), "Gen IV party Pokemon editable: " + error);
            if (!mon) return;
            Cell c;
            std::u16string wide;
            // The shared UI converts the keyboard's UTF-8 strictly; malformed text never reaches
            // the writer (TextFieldCapability.h strictUtf16).
            const bool wellFormed = PokeBank::Text::strictUtf16(in.text, wide);
            const bool set = wellFormed && (nickname ? mon->setNickname(wide) : mon->setOriginalTrainerName(wide));
            error.clear();
            c.accepted = set && editor->commitPartyPokemon(0, *mon, &error);
            c.error = c.accepted ? "" : (error.empty() ? "refused by the Gen IV name writer" : error);
            c.unchanged = editor->stagedBytes() == before;
            if (c.accepted) {
                auto re = G4::Gen4StagedPokemonEditor::create(editor->stagedBytes(), G4::Layout::Platinum, "platinum_nds", &error);
                const auto record = re ? re->partyPokemon(0, &error) : std::nullopt;
                expect(record.has_value(), "Gen IV reparse");
                if (record) c.readBack = Utils::utf16ToUtf8(nickname ? record->nickname() : record->originalTrainerName());
            }
            judge("Gen IV", nickname ? "nickname" : "OT", in, c);
        }
    }
    for (const auto& in : corpus(7)) {
        auto editor = G4::Gen4StagedPokemonEditor::create(raw, G4::Layout::Platinum, "platinum_nds", &error);
        if (!editor) return;
        expect(editor->trainerNameEditable(), "Gen IV English trainer name is editable");
        const auto before = editor->stagedBytes();
        Cell c;
        std::u16string wide;
        const bool wellFormed = PokeBank::Text::strictUtf16(in.text, wide);
        error.clear();
        c.accepted = wellFormed && editor->stageTrainerName(wide, &error);
        c.error = c.accepted ? "" : (error.empty() ? "malformed text refused" : error);
        c.unchanged = editor->stagedBytes() == before;
        if (c.accepted) {
            const auto trainer = editor->stagedTrainer(&error);
            expect(trainer.has_value(), "Gen IV trainer reparse");
            if (trainer) c.readBack = Utils::utf16ToUtf8(trainer->name);
        }
        judge("Gen IV", "trainer", in, c);
    }
}

// ------------------------------------------------------------------ Gen V (Black)
void gen5() {
    namespace G5 = PokeVault::Integration::Gen5;
    using Family = G5::SaveFamily;
    auto raw = qasynth::gen5::save(Family::BlackWhite, 21);
    qasynth::gen5::forge(raw);
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(raw, "black_nds", &error, G5::SaveCopySelection::Primary);
    expect(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return;
    for (const bool nickname : {true, false}) {
        for (const auto& in : corpus(nickname ? 10 : 7)) {
            G5::Gen5StagedPokemonWorkspace ws(*parsed);
            Cell c;
            error.clear();
            c.accepted = ws.stagePartyName(0, nickname, in.text, &error);
            c.error = error;
            c.unchanged = !ws.hasChanges();
            if (c.accepted) {
                const auto candidate = G5::buildStagedSaveCandidate(ws, &error);
                expect(candidate.has_value(), "Gen V candidate: " + error);
                if (candidate) {
                    const auto re = G5::Gen5ReadOnlySave::parse(candidate->bytes, "black_nds", &error, G5::SaveCopySelection::Primary);
                    const auto record = re ? re->partyPokemon(0) : std::nullopt;
                    expect(record.has_value(), "Gen V reparse");
                    if (record) {
                        // Round 15: Gen V names are no longer ASCII-only (Text::Gen5NameField); the
                        // reparsed name is read through the same rule the writer used.
                        const auto name = G5::editableName5(record->decodedBytes(), nickname, record->language());
                        expect(name.has_value(), "Gen V reparsed name reads back through the Gen V rule");
                        if (name) c.readBack = *name;
                    }
                }
            }
            judge("Gen V", nickname ? "nickname" : "OT", in, c);
        }
    }
    for (const auto& in : corpus(7)) {
        auto trainer = G5::Gen5StagedTrainer::create(*parsed, &error);
        expect(trainer.has_value(), "Gen V trainer opens: " + error);
        if (!trainer) return;
        Cell c;
        error.clear();
        c.accepted = trainer->setName(in.text, &error);
        c.error = error;
        c.unchanged = !trainer->dirty();
        // As the game shows it: the half-width gender signs read as ♂ / ♀ (Round 15 Gen V names).
        if (c.accepted) c.readBack = G5::trainerNameText5(trainer->current().rawName);
        judge("Gen V", "trainer", in, c);
    }
}

}  // namespace

int main() {
    gen1();
    gen2();
    gen3();
    gen4();
    gen5();
    // The matrix, for the report: A = stored exactly (reparse equal), R = refused, nothing staged.
    const char* labels[] = {"ascii", "latin1", "times", "ellipsis", "kana", "hangul", "emoji",
                            "malformed", "max", "max+1"};
    std::cout << std::left << std::setw(20) << "format field";
    for (const char* l : labels) std::cout << std::setw(10) << l;
    std::cout << '\n';
    for (const auto& [row, cells] : table) {
        std::cout << std::setw(20) << row;
        for (const char* l : labels) {
            const auto it = cells.find(l);
            std::cout << std::setw(10) << (it == cells.end() ? '-' : it->second);
        }
        std::cout << '\n';
    }
    if (failures) {
        std::cerr << failures << " text-safety matrix failure(s)\n";
        return 1;
    }
    std::cout << "Gen I-V text safety matrix: every cell stored exactly or refused with nothing staged PASS\n";
    return 0;
}
