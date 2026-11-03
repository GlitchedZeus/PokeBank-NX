// Master Vault v1 storage core. See include/Vault/VaultStore.h for the contract and
// docs/roadmap/VAULT_V1_IMPLEMENTATION_PLAN.md for the design.
#include "Vault/VaultStore.h"

#include "Utils/PokeBankPaths.h"
#include "Utils/SHA256.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <random>
#include <sys/stat.h>

namespace PokeBank::Vault {
namespace {

namespace PF = PokeBank::PokemonFile;
namespace DF = PokeBank::Storage::DurableFile;

constexpr char kRecordMagic[8] = {'P', 'B', 'N', 'X', 'V', 'R', 'E', 'C'};
constexpr char kIndexMagic[8] = {'P', 'B', 'N', 'X', 'V', 'I', 'D', 'X'};
constexpr std::string_view kJournalHeader = "PBNX-VAULT-JOURNAL 1\n";

void put32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
}
void put64(std::vector<uint8_t>& out, uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
}
uint32_t get32(std::span<const uint8_t> b, std::size_t o) {
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(b[o + static_cast<std::size_t>(i)]) << (8 * i);
    return v;
}
uint64_t get64(std::span<const uint8_t> b, std::size_t o) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(b[o + static_cast<std::size_t>(i)]) << (8 * i);
    return v;
}

std::string escape(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (const char c : value) {
        if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else out.push_back(c);
    }
    return out;
}
std::string unescape(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            const char n = value[++i];
            out.push_back(n == 'n' ? '\n' : n == 'r' ? '\r' : n);
        } else {
            out.push_back(value[i]);
        }
    }
    return out;
}

bool parseInt(std::string_view text, int64_t& out) {
    if (text.empty() || text.size() > 20) return false;
    bool negative = false;
    std::size_t i = 0;
    if (text[0] == '-') { negative = true; i = 1; }
    if (i >= text.size()) return false;
    uint64_t v = 0;
    for (; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        v = v * 10 + static_cast<uint64_t>(text[i] - '0');
    }
    out = negative ? -static_cast<int64_t>(v) : static_cast<int64_t>(v);
    return true;
}

