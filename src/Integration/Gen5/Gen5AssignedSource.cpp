#include "Integration/Gen5/Gen5AssignedSource.h"

namespace PokeVault::Integration::Gen5 {
namespace {
std::string_view expectedFamily(std::string_view exactGame) noexcept {
    if(exactGame=="black_nds" || exactGame=="white_nds")return "BW";
    if(exactGame=="black2_nds" || exactGame=="white2_nds")return "B2W2";
    return {};
}
AssignedOpenStatus mapUnavailable(Legacy::AssignedFileStatus status) noexcept {
    switch(status) {
        case Legacy::AssignedFileStatus::Unassigned: return AssignedOpenStatus::Unassigned;
        case Legacy::AssignedFileStatus::Missing: return AssignedOpenStatus::Missing;
        case Legacy::AssignedFileStatus::Ambiguous: return AssignedOpenStatus::Ambiguous;
        case Legacy::AssignedFileStatus::Unreadable: return AssignedOpenStatus::Unreadable;
        case Legacy::AssignedFileStatus::Ready: return AssignedOpenStatus::InvalidSave;
    }
    return AssignedOpenStatus::InvalidSave;
}
} // namespace

AssignedSourceReadOnly openAssignedSource(
    const Legacy::LegacySourceBindings& bindings,
    std::string_view profile,std::string_view exactGame,
    SaveCopySelection copy) {
    AssignedSourceReadOnly out;
    if(profile.empty() || !isExactGen5Id(exactGame)) {
        out.status=AssignedOpenStatus::AssignmentMismatch;
        out.diagnostic="Generation V source open requires a profile and an exact BW/B2W2 title";
        return out;
    }
    out.source=bindings.resolveFileForGame(profile,exactGame);
    if(out.source.status!=Legacy::AssignedFileStatus::Ready) {
        out.status=mapUnavailable(out.source.status);
        out.diagnostic="Explicit Gen V profile/game assignment unavailable; do not substitute discovered saves";
        return out;
    }

    const auto& binding=out.source.binding;
    if(binding.profileIdentity!=profile || binding.gameIdentity!=exactGame ||
       binding.sourcePath.empty() || binding.sourceType.empty() ||
       binding.expectedRawFamily!=expectedFamily(exactGame)) {
        out.status=AssignedOpenStatus::AssignmentMismatch;
        out.diagnostic="Gen V persisted assignment has wrong exact title, provider, or BW/B2W2 family";
        return out;
    }
    // Independently re-validate the chosen source on open. A filename, family
    // claim or earlier scan is never authority for the actual save format.
    out.instance=inspectSourceFile(binding.sourcePath,binding.sourceType,exactGame,copy);
    if(!out.instance.ready()) {
        switch(out.instance.validation) {
            case Source::ValidationStatus::AssignmentMismatch:
                out.status=AssignedOpenStatus::AssignmentMismatch;break;
            case Source::ValidationStatus::ReadError:
            case Source::ValidationStatus::Missing:
                out.status=AssignedOpenStatus::Unreadable;break;
            default:
                out.status=AssignedOpenStatus::InvalidSave;break;
        }
        out.diagnostic=out.instance.diagnostic;
        return out;
    }
    // The persistent assignment references a precise source identity. Never
    // let a replaced path be silently adopted as the old profile's save.
    if(out.instance.sourceIdentity!=out.source.sourceIdentity) {
        out.status=AssignedOpenStatus::AssignmentMismatch;
        out.diagnostic="Gen V source identity differs from persisted binding; explicit reassignment required";
        out.instance.validation=Source::ValidationStatus::AssignmentMismatch;
        return out;
    }
    out.instance.claimedProfile=std::string(profile);
    out.instance.rememberedSource=true;
    const auto probe=reopenValidatedSource(out.instance,copy);
    if(!probe.ready()) {
        out.status=AssignedOpenStatus::InvalidSave;
        out.diagnostic=probe.instance.diagnostic;
        return out;
    }
    out.instance=probe.instance;
    out.save=probe.save;
    out.status=AssignedOpenStatus::Ready;
    out.diagnostic="Explicit Gen V profile/game source validated read-only; no save write authority";
    return out;
}
} // namespace PokeVault::Integration::Gen5
