#include "Utils/PhysicalMoveAudit.h"

#include <switch.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "Conversion/RouteEvidence.h"
#include "Utils/DurableFile.h"
#include "Utils/MoveTransaction.h"
#include "Utils/PokeBankPaths.h"

namespace PokeBank::Storage::PhysicalAudit {
namespace {

namespace DF = PokeBank::Storage::DurableFile;
namespace MT = PokeBank::Storage::MoveTx;

constexpr std::string_view kMarkerText =
    "POKEBANK-NX-PHYSICAL-AUDIT-V1\n"
    "DISPOSABLE-POKEBANK-OWNED-DATA-ONLY\n";

std::string auditRoot() {
    return PokeBank::Paths::root() + "/audit";
}

std::string physicalRoot() {
    return auditRoot() + "/physical";
}

std::string markerPath() {
    return auditRoot() + "/ENABLE_PHYSICAL_AUDIT_V1.txt";
}

std::string progressPath() {
    return physicalRoot() + "/progress.txt";
}

struct Progress {
    int test = 1;
    int stage = 0;   // 0 ready, 1 interruption armed/reached, 2 recovered PASS
    int attempt = 1;
};

bool readBytes(const std::string& path, std::vector<uint8_t>& out, std::string& error) {
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
    const long size = std::ftell(f);
    if (size < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
        error = "file size invalid";
        std::fclose(f);
        return false;
    }
    out.assign(static_cast<size_t>(size), 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size() || !closeOk) {
        error = "read/close failed";
        return false;
    }
    return true;
}

bool writeBytes(const std::string& path, std::span<const uint8_t> bytes, std::string& error) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        error = std::strerror(errno);
        return false;
    }
    const size_t wrote = bytes.empty() ? 0 : std::fwrite(bytes.data(), 1, bytes.size(), f);
    bool ok = wrote == bytes.size();
    if (ok && std::fflush(f) != 0) ok = false;
    if (ok && ::fsync(fileno(f)) != 0) ok = false;
    if (std::fclose(f) != 0) ok = false;
    if (!ok) error = "audit metadata write/flush failed";
    return ok;
}

bool writeText(const std::string& path, std::string_view text, std::string& error) {
    return writeBytes(path,
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text.data()), text.size()),
        error);
}

bool fileExists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool validFixture(std::span<const uint8_t> bytes, std::string& error) {
    if (bytes.size() != 8 || bytes[0] != 'P' || bytes[1] != 'B' ||
        bytes[2] != 'A' || bytes[3] != 'U') {
        error = "physical-audit fixture is malformed";
        return false;
    }
    return true;
}

const std::array<uint8_t, 8> kSourceBefore{'P','B','A','U',1,25,0,1};
const std::array<uint8_t, 8> kSourceAfter {'P','B','A','U',1,25,0,0};
const std::array<uint8_t, 8> kDestBefore  {'P','B','A','U',2,25,0,0};
const std::array<uint8_t, 8> kDestAfter   {'P','B','A','U',2,25,0,1};

MT::StoreDescriptor sourceDescriptor() {
    MT::StoreDescriptor d;
    d.type = MT::StoreType::Bank;
    d.fileId = "bank.dat";
    return d;
}

MT::StoreDescriptor destinationDescriptor() {
    MT::StoreDescriptor d;
    d.type = MT::StoreType::MutableWorkspaceSingleFile;
    d.profile = "account-a11d1a11d1a11d1a11d1a11d1a11d1a1";
    d.gameId = "sword_switch";
    d.workspace = "PhysicalAudit";
    d.fileId = "main";
    return d;
}

class AuditFileStore final : public MT::Store {
public:
    AuditFileStore(MT::StoreDescriptor descriptor, std::string path)
        : descriptor_(std::move(descriptor)), path_(std::move(path)) {}

    const MT::StoreDescriptor& descriptor() const noexcept override { return descriptor_; }

    bool read(std::vector<uint8_t>& out, std::string& error) const override {
        if (!fileExists(path_)) {
            const auto recovered = DF::recoverMissingTarget(path_, validFixture);
            if (!recovered.ok) {
                error = recovered.error;
                return false;
            }
        }
        return readBytes(path_, out, error);
    }

    bool validate(std::span<const uint8_t> bytes, std::string& error) const override {
        return validFixture(bytes, error);
    }

