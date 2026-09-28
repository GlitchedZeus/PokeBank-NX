#include "Utils/MoveTransactionProduction.h"

#include <cassert>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readText(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

struct BidirectionalStore final : PokeBank::Storage::MoveTx::Store {
    PokeBank::Storage::MoveTx::StoreDescriptor d;
    std::string path;

    BidirectionalStore(PokeBank::Storage::MoveTx::StoreDescriptor descriptor,
                       std::string p,
                       std::span<const uint8_t> initial)
        : d(std::move(descriptor)), path(std::move(p)) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        assert(out);
        out.write(reinterpret_cast<const char*>(initial.data()),
                  static_cast<std::streamsize>(initial.size()));
        assert(out.good());
    }

    const PokeBank::Storage::MoveTx::StoreDescriptor& descriptor() const noexcept override {
        return d;
    }
    bool read(std::vector<uint8_t>& out, std::string& error) const override {
        std::ifstream in(path, std::ios::binary);
        if (!in) { error = "read failed"; return false; }
        out.assign(std::istreambuf_iterator<char>(in), {});
        return true;
    }
    bool validate(std::span<const uint8_t> bytes, std::string& error) const override {
        if (bytes.empty()) { error = "empty image"; return false; }
        return true;
    }
    bool replace(std::span<const uint8_t> bytes, std::string& error) override {
        auto validator = [&](std::span<const uint8_t> candidate, std::string& e) {
            return validate(candidate, e);
        };
        const auto result = PokeBank::Storage::DurableFile::replace(path, bytes, validator);
        if (!result.ok) { error = result.error; return false; }
        return true;
    }
};

std::vector<uint8_t> readStore(const BidirectionalStore& store) {
    std::vector<uint8_t> out;
    std::string error;
    assert(store.read(out, error));
    return out;
}

}

