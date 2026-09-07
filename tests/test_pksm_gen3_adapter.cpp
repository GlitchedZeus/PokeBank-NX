#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Utils/Gen3Text.h"
#include "Utils/SHA256.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace {
    constexpr size_t kSaveSize = 0x20000;
    constexpr size_t kSectorSize = 0x1000;
    constexpr size_t kSectorData = 0xF80;
    constexpr size_t kSectorCount = 14;
    constexpr size_t kSlotBase[2] = {0, 0xE000};
    constexpr std::array<size_t, kSectorCount> kChunkLengths = {
        0xF2C, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
        0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0,
    };

    void write16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
        bytes[offset] = static_cast<uint8_t>(value);
        bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
    }

    void write32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
        write16(bytes, offset, static_cast<uint16_t>(value));
        write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
    }

    uint16_t read16(std::span<const uint8_t> bytes, size_t offset) {
        return static_cast<uint16_t>(bytes[offset]) |
               static_cast<uint16_t>(bytes[offset + 1] << 8);
    }

    uint32_t read32(std::span<const uint8_t> bytes, size_t offset) {
        return static_cast<uint32_t>(read16(bytes, offset)) |
               (static_cast<uint32_t>(read16(bytes, offset + 2)) << 16);
    }

    uint16_t sectorChecksum(std::span<const uint8_t> data) {
        uint32_t sum = 0;
        for (size_t offset = 0; offset < data.size(); offset += 4) {
            uint32_t word = data[offset];
            if (offset + 1 < data.size()) word |= static_cast<uint32_t>(data[offset + 1]) << 8;
            if (offset + 2 < data.size()) word |= static_cast<uint32_t>(data[offset + 2]) << 16;
            if (offset + 3 < data.size()) word |= static_cast<uint32_t>(data[offset + 3]) << 24;
            sum += word;
        }
        return static_cast<uint16_t>((sum & 0xFFFFu) + (sum >> 16));
    }

    uint32_t packIvs(const std::array<uint8_t, 6>& ivs, bool abilityTwo) {
        uint32_t packed = 0;
        for (size_t i = 0; i < ivs.size(); ++i) {
            packed |= static_cast<uint32_t>(ivs[i] & 0x1F) << (i * 5);
        }
        if (abilityTwo) packed |= 1u << 31;
        return packed;
    }

    void writeName(std::span<uint8_t> bytes, size_t offset, size_t length,
                   std::u16string_view name) {
        std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), length,
                    Utils::GEN3_TERMINATOR);
        for (size_t i = 0; i < std::min(length, name.size()); ++i) {
            bytes[offset + i] = Utils::charToGen3(name[i]);
        }
    }

    std::vector<uint8_t> makePokemon(uint16_t species, uint32_t pid, bool party) {
        std::vector<uint8_t> canonical(party ? 100 : 80, 0);
        write32(canonical, 0x00, pid);                  // PID, permutation 1 for the main fixture
        write16(canonical, 0x04, 54321);               // TID
        write16(canonical, 0x06, 12345);               // SID
        writeName(canonical, 0x08, 10, u"BULBY");
        canonical[0x12] = 2;                           // English
        canonical[0x13] = 2;                           // has species
        writeName(canonical, 0x14, 7, u"WILL");

        write16(canonical, 0x20, species);             // Gen III internal species
        write16(canonical, 0x22, 13);                  // Potion, Gen III internal item
        write32(canonical, 0x24, 123456);
        canonical[0x28] = 0x39;                        // PP Ups 1/2/3/0
        canonical[0x29] = 200;

        const uint16_t moves[4] = {33, 45, 73, 22};   // Tackle/Growl/Leech Seed/Vine Whip
        const uint8_t pp[4] = {35, 40, 10, 10};
        for (size_t i = 0; i < 4; ++i) {
            write16(canonical, 0x2C + i * 2, moves[i]);
            canonical[0x34 + i] = pp[i];
        }
        const uint8_t evs[6] = {10, 20, 30, 40, 50, 60};
        std::copy(std::begin(evs), std::end(evs), canonical.begin() + 0x38);
        canonical[0x44] = 0x23;                        // Pokerus
        canonical[0x45] = 88;                          // met location
        write16(canonical, 0x46, static_cast<uint16_t>(5 | (4 << 7) | (4 << 11)));
        write32(canonical, 0x48, packIvs({31, 30, 29, 28, 27, 26}, true));
        write16(canonical, 0x1C, Encryption::checksum3FRLG(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(canonical.data()),
                                       canonical.size())));

        std::byte* encrypted = Encryption::encryptArray3FRLG(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(canonical.data()),
                                       canonical.size()));
        std::vector<uint8_t> result(canonical.size());
        std::memcpy(result.data(), encrypted, result.size());
        delete[] encrypted;
        return result;
    }

    using LogicalSectors = std::array<std::array<uint8_t, kSectorSize>, kSectorCount>;

    LogicalSectors makeLogicalSlot(uint16_t species, uint32_t pid) {
        LogicalSectors logical{};
        write32(logical[0], 0xAC, 1);                  // PKSM-Core FRLG family marker
        logical[1][0x34] = 1;
        const auto party = makePokemon(species, pid, true);
        std::copy(party.begin(), party.end(), logical[1].begin() + 0x38);
        const auto boxed = makePokemon(species, pid, false);
        std::copy(boxed.begin(), boxed.end(), logical[5].begin() + 4);
        // Box 2, slot 20 starts 44 bytes before a PC-sector boundary. Keep a second
        // deterministic record there to prove PKSM-Core and the wrapper assemble split PK3 data.
        const auto splitBoxed = makePokemon(2, pid + 24, false); // Ivysaur; same legal move family
        constexpr size_t splitLogical = 4 + 49 * 80;
        const size_t firstPart = kSectorData - splitLogical;
        std::copy_n(splitBoxed.begin(), firstPart, logical[5].begin() + splitLogical);
        std::copy(splitBoxed.begin() + static_cast<std::ptrdiff_t>(firstPart), splitBoxed.end(),
                  logical[6].begin());
        return logical;
    }

    void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter,
                   uint8_t rotation, uint16_t species, uint32_t pid) {
        auto logical = makeLogicalSlot(species, pid);
        for (size_t physical = 0; physical < kSectorCount; ++physical) {
            const uint16_t id = static_cast<uint16_t>((physical + rotation) % kSectorCount);
            const size_t offset = kSlotBase[slot] + physical * kSectorSize;
            std::copy(logical[id].begin(), logical[id].end(), save.begin() + offset);
            write16(save, offset + 0xFF4, id);
            write16(save, offset + 0xFF6, sectorChecksum(
                std::span<const uint8_t>(save.data() + offset, kChunkLengths[id])));
            write32(save, offset + 0xFF8, 0x08012025);
            write32(save, offset + 0xFFC, counter);
        }
    }

    std::vector<uint8_t> makeFixture() {
        std::vector<uint8_t> save(kSaveSize, 0);
        writeSlot(save, 0, 7, 0, 1, 0x23456789);       // older slot: distinct Bulbasaur
        writeSlot(save, 1, 9, 5, 1, 0x12345679);       // newest slot: Bulbasaur, PID % 24 == 1
        return save;
    }

    size_t physicalSectorForId(std::span<const uint8_t> save, uint8_t slot, uint16_t wanted) {
        for (size_t physical = 0; physical < kSectorCount; ++physical) {
            const size_t offset = kSlotBase[slot] + physical * kSectorSize;
            if (read16(save, offset + 0xFF4) == wanted) return offset;
        }
        assert(false && "fixture is missing requested logical sector");
        return 0;
    }

    void refreshSectorChecksum(std::vector<uint8_t>& save, size_t sectorOffset) {
        const uint16_t id = read16(save, sectorOffset + 0xFF4);
        write16(save, sectorOffset + 0xFF6, sectorChecksum(
            std::span<const uint8_t>(save.data() + sectorOffset, kChunkLengths[id])));
    }

    std::string sha256(std::span<const uint8_t> bytes) {
        uint8_t digest[Utils::PKSE_SHA256_HASH_SIZE];
        Utils::SHA256 hash;
        hash.update(bytes.data(), bytes.size());
        hash.finalize(digest);
        std::ostringstream out;
        for (uint8_t byte : digest) {
            out << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
        }
        return out.str();
    }

    void assertKnownPokemon(const PokeVault::Integration::Gen3::PokemonRecord& pokemon) {
        assert(pokemon.species == 1);
        assert(pokemon.pid == 0x12345679);
        assert(pokemon.tid == 54321);
        assert(pokemon.sid == 12345);
        assert(pokemon.experience == 123456);
        assert(pokemon.heldItem == 17);                 // PKSM-Core normalized item id
        assert(pokemon.heldItemGen3 == 13);             // exact Gen III item id
        assert((pokemon.moves == std::array<uint16_t, 4>{33, 45, 73, 22}));
        assert((pokemon.pp == std::array<uint8_t, 4>{35, 40, 10, 10}));
        assert((pokemon.ivs == std::array<uint8_t, 6>{31, 30, 29, 28, 27, 26}));
        assert((pokemon.evs == std::array<uint8_t, 6>{10, 20, 30, 40, 50, 60}));
        assert(pokemon.nickname == "BULBY");
        assert(pokemon.otName == "WILL");
        assert(pokemon.checksumValid);
        assert(pokemon.byteIdenticalRoundTrip);
    }
}

