#pragma once

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"

#include <string>
#include <vector>

namespace PokeVault::Integration::Gen4 {

struct Gen4ExportRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string sourcePath;
    std::string timestamp;
    std::string applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

// Human-readable differences between the original and staged saves (party, boxes and per-pocket
// Bag item quantities), derived from strict parses of both buffers. Used for the export manifest
// and review summaries.
[[nodiscard]] std::vector<std::string> describeStagedSlotChanges(
    const Gen4StagedPokemonEditor& editor);

// Verified app-owned export of the staged Gen IV copy. The edited bytes must strictly reparse as
// the same exact game (assignment Match, not a recovered older copy) before and after writing.
[[nodiscard]] Common::VerifiedExportResult publishVerifiedGen4Export(
    const Gen4StagedPokemonEditor& editor, const Gen4ExportRequest& request);

} // namespace PokeVault::Integration::Gen4