bool parseHex(std::string_view text, std::span<uint8_t> out) {
    if (text.size() != out.size() * 2) return false;
    const auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < out.size(); ++i) {
        const int hi = nibble(text[2 * i]), lo = nibble(text[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<uint8_t>(hi << 4 | lo);
    }
    return true;
}

std::string encodeMeta(const Entry& e) {
    std::string m;
    const auto kv = [&m](std::string_view k, std::string_view v) {
        m.append(k);
        m.push_back('=');
        m += escape(v);
        m.push_back('\n');
    };
    const auto num = [&kv](std::string_view k, int64_t v) { kv(k, std::to_string(v)); };
    kv("id", e.id.text());
    num("sequence", static_cast<int64_t>(e.sequence));
    num("generation", e.generation);
    kv("format", e.format);
    num("party", e.party ? 1 : 0);
    num("japanese", e.japanese ? 1 : 0);
    num("raw_size", e.rawSize);
    kv("raw_sha256", hex(e.rawSha256));
    kv("canonicalization", e.canonicalization);
    num("species", e.summary.species);
    kv("species_name", e.summary.speciesName);
    kv("nickname", e.summary.nickname);
    kv("ot_name", e.summary.otName);
    num("level", e.summary.level);
    num("shiny", e.summary.shiny ? 1 : 0);
    num("egg", e.summary.egg ? 1 : 0);
    kv("source_kind", e.provenance.sourceKind);
    kv("source_game", e.provenance.sourceGame);
    kv("source_label", e.provenance.sourceLabel);
    num("source_box", e.provenance.sourceBox);
    num("source_slot", e.provenance.sourceSlot);
    num("source_party", e.provenance.sourceParty ? 1 : 0);
    num("staged_edits", e.provenance.stagedEdits ? 1 : 0);
    num("deposited_at", e.provenance.depositedAt);
    kv("app_version", e.provenance.appVersion);
    return m;
}

bool decodeMeta(std::string_view text, Entry& e, std::string& error) {
    bool haveId = false, haveSha = false, haveGen = false, haveSize = false, haveSeq = false;
    std::size_t pos = 0;
    while (pos < text.size()) {
        const std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) { error = "metadata line is not terminated"; return false; }
        const std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos) { error = "metadata line has no key"; return false; }
        const std::string_view key = line.substr(0, eq);
        const std::string value = unescape(line.substr(eq + 1));
        int64_t n = 0;
        const bool isNum = parseInt(value, n);
        const auto needNum = [&]() {
            if (!isNum) error = "metadata field " + std::string(key) + " is not a number";
            return isNum;
        };
        if (key == "id") {
            const auto id = VaultId::parse(value);
            if (!id) { error = "metadata id is not a Vault ID"; return false; }
            e.id = *id; haveId = true;
        } else if (key == "sequence") { if (!needNum() || n < 0) return false; e.sequence = static_cast<uint64_t>(n); haveSeq = true; }
        else if (key == "generation") { if (!needNum() || n < 1 || n > 9) return false; e.generation = static_cast<int>(n); haveGen = true; }
        else if (key == "format") e.format = value;
        else if (key == "party") { if (!needNum()) return false; e.party = n != 0; }
        else if (key == "japanese") { if (!needNum()) return false; e.japanese = n != 0; }
        else if (key == "raw_size") { if (!needNum() || n <= 0 || n > 65535) return false; e.rawSize = static_cast<uint32_t>(n); haveSize = true; }
        else if (key == "raw_sha256") {
            if (!parseHex(value, e.rawSha256)) { error = "metadata raw_sha256 is malformed"; return false; }
            haveSha = true;
        }
        else if (key == "canonicalization") e.canonicalization = value;
        else if (key == "species") { if (!needNum() || n < 0 || n > 65535) return false; e.summary.species = static_cast<uint16_t>(n); }
        else if (key == "species_name") e.summary.speciesName = value;
        else if (key == "nickname") e.summary.nickname = value;
        else if (key == "ot_name") e.summary.otName = value;
        else if (key == "level") { if (!needNum() || n < 0 || n > 255) return false; e.summary.level = static_cast<uint8_t>(n); }
        else if (key == "shiny") { if (!needNum()) return false; e.summary.shiny = n != 0; }
        else if (key == "egg") { if (!needNum()) return false; e.summary.egg = n != 0; }
        else if (key == "source_kind") e.provenance.sourceKind = value;
        else if (key == "source_game") e.provenance.sourceGame = value;
        else if (key == "source_label") e.provenance.sourceLabel = value;
        else if (key == "source_box") { if (!needNum()) return false; e.provenance.sourceBox = static_cast<int>(n); }
        else if (key == "source_slot") { if (!needNum()) return false; e.provenance.sourceSlot = static_cast<int>(n); }
        else if (key == "source_party") { if (!needNum()) return false; e.provenance.sourceParty = n != 0; }
        else if (key == "staged_edits") { if (!needNum()) return false; e.provenance.stagedEdits = n != 0; }
        else if (key == "deposited_at") { if (!needNum()) return false; e.provenance.depositedAt = n; }
        else if (key == "app_version") e.provenance.appVersion = value;
        // Unknown keys are kept forward-compatible: ignored, never an error.
    }
    if (!haveId || !haveSha || !haveGen || !haveSize || !haveSeq) {
        if (error.empty()) error = "metadata is missing a required field";
        return false;
    }
    return true;
}

std::string pkName(int generation) {
    return "vault" + std::string(PF::plainExtension(generation));
}

// The raw bytes must be a plaintext native record of `generation`, stored exactly as given.
std::optional<PF::Decoded> decodeNative(std::span<const uint8_t> raw, int generation, std::string& error) {
    if (generation < 1 || generation > 5) {
        error = "The Vault stores Generation I-V records only in this version";
        return std::nullopt;
    }
    std::string why;
    auto decoded = PF::decode(raw, pkName(generation), why);
    if (!decoded) {
        error = "The record is not a valid Generation " + std::to_string(generation) + " Pokemon" +
                (why.empty() ? std::string() : ": " + why);
        return std::nullopt;
    }
    if (decoded->generation != generation || decoded->wasEncrypted ||
        !std::equal(decoded->plain.begin(), decoded->plain.end(), raw.begin(), raw.end())) {
        error = "The record is not an exact plaintext Generation " + std::to_string(generation) + " record";
        return std::nullopt;
    }
    return decoded;
}

