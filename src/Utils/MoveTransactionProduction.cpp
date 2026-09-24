#include "Utils/MoveTransactionProduction.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <utility>

#include "Games/GameIdentity.h"
#include "Save/GetSaveFileContents.h"
#include "Trainer/Bank.h"
#include "Utils/PokeBankPaths.h"

namespace PokeBank::Storage::MoveTx::Production {
namespace {

bool readRegularFile(const std::string& path, std::vector<uint8_t>& out, std::string& error) {
    errno = 0;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        error = std::strerror(errno);
        return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
        error = "seek failed";
        std::fclose(f);
        return false;
    }
    const long n = std::ftell(f);
    if (n < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
        error = "file size is invalid";
        std::fclose(f);
        return false;
    }
    out.assign(static_cast<size_t>(n), 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size() || !closeOk) {
        error = "file read/close failed";
        return false;
    }
    return true;
}

std::string parentDirectory(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string{} : path.substr(0, slash);
}

bool exactFrlgWorkspaceFile(const std::string& root,
                            std::string_view requested,
                            std::string& error) {
    DIR* dir = ::opendir(root.c_str());
    if (!dir) {
        error = "FRLG workspace directory is unavailable";
        return false;
    }

    std::vector<std::string> candidates;
    while (auto* ent = ::readdir(dir)) {
        const std::string name = ent->d_name;
        if (name == "." || name == ".." || name == "ModifiedSave") continue;
        if (name.find(".tmp.") != std::string::npos ||
            name.find(".previous.") != std::string::npos ||
            name.find(".failed.") != std::string::npos) continue;
        if (!PokeBank::Paths::isSafeComponent(name)) continue;

        const std::string path = root + "/" + name;
        struct stat st{};
        if (::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
            static_cast<size_t>(st.st_size) == Trainer::FRLG_SAVE_SIZE) {
            candidates.push_back(name);
        }
    }
    ::closedir(dir);

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    if (candidates.size() != 1) {
        error = candidates.empty()
            ? "FRLG authoritative 128 KiB workspace file was not found"
            : "FRLG workspace has multiple authoritative-size candidates";
        return false;
    }
    if (candidates.front() != requested) {
        error = "FRLG transaction file does not match the authoritative workspace file";
        return false;
    }
    return true;
}

} // namespace

Resolver::Resolver()
    : bankPath_(PokeBank::Paths::legacyBankRoot() + "/bank.dat"),
      backupsRoot_(PokeBank::Paths::backupsRoot()) {}

Resolver::Resolver(std::string bankPath, std::string backupsRoot)
    : bankPath_(std::move(bankPath)), backupsRoot_(std::move(backupsRoot)) {}

bool Resolver::resolve(const StoreDescriptor& descriptor,
                       ResolvedStorePath& out,
                       std::string& error) const {
    out = {};
    if (!descriptor.valid()) {
        error = "invalid transaction store descriptor";
        return false;
    }

    if (descriptor.type == StoreType::Bank) {
        if (bankPath_.empty()) {
            error = "Bank path is unavailable";
            return false;
        }
        out.descriptor = descriptor;
        out.path = bankPath_;
        out.bankMayBeMissing = true;
        return true;
    }

    if (descriptor.type != StoreType::MutableWorkspaceSingleFile) {
        error = "file-set transaction stores are not enabled";
        return false;
    }
    if (!supportsSingleFileTrueMove(descriptor.gameId)) {
        error = "exact game is not enabled for single-file true Move";
        return false;
    }

    const auto* game = PokeVault::Games::findGame(descriptor.gameId);
    if (!game || !game->isSwitchRelease() || game->switchTitleId == 0) {
        error = "transaction game identity is not a supported Switch release";
        return false;
    }

    const std::string profileRoot = backupsRoot_ + "/" + descriptor.profile;
    const std::string gameRoot = profileRoot + "/" + descriptor.gameId;
    const std::string workspaceRoot = gameRoot + "/" + descriptor.workspace;

    const std::string_view staticFile = expectedStaticFileId(descriptor.gameId);
    if (!staticFile.empty()) {
        if (descriptor.fileId != staticFile) {
            error = "workspace file id is not allowed for this exact game";
            return false;
        }
    } else if (!exactFrlgWorkspaceFile(workspaceRoot, descriptor.fileId, error)) {
        return false;
    }

    const std::string path = workspaceRoot + "/" + descriptor.fileId;
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        error = "authoritative mutable workspace file is unavailable";
        return false;
    }

    out.descriptor = descriptor;
    out.path = path;
    out.titleId = game->switchTitleId;
    return true;
}

FileStore::FileStore(ResolvedStorePath resolved,
                     Validator validator,
                     std::vector<uint8_t> missingReadImage)
    : resolved_(std::move(resolved)),
      validator_(std::move(validator)),
      missingReadImage_(std::move(missingReadImage)) {}

