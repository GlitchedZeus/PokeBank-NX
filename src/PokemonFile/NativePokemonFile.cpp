#include "PokemonFile/NativePokemonFile.h"

#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Encryption/Encryption5.h"

#include <algorithm>
#include <cctype>

namespace PokeBank::PokemonFile {
namespace {

constexpr std::size_t kPk1Intl = 69, kPk1Jpn = 59, kPk2Intl = 73, kPk2Jpn = 63;   // PokeCrypto.cs:15-22
constexpr std::size_t kPk3Stored = 80, kPk3Party = 100;
constexpr std::size_t kPk45Stored = 136, kPk4Party = 236, kPk5Party = 220;

uint16_t rd16(std::span<const uint8_t> b, std::size_t o) noexcept { return static_cast<uint16_t>(b[o] | (b[o + 1] << 8)); }
uint32_t rd32(std::span<const uint8_t> b, std::size_t o) noexcept {
    return static_cast<uint32_t>(rd16(b, o)) | (static_cast<uint32_t>(rd16(b, o + 2)) << 16);
}

std::string lowerExtension(std::string_view name) {
    const auto slash = name.find_last_of("/\\");
    const auto dot = name.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) return {};
    std::string ext(name.substr(dot));
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

struct ExtensionInfo { int generation = 0; bool encryptedName = false; };
ExtensionInfo classify(std::string_view name) {
    const std::string ext = lowerExtension(name);
    if (ext.size() != 4 || ext[0] != '.' || (ext[1] != 'p' && ext[1] != 'e') || ext[2] != 'k') return {};
    const int gen = ext[3] - '0';
    if (gen < 1 || gen > 5) return {};
    const bool encrypted = ext[1] == 'e';
    if (encrypted && gen < 3) return {};   // Game Boy records have no encrypted form
    return {gen, encrypted};
}

bool terminated(std::span<const uint8_t> field) noexcept {
    return std::find(field.begin(), field.end(), uint8_t{0x50}) != field.end();
}

std::optional<Decoded> fail(std::string& error, std::string why) { error = std::move(why); return std::nullopt; }

// Gen I/II single-entry list: count, species, 0xFF, party body, OT, nickname.
std::optional<Decoded> decodeList(std::span<const uint8_t> b, int gen, std::string& error) {
    const std::size_t body = gen == 1 ? 44 : 48;
    const bool japanese = b.size() == (gen == 1 ? kPk1Jpn : kPk2Jpn);
    const std::size_t str = japanese ? 6 : 11;
    if (b[0] != 1 || b[2] != 0xFF) return fail(error, "Not a single Pokemon list (count or terminator)");
    const uint8_t listSpecies = b[1], bodySpecies = b[3];
    if (bodySpecies == 0 || bodySpecies == 0xFF) return fail(error, "The record holds no Pokemon");
    // Gen II lists an Egg as 0xFD while the body keeps the real species (pret EGG).
    if (listSpecies != bodySpecies && !(gen == 2 && listSpecies == 0xFD))
        return fail(error, "The list and record species disagree");
    if (gen == 2 && bodySpecies > 251) return fail(error, "Species is outside Generation II");
    if (gen == 1 && bodySpecies > 190) return fail(error, "Species index is outside Generation I");
    const auto ot = b.subspan(3 + body, str), nick = b.subspan(3 + body + str, str);
    if (!terminated(ot) || !terminated(nick)) return fail(error, "A name has no terminator");
    Decoded d;
    d.format = gen == 1 ? Format::PK1 : Format::PK2;
    d.generation = gen;
    d.party = true;
    d.japanese = japanese;
    d.plain.assign(b.begin(), b.end());
    return d;
}

// Gen III: PK3 checksum over the canonical 48 data bytes, stored at 0x1C (header is never encrypted).
bool pk3ChecksumOk(std::span<const uint8_t> canonical) {
    const auto sum = Encryption::checksum3FRLG(std::as_bytes(canonical.first(kPk3Stored)));
    return sum == rd16(canonical, 0x1C);
}

std::optional<Decoded> decode3(std::span<const uint8_t> b, bool encryptedName, std::string& error) {
    const bool plainOk = pk3ChecksumOk(b);
    std::byte* dec = Encryption::decryptArray3FRLG(std::as_bytes(b));
    std::vector<uint8_t> decrypted(reinterpret_cast<uint8_t*>(dec), reinterpret_cast<uint8_t*>(dec) + b.size());
    delete[] dec;
    const bool encOk = pk3ChecksumOk(decrypted);
    bool encrypted;
    if (plainOk != encOk) encrypted = encOk;
    else if (!plainOk) return fail(error, "Checksum does not match (corrupt or not a Generation III record)");
    else {
        // Both readings pass: PKHeX IsEncrypted3 says plain (checksum matches as given); the name must agree.
        if (encryptedName) return fail(error, "Ambiguous record: it reads as both plain and encrypted");
        encrypted = false;
    }
    Decoded d;
    d.format = Format::PK3;
    d.generation = 3;
    d.party = b.size() == kPk3Party;
    d.wasEncrypted = encrypted;
    d.plain = encrypted ? std::move(decrypted) : std::vector<uint8_t>(b.begin(), b.end());
    const uint16_t species = rd16(d.plain, 0x20);
    if (species == 0 || species > 412) return fail(error, "The record holds no valid Generation III species");
    if (d.plain[0x13] & 0x01) return fail(error, "The record is marked as a Bad Egg");
    return d;
}

uint16_t sum45(std::span<const uint8_t> canonical) {
    uint32_t s = 0;
    for (std::size_t o = 8; o < kPk45Stored; o += 2) s += rd16(canonical, o);
    return static_cast<uint16_t>(s);
}

std::vector<uint8_t> decrypt45(std::span<const uint8_t> b, int gen) {
    if (gen == 5) return PokeVault::Integration::Gen5::Crypto::decrypt(b);
    const auto d = Encryption::decryptArray4(std::as_bytes(b));
    std::vector<uint8_t> out(d.size());
    std::transform(d.begin(), d.end(), out.begin(), [](std::byte x) { return static_cast<uint8_t>(x); });
    return out;
}

std::optional<Decoded> decode45(std::span<const uint8_t> b, int gen, bool encryptedName, std::string& error) {
    if (b.size() == kPk4Party && gen != 4) return fail(error, "A 236-byte record is a Generation IV party record");
    if (b.size() == kPk5Party && gen != 5) return fail(error, "A 220-byte record is a Generation V party record");
    const bool plainOk = sum45(b) == rd16(b, 6);
    const auto decrypted = decrypt45(b, gen);
    const bool encOk = decrypted.size() == b.size() && sum45(decrypted) == rd16(decrypted, 6);
    bool encrypted;
    if (plainOk != encOk) encrypted = encOk;
    else if (!plainOk) return fail(error, "Checksum does not match (corrupt or not this generation)");
    else {
        encrypted = rd32(b, 0x64) != 0;   // PKHeX IsEncrypted45
        if (encrypted != encryptedName) return fail(error, "Ambiguous record: it reads as both plain and encrypted");
    }
    Decoded d;
    d.format = gen == 4 ? Format::PK4 : Format::PK5;
    d.generation = gen;
    d.party = b.size() != kPk45Stored;
    d.wasEncrypted = encrypted;
    d.plain = encrypted ? decrypted : std::vector<uint8_t>(b.begin(), b.end());
    const auto& p = d.plain;
    const uint16_t species = rd16(p, 0x08);
    if (species == 0 || species > (gen == 4 ? 493 : 649)) return fail(error, "The record holds no valid species for this generation");
    // PKHeX EntityFormat.GetFormat45 on the plaintext.
    if (rd16(p, 0x04) != 0) return fail(error, "Battle Revolution records are not supported");
    const bool looksGen4 = (p[0x5F] < 0x10 && rd16(p, 0x80) < 0x3333) || rd16(p, 0x46) != 0;
    if (!d.party && looksGen4 != (gen == 4))
        return fail(error, gen == 4 ? "The content is a Generation V record (.pk5)" : "The content is a Generation IV record (.pk4)");
    return d;
}

}  // namespace

bool supportedFileName(std::string_view fileName) noexcept { return classify(fileName).generation != 0; }

std::string_view plainExtension(int generation) noexcept {
    switch (generation) {
        case 1: return ".pk1"; case 2: return ".pk2"; case 3: return ".pk3"; case 4: return ".pk4"; case 5: return ".pk5";
        default: return {};
    }
}

std::optional<Decoded> decode(std::span<const uint8_t> bytes, std::string_view fileName, std::string& error) {
    error.clear();
    const auto info = classify(fileName);
    if (!info.generation) return fail(error, "Unsupported file type (.pk1-.pk5, .ek3-.ek5)");
    const std::size_t n = bytes.size();
    switch (info.generation) {
        case 1: if (n != kPk1Intl && n != kPk1Jpn) return fail(error, "Wrong size for a .pk1 file"); return decodeList(bytes, 1, error);
        case 2: if (n != kPk2Intl && n != kPk2Jpn) return fail(error, "Wrong size for a .pk2 file"); return decodeList(bytes, 2, error);
        case 3: if (n != kPk3Stored && n != kPk3Party) return fail(error, "Wrong size for a Generation III record"); return decode3(bytes, info.encryptedName, error);
        case 4: if (n != kPk45Stored && n != kPk4Party) return fail(error, "Wrong size for a Generation IV record"); return decode45(bytes, 4, info.encryptedName, error);
        case 5: if (n != kPk45Stored && n != kPk5Party) return fail(error, "Wrong size for a Generation V record"); return decode45(bytes, 5, info.encryptedName, error);
    }
    return fail(error, "Unsupported file type");
}

}  // namespace PokeBank::PokemonFile
