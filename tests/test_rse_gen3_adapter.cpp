#include "Encryption/Encryption3FRLG.h"
#include "Games/GameIdentity.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "Utils/Gen3Text.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

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

    enum class Family { RS, Emerald, FRLG };

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

    void writeName(std::span<uint8_t> bytes, size_t offset, size_t length,
                   std::u16string_view name) {
        std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), length,
                    Utils::GEN3_TERMINATOR);
        for (size_t index = 0; index < std::min(length, name.size()); ++index)
            bytes[offset + index] = Utils::charToGen3(name[index]);
    }

    uint32_t packIvs(const std::array<uint8_t, 6>& ivs) {
        uint32_t packed = 0;
        for (size_t index = 0; index < ivs.size(); ++index)
            packed |= static_cast<uint32_t>(ivs[index] & 0x1F) << (index * 5);
        return packed;
    }

    std::vector<uint8_t> makePokemon(uint16_t species, uint32_t pid, bool party) {
        std::vector<uint8_t> canonical(party ? 100 : 80, 0);
        write32(canonical, 0x00, pid);
        write16(canonical, 0x04, 54321);
        write16(canonical, 0x06, 12345);
        writeName(canonical, 0x08, 10, u"RSEMON");
        canonical[0x12] = 2;
        canonical[0x13] = 2;
        writeName(canonical, 0x14, 7, u"WILL");
        write16(canonical, 0x20, species);
        write16(canonical, 0x22, 13);
        write32(canonical, 0x24, 123456);
        const uint16_t moves[4] = {33, 45, 73, 22};
        const uint8_t pp[4] = {35, 40, 10, 10};
        for (size_t index = 0; index < 4; ++index) {
            write16(canonical, 0x2C + index * 2, moves[index]);
            canonical[0x34 + index] = pp[index];
        }
        const uint8_t evs[6] = {10, 20, 30, 40, 50, 60};
        std::copy(std::begin(evs), std::end(evs), canonical.begin() + 0x38);
        write32(canonical, 0x48, packIvs({31, 30, 29, 28, 27, 26}));
        write16(canonical, 0x1C, Encryption::checksum3FRLG(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(canonical.data()),
                                       canonical.size())));
        std::unique_ptr<std::byte[]> encrypted(Encryption::encryptArray3FRLG(
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(canonical.data()),
                                       canonical.size())));
        std::vector<uint8_t> result(canonical.size());
        std::memcpy(result.data(), encrypted.get(), result.size());
        return result;
    }

    using LogicalSectors = std::array<std::array<uint8_t, kSectorSize>, kSectorCount>;

    LogicalSectors makeLogicalSlot(Family family, uint16_t species, uint32_t pid) {
        LogicalSectors logical{};
        writeName(logical[0], 0x00, 8, u"WILL");
        logical[0][0x08] = 1;
        write16(logical[0], 0x0A, 54321);
        write16(logical[0], 0x0C, 12345);

        uint32_t securityKey = 0;
        if (family == Family::FRLG) {
            write32(logical[0], 0xAC, 1);
        } else if (family == Family::Emerald) {
            securityKey = 0xA1B2C3D4;
            write32(logical[0], 0xAC, securityKey);
            logical[0][0xEE0] = 0x42;
        } else {
            write32(logical[0], 0xAC, 0);
        }
        write32(logical[1], 0x490, 500000u ^ securityKey);

        // Real Gen III four-byte item entries. Ruby/Sapphire quantities are plaintext; Emerald
        // bag quantities use the low 16 bits of the save security key. PC item quantities are
        // plaintext in all three games. Potion (item 13) x3 mirrors the physical Emerald evidence
        // without hardcoding anything in production parsing.
        const auto writeInventoryEntry = [&](size_t offset, uint16_t itemId, uint16_t count,
                                             bool keyed) {
            write16(logical[1], offset, itemId);
            const uint16_t key16 = keyed ? static_cast<uint16_t>(securityKey) : 0;
            write16(logical[1], offset + 2, static_cast<uint16_t>(count ^ key16));
        };
        if (family == Family::RS) {
            writeInventoryEntry(0x0560, 13, 3, false);   // Items: Potion x3
            writeInventoryEntry(0x05B0, 259, 1, false); // Key Items
            writeInventoryEntry(0x0600, 4, 7, false);   // Poké Balls
            writeInventoryEntry(0x0640, 289, 2, false); // TM/HM
            writeInventoryEntry(0x0740, 133, 5, false); // Berries
            writeInventoryEntry(0x0498, 13, 9, false);  // PC Items
        } else if (family == Family::Emerald) {
            writeInventoryEntry(0x0560, 13, 3, true);
            writeInventoryEntry(0x05D8, 259, 1, true);
            writeInventoryEntry(0x0650, 4, 7, true);
            writeInventoryEntry(0x0690, 289, 2, true);
            writeInventoryEntry(0x0790, 133, 5, true);
            writeInventoryEntry(0x0498, 13, 9, false);
        }

        logical[1][0x234] = 1;
        const auto party = makePokemon(species, pid, true);
        std::copy(party.begin(), party.end(), logical[1].begin() + 0x238);

        const auto boxed = makePokemon(species, pid, false);
        std::copy(boxed.begin(), boxed.end(), logical[5].begin() + 4);

        // Deliberately cross the section-5/section-6 PC payload boundary. Do not simplify this
        // fixture to aligned-only storage: split PK3 reconstruction is part of the RSE contract.
        const auto splitBoxed = makePokemon(2, pid + 24, false);
        constexpr size_t splitLogical = 4 + 49 * 80;
        const size_t firstPart = kSectorData - splitLogical;
        std::copy_n(splitBoxed.begin(), firstPart, logical[5].begin() + splitLogical);
        std::copy(splitBoxed.begin() + static_cast<std::ptrdiff_t>(firstPart), splitBoxed.end(),
                  logical[6].begin());
        return logical;
    }

    void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter,
                   uint8_t rotation, Family family, uint16_t species, uint32_t pid) {
        auto logical = makeLogicalSlot(family, species, pid);
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

    std::vector<uint8_t> makeFixture(Family family, uint32_t counter0 = 7,
                                     uint32_t counter1 = 9, uint32_t newestPid = 0x12345679) {
        std::vector<uint8_t> save(kSaveSize, 0);
        writeSlot(save, 0, counter0, 0, family, 1, 0x23456789);
        writeSlot(save, 1, counter1, 5, family, 1, newestPid);
        return save;
    }

    size_t physicalSectorForId(std::span<const uint8_t> save, uint8_t slot, uint16_t wanted) {
        for (size_t physical = 0; physical < kSectorCount; ++physical) {
            const size_t offset = kSlotBase[slot] + physical * kSectorSize;
            if (read16(save, offset + 0xFF4) == wanted) return offset;
        }
        assert(false && "fixture missing logical section");
        return 0;
    }

    void corruptSignature(std::vector<uint8_t>& save, uint8_t slot, uint16_t id) {
        write32(save, physicalSectorForId(save, slot, id) + 0xFF8, 0);
    }

    void corruptChecksum(std::vector<uint8_t>& save, uint8_t slot, uint16_t id) {
        const size_t offset = physicalSectorForId(save, slot, id);
        save[offset + 0x20] ^= 0x5A;
    }

    void removeLogicalSectionByDuplicate(std::vector<uint8_t>& save, uint8_t slot,
                                         uint16_t missing, uint16_t duplicate) {
        const size_t offset = physicalSectorForId(save, slot, missing);
        write16(save, offset + 0xFF4, duplicate);
    }

    void overwriteInventoryEntry(std::vector<uint8_t>& save, uint8_t slot,
                                 size_t logicalOffset, uint16_t itemId, uint16_t storedCount) {
        const size_t sector = physicalSectorForId(save, slot, 1);
        write16(save, sector + logicalOffset, itemId);
        write16(save, sector + logicalOffset + 2, storedCount);
        write16(save, sector + 0xFF6, sectorChecksum(
            std::span<const uint8_t>(save.data() + sector, kChunkLengths[1])));
    }

    void writeFile(const fs::path& path, const std::vector<uint8_t>& bytes) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        assert(output.good());
    }

    std::vector<uint8_t> readFile(const fs::path& path) {
        std::ifstream input(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    void assertPokemon(const PokeVault::Integration::Gen3::PokemonRecord& pokemon,
                       uint32_t pid, uint16_t species = 1) {
        assert(pokemon.species == species);
        assert(pokemon.pid == pid);
        assert(pokemon.tid == 54321 && pokemon.sid == 12345);
        assert(pokemon.experience == 123456);
        assert(pokemon.heldItemGen3 == 13);
        assert((pokemon.moves == std::array<uint16_t, 4>{33, 45, 73, 22}));
        assert((pokemon.pp == std::array<uint8_t, 4>{35, 40, 10, 10}));
        assert((pokemon.ivs == std::array<uint8_t, 6>{31, 30, 29, 28, 27, 26}));
        assert((pokemon.evs == std::array<uint8_t, 6>{10, 20, 30, 40, 50, 60}));
        assert(pokemon.nickname == "RSEMON");
        assert(pokemon.otName == "WILL");
        assert(pokemon.checksumValid && pokemon.byteIdenticalRoundTrip);
    }

    void assertRSERead(const std::vector<uint8_t>& fixture,
                       PokeVault::Integration::Gen3::SourceGame game,
                       std::string_view expectedId, uint8_t expectedSlot,
                       uint32_t expectedCounter, uint32_t expectedPid) {
        using namespace PokeVault::Integration::Gen3;
        const auto untouched = fixture;
        auto parsed = parse(fixture, game);
        assert(parsed);
        assert(fixture == untouched);
        assert(parsed.save->metadata().sourceGameId == expectedId);
        assert(parsed.save->metadata().activeSlot == expectedSlot);
        assert(parsed.save->metadata().saveCounter == expectedCounter);
        assert(parsed.save->metadata().partyCount == 1);
        const auto& trainer = parsed.save->trainer();
        assert(trainer.name == "WILL");
        assert(trainer.gender == 1);
        assert(trainer.tid16 == 54321 && trainer.sid16 == 12345);
        assert(trainer.money == 500000);

        const auto& inventory = parsed.save->inventory();
        assert(inventory.size() == 6);
        const bool isEmerald = expectedId == "emerald_gba";
        const std::array<InventoryPouch, 6> expectedPouches = {
            InventoryPouch::Items, InventoryPouch::KeyItems, InventoryPouch::PokeBalls,
            InventoryPouch::TMCase, InventoryPouch::BerryPouch, InventoryPouch::PCItems,
        };
        const std::array<std::string_view, 6> expectedNames = {
            "Items", "Key Items", "Poké Balls", "TM/HM", "Berries", "PC Items",
        };
        const std::array<uint16_t, 6> rsCapacities = {20, 20, 16, 64, 46, 50};
        const std::array<uint16_t, 6> emeraldCapacities = {30, 30, 16, 64, 46, 50};
        const std::array<uint16_t, 6> expectedIds = {13, 259, 4, 289, 133, 13};
        const std::array<uint16_t, 6> expectedCounts = {3, 1, 7, 2, 5, 9};
        for (size_t index = 0; index < inventory.size(); ++index) {
            assert(inventory[index].pouch == expectedPouches[index]);
            assert(inventory[index].name == expectedNames[index]);
            assert(inventory[index].capacity ==
                   (isEmerald ? emeraldCapacities[index] : rsCapacities[index]));
            assert(inventory[index].countEncrypted == (isEmerald && index < 5));
            assert(inventory[index].items.size() == 1); // remaining zero-id slots are ignored
            assert(inventory[index].items.front().itemId == expectedIds[index]);
            assert(inventory[index].items.front().count == expectedCounts[index]);
        }

        const auto party = parsed.save->party();
        assert(parsed.save->lastEnumerationError() == SaveError::None);
        assert(party.size() == 1);
        assertPokemon(party.front(), expectedPid);

        const auto boxes = parsed.save->boxes();
        assert(parsed.save->lastEnumerationError() == SaveError::None);
        assert(boxes.size() == 2);
        const auto mainBox = std::find_if(boxes.begin(), boxes.end(), [](const auto& pokemon) {
            return pokemon.location.box == 0 && pokemon.location.slot == 0;
        });
        assert(mainBox != boxes.end());
        assertPokemon(*mainBox, expectedPid);
        const auto splitBox = std::find_if(boxes.begin(), boxes.end(), [](const auto& pokemon) {
            return pokemon.location.box == 1 && pokemon.location.slot == 19;
        });
        assert(splitBox != boxes.end());
        assertPokemon(*splitBox, expectedPid + 24, 2);
        assert(fixture == untouched);
    }
}

int main() {
    using namespace PokeVault::Integration::Gen3;

    const auto rs = makeFixture(Family::RS);
    assertRSERead(rs, SourceGame::RubyGBA, "ruby_gba", 1, 9, 0x12345679);
    assertRSERead(rs, SourceGame::SapphireGBA, "sapphire_gba", 1, 9, 0x12345679);
    assert(!parse(rs, SourceGame::EmeraldGBA));
    assert(!parse(rs, SourceGame::FireRedGBA));

    const auto emerald = makeFixture(Family::Emerald);
    assertRSERead(emerald, SourceGame::EmeraldGBA, "emerald_gba", 1, 9, 0x12345679);
    assert(!parse(emerald, SourceGame::RubyGBA));

    // Emerald's bag quantity is genuinely keyed on disk; PC Items remain plaintext.
    const size_t emeraldSection1 = physicalSectorForId(emerald, 1, 1);
    assert(read16(emerald, emeraldSection1 + 0x0562) ==
           static_cast<uint16_t>(3 ^ 0xC3D4));
    assert(read16(emerald, emeraldSection1 + 0x049A) == 9);

    auto invalidItem = rs;
    for (uint8_t slot = 0; slot < 2; ++slot)
        overwriteInventoryEntry(invalidItem, slot, 0x0560, 377, 1);
    const auto invalidItemUntouched = invalidItem;
    auto invalidItemResult = parse(invalidItem, SourceGame::RubyGBA);
    assert(invalidItemResult && invalidItemResult.error == SaveError::None);
    assert(invalidItem == invalidItemUntouched);
    assert(invalidItemResult.save->inventory().empty());
    assert(invalidItemResult.save->trainer().name == "WILL");
    assert(invalidItemResult.save->party().size() == 1);
    assert(invalidItemResult.save->lastEnumerationError() == SaveError::None);
    assert(invalidItemResult.save->boxes().size() == 2);
    assert(invalidItemResult.save->lastEnumerationError() == SaveError::None);

    auto impossibleCount = rs;
    for (uint8_t slot = 0; slot < 2; ++slot)
        overwriteInventoryEntry(impossibleCount, slot, 0x0560, 13, 1000);
    const auto impossibleCountUntouched = impossibleCount;
    auto impossibleCountResult = parse(impossibleCount, SourceGame::SapphireGBA);
    assert(impossibleCountResult && impossibleCountResult.error == SaveError::None);
    assert(impossibleCount == impossibleCountUntouched);
    assert(impossibleCountResult.save->inventory().empty());
    assert(impossibleCountResult.save->trainer().money == 500000);
    assert(impossibleCountResult.save->party().size() == 1);
    assert(impossibleCountResult.save->lastEnumerationError() == SaveError::None);
    assert(impossibleCountResult.save->boxes().size() == 2);
    assert(impossibleCountResult.save->lastEnumerationError() == SaveError::None);

    // Repeat the optional-inventory failure contract on Emerald, including keyed quantity storage.
    auto invalidEmeraldItem = emerald;
    for (uint8_t slot = 0; slot < 2; ++slot)
        overwriteInventoryEntry(invalidEmeraldItem, slot, 0x0560, 377,
                                static_cast<uint16_t>(1 ^ 0xC3D4));
    const auto invalidEmeraldUntouched = invalidEmeraldItem;
    auto invalidEmeraldResult = parse(invalidEmeraldItem, SourceGame::EmeraldGBA);
    assert(invalidEmeraldResult && invalidEmeraldResult.error == SaveError::None);
    assert(invalidEmeraldItem == invalidEmeraldUntouched);
    assert(invalidEmeraldResult.save->inventory().empty());
    assert(invalidEmeraldResult.save->trainer().name == "WILL");
    assert(invalidEmeraldResult.save->party().size() == 1);
    assert(invalidEmeraldResult.save->lastEnumerationError() == SaveError::None);
    assert(invalidEmeraldResult.save->boxes().size() == 2);
    assert(invalidEmeraldResult.save->lastEnumerationError() == SaveError::None);

    // Current PKHeX Gen III pouch semantics load fixed-width slots and treat count-zero entries
    // as unowned/clearable state. Preserve realistic stale slots instead of rejecting inventory.
    auto staleZero = rs;
    for (uint8_t slot = 0; slot < 2; ++slot)
        overwriteInventoryEntry(staleZero, slot, 0x0564, 600, 0);
    auto staleZeroResult = parse(staleZero, SourceGame::RubyGBA);
    assert(staleZeroResult);
    assert(staleZeroResult.save->inventory().size() == 6);
    assert(staleZeroResult.save->inventory()[0].items.size() == 1);
    assert(staleZeroResult.save->inventory()[0].items[0].itemId == 13);
    assert(staleZeroResult.save->inventory()[0].items[0].count == 3);

    auto staleEmerald = emerald;
    for (uint8_t slot = 0; slot < 2; ++slot)
        overwriteInventoryEntry(staleEmerald, slot, 0x0564, 600, 0xC3D4);
    auto staleEmeraldResult = parse(staleEmerald, SourceGame::EmeraldGBA);
    assert(staleEmeraldResult);
    assert(staleEmeraldResult.save->inventory().size() == 6);
    assert(staleEmeraldResult.save->inventory()[0].items.size() == 1);

    auto slotA = makeFixture(Family::RS, 12, 9);
    assertRSERead(slotA, SourceGame::RubyGBA, "ruby_gba", 0, 12, 0x23456789);

    auto corruptNewer = rs;
    corruptSignature(corruptNewer, 1, 7);
    assertRSERead(corruptNewer, SourceGame::RubyGBA, "ruby_gba", 0, 7, 0x23456789);

    auto badChecksum = rs;
    corruptChecksum(badChecksum, 0, 2);
    corruptChecksum(badChecksum, 1, 2);
    assert(!parse(badChecksum, SourceGame::RubyGBA));

    auto badSignature = rs;
    corruptSignature(badSignature, 0, 3);
    corruptSignature(badSignature, 1, 3);
    assert(!parse(badSignature, SourceGame::RubyGBA));

    auto missing = rs;
    removeLogicalSectionByDuplicate(missing, 0, 4, 5);
    removeLogicalSectionByDuplicate(missing, 1, 4, 5);
    assert(!parse(missing, SourceGame::RubyGBA));

    auto truncated = rs;
    truncated.resize(0x1FFFF);
    auto truncatedResult = parse(truncated, SourceGame::RubyGBA);
    assert(!truncatedResult && truncatedResult.error == SaveError::WrongSize);

    const auto frlgFamily = makeFixture(Family::FRLG);
    auto falseRuby = parse(frlgFamily, SourceGame::RubyGBA);
    assert(!falseRuby && falseRuby.error == SaveError::UnsupportedGame);

    // Discovery: exact release comes from the source filename, while parse independently proves
    // the underlying family. Arbitrary .state savestates are not candidates.
    char tempTemplate[] = "/tmp/pokebank-rse-XXXXXX";
    const char* created = mkdtemp(tempTemplate);
    assert(created != nullptr);
    const fs::path root(created);
    const fs::path rubyPath = root / "Pokemon Ruby.srm";
    const fs::path sapphirePath = root / "Pokemon Sapphire.sav";
    const fs::path emeraldPath = root / "Pokemon Emerald.srm";
    const fs::path noHintPath = root / "Mystery.sav";
    const fs::path multiHintPath = root / "Pokemon Ruby Sapphire.sav";
    writeFile(rubyPath, rs);
    writeFile(sapphirePath, rs);
    writeFile(emeraldPath, emerald);
    writeFile(noHintPath, rs);
    writeFile(multiHintPath, rs);
    writeFile(root / "FireRed Fake.sav", rs);
    writeFile(root / "unrelated.sav", {1, 2, 3});
    writeFile(root / "Pokemon Emerald.state", emerald);

    const auto rubyBefore = readFile(rubyPath);
    const std::vector<std::string> roots{root.string()};
    auto discovered = PokeVault::Legacy::discoverFRLGSaves(roots);
    assert(discovered.filesExamined == 7);
    assert(readFile(rubyPath) == rubyBefore);

    auto findReady = [&](std::string_view id) {
        return std::find_if(discovered.sources.begin(), discovered.sources.end(),
            [&](const auto& source) { return source.ready() && source.gameId == id; });
    };
    const auto ruby = findReady("ruby_gba");
    const auto sapphire = findReady("sapphire_gba");
    const auto foundEmerald = findReady("emerald_gba");
    assert(ruby != discovered.sources.end());
    assert(sapphire != discovered.sources.end());
    assert(foundEmerald != discovered.sources.end());
    assert(ruby->save->party().front().pid == 0x12345679);

    const auto noHint = std::find_if(discovered.sources.begin(), discovered.sources.end(),
        [&](const auto& source) { return source.path == noHintPath.string(); });
    const auto multiHint = std::find_if(discovered.sources.begin(), discovered.sources.end(),
        [&](const auto& source) { return source.path == multiHintPath.string(); });
    assert(noHint != discovered.sources.end() &&
           noHint->status == PokeVault::Legacy::LegacySourceStatus::AmbiguousIdentity);
    assert(multiHint != discovered.sources.end() &&
           multiHint->status == PokeVault::Legacy::LegacySourceStatus::AmbiguousIdentity);
    assert(std::none_of(discovered.sources.begin(), discovered.sources.end(), [](const auto& source) {
        return source.path.find(".state") != std::string::npos;
    }));

    // Refresh is a fresh raw battery-save read, not a cached model.
    const std::string stableIdentity = ruby->sourceIdentity;
    const std::string oldFingerprint = ruby->contentFingerprint;
    const auto refreshedBytes = makeFixture(Family::RS, 7, 10, 0x34567891);
    writeFile(rubyPath, refreshedBytes);
    auto refreshed = PokeVault::Legacy::discoverFRLGSaves(roots);
    const auto currentRuby = std::find_if(refreshed.sources.begin(), refreshed.sources.end(),
        [&](const auto& source) { return source.path == rubyPath.string(); });
    assert(currentRuby != refreshed.sources.end() && currentRuby->ready());
    assert(currentRuby->sourceIdentity == stableIdentity);
    assert(currentRuby->contentFingerprint != oldFingerprint);
    assert(currentRuby->save->party().front().pid == 0x34567891);
    assert(readFile(rubyPath) == refreshedBytes);

    const auto* rubyGame = PokeVault::Games::findGame("ruby_gba");
    const auto* sapphireGame = PokeVault::Games::findGame("sapphire_gba");
    const auto* emeraldGame = PokeVault::Games::findGame("emerald_gba");
    assert(rubyGame && sapphireGame && emeraldGame);
    assert(rubyGame->support == PokeVault::Games::SourceSupport::ReadOnly);
    assert(sapphireGame->support == PokeVault::Games::SourceSupport::ReadOnly);
    assert(emeraldGame->support == PokeVault::Games::SourceSupport::ReadOnly);
    assert(PokeVault::Games::gameCardArtworkPath("ruby_gba") ==
           "romfs:/game_cards/ruby_gba.png");
    assert(PokeVault::Games::gameCardArtworkPath("sapphire_gba") ==
           "romfs:/game_cards/sapphire_gba.png");
    assert(PokeVault::Games::gameCardArtworkPath("emerald_gba") ==
           "romfs:/game_cards/emerald_gba.png");
    assert(PokeVault::Games::gameCardArtworkPath("firered_gba") ==
           "romfs:/game_cards/firered_gba.png");
    assert(PokeVault::Games::gameCardArtworkPath("leafgreen_gba") ==
           "romfs:/game_cards/leafgreen_gba.png");

    fs::remove_all(root);
    std::cout << "RSE strict read-only adapter/discovery tests passed\n";
    return 0;
}