std::string baseName(const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool endsWith(std::string_view s, std::string_view suffix) {
    return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

std::optional<Record> decodeRecordImpl(std::span<const uint8_t> file, std::string& error, bool* newerSchema) {
    if (newerSchema) *newerSchema = false;
    constexpr std::size_t header = 8 + 4 + 4 + 4;
    if (file.size() < header + 64) { error = "record file is truncated"; return std::nullopt; }
    if (std::memcmp(file.data(), kRecordMagic, sizeof(kRecordMagic)) != 0) {
        error = "record file has no Vault record header"; return std::nullopt;
    }
    const uint32_t schema = get32(file, 8);
    if (schema > kSchemaVersion) {
        if (newerSchema) *newerSchema = true;
        error = "record was written by a newer PokeBank NX (schema " + std::to_string(schema) + ")";
        return std::nullopt;
    }
    if (schema == 0) { error = "record schema is invalid"; return std::nullopt; }
    const uint64_t metaLen = get32(file, 12), rawLen = get32(file, 16);
    if (header + metaLen + rawLen + 64 != file.size()) { error = "record lengths do not match the file size"; return std::nullopt; }
    const Sha256 fileSha = sha256(file.first(file.size() - 32));
    if (!std::equal(fileSha.begin(), fileSha.end(), file.end() - 32)) {
        error = "record file checksum (SHA-256) does not match"; return std::nullopt;
    }
    Record record;
    const auto meta = file.subspan(header, static_cast<std::size_t>(metaLen));
    if (!decodeMeta(std::string_view(reinterpret_cast<const char*>(meta.data()), meta.size()), record.entry, error))
        return std::nullopt;
    const auto raw = file.subspan(header + static_cast<std::size_t>(metaLen), static_cast<std::size_t>(rawLen));
    const auto storedRawSha = file.subspan(file.size() - 64, 32);
    const Sha256 rawSha = sha256(raw);
    if (!std::equal(rawSha.begin(), rawSha.end(), storedRawSha.begin()) || rawSha != record.entry.rawSha256 ||
        record.entry.rawSize != rawLen) {
        error = "record Pokemon bytes do not match their SHA-256"; return std::nullopt;
    }
    if (!decodeNative(raw, record.entry.generation, error)) return std::nullopt;
    record.raw.assign(raw.begin(), raw.end());
    return record;
}

}  // namespace

// ---- ids and hashing --------------------------------------------------------------------------

Sha256 sha256(std::span<const uint8_t> bytes) {
    Utils::SHA256 h;
    if (!bytes.empty()) h.update(bytes.data(), bytes.size());
    Sha256 out{};
    h.finalize(out.data());
    return out;
}

std::string hex(std::span<const uint8_t> bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const uint8_t b : bytes) {
        out.push_back(digits[b >> 4]);
        out.push_back(digits[b & 15]);
    }
    return out;
}

std::string VaultId::text() const {
    const std::string h = hex(bytes);
    return h.substr(0, 8) + "-" + h.substr(8, 4) + "-" + h.substr(12, 4) + "-" + h.substr(16, 4) + "-" + h.substr(20);
}

std::optional<VaultId> VaultId::parse(std::string_view text) {
    if (text.size() != 36 || text[8] != '-' || text[13] != '-' || text[18] != '-' || text[23] != '-')
        return std::nullopt;
    std::string compact;
    for (std::size_t i = 0; i < text.size(); ++i)
        if (i != 8 && i != 13 && i != 18 && i != 23) compact.push_back(text[i]);
    for (const char c : compact)   // canonical text is lower case: one id, one file name
        if (c >= 'A' && c <= 'F') return std::nullopt;
    VaultId id;
    if (!parseHex(compact, id.bytes)) return std::nullopt;
    return id;
}

// ---- default file system ----------------------------------------------------------------------

