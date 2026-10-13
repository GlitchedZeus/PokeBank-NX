// L5 source-scoped QA: independently compiled; never writes saves or declares global Invalid.
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string_view>
enum class Evidence { CandidateCompatible, CandidateContradiction, Unknown };
constexpr Evidence everstoneThreshold(bool sourceParentKnown, uint16_t draw, bool proposedPass) {
    if (!sourceParentKnown) return Evidence::Unknown;
    return (proposedPass == (draw < 0x7fffu)) ? Evidence::CandidateCompatible : Evidence::CandidateContradiction;
}
constexpr Evidence conditionalMoveOrder(bool versionKnown, std::string_view version,
        std::string_view species, std::string_view first, std::string_view second) {
    if (!versionKnown || species != "DUGTRIO") return Evidence::Unknown;
    if (version == "firered")
        return first == "SCRATCH" && second == "SAND_ATTACK"
            ? Evidence::CandidateCompatible : Evidence::CandidateContradiction;
    if (version == "leafgreen")
        return first == "SAND_ATTACK" && second == "SCRATCH"
            ? Evidence::CandidateCompatible : Evidence::CandidateContradiction;
    return Evidence::Unknown;
}
struct EventIdentity { int index, species, tid, sid, cardId; };
constexpr Evidence checkEventCandidate(bool originalAuthenticated, bool sourceFieldsKnown,
        const EventIdentity& expected, const EventIdentity& observed) {
    if (!sourceFieldsKnown || !originalAuthenticated) return Evidence::Unknown;
    if (expected.index == observed.index && expected.species == observed.species &&
        expected.tid == observed.tid && expected.sid == observed.sid &&
        expected.cardId == observed.cardId) return Evidence::CandidateCompatible;
    return Evidence::CandidateContradiction;
}
constexpr bool archiveCleared(bool authentic, bool digest, bool release, bool license) {
    return authentic && digest && release && license;
}
int main() {
    static_assert(everstoneThreshold(true,0x7ffe,true)==Evidence::CandidateCompatible);
    static_assert(everstoneThreshold(true,0x7fff,false)==Evidence::CandidateCompatible);
    static_assert(everstoneThreshold(true,0x7fff,true)==Evidence::CandidateContradiction);
    static_assert(everstoneThreshold(false,0x7ffe,true)==Evidence::Unknown);
    static_assert(conditionalMoveOrder(true,"firered","DUGTRIO","SCRATCH","SAND_ATTACK")==Evidence::CandidateCompatible);
    static_assert(conditionalMoveOrder(true,"leafgreen","DUGTRIO","SAND_ATTACK","SCRATCH")==Evidence::CandidateCompatible);
    static_assert(conditionalMoveOrder(true,"firered","DUGTRIO","SAND_ATTACK","SCRATCH")==Evidence::CandidateContradiction);
    static_assert(conditionalMoveOrder(false,"firered","DUGTRIO","SCRATCH","SAND_ATTACK")==Evidence::Unknown);
    constexpr EventIdentity a{0,4,7200,58364,67},b{1,4,7207,12105,15};
    static_assert(checkEventCandidate(true,true,a,a)==Evidence::CandidateCompatible);
    static_assert(checkEventCandidate(true,true,b,a)==Evidence::CandidateContradiction);
    static_assert(checkEventCandidate(false,true,a,a)==Evidence::Unknown);
    static_assert(checkEventCandidate(true,false,a,a)==Evidence::Unknown);
    static_assert(!archiveCleared(false,false,false,false));
    static_assert(!archiveCleared(true,true,false,true));
    std::cout << "L5 Q04/Q05/Q07 native source-scope 14 assertions: PASS\n";
}
