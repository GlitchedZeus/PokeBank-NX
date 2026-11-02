#pragma once

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen1/Gen1StagedTrainerEditor.h"

#include <string>
#include <vector>

namespace PokeVault::Integration::Gen1 {

struct Gen1ExportRequest {
    std::string rootDirectory;
    std::string directoryStem;
    std::string sourcePath;
    std::string timestamp;
    std::string applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

// Review lines for every pending staged Pokemon, Bag/PC item and Trainer change, in staging order.
[[nodiscard]] std::vector<std::string> gen1ExportChangeLines(
    const StagedPokemonEditor& editor, const StagedInventoryEditor* inventory = nullptr,
    const StagedTrainerEditor* trainer = nullptr);

// Verified app-owned export of the staged Gen I copy (Boxes, plus Bag/PC items when a staged
// inventory workspace with pending changes is supplied). Both workspaces must have been opened
// from the exact same original bytes. The edited bytes are strictly reparsed as the editor's own
// source game before and after writing; the source save is never opened for writing.
[[nodiscard]] Common::VerifiedExportResult publishVerifiedGen1Export(
    const StagedPokemonEditor& editor, const Gen1ExportRequest& request,
    const StagedInventoryEditor* inventory = nullptr, const StagedTrainerEditor* trainer = nullptr);

} // namespace PokeVault::Integration::Gen1