    bool replace(std::span<const uint8_t> bytes, std::string& error) override {
        const auto result = DF::replace(path_, bytes, validFixture);
        if (!result.ok) {
            error = result.error;
            return false;
        }
        return true;
    }

    const std::string& path() const noexcept { return path_; }

private:
    MT::StoreDescriptor descriptor_;
    std::string path_;
};

enum class CaseKind : uint8_t {
    Transaction,
    DurableDestination,
    DurableSource,
};

struct CaseDef {
    const char* title;
    const char* simpleAction;
    CaseKind kind;
    MT::FaultPoint txFault;
    DF::AuditCheckpoint durablePoint;
};

const std::array<CaseDef, 8> kCases{{
    {"Prepared journal", "Close the app from HOME when told.", CaseKind::Transaction,
     MT::FaultPoint::AfterPreparedJournal, DF::AuditCheckpoint::BeforeTempOpen},
    {"Destination temp write", "Close the app or reboot when told.", CaseKind::DurableDestination,
     MT::FaultPoint::None, DF::AuditCheckpoint::AfterTempWrite},
    {"Destination fsync", "Reboot or close the app when told.", CaseKind::DurableDestination,
     MT::FaultPoint::None, DF::AuditCheckpoint::AfterTempFsync},
    {"Destination promote", "Reboot or close the app when told.", CaseKind::DurableDestination,
     MT::FaultPoint::None, DF::AuditCheckpoint::AfterPromote},
    {"Destination verified", "Close the app from HOME when told.", CaseKind::Transaction,
     MT::FaultPoint::AfterDestinationVerifiedJournal, DF::AuditCheckpoint::BeforeTempOpen},
    {"Source retire pending", "Reboot or close the app when told.", CaseKind::Transaction,
     MT::FaultPoint::AfterSourceRetirePendingJournal, DF::AuditCheckpoint::BeforeTempOpen},
    {"Disposable source write", "Reboot or close the app when told.", CaseKind::DurableSource,
     MT::FaultPoint::None, DF::AuditCheckpoint::AfterTempWrite},
    {"Final committed journal", "Close the app from HOME when told.", CaseKind::Transaction,
     MT::FaultPoint::AfterCommitted, DF::AuditCheckpoint::BeforeTempOpen},
}};

std::string attemptRoot(const Progress& p) {
    char name[64];
    std::snprintf(name, sizeof(name), "case-%02d-attempt-%03d", p.test, p.attempt);
    return physicalRoot() + "/" + name;
}

std::string reachedPath(const Progress& p) {
    return attemptRoot(p) + "/CHECKPOINT_REACHED.txt";
}

std::string sourcePath(const Progress& p) {
    return attemptRoot(p) + "/source.bin";
}

std::string destinationPath(const Progress& p) {
    return attemptRoot(p) + "/destination.bin";
}

std::string transactionRoot(const Progress& p) {
    return attemptRoot(p) + "/transactions";
}

std::string transactionId(const Progress& p) {
    char id[64];
    std::snprintf(id, sizeof(id), "tx-audit-%02d-%08d", p.test, p.attempt);
    return id;
}

bool saveProgress(const Progress& p, std::string& error) {
    char text[64];
    std::snprintf(text, sizeof(text), "%d %d %d\n", p.test, p.stage, p.attempt);
    return writeText(progressPath(), text, error);
}

Progress loadProgress() {
    Progress p;
    std::vector<uint8_t> bytes;
    std::string error;
    if (!readBytes(progressPath(), bytes, error)) return p;
    std::string text(bytes.begin(), bytes.end());
    int test = 1, stage = 0, attempt = 1;
    if (std::sscanf(text.c_str(), "%d %d %d", &test, &stage, &attempt) == 3 &&
        test >= 1 && test <= static_cast<int>(kCases.size()) + 1 &&
        stage >= 0 && stage <= 2 && attempt >= 1) {
        p.test = test;
        p.stage = stage;
        p.attempt = attempt;
    }
    return p;
}

void clearScreen() {
    std::printf("\x1b[2J\x1b[H");
}

void drawHeader() {
    clearScreen();
    std::printf("POKEBANK NX - AUDIT MODE\n");
    std::printf("========================\n\n");
    std::printf("DISPOSABLE TEST DATA ONLY\n");
    std::printf("Product True Move: DISABLED\n");
    std::printf("Real saves are NOT opened by this harness.\n");
    std::printf("Filesystem: record FAT32/exFAT manually.\n\n");
}

