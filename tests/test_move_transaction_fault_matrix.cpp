#include "Utils/DurableFile.h"
#include "Utils/MoveTransaction.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

using namespace PokeBank::Storage::MoveTx;

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return {std::istreambuf_iterator<char>(in), {}};
}

void writeFile(const std::string& path, std::span<const uint8_t> bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    assert(out.good());
}

bool exists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

bool validFixture(std::span<const uint8_t> bytes, std::string& error) {
    if (bytes.size() != 8 || bytes[0] != 'P' || bytes[1] != 'B' ||
        bytes[2] != 'A' || bytes[3] != 'U') {
        error = "fixture image is malformed";
        return false;
    }
    return true;
}

struct AuditStore final : Store {
    StoreDescriptor d;
    std::string path;

    AuditStore(StoreDescriptor descriptor, std::string p, std::span<const uint8_t> initial)
        : d(std::move(descriptor)), path(std::move(p)) {
        writeFile(path, initial);
    }

    const StoreDescriptor& descriptor() const noexcept override { return d; }

    bool read(std::vector<uint8_t>& out, std::string& error) const override {
        if (!exists(path)) {
            const auto recovered =
                PokeBank::Storage::DurableFile::recoverMissingTarget(path, validFixture);
            if (!recovered.ok) {
                error = recovered.error;
                return false;
            }
        }
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            error = "audit store read failed";
            return false;
        }
        out.assign(std::istreambuf_iterator<char>(in), {});
        return true;
    }

    bool validate(std::span<const uint8_t> bytes, std::string& error) const override {
        return validFixture(bytes, error);
    }

    bool replace(std::span<const uint8_t> bytes, std::string& error) override {
        const auto result =
            PokeBank::Storage::DurableFile::replace(path, bytes, validFixture);
        if (!result.ok) {
            error = result.error;
            return false;
        }
        return true;
    }
};

StoreDescriptor bankDescriptor() {
    StoreDescriptor d;
    d.type = StoreType::Bank;
    d.fileId = "bank.dat";
    return d;
}

StoreDescriptor workspaceDescriptor() {
    StoreDescriptor d;
    d.type = StoreType::MutableWorkspaceSingleFile;
    d.profile = "account-11111111111111112222222222222222";
    d.gameId = "sword_switch";
    d.workspace = "Audit";
    d.fileId = "main";
    return d;
}

bool oneOf(std::span<const uint8_t> bytes,
           std::span<const uint8_t> a,
           std::span<const uint8_t> b) {
    return (bytes.size() == a.size() && std::equal(bytes.begin(), bytes.end(), a.begin())) ||
           (bytes.size() == b.size() && std::equal(bytes.begin(), bytes.end(), b.begin()));
}

} // namespace

