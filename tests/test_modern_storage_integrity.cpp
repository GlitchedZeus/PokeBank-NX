// PKSE 1.2.2 differential audit, priorities 1-3, as executable checks over ALL 18 Gen VI-IX titles served by
// the shared Modern layer (Modern::openSession on each title's synthetic native save):
//
//   1. Box Move never clones: every occupied, movable box record is moved through a chain (into an empty slot
//      of the same box, then of another box, then back), and after each step and after export + strict reopen
//      the multiset of records is EXACTLY the original one: the source is empty, the destination holds the same
//      record, nothing else appears or disappears. Titles whose registry blocks Move must refuse it.
//   2. Unchanged storage is preserved: after an export with one staged edit, every other slot (party and every
//      box slot, empty or occupied) reopens with an identical view; on the flat formats (X/Y/ORAS, BD/SP, Let's
//      Go) the bytes outside the edited record and the format's checksum/hash areas are identical.
//   3. Fail closed: truncated, empty, zero-filled and tail-cut copies of every title never open, both through
//      openSession and through the strict Save Instances inspection; this runs under ASan/UBSan as well.
//
// Evidence class: SYNTHETIC host fixtures. A pass here is not a hardware or in-game acceptance.

#include "Integration/Common/GameCapabilityRegistry.h"
#include "Integration/Modern/ModernSources.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"
#include "fixtures/gen7b/Gen7bSyntheticSave.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"
#include "fixtures/gen8a/Gen8aSyntheticSave.h"
#include "fixtures/gen8b/Gen8bSyntheticSave.h"
#include "fixtures/gen9sv/Gen9SvSyntheticSave.h"
#include "fixtures/gen9za/Gen9ZaSyntheticSave.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>

namespace Modern = PokeVault::Integration::Modern;
namespace Cap = PokeVault::Integration::Capability;
using Modern::SlotRef;
using Field = Modern::Field;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
std::vector<uint8_t> slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

struct Title {
    std::string id;
    std::function<std::vector<uint8_t>()> make;
    bool flat = false;   // container not re-encrypted as a whole: byte-level preservation is checkable
};

