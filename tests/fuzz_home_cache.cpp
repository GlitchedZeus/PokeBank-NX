// QA fuzz target: the Home card snapshot cache (include/Source/HomeCardSnapshot.h HomeSnapshotCache),
// shared by the Gen IV and Gen V Home cards.
//
// Each 4-byte input op drives one refresh / invalidate / clear on a small set of cards, with a
// scripted file stamp (missing, two stamps, or a write landing during the parse) and a scripted
// parser result (Current with a party, or a validation failure). Invariants checked after every op:
//  * no assignment -> Unavailable, outcome Unassigned, and the parser is not run;
//  * missing file -> Unavailable, outcome Missing, no parse, and no old party shown;
//  * Reused only for the same source path, binding signature and an unchanged stamp that was also
//    stable during the previous parse; anything else runs the parser exactly once;
//  * a Current parser result is presented as-is; a failed one is never Current: it is Stale only
//    with the last validated data of the SAME assignment, otherwise Unavailable with a reason;
//  * find() returns what refresh() returned; invalidate()/clear() force a re-parse; parseCount()
//    counts parser runs.
#include "fuzz_support.h"

#include "Source/HomeCardSnapshot.h"

#include <array>
#include <map>
#include <optional>

namespace {

namespace S = PokeVault::Source;

struct Model {
    bool cached = false;              // an entry that may be reused exists
    std::string path, signature;      // its assignment
    S::FileStamp stamp{};             // its stamp
    bool stampTrusted = false;        // false after a write landed during the parse
    std::optional<S::HomeCardSnapshot> lastData; // last snapshot with data for that assignment
};

bool sameData(const S::HomeCardSnapshot& a, const S::HomeCardSnapshot& b) {
    if (a.gameId != b.gameId || a.trainerName != b.trainerName || a.partyCount != b.partyCount) return false;
    for (size_t i = 0; i < a.party.size(); ++i)
        if (a.party[i].species != b.party[i].species || a.party[i].level != b.party[i].level ||
            a.party[i].form != b.party[i].form || a.party[i].shiny != b.party[i].shiny)
            return false;
    return true;
}

void runOne(std::span<const uint8_t> data) {
    const std::array<std::string, 3> keys{"p1|black_nds", "p1|diamond_nds", "p2|black_nds"};
    const std::array<std::string, 3> paths{"", "sdmc:/saves/a.sav", "sdmc:/saves/b.sav"};
    const std::array<std::string, 3> signatures{"id-1\tDraStic\t", "id-1\tmelonDS\t", "id-2\tDraStic\t"};
    S::HomeSnapshotCache cache;
    std::map<std::string, Model> model;
    size_t parses = 0;

    for (size_t at = 0; at + 4 <= data.size() && at < 4 * 512; at += 4) {
        const uint8_t a = data[at], b = data[at + 1], c = data[at + 2], d = data[at + 3];
        const std::string& key = keys[a % 3];
        if ((a >> 4) == 0xF) {
            cache.clear();
            model.clear();
            QA_CHECK(cache.find(key) == nullptr);
            continue;
        }
        if ((a >> 4) == 0xE) {
            cache.invalidate(key);
            model.erase(key);
            QA_CHECK(cache.find(key) == nullptr);
            continue;
        }
        S::SnapshotRequest request;
        request.key = key;
        request.sourcePath = paths[b % 3];
        request.bindingSignature = signatures[(b >> 2) % 3];
        // Stamp script: 0 missing, 1/2 fixed stamps, 3 a write lands during the parse.
        const int stampMode = c % 4;
        const S::FileStamp s1{0x80000, 1000 + (c >> 2)}, s2{0x80000 + 0x10, 2000};
        int statCalls = 0;
        auto stat = [&](const std::string& path) -> std::optional<S::FileStamp> {
            QA_CHECK(path == request.sourcePath);
            ++statCalls;
            if (stampMode == 0) return std::nullopt;
            if (stampMode == 3) return statCalls == 1 ? s1 : s2;
            return stampMode == 1 ? s1 : s2;
        };
        // Parser script: Current with a small party, or a failed validation (with/without reason).
        int parserRuns = 0;
        S::HomeCardSnapshot parsed;
        if (d % 3 == 0) {
            parsed.state = S::SnapshotState::Current;
            parsed.gameId = key.substr(key.find('|') + 1);
            parsed.trainerName = "QA";
            parsed.partyCount = static_cast<uint8_t>(1 + (d >> 2) % 6);
            for (size_t i = 0; i < parsed.partyCount; ++i)
                parsed.party[i] = {static_cast<uint16_t>(1 + (d + i) % 649), static_cast<uint8_t>(1 + d % 100),
                                   static_cast<uint8_t>(d % 3), (d & 0x80) != 0};
        } else {
            parsed = S::unavailableSnapshot(d % 3 == 1 ? "checksum mismatch" : "");
        }
        auto parse = [&] {
            ++parserRuns;
            return parsed;
        };
        S::SnapshotRefresh outcome{};
        const S::HomeCardSnapshot& got = cache.refresh(request, stat, parse, &outcome);
        parses += static_cast<size_t>(parserRuns);
        QA_CHECK(cache.parseCount() == parses);
        QA_CHECK(cache.find(key) == &got);
        QA_CHECK(parserRuns <= 1);
        Model& m = model[key];

        if (request.sourcePath.empty()) {
            QA_CHECK(outcome == S::SnapshotRefresh::Unassigned && parserRuns == 0 && statCalls == 0);
            QA_CHECK(got.state == S::SnapshotState::Unavailable && !got.reason.empty() && got.partyCount == 0);
            m = Model{};
            continue;
        }
        if (stampMode == 0) {
            QA_CHECK(outcome == S::SnapshotRefresh::Missing && parserRuns == 0);
            QA_CHECK(got.state == S::SnapshotState::Unavailable && !got.reason.empty() && got.partyCount == 0);
            m = Model{};
            continue;
        }
        const S::FileStamp before = stampMode == 2 ? s2 : s1;
        const bool reusable = m.cached && m.stampTrusted && m.path == request.sourcePath &&
                              m.signature == request.bindingSignature && m.stamp == before;
        if (reusable) {
            QA_CHECK(outcome == S::SnapshotRefresh::Reused && parserRuns == 0);
            continue;
        }
        QA_CHECK(outcome == S::SnapshotRefresh::Parsed && parserRuns == 1);
        const bool sameAssignment = m.cached && m.path == request.sourcePath && m.signature == request.bindingSignature;
        if (parsed.state == S::SnapshotState::Current) {
            qafuzz::noteAccepted();
            QA_CHECK(got.state == S::SnapshotState::Current && got.reason.empty() && sameData(got, parsed));
            m.lastData = got;
        } else {
            QA_CHECK(got.state != S::SnapshotState::Current && !got.reason.empty());
            if (sameAssignment && m.lastData) {
                QA_CHECK(got.state == S::SnapshotState::Stale && sameData(got, *m.lastData));
            } else {
                QA_CHECK(got.state == S::SnapshotState::Unavailable && got.partyCount == 0);
                m.lastData.reset();
            }
        }
        if (!sameAssignment && parsed.state != S::SnapshotState::Current) m.lastData.reset();
        m.cached = true;
        m.path = request.sourcePath;
        m.signature = request.bindingSignature;
        m.stamp = before;
        m.stampTrusted = stampMode != 3; // a write during the parse must force the next re-parse
    }
}

std::vector<qafuzz::Bytes> seeds() {
    return {
        // assign + parse current, reuse, file rewritten (new stamp) parse current, then a failed
        // validation (stale), relink (signature change), missing file, unassigned.
        {0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x01, 0x01,
         0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00},
        // a write during the parse, then the same stamp again (must re-parse), invalidate, clear.
        {0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x02, 0x00, 0x01, 0x02, 0x02, 0x00, 0xE1, 0, 0, 0,
         0x02, 0x02, 0x01, 0x00, 0xF0, 0, 0, 0, 0x02, 0x02, 0x01, 0x00},
    };
}

} // namespace

const qafuzz::Target& qaTarget() {
    // No checksums, count bytes or copy regions: forge/countOffsets/swaps stay empty.
    static const qafuzz::Target target{"home-cache", runOne, seeds, {}, {}, {}};
    return target;
}
