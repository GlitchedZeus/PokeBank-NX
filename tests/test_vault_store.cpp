// Master Vault v1 storage core (include/Vault/VaultStore.h): create/open, exact native deposit of
// Gen I-V records under stable Vault IDs, byte-identical read-back, withdraw-as-copy, persistence
// across reopen, search with highlight spans, and fault injection (full SD at every write,
// interrupted deposits at every step, torn record, corrupt / truncated / missing index, newer
// schema) with no data loss. Synthetic records only (the builders of tests/test_native_pokemon_file.cpp);
// every Vault lives in its own mkdtemp folder.
#include "Vault/VaultSearch.h"
#include "Vault/VaultStore.h"
#include "Encryption/Encryption3FRLG.h"
#include "PokemonFile/NativePokemonFile.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace V = PokeBank::Vault;
namespace S = PokeBank::Vault::Search;
namespace F = PokeBank::PokemonFile;
using Bytes = std::vector<uint8_t>;

namespace {
void w16(Bytes& b, size_t o, uint16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(v >> 8); }
void w32(Bytes& b, size_t o, uint32_t v) { w16(b, o, uint16_t(v)); w16(b, o + 2, uint16_t(v >> 16)); }

Bytes pk3(size_t size, uint16_t species, uint32_t pid = 0x89ABCDEFu) {
    Bytes b(size, 0);
    w32(b, 0, pid); w32(b, 4, 0x12345678u);
    b[0x12] = 2;
    w16(b, 0x20, species); w32(b, 0x24, 1000);
    w16(b, 0x1C, Encryption::checksum3FRLG(std::as_bytes(std::span<const uint8_t>(b).first(80))));
    return b;
}
Bytes ek3(const Bytes& plain) {
    std::byte* e = Encryption::encryptArray3FRLG(std::as_bytes(std::span<const uint8_t>(plain)));
    Bytes out(reinterpret_cast<const uint8_t*>(e), reinterpret_cast<const uint8_t*>(e) + plain.size());
    delete[] e;
    return out;
}
uint16_t sum45(const Bytes& b) { uint32_t s = 0; for (size_t o = 8; o < 136; o += 2) s += uint16_t(b[o] | (b[o + 1] << 8)); return uint16_t(s); }
Bytes pk45(int gen, size_t size, uint16_t species) {
    Bytes b(size, 0);
    w32(b, 0, 0x2468ACE0u);
    w16(b, 0x08, species);
    w32(b, 0x10, 5000);
    b[0x5F] = gen == 4 ? 10 : 20;
    if (gen == 5) w16(b, 0x80, 0x5555);
    w16(b, 0x06, sum45(b));
    return b;
}
Bytes list(int gen, uint8_t species) {
    const size_t body = gen == 1 ? 44 : 48, str = 11;
    Bytes b(3 + body + 2 * str, 0x50);
    b[0] = 1; b[1] = species; b[2] = 0xFF;
    std::fill(b.begin() + 3, b.begin() + 3 + static_cast<long>(body), 0);
    b[3] = species;
    b[3 + body] = 0x80;
    b[3 + body + str] = 0x81;
    return b;
}

std::string makeRoot() {
    char tmpl[] = "/tmp/pokebank-vault-XXXXXX";
    const char* dir = ::mkdtemp(tmpl);
    assert(dir);
    return std::string(dir) + "/vault";
}

std::vector<std::string> names(const std::string& dir) {
    std::vector<std::string> out;
    if (DIR* d = ::opendir(dir.c_str())) {
        while (dirent* e = ::readdir(d)) {
            const std::string n = e->d_name;
            if (n != "." && n != "..") out.push_back(n);
        }
        ::closedir(d);
    }
    return out;
}
size_t count(const std::string& dir) { return names(dir).size(); }

Bytes readFile(const std::string& path) {
    Bytes out;
    if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
        int c;
        while ((c = std::fgetc(f)) != EOF) out.push_back(static_cast<uint8_t>(c));
        std::fclose(f);
    }
    return out;
}
void writeFile(const std::string& path, const Bytes& bytes) {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    assert(f);
    if (!bytes.empty()) assert(std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size());
    std::fclose(f);
}

