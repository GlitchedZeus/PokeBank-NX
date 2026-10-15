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
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "fuzz_synthetic_saves.h"

#include <iostream>

using namespace qacontract;

namespace {

// Fixed cases live in test_qa_regressions.cpp: QA-F4 (Gen V PID replay links, fixed bc83815),
// QA-F5 (Gen V Home party strip form, fixed on #307).

namespace L = PokeVault::Legacy;

// QA-F6 (Low, Thread C: src/Legacy/RetroArchFRLGDiscovery.cpp identityHint).
// Gen III discovery takes the exact release (Ruby/Sapphire/Emerald/FireRed/LeafGreen) from a raw
// SUBSTRING of the whole lower-cased alphanumeric path, directories included. Any word or folder that
// merely contains a release name therefore decides the identity of a save whose own name carries no
// hint. Ruby/Sapphire and FireRed/LeafGreen saves are byte-identical families, so the family check
// cannot catch it. Gen I (RetroArchRBYDiscovery.cpp identityHint) deliberately accepts only isolated
// path tokens ("/savefiles/red/game.srm" works, "redesign.srm" does not), and a hint-less valid save
// is held as AmbiguousIdentity. Project rule: save discovery never guesses save identity.
bool gen3HintRequiresAnIsolatedToken() {
    currentCase() = "QA-F6 Gen III release hint must not come from a substring of an unrelated folder";
    const int before = failures();
    const auto root = scratchRoot("known-f6");
    const auto save = qasynth::gen3::fixture(); // accepted FR/LG fixture (family marker only)
    const auto inFolder = root / "myleafgreenerybackups" / "game.sav";
    writeSource(inFolder, save);
    const auto control = root / "plain" / "game.sav";
    writeSource(control, save);
    const std::vector<std::string> roots{root.string()};
    const auto result = L::discoverFRLGSaves(roots);
    const L::FRLGSource* hinted = nullptr;
    const L::FRLGSource* neutral = nullptr;
    for (const auto& s : result.sources) {
        if (s.path == inFolder.string()) hinted = &s;
        if (s.path == control.string()) neutral = &s;
    }
    QA_EXPECT(neutral && neutral->status == L::LegacySourceStatus::AmbiguousIdentity,
              "control: a hint-less FR/LG save is held as AmbiguousIdentity");
    QA_EXPECT(hinted != nullptr, "discovery lists the save in the unrelated folder");
    if (failures() != before) return false;
    QA_EXPECT(hinted->status == L::LegacySourceStatus::AmbiguousIdentity,
              "folder 'myleafgreenerybackups' decided the exact release: status " +
                  std::to_string(static_cast<int>(hinted->status)) + ", gameId '" + hinted->gameId + "'");
    return failures() == before;
}

// QA-F7 (Low, Thread C: src/Pokemon/Pokemon4Mutable.cpp setForm).
// setForm() bounds the form by the Gen IV personal table's raw form count. Unown (201) stores 1 there
// because its 28 letters share one stat entry; the pinned reference normalises it explicitly:
// vendor/PKSM-Core/source/personal/personal.cpp PersonalDPPtHGSS::formCount returns 28 for species
// 201. The shared editor therefore refuses every Unown letter except A in D/P, Pt and HG/SS, while
// other multi-form species (Burmy, the control below) edit normally.
bool gen4UnownFormIsEditable() {
    currentCase() = "QA-F7 Gen IV Unown letter (form) is editable";
    const int before = failures();
    using Enums::GameVersion;
    for (const auto group : {GameVersion::DP, GameVersion::PT, GameVersion::HGSS}) {
        for (const uint16_t species : {uint16_t{412}, uint16_t{201}}) { // Burmy (control), Unown
            ::Pokemon::Pokemon4CreateDefaults defaults;
            defaults.species = species;
            defaults.tid = 12150;
            defaults.sid = 22558;
            defaults.originVersion = group == GameVersion::HGSS ? 7 : group == GameVersion::PT ? 12 : 10;
            defaults.otName = u"QA";
            std::string error;
            auto pk = ::Pokemon::Pokemon4Mutable::createStored(defaults, group, &error);
            QA_EXPECT(pk.has_value(), "createStored species " + std::to_string(species) + ": " + error);
            if (!pk) continue;
            const bool set = pk->setForm(1);
            QA_EXPECT(set && pk->form() == 1,
                      std::string(species == 201 ? "Unown" : "Burmy (control)") + " form 1 refused in group " +
                          std::to_string(static_cast<int>(group)));
        }
    }
    return failures() == before;
}

// QA-F8 (Low, presentation; Thread C: Gen4StagedPokemonEditor stageReleaseBoxPokemon +
// Gen4ExportTransaction.cpp describeStagedSlotChanges). Release writes an all-zero record, while
// retail/synthetic empty box slots hold an encrypted blank record (both are "empty", PKHeX
// convention). A clone into an empty slot followed by releasing that clone is a net no-op for the
// player, yet the review and the export manifest still list "Box N slot M: (empty) (edited)", and the
// exported save differs from the source in that slot.
bool gen4NoOpReleaseHasNoReviewLine() {
    currentCase() = "QA-F8 Gen IV clone + release of the clone leaves no review line";
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
    const auto lines = G4::describeStagedSlotChanges(*editor);
    QA_EXPECT(lines.empty(), "net no-op still reviews as: " + (lines.empty() ? std::string() : lines.front()));
    return failures() == before;
}

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    const bool f6 = gen3HintRequiresAnIsolatedToken();
    std::cout << "  QA-F6 Gen III release hint needs an isolated path token: " << (f6 ? "fixed" : "OPEN") << "\n";
    const bool f7 = gen4UnownFormIsEditable();
    std::cout << "  QA-F7 Gen IV Unown letter is editable: " << (f7 ? "fixed" : "OPEN") << "\n";
    const bool f8 = gen4NoOpReleaseHasNoReviewLine();
    std::cout << "  QA-F8 Gen IV no-op clone+release has no review line: " << (f8 ? "fixed" : "OPEN") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " known defect(s) still reproduce\n";
        return 1;
    }
    std::cout << "No known C++ defect reproduces any more: move these cases into test_qa_regressions.cpp.\n";
    return 0;
}
