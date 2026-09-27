#include "Integration/Gen4/Gen4AssignedSource.h"

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
    std::vector<uint8_t> bytes(0x80000);
    FILE* file = std::fopen(out.source.binding.sourcePath.c_str(), "rb");
    if (!file) {
        out.status = OpenStatus::Unreadable;
        out.diagnostic = "Could not open the assigned save read-only";
        return out;
    }
    const size_t count = std::fread(bytes.data(), 1, bytes.size(), file);
    const int extra = std::fgetc(file);
    const bool readError = std::ferror(file) != 0;
    const bool closed = std::fclose(file) == 0;
    if (readError || !closed) {
        out.status = OpenStatus::Unreadable;
        out.diagnostic = "Assigned save read failed";
        return out;
    }
    if (count != bytes.size() || extra != EOF) {
        out.status = OpenStatus::InvalidSave;
        out.diagnostic = "Assigned Gen IV save must be exactly 0x80000 bytes; wrappers are not yet supported";
        return out;
    }
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
    return out;
}
}
