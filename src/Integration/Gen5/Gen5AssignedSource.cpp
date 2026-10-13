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

SourceAssignmentResult assignValidatedSource(
    Legacy::LegacySourceBindings& bindings,const Source::SaveInstance& selected,
    std::string_view profile,std::string_view exactGame) {
    SourceAssignmentResult out;
    if(profile.empty() || !isExactGen5Id(exactGame) ||
       selected.gameId!=exactGame || !selected.readOnly() || !selected.ready()) {
        out.diagnostic="Choose a validated read-only save for this exact Gen V game and profile";
        return out;
    }
    const auto fresh=reopenValidatedSource(selected);
    if(!fresh.ready() || fresh.instance.contentFingerprint.empty() ||
       (fresh.instance.sourceVariant!="gen5-copy-0-primary" &&
        fresh.instance.sourceVariant!="gen5-copy-1-backup")) {
        out.diagnostic="Selected Gen V file or copy changed; refresh Save Instances or re-link the file";
        return out;
    }
    auto checked=fresh.instance;
    bindings.applyClaims(checked);
    if(!Source::visibleToProfile(checked,profile)) {
        out.diagnostic="This save is assigned to another profile or game";
        return out;
    }
    Legacy::BindingRecord binding;
    binding.profileIdentity=profile;
    binding.gameIdentity=exactGame;
    binding.sourcePath=checked.path();
    binding.sourceType=checked.providerLabel;
    binding.expectedRawFamily=expectedFamily(exactGame);
    binding.sourceVariant=checked.sourceVariant;
    binding.physicalIdentity=checked.physicalIdentity;
    if(!bindings.replaceFileAssignmentAndSave(checked.sourceIdentity,std::move(binding))) {
        out.diagnostic="Could not remember the Gen V save; previous assignment kept. "+bindings.lastError();
        return out;
    }
    checked.claimedProfile=profile;
    checked.rememberedSource=true;
    out.instance=std::move(checked);
    out.success=true;
    out.diagnostic="Exact Gen V source and copy remembered for this profile; original unchanged";
    return out;
}

AssignedSourceReadOnly openAssignedSource(
    const Legacy::LegacySourceBindings& bindings,
    std::string_view profile,std::string_view exactGame,
    SaveCopySelection copy,std::string_view expectedContentFingerprint) {
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
    if(!binding.sourceVariant.empty()) {
        const auto persisted=binding.sourceVariant=="gen5-copy-0-primary"
            ? SaveCopySelection::Primary : SaveCopySelection::Backup;
        if(binding.sourceVariant!="gen5-copy-0-primary" &&
           binding.sourceVariant!="gen5-copy-1-backup") {
            out.status=AssignedOpenStatus::AssignmentMismatch;
            out.diagnostic="Unsupported remembered Gen V copy; explicit reassignment required";
            return out;
        }
        if(copy!=SaveCopySelection::Automatic && copy!=persisted) {
            out.status=AssignedOpenStatus::AssignmentMismatch;
            out.diagnostic="Gen V copy conflicts with remembered choice; select and assign the desired copy again";
            return out;
        }
        copy=persisted;
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
    // An emulator may update a save in-place after the user picks it:
    // inode/path identity is NOT a snapshot. The exact normalized 0x80000
    // bytes selected in Save Instances must still be current at final Open.
    // An empty fingerprint is only for read-only passive card previews and
    // other callers without an explicit in-session source selection.
    if(!expectedContentFingerprint.empty() &&
       out.instance.contentFingerprint!=expectedContentFingerprint) {
        out.status=AssignedOpenStatus::InvalidSave;
        out.instance.validation=Source::ValidationStatus::Invalid;
        out.diagnostic="Gen V source contents changed after selection; refresh and choose the Save Instance again";
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
    // Independently re-read before handing the native payload to the UI.
    // A concurrent write during the second probe must not substitute bytes
    // even when the file's inode/path were unchanged.
    if(!expectedContentFingerprint.empty() &&
       probe.instance.contentFingerprint!=expectedContentFingerprint) {
        out.status=AssignedOpenStatus::InvalidSave;
        out.diagnostic="Gen V source contents changed during final revalidation; reselect the Save Instance";
        return out;
    }
    out.instance=probe.instance;
    out.save=probe.save;
    out.status=AssignedOpenStatus::Ready;
    out.diagnostic="Explicit Gen V profile/game source validated read-only; no save write authority";
    return out;
}
} // namespace PokeVault::Integration::Gen5
