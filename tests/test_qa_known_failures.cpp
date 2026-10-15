// PokeBank NX application QA: reproductions of OPEN defects (Thread 4, QA-owned).
//
// NOT part of host-test / host-sanitize / `make qa`: each case FAILS (non-zero exit) while its defect
// is open, so it can be ported verbatim into the owning thread's suite as the regression test for
// the fix. When the fix lands, the case moves to test_qa_regressions.cpp. Run:
//   make -f Makefile.host qa-known-failures
// Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"

#include "Legacy/RetroArchFRLGDiscovery.h"
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

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    const bool f6 = gen3HintRequiresAnIsolatedToken();
    std::cout << "  QA-F6 Gen III release hint needs an isolated path token: " << (f6 ? "fixed" : "OPEN") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " known defect(s) still reproduce\n";
        return 1;
    }
    std::cout << "No known C++ defect reproduces any more: move these cases into test_qa_regressions.cpp.\n";
    return 0;
}
