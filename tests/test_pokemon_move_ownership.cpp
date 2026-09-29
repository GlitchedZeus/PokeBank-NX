#include "Pokemon/Pokemon.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

class TestPokemon final : public Pokemon::Pokemon {
public:
    explicit TestPokemon(const std::vector<uint8_t>& bytes) {
        dataSize = bytes.size();
        buffer = dataSize ? new std::byte[dataSize] : nullptr;
        for (std::size_t i = 0; i < dataSize; ++i)
            buffer[i] = static_cast<std::byte>(bytes[i]);
        data = buffer ? std::span<std::byte>(buffer, dataSize) : std::span<std::byte>{};
    }

    TestPokemon(TestPokemon&&) noexcept = default;
    TestPokemon& operator=(TestPokemon&&) noexcept = default;

    uint16_t speciesID() const noexcept override { return 1; }
    const char* species() const noexcept override { return "Test"; }
    std::u16string nickname() const override { return u"Test"; }
    uint8_t formID() const noexcept override { return 0; }
    uint8_t form() const noexcept override { return 0; }
    uint16_t heldItem() const noexcept override { return 0; }
    uint32_t id32() const noexcept override { return 0; }
    uint32_t exp() const noexcept override { return 0; }
    uint16_t ability() const noexcept override { return 0; }
    uint8_t nature() const noexcept override { return 0; }
    uint8_t statNature() const noexcept override { return 0; }
    uint8_t level() const noexcept override { return 1; }
    uint8_t gender() const noexcept override { return 0; }
    const char* genderSymbol() const noexcept override { return ""; }
    uint32_t pid() const noexcept override { return 0; }
    uint32_t encryptionConstant() const noexcept override { return 0; }

    uint8_t ivHP() const noexcept override { return 0; }
    uint8_t ivATK() const noexcept override { return 0; }
    uint8_t ivDEF() const noexcept override { return 0; }
    uint8_t ivSPE() const noexcept override { return 0; }
    uint8_t ivSPA() const noexcept override { return 0; }
    uint8_t ivSPD() const noexcept override { return 0; }
    void setIV(int, uint8_t) noexcept override {}

    uint8_t evHP() const noexcept override { return 0; }
    uint8_t evATK() const noexcept override { return 0; }
    uint8_t evDEF() const noexcept override { return 0; }
    uint8_t evSPE() const noexcept override { return 0; }
    uint8_t evSPA() const noexcept override { return 0; }
    uint8_t evSPD() const noexcept override { return 0; }
    void setEV(int, uint8_t) noexcept override {}

    Enums::GameVersion getGameGroup() const noexcept override { return Enums::GameVersion::Invalid; }

    uint8_t baseHP() const noexcept override { return 1; }
    uint8_t baseATK() const noexcept override { return 1; }
    uint8_t baseDEF() const noexcept override { return 1; }
    uint8_t baseSPE() const noexcept override { return 1; }
    uint8_t baseSPA() const noexcept override { return 1; }
    uint8_t baseSPD() const noexcept override { return 1; }

    uint16_t statHPMax() const noexcept override { return 1; }
    uint16_t statATK() const noexcept override { return 1; }
    uint16_t statDEF() const noexcept override { return 1; }
    uint16_t statSPE() const noexcept override { return 1; }
    uint16_t statSPA() const noexcept override { return 1; }
    uint16_t statSPD() const noexcept override { return 1; }

    uint8_t friendship() const noexcept override { return 0; }
    bool isEgg() const noexcept override { return false; }
    bool isShiny(uint32_t, std::string) const noexcept override { return false; }
    bool isPokerusInfected() const noexcept override { return false; }
    bool isPokerusCured() const noexcept override { return false; }

    uint16_t checksum() const noexcept override { return 0; }
    uint16_t calculateChecksum() const noexcept override { return 0; }
    void refreshChecksum() noexcept override {}
    bool checksumValid() const noexcept override { return true; }
    void recalculateStats() noexcept override {}
    void regeneratePID(uint32_t) noexcept override {}
    void setShiny(bool, uint32_t) noexcept override {}
};

void expectBytes(const TestPokemon& pokemon, const std::vector<uint8_t>& expected) {
    const auto bytes = pokemon.getData();
    assert(bytes.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i)
        assert(std::to_integer<uint8_t>(bytes[i]) == expected[i]);
}

} // namespace

int main() {
    std::unique_ptr<TestPokemon> constructed;
    {
        TestPokemon source({0x11, 0x80, 0xFF, 0x44});
        const std::byte* original = source.getData().data();
        constructed = std::make_unique<TestPokemon>(std::move(source));
        assert(source.getData().empty());
        assert(source.getDataSize() == 0);
        assert(constructed->getData().data() == original);
    }
    // The moved-from object has been destroyed; the destination must still own valid bytes.
    expectBytes(*constructed, {0x11, 0x80, 0xFF, 0x44});

    TestPokemon assigned({0xAA, 0xBB}); // owns an allocation that move-assignment must release.
    {
        TestPokemon source({0x01, 0x02, 0xFE, 0xFD, 0xFC});
        const std::byte* original = source.getData().data();
        assigned = std::move(source);
        assert(source.getData().empty());
        assert(source.getDataSize() == 0);
        assert(assigned.getData().data() == original);
    }
    expectBytes(assigned, {0x01, 0x02, 0xFE, 0xFD, 0xFC});

    // Self-move must retain ownership and bytes.
    const std::byte* beforeSelfMove = assigned.getData().data();
    assigned = std::move(assigned);
    assert(assigned.getData().data() == beforeSelfMove);
    expectBytes(assigned, {0x01, 0x02, 0xFE, 0xFD, 0xFC});

    std::cout << "Pokemon raw-buffer move ownership: PASS\n";
    return 0;
}
