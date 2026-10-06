#include "Legality/Gen4LeadHistoryEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t minimum, uint8_t maximum,
                           uint8_t rate = 0) {
    return (static_cast<uint64_t>(minimum) << 17) |
           (static_cast<uint64_t>(maximum) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
}
}

int main() {
    using namespace Legality;
    using Gen4LeadHistory::Path;
    using Gen4LeadFrame::sequentialPid;

    constexpr uint32_t depth1Seed = 4423u;
    constexpr uint32_t depth1Pid = sequentialPid(depth1Seed);
    constexpr uint64_t depth1Row = makeRow(8, 0, 7, 18, 100);
    static_assert(!Gen4LeadFailure::matchRow(
        true, depth1Row, depth1Seed, depth1Pid, 9,
        Gen4LeadFailure::Lead::Synchronize).matched());
    constexpr auto depth1 = Gen4LeadHistory::matchIndexedRow(
        true, depth1Row, 0, 18, depth1Seed, depth1Pid, 9);
    static_assert(depth1.has(Path::SynchronizeFailure));

    constexpr uint32_t depth2Seed = 1857192u;
    constexpr uint32_t depth2Pid = sequentialPid(depth2Seed);
    constexpr uint64_t depth2Row = makeRow(8, 0, 7, 18, 100);
    static_assert(!Gen4LeadFailure::matchRow(
        true, depth2Row, depth2Seed, depth2Pid, 17,
        Gen4LeadFailure::Lead::Synchronize).matched());
    constexpr auto depth2 = Gen4LeadHistory::matchIndexedRow(
        true, depth2Row, 0, 18, depth2Seed, depth2Pid, 17);
    static_assert(depth2.has(Path::SynchronizeFailure));

    constexpr uint32_t depth3Seed = 0xEEADB1AFu;
    constexpr uint32_t depth3Pid = sequentialPid(depth3Seed);
    constexpr uint64_t depth3Row = makeRow(8, 1, 7, 18, 100);
    static_assert(!Gen4LeadFrame::directMinimum31Satisfied(depth3Seed));
    static_assert(!Gen4LeadFailure::matchRow(
        true, depth3Row, depth3Seed, depth3Pid, 16,
        Gen4LeadFailure::Lead::Synchronize).matched());
    constexpr auto depth3 = Gen4LeadHistory::matchIndexedRow(
        true, depth3Row, 0, 18, depth3Seed, depth3Pid, 16);
    static_assert(depth3.has(Path::SynchronizeFailure));

    // Bug Contest is Method K/HGSS-only; the same synthetic row must not gain
    // a Synchronize-failure history through a Method J source identity.
    static_assert(!Gen4LeadHistory::matchIndexedRow(
        false, depth1Row, 0, 18, depth1Seed, depth1Pid, 9)
        .has(Path::SynchronizeFailure));

    assert(depth1.has(Path::SynchronizeFailure));
    assert(depth2.has(Path::SynchronizeFailure));
    assert(depth3.has(Path::SynchronizeFailure));

    std::cout << "Gen IV central BCC failed-Synchronize history: PASS\n";
}
