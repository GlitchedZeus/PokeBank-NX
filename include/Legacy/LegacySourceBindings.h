#ifndef POKEBANK_LEGACY_SOURCE_BINDINGS_H
#define POKEBANK_LEGACY_SOURCE_BINDINGS_H

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
        [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }
        [[nodiscard]] bool unassign(std::string_view sourceIdentity);
        [[nodiscard]] bool isAssigned(std::string_view sourceIdentity) const;
        [[nodiscard]] bool isVisibleTo(std::string_view sourceIdentity,
                                       std::string_view profileIdentity) const;
        [[nodiscard]] std::string assignedProfile(std::string_view sourceIdentity) const;
        [[nodiscard]] size_t size() const noexcept { return owners_.size(); }
        [[nodiscard]] const std::string& storagePath() const noexcept { return storagePath_; }

    private:
        std::string storagePath_;
        BindingFileOps ops_;
        mutable std::string lastError_;
        bool checkpoint(const char* stage) const;
        bool fail(const char* stage) const;
        std::unordered_map<std::string, std::string> owners_;
    };
}

#endif