V::DepositRequest request(int gen, Bytes raw, const char* species, const char* nickname, const char* game) {
    V::DepositRequest r;
    r.generation = gen;
    r.raw = std::move(raw);
    r.summary.species = 25;
    r.summary.speciesName = species;
    r.summary.nickname = nickname;
    r.summary.otName = "RED";
    r.summary.level = 20;
    r.provenance.sourceGame = game;
    r.provenance.sourceLabel = "test.sav";
    r.provenance.sourceBox = 0;
    r.provenance.sourceSlot = 3;
    r.provenance.appVersion = "test";
    return r;
}

struct Snapshot {
    std::vector<std::string> ids;
    std::vector<uint64_t> seqs;
};
Snapshot snap(const V::Store& s) {
    Snapshot out;
    for (const auto& e : s.entries()) { out.ids.push_back(e.id.text()); out.seqs.push_back(e.sequence); }
    return out;
}
bool same(const Snapshot& a, const Snapshot& b) { return a.ids == b.ids && a.seqs == b.seqs; }

// Every deposited record still reads back byte-identical.
void assertAllReadable(const V::Store& s, const std::vector<std::pair<std::string, Bytes>>& expected) {
    for (const auto& [idText, raw] : expected) {
        const auto id = V::VaultId::parse(idText);
        assert(id);
        std::string error;
        const auto rec = s.read(*id, error);
        if (!rec) std::cerr << idText << ": " << error << "\n";
        assert(rec && rec->raw == raw);
    }
}

V::Store open(const std::string& root, V::OpenReport* report = nullptr, V::Io io = V::defaultIo()) {
    V::Store s(std::move(io));
    std::string error;
    const bool ok = s.open(root, report, error);
    if (!ok) std::cerr << "open: " << error << "\n";
    assert(ok);
    return s;
}

// The real file system, except that writes whose path contains `failOn` report a full SD card and
// leave a partial temp file behind, as an interrupted FAT write would.
V::Io fullSdOn(std::string failOn) {
    V::Io io = V::defaultIo();
    const auto real = io.replace;
    io.replace = [real, failOn](const std::string& target, std::span<const uint8_t> bytes,
                                const PokeBank::Storage::DurableFile::Validator& validator) {
        if (target.find(failOn) != std::string::npos) {
            writeFile(target + ".tmp.0", Bytes(bytes.begin(), bytes.begin() + static_cast<long>(bytes.size() / 2)));
            PokeBank::Storage::DurableFile::Result r;
            r.error = "No space left on device";
            return r;
        }
        return real(target, bytes, validator);
    };
    return io;
}
}  // namespace