Io defaultIo() {
    Io io;
    io.replace = [](const std::string& target, std::span<const uint8_t> bytes, const DF::Validator& validator) {
        return DF::replace(target, bytes, validator);
    };
    io.read = [](const std::string& path, std::size_t maxBytes, std::vector<uint8_t>& out, std::string& error) {
        out.clear();
        struct stat st{};
        if (::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) { error = "not found: " + path; return false; }
        if (static_cast<std::size_t>(st.st_size) > maxBytes) { error = "file is too large: " + path; return false; }
        std::FILE* f = std::fopen(path.c_str(), "rb");
        if (!f) { error = "could not open " + path; return false; }
        out.resize(maxBytes + 1);
        const std::size_t got = std::fread(out.data(), 1, out.size(), f);
        const bool failed = std::ferror(f) != 0;
        std::fclose(f);
        if (failed) { error = "could not read " + path; out.clear(); return false; }
        if (got > maxBytes) { error = "file is too large: " + path; out.clear(); return false; }
        out.resize(got);
        return true;
    };
    io.rename = [](const std::string& from, const std::string& to) { return std::rename(from.c_str(), to.c_str()) == 0; };
    io.remove = [](const std::string& path) { return std::remove(path.c_str()) == 0; };
    io.list = [](const std::string& dir, std::vector<std::string>& names) {
        names.clear();
        DIR* d = ::opendir(dir.c_str());
        if (!d) return false;
        while (dirent* ent = ::readdir(d)) {
            const std::string name = ent->d_name;
            if (name != "." && name != "..") names.push_back(name);
        }
        ::closedir(d);
        std::sort(names.begin(), names.end());
        return true;
    };
    io.exists = [](const std::string& path) {
        struct stat st{};
        return ::stat(path.c_str(), &st) == 0;
    };
    io.ensureDir = [](const std::string& path, std::string* error) {
        return PokeBank::Paths::ensureDirectoryTree(path, error);
    };
    io.random = [](uint8_t* out, std::size_t n) {
        // Whatever entropy the platform offers, mixed with clocks and a counter through SHA-256, so
        // a weak random_device still yields distinct ids; a collision is also checked by the Store.
        static uint64_t counter = 0;
        std::vector<uint8_t> seed;
        std::random_device rd;
        for (int i = 0; i < 8; ++i) put32(seed, rd());
        put64(seed, static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
        put64(seed, static_cast<uint64_t>(std::time(nullptr)));
        put64(seed, ++counter);
        const Sha256 h = sha256(seed);
        if (n > h.size()) return false;
        std::memcpy(out, h.data(), n);
        return true;
    };
    io.now = []() { return static_cast<int64_t>(std::time(nullptr)); };
    return io;
}

// ---- serialisation ----------------------------------------------------------------------------

std::vector<uint8_t> Store::encodeRecord(const Entry& entry, std::span<const uint8_t> raw) {
    const std::string meta = encodeMeta(entry);
    std::vector<uint8_t> out(kRecordMagic, kRecordMagic + sizeof(kRecordMagic));
    put32(out, kSchemaVersion);
    put32(out, static_cast<uint32_t>(meta.size()));
    put32(out, static_cast<uint32_t>(raw.size()));
    out.insert(out.end(), meta.begin(), meta.end());
    out.insert(out.end(), raw.begin(), raw.end());
    const Sha256 rawSha = sha256(raw);
    out.insert(out.end(), rawSha.begin(), rawSha.end());
    const Sha256 fileSha = sha256(out);
    out.insert(out.end(), fileSha.begin(), fileSha.end());
    return out;
}

std::optional<Record> Store::decodeRecord(std::span<const uint8_t> file, std::string& error) {
    return decodeRecordImpl(file, error, nullptr);
}

std::vector<uint8_t> Store::encodeIndex(const std::vector<Entry>& entries, uint64_t nextSequence) {
    std::vector<uint8_t> out(kIndexMagic, kIndexMagic + sizeof(kIndexMagic));
    put32(out, kSchemaVersion);
    put32(out, static_cast<uint32_t>(entries.size()));
    put64(out, nextSequence);
    for (const auto& e : entries) {
        const std::string meta = encodeMeta(e);
        put32(out, static_cast<uint32_t>(meta.size()));
        out.insert(out.end(), meta.begin(), meta.end());
    }
    const Sha256 h = sha256(out);
    out.insert(out.end(), h.begin(), h.end());
    return out;
}

bool Store::decodeIndex(std::span<const uint8_t> file, std::vector<Entry>& entries, uint64_t& nextSeq,
                        uint32_t& schema, std::string& error) {
    entries.clear();
    schema = 0;
    constexpr std::size_t header = 8 + 4 + 4 + 8;
    if (file.size() < header + 32) { error = "index is truncated"; return false; }
    if (std::memcmp(file.data(), kIndexMagic, sizeof(kIndexMagic)) != 0) { error = "index has no Vault header"; return false; }
    schema = get32(file, 8);
    if (schema > kSchemaVersion) { error = "index was written by a newer PokeBank NX"; return false; }
    const Sha256 h = sha256(file.first(file.size() - 32));
    if (!std::equal(h.begin(), h.end(), file.end() - 32)) { error = "index checksum (SHA-256) does not match"; return false; }
    const uint32_t count = get32(file, 12);
    nextSeq = get64(file, 16);
    std::size_t pos = header;
    const std::size_t end = file.size() - 32;
    for (uint32_t i = 0; i < count; ++i) {
        if (pos + 4 > end) { error = "index entry table is truncated"; return false; }
        const uint32_t len = get32(file, pos);
        pos += 4;
        if (len > end - pos) { error = "index entry is truncated"; return false; }
        Entry e;
        if (!decodeMeta(std::string_view(reinterpret_cast<const char*>(file.data() + pos), len), e, error)) return false;
        pos += len;
        entries.push_back(std::move(e));
    }
    if (pos != end) { error = "index has trailing bytes"; return false; }
    return true;
}

// ---- store ------------------------------------------------------------------------------------

Store::Store(Io ioIn) : io(std::move(ioIn)) {}

std::string Store::recordPath(const VaultId& id) const {
    return rootPath + "/records/" + id.text() + std::string(kRecordExtension);
}
std::string Store::journalPath(const VaultId& id) const {
    return rootPath + "/journal/" + id.text() + std::string(kJournalExtension);
}
std::string Store::indexPath() const { return rootPath + "/" + std::string(kIndexName); }
std::string Store::quarantineDir() const { return rootPath + "/quarantine"; }

const Entry* Store::find(const VaultId& id) const noexcept {
    for (const auto& e : list)
        if (e.id == id) return &e;
    return nullptr;
}

bool Store::quarantine(const std::string& path, std::string_view why, OpenReport* report) {
    const std::string base = quarantineDir() + "/" + baseName(path);
    for (int n = 0; n < 100000; ++n) {
        const std::string target = base + "." + std::to_string(n);
        if (io.exists(target)) continue;
        if (!io.rename(path, target)) return false;
        if (report) {
            ++report->quarantined;
            report->notes.push_back("Quarantined " + baseName(path) + ": " + std::string(why));
        }
        return true;
    }
    return false;
}

bool Store::writeIndex(const std::vector<Entry>& entries, uint64_t nextSeq, std::string& error) {
    const auto bytes = encodeIndex(entries, nextSeq);
    const std::size_t expected = entries.size();
    const auto result = io.replace(indexPath(), bytes, [expected](std::span<const uint8_t> written, std::string& why) {
        std::vector<Entry> again;
        uint64_t seq = 0;
        uint32_t schema = 0;
        if (!Store::decodeIndex(written, again, seq, schema, why)) return false;
        if (again.size() != expected) { why = "index entry count changed"; return false; }
        return true;
    });
    if (!result.ok) {
        error = "The Vault index could not be written: " + (result.error.empty() ? std::string("write error") : result.error);
        return false;
    }
    return true;
}

void Store::pruneIndexGenerations() {
    // The index is a cache rebuilt from records/, so only the newest preserved generation is kept.
    std::vector<std::string> names;
    if (!io.list(rootPath, names)) return;
    const std::string previous = std::string(kIndexName) + ".previous.";
    int64_t newest = -1;
    for (const auto& n : names) {
        int64_t g = 0;
        if (n.rfind(previous, 0) == 0 && parseInt(std::string_view(n).substr(previous.size()), g)) newest = std::max(newest, g);
    }
    const std::string temp = std::string(kIndexName) + ".tmp.";
    const std::string failed = std::string(kIndexName) + ".failed.";
    for (const auto& n : names) {
        int64_t g = 0;
        if ((n.rfind(previous, 0) == 0 && parseInt(std::string_view(n).substr(previous.size()), g) && g != newest) ||
            n.rfind(temp, 0) == 0 || n.rfind(failed, 0) == 0)
            (void)io.remove(rootPath + "/" + n);
    }
}

bool Store::rebuildFromRecords(std::vector<Entry>& out, uint64_t& nextSeq, OpenReport* report) {
    out.clear();
    std::vector<std::string> names;
    if (!io.list(rootPath + "/records", names)) return false;
    for (const auto& name : names) {
        if (!endsWith(name, kRecordExtension)) continue;
        const auto id = VaultId::parse(std::string_view(name).substr(0, name.size() - kRecordExtension.size()));
        if (!id) continue;
        const std::string path = rootPath + "/records/" + name;
        std::vector<uint8_t> bytes;
        std::string error;
        bool newer = false;
        std::optional<Record> rec;
        if (io.read(path, kMaxRecordFileBytes, bytes, error)) rec = decodeRecordImpl(bytes, error, &newer);
        if (newer) {
            readOnlyMode = true;
            if (report) report->notes.push_back("A record was written by a newer PokeBank NX; the Vault is read-only");
            continue;
        }
        if (!rec || !(rec->entry.id == *id)) {
            (void)quarantine(path, rec ? "file name does not match its Vault ID" : error, report);
            continue;
        }
        nextSeq = std::max(nextSeq, rec->entry.sequence + 1);
        out.push_back(std::move(rec->entry));
    }
    std::sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) { return a.sequence < b.sequence; });
    return true;
}