u64 waitForButton(PadState& pad, u64 buttons) {
    while (appletMainLoop()) {
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        if (down & buttons) return down;
        consoleUpdate(nullptr);
        svcSleepThread(16'000'000);
    }
    return 0;
}

[[noreturn]] void waitForPhysicalInterruption(PadState& pad,
                                               const Progress& p,
                                               std::string_view checkpoint) {
    drawHeader();
    const auto& def = kCases[static_cast<size_t>(p.test - 1)];
    std::printf("TEST %d / %zu: %s\n\n", p.test, kCases.size(), def.title);
    std::printf("CHECKPOINT REACHED:\n%.*s\n\n",
                static_cast<int>(checkpoint.size()), checkpoint.data());
    std::printf("%s\n\n", def.simpleAction);
    std::printf("NOW interrupt the app.\n");
    std::printf("HOME -> close the app, or reboot/power off if this case says so.\n");
    std::printf("Then reopen the SAME NRO.\n\n");
    std::printf("Do not remove the SD card. Do not touch real Pokemon saves.\n");
    consoleUpdate(nullptr);

    while (appletMainLoop()) {
        padUpdate(&pad);
        consoleUpdate(nullptr);
        svcSleepThread(16'000'000);
    }
    _exit(0);
}

bool createFreshCase(const Progress& p, std::string& error) {
    const std::string root = attemptRoot(p);
    if (!PokeBank::Paths::ensureDirectoryTree(root, &error)) return false;
    if (!writeBytes(sourcePath(p), kSourceBefore, error)) return false;
    if (!writeBytes(destinationPath(p), kDestBefore, error)) return false;

    AuditFileStore source(sourceDescriptor(), sourcePath(p));
    AuditFileStore destination(destinationDescriptor(), destinationPath(p));
    MT::Engine engine(transactionRoot(p));

    MT::Transaction tx;
    tx.id = transactionId(p);
    tx.source = source.descriptor();
    tx.destination = destination.descriptor();
    tx.crossGameConversion = false; // isolated harness only; product cross-game route remains hard-disabled.

    MT::MoveRecord move;
    move.sourceSlot = "audit-pikachu-source";
    move.destinationSlot = "audit-pikachu-destination";
    move.sourcePayload = MT::sha256(kSourceBefore);
    move.destinationPayload = MT::sha256(kDestAfter);
    move.species = 25;
    move.form = 0;
    tx.moves.push_back(move);

    return engine.prepare(tx, source, destination, kDestAfter, kSourceAfter, error);
}

bool markReached(const Progress& p, std::string_view checkpoint, std::string& error) {
    std::string text = "POKEBANK-NX PHYSICAL AUDIT CHECKPOINT\n";
    text += "test=" + std::to_string(p.test) + "\n";
    text += "attempt=" + std::to_string(p.attempt) + "\n";
    text += "transaction=" + transactionId(p) + "\n";
    text += "checkpoint=" + std::string(checkpoint) + "\n";
    text += "source_before_sha256=" + MT::digestHex(MT::sha256(kSourceBefore)) + "\n";
    text += "destination_after_sha256=" + MT::digestHex(MT::sha256(kDestAfter)) + "\n";
    return writeText(reachedPath(p), text, error);
}

bool runArmedCase(PadState& pad, Progress& p, std::string& error) {
    if (!createFreshCase(p, error)) return false;

    p.stage = 1;
    if (!saveProgress(p, error)) return false;

    const auto& def = kCases[static_cast<size_t>(p.test - 1)];
    AuditFileStore source(sourceDescriptor(), sourcePath(p));
    AuditFileStore destination(destinationDescriptor(), destinationPath(p));
    MT::Engine engine(transactionRoot(p));

    if (def.kind == CaseKind::Transaction) {
        const auto interrupted =
            engine.recover(transactionId(p), source, destination, def.txFault);
        if (interrupted.status != MT::RecoveryStatus::Interrupted) {
            error = interrupted.error.empty()
                ? "transaction checkpoint did not interrupt as expected"
                : interrupted.error;
            return false;
        }
        const std::string_view name = MT::faultPointName(def.txFault);
        if (!markReached(p, name, error)) return false;
        waitForPhysicalInterruption(pad, p, name);
    }

    const std::string target =
        def.kind == CaseKind::DurableDestination ? destination.path() : source.path();

    bool installed = DF::installAuditHook(attemptRoot(p),
        [&](DF::AuditCheckpoint point, std::string_view hookTarget, std::string_view) {
            if (point != def.durablePoint || hookTarget != target) return;

            std::string hookError;
            const std::string_view name = DF::auditCheckpointName(point);
            if (!markReached(p, name, hookError)) {
                drawHeader();
                std::printf("AUDIT ERROR: could not persist checkpoint marker.\n%s\n",
                            hookError.c_str());
                consoleUpdate(nullptr);
                while (appletMainLoop()) svcSleepThread(16'000'000);
                _exit(2);
            }
            waitForPhysicalInterruption(pad, p, name);
        });
    if (!installed) {
        error = "audit hook refused the disposable physical-audit root";
        return false;
    }

    const auto unexpected = engine.recover(transactionId(p), source, destination);
    DF::clearAuditHook();
    error = unexpected.error.empty()
        ? "durable checkpoint was not reached"
        : unexpected.error;
    return false;
}

