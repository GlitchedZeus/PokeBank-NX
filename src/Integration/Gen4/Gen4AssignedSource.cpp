#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"

#include <cstdio>

namespace PokeVault::Integration::Gen4 {
AssignedSourceReadOnly openAssignedSource(const Legacy::LegacySourceBindings& bindings,
                                         std::string_view profile, std::string_view game) {
    AssignedSourceReadOnly out;
    out.source = bindings.resolveFileForGame(profile, game);
    switch (out.source.status) {
        case Legacy::AssignedFileStatus::Unassigned: out.status = OpenStatus::Unassigned; break;
        case Legacy::AssignedFileStatus::Missing: out.status = OpenStatus::Missing; break;
        case Legacy::AssignedFileStatus::Ambiguous: out.status = OpenStatus::Ambiguous; break;
        case Legacy::AssignedFileStatus::Unreadable: out.status = OpenStatus::Unreadable; break;
        case Legacy::AssignedFileStatus::Ready: break;
    }
    if (out.source.status != Legacy::AssignedFileStatus::Ready) {
        out.diagnostic = "Assigned save unavailable; choose or reassign a source explicitly";
        return out;
    }
    const auto payload = readSourcePayloadReadOnly(out.source.binding.sourcePath);
    if (!payload.ready()) {
        out.status = payload.status == SourcePayloadStatus::ReadError
            ? OpenStatus::Unreadable : OpenStatus::InvalidSave;
        out.diagnostic = payload.diagnostic;
        return out;
    }
    const auto& bytes = payload.bytes;
    // Detect actual layout first; an assignment never changes how the bytes are interpreted.
    for (Layout layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
        auto candidate = Gen4ReadOnlySave::parse(bytes, layout, game);
        if (!candidate) continue;
        if (out.save) {
            out.save.reset();
            out.status = OpenStatus::InvalidSave;
            out.diagnostic = "Multiple valid raw save layouts; source identity is ambiguous";
            return out;
        }
        out.save = std::move(candidate);
    }
    if (!out.save) {
        out.status = OpenStatus::InvalidSave;
        out.diagnostic = "No validated Gen IV General/Storage layout";
        return out;
    }
    const std::string_view family = out.save->layout() == Layout::DiamondPearl ? "DP" :
        out.save->layout() == Layout::Platinum ? "PT" : "HGSS";
    if (out.save->assignmentStatus() != AssignmentStatus::Match ||
        family != out.source.binding.expectedRawFamily) {
        out.status = OpenStatus::AssignmentMismatch;
        out.diagnostic = "Assigned game/family does not match the validated save; explicit reassignment required";
        return out;
    }
    out.status = OpenStatus::Ready;
    if (out.save->recovered())
        out.diagnostic = "RecoveredOlderCopy: read-only recovery source, not normal retail-state authority";
    else if (payload.kind == SourceContainerKind::DsvFooter)
        out.diagnostic = "Read-only .dsv container adapter; source wrapper remains immutable";
    return out;
}
}
