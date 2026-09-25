#include "Conversion/RouteEvidence.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

using namespace Conversion;
using namespace PokeBank::Storage::MoveTx;

void ensureDir(const std::string& path) {
    ::mkdir(path.c_str(), 0777);
}

void writeFile(const std::string& path, std::span<const uint8_t> bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    assert(out.good());
}

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string readText(const std::string& path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> enumMembers(const std::string& text, const std::string& marker) {
    const size_t begin = text.find(marker);
    assert(begin != std::string::npos);
    const size_t end = text.find("};", begin);
    assert(end != std::string::npos);
    std::istringstream lines(text.substr(begin, end - begin));
    std::vector<std::string> names;
    std::string line;
    while (std::getline(lines, line)) {
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string name = trim(line.substr(0, eq));
        if (name.empty() || name == "None") continue;
        names.push_back(std::move(name));
    }
    return names;
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
    DIR* dir = ::opendir(records.c_str());
    if (dir) {
        while (dirent* ent = ::readdir(dir)) {
            const std::string name = ent->d_name;
            if (name == "." || name == "..") continue;
            std::remove((records + "/" + name).c_str());
        }
        ::closedir(dir);
    }
    ::rmdir(records.c_str());
    ::rmdir(root.c_str());
}

StoreDescriptor workspaceDescriptor(const std::string& game,
                                    const std::string& profile,
                                    const std::string& workspace = "Working") {
    StoreDescriptor d;
    d.type = StoreType::MutableWorkspaceSingleFile;
    d.profile = profile;
    d.gameId = game;
    d.workspace = workspace;
    d.fileId = "main";
    return d;
}

struct FileStore final : Store {
    StoreDescriptor d;
    std::string path;
    int replaceCount = 0;

    FileStore(StoreDescriptor descriptor, std::string p, std::span<const uint8_t> initial)
        : d(std::move(descriptor)), path(std::move(p)) {
        writeFile(path, initial);
    }

    const StoreDescriptor& descriptor() const noexcept override { return d; }

    bool read(std::vector<uint8_t>& out, std::string& error) const override {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            error = "test store read failed";
            return false;
        }
        out.assign(std::istreambuf_iterator<char>(in), {});
        return true;
    }

    bool validate(std::span<const uint8_t> bytes, std::string& error) const override {
        if (bytes.empty()) {
            error = "test store rejects empty image";
            return false;
        }
        return true;
    }

    bool replace(std::span<const uint8_t> bytes, std::string& error) override {
        ++replaceCount;
        const auto validator = [&](std::span<const uint8_t> candidate, std::string& e) {
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

Transaction makeTransaction(const std::string& id,
                            const StoreDescriptor& src,
                            const StoreDescriptor& dst) {
    static constexpr uint8_t kSourceEntity[] = {25, 0, 1, 2, 3, 4};
    static constexpr uint8_t kDestinationEntity[] = {25, 0, 5, 6, 7, 8};

    Transaction tx;
    tx.id = id;
    tx.source = src;
    tx.destination = dst;
    tx.crossGameConversion = true;

    MoveRecord move;
    move.sourceSlot = "box:0:0";
    move.destinationSlot = "box:0:0";
    move.species = 25;
    move.form = 0;
    move.sourcePayload = sha256(kSourceEntity);
    move.destinationPayload = sha256(kDestinationEntity);
    tx.moves.push_back(move);
    return tx;
}

RouteEvidence makeEvidence(const Transaction& tx) {
    RouteEvidence evidence;
    evidence.transactionId = tx.id;
    evidence.sourceGameIdentity = "sword_switch";
    evidence.destinationGameIdentity = "scarlet_switch";
    evidence.sourceFormat = Enums::GameVersion::SWSH;
    evidence.destinationFormat = Enums::GameVersion::SV;
    evidence.historicalOriginVersion = static_cast<uint8_t>(Enums::GameVersion::SW);
    evidence.fidelity.sourceOriginVersion = static_cast<uint8_t>(Enums::GameVersion::SW);
    evidence.fidelity.destinationEntityOriginVersion = static_cast<uint8_t>(Enums::GameVersion::SW);
    evidence.sourceStore = tx.source;
    evidence.destinationStore = tx.destination;
    evidence.sourcePayload = tx.moves.front().sourcePayload;
    evidence.destinationPayload = tx.moves.front().destinationPayload;
    evidence.sourceEntity = EntityIdentity{25, 0, 0x12345678u, 0x01020304u};
    evidence.destinationEntity = EntityIdentity{25, 0, 0x12345678u, 0x01020304u};
    evidence.conversionResult = EvidenceConversionResult::Ok;
    evidence.candidateAvailable = true;
    evidence.createdAtUnix = 1000;
    evidence.relation = ProvenanceRelation::Conversion;
    return evidence;
}

void assertSourceSafe(const FileStore& src, std::span<const uint8_t> expected) {
    const auto actual = readFile(src.path);
    assert(actual.size() == expected.size());
    assert(std::equal(actual.begin(), actual.end(), expected.begin()));
    assert(sha256(actual) == sha256(expected));
}

bool contains(const std::vector<std::string>& values, const std::string& needle) {
    return std::find(values.begin(), values.end(), needle) != values.end();
}

} // namespace

int main() {
    const std::string base = "/tmp/pokebank-conversion-evidence-" +
        std::to_string(static_cast<long long>(::getpid()));
    ensureDir(base);

    const std::string sourceProfile = "account-11111111111111112222222222222222";
    const std::string destinationProfile = "account-33333333333333334444444444444444";
    const auto srcDesc = workspaceDescriptor("sword_switch", sourceProfile);
    const auto dstDesc = workspaceDescriptor("scarlet_switch", destinationProfile);

    const std::vector<uint8_t> sourceBefore{1,2,3,4,5};
    const std::vector<uint8_t> sourceRetired{1,2,3,4,0};
    const std::vector<uint8_t> destinationBefore{9,9,9,9};
    const std::vector<uint8_t> destinationAfter{9,9,9,9,2,5};

    // Mapping completeness contract for every currently declared fidelity bit.
    assert(lossPresentationCatalog().size() == 14);
    assert(adaptationPresentationCatalog().size() == 7);
    assert(knownLossMask() == ((1u << 14) - 1u));
    assert(knownAdaptationMask() == ((1u << 7) - 1u));
    for (const auto& item : lossPresentationCatalog()) {
        assert(item.bit != 0 && item.key && *item.key && item.message && *item.message);
    }
    for (const auto& item : adaptationPresentationCatalog()) {
        assert(item.bit != 0 && item.key && *item.key && item.message && *item.message);
    }

    // Source-level completeness guard: adding a new fidelity enum without a presentation-key
    // entry must break this test rather than silently producing an unexplainable loss/adaptation.
    const std::string fidelityHeader = readText("include/Conversion/Fidelity.h");
    const std::string evidenceSource = readText("src/Conversion/RouteEvidence.cpp");
    const auto declaredLosses = enumMembers(fidelityHeader, "enum class Loss");
    const auto declaredAdaptations = enumMembers(fidelityHeader, "enum class Adaptation");
    assert(declaredLosses.size() == lossPresentationCatalog().size());
    assert(declaredAdaptations.size() == adaptationPresentationCatalog().size());
    for (const auto& name : declaredLosses)
        assert(evidenceSource.find("\"" + name + "\"") != std::string::npos);
    for (const auto& name : declaredAdaptations)
        assert(evidenceSource.find("\"" + name + "\"") != std::string::npos);

    Report presentationReport;
    presentationReport.addLoss(Loss::HeldItemDropped);
    presentationReport.addLoss(Loss::HomeTrackerDropped);
    presentationReport.addAdaptation(Adaptation::PidAdjustedForShinyThreshold);
    const auto summary = summarizeFidelity(presentationReport);
    assert(summary.losses.size() == 2);
    assert(summary.adaptations.size() == 1);
    assert(contains(summary.losses, "The held item cannot be carried into the destination and will be removed."));
    assert(contains(summary.losses, "The HOME tracker cannot be stored in the destination format and will be removed."));
    assert(!summary.unknownLossBits && !summary.unknownAdaptationBits);

    // Clone/copy/conversion provenance relationships stay semantically distinct.
    assert(std::string(provenanceRelationName(ProvenanceRelation::Conversion)) == "CONVERSION");
    assert(std::string(provenanceRelationName(ProvenanceRelation::Move)) == "MOVE");
    assert(std::string(provenanceRelationName(ProvenanceRelation::Copy)) == "COPY");
    assert(std::string(provenanceRelationName(ProvenanceRelation::ExactCloneOf)) == "EXACT_CLONE_OF");
    assert(std::string(provenanceRelationName(ProvenanceRelation::DerivedFrom)) == "DERIVED_FROM");
    assert(std::string(provenanceRelationName(ProvenanceRelation::ArchiveRecovery)) == "ARCHIVE_RECOVERY");

    // Zero-loss candidates do not require loss acknowledgement, but route policy remains separate.
    {
        auto tx = makeTransaction("tx-0000000000002001", srcDesc, dstDesc);
        auto evidence = makeEvidence(tx);
        assert(!evidence.requiresLossAcknowledgement());
        assert(evidence.lossPolicySatisfied());
        assert(authorizeSourceRetirement(evidence, tx, true).allowed);
        const auto disabled = authorizeSourceRetirement(evidence, tx, false);
        assert(!disabled.allowed);
        assert(disabled.reason.find("route is disabled") != std::string::npos);
        assert(!routeEnabledForTrueMove(evidence));

        const auto provenance = makeProvenanceNode(evidence, 7);
        assert(provenance.sequence == 7);
        assert(provenance.historicalOriginVersion == static_cast<uint8_t>(Enums::GameVersion::SW));
        assert(provenance.sourceGameIdentity == "sword_switch");
        assert(provenance.destinationGameIdentity == "scarlet_switch");
        assert(provenance.sourceStore.profile == sourceProfile);
        assert(provenance.destinationStore.profile == destinationProfile);
    }

    // Loss-bearing candidate: shown + explicit acknowledgement + exact binding are all required.
    {
        auto tx = makeTransaction("tx-0000000000002002", srcDesc, dstDesc);
        auto evidence = makeEvidence(tx);
        evidence.fidelity.addLoss(Loss::HeldItemDropped);

        auto missing = authorizeSourceRetirement(evidence, tx, true);
        assert(!missing.allowed);
        assert(missing.reason.find("explicitly acknowledged") != std::string::npos);

        std::string error;
        assert(!markLossesAcknowledged(evidence, 1001, error));
        assert(error.find("loss summary was shown") != std::string::npos);
        assert(!evidence.lossesShownToUser);
        assert(!evidence.lossesAcknowledged);

        evidence.lossesShownToUser = true;
        auto notAcknowledged = authorizeSourceRetirement(evidence, tx, true);
        assert(!notAcknowledged.allowed);

        error.clear();
        assert(markLossesAcknowledged(evidence, 1001, error));
        assert(evidence.lossesAcknowledged);
        assert(!std::all_of(evidence.acknowledgementBinding.begin(),
                            evidence.acknowledgementBinding.end(),
                            [](uint8_t b) { return b == 0; }));
        assert(authorizeSourceRetirement(evidence, tx, true).allowed);

        auto staleLoss = evidence;
        staleLoss.fidelity.addLoss(Loss::HomeTrackerDropped);
        auto staleLossResult = authorizeSourceRetirement(staleLoss, tx, true);
        assert(!staleLossResult.allowed);
        assert(staleLossResult.reason.find("stale") != std::string::npos);

        auto staleAdaptation = evidence;
        staleAdaptation.fidelity.addAdaptation(Adaptation::MovePPClamped);
        assert(!authorizeSourceRetirement(staleAdaptation, tx, true).allowed);
    }

    // Exact transaction/candidate/store bindings reject mismatches and ambiguous move evidence.
    {
        auto tx = makeTransaction("tx-0000000000002003", srcDesc, dstDesc);
        auto evidence = makeEvidence(tx);

        auto wrongSource = evidence;
        wrongSource.sourcePayload[0] ^= 0x80;
        assert(!authorizeSourceRetirement(wrongSource, tx, true).allowed);

        auto wrongDestination = evidence;
        wrongDestination.destinationPayload[0] ^= 0x40;
        assert(!authorizeSourceRetirement(wrongDestination, tx, true).allowed);

        auto wrongStore = evidence;
        wrongStore.sourceStore.profile = "account-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        assert(!authorizeSourceRetirement(wrongStore, tx, true).allowed);

        auto wrongRoute = evidence;
        wrongRoute.destinationGameIdentity = "violet_switch";
        wrongRoute.destinationStore.gameId = "violet_switch";
        assert(!authorizeSourceRetirement(wrongRoute, tx, true).allowed);

        auto wrongId = evidence;
        wrongId.transactionId = "tx-0000000000009999";
        assert(!authorizeSourceRetirement(wrongId, tx, true).allowed);

        auto ambiguousTx = tx;
        ambiguousTx.moves.push_back(ambiguousTx.moves.front());
        const auto ambiguous = authorizeSourceRetirement(evidence, ambiguousTx, true);
        assert(!ambiguous.allowed);
        assert(ambiguous.reason.find("ambiguously") != std::string::npos);

        auto unknown = evidence;
        unknown.fidelity.losses |= (1u << 31);
        assert(!authorizeSourceRetirement(unknown, tx, true).allowed);

        auto failed = evidence;
        failed.candidateAvailable = false;
        failed.conversionResult = EvidenceConversionResult::TextNotRepresentable;
        assert(!authorizeSourceRetirement(failed, tx, true).allowed);
    }

    // Durable persistence round-trip; acknowledgement remains bound after restart.
    {
        const std::string root = base + "-persist";
        ensureDir(root);
        EvidenceStore store(root);
        auto tx = makeTransaction("tx-0000000000002010", srcDesc, dstDesc);
        auto evidence = makeEvidence(tx);
        evidence.fidelity.addLoss(Loss::HeldItemDropped);
        evidence.lossesShownToUser = true;
        std::string error;
        assert(markLossesAcknowledged(evidence, 1001, error));
        assert(store.persist(evidence, error));
        const auto loaded = store.load(tx.id);
        assert(loaded.status == EvidenceLoadStatus::Ok);
        assert(loaded.evidence.transactionId == tx.id);
        assert(loaded.evidence.acknowledgementBinding == evidence.acknowledgementBinding);
        assert(authorizeSourceRetirement(loaded.evidence, tx, true).allowed);
        cleanup(root, {});
    }

    // Truncation, corruption and unsupported evidence versions are classified fail-closed.
    {
        const std::string root = base + "-format";
        ensureDir(root);
        EvidenceStore store(root);
        auto tx = makeTransaction("tx-0000000000002011", srcDesc, dstDesc);
        auto evidence = makeEvidence(tx);
        std::string error;
        assert(store.persist(evidence, error));
        const std::string path = store.evidencePath(tx.id);
        const auto full = readFile(path);

        writeFile(path, std::span<const uint8_t>(full.data(), 16));
        assert(store.load(tx.id).status == EvidenceLoadStatus::Corrupt);

        writeFile(path, full);
        mutateByte(path, 40, 0x7f);
        assert(store.load(tx.id).status == EvidenceLoadStatus::Corrupt);

        writeFile(path, full);
        mutateByte(path, 8, static_cast<uint8_t>(kRouteEvidenceVersion + 1));
        assert(store.load(tx.id).status == EvidenceLoadStatus::UnsupportedVersion);

        cleanup(root, {});
    }

    // Missing evidence blocks retirement. Destination may be prepared, but source stays authoritative.
    {
        const std::string root = base + "-missing";
        ensureDir(root);
        FileStore src(srcDesc, root + ".src", sourceBefore);
        FileStore dst(dstDesc, root + ".dst", destinationBefore);
        Engine engine(root, {}, makeEvidenceRetirementGate(EvidenceStore(root), true));
        auto tx = makeTransaction("tx-0000000000002020", src.descriptor(), dst.descriptor());
        std::string error;
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        const auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(rr.state == State::DestinationVerified);
        assertSourceSafe(src, sourceBefore);
        assert(src.replaceCount == 0);
        cleanup(root, {src.path, dst.path});
    }

    // Product route-disabled gate overrides even perfect zero-loss persisted evidence.
    {
        const std::string root = base + "-route-disabled";
        ensureDir(root);
        FileStore src(srcDesc, root + ".src", sourceBefore);
        FileStore dst(dstDesc, root + ".dst", destinationBefore);
        auto tx = makeTransaction("tx-0000000000002021", src.descriptor(), dst.descriptor());
        EvidenceStore evidenceStore(root);
        std::string error;

        Engine engine(root, {}, makeEvidenceRetirementGate(evidenceStore, false));
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto evidence = makeEvidence(tx);
        assert(evidenceStore.persist(evidence, error));

        const auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(rr.state == State::DestinationVerified);
        assert(rr.error.find("route is disabled") != std::string::npos);
        assertSourceSafe(src, sourceBefore);
        assert(src.replaceCount == 0);
        cleanup(root, {src.path, dst.path});
    }

    // Evidence survives restart. A harness-only enabled gate proves recovery plumbing;
    // product routeEnabledForTrueMove() remains false above and no route is enabled here.
    {
        const std::string root = base + "-restart";
        ensureDir(root);
        FileStore src(srcDesc, root + ".src", sourceBefore);
        FileStore dst(dstDesc, root + ".dst", destinationBefore);
        auto tx = makeTransaction("tx-0000000000002022", src.descriptor(), dst.descriptor());
        EvidenceStore evidenceStore(root);
        std::string error;

        {
            Engine first(root, {}, makeEvidenceRetirementGate(evidenceStore, false));
            assert(first.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
            auto evidence = makeEvidence(tx);
            assert(evidenceStore.persist(evidence, error));
            const auto blocked = first.recover(tx.id, src, dst);
            assert(blocked.status == RecoveryStatus::Failed);
            assert(blocked.state == State::DestinationVerified);
            assertSourceSafe(src, sourceBefore);
        }

        Engine restarted(root, {}, makeEvidenceRetirementGate(EvidenceStore(root), true));
        const auto recovered = restarted.recover(tx.id, src, dst);
        assert(recovered.status == RecoveryStatus::Committed);
        assert(readFile(src.path) == sourceRetired);
        assert(readFile(dst.path) == destinationAfter);
        cleanup(root, {src.path, dst.path});
    }

    // Acknowledged evidence that becomes stale after persistence cannot authorize retirement.
    {
        const std::string root = base + "-stale";
        ensureDir(root);
        FileStore src(srcDesc, root + ".src", sourceBefore);
        FileStore dst(dstDesc, root + ".dst", destinationBefore);
        auto tx = makeTransaction("tx-0000000000002023", src.descriptor(), dst.descriptor());
        EvidenceStore evidenceStore(root);
        std::string error;

        Engine engine(root, {}, makeEvidenceRetirementGate(evidenceStore, true));
        assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto evidence = makeEvidence(tx);
        evidence.fidelity.addLoss(Loss::HeldItemDropped);
        evidence.lossesShownToUser = true;
        assert(markLossesAcknowledged(evidence, 1001, error));
        assert(evidenceStore.persist(evidence, error));

        evidence.fidelity.addAdaptation(Adaptation::MovePPClamped);
        assert(evidenceStore.persist(evidence, error)); // integrity valid, acknowledgement now stale
        const auto rr = engine.recover(tx.id, src, dst);
        assert(rr.status == RecoveryStatus::Failed);
        assert(rr.error.find("stale") != std::string::npos);
        assertSourceSafe(src, sourceBefore);
        cleanup(root, {src.path, dst.path});
    }

    // If interruption occurs after SOURCE_RETIRE_PENDING is durable, every restart rechecks
    // evidence before touching the source. Removing or corrupting evidence leaves source intact.
    for (int mode = 0; mode < 2; ++mode) {
        const std::string root = base + (mode == 0 ? "-pending-missing" : "-pending-corrupt");
        ensureDir(root);
        FileStore src(srcDesc, root + ".src", sourceBefore);
        FileStore dst(dstDesc, root + ".dst", destinationBefore);
        auto tx = makeTransaction(mode == 0
            ? "tx-0000000000002024"
            : "tx-0000000000002025", src.descriptor(), dst.descriptor());
        EvidenceStore evidenceStore(root);
        std::string error;
        auto evidence = makeEvidence(tx);

        Engine first(root, {}, makeEvidenceRetirementGate(evidenceStore, true));
        assert(first.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        assert(evidenceStore.persist(evidence, error));
        const auto interrupted = first.recover(tx.id, src, dst, FaultPoint::BeforeSourceRetire);
        assert(interrupted.status == RecoveryStatus::Interrupted);
        assert(interrupted.state == State::SourceRetirePending);
        assertSourceSafe(src, sourceBefore);

        const std::string path = evidenceStore.evidencePath(tx.id);
        if (mode == 0) {
            assert(std::remove(path.c_str()) == 0);
        } else {
            mutateByte(path, 40, 0xa5);
        }

        Engine restarted(root, {}, makeEvidenceRetirementGate(EvidenceStore(root), true));
        const auto refused = restarted.recover(tx.id, src, dst);
        assert(refused.status == RecoveryStatus::Failed);
        assert(refused.state == State::SourceRetirePending);
        assertSourceSafe(src, sourceBefore);
        cleanup(root, {src.path, dst.path});
    }

    // Once SOURCE_RETIRED is durable, evidence must still exist before COMMITTED can be
    // recorded. Missing history leaves the transaction unresolved for manual reconciliation.
    {
        const std::string root = base + "-retired-missing-evidence";
        ensureDir(root);
        FileStore src(srcDesc, root + ".src", sourceBefore);
        FileStore dst(dstDesc, root + ".dst", destinationBefore);
        auto tx = makeTransaction("tx-0000000000002026", src.descriptor(), dst.descriptor());
        EvidenceStore evidenceStore(root);
        std::string error;
        Engine first(root, {}, makeEvidenceRetirementGate(evidenceStore, true));
        assert(first.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
        auto evidence = makeEvidence(tx);
        assert(evidenceStore.persist(evidence, error));
        const auto interrupted =
            first.recover(tx.id, src, dst, FaultPoint::AfterSourceRetiredJournal);
        assert(interrupted.status == RecoveryStatus::Interrupted);
        assert(interrupted.state == State::SourceRetired);
        assert(readFile(src.path) == sourceRetired);
        assert(std::remove(evidenceStore.evidencePath(tx.id).c_str()) == 0);

        Engine restarted(root, {}, makeEvidenceRetirementGate(EvidenceStore(root), true));
        const auto refused = restarted.recover(tx.id, src, dst);
        assert(refused.status == RecoveryStatus::Failed);
        assert(refused.state == State::SourceRetired);
        assert(readFile(src.path) == sourceRetired);
        assert(readFile(dst.path) == destinationAfter);
        assert(restarted.journal().load(tx.id).transaction.state == State::SourceRetired);
        cleanup(root, {src.path, dst.path});
    }

    // Cross-game evidence is carried through every existing crash boundary. The true gate is
    // enabled only inside this test harness to prove restart mechanics; product policy stays false.
    {
        const std::vector<FaultPoint> faults{
            FaultPoint::AfterPreparedJournal,
            FaultPoint::AfterDestinationWrite,
            FaultPoint::AfterDestinationVerify,
            FaultPoint::AfterDestinationVerifiedJournal,
            FaultPoint::BeforeSourceRetire,
            FaultPoint::AfterSourceWrite,
            FaultPoint::AfterSourceVerify,
            FaultPoint::AfterSourceRetiredJournal,
            FaultPoint::BeforeCommitted,
        };
        for (size_t i = 0; i < faults.size(); ++i) {
            const std::string root = base + "-fault-" + std::to_string(i);
            ensureDir(root);
            FileStore src(srcDesc, root + ".src", sourceBefore);
            FileStore dst(dstDesc, root + ".dst", destinationBefore);
            auto tx = makeTransaction("tx-00000000000021" + std::to_string(10 + i),
                                      src.descriptor(), dst.descriptor());
            EvidenceStore evidenceStore(root);
            std::string error;
            auto evidence = makeEvidence(tx);

            Engine engine(root, {}, makeEvidenceRetirementGate(evidenceStore, true));
            assert(engine.prepare(tx, src, dst, destinationAfter, sourceRetired, error));
            assert(evidenceStore.persist(evidence, error));
            const auto interrupted = engine.recover(tx.id, src, dst, faults[i]);
            assert(interrupted.status == RecoveryStatus::Interrupted);
            Engine restarted(root, {}, makeEvidenceRetirementGate(EvidenceStore(root), true));
            assert(restarted.recover(tx.id, src, dst).status == RecoveryStatus::Committed);
            assert(readFile(src.path) == sourceRetired);
            assert(readFile(dst.path) == destinationAfter);
            cleanup(root, {src.path, dst.path});
        }
    }


    // Exact SWSH <-> S/V title identities are part of the acknowledgement binding even though
    // Sword/Shield share PK8 and Scarlet/Violet share PK9. Prove all eight current-store pairs.
    {
        struct ExactEvidenceRoute {
            const char* sourceGame;
            const char* destinationGame;
            Enums::GameVersion sourceFormat;
            Enums::GameVersion destinationFormat;
            uint8_t historicalOrigin;
        };
        const ExactEvidenceRoute routes[] = {
            {"sword_switch",   "scarlet_switch", Enums::GameVersion::SWSH, Enums::GameVersion::SV,   static_cast<uint8_t>(Enums::GameVersion::SW)},
            {"sword_switch",   "violet_switch",  Enums::GameVersion::SWSH, Enums::GameVersion::SV,   static_cast<uint8_t>(Enums::GameVersion::SW)},
            {"shield_switch",  "scarlet_switch", Enums::GameVersion::SWSH, Enums::GameVersion::SV,   static_cast<uint8_t>(Enums::GameVersion::SH)},
            {"shield_switch",  "violet_switch",  Enums::GameVersion::SWSH, Enums::GameVersion::SV,   static_cast<uint8_t>(Enums::GameVersion::SH)},
            {"scarlet_switch", "sword_switch",   Enums::GameVersion::SV,   Enums::GameVersion::SWSH, static_cast<uint8_t>(Enums::GameVersion::SL)},
            {"scarlet_switch", "shield_switch",  Enums::GameVersion::SV,   Enums::GameVersion::SWSH, static_cast<uint8_t>(Enums::GameVersion::SL)},
            {"violet_switch",  "sword_switch",   Enums::GameVersion::SV,   Enums::GameVersion::SWSH, static_cast<uint8_t>(Enums::GameVersion::VL)},
            {"violet_switch",  "shield_switch",  Enums::GameVersion::SV,   Enums::GameVersion::SWSH, static_cast<uint8_t>(Enums::GameVersion::VL)},
        };

        for (size_t i = 0; i < std::size(routes); ++i) {
            const auto& route = routes[i];
            const auto src = workspaceDescriptor(route.sourceGame, sourceProfile);
            const auto dst = workspaceDescriptor(route.destinationGame, destinationProfile);
            char id[32];
            std::snprintf(id, sizeof(id), "tx-%016llx",
                          static_cast<unsigned long long>(0x3000u + i));
            auto tx = makeTransaction(id, src, dst);
            auto evidence = makeEvidence(tx);
            evidence.sourceGameIdentity = route.sourceGame;
            evidence.destinationGameIdentity = route.destinationGame;
            evidence.sourceFormat = route.sourceFormat;
            evidence.destinationFormat = route.destinationFormat;
            evidence.historicalOriginVersion = route.historicalOrigin;
            evidence.fidelity.sourceOriginVersion = route.historicalOrigin;
            evidence.fidelity.destinationEntityOriginVersion = route.historicalOrigin;

            // Both directions can be loss-bearing: entering S/V can drop source-only game state;
            // returning to SWSH necessarily drops Tera. This exercises the exact acknowledgement
            // binding without changing product route policy.
            if (route.sourceFormat == Enums::GameVersion::SWSH) {
                evidence.fidelity.addLoss(Loss::DivergentGameDataDropped);
                evidence.fidelity.addAdaptation(Adaptation::TargetDefaultTeraSynthesized);
                evidence.fidelity.addAdaptation(Adaptation::TargetScaleSynthesized);
                evidence.fidelity.addAdaptation(Adaptation::TargetObedienceLevelSynthesized);
            } else {
                evidence.fidelity.addLoss(Loss::TeraDataDropped);
            }
            evidence.lossesShownToUser = true;
            std::string error;
            assert(markLossesAcknowledged(evidence, 2000 + i, error));
            assert(evidence.lossPolicySatisfied());

            const auto harnessEnabled = authorizeSourceRetirement(evidence, tx, true);
            assert(harnessEnabled.allowed);
            const auto productDisabled = authorizeSourceRetirement(evidence, tx, false);
            assert(!productDisabled.allowed);
            assert(productDisabled.reason.find("route is disabled") != std::string::npos);
            assert(!routeEnabledForTrueMove(evidence));

            // Exact title/store identity is cryptographically bound. Re-labeling Sword as Shield,
            // Scarlet as Violet, or changing either candidate hash invalidates the acknowledgement.
            auto staleGame = evidence;
            staleGame.destinationGameIdentity =
                std::string(route.destinationGame) == "scarlet_switch" ? "violet_switch" : "scarlet_switch";
            staleGame.destinationStore.gameId = staleGame.destinationGameIdentity;
            assert(!authorizeSourceRetirement(staleGame, tx, true).allowed);

            auto staleSource = evidence;
            staleSource.sourcePayload[0] ^= 0x01;
            assert(!authorizeSourceRetirement(staleSource, tx, true).allowed);

            auto staleDestination = evidence;
            staleDestination.destinationPayload[0] ^= 0x01;
            assert(!authorizeSourceRetirement(staleDestination, tx, true).allowed);

            const auto provenance = makeProvenanceNode(evidence, static_cast<uint64_t>(i + 1));
            assert(provenance.historicalOriginVersion == route.historicalOrigin);
            assert(provenance.sourceGameIdentity == route.sourceGame);
            assert(provenance.destinationGameIdentity == route.destinationGame);
            assert(provenance.sourceStore.gameId == route.sourceGame);
            assert(provenance.destinationStore.gameId == route.destinationGame);
        }
    }

    std::cout << "Declared-loss acknowledgement + F13 persisted provenance: PASS\n";
}
