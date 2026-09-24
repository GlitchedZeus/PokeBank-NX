#ifndef UTILS_MOVE_TRANSACTION_PRODUCTION_H
#define UTILS_MOVE_TRANSACTION_PRODUCTION_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Utils/MoveTransaction.h"

namespace PokeBank::Storage::MoveTx::Production {

struct Eligibility {
    bool workspaceDirtyBeforePickup = false;
    bool bankDirtyBeforePickup = false;
    bool editorDirtyBeforePickup = false;
    bool destinationOccupied = false;
    bool destinationLocked = false;
    bool crossGameConversion = false;
    bool copyOperation = false;
};

inline bool eligibleForTrueMove(const Eligibility& e) noexcept {
    return !e.workspaceDirtyBeforePickup && !e.bankDirtyBeforePickup &&
           !e.editorDirtyBeforePickup && !e.destinationOccupied &&
           !e.destinationLocked && !e.crossGameConversion && !e.copyOperation;
}

inline std::string_view expectedStaticFileId(std::string_view gameId) noexcept {
    if (gameId == "letsgo_pikachu_switch" || gameId == "letsgo_eevee_switch")
        return "savedata.bin";
    if (gameId == "sword_switch" || gameId == "shield_switch" ||
        gameId == "legends_arceus_switch" || gameId == "scarlet_switch" ||
        gameId == "violet_switch" || gameId == "legends_za_switch")
        return "main";
    return {};
}

inline bool supportsSingleFileTrueMove(std::string_view gameId) noexcept {
    return !expectedStaticFileId(gameId).empty() ||
           gameId == "firered_switch" || gameId == "leafgreen_switch";
}

inline std::span<const uint8_t> bankMissingSentinel() noexcept {
    static constexpr std::array<uint8_t, 24> kMissing{
        'P','B','N','K','-','M','I','S','S','I','N','G','-','V','1',0,
        0x8d,0x22,0x51,0xa3,0x4f,0x90,0x17,0xce
    };
    return kMissing;
}

inline bool isBankMissingSentinel(std::span<const uint8_t> bytes) noexcept {
    const auto sentinel = bankMissingSentinel();
    return bytes.size() == sentinel.size() &&
           std::equal(bytes.begin(), bytes.end(), sentinel.begin());
}

struct ResolvedStorePath {
    StoreDescriptor descriptor;
    std::string path;
    uint64_t titleId = 0;
    bool bankMayBeMissing = false;
};

class Resolver {
public:
    Resolver();
    Resolver(std::string bankPath, std::string backupsRoot);

    bool resolve(const StoreDescriptor& descriptor,
                 ResolvedStorePath& out,
                 std::string& error) const;

private:
    std::string bankPath_;
    std::string backupsRoot_;
};

class FileStore final : public Store {
public:
    using Validator = PokeBank::Storage::DurableFile::Validator;

    FileStore(ResolvedStorePath resolved,
              Validator validator,
              std::vector<uint8_t> missingReadImage = {});

    const StoreDescriptor& descriptor() const noexcept override { return resolved_.descriptor; }
    bool read(std::vector<uint8_t>& out, std::string& error) const override;
    bool validate(std::span<const uint8_t> bytes, std::string& error) const override;
    bool replace(std::span<const uint8_t> bytes, std::string& error) override;

    const std::string& path() const noexcept { return resolved_.path; }

private:
    ResolvedStorePath resolved_;
    Validator validator_;
    std::vector<uint8_t> missingReadImage_;
};

std::unique_ptr<Store> makeStore(const StoreDescriptor& descriptor,
                                 const Resolver& resolver,
                                 std::string& error);

struct RecoverySweep {
    size_t committedSeen = 0;
    size_t recovered = 0;
    size_t blocked = 0;
    bool mutationLocked = false;
    std::string notice;
    std::vector<std::string> problems;
};

RecoverySweep recoverPendingMoveTransactions(Engine& engine, const Resolver& resolver);
RecoverySweep recoverPendingMoveTransactions();

} // namespace PokeBank::Storage::MoveTx::Production

#endif
