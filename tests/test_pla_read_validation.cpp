#include <cassert>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include "Utils/FileUtilities.h"
#include "Save/PLAReadValidation.h"
#include "Save/SCReadValidation.h"
#include "Encryption/Encryption.h"
#include "Utils/StringHelpers.h"

int main() {
    using namespace Save;
    using namespace Encryption;
    using Enums::SCTypeCode;
    size_t fileSize = 99;
    assert(Utils::readAllBytes(nullptr, &fileSize) == nullptr && fileSize == 0);
    char path[] = "/tmp/pokebank-read-test-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    const uint8_t fileBytes[]{1, 2, 3};
    assert(write(fd, fileBytes, sizeof fileBytes) == sizeof fileBytes);
    close(fd);
    auto* loaded = Utils::readAllBytes(path, &fileSize);
    assert(loaded && fileSize == sizeof fileBytes && loaded[2] == 3);
    delete[] loaded; // Regression: shared reader previously returned malloc memory.
    unlink(path);
    assert(Utils::readAllBytes(path, &fileSize) == nullptr && fileSize == 0);
    std::vector<Block> blocks;
    assert(tryDecrypt(nullptr, 100, blocks) == DecryptStatus::MissingData);
    std::vector<uint8_t> tiny(32);
    assert(tryDecrypt(tiny.data(), tiny.size(), blocks) == DecryptStatus::TooSmall);
    assert(!validatePLAReadLayout(blocks).empty());
    std::vector<Block> valid{
        {0xf25c070e, SCTypeCode::Object, SCTypeCode::None, std::vector<uint8_t>(0x3a)},
        {0x2985fe5d, SCTypeCode::Object, SCTypeCode::None, std::vector<uint8_t>(6 * 0x178)},
        {0x47e1ceab, SCTypeCode::Object, SCTypeCode::None, std::vector<uint8_t>(32 * 30 * 0x168)}};
    assert(validatePLAReadLayout(valid).empty());
    std::array<std::byte, 0x168> pokemon{};
    pokemon[6] = std::byte{25}; // checksum of a minimal synthetic species-25 record
    pokemon[8] = std::byte{25};
    std::unique_ptr<std::byte[]> encryptedPokemon(encryptArray8LA(pokemon, 0));
    std::copy_n(reinterpret_cast<const uint8_t*>(encryptedPokemon.get()), pokemon.size(), valid[2].data.begin());
    assert(validatePLAReadLayout(valid).empty()); // nonblank positive structural fixture
    auto truncated = valid;
    truncated[2].data.pop_back();
    assert(!validatePLAReadLayout(truncated).empty());
    auto duplicate = valid;
    duplicate.push_back(valid[0]);
    assert(!validatePLAReadLayout(duplicate).empty());
    auto corruptPokemon = valid;
    corruptPokemon[2].data[10] ^= 1;
    assert(!validatePLAReadLayout(corruptPokemon).empty());
    auto encoded = serializeAllBlocks(valid);
    cryptStaticXorpadBytes(encoded, encoded.size());
    uint8_t hash[32];
    computeHash(encoded.data(), encoded.size(), hash);
    encoded.insert(encoded.end(), hash, hash + 32);
    const auto original = encoded;
    assert(tryDecrypt(encoded.data(), encoded.size(), blocks) == DecryptStatus::Ok);
    assert(validatePLAReadLayout(blocks).empty());
    assert(encoded == original); // Read validation cannot repair/mutate input.
    encoded[0] ^= 1;
    assert(tryDecrypt(encoded.data(), encoded.size(), blocks) == DecryptStatus::HashMismatch);
    assert(blocks.empty());
    size_t consumed = 99;
    assert(parseAllBlocks(nullptr, 100, &consumed).empty() && consumed == 0);
    // Every truncated prefix must remain in bounds; full-consumption is explicit.
    auto raw = serializeAllBlocks({{1, SCTypeCode::Object, SCTypeCode::None, {1, 2, 3}}});
    for (size_t n = 0; n < raw.size(); ++n) {
        auto partial = parseAllBlocks(raw.data(), n, &consumed);
        assert(partial.empty() && consumed == 0);
    }
    raw.push_back(0xff); // hash-valid but structurally incomplete container
    cryptStaticXorpadBytes(raw, raw.size());
    computeHash(raw.data(), raw.size(), hash);
    raw.insert(raw.end(), hash, hash + 32);
    assert(tryDecrypt(raw.data(), raw.size(), blocks) == DecryptStatus::MalformedBlocks);
    // AUDIT-021: SWSH/SV/Z-A must pass a semantic layout gate after SC hash parsing,
    // before Trainer construction or durable replacement.
    auto makeSWSH = [] {
        return std::vector<Block>{
            {SCReadValidation::SWSH_MY_STATUS, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(0xB0 + 0x1A)},
            {SCReadValidation::SWSH_PARTY, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(6 * Encryption::SIZE_PARTY8_SWSH)},
            {SCReadValidation::SWSH_MONEY, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(8)},
            {SCReadValidation::SWSH_ITEMS, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(4600 + 64 * 4)},
            {SCReadValidation::SWSH_BOX, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::BOX_COUNT * SCReadValidation::BOX_SLOTS * Encryption::SIZE_PARTY8_SWSH)},
            {SCReadValidation::SWSH_BOX_LAYOUT, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::BOX_COUNT * SCReadValidation::BOX_NAME_BYTES)},
            {SCReadValidation::SWSH_CURRENT_BOX, SCTypeCode::Byte, SCTypeCode::None,
                std::vector<uint8_t>(1)},
        };
    };
    auto makeSV = [] {
        return std::vector<Block>{
            {SCReadValidation::GEN9_MY_STATUS, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(0x10 + 0x1A)},
            {SCReadValidation::GEN9_PARTY, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(6 * Encryption::SIZE_PARTY9_SV)},
            {SCReadValidation::GEN9_MONEY, SCTypeCode::UInt32, SCTypeCode::None,
                std::vector<uint8_t>(4)},
            {SCReadValidation::GEN9_ITEMS, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::GEN9_ITEM_BLOCK_BYTES)},
            {SCReadValidation::GEN9_BOX, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::BOX_COUNT * SCReadValidation::BOX_SLOTS * Encryption::SIZE_PARTY9_SV)},
            {SCReadValidation::GEN9_BOX_LAYOUT, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::BOX_COUNT * SCReadValidation::BOX_NAME_BYTES)},
            {SCReadValidation::GEN9_CURRENT_BOX, SCTypeCode::Byte, SCTypeCode::None,
                std::vector<uint8_t>(1)},
        };
    };
    auto makeZA = [] {
        return std::vector<Block>{
            {SCReadValidation::GEN9_MY_STATUS, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(0x10 + 0x1A)},
            {SCReadValidation::GEN9_PARTY, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(6 * Encryption::PARTY_SLOT_SIZE9_LZA)},
            {SCReadValidation::GEN9_MONEY, SCTypeCode::UInt32, SCTypeCode::None,
                std::vector<uint8_t>(4)},
            {SCReadValidation::GEN9_ITEMS, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::GEN9_ITEM_BLOCK_BYTES)},
            {SCReadValidation::GEN9_BOX, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::BOX_COUNT * SCReadValidation::BOX_SLOTS * Encryption::BOX_SLOT_SIZE9_LZA)},
            {SCReadValidation::GEN9_BOX_LAYOUT, SCTypeCode::Object, SCTypeCode::None,
                std::vector<uint8_t>(SCReadValidation::BOX_COUNT * SCReadValidation::BOX_NAME_BYTES)},
            {SCReadValidation::GEN9_CURRENT_BOX, SCTypeCode::Byte, SCTypeCode::None,
                std::vector<uint8_t>(1)},
            {SCReadValidation::ZA_SAVE_REVISION, SCTypeCode::UInt64, SCTypeCode::None,
                std::vector<uint8_t>(8)},
        };
    };

    auto swshLayout = makeSWSH();
    auto svLayout = makeSV();
    auto zaLayout = makeZA();
    assert(validateSCReadLayout(swshLayout, Enums::GameVersion::SWSH).empty());
    assert(validateSCReadLayout(svLayout, Enums::GameVersion::SV).empty());
    assert(validateSCReadLayout(zaLayout, Enums::GameVersion::ZA).empty());

    // Native modern empty slots are encrypted blanks, not necessarily all-zero ciphertext.
    // Species 0 after decrypt is authoritative emptiness: stale checksum/nature bytes in an empty
    // slot must not block the whole save, while occupied records remain strict below.
    {
        std::vector<std::byte> plain(Encryption::SIZE_PARTY8_SWSH, std::byte{0});
        plain[6] = std::byte{0x7F};
        plain[0x20] = std::byte{0xFF};
        plain[0x21] = std::byte{0xFF};
        std::unique_ptr<std::byte[]> enc(Encryption::encryptArray8SWSH(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    swshLayout[1].data.begin());
        assert(validateSCReadLayout(swshLayout, Enums::GameVersion::SWSH).empty());
    }
    {
        std::vector<std::byte> plain(Encryption::SIZE_PARTY9_SV, std::byte{0});
        plain[6] = std::byte{0x7F};
        plain[0x20] = std::byte{0xFF};
        plain[0x21] = std::byte{0xFF};
        std::unique_ptr<std::byte[]> enc(Encryption::encryptArray9SV(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    svLayout[1].data.begin());
        assert(validateSCReadLayout(svLayout, Enums::GameVersion::SV).empty());
    }
    {
        std::vector<std::byte> plain(Encryption::SIZE_PARTY9_LZA, std::byte{0});
        plain[6] = std::byte{0x7F};
        plain[0x20] = std::byte{0xFF};
        plain[0x21] = std::byte{0xFF};
        std::unique_ptr<std::byte[]> enc(Encryption::encryptArray9LZA(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    zaLayout[1].data.begin());
        assert(validateSCReadLayout(zaLayout, Enums::GameVersion::ZA).empty());
    }

    auto duplicateSC = svLayout;
    duplicateSC.push_back(svLayout.front());
    assert(!validateSCReadLayout(duplicateSC, Enums::GameVersion::SV).empty());

    auto missingSC = swshLayout;
    missingSC.erase(missingSC.begin() + 1); // no Party block
    assert(!validateSCReadLayout(missingSC, Enums::GameVersion::SWSH).empty());

    auto shortSC = zaLayout;
    shortSC[4].data.pop_back(); // Box block one byte below the supported geometry
    assert(!validateSCReadLayout(shortSC, Enums::GameVersion::ZA).empty());

    // Raw trainer payloads are consumed by key + bytes, not by their SC wrapper tag.
    auto payloadWrapperSC = svLayout;
    payloadWrapperSC[3].type = SCTypeCode::Array;
    assert(validateSCReadLayout(payloadWrapperSC, Enums::GameVersion::SV).empty());

    // True scalar fields remain exact-type checked.
    auto wrongScalarSC = svLayout;
    wrongScalarSC[2].type = SCTypeCode::UInt64; // Money is a UInt32 scalar.
    std::string scDiagnostic;
    assert(!validateSCReadLayout(wrongScalarSC, Enums::GameVersion::SV, &scDiagnostic).empty());
    assert(scDiagnostic.find("0x4F35D0DD") != std::string::npos);
    assert(scDiagnostic.find("wrong type") != std::string::npos);
    auto wrongCurrentBoxSC = swshLayout;
    wrongCurrentBoxSC[6].type = SCTypeCode::UInt32;
    wrongCurrentBoxSC[6].data.resize(4);
    assert(!validateSCReadLayout(wrongCurrentBoxSC, Enums::GameVersion::SWSH, &scDiagnostic).empty());
    assert(scDiagnostic.find("0x017C3CBB") != std::string::npos);
    assert(scDiagnostic.find("wrong type") != std::string::npos);
    // PKHeX CurrentBox accessors use GetValue<byte>/SetValue<byte>, and its blank-block
    // geometry allocates exactly one byte for 0x017C3CBB in SWSH, SV, and Z-A.
    assert(swshLayout[6].data.size() == 1);
    assert(svLayout[6].data.size() == 1);
    assert(zaLayout[6].data.size() == 1);
    auto wrongRevisionSC = zaLayout;
    wrongRevisionSC[7].type = SCTypeCode::UInt32;
    assert(!validateSCReadLayout(wrongRevisionSC, Enums::GameVersion::ZA).empty());

    // A nonblank Pokemon record must pass its inner entity checksum/basic-domain gate.
    {
        std::vector<std::byte> plain(Encryption::SIZE_PARTY8_SWSH, std::byte{0});
        plain[6] = std::byte{25};
        plain[8] = std::byte{25};
        std::unique_ptr<std::byte[]> enc(Encryption::encryptArray8SWSH(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    swshLayout[1].data.begin());
        assert(validateSCReadLayout(swshLayout, Enums::GameVersion::SWSH).empty());
        plain[6] = std::byte{0}; // wrong checksum for species 25
        enc.reset(Encryption::encryptArray8SWSH(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    swshLayout[1].data.begin());
        assert(!validateSCReadLayout(swshLayout, Enums::GameVersion::SWSH).empty());
    }
    {
        std::vector<std::byte> plain(Encryption::SIZE_PARTY9_SV, std::byte{0});
        plain[6] = std::byte{25};
        plain[8] = std::byte{25};
        std::unique_ptr<std::byte[]> enc(Encryption::encryptArray9SV(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    svLayout[1].data.begin());
        assert(validateSCReadLayout(svLayout, Enums::GameVersion::SV).empty());
        plain[6] = std::byte{0};
        enc.reset(Encryption::encryptArray9SV(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    svLayout[1].data.begin());
        assert(!validateSCReadLayout(svLayout, Enums::GameVersion::SV).empty());
    }
    {
        std::vector<std::byte> plain(Encryption::SIZE_PARTY9_LZA, std::byte{0});
        plain[6] = std::byte{25};
        plain[8] = std::byte{25};
        std::unique_ptr<std::byte[]> enc(Encryption::encryptArray9LZA(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    zaLayout[1].data.begin());
        assert(validateSCReadLayout(zaLayout, Enums::GameVersion::ZA).empty());
        plain[6] = std::byte{0};
        enc.reset(Encryption::encryptArray9LZA(plain, 0));
        std::copy_n(reinterpret_cast<const uint8_t*>(enc.get()), plain.size(),
                    zaLayout[1].data.begin());
        assert(!validateSCReadLayout(zaLayout, Enums::GameVersion::ZA).empty());
    }

    uint8_t oddString[]{'A', 0, 'B'};
    char16_t output[1]{};
    assert(Utils::loadString(oddString, sizeof oddString, output, 1) == 1);
    assert(output[0] == u'A');
    std::cout << "PLA structural/container validation: PASS (synthetic; not device tested)\n";
}
