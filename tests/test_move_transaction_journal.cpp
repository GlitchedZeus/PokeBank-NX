#include "Utils/MoveTransaction.h"
#include "Safety/SourceMutationPolicy.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <dirent.h>
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
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    assert(out.good());
}

void ensureDir(const std::string& path) {
    ::mkdir(path.c_str(), 0777);
}

struct FileStore final : Store {
    StoreDescriptor desc;
    std::string path;
    bool failReplace = false;
    bool failValidation = false;
    int replaceCount = 0;

    FileStore(StoreDescriptor descriptor, std::string p, std::vector<uint8_t> initial)
        : desc(std::move(descriptor)), path(std::move(p)) {
        writeFile(path, initial);
    }

    const StoreDescriptor& descriptor() const noexcept override { return desc; }

    bool read(std::vector<uint8_t>& out, std::string& error) const override {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            error = "test store read failed";
            return false;
        }
        out.assign(std::istreambuf_iterator<char>(in), {});
        return true;
    }

    bool validate(std::span<const uint8_t>, std::string& error) const override {
        if (failValidation) {
            error = "intentional test validation failure";
            return false;
        }
        return true;
    }

    bool replace(std::span<const uint8_t> bytes, std::string& error) override {
        ++replaceCount;
        if (failReplace) {
            error = "intentional test replace failure";
            return false;
        }
        auto validator = [&](std::span<const uint8_t> candidate, std::string& e) {
            return validate(candidate, e);
        };
        const auto result = PokeBank::Storage::DurableFile::replace(path, bytes, validator);
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

StoreDescriptor workspaceDescriptor(const std::string& profile = "account-11111111111111112222222222222222",
                                    const std::string& game = "sword_switch",
                                    const std::string& workspace = "Working") {
    StoreDescriptor d;
    d.type = StoreType::MutableWorkspaceSingleFile;
    d.profile = profile;
    d.gameId = game;
    d.workspace = workspace;
    d.fileId = "main";
    return d;
}

Transaction baseTx(const std::string& id, const StoreDescriptor& src, const StoreDescriptor& dst) {
    Transaction tx;
    tx.id = id;
    tx.source = src;
    tx.destination = dst;
    MoveRecord move;
    move.sourceSlot = "bank:0:0";
    move.destinationSlot = "box:0:0";
    move.species = 25;
    move.form = 0;
    move.sourcePayload = sha256(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>("pikachu-source"), 14));
    move.destinationPayload = sha256(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>("pikachu-dest"), 12));
    tx.moves.push_back(move);
    return tx;
}

void mutateByte(const std::string& path, std::streamoff offset, uint8_t value) {
    std::fstream io(path, std::ios::in | std::ios::out | std::ios::binary);
    assert(io);
    io.seekp(offset);
    io.put(static_cast<char>(value));
    assert(io.good());
}

void cleanup(const std::string& root, const std::vector<std::string>& storePaths) {
    for (const auto& path : storePaths) std::remove(path.c_str());
    const std::string records = root + "/records";
    DIR* dir = opendir(records.c_str());
    if (dir) {
        while (dirent* ent = readdir(dir)) {
            const std::string name = ent->d_name;
            if (name == "." || name == "..") continue;
            std::remove((records + "/" + name).c_str());
        }
        closedir(dir);
    }
    rmdir(records.c_str());
    rmdir(root.c_str());
}
}