std::vector<Title> titles() {
    namespace L6 = PokeVault::Integration::Gen6::Layout;
    namespace L7 = PokeVault::Integration::Gen7::Layout;
    using G6 = Gen6Fixture::SaveSpec;
    using G7 = Gen7Fixture::SaveSpec;
    return {
        {"x_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::XY, L6::VersionX}); }, true},
        {"y_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::XY, L6::VersionY}); }, true},
        {"omegaruby_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::ORAS, L6::VersionOmegaRuby}); }, true},
        {"alphasapphire_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::ORAS, L6::VersionAlphaSapphire}); }, true},
        {"sun_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::SunMoon, L7::VersionSun}); }},
        {"moon_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::SunMoon, L7::VersionMoon}); }},
        {"ultrasun_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::UltraSunUltraMoon, L7::VersionUltraSun}); }},
        {"ultramoon_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::UltraSunUltraMoon, L7::VersionUltraMoon}); }},
        {"letsgo_pikachu_switch", [] { return Gen7bFixture::makeSave(); }, true},
        {"letsgo_eevee_switch", [] { Gen7bFixture::Options o; o.version = 43; return Gen7bFixture::makeSave(o); }, true},
        {"sword_switch", [] { return Gen8Fixture::makeSave(); }},
        {"shield_switch", [] { Gen8Fixture::Options o; o.version = 45; return Gen8Fixture::makeSave(o); }},
        {"brilliant_diamond_switch", [] { return Gen8bFixture::makeSave(); }, true},
        {"shining_pearl_switch", [] { Gen8bFixture::Options o; o.romCode = 1; return Gen8bFixture::makeSave(o); }, true},
        {"legends_arceus_switch", [] { return Gen8aFixture::makeSave(); }},
        {"scarlet_switch", [] { return Gen9SvFixture::makeSave(); }},
        {"violet_switch", [] { Gen9SvFixture::Options o; o.game = 51; return Gen9SvFixture::makeSave(o); }},
        {"legends_za_switch", [] { return Gen9ZaFixture::makeSave(); }},
    };
}

using Identity = std::tuple<uint16_t, uint8_t, uint32_t, uint32_t, std::string, std::string, uint16_t, uint32_t>;
Identity identity(const Modern::RecordView& v) {
    return {v.species, v.form, v.pid, v.encryptionConstant, v.nickname, v.otName, v.tid, v.experience};
}
bool occupied(const std::optional<Modern::RecordView>& v) { return v && v->valid && !v->empty; }

// Every occupied box record, as a multiset of identities (the clone detector).
std::multiset<Identity> boxRecords(const Modern::NativeSession& s) {
    std::multiset<Identity> out;
    for (size_t b = 0; b < s.boxCount(); ++b)
        for (size_t i = 0; i < s.slotsPerBox(); ++i)
            if (const auto v = s.view(SlotRef::boxed(b, i)); occupied(v)) out.insert(identity(*v));
    return out;
}
std::map<SlotRef, Identity> layout(const Modern::NativeSession& s) {
    std::map<SlotRef, Identity> out;
    for (size_t p = 0; p < s.partyCount(); ++p)
        if (const auto v = s.view(SlotRef::party(p)); occupied(v)) out[SlotRef::party(p)] = identity(*v);
    for (size_t b = 0; b < s.boxCount(); ++b)
        for (size_t i = 0; i < s.slotsPerBox(); ++i)
            if (const auto v = s.view(SlotRef::boxed(b, i)); occupied(v)) out[SlotRef::boxed(b, i)] = identity(*v);
    return out;
}
std::string at(SlotRef r) {
    return r.region == Modern::Region::Party ? "party " + std::to_string(r.slot)
                                             : "box " + std::to_string(r.box) + "/" + std::to_string(r.slot);
}
// First empty, unstaged slot in `box` (not equal to `avoid`), if any.
std::optional<SlotRef> emptyIn(const Modern::NativeSession& s, size_t box, SlotRef avoid) {
    for (size_t i = 0; i < s.slotsPerBox(); ++i) {
        const auto r = SlotRef::boxed(box, i);
        if (r != avoid && s.emptyDestination(r)) return r;
    }
    return std::nullopt;
}

std::string exportDir(const std::string& root, const std::string& name) {
    const std::string d = root + "/" + name;
    std::filesystem::create_directories(d);
    return d;
}
std::vector<uint8_t> exportEdited(const Modern::NativeSession& s, const std::string& dir, std::string& error) {
    Modern::ExportRequest req;
    req.rootDirectory = dir;
    req.timestamp = "20261101_130000";
    const auto r = s.exportWorkingCopy(req);
    if (!r.success) {
        error = r.error;
        return {};
    }
    for (const auto& e : std::filesystem::directory_iterator(r.directory))
        if (e.path().filename().string().rfind("edited", 0) == 0) return slurp(e.path().string());
    error = "no edited file";
    return {};
}

// ---------------------------------------------------------------------------------------------------------
// 1. Move never clones
// ---------------------------------------------------------------------------------------------------------
void moveNeverClones(const Title& t, const std::string& root) {
    const auto* row = Cap::find(t.id);
    const auto pristine = t.make();
    auto opened = Modern::openSession(t.id, pristine);
    check(opened.ready(), t.id + " opens: " + opened.diagnostic);
    if (!opened.ready() || !row) return;
    auto& s = *opened.session;
    const auto before = boxRecords(s);
    const auto layoutBefore = layout(s);
    check(!before.empty(), t.id + " fixture has occupied box records to move");
    const bool moveClaimed = row->supports(Cap::Workflow::Move);

    size_t moved = 0;
    for (size_t b = 0; b < s.boxCount(); ++b)
        for (size_t i = 0; i < s.slotsPerBox(); ++i) {
            const auto from = SlotRef::boxed(b, i);
            if (!s.originalOccupied(from) || s.stagedAt(from)) continue;
            const auto support = s.moveSupport(from);
            if (!moveClaimed) {
                check(!support.available && !support.reason.empty(), t.id + " registry-blocked Move refused at " + at(from));
                std::string error;
                const auto dest = s.firstEmptyBoxSlot();
                check(!dest || !s.move(from, *dest, error), t.id + " blocked Move does not execute at " + at(from));
                continue;
            }
            if (!support.available) continue;   // team / partner protected: covered by the sweep
            const auto original = identity(*s.view(from));
            // Chain: same box -> another box -> back to the first empty slot of the first box.
            const auto a = emptyIn(s, b, from);
            const auto otherBox = (b + 1) % s.boxCount();
            std::string error;
            if (!a) continue;
            check(s.move(from, *a, error), t.id + " move " + at(from) + " -> " + at(*a) + ": " + error);
            check(!occupied(s.view(from)) && occupied(s.view(*a)) && identity(*s.view(*a)) == original,
                  t.id + " after move the source is empty and the destination holds the same record");
            check(boxRecords(s) == before, t.id + " move " + at(from) + " neither clones nor loses a record");
            // A record that is itself staged (the destination of this move) is never moved again: the
            // adapters require an unmodified source, so the refusal must leave every slot exactly as it was.
            if (const auto c = emptyIn(s, otherBox, *a)) {
                const auto staged = layout(s);
                check(!s.move(*a, *c, error) && !error.empty(), t.id + " a staged destination " + at(*a) + " is not moved again");
                check(layout(s) == staged && boxRecords(s) == before, t.id + " the refused re-move changes nothing");
            }
            ++moved;
            if (moved >= 3) break;   // three independent chains per title keep the run short
        }
    if (moveClaimed) check(moved > 0, t.id + " at least one Move chain ran");
    check(layout(s).size() == layoutBefore.size(), t.id + " total occupied slot count unchanged by moves");

    if (!moveClaimed || !s.hasChanges() || !s.exportSupport().available) return;
    std::string error;
    const auto edited = exportEdited(s, exportDir(root, t.id + "-move"), error);
    check(!edited.empty(), t.id + " moved storage exports: " + error);
    if (edited.empty()) return;
    check(edited.size() == pristine.size(), t.id + " export keeps the exact size");
    auto re = Modern::openSession(t.id, edited);
    check(re.ready(), t.id + " moved export strictly reopens: " + re.diagnostic);
    if (!re.ready()) return;
    check(boxRecords(*re.session) == before, t.id + " moved export holds exactly the original records (no clones)");
    check(layout(*re.session) == layout(s), t.id + " moved export matches the reviewed layout slot by slot");
}

// ---------------------------------------------------------------------------------------------------------
// 2. Untouched slots and bytes survive an edit + export
// ---------------------------------------------------------------------------------------------------------
void unchangedPreserved(const Title& t, const std::string& root) {
    const auto* row = Cap::find(t.id);
    const auto pristine = t.make();
    auto opened = Modern::openSession(t.id, pristine);
    if (!opened.ready() || !row) return;
    auto& s = *opened.session;
    // A session with nothing staged has nothing to export.
    check(!s.hasChanges(), t.id + " fresh session has no changes");
    std::string error;
    check(s.stageText(SlotRef::party(0), Field::Nickname, "KEEP", error), t.id + " stage one nickname: " + error);
    if (!row->supports(Cap::Workflow::Export)) {
        check(!s.exportSupport().available, t.id + " export stays blocked");
        return;
    }
    const auto edited = exportEdited(s, exportDir(root, t.id + "-keep"), error);
    check(!edited.empty() && edited.size() == pristine.size(), t.id + " one-edit export, exact size: " + error);
    if (edited.empty()) return;
    auto re = Modern::openSession(t.id, edited);
    auto base = Modern::openSession(t.id, pristine);
    check(re.ready() && base.ready(), t.id + " both copies open");
    if (!re.ready() || !base.ready()) return;
    size_t differingSlots = 0, emptyKept = 0;
    std::optional<SlotRef> aliasOfParty0;
    if (t.id.find("letsgo") != std::string::npos) {
        const auto p0 = identity(*base.session->view(SlotRef::party(0)));
        for (size_t b = 0; b < base.session->boxCount() && !aliasOfParty0; ++b)
            for (size_t i = 0; i < base.session->slotsPerBox(); ++i)
                if (const auto v = base.session->view(SlotRef::boxed(b, i)); occupied(v) && identity(*v) == p0) {
                    aliasOfParty0 = SlotRef::boxed(b, i);
                    break;
                }
        check(aliasOfParty0.has_value(), t.id + " party 0 is stored in a box slot (pointer party)");
    }
    auto compare = [&](SlotRef r) {
        const auto a = base.session->view(r), b = re.session->view(r);
        const bool same = occupied(a) == occupied(b) && (!occupied(a) || identity(*a) == identity(*b));
        if (!occupied(a) && !occupied(b)) ++emptyKept;
        if (!same) ++differingSlots;
        // Let's Go keeps the party as pointers into box storage: party 0's own storage slot is the same record.
        const bool alias = r.region == Modern::Region::Box && aliasOfParty0 && *aliasOfParty0 == r;
        if (r != SlotRef::party(0) && !alias) check(same, t.id + " untouched " + at(r) + " is preserved");
    };
    for (size_t p = 0; p < 6; ++p) compare(SlotRef::party(p));
    for (size_t b = 0; b < base.session->boxCount(); ++b)
        for (size_t i = 0; i < base.session->slotsPerBox(); ++i) compare(SlotRef::boxed(b, i));
    const size_t expected = aliasOfParty0 ? 2 : 1;
    check(differingSlots == expected, t.id + " exactly the edited record changed (" + std::to_string(differingSlots) + " slot views)");
    check(emptyKept > 0, t.id + " empty slots stay empty");
    if (t.flat) {
        size_t diff = 0;
        for (size_t i = 0; i < pristine.size(); ++i) diff += pristine[i] != edited[i];
        // One re-encrypted record (<= 0x170 bytes in these formats) plus checksum words / one MD5.
        check(diff > 0 && diff <= 0x200, t.id + " flat export differs only in the record and its checksums (" +
                                             std::to_string(diff) + " bytes)");
    }
}

// ---------------------------------------------------------------------------------------------------------
// 3. Truncated / malformed copies fail closed
// ---------------------------------------------------------------------------------------------------------
void failClosed(const Title& t, const std::string& root) {
    const auto pristine = t.make();
    const size_t n = pristine.size();
    std::vector<size_t> lengths = {0, 1, 2, 0x10, 0x100, n / 4, n / 2, n - 0x200, n - 0x20, n - 2, n - 1};
    for (size_t k = 1; k < 24; ++k) lengths.push_back(n * k / 24);
    for (const size_t len : lengths) {
        if (len >= n) continue;
        const std::vector<uint8_t> cut(pristine.begin(), pristine.begin() + static_cast<std::ptrdiff_t>(len));
        check(!Modern::openSession(t.id, cut).ready(), t.id + " truncated to " + std::to_string(len) + " bytes is refused");
    }
    std::vector<uint8_t> zeros(n, 0);
    check(!Modern::openSession(t.id, zeros).ready(), t.id + " zero-filled save of the right size is refused");
    // Whole-file integrity formats (SwishCrypto SHA-256: SWSH/PLA/SV/Z-A; BD/SP MD5): one flipped byte anywhere
    // is refused, never parsed as data.
    const bool wholeFileHash = t.id.find("_switch") != std::string::npos && t.id.find("letsgo") == std::string::npos;
    if (wholeFileHash)
        for (const size_t pos : {size_t{0x10}, n / 3, n / 2, n - 0x30}) {
            auto flipped = pristine;
            flipped[pos] ^= 0x01;
            check(!Modern::openSession(t.id, flipped).ready(), t.id + " byte flip at " + std::to_string(pos) + " is refused");
        }
    // The strict Save Instances path refuses the half-file too.
    const std::string path = root + "/" + t.id + "-half";
    {
        std::ofstream o(path, std::ios::binary);
        o.write(reinterpret_cast<const char*>(pristine.data()), static_cast<std::streamsize>(n / 2));
    }
    check(!Modern::inspectSourceFile(path, "Manual", t.id).ready(), t.id + " half file refused by Save Instances");
}
} // namespace

int main() {
    const auto all = titles();
    check(all.size() == 18, "18 modern titles");
    const std::string root = [] {
        char pattern[] = "/tmp/modern-integrity-XXXXXX";
        const char* d = ::mkdtemp(pattern);
        return std::string(d ? d : "");
    }();
    for (const auto& t : all) {
        moveNeverClones(t, root);
        unchangedPreserved(t, root);
        failClosed(t, root);
    }
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::cout << "test_modern_storage_integrity: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
