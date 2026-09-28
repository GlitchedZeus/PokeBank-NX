#include "UI/StoragePlacementCommit.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

struct TrackedPokemon {
    explicit TrackedPokemon(int value) : id(value) {}
    int id;

    inline static const std::unique_ptr<TrackedPokemon>* destinationProbe = nullptr;
    inline static bool originalRetiredAfterCandidateOwned = false;

    ~TrackedPokemon() {
        if (id == 1 && destinationProbe) {
            const auto* current = destinationProbe->get();
            originalRetiredAfterCandidateOwned = current && current->id == 3;
        }
    }
};
}

int main() {
    using PokeBank::UIModel::commitNativeSwap;
    using PokeBank::UIModel::commitPreparedCandidate;

    // Executable commit-order regression: an occupied destination receives candidate A'
    // before authoritative original A is destroyed/replaced by displaced B.
    {
        auto custody = std::make_unique<TrackedPokemon>(1);
        auto destination = std::make_unique<TrackedPokemon>(2);
        auto candidate = std::make_unique<TrackedPokemon>(3);
        TrackedPokemon* displacedIdentity = destination.get();

        TrackedPokemon::destinationProbe = &destination;
        TrackedPokemon::originalRetiredAfterCandidateOwned = false;
        commitPreparedCandidate(custody, destination, candidate);

        assert(!candidate);
        assert(destination && destination->id == 3);
        assert(custody && custody.get() == displacedIdentity && custody->id == 2);
        assert(TrackedPokemon::originalRetiredAfterCandidateOwned);
        TrackedPokemon::destinationProbe = nullptr;
    }

    // Empty destination: candidate becomes authoritative first, then held custody clears.
    {
        auto custody = std::make_unique<TrackedPokemon>(1);
        std::unique_ptr<TrackedPokemon> destination;
        auto candidate = std::make_unique<TrackedPokemon>(3);

        TrackedPokemon::destinationProbe = &destination;
        TrackedPokemon::originalRetiredAfterCandidateOwned = false;
        commitPreparedCandidate(custody, destination, candidate);

        assert(destination && destination->id == 3);
        assert(!custody);
        assert(TrackedPokemon::originalRetiredAfterCandidateOwned);
        TrackedPokemon::destinationProbe = nullptr;
    }

    // Bank path: the exact native object changes location. No candidate/clone replaces it.
    {
        auto custody = std::make_unique<TrackedPokemon>(10);
        auto destination = std::make_unique<TrackedPokemon>(20);
        TrackedPokemon* originalIdentity = custody.get();
        TrackedPokemon* displacedIdentity = destination.get();

        commitNativeSwap(custody, destination);

        assert(destination.get() == originalIdentity);
        assert(custody.get() == displacedIdentity);
        assert(destination->id == 10);
        assert(custody->id == 20);
    }

    const std::string header = read("include/UI/TrainerViewScreenBase.h");
    const std::string source = read("src/UI/TrainerViewScreenBase.inc");

    // A03 architecture: adaptation is a separate prepared candidate. The carried
    // moveMon object is authoritative custody until the destination commit.
    assert(header.find("struct PreparedPlacement") != std::string::npos);
    assert(header.find("prepareForPane(") != std::string::npos);
    assert(header.find("const Pokemon::Pokemon& original") != std::string::npos);
    assert(header.find("bool convertForPane(std::unique_ptr<Pokemon::Pokemon>&") ==
           std::string::npos);

    const auto prepareBegin = source.find("TrainerViewScreen::prepareForPane(");
    const auto prepareEnd = source.find(
        "bool TrainerViewScreen::lgpeConversionInvolved", prepareBegin);
    assert(prepareBegin != std::string::npos);
    assert(prepareEnd != std::string::npos && prepareEnd > prepareBegin);
    const std::string prepare = source.substr(prepareBegin, prepareEnd - prepareBegin);

    // Bank placement keeps the native payload and moves it only at commit.
    assert(prepare.find("useOriginal = true") != std::string::npos);

    // Same-game destination repair operates only on a cloned candidate.
    assert(prepare.find("original.clone()") != std::string::npos);
    assert(prepare.find("normalizeAffixedRibbon(*candidate)") != std::string::npos);
    assert(prepare.find("candidate->refreshChecksum()") != std::string::npos);

    // Cross-game conversion consumes a const source and returns a separate candidate.
    assert(prepare.find("Conversion::convert(") != std::string::npos);
    assert(prepare.find("original, trainer.getGameGroup()") != std::string::npos);
    assert(prepare.find("pk = std::move(converted)") == std::string::npos);

    const auto putBegin = source.find("void TrainerViewScreen::putDownBlock()");
    const auto putEnd = source.find(
        "void TrainerViewScreen::storagePickup()", putBegin);
    assert(putBegin != std::string::npos);
    assert(putEnd != std::string::npos && putEnd > putBegin);
    const std::string put = source.substr(putBegin, putEnd - putBegin);

    assert(put.find("prepareForPane(*moveMon[idx], pane)") != std::string::npos);
    assert(put.find("prepared.useOriginal") != std::string::npos);
    assert(put.find("commitNativeSwap(moveMon[idx], dst)") != std::string::npos);
    assert(put.find("commitPreparedCandidate(") != std::string::npos);
    assert(put.find("moveMon[idx], dst, prepared.candidate") != std::string::npos);

    // Locked slots are rejected before candidate preparation.
    const auto lockedGuard = put.find("if (storageSlotLocked(pane, box, dSlot))");
    const auto prepareCall = put.find("PreparedPlacement prepared = prepareForPane");
    assert(lockedGuard != std::string::npos);
    assert(prepareCall != std::string::npos);
    assert(lockedGuard < prepareCall);

    // A failed candidate is left in authoritative custody and the cell continues
    // without executing either commit primitive.
    const auto failedGuard = put.find("if (!prepared.ready())");
    const auto failedContinue = put.find("continue;", failedGuard);
    const auto firstCommit = put.find("commitNativeSwap", failedGuard);
    assert(failedGuard != std::string::npos);
    assert(failedContinue != std::string::npos);
    assert(firstCommit != std::string::npos);
    assert(failedGuard < failedContinue && failedContinue < firstCommit);

    // Warnings/cancel occur before putDownBlock, so no hidden candidate is created
    // until the user actually continues the pending placement.
    const auto inputBegin = source.find("void TrainerViewScreen::handleStorageInput");
    const auto inputEnd = source.find("void TrainerViewScreen::update(", inputBegin);
    assert(inputBegin != std::string::npos);
    assert(inputEnd != std::string::npos && inputEnd > inputBegin);
    const std::string input = source.substr(inputBegin, inputEnd - inputBegin);
    const auto warning = input.find("if (g3warn || lgwarn)");
    const auto pending = input.find("pendingMove = PendingMove::PlaceHeld", warning);
    const auto warningReturn = input.find("return;", pending);
    const auto directPutDown = input.find("putDownBlock();", warningReturn);
    assert(warning != std::string::npos);
    assert(pending != std::string::npos);
    assert(warningReturn != std::string::npos);
    assert(directPutDown != std::string::npos);
    assert(warning < pending && pending < warningReturn && warningReturn < directPutDown);

    const auto confirmBegin = source.find("if (moveConfirmActive())");
    const auto confirmEnd = source.find("// Shared Party / Boxes / Storage action sheet", confirmBegin);
    assert(confirmBegin != std::string::npos);
    assert(confirmEnd != std::string::npos && confirmEnd > confirmBegin);
    const std::string confirm = source.substr(confirmBegin, confirmEnd - confirmBegin);
    assert(confirm.find("if (go && pendingMove == PendingMove::PlaceHeld)") !=
           std::string::npos);
    assert(confirm.find("putDownBlock();") != std::string::npos);

    // A02 rollback protection must remain present while A03 changes placement.
    const auto returnBegin = source.find("void TrainerViewScreen::returnHeldToOrigin()");
    const auto returnEnd = source.find(
        "std::unique_ptr<Pokemon::Pokemon>& TrainerViewScreen::storageSlot", returnBegin);
    assert(returnBegin != std::string::npos);
    assert(returnEnd != std::string::npos && returnEnd > returnBegin);
    const std::string rollback = source.substr(returnBegin, returnEnd - returnBegin);
    assert(rollback.find("if (!returnPlan.complete)") != std::string::npos);
    assert(rollback.find("custody retained") != std::string::npos);

    std::cout << "Storage conversion custody/candidate contract: PASS\n";
}
