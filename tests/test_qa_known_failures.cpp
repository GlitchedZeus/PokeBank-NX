// PokeBank NX application QA: reproductions of OPEN defects (Thread 4, QA-owned).
//
// NOT part of host-test / host-sanitize / `make qa`: each case FAILS (non-zero exit) while its defect
// is open, so it can be ported verbatim into the owning thread's suite as the regression test for
// the fix. Run: make -f Makefile.host qa-known-failures. Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "fuzz_synthetic_saves.h"

#include <iostream>

using namespace qacontract;
namespace Common = PokeVault::Integration::Common;
namespace G1 = PokeVault::Integration::Gen1;

namespace {

// QA-F3 (Low, Thread 2: Integration/Common/VerifiedStagedExport.cpp).
// publishVerifiedStagedExport chooses a destination name with stat() and publishes much later with
// rename(2). POSIX rename() of a directory silently REPLACES an existing empty directory, so a
// destination that appears between the uniqueness check and publication is replaced instead of the
// export failing closed or moving to the next free name. The window is widened deterministically
// here through the caller's strict validator, which the exporter invokes after read-back.
bool commonExportReplacesConcurrentDestination() {
    currentCase() = "QA-F3 common export publish is not exclusive";
    const fs::path root = scratchRoot("kf-f3");
    auto original = qasynth::gen1::save(0x012345);
    auto edited = qasynth::gen1::save(0x054321);
    const fs::path destination = root / "red_gb_20261014_120000";
    int calls = 0;
    const auto strict = Common::gen1StrictValidator(G1::SourceGame::Red);

    Common::VerifiedExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = destination.filename().string();
    request.generationLabel = "Generation I";
    request.fileExtension = ".srm";
    request.gameId = "red_gb";
    request.timestamp = "20261014_120000";
    request.originalBytes = original;
    request.editedBytes = edited;
    request.validator = [&](std::span<const uint8_t> bytes, std::string& error) {
        // Second call = after the files were written and read back, before rename(): another
        // writer creates the destination directory now.
        if (++calls == 2) fs::create_directory(destination);
        return strict(bytes, error);
    };
    const auto result = Common::publishVerifiedStagedExport(request);
    const bool replaced = result.success && result.directory == destination.string();
    QA_EXPECT(!replaced, "an existing destination directory was replaced by rename(2); expected the export "
                         "to fail closed or to publish under a new unique name");
    fs::remove_all(root);
    return !replaced;
}

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    const bool f3 = commonExportReplacesConcurrentDestination();
    std::cout << "  QA-F3 common export publish exclusivity: " << (f3 ? "fixed" : "OPEN") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " known defect(s) still reproduce\n";
        return 1;
    }
    std::cout << "No known defect reproduces any more: move these cases into the owning suite.\n";
    return 0;
}
