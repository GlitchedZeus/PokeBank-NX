// PKSE October 2026 campaign (owner Oct 28) source contracts for UI paths without a host harness.
// C-4 (PKSE 620c5f6): the legacy details-page stat dialog called regeneratePID after any IV/EV edit,
// re-keying the record (Let's Go: a new encryption constant). Reachable for Gen VII+ Switch sessions;
// Gen I-V details pages are read-only (ClassicGenericUiGuard). Also, discarding a details-page edit
// restored the bytes but left hasUnsavedChanges set, so Leave offered to save an unchanged save.
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
std::string between(const std::string& s, const std::string& a, const std::string& b) {
    const auto i = s.find(a);
    assert(i != std::string::npos);
    const auto j = s.find(b, i + a.size());
    assert(j != std::string::npos);
    return s.substr(i, j - i);
}
}  // namespace

int main() {
    const std::string ui = read("src/UI/TrainerViewScreenBase.inc");
    const std::string hdr = read("include/UI/TrainerViewScreenBase.h");
    // No UI path rerolls a PID as a side effect of an IV/EV edit.
    assert(ui.find("regeneratePID(") == std::string::npos);
    assert(ui.find("an IV/EV edit never rerolls it") != std::string::npos);
    // Discard restores the dirty flag captured with the snapshot.
    assert(hdr.find("bool unsavedChangesAtSnapshot = false;") != std::string::npos);
    const std::string snap = between(ui, "void TrainerViewScreen::snapshotEditTarget() {", "bool TrainerViewScreen::pokemonEditDirty()");
    assert(snap.find("details.unsavedChangesAtSnapshot = hasUnsavedChanges;") != std::string::npos);
    const std::string restore = between(ui, "void TrainerViewScreen::restoreEditTarget() {", "void TrainerViewScreen::closeDetailsModal()");
    assert(restore.find("hasUnsavedChanges = details.unsavedChangesAtSnapshot;") != std::string::npos);
    assert(restore.find("std::copy(details.editSnapshot.begin()") < restore.find("hasUnsavedChanges = details.unsavedChangesAtSnapshot;"));
    std::cout << "PKSE October source contracts (C-4 PID reroll, discard flag): PASS\n";
    return 0;
}
