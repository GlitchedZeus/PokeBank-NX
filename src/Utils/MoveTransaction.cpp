#include "Utils/MoveTransaction.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <utility>

#include "Utils/PokeBankPaths.h"

namespace PokeBank::Storage::MoveTx {
namespace {

constexpr size_t kMaxJournalBytes = 1024u * 1024u;
constexpr size_t kMaxEvidenceBytes = 128u * 1024u * 1024u;
constexpr size_t kMaxStringBytes = 128;
constexpr std::array<uint8_t, 8> kMagic{'P','B','M','T','X','0','1',0};

uint32_t rotr(uint32_t x, uint32_t n) noexcept {
    return (x >> n) | (x << (32 - n));
}

void shaBlock(const uint8_t* block, std::array<uint32_t, 8>& h) noexcept {
    static constexpr uint32_t k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    uint32_t w[64]{};
    for (int i = 0; i < 16; ++i) {
        const int o = i * 4;
        w[i] = (static_cast<uint32_t>(block[o]) << 24) |
               (static_cast<uint32_t>(block[o + 1]) << 16) |
               (static_cast<uint32_t>(block[o + 2]) << 8) |
               static_cast<uint32_t>(block[o + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        const uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
        const uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
    for (int i = 0; i < 64; ++i) {
        const uint32_t s1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
        const uint32_t ch = (e & f) ^ ((~e) & g);
        const uint32_t t1 = hh + s1 + ch + k[i] + w[i];
        const uint32_t s0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t t2 = s0 + maj;
        hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
}

bool safeText(std::string_view text) noexcept {
    if (text.empty() || text.size() > kMaxStringBytes) return false;
    for (const unsigned char c : text) {
        if (c < 0x20 || c > 0x7e) return false;
    }
    return true;
}

bool validAccountComponent(std::string_view value) noexcept {
    constexpr std::string_view prefix = "account-";
    if (value.size() != prefix.size() + 32 || value.substr(0, prefix.size()) != prefix) return false;
    for (size_t i = prefix.size(); i < value.size(); ++i) {
        const char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

bool validState(State state) noexcept {
    const auto v = static_cast<uint8_t>(state);
    return v >= static_cast<uint8_t>(State::Prepared) &&
           v <= static_cast<uint8_t>(State::Committed);
}

bool validTransactionId(std::string_view id) noexcept {
    return id.rfind("tx-", 0) == 0 && id.size() <= 96 && PokeBank::Paths::isSafeComponent(id);
}

bool validTransaction(const Transaction& tx, std::string& error) {
    if (tx.schemaVersion < kOldestSupportedSchemaVersion || tx.schemaVersion > kSchemaVersion) {
        error = "unsupported transaction schema";
        return false;
    }
    if (tx.schemaVersion == 1 && tx.crossGameConversion) {
        error = "v1 transaction cannot require cross-game conversion evidence";
        return false;
    }
    if (!validTransactionId(tx.id) || !validState(tx.state)) {
        error = "invalid transaction identity/state";
        return false;
    }
    if (!tx.source.valid() || !tx.destination.valid() || tx.source == tx.destination) {
        error = "invalid or identical store descriptors";
        return false;
    }
    if (tx.moves.size() > kMaxMoves) {
        error = "too many move records";
        return false;
    }
    for (const auto& move : tx.moves) {
        if (!safeText(move.sourceSlot) || !safeText(move.destinationSlot)) {
            error = "invalid move slot descriptor";
            return false;
        }
    }
    return true;
}

void appendU8(std::vector<uint8_t>& out, uint8_t value) { out.push_back(value); }
void appendU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
}
bool appendString(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > kMaxStringBytes) return false;
    appendU16(out, static_cast<uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}
void appendDigest(std::vector<uint8_t>& out, const Digest& digest) {
    out.insert(out.end(), digest.begin(), digest.end());
}

struct Reader {
    std::span<const uint8_t> bytes;
    size_t pos = 0;
    bool u8(uint8_t& out) {
        if (pos + 1 > bytes.size()) return false;
        out = bytes[pos++];
        return true;
    }
    bool u16(uint16_t& out) {
        if (pos + 2 > bytes.size()) return false;
        out = static_cast<uint16_t>(bytes[pos]) |
              (static_cast<uint16_t>(bytes[pos + 1]) << 8);
        pos += 2;
        return true;
    }
    bool str(std::string& out) {
        uint16_t n = 0;
        if (!u16(n) || n > kMaxStringBytes || pos + n > bytes.size()) return false;
        out.assign(reinterpret_cast<const char*>(bytes.data() + pos), n);
        pos += n;
        return true;
    }
    bool digest(Digest& out) {
        if (pos + out.size() > bytes.size()) return false;
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(pos), out.size(), out.begin());
        pos += out.size();
        return true;
    }
};

bool appendDescriptor(std::vector<uint8_t>& out, const StoreDescriptor& d) {
    appendU8(out, static_cast<uint8_t>(d.type));
    return appendString(out, d.profile) && appendString(out, d.gameId) &&
           appendString(out, d.workspace) && appendString(out, d.fileId);
}

bool readDescriptor(Reader& r, StoreDescriptor& d) {
    uint8_t type = 0;
    if (!r.u8(type)) return false;
    if (type < static_cast<uint8_t>(StoreType::Bank) ||
        type > static_cast<uint8_t>(StoreType::MutableWorkspaceFileSet)) return false;
    d.type = static_cast<StoreType>(type);
    return r.str(d.profile) && r.str(d.gameId) && r.str(d.workspace) && r.str(d.fileId);
}

std::vector<uint8_t> serialize(const Transaction& tx, std::string& error) {
    if (!validTransaction(tx, error)) return {};
    std::vector<uint8_t> out;
    out.reserve(512 + tx.moves.size() * 128);
    out.insert(out.end(), kMagic.begin(), kMagic.end());
    appendU16(out, tx.schemaVersion);
    appendU8(out, static_cast<uint8_t>(tx.state));
    appendU8(out, tx.crossGameConversion ? 0x01u : 0x00u);
    if (!appendString(out, tx.id) ||
        !appendDescriptor(out, tx.source) ||
        !appendDescriptor(out, tx.destination)) {
        error = "journal metadata is too long";
        return {};
    }
    appendDigest(out, tx.sourceBefore);
    appendDigest(out, tx.sourceRetired);
    appendDigest(out, tx.destinationBefore);
    appendDigest(out, tx.destinationAfter);
    appendU16(out, static_cast<uint16_t>(tx.moves.size()));
    for (const auto& move : tx.moves) {
        if (!appendString(out, move.sourceSlot) || !appendString(out, move.destinationSlot)) {
            error = "move metadata is too long";
            return {};
        }
        appendDigest(out, move.sourcePayload);
        appendDigest(out, move.destinationPayload);
        appendU16(out, move.species);
        appendU8(out, move.form);
        appendU8(out, 0);
    }
    const Digest integrity = sha256(out);
    appendDigest(out, integrity);
    if (out.size() > kMaxJournalBytes) {
        error = "journal exceeds maximum supported size";
        return {};
    }
    return out;
}

LoadResult parse(std::span<const uint8_t> bytes) {
    LoadResult result;
    result.status = LoadStatus::Corrupt;
    if (bytes.size() < kMagic.size() + 2 + 32) {
        result.error = "journal is truncated";
        return result;
    }
    if (!std::equal(kMagic.begin(), kMagic.end(), bytes.begin())) {
        result.error = "journal magic mismatch";
        return result;
    }
    const uint16_t version = static_cast<uint16_t>(bytes[8]) |
                             (static_cast<uint16_t>(bytes[9]) << 8);
    if (version < kOldestSupportedSchemaVersion || version > kSchemaVersion) {
        result.status = LoadStatus::UnsupportedVersion;
        result.error = "journal schema is newer/unsupported";
        return result;
    }
    if (bytes.size() > kMaxJournalBytes || bytes.size() < 32) {
        result.error = "journal size is invalid";
        return result;
    }
    const size_t bodySize = bytes.size() - 32;
    Digest stored{};
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(bodySize), stored.size(), stored.begin());
    if (sha256(bytes.first(bodySize)) != stored) {
        result.error = "journal integrity hash mismatch";
        return result;
    }

    Reader r{bytes.first(bodySize)};
    r.pos = 10;
    uint8_t state = 0, flags = 0;
    Transaction tx;
    tx.schemaVersion = version;
    if (!r.u8(state) || !r.u8(flags) ||
        (version == 1 && flags != 0) || (version >= 2 && (flags & ~0x01u) != 0) ||
        !r.str(tx.id) ||
        !readDescriptor(r, tx.source) || !readDescriptor(r, tx.destination) ||
        !r.digest(tx.sourceBefore) || !r.digest(tx.sourceRetired) ||
        !r.digest(tx.destinationBefore) || !r.digest(tx.destinationAfter)) {
        result.error = "journal metadata is malformed";
        return result;
    }
    tx.state = static_cast<State>(state);
    tx.crossGameConversion = version >= 2 && (flags & 0x01u) != 0;
    uint16_t moveCount = 0;
    if (!r.u16(moveCount) || moveCount > kMaxMoves) {
        result.error = "journal move count is invalid";
        return result;
    }
    tx.moves.reserve(moveCount);
    for (uint16_t i = 0; i < moveCount; ++i) {
        MoveRecord move;
        uint8_t pad = 0;
        if (!r.str(move.sourceSlot) || !r.str(move.destinationSlot) ||
            !r.digest(move.sourcePayload) || !r.digest(move.destinationPayload) ||
            !r.u16(move.species) || !r.u8(move.form) || !r.u8(pad) || pad != 0) {
            result.error = "journal move record is malformed";
            return result;
        }
        tx.moves.push_back(std::move(move));
    }
    if (r.pos != bodySize) {
        result.error = "journal has trailing or unparsed data";
        return result;
    }
    std::string metadataError;
    if (!validTransaction(tx, metadataError)) {
        result.error = metadataError;
        return result;
    }
    result.status = LoadStatus::Ok;
    result.transaction = std::move(tx);
    return result;
}

bool readFile(const std::string& path, size_t maxBytes, std::vector<uint8_t>& out, std::string& error) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        error = "open failed: " + std::string(std::strerror(errno));
        return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
        error = "seek failed";
        std::fclose(f);
        return false;
    }
    const long size = std::ftell(f);
    if (size < 0 || static_cast<size_t>(size) > maxBytes || std::fseek(f, 0, SEEK_SET) != 0) {
        error = "file size is invalid or unsupported";
        std::fclose(f);
        return false;
    }
    out.assign(static_cast<size_t>(size), 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size() || !closeOk) {
        error = "file read/close failed";
        return false;
    }
    return true;
}

bool exists(const std::string& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

RecoveryResult result(RecoveryStatus status, State state, std::string error = {}) {
    return RecoveryResult{status, state, std::move(error)};
}

} // namespace

Digest sha256(std::span<const uint8_t> bytes) noexcept {
    std::array<uint32_t, 8> h{
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    size_t offset = 0;
    while (offset + 64 <= bytes.size()) {
        shaBlock(bytes.data() + offset, h);
        offset += 64;
    }
    std::array<uint8_t, 128> tail{};
    const size_t rem = bytes.size() - offset;
    if (rem) std::copy_n(bytes.data() + offset, rem, tail.data());
    tail[rem] = 0x80;
    const size_t totalTail = rem < 56 ? 64 : 128;
    const uint64_t bits = static_cast<uint64_t>(bytes.size()) * 8u;
    for (int i = 0; i < 8; ++i)
        tail[totalTail - 1 - static_cast<size_t>(i)] =
            static_cast<uint8_t>((bits >> (i * 8)) & 0xff);
    shaBlock(tail.data(), h);
    if (totalTail == 128) shaBlock(tail.data() + 64, h);

    Digest out{};
    for (size_t i = 0; i < h.size(); ++i) {
        out[i*4] = static_cast<uint8_t>((h[i] >> 24) & 0xff);
        out[i*4+1] = static_cast<uint8_t>((h[i] >> 16) & 0xff);
        out[i*4+2] = static_cast<uint8_t>((h[i] >> 8) & 0xff);
        out[i*4+3] = static_cast<uint8_t>(h[i] & 0xff);
    }
    return out;
}

std::string digestHex(const Digest& digest) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.resize(64);
    for (size_t i = 0; i < digest.size(); ++i) {
        out[i*2] = hex[digest[i] >> 4];
        out[i*2+1] = hex[digest[i] & 0x0f];
    }
    return out;
}

bool StoreDescriptor::valid() const noexcept {
    if (type == StoreType::Bank) {
        return profile.empty() && gameId.empty() && workspace.empty() && fileId == "bank.dat";
    }
    if (type != StoreType::MutableWorkspaceSingleFile &&
        type != StoreType::MutableWorkspaceFileSet) return false;
    return validAccountComponent(profile) &&
           PokeBank::Paths::isSafeComponent(gameId) &&
           PokeBank::Paths::isSafeComponent(workspace) &&
           PokeBank::Paths::isSafeComponent(fileId);
}

bool StoreDescriptor::operator==(const StoreDescriptor& other) const noexcept {
    return type == other.type && profile == other.profile && gameId == other.gameId &&
           workspace == other.workspace && fileId == other.fileId;
}

Journal::Journal(std::string root)
    : root_(std::move(root)), recordsRoot_(root_ + "/records") {}

bool Journal::ensure(std::string& error) const {
    return PokeBank::Paths::ensureDirectoryTree(recordsRoot_, &error);
}

std::string Journal::journalPath(const std::string& id) const {
    return validTransactionId(id) ? recordsRoot_ + "/" + id + ".pbtx" : std::string{};
}
std::string Journal::destinationEvidencePath(const std::string& id) const {
    return validTransactionId(id) ? recordsRoot_ + "/" + id + ".destination.bin" : std::string{};
}
std::string Journal::sourceRetiredEvidencePath(const std::string& id) const {
    return validTransactionId(id) ? recordsRoot_ + "/" + id + ".source-retired.bin" : std::string{};
}

bool Journal::idAvailable(const std::string& id) const {
    const auto j = journalPath(id), d = destinationEvidencePath(id), s = sourceRetiredEvidencePath(id);
    return !j.empty() && !exists(j) && !exists(d) && !exists(s);
}

std::string Journal::allocateTransactionId(std::string& error) const {
    if (!ensure(error)) return {};
    for (uint64_t n = 1; n < 1000000000ull; ++n) {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "tx-%016llx",
                      static_cast<unsigned long long>(n));
        const std::string id(buffer);
        if (idAvailable(id)) return id;
    }
    error = "transaction ID space exhausted";
    return {};
}

bool Journal::persist(const Transaction& transaction, std::string& error) const {
    if (!ensure(error)) return false;
    const std::string path = journalPath(transaction.id);
    if (path.empty()) {
        error = "invalid transaction ID";
        return false;
    }
    auto bytes = serialize(transaction, error);
    if (bytes.empty()) return false;
    const auto validator = [&](std::span<const uint8_t> candidate, std::string& validationError) {
        const auto loaded = parse(candidate);
        if (loaded.status != LoadStatus::Ok ||
            loaded.transaction.id != transaction.id ||
            loaded.transaction.state != transaction.state) {
            validationError = loaded.error.empty() ? "journal round-trip mismatch" : loaded.error;
            return false;
        }
        return true;
    };
    const auto durable = PokeBank::Storage::DurableFile::replace(path, bytes, validator);
    if (!durable.ok) {
        error = durable.error;
        return false;
    }
    return true;
}

LoadResult Journal::load(const std::string& id) const {
    LoadResult result;
    const std::string path = journalPath(id);
    if (path.empty()) {
        result.status = LoadStatus::Corrupt;
        result.error = "invalid transaction ID";
        return result;
    }
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) {
        result.status = errno == ENOENT ? LoadStatus::NotFound : LoadStatus::Corrupt;
        result.error = errno == ENOENT ? "journal not found" : "journal stat failed";
        return result;
    }
    std::vector<uint8_t> bytes;
    if (!readFile(path, kMaxJournalBytes, bytes, result.error)) {
        result.status = LoadStatus::Corrupt;
        return result;
    }
    return parse(bytes);
}

std::vector<ScanEntry> Journal::scan() const {
    std::vector<ScanEntry> out;
    DIR* dir = ::opendir(recordsRoot_.c_str());
    if (!dir) return out;
    while (dirent* ent = ::readdir(dir)) {
        const std::string name = ent->d_name;
        constexpr std::string_view suffix = ".pbtx";
        if (name.size() <= suffix.size() ||
            name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
        const std::string id = name.substr(0, name.size() - suffix.size());
        const auto loaded = load(id);
        ScanEntry entry;
        entry.id = id;
        entry.status = loaded.status;
        entry.state = loaded.transaction.state;
        entry.error = loaded.error;
        out.push_back(std::move(entry));
    }
    ::closedir(dir);
    std::sort(out.begin(), out.end(), [](const ScanEntry& a, const ScanEntry& b) {
        return a.id < b.id;
    });
    return out;
}

bool Journal::storeEvidence(const Transaction& transaction,
                            std::span<const uint8_t> destinationAfter,
                            std::span<const uint8_t> sourceRetired,
                            std::string& error) const {
    if (!ensure(error) || !idAvailable(transaction.id)) {
        if (error.empty()) error = "transaction ID/evidence already exists";
        return false;
    }
    const auto writeOne = [&](const std::string& path, std::span<const uint8_t> bytes,
                              const Digest& expected) {
        const auto validator = [&](std::span<const uint8_t> candidate, std::string& e) {
            if (sha256(candidate) != expected) {
                e = "recovery evidence hash mismatch";
                return false;
            }
            return true;
        };
        const auto durable = PokeBank::Storage::DurableFile::replace(path, bytes, validator);
        if (!durable.ok) {
            error = durable.error;
            return false;
        }
        return true;
    };
    if (!writeOne(destinationEvidencePath(transaction.id), destinationAfter,
                  transaction.destinationAfter)) return false;
    if (!writeOne(sourceRetiredEvidencePath(transaction.id), sourceRetired,
                  transaction.sourceRetired)) return false;
    return true;
}

bool Journal::loadEvidence(const Transaction& transaction,
                           std::vector<uint8_t>& destinationAfter,
                           std::vector<uint8_t>& sourceRetired,
                           std::string& error) const {
    if (!readFile(destinationEvidencePath(transaction.id), kMaxEvidenceBytes,
                  destinationAfter, error) ||
        !readFile(sourceRetiredEvidencePath(transaction.id), kMaxEvidenceBytes,
                  sourceRetired, error)) return false;
    if (sha256(destinationAfter) != transaction.destinationAfter ||
        sha256(sourceRetired) != transaction.sourceRetired) {
        error = "transaction recovery evidence failed SHA-256 verification";
        return false;
    }
    return true;
}

Engine::Engine()
    : journal_(PokeBank::Paths::transactionsRoot()) {}

Engine::Engine(std::string root, PersistGate persistGate, RetirementGate retirementGate)
    : journal_(std::move(root)),
      persistGate_(std::move(persistGate)),
      retirementGate_(std::move(retirementGate)) {}

bool Engine::persistState(Transaction& transaction, State next, std::string& error) {
    if (static_cast<uint8_t>(next) != static_cast<uint8_t>(transaction.state) + 1u) {
        error = "refusing non-sequential transaction state transition";
        return false;
    }
    if (persistGate_ && !persistGate_(next)) {
        error = "journal persistence rejected by injected gate";
        return false;
    }
    const State previous = transaction.state;
    transaction.state = next;
    if (!journal_.persist(transaction, error)) {
        transaction.state = previous;
        return false;
    }
    return true;
}

bool Engine::prepare(Transaction& transaction,
                     Store& source,
                     Store& destination,
                     std::span<const uint8_t> destinationAfter,
                     std::span<const uint8_t> sourceRetired,
                     std::string& error) {
    transaction.schemaVersion = kSchemaVersion;
    transaction.state = State::Prepared;
    if (!transaction.source.valid() || !transaction.destination.valid() ||
        transaction.source != source.descriptor() ||
        transaction.destination != destination.descriptor() ||
        transaction.source == transaction.destination) {
        error = "transaction/store descriptors do not match";
        return false;
    }
    if (!journal_.ensure(error) || !journal_.idAvailable(transaction.id)) {
        if (error.empty()) error = "transaction ID is not available";
        return false;
    }

    std::vector<uint8_t> sourceBefore, destinationBefore;
    if (!source.read(sourceBefore, error) || !destination.read(destinationBefore, error)) return false;
    if (!source.validate(sourceBefore, error) || !destination.validate(destinationBefore, error) ||
        !destination.validate(destinationAfter, error) || !source.validate(sourceRetired, error)) return false;

    transaction.sourceBefore = sha256(sourceBefore);
    transaction.sourceRetired = sha256(sourceRetired);
    transaction.destinationBefore = sha256(destinationBefore);
    transaction.destinationAfter = sha256(destinationAfter);
    if (transaction.sourceBefore == transaction.sourceRetired ||
        transaction.destinationBefore == transaction.destinationAfter) {
        error = "MOVE transaction must change both authoritative stores";
        return false;
    }

    std::string metadataError;
    if (!validTransaction(transaction, metadataError)) {
        error = metadataError;
        return false;
    }
    if (!journal_.storeEvidence(transaction, destinationAfter, sourceRetired, error)) return false;
    if (persistGate_ && !persistGate_(State::Prepared)) {
        error = "journal persistence rejected by injected gate";
        return false;
    }
    return journal_.persist(transaction, error);
}

RecoveryResult Engine::recover(const std::string& transactionId,
                               Store& source,
                               Store& destination,
                               FaultPoint fault) {
    auto loaded = journal_.load(transactionId);
    if (loaded.status == LoadStatus::UnsupportedVersion)
        return result(RecoveryStatus::UnsupportedVersion, State::Prepared, loaded.error);
    if (loaded.status == LoadStatus::Corrupt)
        return result(RecoveryStatus::Corrupt, State::Prepared, loaded.error);
    if (loaded.status != LoadStatus::Ok)
        return result(RecoveryStatus::Failed, State::Prepared, loaded.error);

    Transaction tx = std::move(loaded.transaction);
    if (source.descriptor() != tx.source || destination.descriptor() != tx.destination)
        return result(RecoveryStatus::Conflict, tx.state, "store descriptor mismatch");

    std::vector<uint8_t> destinationEvidence, sourceRetiredEvidence;
    std::string error;
    if (!journal_.loadEvidence(tx, destinationEvidence, sourceRetiredEvidence, error))
        return result(RecoveryStatus::Corrupt, tx.state, error);

    auto readStores = [&](std::vector<uint8_t>& src, std::vector<uint8_t>& dst) {
        return source.read(src, error) && destination.read(dst, error);
    };
    std::vector<uint8_t> srcBytes, dstBytes;
    if (!readStores(srcBytes, dstBytes))
        return result(RecoveryStatus::Failed, tx.state, error);

    auto srcDigest = sha256(srcBytes);
    auto dstDigest = sha256(dstBytes);
    const bool srcBefore = srcDigest == tx.sourceBefore;
    const bool srcAfter = srcDigest == tx.sourceRetired;
    const bool dstBefore = dstDigest == tx.destinationBefore;
    const bool dstAfter = dstDigest == tx.destinationAfter;
    if ((!srcBefore && !srcAfter) || (!dstBefore && !dstAfter) || (srcAfter && dstBefore))
        return result(RecoveryStatus::Conflict, tx.state, "store content no longer matches known transaction states");

    const auto stateValue = static_cast<uint8_t>(tx.state);
    if (stateValue <= static_cast<uint8_t>(State::DestinationVerified) && !srcBefore)
        return result(RecoveryStatus::Conflict, tx.state, "source retired before journal authorized retirement");
    if (stateValue >= static_cast<uint8_t>(State::SourceRetired) && !srcAfter)
        return result(RecoveryStatus::Conflict, tx.state, "journal says source retired but source disagrees");
    if (stateValue >= static_cast<uint8_t>(State::DestinationVerified) && !dstAfter)
        return result(RecoveryStatus::Conflict, tx.state, "verified destination no longer matches transaction");

    if (tx.state == State::Committed)
        return result(RecoveryStatus::Committed, tx.state);

    if (fault == FaultPoint::AfterPreparedJournal && tx.state == State::Prepared)
        return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after PREPARED journal");

    // Destination-first phase. PREPARED/DestinationWritten may safely retry only when the
    // destination still exactly matches its recorded precondition.
    if (tx.state == State::Prepared || tx.state == State::DestinationWritten) {
        if (dstBefore) {
            if (!destination.replace(destinationEvidence, error))
                return result(RecoveryStatus::Failed, tx.state, error);
            if (fault == FaultPoint::AfterDestinationWrite)
                return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after destination write");
            if (!destination.read(dstBytes, error))
                return result(RecoveryStatus::Failed, tx.state, error);
            dstDigest = sha256(dstBytes);
            if (dstDigest != tx.destinationAfter)
                return result(RecoveryStatus::Failed, tx.state, "destination write did not produce expected SHA-256");
        }
        if (tx.state == State::Prepared) {
            if (!persistState(tx, State::DestinationWritten, error))
                return result(RecoveryStatus::Failed, tx.state, error);
        }
        if (!destination.validate(dstBytes, error))
            return result(RecoveryStatus::Failed, tx.state, error);
        if (fault == FaultPoint::AfterDestinationVerify)
            return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after destination verification");
        if (!persistState(tx, State::DestinationVerified, error))
            return result(RecoveryStatus::Failed, tx.state, error);
    }

    if (tx.state == State::DestinationVerified) {
        if (fault == FaultPoint::AfterDestinationVerifiedJournal)
            return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after destination verified journal");
        // This durable state is the authorization boundary for source retirement.
        // Cross-game transactions must prove their persisted conversion evidence on EVERY
        // recovery attempt before the journal may authorize retirement.
        if (tx.crossGameConversion) {
            if (!retirementGate_)
                return result(RecoveryStatus::Failed, tx.state,
                              "cross-game source retirement requires conversion evidence authorization");
            if (!retirementGate_(tx, error))
                return result(RecoveryStatus::Failed, tx.state,
                              error.empty() ? "conversion evidence authorization failed" : error);
        }
        if (!persistState(tx, State::SourceRetirePending, error))
            return result(RecoveryStatus::Failed, tx.state, error);
    }

    if (tx.state == State::SourceRetirePending) {
        if (fault == FaultPoint::BeforeSourceRetire)
            return result(RecoveryStatus::Interrupted, tx.state, "injected interruption before source retirement");

        // Re-check after restart as well: a crash may have happened after persisting
        // SOURCE_RETIRE_PENDING but before the source write.
        if (tx.crossGameConversion) {
            if (!retirementGate_)
                return result(RecoveryStatus::Failed, tx.state,
                              "cross-game source retirement requires conversion evidence authorization");
            if (!retirementGate_(tx, error))
                return result(RecoveryStatus::Failed, tx.state,
                              error.empty() ? "conversion evidence authorization failed" : error);
        }

        if (!source.read(srcBytes, error))
            return result(RecoveryStatus::Failed, tx.state, error);
        srcDigest = sha256(srcBytes);
        if (srcDigest == tx.sourceBefore) {
            if (!source.replace(sourceRetiredEvidence, error))
                return result(RecoveryStatus::Failed, tx.state, error);
            if (fault == FaultPoint::AfterSourceWrite)
                return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after source write");
            if (!source.read(srcBytes, error))
                return result(RecoveryStatus::Failed, tx.state, error);
            srcDigest = sha256(srcBytes);
        } else if (srcDigest != tx.sourceRetired) {
            return result(RecoveryStatus::Conflict, tx.state, "source changed during retirement");
        }
        if (srcDigest != tx.sourceRetired)
            return result(RecoveryStatus::Failed, tx.state, "source retirement did not produce expected SHA-256");
        if (!source.validate(srcBytes, error))
            return result(RecoveryStatus::Failed, tx.state, error);
        if (fault == FaultPoint::AfterSourceVerify)
            return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after source verification");
        if (!persistState(tx, State::SourceRetired, error))
            return result(RecoveryStatus::Failed, tx.state, error);
    }

    if (tx.state == State::SourceRetired) {
        if (fault == FaultPoint::AfterSourceRetiredJournal)
            return result(RecoveryStatus::Interrupted, tx.state, "injected interruption after source-retired journal");
        // Final destination re-verification prevents COMMITTED from being recorded after an
        // unrelated destination change between source retirement and the final journal state.
        if (!destination.read(dstBytes, error))
            return result(RecoveryStatus::Failed, tx.state, error);
        if (sha256(dstBytes) != tx.destinationAfter)
            return result(RecoveryStatus::Conflict, tx.state, "destination changed before commit");
        if (!destination.validate(dstBytes, error))
            return result(RecoveryStatus::Failed, tx.state, error);
        if (fault == FaultPoint::BeforeCommitted)
            return result(RecoveryStatus::Interrupted, tx.state, "injected interruption before COMMITTED");
        if (!persistState(tx, State::Committed, error))
            return result(RecoveryStatus::Failed, tx.state, error);
    }

    return result(tx.state == State::Committed ? RecoveryStatus::Committed : RecoveryStatus::Failed,
                  tx.state,
                  tx.state == State::Committed ? std::string{} : "transaction did not reach COMMITTED");
}

} // namespace PokeBank::Storage::MoveTx
