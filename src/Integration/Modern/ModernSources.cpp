#include "Integration/Modern/ModernSources.h"

#include "Integration/Common/GameCapabilityRegistry.h"
#include "Integration/Gen6/Gen6NativeSession.h"
#include "Integration/Gen6/Gen6SaveSources.h"
#include "Integration/Gen7/Gen7NativeSession.h"
#include "Integration/Gen7/Gen7SaveSources.h"
#include "Integration/Gen7b/Gen7bNativeSession.h"
#include "Integration/Gen8/Gen8NativeSession.h"
#include "Integration/Gen8b/Gen8bNativeSession.h"
#include "Integration/Gen8a/Gen8aNativeSession.h"
#include "Integration/Gen9/Gen9SvNativeSession.h"
#include "Utils/PokeBankPaths.h"
#include "Utils/SHA256.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <utility>

namespace PokeVault::Integration::Modern {
namespace {

std::string hex(const uint8_t* bytes, size_t n) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        out.push_back(digits[bytes[i] >> 4]);
        out.push_back(digits[bytes[i] & 0xF]);
    }
    return out;
}
std::string sha256Hex(std::span<const uint8_t> data) {
    Utils::SHA256 sha;
    sha.update(data.data(), data.size());
    uint8_t digest[32];
    sha.finalize(digest);
    return hex(digest, sizeof digest);
}

bool hasDotSegment(std::string_view path) {
    size_t start = 0;
    while (start <= path.size()) {
        const size_t end = path.find('/', start);
        const auto segment = path.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start);
        if (segment == "." || segment == "..") return true;
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return false;
}
std::string normalizedPath(std::string_view path) {
    std::string out;
    out.reserve(path.size());
    for (const char c : path) {
        if (c == '/' && !out.empty() && out.back() == '/') continue;
        out.push_back(c);
    }
    while (out.size() > 1 && out.back() == '/') out.pop_back();
    return out;
}
std::string physicalIdentity(std::string_view normalized, uint64_t device, uint64_t inode) {
    std::string label = "modern-physical-v1:" + std::string(normalized);
    if (inode != 0) label += ":inode:" + std::to_string(device) + ":" + std::to_string(inode);
    return sha256Hex({reinterpret_cast<const uint8_t*>(label.data()), label.size()});
}

bool readFileContents(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

Gen6::Sources::ProviderId gen6Provider(std::string_view label) noexcept {
    if (label == kProviderDekopon) return Gen6::Sources::ProviderId::DekoponEmulator;
    if (label == kProviderCheckpoint) return Gen6::Sources::ProviderId::CheckpointLayout;
    return Gen6::Sources::ProviderId::ManualLink;
}

const Capability::GameCapabilities* registered(std::string_view gameId) noexcept {
    const auto* row = Capability::find(gameId);
    return row && row->provider == "modern-native" ? row : nullptr;
}

Source::SaveInstance baseRow(const std::string& path, std::string_view providerLabel, std::string_view exactGame) {
    Source::SaveInstance row;
    row.gameId = std::string(exactGame);
    row.generation = registered(exactGame) ? registered(exactGame)->generation : 0;
    row.platformLabel = std::string(platformLabelFor(exactGame));
    row.providerLabel = std::string(providerLabel);
    row.providerId = Source::providerIdFor(providerLabel);
    row.sourcePath = path;
    row.location = path;
    row.normalizedPath = normalizedPath(path);
    row.label = path.substr(path.find_last_of('/') == std::string::npos ? 0 : path.find_last_of('/') + 1);
    row.sourceLabel = providerLabel == kProviderCheckpoint ? "Checkpoint backup" : providerLabel == kProviderDekopon ? "Emulator save" : "Linked file";
    const bool switchMain = sourceVariantFor(exactGame) == kSourceVariantSwitchMain;
    row.containerType = switchMain ? "switch-swish-main" : "3ds-save-archive-main";
    row.sourceVariant = std::string(switchMain ? kSourceVariantSwitchMain : kSourceVariant3ds);
    row.access = Source::AccessMode::ReadOnly;
    row.kind = providerLabel == kProviderManual ? Source::SaveInstanceKind::ManualImport
             : providerLabel == kProviderCheckpoint ? Source::SaveInstanceKind::Backup
                                                     : Source::SaveInstanceKind::BatterySave;
    row.validation = Source::ValidationStatus::Invalid;
    return row;
}

} // namespace

