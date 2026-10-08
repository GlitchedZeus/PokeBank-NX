#ifndef POKEBANK_GEN5_SAVE_INSTANCE_ADAPTER_H
#define POKEBANK_GEN5_SAVE_INSTANCE_ADAPTER_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Source/SaveInstance.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace PokeVault::Integration::Gen5 {

// Pure, read-only boundary between a verified NDS battery-save payload and
// the existing provider-neutral Save Instance UI contract. A caller supplies
// real provider/path provenance: it is NEVER inferred from the save filename,
// string match, emulator installation, or game artwork.
struct SourceContext {
    std::string_view assignedExactGame;
    std::string_view providerId;
    std::string_view providerLabel;
    std::string_view sourcePath;
    std::string_view normalizedPath;
    std::string_view physicalIdentity;
    std::string_view sourceIdentity;
    std::string_view claimedProfile;
    size_t validationHandle = 0;
};

struct ReadOnlyProbe {
    Source::SaveInstance instance;
    std::optional<Gen5ReadOnlySave> save;

    [[nodiscard]] bool ready() const noexcept {
        return instance.ready() && instance.readOnly() && save.has_value();
    }
};

[[nodiscard]] inline bool isExactGen5Id(std::string_view gameId) noexcept {
    return gameId == "black_nds" || gameId == "white_nds" ||
           gameId == "black2_nds" || gameId == "white2_nds";
}

// In-memory only; never opens a filesystem file or modifies a byte.
[[nodiscard]] inline ReadOnlyProbe probeNormalizedBattery(
    std::span<const uint8_t> raw,
    const SourceContext& context,
    SaveCopySelection selection = SaveCopySelection::Automatic) {

    ReadOnlyProbe out;
    auto& row = out.instance;
    row.sourceIndex = context.validationHandle;
    row.gameId = std::string(context.assignedExactGame);
    row.generation = 5;
    row.platformLabel = "Nintendo DS";
    row.kind = context.providerId == "manual" ?
        Source::SaveInstanceKind::ManualImport : Source::SaveInstanceKind::BatterySave;
    row.providerId = std::string(context.providerId);
    row.providerLabel = std::string(context.providerLabel);
    row.sourcePath = std::string(context.sourcePath);
    row.location = row.sourcePath;
    row.normalizedPath = std::string(context.normalizedPath);
    row.physicalIdentity = std::string(context.physicalIdentity);
    row.sourceIdentity = std::string(context.sourceIdentity);
    row.claimedProfile = std::string(context.claimedProfile);
    row.fileSize = raw.size();
    row.containerType = "raw-nds-battery";
    row.access = Source::AccessMode::ReadOnly;

    if (!isExactGen5Id(context.assignedExactGame) ||
        context.providerId.empty() || context.providerLabel.empty() ||
        context.sourcePath.empty()) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "Gen V requires explicit exact-game assignment and provider provenance";
        return out;
    }

    std::string diagnostic;
    auto save = Gen5ReadOnlySave::parse(raw, {}, &diagnostic, selection);
    if (!save) {
        row.validation = diagnostic.find("two valid but different") != std::string::npos ||
                         diagnostic.find("ambiguous") != std::string::npos ?
                         Source::ValidationStatus::Unsupported : Source::ValidationStatus::Invalid;
        row.diagnostic = diagnostic.empty() ? "Gen V save validation failed" : diagnostic;
        return out;
    }
    if (save->exactGameId() != context.assignedExactGame) {
        row.validation = Source::ValidationStatus::AssignmentMismatch;
        row.diagnostic = "Validated Gen V exact title differs from assigned game; reassign explicitly";
        return out;
    }

    row.validation = Source::ValidationStatus::Ready;
    // Do not invent a trainer label until the Gen V UTF-16 text codec is wired.
    row.partyCount = save->partyCount();
    row.diagnostic = save->selectedBackupPartition() ?
        "Gen V backup copy selected explicitly or via checksum fallback; source is read-only" :
        "Gen V primary copy validated; source is read-only";
    out.save = std::move(save);
    return out;
}
} // namespace PokeVault::Integration::Gen5
#endif