bool verifyRecoveredCase(const Progress& p, std::string& error) {
    AuditFileStore source(sourceDescriptor(), sourcePath(p));
    AuditFileStore destination(destinationDescriptor(), destinationPath(p));

    MT::Engine restarted(transactionRoot(p));
    const auto recovered = restarted.recover(transactionId(p), source, destination);
    if (recovered.status != MT::RecoveryStatus::Committed) {
        error = recovered.error.empty() ? "restart recovery did not commit" : recovered.error;
        return false;
    }

    std::vector<uint8_t> sourceBytes, destinationBytes;
    if (!source.read(sourceBytes, error) || !destination.read(destinationBytes, error)) return false;
    if (sourceBytes.size() != kSourceAfter.size() ||
        !std::equal(sourceBytes.begin(), sourceBytes.end(), kSourceAfter.begin()) ||
        destinationBytes.size() != kDestAfter.size() ||
        !std::equal(destinationBytes.begin(), destinationBytes.end(), kDestAfter.begin())) {
        error = "recovered source/destination bytes do not match the expected committed state";
        return false;
    }

    MT::Engine again(transactionRoot(p));
    const auto idempotent = again.recover(transactionId(p), source, destination);
    if (idempotent.status != MT::RecoveryStatus::Committed) {
        error = idempotent.error.empty() ? "second recovery was not idempotent" : idempotent.error;
        return false;
    }

    const auto journal = again.journal().load(transactionId(p));
    if (journal.status != MT::LoadStatus::Ok || journal.transaction.state != MT::State::Committed) {
        error = "final journal is not COMMITTED";
        return false;
    }

    std::string result;
    result += "POKEBANK-NX PHYSICAL AUDIT RESULT\n";
    result += "test=" + std::to_string(p.test) + "\n";
    result += "attempt=" + std::to_string(p.attempt) + "\n";
    result += "transaction=" + transactionId(p) + "\n";
    result += "status=PASS\n";
    result += "journal=COMMITTED\n";
    result += "idempotence=PASS\n";
    result += "source_sha256=" + MT::digestHex(MT::sha256(sourceBytes)) + "\n";
    result += "destination_sha256=" + MT::digestHex(MT::sha256(destinationBytes)) + "\n";
    result += "product_true_move=DISABLED\n";
    return writeText(attemptRoot(p) + "/RESULT.txt", result, error);
}

} // namespace

bool shouldRun() {
    std::vector<uint8_t> bytes;
    std::string error;
    if (!readBytes(markerPath(), bytes, error)) return false;
    const std::string marker(bytes.begin(), bytes.end());
    return marker == kMarkerText;
}

