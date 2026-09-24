#include "Utils/MoveTransactionProduction.h"

#include <cassert>
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

    std::cout << "A04b production true-Move contract: PASS\n";
}