bool isModernAssignedId(std::string_view gameId) noexcept { return registered(gameId) != nullptr; }

std::string_view sourceVariantFor(std::string_view gameId) noexcept {
    const auto* row = registered(gameId);
    if (!row) return {};
    return row->platform == Capability::Platform::NintendoSwitch ? kSourceVariantSwitchMain : kSourceVariant3ds;
}

Enums::GameVersion gameGroupFor(std::string_view gameId) noexcept {
    const auto family = expectedFamily(gameId);
    if (family == "XY") return Enums::GameVersion::XY;
    if (family == "ORAS") return Enums::GameVersion::ORAS;
    if (family == "SM") return Enums::GameVersion::SM;
    if (family == "USUM") return Enums::GameVersion::USUM;
    if (family == "SWSH") return Enums::GameVersion::SWSH;
    if (family == "LGPE") return Enums::GameVersion::GG;
    if (family == "BDSP") return Enums::GameVersion::BDSP;
    if (family == "PLA") return Enums::GameVersion::PLA;
    if (family == "SV") return Enums::GameVersion::SV;
    return Enums::GameVersion::XY;
}

std::string_view expectedFamily(std::string_view gameId) noexcept {
    const auto* row = registered(gameId);
    return row ? row->family : std::string_view{};
}

std::string_view platformLabelFor(std::string_view gameId) noexcept {
    const auto* row = registered(gameId);
    if (!row) return {};
    return row->platform == Capability::Platform::Nintendo3DS ? "Nintendo 3DS" : "Nintendo Switch";
}

std::string_view generationLabelFor(std::string_view gameId) noexcept {
    const auto* row = registered(gameId);
    if (!row) return {};
    switch (row->generation) {
        case 6: return "Generation VI";
        case 7: return "Generation VII";
        case 8: return "Generation VIII";
        case 9: return "Generation IX";
        default: return {};
    }
}

std::string exportsRootFor(std::string_view gameId) {
    const auto* row = registered(gameId);
    if (!row || row->exportRoot.empty()) return {};
    return PokeBank::Paths::root() + "/" + std::string(row->exportRoot);
}

OpenedSession openSession(std::string_view gameId, std::span<const uint8_t> bytes) {
    const auto* row = registered(gameId);
    if (!row) return {nullptr, "No shared Modern adapter serves this title"};
    if (row->generation == 6) return Gen6::Gen6NativeSession::open(bytes, gameId);
    if (row->generation == 7 && row->platform == Capability::Platform::Nintendo3DS)
        return Gen7::Gen7NativeSession::open(bytes, gameId);
    if (row->family == "SWSH") return Gen8::Gen8NativeSession::open(bytes, gameId);
    if (row->family == "LGPE") return Gen7b::Gen7bNativeSession::open(bytes, gameId);
    if (row->family == "BDSP") return Gen8b::Gen8bNativeSession::open(bytes, gameId);
    if (row->family == "PLA") return Gen8a::Gen8aNativeSession::open(bytes, gameId);
    if (row->family == "SV") return Gen9Sv::Gen9SvNativeSession::open(bytes, gameId);
    return {nullptr, std::string(row->title) + " has no verified staged adapter yet: " +
                         std::string(row->status(Capability::Workflow::PokemonEditor).reason)};
}

bool readSourceFileReadOnly(const std::string& path, std::vector<uint8_t>& bytes, std::string& diagnostic) {
    bytes.clear();
    if (hasDotSegment(path)) {
        diagnostic = "unsupported source path with dot or parent segments";
        return false;
    }
    struct stat st{};
    if (::lstat(path.c_str(), &st) != 0) {
        diagnostic = "source file is missing";
        return false;
    }
    if (!S_ISREG(st.st_mode)) {
        diagnostic = "source path is not a regular file";
        return false;
    }
    const auto size = static_cast<size_t>(st.st_size);
    if (size == 0 || size > (4u << 20)) {
        diagnostic = "source file size is outside every supported save size";
        return false;
    }
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) {
        diagnostic = "source file cannot be opened for reading";
        return false;
    }
    bytes.resize(size);
    const bool ok = std::fread(bytes.data(), 1, size, file) == size && std::fgetc(file) == EOF;
    std::fclose(file);
    if (!ok) {
        bytes.clear();
        diagnostic = "source file changed size or could not be read completely";
        return false;
    }
    diagnostic.clear();
    return true;
}

