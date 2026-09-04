#include <cassert>
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include "Utils/FileUtilities.h"
#include "Save/PLAReadValidation.h"
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
    uint8_t oddString[]{'A', 0, 'B'};
    char16_t output[1]{};
    assert(Utils::loadString(oddString, sizeof oddString, output, 1) == 1);
    assert(output[0] == u'A');
    std::cout << "PLA structural/container validation: PASS (synthetic; not device tested)\n";
}
