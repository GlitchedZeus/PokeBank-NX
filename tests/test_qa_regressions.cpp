// PokeBank NX application QA: regression tests for QA findings that have been FIXED (Thread 4).
//
// Each case started life as a failing reproduction in qa-known-failures and moved here once the
// owning thread's fix landed. Run by `make qa-contracts` (GCC and Clang, plain + ASan/UBSan).
// Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "fuzz_synthetic_saves.h"

#include <iostream>

using namespace qacontract;
namespace Common = PokeVault::Integration::Common;
namespace G1 = PokeVault::Integration::Gen1;

namespace {

// QA-F3 (Low, Thread 2: Integration/Common/VerifiedStagedExport.cpp; fixed in fa9cfcea).
// publishVerifiedStagedExport used to choose a destination name with stat() and publish much later
// with rename(2), which silently REPLACES an existing empty directory. A destination that appears
// between the uniqueness check and publication must now be left untouched (export fails closed or
// moves on). The window is widened deterministically through the caller's strict validator, which
// the exporter invokes after read-back; a racing empty directory AND a racing non-empty directory
// with a sentinel are both checked.
bool commonExportPublishIsExclusive(bool sentinel) {
    currentCase() = sentinel ? "QA-F3 racing non-empty destination" : "QA-F3 racing empty destination";
    const fs::path root = scratchRoot(sentinel ? "reg-f3-full" : "reg-f3-empty");
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
        // Second call = after the files were written and read back, before publication: another
        // writer creates the destination directory now.
        if (++calls == 2) {
            fs::create_directory(destination);
            if (sentinel) spit(destination / "edited.srm", Bytes{'o', 't', 'h', 'e', 'r'});
        }
        return strict(bytes, error);
    };
    const auto result = Common::publishVerifiedStagedExport(request);
    const bool replaced = result.success && result.directory == destination.string();
    QA_EXPECT(!replaced, "an existing destination directory was replaced; expected the export to fail closed "
                         "or to publish under a new unique name");
    if (sentinel)
        QA_EXPECT(slurp(destination / "edited.srm") == (Bytes{'o', 't', 'h', 'e', 'r'}),
                  "the racing writer's file was modified");
    // No temporary/pending directory or stray file may remain next to the racing destination.
    for (const auto& entry : fs::directory_iterator(root)) {
        const auto name = entry.path().filename().string();
        QA_EXPECT(name.find(".tmp") == std::string::npos && name.find(".pending") == std::string::npos,
                  "temporary directory left behind: " + name);
        if (!result.success) QA_EXPECT(entry.path() == destination, "unexpected entry after refusal: " + name);
    }
    fs::remove_all(root);
    return !replaced;
}

} // namespace

int main() {
    std::cout << "Regression tests for fixed QA findings:\n";
    const bool empty = commonExportPublishIsExclusive(false);
    const bool full = commonExportPublishIsExclusive(true);
    std::cout << "  QA-F3 export publish exclusivity (racing empty dir): " << (empty ? "PASS" : "FAIL") << "\n";
    std::cout << "  QA-F3 export publish exclusivity (racing non-empty dir): " << (full ? "PASS" : "FAIL") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " regression(s) of fixed QA findings\n";
        return 1;
    }
    return 0;
}
