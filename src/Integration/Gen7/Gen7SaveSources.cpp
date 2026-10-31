#include "Integration/Gen7/Gen7SaveSources.h"

#include <array>
#include <cstdio>

namespace PokeVault::Integration::Gen7::Sources {
namespace {
constexpr std::array<std::string_view, 4> GameIds = {"sun_3ds", "moon_3ds", "ultrasun_3ds", "ultramoon_3ds"};
}

std::string checkpointFolderPrefix(std::string_view gameId) {
    const uint64_t title = Layout::titleIdForGameId(gameId);
    if (title == 0) return {};
    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%05X ", static_cast<unsigned>((title >> 8) & 0xFFFFF));
    return buffer;
}

std::string_view gameIdFromCheckpointFolder(std::string_view folderName) {
    for (const auto id : GameIds) {
        const std::string prefix = checkpointFolderPrefix(id);
        if (folderName.size() > prefix.size() && folderName.substr(0, prefix.size()) == prefix) return id;
    }
    return {};
}

std::string emulatorSaveFile(std::string_view userDir, std::string_view gameId) {
    const uint64_t title = Layout::titleIdForGameId(gameId);
    if (title == 0 || userDir.empty()) return {};
    char tail[64] = {};
    std::snprintf(tail, sizeof(tail), "title/%08x/%08x/data/00000001/main",
                  static_cast<unsigned>(title >> 32), static_cast<unsigned>(title & 0xFFFFFFFFu));
    std::string out(userDir);
    if (out.back() != '/') out.push_back('/');
    out += "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/";
    out += tail;
    return out;
}

Probe probeSave(std::span<const uint8_t> bytes, const SourceContext& context) {
    Probe out;
    auto& row = out.instance;
    const auto& provider = Gen6::Sources::descriptor(context.provider);
    row.gameId = std::string(context.assignedExactGame);
    row.generation = 7;
    row.platformLabel = context.provider == ProviderId::DekoponEmulator ? "Nintendo 3DS (emulated)" : "Nintendo 3DS";
    row.kind = context.provider == ProviderId::ManualLink ? Source::SaveInstanceKind::ManualImport
               : context.provider == ProviderId::DekoponEmulator ? Source::SaveInstanceKind::BatterySave
                                                                 : Source::SaveInstanceKind::Backup;
    row.providerId = std::string(provider.providerId);
    row.providerLabel = std::string(provider.label);
    row.sourcePath = std::string(context.sourcePath);
    row.location = row.sourcePath;
    row.physicalIdentity = std::string(context.physicalIdentity);
    row.claimedProfile = std::string(context.claimedProfile);
    row.fileSize = bytes.size();
    row.containerType = "3ds-save-archive-main";
    row.access = Source::AccessMode::ReadOnly;
    if (provider.status == ProviderStatus::Unsupported) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "Gen VII provider has no verified Switch save path";
        return out;
    }
    if (!Layout::isGen7GameId(context.assignedExactGame) || context.sourcePath.empty()) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "Gen VII requires an explicit exact-game assignment and a source path";
        return out;
    }
    std::string error;
    auto save = Gen7Save::parse(bytes, {}, &error);
    if (!save) {
        row.validation = Source::ValidationStatus::Invalid;
        row.diagnostic = error;
        return out;
    }
    if (save->exactGameId() != context.assignedExactGame ||
        (!context.evidenceGameId.empty() && context.evidenceGameId != save->exactGameId())) {
        row.validation = Source::ValidationStatus::AssignmentMismatch;
        row.diagnostic = "Validated Gen VII save is " + std::string(save->exactGameId()) +
                         "; assignment or title-ID folder disagrees";
        return out;
    }
    row.validation = Source::ValidationStatus::Ready;
    row.partyCount = save->partyCount();
    std::string name;
    for (const char16_t ch : save->trainer().rawName) {
        if (ch < 0x20 || ch > 0x7E) { name.clear(); break; }
        name.push_back(static_cast<char>(ch));
    }
    row.trainerName = name;
    row.diagnostic = std::string("Gen VII save validated (block CRCs + MemeCrypto signature, payload tail ") +
                     (save->diagnostics().signatureTail == SignatureTail::Zero ? "zero" : "nonzero") +
                     "); read-only, staged export unsupported";
    out.save = std::move(save);
    return out;
}

} // namespace PokeVault::Integration::Gen7::Sources