int main() {
    namespace DF = PokeBank::Storage::DurableFile;

    const std::vector<uint8_t> sourceBefore{'P','B','A','U',1,1,1,1};
    const std::vector<uint8_t> sourceAfter {'P','B','A','U',1,1,1,0};
    const std::vector<uint8_t> destBefore  {'P','B','A','U',2,2,2,2};
    const std::vector<uint8_t> destAfter   {'P','B','A','U',2,2,2,9};

    const std::string processRoot =
        "/tmp/pokebank-physical-audit-" + std::to_string(static_cast<long long>(getpid()));
    std::filesystem::remove_all(processRoot);
    std::filesystem::create_directories(processRoot);

    // Audit hooks are inert unless explicitly installed on an allowed disposable root.
    std::vector<DF::AuditCheckpoint> seen;
    assert(!DF::installAuditHook("/etc", [&](auto, auto, auto) {}));
    assert(DF::installAuditHook(processRoot, [&](DF::AuditCheckpoint point, auto, auto) {
        seen.push_back(point);
    }));
    {
        const std::string target = processRoot + "/durable.bin";
        writeFile(target, sourceBefore);
        const auto result = DF::replace(target, sourceAfter, validFixture);
        assert(result.ok);
        assert(readFile(target) == sourceAfter);
    }
    DF::clearAuditHook();
    auto saw = [&](DF::AuditCheckpoint p) {
        return std::find(seen.begin(), seen.end(), p) != seen.end();
    };
    assert(saw(DF::AuditCheckpoint::BeforeTempWrite));
    assert(saw(DF::AuditCheckpoint::AfterTempFsync));
    assert(saw(DF::AuditCheckpoint::AfterPreviousPreserve));
    assert(saw(DF::AuditCheckpoint::AfterPromote));
    assert(saw(DF::AuditCheckpoint::AfterPromotedValidate));

    // P1 restart window: old target was renamed aside, process died before temp promotion.
    // Restore ONLY the newest validated previous generation; never promote the temp by guessing.
    {
        const std::string root = processRoot + "/missing-target";
        std::filesystem::create_directories(root);
        const std::string target = root + "/main";
        const std::string previous = target + ".previous.7";
        const std::string temp = target + ".tmp.8";
        writeFile(previous, destBefore);
        writeFile(temp, destAfter);
        assert(!exists(target));

        const auto recovered = DF::recoverMissingTarget(target, validFixture);
        assert(recovered.ok && recovered.restored);
        assert(recovered.restoredFrom == previous);
        assert(readFile(target) == destBefore);
        assert(readFile(temp) == destAfter);
    }

    // When several previous generations exist, restore the newest one that validates.
    {
        const std::string root = processRoot + "/generation-order";
        std::filesystem::create_directories(root);
        const std::string target = root + "/main";
        auto old0 = destBefore;
        auto old1 = destBefore;
        old0[7] = 3;
        old1[7] = 4;
        writeFile(target + ".previous.2", old0);
        writeFile(target + ".previous.5", old1);
        const auto recovered = DF::recoverMissingTarget(target, validFixture);
        assert(recovered.ok && recovered.restored);
        assert(readFile(target) == old1);
    }

    // If preserved generations exist but none validate, recovery fails closed.
    {
        const std::string root = processRoot + "/invalid-previous";
        std::filesystem::create_directories(root);
        const std::string target = root + "/main";
        const std::vector<uint8_t> garbage{0,1,2};
        writeFile(target + ".previous.0", garbage);
        const auto recovered = DF::recoverMissingTarget(target, validFixture);
        assert(!recovered.ok && !recovered.restored);
        assert(!exists(target));
    }

    // Full Move state-machine interruption matrix. Each injected interruption is followed by
    // reconstruction of the Engine object to model a fresh process, real recovery, then a second
    // recovery pass to prove idempotence.
    const std::array<FaultPoint, 14> faults{{
        FaultPoint::AfterPreparedJournal,
        FaultPoint::BeforeDestinationWrite,
        FaultPoint::AfterDestinationWrite,
        FaultPoint::AfterDestinationWrittenJournal,
        FaultPoint::BeforeDestinationVerify,
        FaultPoint::AfterDestinationVerify,
        FaultPoint::AfterDestinationVerifiedJournal,
        FaultPoint::AfterSourceRetirePendingJournal,
        FaultPoint::BeforeSourceRetire,
        FaultPoint::AfterSourceWrite,
        FaultPoint::AfterSourceVerify,
        FaultPoint::AfterSourceRetiredJournal,
        FaultPoint::BeforeCommitted,
        FaultPoint::AfterCommitted,
    }};

    for (size_t i = 0; i < faults.size(); ++i) {
        const std::string root = processRoot + "/case-" + std::to_string(i);
        std::filesystem::create_directories(root);

        AuditStore source(bankDescriptor(), root + "/source.bin", sourceBefore);
        AuditStore destination(workspaceDescriptor(), root + "/destination.bin", destBefore);
        Engine engine(root + "/transactions");

        Transaction tx;
        char id[32];
        std::snprintf(id, sizeof(id), "tx-%016llx",
                      static_cast<unsigned long long>(1000 + i));
        tx.id = id;
        tx.source = source.descriptor();
        tx.destination = destination.descriptor();
        MoveRecord move;
        move.sourceSlot = "audit-source";
        move.destinationSlot = "audit-destination";
        move.sourcePayload = sha256(sourceBefore);
        move.destinationPayload = sha256(destAfter);
        move.species = 25;
        move.form = 0;
        tx.moves.push_back(move);

        std::string error;
        assert(engine.prepare(tx, source, destination, destAfter, sourceAfter, error));

        const auto interrupted = engine.recover(tx.id, source, destination, faults[i]);
        assert(interrupted.status == RecoveryStatus::Interrupted);

        const auto sourceAtCrash = readFile(source.path);
        const auto destinationAtCrash = readFile(destination.path);
        assert(oneOf(sourceAtCrash, sourceBefore, sourceAfter));
        assert(oneOf(destinationAtCrash, destBefore, destAfter));
        // The engine forbids the impossible custody state: retired source with old destination.
        assert(!(sourceAtCrash == sourceAfter && destinationAtCrash == destBefore));

        Engine restarted(root + "/transactions");
        const auto recovered = restarted.recover(tx.id, source, destination);
        assert(recovered.status == RecoveryStatus::Committed);
        assert(readFile(source.path) == sourceAfter);
        assert(readFile(destination.path) == destAfter);

        Engine restartedAgain(root + "/transactions");
        const auto again = restartedAgain.recover(tx.id, source, destination);
        assert(again.status == RecoveryStatus::Committed);
        assert(readFile(source.path) == sourceAfter);
        assert(readFile(destination.path) == destAfter);

        std::cout << "fault-matrix " << faultPointName(faults[i]) << ": PASS\n";
    }

    std::filesystem::remove_all(processRoot);
    std::cout << "Physical durability host fault matrix + idempotence: PASS\n";
}