Source::SaveInstance inspectSourceFile(const std::string& path, std::string_view providerLabel,
                                       std::string_view assignedExactGame, std::string_view evidenceGameId) {
    auto row = baseRow(path, providerLabel, assignedExactGame);
    if (!registered(assignedExactGame)) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "No shared Modern adapter serves this title";
        return row;
    }
    if (hasDotSegment(path) || path.empty()) {
        row.validation = Source::ValidationStatus::Unsupported;
        row.diagnostic = "unsupported source path with dot or parent segments";
        return row;
    }
    struct stat st{};
    if (::lstat(path.c_str(), &st) != 0) {
        row.validation = Source::ValidationStatus::Missing;
        row.diagnostic = "source file is missing";
        return row;
    }
    row.fileSize = static_cast<uint64_t>(st.st_size);
    row.modifiedTime = static_cast<int64_t>(st.st_mtime);
    row.physicalIdentity = physicalIdentity(row.normalizedPath, static_cast<uint64_t>(st.st_dev), static_cast<uint64_t>(st.st_ino));
    row.sourceIdentity = row.physicalIdentity;
    std::vector<uint8_t> bytes;
    std::string diagnostic;
    if (!readSourceFileReadOnly(path, bytes, diagnostic)) {
        row.validation = Source::ValidationStatus::ReadError;
        row.diagnostic = diagnostic;
        return row;
    }
    row.contentFingerprint = sha256Hex(bytes);
    if (!evidenceGameId.empty() && evidenceGameId != assignedExactGame) {
        row.validation = Source::ValidationStatus::AssignmentMismatch;
        row.diagnostic = "provider folder names " + std::string(evidenceGameId) + "; reassign the source explicitly";
        return row;
    }
    const auto* reg = registered(assignedExactGame);
    if (reg->generation == 6) {
        Gen6::Sources::SourceContext ctx;
        ctx.provider = gen6Provider(providerLabel);
        ctx.assignedExactGame = assignedExactGame;
        ctx.evidenceGameId = evidenceGameId;
        ctx.sourcePath = path;
        ctx.normalizedPath = row.normalizedPath;
        ctx.physicalIdentity = row.physicalIdentity;
        ctx.sourceIdentity = row.sourceIdentity;
        auto probe = Gen6::Sources::probeSave(bytes, ctx);
        row.validation = probe.instance.validation;
        row.diagnostic = probe.instance.diagnostic;
        row.partyCount = probe.instance.partyCount;
        row.trainerName = probe.instance.trainerName;
        return row;
    }
    if (reg->generation == 7 && reg->platform == Capability::Platform::Nintendo3DS) {
        Gen7::Sources::SourceContext ctx;
        ctx.provider = gen6Provider(providerLabel);
        ctx.assignedExactGame = assignedExactGame;
        ctx.evidenceGameId = evidenceGameId;
        ctx.sourcePath = path;
        ctx.physicalIdentity = row.physicalIdentity;
        auto probe = Gen7::Sources::probeSave(bytes, ctx);
        row.validation = probe.instance.validation;
        row.diagnostic = probe.instance.diagnostic;
        row.partyCount = probe.instance.partyCount;
        row.trainerName = probe.instance.trainerName;
        return row;
    }
    if (reg->family == "SV") {
        std::string why;
        const auto save = Gen9Sv::Gen9SvSave::parse(bytes, assignedExactGame, &why);
        if (!save) {
            row.validation = why.find("does not match the assigned") != std::string::npos
                                 ? Source::ValidationStatus::AssignmentMismatch
                                 : Source::ValidationStatus::Invalid;
            row.diagnostic = why;
            return row;
        }
        row.validation = Source::ValidationStatus::Ready;
        row.diagnostic.clear();
        row.partyCount = save->partyCount();
        std::u16string name;
        for (size_t i = 0; i + 1 < Gen9Sv::Layout::MyNameBytes; i += 2) {
            const uint16_t ch = Gen9Sv::PK8::read16(save->myStatus().data, Gen9Sv::Layout::MyName + i);
            if (ch == 0) break;
            name.push_back(static_cast<char16_t>(ch));
        }
        row.trainerName = Utils::utf16ToUtf8(name);
        return row;
    }
    if (reg->family == "PLA") {
        std::string why;
        const auto save = Gen8a::Gen8aSave::parse(bytes, assignedExactGame, &why);
        if (!save) {
            row.validation = Source::ValidationStatus::Invalid;
            row.diagnostic = why;
            return row;
        }
        row.validation = Source::ValidationStatus::Ready;
        row.diagnostic.clear();
        row.partyCount = save->partyCount();
        std::u16string name;
        for (size_t i = 0; i + 1 < Gen8a::Layout::MyNameBytes; i += 2) {
            const uint16_t ch = Gen8a::PA8::read16(save->myStatus().data, Gen8a::Layout::MyName + i);
            if (ch == 0) break;
            name.push_back(static_cast<char16_t>(ch));
        }
        row.trainerName = Utils::utf16ToUtf8(name);
        return row;
    }
    if (reg->family == "BDSP") {
        std::string why;
        const auto save = Gen8b::Gen8bSave::parse(bytes, assignedExactGame, &why);
        if (!save) {
            row.validation = why.find("does not match the assigned") != std::string::npos
                                 ? Source::ValidationStatus::AssignmentMismatch
                                 : Source::ValidationStatus::Invalid;
            row.diagnostic = why;
            return row;
        }
        row.validation = Source::ValidationStatus::Ready;
        row.diagnostic.clear();
        row.partyCount = save->partyCount();
        row.trainerName = Utils::utf16ToUtf8(save->trainer().name);
        return row;
    }
    if (reg->family == "LGPE") {
        std::string why;
        const auto save = Gen7b::Gen7bSave::parse(bytes, assignedExactGame, &why);
        if (!save) {
            row.validation = why.find("does not match the assigned") != std::string::npos
                                 ? Source::ValidationStatus::AssignmentMismatch
                                 : Source::ValidationStatus::Invalid;
            row.diagnostic = why;
            return row;
        }
        row.validation = Source::ValidationStatus::Ready;
        row.diagnostic.clear();
        row.partyCount = save->model().header.partyCount();
        row.trainerName = Utils::utf16ToUtf8(save->trainer().name);
        return row;
    }
    if (reg->family == "SWSH") {
        // Strict SCBlock parse of the exact assigned title; the same parser the adapter opens with.
        std::string why;
        const auto save = Gen8::Gen8Save::parse(bytes, assignedExactGame, &why);
        if (!save) {
            row.validation = why.find("does not match the assigned") != std::string::npos
                                 ? Source::ValidationStatus::AssignmentMismatch
                                 : Source::ValidationStatus::Invalid;
            row.diagnostic = why;
            return row;
        }
        row.validation = Source::ValidationStatus::Ready;
        row.diagnostic.clear();
        row.partyCount = save->partyCount();
        const auto trainer = save->trainer();
        row.trainerName = Utils::utf16ToUtf8(trainer.rawName);
        return row;
    }
    row.validation = Source::ValidationStatus::Unsupported;
    row.diagnostic = std::string(reg->title) + " has no verified strict parser registered in the shared Modern layer";
    return row;
}