int main() {
    std::string error;

    // ---- create / open ---------------------------------------------------------------------
    const std::string root = makeRoot();
    V::OpenReport report;
    V::Store vault = open(root, &report);
    assert(report.createdNew && !report.readOnly && vault.entries().empty());
    for (const char* sub : {"/records", "/journal", "/quarantine"}) {
        struct stat st{};
        assert(::stat((root + sub).c_str(), &st) == 0 && S_ISDIR(st.st_mode));
    }

    // ---- deposit one record of every Gen I-V format ------------------------------------------
    const std::vector<std::pair<int, Bytes>> records = {
        {1, list(1, 0x54)}, {2, list(2, 25)}, {3, pk3(80, 25)}, {3, pk3(100, 6)},
        {4, pk45(4, 136, 393)}, {4, pk45(4, 236, 394)}, {5, pk45(5, 136, 495)}, {5, pk45(5, 220, 496)},
    };
    std::vector<std::pair<std::string, Bytes>> deposited;
    std::set<std::string> idTexts;
    for (const auto& [gen, raw] : records) {
        const auto id = vault.deposit(request(gen, raw, gen == 3 ? "Pikachu" : "Bulbasaur", "SPARKY", "FireRed"), error);
        if (!id) std::cerr << "gen " << gen << ": " << error << "\n";
        assert(id);
        const std::string t = id->text();
        assert(t.size() == 36 && t[14] == '4');                       // UUID v4 text
        assert(t[19] == '8' || t[19] == '9' || t[19] == 'a' || t[19] == 'b');
        assert(V::VaultId::parse(t) && *V::VaultId::parse(t) == *id);
        assert(idTexts.insert(t).second);                             // stable, unique
        deposited.push_back({t, raw});
    }
    assert(vault.entries().size() == records.size());
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& e = vault.entries()[i];
        assert(e.generation == records[i].first && e.rawSize == records[i].second.size());
        assert(e.rawSha256 == V::sha256(records[i].second));
        assert(e.sequence == i + 1);
        assert(e.format == "pk" + std::to_string(records[i].first));
        assert(e.provenance.sourceGame == "FireRed" && e.provenance.sourceSlot == 3 && e.provenance.depositedAt > 0);
    }
    assert(vault.entries()[3].party && !vault.entries()[2].party);
    assert(count(root + "/journal") == 0);
    assertAllReadable(vault, deposited);
    // The record file holds the exact native bytes (no hidden canonicalisation).
    {
        const auto file = readFile(vault.recordPath(vault.entries()[2].id));
        const auto& raw = records[2].second;
        assert(std::search(file.begin(), file.end(), raw.begin(), raw.end()) != file.end());
    }

    // The same bytes again are a second, independent Vault entity (never silently dropped).
    const auto twin = vault.deposit(request(3, records[2].second, "Pikachu", "TWIN", "Emerald"), error);
    assert(twin && !(*twin == vault.entries()[2].id));
    deposited.push_back({twin->text(), records[2].second});
    assert(vault.entries().back().rawSha256 == vault.entries()[2].rawSha256);

    // ---- deposit refusals write nothing ------------------------------------------------------
    {
        const size_t before = count(root + "/records");
        const Snapshot s0 = snap(vault);
        assert(!vault.deposit(request(3, ek3(records[2].second), "x", "x", "x"), error));   // encrypted form
        assert(!vault.deposit(request(4, records[2].second, "x", "x", "x"), error));        // wrong generation
        assert(!vault.deposit(request(6, records[2].second, "x", "x", "x"), error));        // Gen VI+ not in v1
        Bytes bad = records[4].second; bad[0x40] ^= 1;                                      // native checksum
        assert(!vault.deposit(request(4, bad, "x", "x", "x"), error));
        assert(!vault.deposit(request(3, {}, "x", "x", "x"), error));
        assert(count(root + "/records") == before && count(root + "/journal") == 0 && same(s0, snap(vault)));
    }

    // ---- withdraw-as-copy -------------------------------------------------------------------
    {
        const auto& e = vault.entries()[4];   // Gen IV stored record
        const Bytes fileBefore = readFile(vault.recordPath(e.id));
        const auto copy = vault.withdrawCopy(e.id, 4, error);
        assert(copy && copy->generation == 4 && copy->plain == records[4].second && !copy->party);
        assert(!vault.withdrawCopy(e.id, 5, error) && error.find("does not convert") != std::string::npos);
        assert(!vault.withdrawCopy(e.id, 3, error));
        const auto again = vault.withdrawCopy(e.id, 4, error);   // a copy: the Vault keeps it
        assert(again && again->plain == copy->plain);
        assert(readFile(vault.recordPath(e.id)) == fileBefore && vault.entries().size() == deposited.size());
        V::VaultId unknown;
        assert(!vault.withdrawCopy(unknown, 4, error) && !vault.read(unknown, error));
    }

    // ---- persistence across reopen ----------------------------------------------------------
    const Snapshot before = snap(vault);
    {
        V::OpenReport r;
        V::Store reopened = open(root, &r);
        assert(!r.createdNew && !r.indexRebuilt && r.rolledForward == 0 && r.rolledBack == 0 && r.quarantined == 0);
        assert(same(before, snap(reopened)));
        assert(reopened.entries()[0].summary.nickname == "SPARKY" && reopened.entries()[0].summary.otName == "RED");
        assertAllReadable(reopened, deposited);
        // Next deposit continues the sequence; ids never repeat.
        const auto id = reopened.deposit(request(2, list(2, 152), "Chikorita", "LEAF", "Crystal"), error);
        assert(id && reopened.entries().back().sequence == before.seqs.back() + 1);
        deposited.push_back({id->text(), list(2, 152)});
        vault = open(root);
    }

    // ---- search with highlight spans --------------------------------------------------------
    {
        auto hits = S::search(vault.entries(), "pika");
        assert(!hits.empty());
        for (const auto& h : hits) {
            const auto spans = S::spansFor(h, S::Field::Species);
            assert(spans.size() == 1 && spans[0].start == 0 && spans[0].length == 4);
        }
        hits = S::search(vault.entries(), "PIKA twin");   // AND of terms, case-insensitive
        assert(hits.size() == 1 && vault.entries()[hits[0].entryIndex].summary.nickname == "TWIN");
        assert(S::spansFor(hits[0], S::Field::Nickname).size() == 1);
        assert(S::search(vault.entries(), "").size() == vault.entries().size());
        assert(S::search(vault.entries(), "", 4).size() == 2);           // generation filter (withdraw)
        assert(S::search(vault.entries(), "leaf", 3).empty());
        assert(S::search(vault.entries(), "zzz").empty());
        // Overlapping matches merge into one highlighted run.
        S::Hit h;
        h.spans = {{S::Field::Nickname, 2, 3}, {S::Field::Nickname, 0, 3}, {S::Field::Species, 1, 1}};
        const auto merged = S::spansFor(h, S::Field::Nickname);
        assert(merged.size() == 1 && merged[0].start == 0 && merged[0].length == 5);
        const auto idHits = S::search(vault.entries(), vault.entries()[1].id.text().substr(0, 8));
        assert(idHits.size() == 1 && idHits[0].entryIndex == 1);
    }

    // ---- fault: full SD card at each write of a deposit -> nothing lost, nothing half-added ---
    for (const char* where : {"/journal/", "/records/", "index.pbvi"}) {
        const Snapshot s0 = snap(vault);
        V::Store faulty = open(root, nullptr, fullSdOn(where));
        assert(!faulty.deposit(request(3, pk3(80, 7), "Squirtle", "FULL", "Ruby"), error));
        assert(error.rfind("Nothing was copied", 0) == 0);
        assert(same(s0, snap(faulty)) && count(root + "/journal") == 0);
        V::OpenReport r;
        vault = open(root, &r);
        assert(same(s0, snap(vault)));
        assertAllReadable(vault, deposited);
        for (const auto& n : names(root + "/records"))   // only published, verified records remain
            assert(n.size() == 36 + 5 && n.substr(36) == ".pbvr");
    }

    // ---- fault: interrupted deposit at each step (power loss: no cleanup ran) ------------------
    {
        const Snapshot s0 = snap(vault);
        // After the journal, before the record: rolled back.
        V::Store crash = open(root);
        crash.setCrashHookForTesting([](V::Step s) { return s == V::Step::JournalWritten; });
        assert(!crash.deposit(request(3, pk3(80, 8), "Wartortle", "J", "Ruby"), error));
        assert(count(root + "/journal") == 1);
        V::OpenReport r;
        vault = open(root, &r);
        assert(r.rolledBack == 1 && r.rolledForward == 0 && same(s0, snap(vault)) && count(root + "/journal") == 0);

        // After the record, before the index: the verified record is rolled forward.
        crash = open(root);
        crash.setCrashHookForTesting([](V::Step s) { return s == V::Step::RecordPublished; });
        const Bytes raw = pk3(80, 9);
        assert(!crash.deposit(request(3, raw, "Blastoise", "R", "Ruby"), error));
        r = {};
        vault = open(root, &r);
        assert(r.rolledForward == 1 && vault.entries().size() == s0.ids.size() + 1 && count(root + "/journal") == 0);
        assert(vault.entries().back().summary.nickname == "R");
        deposited.push_back({vault.entries().back().id.text(), raw});
        assertAllReadable(vault, deposited);

        // After the index, before the journal is cleared: committed, journal tidied.
        const Snapshot s1 = snap(vault);
        crash = open(root);
        crash.setCrashHookForTesting([](V::Step s) { return s == V::Step::IndexPublished; });
        const Bytes raw2 = pk45(5, 136, 501);
        assert(!crash.deposit(request(5, raw2, "Oshawott", "I", "White"), error));
        r = {};
        vault = open(root, &r);
        assert(r.rolledForward == 0 && r.rolledBack == 0 && vault.entries().size() == s1.ids.size() + 1);
        assert(count(root + "/journal") == 0);
        deposited.push_back({vault.entries().back().id.text(), raw2});
        assertAllReadable(vault, deposited);
    }

    // ---- fault: torn record (interrupted write left a short file under the final name) ---------
    {
        const Snapshot s0 = snap(vault);
        V::Store crash = open(root);
        crash.setCrashHookForTesting([](V::Step s) { return s == V::Step::RecordPublished; });
        assert(!crash.deposit(request(4, pk45(4, 136, 400), "Bibarel", "TORN", "Diamond"), error));
        // Find the journaled id and tear its record in half.
        const auto journals = names(root + "/journal");
        assert(journals.size() == 1);
        const auto id = V::VaultId::parse(journals[0].substr(0, 36));
        assert(id);
        Bytes full = readFile(crash.recordPath(*id));
        full.resize(full.size() / 2);
        writeFile(crash.recordPath(*id), full);
        V::OpenReport r;
        vault = open(root, &r);
        assert(r.rolledBack == 1 && r.quarantined >= 1 && same(s0, snap(vault)));
        assert(!vault.find(*id) && count(root + "/journal") == 0);
        assertAllReadable(vault, deposited);
    }

    // ---- fault: corrupt, truncated and missing index -> rebuilt from the records ---------------
    {
        const Snapshot s0 = snap(vault);
        const std::string index = vault.indexPath();
        Bytes bytes = readFile(index);
        bytes[bytes.size() / 2] ^= 0x40;
        writeFile(index, bytes);
        V::OpenReport r;
        vault = open(root, &r);
        assert(r.indexRebuilt && r.quarantined == 1 && same(s0, snap(vault)));
        assertAllReadable(vault, deposited);

        bytes = readFile(index);
        bytes.resize(bytes.size() - 10);
        writeFile(index, bytes);
        r = {};
        vault = open(root, &r);
        assert(r.indexRebuilt && same(s0, snap(vault)));

        assert(std::remove(index.c_str()) == 0);
        r = {};
        vault = open(root, &r);
        assert(r.indexRebuilt && !r.createdNew && same(s0, snap(vault)));
        assertAllReadable(vault, deposited);
        // Quarantined indexes are kept, never deleted.
        assert(count(vault.quarantineDir()) >= 2);
    }

    // ---- corruption detection on read; a damaged record is never handed out --------------------
    {
        const auto& e = vault.entries()[0];
        const std::string path = vault.recordPath(e.id);
        const Bytes good = readFile(path);
        Bytes bad = good;
        bad[bad.size() - 70] ^= 0x01;   // inside the raw Pokemon bytes
        writeFile(path, bad);
        assert(!vault.read(e.id, error) && error.find("verification") != std::string::npos);
        assert(!vault.withdrawCopy(e.id, e.generation, error));
        // Metadata tampering (only the whole-file SHA-256 can see it) is refused as well.
        const std::string needle = "nickname=SPARKY";
        const auto at = std::search(good.begin(), good.end(), needle.begin(), needle.end());
        assert(at != good.end());
        bad = good;
        bad[static_cast<size_t>(at - good.begin()) + 9] = 'T';
        writeFile(path, bad);
        assert(!vault.read(e.id, error) && error.find("SHA-256") != std::string::npos);
        writeFile(path, good);
        assert(vault.read(e.id, error));
    }

    // ---- stray files are quarantined, not deleted; index generations stay bounded ---------------
    {
        writeFile(root + "/records/notes.txt", {'h', 'i'});
        V::OpenReport r;
        vault = open(root, &r);
        assert(r.quarantined == 1 && count(root + "/records") == vault.entries().size());
        size_t previous = 0;
        for (const auto& n : names(root))
            if (n.rfind("index.pbvi.previous.", 0) == 0) ++previous;
        assert(previous <= 1);
    }

    // ---- newer schema: read-only, never rewritten ----------------------------------------------
    {
        const std::string index = vault.indexPath();
        Bytes bytes = readFile(index);
        bytes[8] = 99;   // schema field
        writeFile(index, bytes);
        V::OpenReport r;
        V::Store newer = open(root, &r);
        assert(r.readOnly && newer.readOnly());
        assert(!newer.deposit(request(3, pk3(80, 10), "Caterpie", "N", "Ruby"), error));
        assert(readFile(index) == bytes);
        bytes[8] = 1;
        writeFile(index, bytes);   // the original bytes again
        r = {};
        vault = open(root, &r);
        assert(!r.readOnly && vault.entries().size() == deposited.size());
        assertAllReadable(vault, deposited);
    }

    // ---- unopened store refuses everything ------------------------------------------------------
    {
        V::Store closed;
        assert(!closed.deposit(request(3, pk3(80, 11), "x", "x", "x"), error));
        assert(!closed.read(vault.entries()[0].id, error));
    }

    std::cout << "test_vault_store: all assertions passed (" << deposited.size() << " records)\n";
    return 0;
}
