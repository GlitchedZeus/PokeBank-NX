#pragma once
// L3 M06/M08: a deliberately narrow three-species original Nincada line.
// Source-checked separately against pokeruby@63a8cbf, pokeemerald@5eff786
// and pokefirered@c75f352; each original game has identical ordered rows.
// All entries are level-up offers, never proof that a native specimen
// performed the offer/decision or that a Shedinja was actually produced.
#include <array>
#include <cstdint>
namespace Legality::Gen14CandidateMoveGbaNinjaTables {
struct Offer {uint8_t level;uint16_t move;};
inline constexpr std::array<Offer,10> kNincada{{
 {1,10},{1,106},{5,141},{9,28},{14,154},{19,170},
 {25,206},{31,189},{38,232},{45,91}
}};
inline constexpr std::array<Offer,15> kNinjask{{
 {1,10},{1,106},{1,141},{1,28},{5,141},{9,28},
 {14,154},{19,170},{20,104},{20,210},{20,103},
 {25,14},{31,163},{38,97},{45,226}
}};
inline constexpr std::array<Offer,10> kShedinja{{
 {1,10},{1,106},{5,141},{9,28},{14,154},{19,170},
 {25,180},{31,109},{38,247},{45,288}
}};
static_assert(kNincada[5].move==170);
static_assert(kNinjask[8].level==20 && kNinjask[8].move==104);
static_assert(kNinjask[9].move==210 && kNinjask[10].move==103);
static_assert(kNinjask[13].move==97 && kNinjask[14].move==226);
static_assert(kShedinja[8].move==247);
} // namespace Legality::Gen14CandidateMoveGbaNinjaTables