int main() {
    using namespace PokeBank::Storage::MoveTx::Production;

    // Only the A09-hardened single-file Switch workspaces may participate in A04b.
    assert(expectedStaticFileId("letsgo_pikachu_switch") == "savedata.bin");
    assert(expectedStaticFileId("letsgo_eevee_switch") == "savedata.bin");
    assert(expectedStaticFileId("sword_switch") == "main");
    assert(expectedStaticFileId("shield_switch") == "main");
    assert(expectedStaticFileId("legends_arceus_switch") == "main");
    assert(expectedStaticFileId("scarlet_switch") == "main");
    assert(expectedStaticFileId("violet_switch") == "main");
    assert(expectedStaticFileId("legends_za_switch") == "main");
    assert(expectedStaticFileId("firered_switch").empty());   // resolved from the exact workspace
    assert(expectedStaticFileId("leafgreen_switch").empty());
    assert(!supportsSingleFileTrueMove("brilliant_diamond_switch"));
    assert(!supportsSingleFileTrueMove("shining_pearl_switch"));
    assert(!supportsSingleFileTrueMove("firered_gba"));

    // Missing Bank is a semantic pre-state distinct from an established-but-empty bank.dat.
    assert(!bankMissingSentinel().empty());
    assert(isBankMissingSentinel(bankMissingSentinel()));

    // Product policy: unrelated dirty state and occupied destinations fail before durable mutation.
    Eligibility clean{};
    clean.workspaceDirtyBeforePickup = false;
    clean.bankDirtyBeforePickup = false;
    clean.editorDirtyBeforePickup = false;
    clean.destinationOccupied = false;
    clean.destinationLocked = false;
    clean.crossGameConversion = false;
    clean.copyOperation = false;
    assert(eligibleForTrueMove(clean));

    Eligibility dirty = clean;
    dirty.workspaceDirtyBeforePickup = true;
    assert(!eligibleForTrueMove(dirty));

    Eligibility occupied = clean;
    occupied.destinationOccupied = true;
    assert(!eligibleForTrueMove(occupied));

    Eligibility crossGame = clean;
    crossGame.crossGameConversion = true;
    assert(!eligibleForTrueMove(crossGame));

    Eligibility copy = clean;
    copy.copyOperation = true;
    assert(!eligibleForTrueMove(copy));

    const std::string header = readText("include/UI/TrainerViewScreenBase.h");
    const std::string source = readText("src/UI/TrainerViewScreenBase.inc");
    const std::string ui = readText("src/UI/UI.cpp");

    // A04b must be an explicit durable cross-store coordinator, not a hidden change to A03.
    assert(header.find("captureCrossStoreMoveBaseline") != std::string::npos);
    assert(header.find("tryCommitCrossStoreMove") != std::string::npos);
    assert(source.find("tryCommitCrossStoreMove") != std::string::npos);
    assert(source.find("MOVE COMMITTED") != std::string::npos);
    assert(source.find("Save or discard existing Bank/workspace edits before a cross-store Move.") !=
           std::string::npos);
    assert(source.find("Cross-store true Move requires empty destination slots.") !=
           std::string::npos);
    assert(source.find("Cross-game true Move is locked until conversion preservation is proven.") !=
           std::string::npos);
    assert(source.find("engine.prepare(") != std::string::npos);
    assert(source.find("engine.recover(") != std::string::npos);

    // Successful true Move establishes a new committed baseline instead of leaving legacy
    // Save/Discard snapshots capable of undoing one side of the transaction.
    const auto bankReload = source.find("bank->load()");
    const auto committedStatus = source.find("MOVE COMMITTED");
    assert(bankReload != std::string::npos);
    assert(committedStatus != std::string::npos);
    assert(bankReload < committedStatus);
    assert(source.find("hasUnsavedChanges = false") != std::string::npos);

    // Pickup is staging only. Returning the held object restores the dirty state that existed
    // before pickup; it must not invent a dirty workspace just because a Move was cancelled.
    assert(source.find("crossStoreBaseline.workspaceDirtyBeforePickup") != std::string::npos);
    assert(source.find("hasUnsavedChanges = crossStoreBaseline.workspaceDirtyBeforePickup") !=
           std::string::npos);

    // Startup/session open must run transaction recovery before parsing the workspace.
    const auto recovery = ui.find("recoverPendingMoveTransactions");
    const auto open = ui.find("validateTrainerSaveForOpen");
    assert(recovery != std::string::npos);
    assert(open != std::string::npos);
    assert(recovery < open);

    // Live-title injection must remain outside the transaction integration.
    const std::string prod = readText("src/Utils/MoveTransactionProduction.cpp");
    assert(prod.find("restoreBackupToTitle") == std::string::npos);
    assert(prod.find("injectToTitle") == std::string::npos);

    const std::string save = readText("src/Save/GetSaveFileContents.cpp");
    assert(save.find("buildWorkspaceImage(") != std::string::npos);
    assert(save.find("persistWorkspaceImage(") != std::string::npos);
    assert(save.find("BDSP true Move is blocked") != std::string::npos);

    const std::string bank = readText("src/Trainer/Bank.cpp");
    assert(bank.find("buildVerifiedImage(") != std::string::npos);
    assert(bank.find("validateStorageImage(") != std::string::npos);

    // A04b bidirectional serialization: the reverse Move may start only after the first
    // transaction commits, and its before-fingerprints must be the first transaction's post-state.
    {
        using namespace PokeBank::Storage::MoveTx;
        const std::string base = "/tmp/pokebank-a04b-bidir-" +
            std::to_string(static_cast<long long>(getpid()));
        ::mkdir(base.c_str(), 0777);

        StoreDescriptor bankDesc;
        bankDesc.type = StoreType::Bank;
        bankDesc.fileId = "bank.dat";

        StoreDescriptor wsDesc;
        wsDesc.type = StoreType::MutableWorkspaceSingleFile;
        wsDesc.profile = "account-11111111111111112222222222222222";
        wsDesc.gameId = "sword_switch";
        wsDesc.workspace = "Working";
        wsDesc.fileId = "main";

        const std::vector<uint8_t> bank0{1,2,3,4};
        const std::vector<uint8_t> workspace0{8,8,8,8};
        const std::vector<uint8_t> bank1{1,2,3,0};
        const std::vector<uint8_t> workspace1{8,8,8,8,25};
        const std::vector<uint8_t> bank2{1,2,3,0,133};
        const std::vector<uint8_t> workspace2{8,8,8,8,25,0};

        BidirectionalStore bank(bankDesc, base + "/bank.dat", bank0);
        BidirectionalStore workspace(wsDesc, base + "/main", workspace0);
        Engine engine(base + "/transactions");

        Transaction toWorkspace;
        toWorkspace.id = "tx-0000000000001001";
        toWorkspace.source = bankDesc;
        toWorkspace.destination = wsDesc;
        MoveRecord firstMove;
        firstMove.sourceSlot = "0:0";
        firstMove.destinationSlot = "0:0";
        firstMove.species = 25;
        firstMove.sourcePayload = sha256(bank0);
        firstMove.destinationPayload = sha256(workspace1);
        toWorkspace.moves.push_back(firstMove);
        std::string error;
        assert(engine.prepare(toWorkspace, bank, workspace, workspace1, bank1, error));
        assert(engine.recover(toWorkspace.id, bank, workspace).status == RecoveryStatus::Committed);
        assert(readStore(bank) == bank1);
        assert(readStore(workspace) == workspace1);

        Transaction backToBank;
        backToBank.id = "tx-0000000000001002";
        backToBank.source = wsDesc;
        backToBank.destination = bankDesc;
        MoveRecord secondMove;
        secondMove.sourceSlot = "0:1";
        secondMove.destinationSlot = "0:1";
        secondMove.species = 133;
        secondMove.sourcePayload = sha256(workspace1);
        secondMove.destinationPayload = sha256(bank2);
        backToBank.moves.push_back(secondMove);
        assert(engine.prepare(backToBank, workspace, bank, bank2, workspace2, error));

        const auto loaded = engine.journal().load(backToBank.id);
        assert(loaded.status == LoadStatus::Ok);
        assert(loaded.transaction.sourceBefore == sha256(workspace1));
        assert(loaded.transaction.destinationBefore == sha256(bank1));
        assert(engine.recover(backToBank.id, workspace, bank).status == RecoveryStatus::Committed);
        assert(readStore(bank) == bank2);
        assert(readStore(workspace) == workspace2);

        std::remove((base + "/bank.dat").c_str());
        std::remove((base + "/main").c_str());
        // Transaction evidence is intentionally retained in product code; test cleanup is best-effort.
        std::cout << "A04b bidirectional serialization: PASS\n";
    }

    std::cout << "A04b production true-Move contract: PASS\n";
}
