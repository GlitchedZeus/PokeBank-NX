#ifndef POKEBANK_GEN5_POKEDEX_H
#define POKEBANK_GEN5_POKEDEX_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace PokeVault::Integration::Gen5 {

// Read-only native Pokedex (Zukan) per pinned PKHeX 6501f0ab:
//   SaveBlockAccessor5BW.cs Zukan = Block(55) (0x21600 len 0x4D4),
//   SaveBlockAccessor5B2W2.cs Zukan = Block(54) (0x21400 len 0x4DC),
//   Saves/Substructures/PokeDex/Zukan5.cs: OFS_CAUGHT 0x8, OFS_SEEN 0x5C,
//   four 0x54-byte seen regions (male/female x normal/shiny), bit = species-1,
//   MaxSpecies 649. No flag is ever written: there is no Pokedex editor.
namespace DexLayout5 {
inline constexpr size_t BlackWhiteBlockIndex = 55;
inline constexpr size_t Black2White2BlockIndex = 54;
inline constexpr size_t CaughtOffset = 0x08;
inline constexpr size_t SeenOffset = 0x5C;
inline constexpr size_t RegionSize = 0x54;
inline constexpr size_t SeenRegions = 4;
inline constexpr uint16_t MaxSpecies = 649;
static_assert(LayoutInfo::BlackWhite[BlackWhiteBlockIndex].offset == 0x21600 &&
              LayoutInfo::BlackWhite[BlackWhiteBlockIndex].length == 0x4D4);
static_assert(LayoutInfo::Black2White2[Black2White2BlockIndex].offset == 0x21400 &&
              LayoutInfo::Black2White2[Black2White2BlockIndex].length == 0x4DC);
static_assert(SeenOffset == CaughtOffset + RegionSize);
} // namespace DexLayout5

struct DexEntry5 { bool seen = false; bool caught = false; };
struct Pokedex5 {
    std::array<DexEntry5, DexLayout5::MaxSpecies> entries{};  // index = species - 1
    size_t seen = 0, caught = 0;
};

[[nodiscard]] inline std::optional<Pokedex5> decodePokedex5(const Gen5ReadOnlySave& save) {
    using namespace DexLayout5;
    const auto& block = save.family() == SaveFamily::BlackWhite
        ? LayoutInfo::BlackWhite[BlackWhiteBlockIndex] : LayoutInfo::Black2White2[Black2White2BlockIndex];
    const auto bytes = save.sourceBytes();
    const size_t at = save.selectedCopyOffset() + block.offset;
    if (at > bytes.size() || block.length > bytes.size() - at ||
        SeenOffset + SeenRegions * RegionSize > block.length)
        return std::nullopt;
    const auto dex = bytes.subspan(at, block.length);
    const auto flag = [&](size_t ofs, size_t bit) { return (dex[ofs + (bit >> 3)] >> (bit & 7)) & 1u; };
    Pokedex5 out;
    for (size_t bit = 0; bit < MaxSpecies; ++bit) {
        auto& e = out.entries[bit];
        e.caught = flag(CaughtOffset, bit) != 0;
        for (size_t region = 0; region < SeenRegions && !e.seen; ++region)
            e.seen = flag(SeenOffset + region * RegionSize, bit) != 0;
        out.seen += e.seen;
        out.caught += e.caught;
    }
    return out;
}

// Read-only list cursor for the shared Trainer panel's Pokedex view.
class Gen5PokedexView {
public:
    [[nodiscard]] bool open() const noexcept { return open_; }
    [[nodiscard]] uint16_t cursor() const noexcept { return cursor_; }   // species - 1
    void openAt(uint16_t species) noexcept {
        open_ = true;
        cursor_ = species >= 1 && species <= DexLayout5::MaxSpecies ? uint16_t(species - 1) : 0;
    }
    void close() noexcept { open_ = false; }
    void move(int delta) noexcept {
        if (!open_) return;
        cursor_ = static_cast<uint16_t>(std::clamp(int(cursor_) + delta, 0, int(DexLayout5::MaxSpecies) - 1));
    }
    // Next/previous species with any seen/caught flag (wraps), for a quick jump.
    void jumpRegistered(const Pokedex5& dex, int direction) noexcept {
        if (!open_ || direction == 0) return;
        for (int step = 1; step <= int(DexLayout5::MaxSpecies); ++step) {
            const int i = ((int(cursor_) + direction * step) % int(DexLayout5::MaxSpecies) +
                           int(DexLayout5::MaxSpecies)) % int(DexLayout5::MaxSpecies);
            if (dex.entries[size_t(i)].seen || dex.entries[size_t(i)].caught) {
                cursor_ = uint16_t(i);
                return;
            }
        }
    }

private:
    bool open_ = false;
    uint16_t cursor_ = 0;
};

} // namespace PokeVault::Integration::Gen5
#endif