bool Store::open(const std::string& root, OpenReport* report, std::string& error) {
    opened = false;
    readOnlyMode = false;
    list.clear();
    nextSequence = 1;
    rootPath = root;
    while (rootPath.size() > 1 && rootPath.back() == '/') rootPath.pop_back();
    OpenReport local;
    OpenReport& rep = report ? *report : local;
    rep = {};
    if (rootPath.empty()) { error = "The Vault folder is not set"; return false; }
    for (const char* sub : {"", "/records", "/journal", "/quarantine"}) {
        std::string why;
        if (!io.ensureDir(rootPath + sub, &why)) {
            error = "The Vault folder could not be created: " + (why.empty() ? rootPath + sub : why);
            return false;
        }
    }

    // 1. Index (a cache): load it, or rebuild it from the authoritative records.
    bool dirty = false;
    if (io.exists(indexPath())) {
        std::vector<uint8_t> bytes;
        std::string why;
        uint32_t schema = 0;
        uint64_t seq = 1;
        if (io.read(indexPath(), kMaxIndexFileBytes, bytes, why) && decodeIndex(bytes, list, seq, schema, why)) {
            nextSequence = std::max<uint64_t>(seq, 1);
        } else if (schema > kSchemaVersion) {
            readOnlyMode = true;
            rep.readOnly = true;
            rep.notes.push_back("The Vault was written by a newer PokeBank NX; it is read-only here");
            list.clear();
        } else {
            if (!quarantine(indexPath(), why, &rep)) {
                error = "The damaged Vault index could not be set aside: " + why;
                return false;
            }
            if (!rebuildFromRecords(list, nextSequence, &rep)) { error = "The Vault records could not be listed"; return false; }
            rep.indexRebuilt = true;
            dirty = true;
        }
    } else {
        if (!rebuildFromRecords(list, nextSequence, &rep)) { error = "The Vault records could not be listed"; return false; }
        rep.createdNew = list.empty();
        rep.indexRebuilt = !list.empty();
        dirty = !list.empty() || !io.exists(indexPath());
    }
    if (readOnlyMode) rep.readOnly = true;

    const auto indexed = [this](const VaultId& id) { return find(id) != nullptr; };
    const auto adopt = [&](const VaultId& id, const std::string& path) -> bool {
        std::vector<uint8_t> bytes;
        std::string why;
        bool newer = false;
        std::optional<Record> rec;
        if (io.read(path, kMaxRecordFileBytes, bytes, why)) rec = decodeRecordImpl(bytes, why, &newer);
        if (newer) { readOnlyMode = rep.readOnly = true; return false; }
        if (!rec || !(rec->entry.id == id)) {
            (void)quarantine(path, rec ? "file name does not match its Vault ID" : why, &rep);
            return false;
        }
        nextSequence = std::max(nextSequence, rec->entry.sequence + 1);
        list.push_back(std::move(rec->entry));
        dirty = true;
        return true;
    };

    // 2. Interrupted deposits: roll forward only from a record that re-reads and verifies.
    std::vector<std::string> recordNames;
    (void)io.list(rootPath + "/records", recordNames);
    std::vector<std::string> journalNames;
    (void)io.list(rootPath + "/journal", journalNames);
    for (const auto& name : journalNames) {
        const std::string path = rootPath + "/journal/" + name;
        const auto id = endsWith(name, kJournalExtension)
            ? VaultId::parse(std::string_view(name).substr(0, name.size() - kJournalExtension.size()))
            : std::nullopt;
        if (!id) { (void)quarantine(path, "not a Vault journal", &rep); continue; }
        if (readOnlyMode) continue;
        // Leftover temp / failed generations of this record were never published: set them aside.
        const std::string stem = id->text() + std::string(kRecordExtension) + ".";
        for (const auto& r : recordNames)
            if (r.rfind(stem, 0) == 0) (void)quarantine(rootPath + "/records/" + r, "unpublished write", &rep);
        if (indexed(*id)) {
            // committed; only the journal removal was interrupted
        } else if (io.exists(recordPath(*id))) {
            if (adopt(*id, recordPath(*id))) ++rep.rolledForward;
            else ++rep.rolledBack;
        } else {
            ++rep.rolledBack;
        }
        (void)io.remove(path);
    }

    // 3. Records the index does not know (e.g. an index rebuilt from an older generation).
    recordNames.clear();
    (void)io.list(rootPath + "/records", recordNames);
    for (const auto& name : recordNames) {
        const std::string path = rootPath + "/records/" + name;
        const auto id = endsWith(name, kRecordExtension)
            ? VaultId::parse(std::string_view(name).substr(0, name.size() - kRecordExtension.size()))
            : std::nullopt;
        if (!id) {
            if (!readOnlyMode) (void)quarantine(path, "not a Vault record", &rep);
            continue;
        }
        if (indexed(*id) || readOnlyMode) continue;
        if (adopt(*id, path)) ++rep.rolledForward;
    }

    // 4. Indexed entries whose record file has gone are not listed (never invented).
    std::vector<Entry> present;
    for (auto& e : list) {
        if (io.exists(recordPath(e.id))) {
            present.push_back(std::move(e));
        } else {
            ++rep.missingRecords;
            rep.notes.push_back("Vault record missing: " + e.id.text());
            dirty = true;
        }
    }
    list = std::move(present);
    std::sort(list.begin(), list.end(), [](const Entry& a, const Entry& b) { return a.sequence < b.sequence; });
    for (const auto& e : list) nextSequence = std::max(nextSequence, e.sequence + 1);

    if (dirty && !readOnlyMode) {
        std::string why;
        if (!writeIndex(list, nextSequence, why)) rep.notes.push_back(why);   // records stay authoritative
        else pruneIndexGenerations();
    }
    opened = true;
    error.clear();
    return true;
}

