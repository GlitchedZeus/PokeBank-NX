#pragma once

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"

#include <string>

namespace PokeVault::Integration::Gen3 {

struct Gen3ExportRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string sourcePath;
    std::string timestamp;
    std::string applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

// Verified app-owned export of the staged Gen III Pokemon copy. The edited bytes must strictly
// reparse (rotating sectors, signatures, checksums, exact family) before and after writing.
// When a staged Bag workspace with pending changes is supplied, its changed sectors are merged in
// (sector-disjoint only; both workspaces must share the exact original) so one export carries both.
[[nodiscard]] Common::VerifiedExportResult publishVerifiedGen3Export(
    const StagedPokemonEditor& editor, const Gen3ExportRequest& request,
    const StagedInventoryEditor* inventory = nullptr);

} // namespace PokeVault::Integration::Gen3
