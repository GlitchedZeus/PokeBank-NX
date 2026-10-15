// PokeBank NX application QA: reproductions of OPEN defects (Thread 4, QA-owned).
//
// NOT part of host-test / host-sanitize / `make qa`: each case FAILS (non-zero exit) while its defect
// is open, so it can be ported verbatim into the owning thread's suite as the regression test for
// the fix. When the fix lands, the case moves to test_qa_regressions.cpp. Run:
//   make -f Makefile.host qa-known-failures
// Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"

#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "fuzz_synthetic_saves.h"

#include <iostream>

using namespace qacontract;

namespace {

// Fixed cases live in test_qa_regressions.cpp: QA-F4, QA-F5, QA-F6, QA-F7 and QA-F8a.

// QA-F8b (Low, Thread C: Gen4StagedPokemonEditor::stageReleaseBoxPokemon). The review half of QA-F8
// is fixed (c7b0f97, regression QA-F8a in test_qa_regressions.cpp). The export-bytes half is open:
// release writes an all-zero record into a slot whose original held an encrypted blank record (both
// mean empty, PKHeX convention). After clone + release of the clone, a net no-op for the player, the
// working copy still differs from the source in that slot, so edited.sav carries a byte change the
// review no longer lists.
bool gen4NoOpReleaseRestoresOriginalBytes() {
    currentCase() = "QA-F8b Gen IV clone + release of the clone restores the slot's original bytes";
    const int before = failures();
    namespace G4 = PokeVault::Integration::Gen4;
    const auto save = qasynth::gen4::save(qasynth::gen4::Kind::DiamondPearl);
    std::string error;
    auto editor = G4::Gen4StagedPokemonEditor::create(save, G4::Layout::DiamondPearl, "diamond_nds", &error);
    QA_EXPECT(editor.has_value(), "staged editor: " + error);
    if (!editor) return false;
    QA_EXPECT(editor->stageCloneBoxPokemon(0, 0, 1, 0, &error), "clone box 1 slot 1 -> box 2 slot 1: " + error);
    QA_EXPECT(editor->stageReleaseBoxPokemon(1, 0, &error), "release box 2 slot 1: " + error);
    if (failures() != before) return false;
    QA_EXPECT(G4::describeStagedSlotChanges(*editor).empty(), "precondition: review is empty (QA-F8a)");
    const auto staged = editor->stagedBytes();
    size_t differing = 0;
    for (size_t i = 0; i < std::min(staged.size(), save.size()); ++i) differing += staged[i] != save[i];
    QA_EXPECT(!editor->hasChanges() && differing == 0,
              "net no-op still changes " + std::to_string(differing) + " byte(s) of the working copy");
    return failures() == before;
}

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    const bool f8b = gen4NoOpReleaseRestoresOriginalBytes();
    std::cout << "  QA-F8b Gen IV no-op clone+release restores the original slot bytes: " << (f8b ? "fixed" : "OPEN") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " known defect(s) still reproduce\n";
        return 1;
    }
    std::cout << "No known C++ defect reproduces any more: move these cases into test_qa_regressions.cpp.\n";
    return 0;
}