std::optional<VaultId> Store::newId() {
    for (int attempt = 0; attempt < 16; ++attempt) {
        VaultId id;
        if (!io.random || !io.random(id.bytes.data(), id.bytes.size())) return std::nullopt;
        id.bytes[6] = static_cast<uint8_t>((id.bytes[6] & 0x0F) | 0x40);   // UUID version 4
        id.bytes[8] = static_cast<uint8_t>((id.bytes[8] & 0x3F) | 0x80);   // RFC 4122 variant
        if (!find(id) && !io.exists(recordPath(id)) && !io.exists(journalPath(id))) return id;
    }
    return std::nullopt;
}

std::optional<VaultId> Store::deposit(const DepositRequest& request, std::string& error) {
    if (!opened) { error = "The Vault is not open"; return std::nullopt; }
    if (readOnlyMode) { error = "This Vault was written by a newer PokeBank NX and is read-only here"; return std::nullopt; }
    const auto decoded = decodeNative(request.raw, request.generation, error);
    if (!decoded) return std::nullopt;
    const auto id = newId();
    if (!id) { error = "A new Vault ID could not be created"; return std::nullopt; }

    Entry entry;
    entry.id = *id;
    entry.sequence = nextSequence;
    entry.generation = request.generation;
    entry.format = std::string(PF::plainExtension(request.generation).substr(1));
    entry.party = decoded->party;
    entry.japanese = decoded->japanese;
    entry.rawSize = static_cast<uint32_t>(request.raw.size());
    entry.rawSha256 = sha256(request.raw);
    entry.summary = request.summary;
    entry.provenance = request.provenance;
    if (entry.provenance.depositedAt == 0 && io.now) entry.provenance.depositedAt = io.now();
    const auto recordBytes = encodeRecord(entry, request.raw);

    // 1. Journal first: a crash from here on is recoverable on the next open.
    const std::string journalText = std::string(kJournalHeader) + "deposit " + id->text() + " " + hex(entry.rawSha256) + "\n";
    const std::span<const uint8_t> journalBytes(reinterpret_cast<const uint8_t*>(journalText.data()), journalText.size());
    const auto journal = io.replace(journalPath(*id), journalBytes, [&journalText](std::span<const uint8_t> w, std::string& why) {
        if (w.size() != journalText.size() || std::memcmp(w.data(), journalText.data(), w.size()) != 0) {
            why = "journal did not read back";
            return false;
        }
        return true;
    });
    const auto setAsideLeftovers = [&]() {
        // Temp / failed generations of THIS deposit were never published: quarantine, never delete.
        for (const auto& [dir, stem] : {std::pair<std::string, std::string>{rootPath + "/records", id->text() + std::string(kRecordExtension) + "."},
                                        std::pair<std::string, std::string>{rootPath + "/journal", id->text() + std::string(kJournalExtension) + "."}}) {
            std::vector<std::string> names;
            if (io.list(dir, names))
                for (const auto& n : names)
                    if (n.rfind(stem, 0) == 0) (void)quarantine(dir + "/" + n, "unpublished write", nullptr);
        }
    };
    if (!journal.ok) {
        setAsideLeftovers();
        error = "Nothing was copied: the Vault journal could not be written (" +
                (journal.error.empty() ? std::string("write error") : journal.error) + ")";
        return std::nullopt;
    }
    if (crashed(Step::JournalWritten)) { error = "interrupted"; return std::nullopt; }

    const auto rollbackUnpublished = [&]() {
        setAsideLeftovers();
        (void)io.remove(journalPath(*id));
        pruneIndexGenerations();
    };

    // 2. The record: written once, never over an existing file, reread and verified.
    if (io.exists(recordPath(*id))) {
        (void)io.remove(journalPath(*id));
        error = "Nothing was copied: a Vault record with this ID already exists";
        return std::nullopt;
    }
    const VaultId expectId = *id;
    const Sha256 expectSha = entry.rawSha256;
    const auto published = io.replace(recordPath(*id), recordBytes,
        [expectId, expectSha](std::span<const uint8_t> written, std::string& why) {
            const auto rec = decodeRecordImpl(written, why, nullptr);
            if (!rec) return false;
            if (!(rec->entry.id == expectId) || rec->entry.rawSha256 != expectSha) { why = "record identity changed"; return false; }
            return true;
        });
    if (!published.ok) {
        rollbackUnpublished();
        error = "Nothing was copied: the Vault record could not be written (" +
                (published.error.empty() ? std::string("write error") : published.error) + ")";
        return std::nullopt;
    }
    if (crashed(Step::RecordPublished)) { error = "interrupted"; return std::nullopt; }

    // 3. The index (cache). On failure the published record is set aside, never deleted.
    std::vector<Entry> next = list;
    next.push_back(entry);
    std::string why;
    if (!writeIndex(next, nextSequence + 1, why)) {
        (void)quarantine(recordPath(*id), "deposit rolled back: " + why, nullptr);
        rollbackUnpublished();
        error = "Nothing was copied: " + why;
        return std::nullopt;
    }
    if (crashed(Step::IndexPublished)) { error = "interrupted"; return std::nullopt; }

    // 4. Commit.
    (void)io.remove(journalPath(*id));   // a leftover journal is resolved as committed on next open
    list = std::move(next);
    ++nextSequence;
    pruneIndexGenerations();
    error.clear();
    return id;
}

