#ifndef POKEBANK_GEN7_SAVE_SOURCES_H
#define POKEBANK_GEN7_SAVE_SOURCES_H

#include "Integration/Gen6/Gen6SaveSources.h"
#include "Integration/Gen7/Gen7Save.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>

// Gen VII sources reuse the Gen VI provider descriptors: the Checkpoint and
// Dekopon layouts are title-ID-generic (see docs/gen6/EMULATOR_SAVE_PROVIDERS.md),
// Azahar stays unsupported and JKSM manual-only. Identity comes from contents.
namespace PokeVault::Integration::Gen7::Sources {
using Gen6::Sources::ProviderId;
using Gen6::Sources::ProviderStatus;

[[nodiscard]] std::string checkpointFolderPrefix(std::string_view gameId);
[[nodiscard]] std::string_view gameIdFromCheckpointFolder(std::string_view folderName);
[[nodiscard]] std::string emulatorSaveFile(std::string_view userDir, std::string_view gameId);

struct SourceContext {
    ProviderId provider = ProviderId::ManualLink;
    std::string_view assignedExactGame;
    std::string_view evidenceGameId;
    std::string_view sourcePath;
    std::string_view physicalIdentity;
    std::string_view claimedProfile;
};
struct Probe {
    Source::SaveInstance instance;
    std::optional<Gen7Save> save;
    [[nodiscard]] bool ready() const noexcept { return instance.ready() && save.has_value(); }
};
// Always read-only: a Ready Gen VII row never offers a staged workspace.
[[nodiscard]] Probe probeSave(std::span<const uint8_t> bytes, const SourceContext& context);

} // namespace PokeVault::Integration::Gen7::Sources

#endif