std::vector<Source::SaveInstance> inspectSourceChoices(const std::string& path, std::string_view providerLabel,
                                                       std::string_view assignedExactGame) {
    // SAV6/SAV7 archives hold one native copy; there is never a Primary/Backup choice to offer.
    return {inspectSourceFile(path, providerLabel, assignedExactGame)};
}

namespace {
std::vector<std::string> listDirectories(const std::string& path) {
    std::vector<std::string> out;
    DIR* dir = ::opendir(path.c_str());
    if (!dir) return out;
    while (const dirent* entry = ::readdir(dir)) {
        const std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        struct stat st{};
        if (::stat((path + "/" + name).c_str(), &st) == 0 && S_ISDIR(st.st_mode)) out.push_back(name);
    }
    ::closedir(dir);
    std::sort(out.begin(), out.end());
    return out;
}
bool regularFile(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}
// "<checkpointRoot>/<0xUNIQUE Title>/<backup>/main" for the 3DS family whose folder prefix matches.
void walkCheckpointExports(const std::string& checkpointRoot, std::string_view exactGame,
                           std::string_view (*gameIdFromFolder)(std::string_view), DiscoveryResult& out) {
    for (const auto& folder : listDirectories(checkpointRoot)) {
        const auto evidence = gameIdFromFolder(folder);
        if (evidence.empty() || evidence != exactGame) continue;
        const std::string titleDir = checkpointRoot + "/" + folder;
        for (const auto& backup : listDirectories(titleDir)) {
            const std::string file = titleDir + "/" + backup + "/main";
            if (!regularFile(file)) continue;
            ++out.filesExamined;
            Source::appendDeduplicated(out.instances, inspectSourceFile(file, kProviderCheckpoint, exactGame, evidence));
        }
    }
}
} // namespace

