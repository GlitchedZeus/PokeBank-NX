// L6 Stage 5 audit-only native cross-lane adversarial contract.
// No fake source packets are promoted to Pokémon legality; these are
// synthetic, explicitly authenticated *hypothetical* route controls.
#include "tools/legality_l6_stage5_joint_history_gate.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace J = Legality::Audit::JointHistory;
using J::Claim;
using J::Status;
using J::Route;
using J::Input;

Route route(uint32_t id, uint8_t mask, Claim value) {
    Route r{};
    r.pokemonIdentity = 4242; r.routeIdentity = id;
    r.independentlyAuthenticated = r.completeSamePokemonHistory = true;
    r.requiredLanes = mask;
    for (unsigned lane = 0; lane < 4; ++lane)
        r.claims[lane] = (mask & (1u << lane)) ? value : Claim::Missing;
    return r;
}
Input base() {
    Input in{}; in.pokemonIdentity = 4242;
    in.everyOriginalRouteEnumerated = true;
    in.everyAlternativeMethodEnumerated = true;
    return in;
}
void scenarios(unsigned& checks) {
    auto in = base(); in.count = 2;
    in.routes[0] = route(1, J::Encounter | J::Moves, Claim::Contradicted);
    in.routes[1] = route(2, J::Encounter | J::Moves, Claim::Compatible);
    auto out = J::evaluate(in);
    assert(out.status == Status::OneAuthenticatedRoute);
    assert(out.consistent == 1 && out.contradicted == 1);
    checks += 3;
    std::swap(in.routes[0], in.routes[1]);
    assert(J::evaluate(in).status == Status::OneAuthenticatedRoute); ++checks;
    in.routes[1] = route(1, J::Encounter | J::Moves, Claim::Contradicted);
    assert(J::evaluate(in).status == Status::OneAuthenticatedRoute); ++checks;
    // An unauthenticated duplicate may not invalidate a real later witness.
    in.routes[1].routeIdentity = 2;
    in.routes[1].independentlyAuthenticated = false;
    assert(J::evaluate(in).status == Status::OneAuthenticatedRoute); ++checks;
    // Two authenticated packets for the same route that disagree on a
    // source/learning claim cannot be "combined" into a fictitious route.
    in.routes[1] = route(2, J::Encounter | J::Moves, Claim::Compatible);
    in.routes[0] = in.routes[1];
    in.routes[1].claims[2] = Claim::Contradicted;
    out = J::evaluate(in);
    assert(out.status == Status::Unresolved && out.duplicateConflicts == 1);
    checks += 2;
    // Two different route IDs with disjoint positive evidence cannot be
    // joined by species, four moves, or event title alone.
    in.routes[0] = route(11, J::Encounter | J::Moves, Claim::Missing);
    in.routes[0].claims[0] = Claim::Compatible;
    in.routes[1] = route(12, J::Encounter | J::Moves, Claim::Missing);
    in.routes[1].claims[2] = Claim::Compatible;
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
    in.count = 1;
    in.routes[0] = route(11, J::Encounter | J::Moves, Claim::Contradicted);
    out = J::evaluate(in);
    assert(out.status == Status::AllEnumeratedRoutesContradicted);
    ++checks;
    for (unsigned bit = 0; bit < 2; ++bit) {
        auto gated = in;
        if (bit == 0) gated.everyOriginalRouteEnumerated = false;
        else gated.everyAlternativeMethodEnumerated = false;
        assert(J::evaluate(gated).status == Status::Unresolved); ++checks;
    }
    // Uncertain historical parent/event method must dominate an otherwise
    // contradicted candidate even when flags falsely assert enumerated all.
    in.routes[0].claims[2] = Claim::Missing;
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
    in.routes[0] = route(0, J::Encounter, Claim::Compatible);
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
    in.routes[0] = route(33, J::Encounter, Claim::Compatible);
    in.routes[0].pokemonIdentity = 4243;  // Different individual.
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
    in.routes[0] = route(33, J::Encounter, Claim::Compatible);
    in.routes[0].completeSamePokemonHistory = false;
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
    in.routes[0] = route(33, J::Encounter, Claim::Compatible);
    in.routes[0].claims[1] = Claim::Compatible; // Non-required L2 packet.
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
    in.routes[0] = route(33, J::Encounter, Claim::Compatible);
    in.routes[0].independentlyAuthenticated = false;
    assert(J::evaluate(in).status == Status::Unresolved); ++checks;
}
void exhaustiveClaimPatterns(unsigned& checks) {
    // Exhaust all 15 meaningful lane masks and 3^4 claim assignments.
    for (uint8_t mask = 1; mask <= 15; ++mask) {
        for (unsigned ternary = 0; ternary < 81; ++ternary) {
            auto in = base(); in.count = 1;
            auto r = route(1, mask, Claim::Missing);
            unsigned code = ternary;
            bool missing = false, contradicted = false;
            for (unsigned lane = 0; lane != 4; ++lane) {
                r.claims[lane] = static_cast<Claim>(code % 3);
                code /= 3;
                if ((mask & (1u << lane)) == 0) {
                    if (r.claims[lane] != Claim::Missing) missing = true;
                } else {
                    if (r.claims[lane] == Claim::Missing) missing = true;
                    if (r.claims[lane] == Claim::Contradicted) contradicted = true;
                }
            }
            in.routes[0] = r;
            const auto out = J::evaluate(in);
            const Status expected = missing ? Status::Unresolved :
                contradicted ? Status::AllEnumeratedRoutesContradicted :
                               Status::OneAuthenticatedRoute;
            assert(out.status == expected);
            assert(out.consistent + out.contradicted + out.incomplete == 1);
            checks += 2;
        }
    }
}
uint32_t randomNext(uint32_t& state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
void permutationFuzz(unsigned& checks) {
    uint32_t seed = 0x174bed25u;
    for (unsigned trial = 0; trial != 5000; ++trial) {
        auto in = base(); in.count = 8;
        in.everyOriginalRouteEnumerated = (randomNext(seed) & 1u) != 0;
        in.everyAlternativeMethodEnumerated = (randomNext(seed) & 1u) != 0;
        for (auto& r : in.routes) r = {};
        for (unsigned i = 0; i < in.count; ++i) {
            const auto mask = static_cast<uint8_t>((randomNext(seed) % 15) + 1);
            const Claim c = static_cast<Claim>(randomNext(seed) % 3);
            in.routes[i] = route(1 + randomNext(seed) % 5, mask, c);
            in.routes[i].independentlyAuthenticated = (randomNext(seed) % 7) != 0;
            in.routes[i].completeSamePokemonHistory = (randomNext(seed) % 6) != 0;
            if ((randomNext(seed) % 11) == 0)
                in.routes[i].claims[randomNext(seed) % 4] = Claim::Missing;
        }
        const Input before = in;
        const auto a = J::evaluate(in);
        std::reverse(in.routes.begin(), in.routes.begin() + in.count);
        const auto b = J::evaluate(in);
        assert(a.status == b.status && a.consistent == b.consistent &&
               a.contradicted == b.contradicted &&
               a.incomplete == b.incomplete &&
               a.duplicateConflicts == b.duplicateConflicts);
        for (unsigned i = 0; i < before.count; ++i)
            assert(J::sameWitness(before.routes[i], in.routes[before.count - 1 - i]) &&
                   before.routes[i].independentlyAuthenticated ==
                       in.routes[before.count - 1 - i].independentlyAuthenticated);
        checks += 6 + before.count;
    }
}
int main() {
    unsigned checks = 0;
    scenarios(checks);
    exhaustiveClaimPatterns(checks);
    permutationFuzz(checks);
    assert(checks >= 72000);
    std::cout << "L6 Stage5 authenticated joint-route adversarial checks: "
              << checks << " PASS (conditional, no global Invalid)\n";
}
