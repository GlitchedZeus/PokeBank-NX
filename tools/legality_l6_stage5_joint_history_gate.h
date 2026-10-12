#pragma once
// L6 Stage 5: independently reviewable informational cross-lane candidate gate.
// No Pokémon/save mutation and no global Invalid verdict. A detached PK1-PK4
// does not authenticate original encounters, parents, move receipts or events.
#include <array>
#include <cstdint>

namespace Legality::Audit::JointHistory {
enum class Claim : uint8_t { Missing, Compatible, Contradicted };
enum class Status : uint8_t { Unresolved, OneAuthenticatedRoute, AllEnumeratedRoutesContradicted };
enum Lane : uint8_t { Encounter = 1, Breeding = 2, Moves = 4, Event = 8 };
struct Route {
    uint32_t pokemonIdentity = 0;
    uint32_t routeIdentity = 0;
    bool independentlyAuthenticated = false;
    bool completeSamePokemonHistory = false;
    uint8_t requiredLanes = 0;
    // L1 encounter, L2 breeding, L3 move and L4 event, in that order.
    std::array<Claim, 4> claims{};
};
struct Input {
    uint32_t pokemonIdentity = 0;
    std::array<Route, 12> routes{};
    uint8_t count = 0;
    bool everyOriginalRouteEnumerated = false;
    bool everyAlternativeMethodEnumerated = false;
};
struct Result {
    Status status = Status::Unresolved;
    uint8_t consistent = 0;
    uint8_t contradicted = 0;
    uint8_t incomplete = 0;
    uint8_t duplicateConflicts = 0;
    uint8_t authenticatedDistinctRoutes = 0;
};
constexpr bool sameWitness(const Route& a, const Route& b) noexcept {
    return a.pokemonIdentity == b.pokemonIdentity &&
           a.routeIdentity == b.routeIdentity &&
           a.completeSamePokemonHistory == b.completeSamePokemonHistory &&
           a.requiredLanes == b.requiredLanes && a.claims == b.claims;
}
constexpr bool validClaim(Claim c) noexcept {
    return c == Claim::Missing || c == Claim::Compatible || c == Claim::Contradicted;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if (!in.pokemonIdentity || !in.count || in.count > in.routes.size())
        return out;
    // Group by independent original-route identity, never by species, move,
    // retained game of origin, or speculative player choices. Duplicate
    // source packets with conflicting histories contaminate that route only.
    for (uint8_t i = 0; i < in.count; ++i) {
        const Route& r = in.routes[i];
        if (!r.independentlyAuthenticated)
            continue; // Unattested duplicate cannot erase an attested route.
        bool first = true, conflict = false;
        for (uint8_t j = 0; j < i; ++j) {
            const Route& earlier = in.routes[j];
            if (earlier.independentlyAuthenticated &&
                earlier.routeIdentity == r.routeIdentity &&
                earlier.pokemonIdentity == r.pokemonIdentity) {
                first = false;
                break;
            }
        }
        if (!first)
            continue;
        ++out.authenticatedDistinctRoutes;
        if (!r.routeIdentity || r.pokemonIdentity != in.pokemonIdentity ||
            !r.completeSamePokemonHistory || !(r.requiredLanes & 15u) ||
            (r.requiredLanes & ~15u)) {
            ++out.incomplete;
            continue;
        }
        for (uint8_t j = static_cast<uint8_t>(i + 1); j < in.count; ++j) {
            const Route& another = in.routes[j];
            if (another.independentlyAuthenticated &&
                another.routeIdentity == r.routeIdentity &&
                another.pokemonIdentity == r.pokemonIdentity &&
                !sameWitness(r, another))
                conflict = true;
        }
        if (conflict) {
            ++out.duplicateConflicts;
            ++out.incomplete;
            continue;
        }
        bool missing = false, contradiction = false;
        for (uint8_t lane = 0; lane < 4; ++lane) {
            const Claim claim = r.claims[lane];
            if (!validClaim(claim)) { missing = true; continue; }
            const bool required = (r.requiredLanes & (1u << lane)) != 0;
            if (!required && claim != Claim::Missing) missing = true;
            if (required && claim == Claim::Missing) missing = true;
            if (required && claim == Claim::Contradicted)
                contradiction = true;
        }
        // A failed selected route with unmodeled required stages cannot
        // exclude all other alternatives. Incomplete always wins locally.
        if (missing) ++out.incomplete;
        else if (contradiction) ++out.contradicted;
        else ++out.consistent;
    }
    // Every unauthenticated distinct hypothesis leaves route enumeration
    // incomplete, even when every authenticated candidate was contradicted.
    for (uint8_t i = 0; i < in.count; ++i) {
        const Route& r = in.routes[i];
        if (!r.independentlyAuthenticated) {
            bool represented = false;
            for (uint8_t j = 0; j < in.count; ++j)
                if (in.routes[j].independentlyAuthenticated &&
                    in.routes[j].routeIdentity == r.routeIdentity &&
                    in.routes[j].pokemonIdentity == r.pokemonIdentity)
                    represented = true;
            if (!represented) ++out.incomplete;
        }
    }
    if (out.consistent) {
        out.status = Status::OneAuthenticatedRoute;
    } else if (out.contradicted && !out.incomplete &&
               in.everyOriginalRouteEnumerated &&
               in.everyAlternativeMethodEnumerated) {
        // Candidate-set-only contradiction: this is NOT Severity::Invalid.
        out.status = Status::AllEnumeratedRoutesContradicted;
    }
    return out;
}
static_assert([] {
    Input in{}; in.pokemonIdentity = 123; in.count = 1;
    auto& r = in.routes[0]; r.pokemonIdentity = 123; r.routeIdentity = 1;
    r.independentlyAuthenticated = r.completeSamePokemonHistory = true;
    r.requiredLanes = Encounter | Moves;
    r.claims = {Claim::Compatible, Claim::Missing, Claim::Compatible, Claim::Missing};
    return evaluate(in).status == Status::OneAuthenticatedRoute;
}());
} // namespace Legality::Audit::JointHistory
