#ifndef POKEBANK_LEGACY_SOURCE_BINDINGS_H
#define POKEBANK_LEGACY_SOURCE_BINDINGS_H

#include "Source/SaveInstance.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace PokeVault::Legacy {
    struct BindingFileOps {
        int (*renameFile)(const char*, const char*) = nullptr;
        // Optional fault-injection checkpoint. Production leaves this null.
        int (*checkpoint)(const char*) = nullptr;
    };
    struct BindingRecord {
        std::string profileIdentity;
        // Optional stable game-card identity (for example diamond_nds). Empty means the
        // pre-Gen-IV two-column binding semantics with no exact external game claim.
        std::string gameIdentity;
        // Optional generic file assignment, separate from original save bytes.
        std::string sourcePath;
        std::string sourceType;
        std::string expectedRawFamily;
    };
    enum class AssignedFileStatus { Unassigned, Ambiguous, Missing, Unreadable, Ready };
    struct AssignedFile {
        AssignedFileStatus status = AssignedFileStatus::Unassigned;
        std::string sourceIdentity;
        BindingRecord binding;
    };

    // Persistent, explicit ownership for filesystem-backed legacy saves. The physical discovery
    // catalog is shared, but normal Game Sources visibility is private to the assigned profile.
    // Trainer data and save contents are deliberately never used to infer ownership.
    class LegacySourceBindings {
    public:
        explicit LegacySourceBindings(std::string storagePath = {}, BindingFileOps ops = {});

        [[nodiscard]] bool load();
        [[nodiscard]] bool save() const;

        [[nodiscard]] bool assign(std::string_view sourceIdentity,
                                  std::string_view profileIdentity);
        [[nodiscard]] bool assignAndSave(std::string_view sourceIdentity,
                                        std::string_view profileIdentity);
        [[nodiscard]] bool assign(std::string_view sourceIdentity,
                                  std::string_view profileIdentity,
                                  std::string_view gameIdentity);
        [[nodiscard]] bool assignAndSave(std::string_view sourceIdentity,
                                        std::string_view profileIdentity,
                                        std::string_view gameIdentity);
        [[nodiscard]] bool assignFileAndSave(std::string_view sourceIdentity, BindingRecord binding);
        [[nodiscard]] bool replaceFileAssignmentAndSave(std::string_view sourceIdentity,
                                                        BindingRecord binding);
        [[nodiscard]] bool unassignGameAndSave(std::string_view profileIdentity,
                                               std::string_view gameIdentity);
        // Rechecks only explicitly bound paths. Never scans or substitutes another save.
        [[nodiscard]] AssignedFile resolveFileForGame(std::string_view profileIdentity,
                                                       std::string_view gameIdentity) const;
        void applyClaims(Source::SaveInstance& instance) const;
        [[nodiscard]] bool claimInstanceAndSave(const Source::SaveInstance& instance, std::string_view profile);
        [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }
        [[nodiscard]] bool unassign(std::string_view sourceIdentity);
        [[nodiscard]] bool unassignAndSave(std::string_view sourceIdentity);
        [[nodiscard]] bool isAssigned(std::string_view sourceIdentity) const;
        [[nodiscard]] bool isVisibleTo(std::string_view sourceIdentity,
                                       std::string_view profileIdentity) const;
        [[nodiscard]] std::string assignedProfile(std::string_view sourceIdentity) const;
        [[nodiscard]] std::string assignedGame(std::string_view sourceIdentity) const;
        [[nodiscard]] size_t size() const noexcept { return owners_.size(); }
        [[nodiscard]] const std::string& storagePath() const noexcept { return storagePath_; }

    private:
        std::string storagePath_;
        BindingFileOps ops_;
        mutable std::string lastError_;
        bool checkpoint(const char* stage) const;
        bool fail(const char* stage) const;
        std::unordered_map<std::string, BindingRecord> owners_;
    };
}

#endif