int main(int argc, char** argv) {
    using namespace PokeVault::Integration::Gen3;

    const auto fixture = makeFixture();
    const auto untouched = fixture;
    const std::string fixtureHash = sha256(fixture);
    std::cout << "Gen III deterministic fixture SHA-256: " << fixtureHash << '\n';
    assert(fixtureHash == "b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120");
    if (argc == 3 && std::string_view(argv[1]) == "--write-fixture") {
        std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(fixture.data()),
                     static_cast<std::streamsize>(fixture.size()));
        return output ? 0 : 2;
    }

    auto parsed = parse(fixture, SourceGame::FireRedGBA);
    assert(parsed);
    assert(parsed.error == SaveError::None);
    assert(parsed.save->metadata().sourceGameId == "firered_gba");
    assert(parsed.save->metadata().activeSlot == 1);
    assert(parsed.save->metadata().saveCounter == 9);
    assert(parsed.save->metadata().partyCount == 1);
    const auto party = parsed.save->party();
    assert(parsed.save->lastEnumerationError() == SaveError::None);
    assert(party.size() == 1);
    assertKnownPokemon(party.front());
    const auto boxes = parsed.save->boxes();
    assert(parsed.save->lastEnumerationError() == SaveError::None);
    assert(boxes.size() == 2);
    assertKnownPokemon(boxes.front());
    assert(boxes.front().location.kind == PokemonLocation::Kind::Box);
    assert(boxes.front().location.box == 0 && boxes.front().location.slot == 0);
    assert(boxes[1].species == 2);
    assert(boxes[1].pid == 0x12345691);
    assert(boxes[1].location.box == 1 && boxes[1].location.slot == 19);
    assert(boxes[1].checksumValid && boxes[1].byteIdenticalRoundTrip);
    assert(fixture == untouched);                         // parsing never mutates caller bytes

    // Independent cross-check: the inherited PKSE Gen III decryptor reaches the same canonical
    // fields and independently reproduces the encrypted source bytes.
    std::byte* independent = Encryption::decryptArray3FRLG(
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(party.front().originalBytes.data()),
                                   party.front().originalBytes.size()));
    const auto independentBytes = std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(independent), party.front().originalBytes.size());
    assert(read16(independentBytes, 0x20) == 1);
    assert(read32(independentBytes, 0x00) == party.front().pid);
    assert(read16(independentBytes, 0x04) == party.front().tid);
    assert(read16(independentBytes, 0x06) == party.front().sid);
    assert(read32(independentBytes, 0x24) == party.front().experience);
    assert(read16(independentBytes, 0x2C) == party.front().moves[0]);
    std::byte* independentlyEncrypted = Encryption::encryptArray3FRLG(
        std::span<const std::byte>(independent, party.front().originalBytes.size()));
    assert(std::memcmp(independentlyEncrypted, party.front().originalBytes.data(),
                       party.front().originalBytes.size()) == 0);
    delete[] independentlyEncrypted;
    delete[] independent;

    auto leafGreen = parse(fixture, SourceGame::LeafGreenGBA);
    assert(leafGreen && leafGreen.save->metadata().sourceGameId == "leafgreen_gba");

    auto truncated = fixture;
    truncated.pop_back();
    assert(parse(truncated, SourceGame::FireRedGBA).error == SaveError::WrongSize);

    auto badChecksum = fixture;
    badChecksum[0x100] ^= 0x80;
    badChecksum[0xE000 + 0x100] ^= 0x80;
    assert(parse(badChecksum, SourceGame::FireRedGBA).error == SaveError::BadSectorChecksum);

    auto badSignature = fixture;
    write32(badSignature, 0xFF8, 0);
    write32(badSignature, 0xE000 + 0xFF8, 0);
    assert(parse(badSignature, SourceGame::FireRedGBA).error == SaveError::BadSectorSignature);

    auto invalidSector = fixture;
    write16(invalidSector, 0xFF4, 14);
    write16(invalidSector, 0xE000 + 0xFF4, 14);
    assert(parse(invalidSector, SourceGame::FireRedGBA).error == SaveError::InvalidSectorId);

    auto duplicateSector = fixture;
    write16(duplicateSector, kSectorSize + 0xFF4, read16(duplicateSector, 0xFF4));
    write16(duplicateSector, 0xE000 + kSectorSize + 0xFF4,
            read16(duplicateSector, 0xE000 + 0xFF4));
    assert(parse(duplicateSector, SourceGame::FireRedGBA).error == SaveError::DuplicateSector);

    auto mismatchedCounter = fixture;
    write32(mismatchedCounter, kSectorSize + 0xFFC, 8);
    write32(mismatchedCounter, 0xE000 + kSectorSize + 0xFFC, 10);
    assert(parse(mismatchedCounter, SourceGame::FireRedGBA).error ==
           SaveError::MismatchedSectorCounters);

    // A bad newest slot falls back to the complete older rotating slot.
    auto fallback = fixture;
    write32(fallback, 0xE000 + 0xFF8, 0);
    auto fallbackParsed = parse(fallback, SourceGame::FireRedGBA);
    assert(fallbackParsed && fallbackParsed.save->metadata().activeSlot == 0);
    const auto fallbackParty = fallbackParsed.save->party();
    assert(fallbackParty.size() == 1 && fallbackParty.front().pid == 0x23456789);

    // Save counters are serial numbers; wrapping from UINT32_MAX to 0/1 still selects slot 1.
    auto wrapped = fixture;
    for (size_t physical = 0; physical < kSectorCount; ++physical) {
        write32(wrapped, physical * kSectorSize + 0xFFC, 0xFFFFFFFEu);
        write32(wrapped, 0xE000 + physical * kSectorSize + 0xFFC, 1);
    }
    auto wrappedParsed = parse(wrapped, SourceGame::FireRedGBA);
    assert(wrappedParsed && wrappedParsed.save->metadata().activeSlot == 1);

    auto unsupported = fixture;
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const size_t block0 = physicalSectorForId(unsupported, slot, 0);
        write32(unsupported, block0 + 0xAC, 0);
        refreshSectorChecksum(unsupported, block0);
    }
    assert(parse(unsupported, SourceGame::FireRedGBA).error == SaveError::UnsupportedGame);

    auto invalidPartyCount = fixture;
    const size_t partySector = physicalSectorForId(invalidPartyCount, 1, 1);
    invalidPartyCount[partySector + 0x34] = 7;
    refreshSectorChecksum(invalidPartyCount, partySector);
    assert(parse(invalidPartyCount, SourceGame::FireRedGBA).error ==
           SaveError::InvalidPartyCount);

    // A Pokemon corruption with a freshly valid save-sector checksum reaches entity validation,
    // then fails safely instead of being presented as a valid record.
    auto malformedPokemon = fixture;
    const size_t activeBlock1 = physicalSectorForId(malformedPokemon, 1, 1);
    malformedPokemon[activeBlock1 + 0x38 + 0x20] ^= 0x40;
    refreshSectorChecksum(malformedPokemon, activeBlock1);
    auto malformedParsed = parse(malformedPokemon, SourceGame::FireRedGBA);
    assert(malformedParsed);
    assert(malformedParsed.save->party().empty());
    assert(malformedParsed.save->lastEnumerationError() == SaveError::MalformedPokemon);

    static_assert(PKSM_CORE_REVISION == "aa22d7a4f87c0351baf7da5962ba5acd01039a7c");
#if defined(POKEBANK_GEN3_SELECTIVE_PORT_TEST)
    std::cout << "Exception-free native Gen III slice tests passed\n";
#else
    std::cout << "PKSM-Core Gen III adapter tests passed\n";
#endif
    return 0;
}
