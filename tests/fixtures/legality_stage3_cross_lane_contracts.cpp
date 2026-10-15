// Stage 3 audit-owned contracts.  Header-only lane contracts are exercised
// alongside the production analyzer on a synthetic encrypted PK4.  These are
// conditional-history tests, not assertions of retail provenance.
#include "Encryption/Encryption4.h"
#include "Legality/Gen14CandidateMoveHistory.h"
#include "Legality/Gen14CandidateSetCoverage.h"
#include "Legality/Gen34SameParentEggHistory.h"
#include "Legality/Gen4EventJointMoveHistory.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {
namespace Move = Legality::Gen14CandidateMoveHistory;
namespace Set = Legality::Gen14CandidateSetCoverage;
namespace Egg = Legality::Gen34SameParentEggHistory;
namespace Event = Legality::Gen4EventJointMoveHistory;

void put16(std::vector<std::byte>& bytes, size_t offset, uint16_t value) {
    bytes[offset] = static_cast<std::byte>(value);
    bytes[offset + 1] = static_cast<std::byte>(value >> 8);
}
void put32(std::vector<std::byte>& bytes, size_t offset, uint32_t value) {
    put16(bytes, offset, static_cast<uint16_t>(value));
    put16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
Move::Slot levelUpSlot(uint32_t candidate, uint16_t move) {
    Move::Slot slot{};
    slot.move = move;
    slot.exhaustive = true;
    slot.count = 1;
    slot.options[0] = {move, candidate, Move::Method::LevelUp, 5, 100, 1, 0, 0, true};
    return slot;
}
bool has(const Legality::Report& report, Legality::Severity severity,
         std::string_view text = {}) {
    for (const auto& issue : report.issues)
        if (issue.severity == severity &&
            (text.empty() || issue.text.find(text) != std::string::npos)) return true;
    return false;
}

void testEncounterMoveCandidateContract(unsigned& checks) {
    Set::Input input{};
    input.count = 2;
    input.allOriginalEncounterCandidatesEnumerated = true;
    for (uint8_t index = 0; index != input.count; ++index) {
        auto& entry = input.alternatives[index];
        entry.originalEncounterIndependentlyGrounded = true;
        entry.candidate = {static_cast<uint32_t>(100 + index), 25, 5, true, 4};
        entry.moves[0] = levelUpSlot(entry.candidate.id, 33);
    }
    // A successful second source candidate remains conditional even when the
    // first candidate's move witness contradicts only that candidate.
    input.alternatives[0].moves[0].options[0].candidateId = 999;
    auto result = Set::evaluate(input);
    assert(result.status == Set::Status::OneConditionalRoute &&
           result.compatible == 1 && result.contradicted == 1);
    checks += 3;
    // With both candidates contradicted, exhaustive original-source evidence
    // is the only condition that permits the stronger all-candidates result.
    input.alternatives[1].moves[0].options[0].candidateId = 999;
    result = Set::evaluate(input);
    assert(result.status == Set::Status::AllGroundedCandidatesContradicted);
    input.allOriginalEncounterCandidatesEnumerated = false;
    result = Set::evaluate(input);
    assert(result.status == Set::Status::UnknownSourceCandidateEnumeration);
    checks += 2;
}

Egg::Input eggInput() {
    return {"firered_gba", 1, 1001, 1002, true,
            {33, 45, 0, 0}, {80, 345, 15, 33}, {33, 45, 0, 0},
            {false, false, true, false}, {false, false, false, true},
            true, false, {80, 345, 15, 33}, 1, 1,
            Egg::PairProof::IndependentlyCompatible};
}
void testBreedingMoveContract(unsigned& checks) {
    const auto compatible = Egg::evaluate(eggInput());
    assert(compatible.status == Egg::Status::CandidateCompatible &&
           compatible.eggSources == 2 && compatible.machineSources == 1);
    auto contradictory = eggInput();
    contradictory.father = {80, 15, 33, 0};
    contradictory.verifiedMachine = {false, true, false, false};
    contradictory.verifiedSharedLevel = {false, false, true, false};
    assert(Egg::evaluate(contradictory).status == Egg::Status::CandidateMismatch);
    auto incomplete = contradictory;
    incomplete.fatherId = 0;
    assert(Egg::evaluate(incomplete).status == Egg::Status::UnknownParents);
    // A Gen-IV-only move in a purported Gen-III breeding snapshot excludes
    // the proposed pair, but is not a whole-Pokémon Invalid verdict.
    auto outOfDomain = eggInput();
    outOfDomain.game = "ruby_gba";
    outOfDomain.father[1] = 437;
    assert(Egg::evaluate(outOfDomain).status == Egg::Status::CandidateMismatch);
    checks += 7;
}

Event::Input eventInput() {
    Event::Input input{};
    input.stored = {4, 7200, 58364, 0x0f199b50u, 40, 3053, 16, 0, 0, 1, 7, 0, true};
    input.claimedCardId = 67;
    input.initialMovesIndependentlyVerified = true;
    input.initialMoves = {10, 52, 0, 0};
    input.observedMoves = {10, 52, 53, 0};
    input.history = {1001, 60, 40, true, 5};
    input.laterRoutes[2].move = 53;
    input.laterRoutes[2].count = 1;
    input.laterRoutes[2].exhaustive = true;
    input.laterRoutes[2].options[0] = {53, 1001, Move::Method::Tutor, 40, 60, 2, 0, 0, true};
    return input;
}
void testEventTransferMoveContract(unsigned& checks) {
    const auto compatible = Event::evaluate(eventInput());
    assert(compatible.status == Event::Status::CompatibleCandidate &&
           compatible.joint.coveredSlots == 3);
    auto swapped = eventInput();
    swapped.laterRoutes[2].options[0].method = Move::Method::Event;
    swapped.laterRoutes[2].options[0].eventId = 68;
    assert(Event::evaluate(swapped).status == Event::Status::CandidateContradicted);
    auto incomplete = eventInput();
    incomplete.initialMovesIndependentlyVerified = false;
    assert(Event::evaluate(incomplete).status == Event::Status::UnknownInitialMoves);
    checks += 5;
}

std::vector<std::byte> nativePk4(uint32_t pid) {
    std::vector<std::byte> plain(Encryption::SIZE_STORED4, std::byte{0});
    put32(plain, 0x00, pid);
    put16(plain, 0x08, 1); // Bulbasaur
    put16(plain, 0x0c, 12345);
    put16(plain, 0x0e, 54321);
    put32(plain, 0x10, 135);
    plain[0x14] = std::byte{70};
    plain[0x15] = std::byte{65}; // Overgrow
    plain[0x17] = std::byte{2};
    put16(plain, 0x28, 33); // Tackle
    plain[0x30] = std::byte{35};
    plain[0x5f] = std::byte{12}; // Platinum stored origin
    plain[0x83] = std::byte{4};
    plain[0x84] = std::byte{5};
    put16(plain, 0x06, Encryption::checksum4(plain));
    return Encryption::encryptArray4(plain);
}
void testProductionAnalyzerContract(unsigned& checks) {
    // Bounded real-analyzer property sweep: native PID/nature variation must
    // neither manufacture provenance nor convert a source-label mismatch
    // into an unrelated invalidity.
    for (uint32_t variant = 0; variant != 32; ++variant) {
        const auto encrypted = nativePk4(0x12345678u + variant * 2u);
        const auto immutable = encrypted;
        Pokemon::Pokemon4ReadOnly record(encrypted, Enums::GameVersion::PT);
        assert(record.valid() && !record.empty());
        Pokemon::Pokemon4ReadOnlyView view(record);
        const auto sourceFree = Legality::analyze(view, Enums::GameVersion::PT);
        assert(!sourceFree.hasInvalid() && sourceFree.verdict() == Legality::Verdict::Incomplete);
        // A wrong external source label must be downgraded to a warning and may
        // not manufacture an unrelated Gen-III candidate contradiction.
        const auto mismatched = Legality::analyze(view, Enums::GameVersion::PT, "firered_gba");
        assert(!mismatched.hasInvalid() &&
               has(mismatched, Legality::Severity::Warning, "source-game label conflicts") &&
               mismatched.verdict() == Legality::Verdict::Incomplete);
        assert(encrypted == immutable);
        checks += 4;
    }
}
}

int main() {
    unsigned checks = 0;
    testEncounterMoveCandidateContract(checks);
    testBreedingMoveContract(checks);
    testEventTransferMoveContract(checks);
    testProductionAnalyzerContract(checks);
    assert(checks == 145u);
    std::cout << "Stage 3 cross-lane contracts: " << checks
              << " PASS (conditional evidence remains non-authenticating)\n";
}