DiscoveryResult discoverKnownSources(std::string_view exactGame, const DiscoveryRoots& roots) {
    DiscoveryResult out;
    const auto* reg = registered(exactGame);
    if (!reg || reg->platform != Capability::Platform::Nintendo3DS) return out;
    const auto consider = [&](const std::string& dir) {
        ++out.rootsConsidered;
        struct stat st{};
        if (::stat(dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            ++out.rootsOpened;
            return true;
        }
        return false;
    };
    const bool gen6 = reg->generation == 6;
    // Checkpoint-layout exports (verified layout: FlagBrew/Checkpoint@740f221a paths.cpp).
    if (consider(roots.checkpointRoot))
        walkCheckpointExports(roots.checkpointRoot, exactGame,
                              gen6 ? &Gen6::Sources::gameIdFromCheckpointFolder : &Gen7::Sources::gameIdFromCheckpointFolder, out);
    // Dekopon (Switch 3DS emulator) user directory: one in-emulator save per title-ID path.
    std::string pointer;
    std::optional<std::string_view> pointerContents;
    if (readFileContents(roots.dekoponPointerFile, pointer)) pointerContents = pointer;
    const auto userDir = Gen6::Sources::dekoponUserDir(pointerContents).value_or(roots.dekoponDefaultUserDir);
    if (consider(userDir)) {
        const std::string file = gen6 ? Gen6::Sources::emulatorSaveFile(userDir, exactGame)
                                      : Gen7::Sources::emulatorSaveFile(userDir, exactGame);
        if (regularFile(file)) {
            ++out.filesExamined;
            Source::appendDeduplicated(out.instances, inspectSourceFile(file, kProviderDekopon, exactGame, exactGame));
        }
    }
    Source::sortNewestFirst(out.instances);
    return out;
}

GameSourceCatalog forGameAndProfile(std::span<const Source::SaveInstance> discovered,
                                    const Legacy::LegacySourceBindings& bindings, std::string_view profile,
                                    std::string_view exactGame) {
    GameSourceCatalog out;
    if (profile.empty() || !registered(exactGame)) return out;
    out.validQuery = true;
    const auto assigned = bindings.resolveFileForGame(profile, exactGame);
    const auto isBoundRow = [&](const Source::SaveInstance& row) {
        return row.sourceIdentity == assigned.sourceIdentity && row.path() == assigned.binding.sourcePath &&
               row.providerLabel == assigned.binding.sourceType;
    };
    std::vector<Source::SaveInstance> normalized;
    for (int priority = 0; priority < 2; ++priority)
        for (const auto& observed : discovered) {
            if (isBoundRow(observed) != (priority == 0)) continue;
            if (!isModernAssignedId(observed.gameId)) continue;
            if (!observed.ready()) { ++out.invalidOrAmbiguous; continue; }
            if (observed.gameId != exactGame) { ++out.wrongGame; continue; }
            if (observed.sourceIdentity.empty() || observed.physicalIdentity.empty() || observed.providerLabel.empty() ||
                observed.providerId.empty() || observed.sourcePath.empty() || observed.contentFingerprint.empty() ||
                !observed.readOnly()) {
                ++out.invalidOrAmbiguous;
                continue;
            }
            Source::appendDeduplicated(normalized, observed);
        }
    for (auto& row : normalized) {
        bindings.applyClaims(row);
        if (!Source::visibleToProfile(row, profile)) {
            if (row.claimConflict) ++out.ambiguousOwnership;
            ++out.ownedByOtherProfile;
            continue;
        }
        row.rememberedSource = bindings.isPreferredGameSource(row, profile, exactGame);
        out.rows.push_back(std::move(row));
    }
    Source::sortNewestFirst(out.rows);
    return out;
}

SourceAssignmentResult assignValidatedSource(Legacy::LegacySourceBindings& bindings, const Source::SaveInstance& selected,
                                             std::string_view profile, std::string_view exactGame) {
    SourceAssignmentResult out;
    if (profile.empty() || !registered(exactGame) || selected.gameId != exactGame || !selected.readOnly() || !selected.ready()) {
        out.diagnostic = "Choose a validated read-only save for this exact game and profile";
        return out;
    }
    auto fresh = inspectSourceFile(selected.path(), selected.providerLabel, exactGame);
    if (!fresh.ready() || !Source::sameValidatedSnapshot(selected, fresh)) {
        out.diagnostic = "Selected file changed since discovery; refresh Save Instances or re-link the file";
        return out;
    }
    bindings.applyClaims(fresh);
    if (!Source::visibleToProfile(fresh, profile)) {
        out.diagnostic = fresh.claimConflict
                             ? "Source ownership is ambiguous; repair or forget the stale assignment before selecting this file"
                             : "This save is assigned to another profile or game";
        return out;
    }
    Legacy::BindingRecord binding;
    binding.profileIdentity = std::string(profile);
    binding.gameIdentity = std::string(exactGame);
    binding.sourcePath = fresh.path();
    binding.sourceType = fresh.providerLabel;
    binding.expectedRawFamily = std::string(expectedFamily(exactGame));
    binding.sourceVariant = std::string(sourceVariantFor(exactGame));
    binding.physicalIdentity = fresh.physicalIdentity;
    binding.contentFingerprint = fresh.contentFingerprint;
    if (!bindings.replaceFileAssignmentAndSave(fresh.sourceIdentity, std::move(binding))) {
        out.diagnostic = "Could not remember the save; previous assignment kept. " + bindings.lastError();
        return out;
    }
    fresh.claimedProfile = std::string(profile);
    fresh.rememberedSource = true;
    out.instance = std::move(fresh);
    out.success = true;
    out.diagnostic = "Exact source remembered for this profile; original unchanged";
    return out;
}

AssignedSourceReadOnly openAssignedSource(const Legacy::LegacySourceBindings& bindings, std::string_view profile,
                                          std::string_view exactGame, std::string_view expectedContentFingerprint) {
    AssignedSourceReadOnly out;
    if (profile.empty() || !registered(exactGame)) {
        out.status = AssignedOpenStatus::AssignmentMismatch;
        out.diagnostic = "Source open requires a profile and a title served by the shared Modern adapter";
        return out;
    }
    out.source = bindings.resolveFileForGame(profile, exactGame);
    switch (out.source.status) {
        case Legacy::AssignedFileStatus::Unassigned: out.status = AssignedOpenStatus::Unassigned; break;
        case Legacy::AssignedFileStatus::Missing: out.status = AssignedOpenStatus::Missing; break;
        case Legacy::AssignedFileStatus::Ambiguous: out.status = AssignedOpenStatus::Ambiguous; break;
        case Legacy::AssignedFileStatus::Unreadable: out.status = AssignedOpenStatus::Unreadable; break;
        case Legacy::AssignedFileStatus::Ready: out.status = AssignedOpenStatus::InvalidSave; break;
    }
    if (out.source.status != Legacy::AssignedFileStatus::Ready) {
        out.diagnostic = "Explicit profile/game assignment unavailable; discovered saves are never substituted";
        return out;
    }
    const auto& binding = out.source.binding;
    if (binding.profileIdentity != profile || binding.gameIdentity != exactGame || binding.sourcePath.empty() ||
        binding.sourceType.empty() || binding.expectedRawFamily != expectedFamily(exactGame) ||
        binding.sourceVariant != sourceVariantFor(exactGame)) {
        out.status = AssignedOpenStatus::AssignmentMismatch;
        out.diagnostic = "Persisted assignment has the wrong exact title, provider or family";
        return out;
    }
    out.instance = inspectSourceFile(binding.sourcePath, binding.sourceType, exactGame);
    if (!out.instance.ready()) {
        switch (out.instance.validation) {
            case Source::ValidationStatus::AssignmentMismatch: out.status = AssignedOpenStatus::AssignmentMismatch; break;
            case Source::ValidationStatus::ReadError:
            case Source::ValidationStatus::Missing: out.status = AssignedOpenStatus::Unreadable; break;
            default: out.status = AssignedOpenStatus::InvalidSave; break;
        }
        out.diagnostic = out.instance.diagnostic;
        return out;
    }
    if (!expectedContentFingerprint.empty() && out.instance.contentFingerprint != expectedContentFingerprint) {
        out.status = AssignedOpenStatus::InvalidSave;
        out.instance.validation = Source::ValidationStatus::Invalid;
        out.diagnostic = "Source contents changed after selection; refresh and choose the Save Instance again";
        return out;
    }
    if (out.instance.sourceIdentity != out.source.sourceIdentity) {
        out.status = AssignedOpenStatus::AssignmentMismatch;
        out.instance.validation = Source::ValidationStatus::AssignmentMismatch;
        out.diagnostic = "Source identity differs from the persisted binding; explicit reassignment required";
        return out;
    }
    // Independent second read before the bytes reach the UI; a concurrent emulator write must not
    // substitute content even when the path/inode is unchanged.
    std::vector<uint8_t> bytes;
    std::string diagnostic;
    if (!readSourceFileReadOnly(binding.sourcePath, bytes, diagnostic) || sha256Hex(bytes) != out.instance.contentFingerprint) {
        out.status = AssignedOpenStatus::InvalidSave;
        out.diagnostic = diagnostic.empty() ? "Source contents changed during final revalidation; reselect the Save Instance" : diagnostic;
        return out;
    }
    auto opened = openSession(exactGame, bytes);
    if (!opened.ready()) {
        out.status = AssignedOpenStatus::InvalidSave;
        out.diagnostic = opened.diagnostic;
        return out;
    }
    out.instance.claimedProfile = std::string(profile);
    out.instance.rememberedSource = true;
    out.session = std::move(opened.session);
    out.status = AssignedOpenStatus::Ready;
    out.diagnostic = "Explicit profile/game source validated read-only; no save write authority";
    return out;
}

Source::HomeCardSnapshot homeCardSnapshot(const AssignedSourceReadOnly& assigned) {
    if (!assigned.ready()) {
        return Source::unavailableSnapshot(assigned.diagnostic.empty()
                                               ? "Linked save did not pass validation; relink it in Save Instances."
                                               : assigned.diagnostic);
    }
    const auto& s = *assigned.session;
    Source::HomeCardSnapshot snap;
    snap.state = Source::SnapshotState::Current;
    snap.gameId = std::string(s.exactGameId());
    snap.sourceIdentity = assigned.instance.sourceIdentity;
    snap.contentFingerprint = assigned.instance.contentFingerprint;
    snap.trainerName = s.trainerName();
    snap.trainerGender = s.trainerGender();
    snap.trainerGenderKnown = true;
    snap.newGameNoTrainer = snap.trainerName.empty() || snap.trainerName == "Unknown";
    const auto dex = s.pokedex();
    snap.dexSeen = dex.seen;
    snap.dexCaught = dex.caught;
    snap.dexTotal = dex.total;
    snap.partyCount = static_cast<uint8_t>(s.partyCount());
    for (size_t i = 0; i < s.partyCount() && i < snap.party.size(); ++i) {
        const auto view = s.view(SlotRef::party(i));
        if (!view || !view->valid || view->empty) {
            return Source::unavailableSnapshot("Linked save has a party slot that did not validate; relink it in Save Instances.");
        }
        snap.party[i] = {view->species, view->level, view->form, view->shiny, view->gender};
    }
    return snap;
}

} // namespace PokeVault::Integration::Modern
