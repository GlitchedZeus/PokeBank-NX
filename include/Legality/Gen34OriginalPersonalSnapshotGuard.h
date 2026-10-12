#pragma once
// A single authentic Ditto source species has BOTH egg groups set to 13.
// A group-13 claim for any non-Ditto species, or an incomplete Ditto
// personal snapshot, is missing/incoherent evidence, not an egg exclusion.
// This is a read-only source-metadata guard; the child's bytes cannot
// prove what historical parents were deposited in retail daycare.
#include <cstdint>

namespace Legality::Gen34OriginalPersonalSnapshotGuard {
constexpr bool coherentDittoGroups(std::uint16_t species,
                                  std::uint8_t group1,
                                  std::uint8_t group2) noexcept {
    const bool ditto=species==132u;
    const bool groupOneDitto=group1==13u;
    const bool groupTwoDitto=group2==13u;
    return ditto ? (groupOneDitto && groupTwoDitto) :
                   (!groupOneDitto && !groupTwoDitto);
}
constexpr bool coherentDittoGenderRatio(std::uint16_t species,
                                        std::uint8_t genderRatio) noexcept {
    return species!=132u || genderRatio==255u;
}
constexpr bool validKnownGender(std::uint8_t gender) noexcept {
    // External source gender codes: 0 unknown, 1 male, 2 female,
    // 3 genderless. Other values are invalid rather than compatible.
    return gender<=3u;
}
static_assert(coherentDittoGroups(132,13,13));
static_assert(!coherentDittoGroups(132,13,0));
static_assert(!coherentDittoGroups(25,5,13));
static_assert(coherentDittoGroups(25,5,0));
static_assert(coherentDittoGenderRatio(132,255));
static_assert(!coherentDittoGenderRatio(132,127));
} // namespace Legality::Gen34OriginalPersonalSnapshotGuard