std::optional<Record> Store::read(const VaultId& id, std::string& error) const {
    if (!opened) { error = "The Vault is not open"; return std::nullopt; }
    const Entry* entry = find(id);
    if (!entry) { error = "This Pokemon is not in the Vault"; return std::nullopt; }
    std::vector<uint8_t> bytes;
    std::string why;
    if (!io.read(recordPath(id), kMaxRecordFileBytes, bytes, why)) {
        error = "The Vault record could not be read: " + why;
        return std::nullopt;
    }
    auto rec = decodeRecordImpl(bytes, why, nullptr);
    if (!rec) { error = "The Vault record failed verification: " + why; return std::nullopt; }
    if (!(rec->entry.id == id) || rec->entry.rawSha256 != entry->rawSha256 || rec->entry.generation != entry->generation) {
        error = "The Vault record does not match its index entry";
        return std::nullopt;
    }
    error.clear();
    return rec;
}

std::optional<PF::Decoded> Store::withdrawCopy(const VaultId& id, int targetGeneration, std::string& error) const {
    const auto rec = read(id, error);
    if (!rec) return std::nullopt;
    if (rec->entry.generation != targetGeneration) {
        error = "This is a Generation " + std::to_string(rec->entry.generation) +
                " Pokemon; the Vault does not convert between generations yet";
        return std::nullopt;
    }
    return decodeNative(rec->raw, targetGeneration, error);
}

}  // namespace PokeBank::Vault
