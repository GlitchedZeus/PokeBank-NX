// Divergence item 4: Gen I Edit is a working-copy draft (Gen III-V parity).
//  Data layer, driving the real Gen I staged editor exactly as the UI does:
//   - the draft is a second StagedPokemonEditor made from the real staged view;
//   - every edit (species, level, EXP, moves/PP/PP Ups, DVs, Stat Exp, nickname, OT, TID, the
//     Random DVs result) lands in the draft only - the real staged copy is byte-identical;
//   - Keep = one stageEdit(editFromRecord(draft record)) reproduces the drafted Pokemon exactly;
//   - Discard = drop the draft: the real staged copy never changed.
//  UI routes: editor() redirects to the draft only for its own screen/editor, Edit fails closed
//  without a draft, Keep commits through the verified path, Discard never restores/stages.
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "UI/Gen1EditDraft.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen1;

namespace {
uint8_t sum(const std::vector<uint8_t>& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += v[i];
    return static_cast<uint8_t>((~s) & 255);
}
size_t bank(size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; }
std::vector<uint8_t> fixture(SourceGame game) {
    std::vector<uint8_t> v(0x8000, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50;
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 0x82;
    v[0x29C3] = game == SourceGame::Yellow ? 0x54 : 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    const size_t box = 0x30C0, body = box + 22;
    v[box] = 1; v[box + 1] = 0x54; v[box + 2] = 0xFF;
    v[body] = 0x54; v[body + 1] = 0; v[body + 2] = 20; v[body + 3] = 5;
    v[body + 5] = v[body + 6] = 23; v[body + 7] = 190; v[body + 8] = 33;
    v[body + 12] = 0x12; v[body + 13] = 0x34; v[body + 16] = 125;
    v[body + 27] = 0x88; v[body + 28] = 0x88; v[body + 29] = 35;
    std::fill_n(v.begin() + box + 682, 11, 0x50); v[box + 682] = 0x91; v[box + 683] = 0x84; v[box + 684] = 0x83;
    std::fill_n(v.begin() + box + 902, 11, 0x50);
    v[box + 902] = 0x8F; v[box + 903] = 0x88; v[box + 904] = 0x8A; v[box + 905] = 0x80;
    std::copy_n(v.begin() + box, 0x462, v.begin() + bank(2));
    v[0x3523] = sum(v, 0x2598, 0xF8B);
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        v[b + 0x1A4C] = sum(v, b, 0x1A4C);
        for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = sum(v, b + i * 0x462, 0x462);
    }
    return v;
}
std::vector<uint8_t> bytes(const StagedPokemonEditor& e) { return {e.stagedBytes().begin(), e.stagedBytes().end()}; }
bool sameRecord(const PokemonRecord& a, const PokemonRecord& b) {
    return a.species == b.species && a.nickname == b.nickname && a.level == b.level &&
           a.experience == b.experience && a.trainerId == b.trainerId && a.moves == b.moves &&
           a.pp == b.pp && a.ppUps == b.ppUps && a.dvs == b.dvs && a.statExperience == b.statExperience &&
           a.originalTrainer == b.originalTrainer && a.currentHP == b.currentHP;
}
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string between(const std::string& t, const char* a, const char* b) {
    const auto i = t.find(a);
    if (i == std::string::npos) std::cerr << "Missing route start: " << a << '\n';
    assert(i != std::string::npos);
    const auto j = t.find(b, i + 1);
    assert(j != std::string::npos);
    return t.substr(i, j - i);
}
void has(const std::string& t, const char* n) {
    if (t.find(n) == std::string::npos) std::cerr << "Missing: " << n << '\n';
    assert(t.find(n) != std::string::npos);
}
void lacks(const std::string& t, const char* n) {
    if (t.find(n) != std::string::npos) std::cerr << "Forbidden: " << n << '\n';
    assert(t.find(n) == std::string::npos);
}

void draftSession(SourceGame game) {
    const auto raw = fixture(game);
    const auto source = parse(raw, game);
    assert(source);
    std::string error;
    auto real = StagedPokemonEditor::create(*source.save, error);
    assert(real);
    // An earlier staged change exists before the session (the draft must start from it).
    BoxPokemonEdit earlier; earlier.trainerId = 4242;
    assert(real->stageEdit(2, 0, earlier, error));
    const auto stagedBefore = bytes(*real);

    for (const bool keep : {false, true}) {
        auto draft = StagedPokemonEditor::create(real->view(), error);
        assert(draft && bytes(*draft) == stagedBefore);
        BoxPokemonEdit e;
        e.species = 26; assert(draft->stageEdit(2, 0, e, error));
        e = {}; e.level = 40; assert(draft->stageEdit(2, 0, e, error));
        e = {}; e.moves = std::array<uint8_t, 4>{85, 87, 98, 0};
        assert(draft->stageEdit(2, 0, e, error));
        e = {}; e.ppUps = std::array<uint8_t, 4>{3, 1, 0, 0};
        e.pp = std::array<uint8_t, 4>{
            StagedPokemonEditor::moveMaxPP(85, 3), StagedPokemonEditor::moveMaxPP(87, 1),
            StagedPokemonEditor::moveBasePP(98), 0};
        assert(draft->stageEdit(2, 0, e, error));
        e = {}; e.dvs = std::array<uint8_t, 4>{15, 10, 10, 10}; assert(draft->stageEdit(2, 0, e, error));
        e = {}; e.statExperience = std::array<uint16_t, 5>{65535, 100, 0, 9, 25600}; assert(draft->stageEdit(2, 0, e, error));
        e = {}; e.nickname = "BOLT"; e.otName = "WILL"; e.trainerId = 7; assert(draft->stageEdit(2, 0, e, error));
        // The real staged copy is untouched by every draft edit.
        assert(bytes(*real) == stagedBefore);
        const auto drafted = *draft->boxedPokemon(2, 0, error);

        if (!keep) {
            draft.reset(); // Discard
            assert(bytes(*real) == stagedBefore);
            continue;
        }
        // Keep: one verified stageEdit reproduces the drafted Pokemon exactly.
        assert(real->stageEdit(2, 0, PokeBank::UIModel::Gen1EditDraft::editFromRecord(drafted), error));
        const auto kept = *real->boxedPokemon(2, 0, error);
        assert(sameRecord(kept, drafted));
        const auto out = real->finalizedBytes(error);
        assert(!out.empty() && parse(out, game));
        assert(std::equal(raw.begin(), raw.end(), real->originalBytes().begin()));
    }
    // A Keep of an unchanged draft is a no-op on the staged copy.
    auto unchanged = StagedPokemonEditor::create(real->view(), error);
    const auto beforeNoop = bytes(*real);
    assert(real->stageEdit(2, 0, PokeBank::UIModel::Gen1EditDraft::editFromRecord(*unchanged->boxedPokemon(2, 0, error)), error));
    assert(bytes(*real) == beforeNoop);
}

void uiRoutes() {
    const auto ux = read("src/UI/Gen1PokemonEditorOverlayUX.inc");
    const auto redirect = between(ux, "Editor* editor(TrainerViewScreen& screen) noexcept {", "const Editor* editor(const TrainerViewScreen& screen) noexcept {");
    has(redirect, "d.draft && d.owner == &screen && d.base == real");
    has(ux, "auto draft = Editor::create(real->view(), error);");

    const auto f = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    const auto sync = between(f, "void foundationSyncMainMode(TrainerViewScreen& screen) {", "const char* foundationTypeName(");
    has(sync, "if (!beginEditDraft(screen, error)) {");
    has(sync, "state.mode = UX2Mode::Actions;"); // fail closed
    has(sync, "dropEditDraft();");
    const auto commit = between(f, "bool foundationCommitEditDraft(TrainerViewScreen& screen) {", "void foundationExitMainMode(");
    has(commit, "real->stageEdit(");
    has(commit, "PokeBank::UIModel::Gen1EditDraft::editFromRecord(*drafted)");
    has(commit, "foundationSameEditablePokemon(*staged, *drafted)");
    // Both Keep routes (X and the exit dialog's A) commit first and stay open on refusal.
    const auto main = between(f, "if (foundation.editExitConfirm) {\n        const bool create", "std::string error;");
    size_t commits = 0;
    for (auto at = main.find("if (!foundationCommitEditDraft(screen)) return;"); at != std::string::npos;
         at = main.find("if (!foundationCommitEditDraft(screen)) return;", at + 1)) ++commits;
    assert(commits == 2);
    // Discard only drops the draft; no restore/stage path remains.
    const auto discard = between(main, "if (down & HidNpadButton_Y) {", "if (down & HidNpadButton_A) {");
    has(discard, "dropEditDraft();");
    lacks(discard, "stageEdit");
    lacks(f, "foundationRestoreEditBaseline");
    const auto exitMode = between(f, "void foundationExitMainMode(TrainerViewScreen& screen) {", "void foundationRandomizeDVs(");
    has(exitMode, "dropEditDraft();");
}
} // namespace

int main() {
    for (const auto game : {SourceGame::Red, SourceGame::Blue, SourceGame::Yellow}) draftSession(game);
    uiRoutes();
    std::cout << "Gen I Edit working-copy draft (real editor Keep/Discard + UI routes): PASS\n";
}