bool FileStore::read(std::vector<uint8_t>& out, std::string& error) const {
    errno = 0;
    if (readRegularFile(resolved_.path, out, error)) return true;
    if (resolved_.bankMayBeMissing && errno == ENOENT && !missingReadImage_.empty()) {
        out = missingReadImage_;
        error.clear();
        return true;
    }
    return false;
}

bool FileStore::validate(std::span<const uint8_t> bytes, std::string& error) const {
    if (!missingReadImage_.empty() &&
        bytes.size() == missingReadImage_.size() &&
        std::equal(bytes.begin(), bytes.end(), missingReadImage_.begin())) {
        return resolved_.bankMayBeMissing;
    }
    if (!validator_) {
        error = "transaction store has no validator";
        return false;
    }
    return validator_(bytes, error);
}

bool FileStore::replace(std::span<const uint8_t> bytes, std::string& error) {
    if (!missingReadImage_.empty() &&
        bytes.size() == missingReadImage_.size() &&
        std::equal(bytes.begin(), bytes.end(), missingReadImage_.begin())) {
        error = "refusing to persist the semantic missing-store sentinel";
        return false;
    }
    if (!validate(bytes, error)) return false;

    const std::string parent = parentDirectory(resolved_.path);
    if (parent.empty() || !PokeBank::Paths::ensureDirectoryTree(parent, &error)) return false;

    const auto durable = PokeBank::Storage::DurableFile::replace(
        resolved_.path, bytes, validator_);
    if (!durable.ok) {
        error = durable.error;
        return false;
    }
    return true;
}

std::unique_ptr<Store> makeStore(const StoreDescriptor& descriptor,
                                 const Resolver& resolver,
                                 std::string& error) {
    ResolvedStorePath resolved;
    if (!resolver.resolve(descriptor, resolved, error)) return nullptr;

    if (descriptor.type == StoreType::Bank) {
        std::vector<uint8_t> missing(bankMissingSentinel().begin(), bankMissingSentinel().end());
        auto validator = [](std::span<const uint8_t> bytes, std::string& e) {
            return Trainer::Bank::validateStorageImage(bytes, e);
        };
        return std::make_unique<FileStore>(std::move(resolved), validator, std::move(missing));
    }

    const uint64_t titleId = resolved.titleId;
    auto validator = [titleId](std::span<const uint8_t> bytes, std::string& e) {
        return Save::validateWorkspaceImage(titleId, bytes, e);
    };
    return std::make_unique<FileStore>(std::move(resolved), std::move(validator));
}

RecoverySweep recoverPendingMoveTransactions(Engine& engine, const Resolver& resolver) {
    RecoverySweep sweep;

    for (const auto& entry : engine.journal().scan()) {
        if (entry.status == LoadStatus::UnsupportedVersion) {
            ++sweep.blocked;
            sweep.mutationLocked = true;
            sweep.problems.push_back(entry.id + ": unsupported transaction schema");
            continue;
        }
        if (entry.status == LoadStatus::Corrupt) {
            ++sweep.blocked;
            sweep.mutationLocked = true;
            sweep.problems.push_back(entry.id + ": corrupt transaction journal");
            continue;
        }
        if (entry.status != LoadStatus::Ok) continue;

        if (entry.state == State::Committed) {
            ++sweep.committedSeen;
            continue;
        }

        const auto loaded = engine.journal().load(entry.id);
        if (loaded.status != LoadStatus::Ok) {
            ++sweep.blocked;
            sweep.mutationLocked = true;
            sweep.problems.push_back(entry.id + ": transaction could not be loaded");
            continue;
        }

        std::string error;
        auto source = makeStore(loaded.transaction.source, resolver, error);
        auto destination = makeStore(loaded.transaction.destination, resolver, error);
        if (!source || !destination) {
            ++sweep.blocked;
            sweep.mutationLocked = true;
            sweep.problems.push_back(entry.id + ": " + error);
            continue;
        }

        const auto recovered = engine.recover(entry.id, *source, *destination);
        if (recovered.status == RecoveryStatus::Committed) {
            ++sweep.recovered;
            continue;
        }

        ++sweep.blocked;
        sweep.mutationLocked = true;
        sweep.problems.push_back(entry.id + ": " +
            (recovered.error.empty() ? std::string("recovery incomplete") : recovered.error));
    }

    if (sweep.mutationLocked) {
        sweep.notice = "Pokemon Move recovery needs attention. Storage moves are locked.";
    } else if (sweep.recovered > 0) {
        sweep.notice = "Interrupted Pokemon Move recovered successfully.";
    }
    return sweep;
}

RecoverySweep recoverPendingMoveTransactions() {
    Engine engine;
    Resolver resolver;
    return recoverPendingMoveTransactions(engine, resolver);
}

} // namespace PokeBank::Storage::MoveTx::Production