int main() {
    using namespace PokeBank::Storage::MoveTx;

    const std::string base = "/tmp/pokebank-movetx-" +
        std::to_string(static_cast<long long>(getpid()));
    ensureDir(base);

    const std::vector<uint8_t> sourceBefore{1,2,3,4,5};
    const std::vector<uint8_t> sourceRetired{1,2,3,4,0};
    const std::vector<uint8_t> destinationBefore{9,9,9,9};
    const std::vector<uint8_t> destinationAfter{9,9,9,9,2,5};

    // Descriptor/path policy: only typed PokeBank-owned stores are representable.
    assert(bankDescriptor().valid());
    assert(workspaceDescriptor().valid());
    auto badTraversal = workspaceDescriptor();
    badTraversal.workspace = "../escape";
    assert(!badTraversal.valid());
    auto badProfile = workspaceDescriptor("profile A");
    assert(!badProfile.valid());
    assert(!PokeVault::Safety::canPerform(
        PokeVault::Safety::SourceKind::InstalledGame,
        PokeVault::Safety::SourceMutation::SaveChanges));
    assert(!PokeVault::Safety::canPerform(
        PokeVault::Safety::SourceKind::RetroArchLegacy,
        PokeVault::Safety::SourceMutation::SaveChanges));

    // Strong whole-store fingerprint contract.
    const auto sourceDigest = sha256(sourceBefore);
    assert(sourceDigest == sha256(sourceBefore));
    assert(sourceDigest != sha256(sourceRetired));
    assert(digestHex(sourceDigest).size() == 64);

    // PREPARED only: journal + evidence become durable; neither store changes.
    {
        const std::string root = base + "-prepared";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000001", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        const auto loaded = engine.journal().load(tx.id);
        assert(loaded.status == LoadStatus::Ok);
        assert(loaded.transaction.state == State::Prepared);
        assert(readFile(src.path) == sourceBefore);
        assert(readFile(dst.path) == destinationBefore);
        assert(loaded.transaction.sourceBefore == sha256(sourceBefore));
        assert(loaded.transaction.sourceRetired == sha256(sourceRetired));
        assert(loaded.transaction.destinationBefore == sha256(destinationBefore));
        assert(loaded.transaction.destinationAfter == sha256(destinationAfter));
        assert(loaded.transaction.moves.size() == 1);
        cleanup(root, {src.path, dst.path});
    }

    // Destination write failure must not retire source.
    {
        const std::string root = base + "-destfail";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000002", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        dst.failReplace = true;
        const auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(readFile(src.path) == sourceBefore);
        assert(readFile(dst.path) == destinationBefore);
        assert(src.replaceCount == 0);
        cleanup(root, {src.path, dst.path});
    }

    // Crash after destination bytes land but before journal state advances. Recovery must
    // recognize exact destination post-state, never duplicate it, then retire source once.
    {
        const std::string root = base + "-afterdest";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000003", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto first = engine.recover(tx.id, src, dst, FaultPoint::AfterDestinationWrite);
        assert(first.status == RecoveryStatus::Interrupted);
        assert(readFile(src.path) == sourceBefore);
        assert(readFile(dst.path) == destinationAfter);
        assert(engine.journal().load(tx.id).transaction.state == State::Prepared);
        const int destWrites = dst.replaceCount;

        auto resumed = engine.recover(tx.id, src, dst);
        assert(resumed.status == RecoveryStatus::Committed);
        assert(readFile(dst.path) == destinationAfter);
        assert(readFile(src.path) == sourceRetired);
        assert(dst.replaceCount == destWrites);
        assert(src.replaceCount == 1);

        const int srcWrites = src.replaceCount;
        const int dstWrites = dst.replaceCount;
        auto again = engine.recover(tx.id, src, dst);
        assert(again.status == RecoveryStatus::Committed);
        assert(src.replaceCount == srcWrites);
        assert(dst.replaceCount == dstWrites);
        cleanup(root, {src.path, dst.path});
    }

    // Destination validation failure cannot advance to source retirement.
    {
        const std::string root = base + "-destvalidate";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000004", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        dst.failValidation = true;
        auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(readFile(src.path) == sourceBefore);
        assert(src.replaceCount == 0);
        cleanup(root, {src.path, dst.path});
    }

    // Destination-verified crash: temporary duplication is transaction state, not Clone.
    {
        const std::string root = base + "-destverified";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000005", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto rr = engine.recover(tx.id, src, dst, FaultPoint::AfterDestinationVerifiedJournal);
        assert(rr.status == RecoveryStatus::Interrupted);
        assert(readFile(src.path) == sourceBefore);
        assert(readFile(dst.path) == destinationAfter);
        assert(engine.journal().load(tx.id).transaction.state == State::DestinationVerified);
        assert(engine.recover(tx.id, src, dst).status == RecoveryStatus::Committed);
        cleanup(root, {src.path, dst.path});
    }

    // Crash before and after source retirement are both idempotently recoverable.
    {
        const std::string root = base + "-sourceretire";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000006", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));

        auto before = engine.recover(tx.id, src, dst, FaultPoint::BeforeSourceRetire);
        assert(before.status == RecoveryStatus::Interrupted);
        assert(engine.journal().load(tx.id).transaction.state == State::SourceRetirePending);
        assert(readFile(src.path) == sourceBefore);
        assert(readFile(dst.path) == destinationAfter);

        auto after = engine.recover(tx.id, src, dst, FaultPoint::AfterSourceWrite);
        assert(after.status == RecoveryStatus::Interrupted);
        assert(readFile(src.path) == sourceRetired);
        const int sourceWrites = src.replaceCount;
        assert(engine.journal().load(tx.id).transaction.state == State::SourceRetirePending);

        assert(engine.recover(tx.id, src, dst).status == RecoveryStatus::Committed);
        assert(src.replaceCount == sourceWrites);
        cleanup(root, {src.path, dst.path});
    }

    // Source-retirement validation failure is not COMMITTED.
    {
        const std::string root = base + "-sourcevalidate";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000007", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto paused = engine.recover(tx.id, src, dst, FaultPoint::BeforeSourceRetire);
        assert(paused.status == RecoveryStatus::Interrupted);
        src.failValidation = true;
        auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(engine.journal().load(tx.id).transaction.state == State::SourceRetirePending);
        assert(rr.status != RecoveryStatus::Committed);
        cleanup(root, {src.path, dst.path});
    }

    // Unexpected independent store changes fail closed without mutation.
    {
        const std::string root = base + "-conflict";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000008", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        writeFile(src.path, {4,4,4,4});
        const int sw = src.replaceCount, dw = dst.replaceCount;
        auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Conflict);
        assert(src.replaceCount == sw && dst.replaceCount == dw);
        cleanup(root, {src.path, dst.path});
    }
    {
        const std::string root = base + "-destconflict";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-0000000000000009", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        writeFile(dst.path, {7,7,7,7});
        auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Conflict);
        assert(readFile(src.path) == sourceBefore);
        cleanup(root, {src.path, dst.path});
    }

    // Journal transition durability failure blocks the next destructive step.
    {
        const std::string root = base + "-journalfail";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        auto gate = [](State next) { return next != State::SourceRetirePending; };
        Engine engine(root, gate);
        auto tx = baseTx("tx-000000000000000a", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(readFile(dst.path) == destinationAfter);
        assert(readFile(src.path) == sourceBefore);
        assert(src.replaceCount == 0);
        assert(engine.journal().load(tx.id).transaction.state == State::DestinationVerified);

        Engine normal(root);
        assert(normal.recover(tx.id, src, dst).status == RecoveryStatus::Committed);
        cleanup(root, {src.path, dst.path});
    }

    // Corrupt journal and newer schema are classified without store mutation.
    {
        const std::string root = base + "-corrupt";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-000000000000000b", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        const std::string jp = engine.journal().journalPath(tx.id);
        mutateByte(jp, 20, 0x7f);
        const auto load = engine.journal().load(tx.id);
        assert(load.status == LoadStatus::Corrupt);
        const int sw = src.replaceCount, dw = dst.replaceCount;
        assert(engine.recover(tx.id, src, dst).status == RecoveryStatus::Corrupt);
        assert(src.replaceCount == sw && dst.replaceCount == dw);
        cleanup(root, {src.path, dst.path});
    }
    {
        const std::string root = base + "-newer";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-000000000000000c", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        const std::string jp = engine.journal().journalPath(tx.id);
        // Binary journal version is little-endian immediately after the 8-byte magic.
        mutateByte(jp, 8, static_cast<uint8_t>(kSchemaVersion + 1));
        const auto load = engine.journal().load(tx.id);
        assert(load.status == LoadStatus::UnsupportedVersion);
        assert(engine.recover(tx.id, src, dst).status == RecoveryStatus::UnsupportedVersion);
        assert(readFile(src.path) == sourceBefore);
        cleanup(root, {src.path, dst.path});
    }

    // Descriptor mismatch cannot recover into a different profile/game/workspace.
    {
        const std::string root = base + "-descriptor";
        ensureDir(root);
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx("tx-000000000000000d", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        FileStore wrongDst(
            workspaceDescriptor("account-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "shield_switch", "Other"),
            root + ".wrong", destinationBefore);
        assert(engine.recover(tx.id, src, wrongDst).status == RecoveryStatus::Conflict);
        assert(readFile(src.path) == sourceBefore);
        cleanup(root, {src.path, dst.path, wrongDst.path});
    }

    // Multiple journals enumerate deterministically and independently. One corrupt
    // record does not erase or execute a neighboring valid record.
    {
        const std::string root = base + "-scan";
        ensureDir(root);
        FileStore srcA(bankDescriptor(), root + ".sa", sourceBefore);
        FileStore dstA(workspaceDescriptor(), root + ".da", destinationBefore);
        FileStore srcB(bankDescriptor(), root + ".sb", sourceBefore);
        FileStore dstB(workspaceDescriptor("account-33333333333333334444444444444444",
                                           "scarlet_switch", "Working"),
                       root + ".db", destinationBefore);
        Engine engine(root);
        std::string error;
        auto a = baseTx("tx-000000000000000e", srcA.descriptor(), dstA.descriptor());
        auto b = baseTx("tx-000000000000000f", srcB.descriptor(), dstB.descriptor());
        assert(engine.prepare(a, srcA, dstA, destinationAfter, sourceRetired, error));
        assert(engine.prepare(b, srcB, dstB, destinationAfter, sourceRetired, error));
        mutateByte(engine.journal().journalPath(b.id), 24, 0x55);
        const auto scan = engine.journal().scan();
        assert(scan.size() == 2);
        assert(scan[0].id < scan[1].id);
        assert(scan[0].status == LoadStatus::Ok);
        assert(scan[1].status == LoadStatus::Corrupt);
        assert(readFile(srcA.path) == sourceBefore);
        assert(readFile(srcB.path) == sourceBefore);
        cleanup(root, {srcA.path, dstA.path, srcB.path, dstB.path});
    }

    // Transaction IDs allocated against retained journals never reuse an existing record.
    {
        const std::string root = base + "-ids";
        ensureDir(root);
        Journal journal(root);
        std::string error;
        assert(journal.ensure(error));
        const auto first = journal.allocateTransactionId(error);
        assert(!first.empty());
        // Allocation does not reserve until a journal is persisted; caller stability begins at PREPARED.
        FileStore src(bankDescriptor(), root + ".src", sourceBefore);
        FileStore dst(workspaceDescriptor(), root + ".dst", destinationBefore);
        Engine engine(root);
        auto tx = baseTx(first, src.descriptor(), dst.descriptor());
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        const auto second = journal.allocateTransactionId(error);
        assert(!second.empty() && second != first);
        cleanup(root, {src.path, dst.path});
    }

    std::cout << "Move transaction journal/recovery core: PASS\n";
}