int run() {
    std::string pathError;
    if (!PokeBank::Paths::ensureDirectoryTree(physicalRoot(), &pathError)) return 2;

    Conversion::RouteEvidence evidence;
    if (Conversion::routeEnabledForTrueMove(evidence)) {
        // The physical harness must never be used to smuggle product route activation through CI.
        return 3;
    }

    consoleInit(nullptr);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad{};
    padInitializeDefault(&pad);

    Progress progress = loadProgress();
    std::string error;
    if (!fileExists(progressPath()) && !saveProgress(progress, error)) {
        drawHeader();
        std::printf("Could not initialize audit progress: %s\n", error.c_str());
        consoleUpdate(nullptr);
        waitForButton(pad, HidNpadButton_Plus);
        consoleExit(nullptr);
        return 4;
    }

    while (appletMainLoop()) {
        if (progress.test > static_cast<int>(kCases.size())) {
            drawHeader();
            std::printf("ALL %zu PHYSICAL CASES: PASS\n\n", kCases.size());
            std::printf("Results are stored under:\n%s\n\n", physicalRoot().c_str());
            std::printf("Remove ENABLE_PHYSICAL_AUDIT_V1.txt to return to the normal app.\n");
            std::printf("Press + to exit.\n");
            consoleUpdate(nullptr);
            if (waitForButton(pad, HidNpadButton_Plus) != 0) break;
            continue;
        }

        const auto& def = kCases[static_cast<size_t>(progress.test - 1)];

        if (progress.stage == 1 && fileExists(reachedPath(progress))) {
            drawHeader();
            std::printf("TEST %d / %zu: %s\n\n", progress.test, kCases.size(), def.title);
            std::printf("Interrupted checkpoint found.\n");
            std::printf("Running startup recovery + a second idempotence pass...\n\n");
            consoleUpdate(nullptr);

            error.clear();
            if (verifyRecoveredCase(progress, error)) {
                progress.stage = 2;
                saveProgress(progress, error);
            } else {
                drawHeader();
                std::printf("TEST %d: FAIL\n\n%s\n\n", progress.test, error.c_str());
                std::printf("Press X to retry this test with a new disposable attempt.\n");
                std::printf("Press + to exit and report the failure.\n");
                consoleUpdate(nullptr);
                const u64 button = waitForButton(pad, HidNpadButton_X | HidNpadButton_Plus);
                if (button & HidNpadButton_X) {
                    ++progress.attempt;
                    progress.stage = 0;
                    error.clear();
                    saveProgress(progress, error);
                    continue;
                }
                break;
            }
        }

        drawHeader();
        std::printf("TEST %d / %zu: %s\n\n", progress.test, kCases.size(), def.title);

        if (progress.stage == 2) {
            std::printf("PASS - recovery and second recovery both succeeded.\n\n");
            std::printf("A = next test\n");
            std::printf("X = rerun this test\n");
            std::printf("+ = exit\n");
            consoleUpdate(nullptr);
            const u64 button =
                waitForButton(pad, HidNpadButton_A | HidNpadButton_X | HidNpadButton_Plus);
            if (button & HidNpadButton_A) {
                ++progress.test;
                progress.stage = 0;
                progress.attempt = 1;
                error.clear();
                saveProgress(progress, error);
                continue;
            }
            if (button & HidNpadButton_X) {
                ++progress.attempt;
                progress.stage = 0;
                error.clear();
                saveProgress(progress, error);
                continue;
            }
            break;
        }

        if (progress.stage == 1 && !fileExists(reachedPath(progress))) {
            std::printf("The previous attempt was armed but never reached its checkpoint.\n\n");
            std::printf("X = retry with a fresh disposable attempt\n");
            std::printf("+ = exit\n");
            consoleUpdate(nullptr);
            const u64 button = waitForButton(pad, HidNpadButton_X | HidNpadButton_Plus);
            if (button & HidNpadButton_X) {
                ++progress.attempt;
                progress.stage = 0;
                error.clear();
                saveProgress(progress, error);
                continue;
            }
            break;
        }

        std::printf("%s\n\n", def.simpleAction);
        std::printf("A = start this disposable test\n");
        std::printf("+ = exit\n\n");
        std::printf("When the checkpoint screen appears, interrupt the app exactly as shown.\n");
        consoleUpdate(nullptr);

        const u64 button = waitForButton(pad, HidNpadButton_A | HidNpadButton_Plus);
        if (button & HidNpadButton_Plus) break;

        error.clear();
        if (!runArmedCase(pad, progress, error)) {
            DF::clearAuditHook();
            drawHeader();
            std::printf("TEST %d could not arm: %s\n\n", progress.test, error.c_str());
            std::printf("X = retry with a new attempt\n");
            std::printf("+ = exit\n");
            consoleUpdate(nullptr);
            const u64 retry = waitForButton(pad, HidNpadButton_X | HidNpadButton_Plus);
            if (retry & HidNpadButton_X) {
                ++progress.attempt;
                progress.stage = 0;
                error.clear();
                saveProgress(progress, error);
                continue;
            }
            break;
        }
    }

    DF::clearAuditHook();
    consoleExit(nullptr);
    return 0;
}

} // namespace PokeBank::Storage::PhysicalAudit
