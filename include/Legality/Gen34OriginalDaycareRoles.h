#pragma once
// Original-source *structural* pair-role bounds, not a reconstruction of
// historical parents from an egg or a complete egg-group compatibility test.
// A matching role only permits continued evaluation of separately attested
// original daycare evidence; it never proves global Pokemon legality.
#include <cstdint>

namespace Legality::Gen34OriginalDaycareRoles {
enum class Status : std::uint8_t {
    UnknownOriginalMetadata, IncompatibleOrdinaryPair,
    StructurallyCompatible
};
struct Parent {
    std::uint16_t species = 0;
    std::uint8_t gender = 2; // source gender: 0 male, 1 female, 2 genderless
};
constexpr Status evaluate(Parent left, Parent right,
                          std::uint16_t sourceMaxSpecies) noexcept {
    if(!left.species || !right.species ||
       left.species>sourceMaxSpecies || right.species>sourceMaxSpecies ||
       left.gender>2 || right.gender>2)
        return Status::UnknownOriginalMetadata;
    const bool ditto0=left.species==132u;
    const bool ditto1=right.species==132u;
    // Two Ditto never breed, including in a malformed claimed snapshot.
    if(ditto0 && ditto1) return Status::IncompatibleOrdinaryPair;
    // A declared Ditto with male/female gender is not an attested retail
    // parent snapshot: quarantine instead of treating it as an exclusion.
    if((ditto0 && left.gender!=2) || (ditto1 && right.gender!=2))
        return Status::UnknownOriginalMetadata;
    if(ditto0 || ditto1) return Status::StructurallyCompatible;
    const bool opposite=(left.gender==0 && right.gender==1) ||
                        (left.gender==1 && right.gender==0);
    return opposite ? Status::StructurallyCompatible :
                      Status::IncompatibleOrdinaryPair;
}
static_assert(evaluate({25,1},{25,0},386)==Status::StructurallyCompatible);
static_assert(evaluate({25,0},{25,0},386)==Status::IncompatibleOrdinaryPair);
static_assert(evaluate({132,2},{81,2},493)==Status::StructurallyCompatible);
static_assert(evaluate({132,2},{132,2},493)==Status::IncompatibleOrdinaryPair);
static_assert(evaluate({494,1},{25,0},493)==Status::UnknownOriginalMetadata);
} // namespace Legality::Gen34OriginalDaycareRoles
