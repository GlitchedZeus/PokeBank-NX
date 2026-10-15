#include "Integration/Gen8/Gen8SwishCrypto.h"

#include "Utils/SHA256.h"

#include <algorithm>
#include <set>

namespace PokeVault::Integration::Gen8::Swish {
namespace {

uint32_t readU32(std::span<const uint8_t> b, size_t o) noexcept {
    return static_cast<uint32_t>(b[o]) | (static_cast<uint32_t>(b[o + 1]) << 8) |
           (static_cast<uint32_t>(b[o + 2]) << 16) | (static_cast<uint32_t>(b[o + 3]) << 24);
}
void pushU32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
}

// Standalone block types SCBlock.ReadFromOffset accepts (Bool3 included: PKHeX only
// asserts in debug builds that it is unused).
bool knownStandalone(uint8_t t) noexcept {
    return (t >= 1 && t <= 5) || isValueType(static_cast<TypeCode>(t));
}
// Array subtypes with a GetTypeSize.
bool knownSubtype(uint8_t t) noexcept {
    return t == static_cast<uint8_t>(TypeCode::Bool3) || isValueType(static_cast<TypeCode>(t));
}

} // namespace

std::array<uint8_t, HashSize> computeHash(std::span<const uint8_t> payload) {
    Utils::SHA256 sha;
    sha.update(IntroHashBytes.data(), IntroHashBytes.size());
    sha.update(payload.data(), payload.size());
    sha.update(OutroHashBytes.data(), OutroHashBytes.size());
    std::array<uint8_t, HashSize> out{};
    sha.finalize(out.data());
    return out;
}

bool hashValid(std::span<const uint8_t> file) {
    if (file.size() <= HashSize) return false;
    const auto payload = file.first(file.size() - HashSize);
    const auto expected = computeHash(payload);
    return std::equal(expected.begin(), expected.end(), file.end() - HashSize);
}

void cryptStaticXorpad(std::span<uint8_t> payload) noexcept {
    size_t k = 0;
    for (auto& b : payload) {
        b ^= StaticXorpad[k];
        if (++k == StaticXorpad.size()) k = 0;
    }
}

const char* parseStatusText(ParseStatus status) noexcept {
    switch (status) {
        case ParseStatus::Ok: return "SwishCrypto container valid";
        case ParseStatus::TooSmall: return "file is too small to hold a SwishCrypto hash";
        case ParseStatus::HashMismatch: return "SwishCrypto SHA-256 does not match";
        case ParseStatus::UnknownType: return "SCBlock has an unknown type code";
        case ParseStatus::Truncated: return "SCBlock length runs past the payload";
        case ParseStatus::DuplicateKey: return "SCBlock key appears more than once";
    }
    return "SwishCrypto container invalid";
}

std::optional<std::vector<Block>> decrypt(std::span<const uint8_t> file, ParseStatus* status) {
    const auto fail = [&](ParseStatus s) -> std::optional<std::vector<Block>> {
        if (status) *status = s;
        return std::nullopt;
    };
    if (file.size() <= HashSize) return fail(ParseStatus::TooSmall);
    if (!hashValid(file)) return fail(ParseStatus::HashMismatch);
    std::vector<uint8_t> payload(file.begin(), file.end() - HashSize);
    cryptStaticXorpad(payload);
    const std::span<const uint8_t> p(payload);

    std::vector<Block> blocks;
    std::set<uint32_t> keys;
    size_t at = 0;
    while (at < p.size()) {
        if (p.size() - at < 5) return fail(ParseStatus::Truncated);
        Block b;
        b.key = readU32(p, at);
        at += 4;
        XorShift32 xk(b.key);
        const uint8_t type = static_cast<uint8_t>(p[at++] ^ xk.next());
        if (!knownStandalone(type)) return fail(ParseStatus::UnknownType);
        b.type = static_cast<TypeCode>(type);
        size_t length = 0;
        if (b.type == TypeCode::Object) {
            if (p.size() - at < 4) return fail(ParseStatus::Truncated);
            const uint32_t n = readU32(p, at) ^ xk.next32();
            at += 4;
            if (n > 0x7FFFFFFFu) return fail(ParseStatus::Truncated);  // PKHeX reads a signed int32
            length = n;
        } else if (b.type == TypeCode::Array) {
            if (p.size() - at < 5) return fail(ParseStatus::Truncated);
            const uint32_t count = readU32(p, at) ^ xk.next32();
            at += 4;
            const uint8_t sub = static_cast<uint8_t>(p[at++] ^ xk.next());
            if (!knownSubtype(sub)) return fail(ParseStatus::UnknownType);
            b.subtype = static_cast<TypeCode>(sub);
            if (count > 0x7FFFFFFFu) return fail(ParseStatus::Truncated);
            const uint64_t bytes = static_cast<uint64_t>(count) * typeSize(b.subtype);
            if (bytes > p.size()) return fail(ParseStatus::Truncated);
            length = static_cast<size_t>(bytes);
        } else if (isValueType(b.type)) {
            length = typeSize(b.type);
        }
        if (length > p.size() - at) return fail(ParseStatus::Truncated);
        b.data.assign(p.begin() + static_cast<std::ptrdiff_t>(at),
                      p.begin() + static_cast<std::ptrdiff_t>(at + length));
        for (auto& v : b.data) v ^= xk.next();
        at += length;
        if (!keys.insert(b.key).second) return fail(ParseStatus::DuplicateKey);
        blocks.push_back(std::move(b));
    }
    if (status) *status = ParseStatus::Ok;
    return blocks;
}

void writeBlock(const Block& block, std::vector<uint8_t>& out) {
    pushU32(out, block.key);
    XorShift32 xk(block.key);
    out.push_back(static_cast<uint8_t>(static_cast<uint8_t>(block.type) ^ xk.next()));
    if (block.type == TypeCode::Object) {
        pushU32(out, static_cast<uint32_t>(block.data.size()) ^ xk.next32());
    } else if (block.type == TypeCode::Array) {
        const size_t size = typeSize(block.subtype);
        const uint32_t count = size ? static_cast<uint32_t>(block.data.size() / size) : 0;
        pushU32(out, count ^ xk.next32());
        out.push_back(static_cast<uint8_t>(static_cast<uint8_t>(block.subtype) ^ xk.next()));
    }
    for (const uint8_t v : block.data) out.push_back(static_cast<uint8_t>(v ^ xk.next()));
}

std::vector<uint8_t> encrypt(const std::vector<Block>& blocks) {
    std::vector<uint8_t> out;
    size_t total = HashSize;
    for (const auto& b : blocks) total += b.serializedSize();
    out.reserve(total);
    for (const auto& b : blocks) writeBlock(b, out);
    cryptStaticXorpad(out);
    const auto hash = computeHash(out);
    out.insert(out.end(), hash.begin(), hash.end());
    return out;
}

} // namespace PokeVault::Integration::Gen8::Swish
