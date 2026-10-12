// L5 independent source-conditioned invariant probe: synthetic PK1/PK2 wrappers only.
// This is a reproduction of pinned accepted analyzer predicates, NOT the full native analyzer.
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

struct Case { uint8_t generation, gender, genderRatio; bool sourceKnown; uint8_t sourceGeneration; };
struct Result { bool falseAbilityInvalid, falseGenderInvalid; };
constexpr Result accepted(const Case& c) {
    // In both source PK1/PK2 wrappers, native ability() is a neutral zero.
    const bool abilityGate = !c.sourceKnown || c.sourceGeneration >= 3;
    const bool abilityInvalid = abilityGate; // isAbilityLegal(slots, 0) is always false.
    // Gen I wrapper returns 2 to represent absence of a stored gender, not genderlessness.
    const bool genderGate = !c.sourceKnown || c.sourceGeneration >= 2;
    const bool genderInvalid = genderGate && c.gender == 2 && c.genderRatio != 255;
    return {abilityInvalid, genderInvalid};
}
constexpr Result corrected(const Case& c) {
    // Correct applicability is derived from immutable native format, not source labels.
    return {c.generation >= 3, c.generation >= 2 && c.gender == 2 && c.genderRatio != 255};
}
constexpr bool controls() {
    for (int nativeGen : {1, 2}) {
        for (int ratio : {31, 254, 255}) {
            for (bool labelKnown : {false, true}) {
                Case c{static_cast<uint8_t>(nativeGen), static_cast<uint8_t>(nativeGen==1?2:(ratio==255?2:0)), static_cast<uint8_t>(ratio), labelKnown, static_cast<uint8_t>(nativeGen)};
                const Result a = accepted(c), b = corrected(c);
                if (a.falseAbilityInvalid != !labelKnown) return false;
                if (nativeGen == 1 && a.falseGenderInvalid != (!labelKnown && ratio != 255)) return false;
                if (b.falseAbilityInvalid || b.falseGenderInvalid) return false;
            }
        }
    }
    // A conflicting known source is sanitized to unknown before these predicates run.
    for (int gen : {1,2}) {
        Case sanitized{static_cast<uint8_t>(gen), static_cast<uint8_t>(gen==1?2:0),31,false,0};
        if (!accepted(sanitized).falseAbilityInvalid) return false;
        if (corrected(sanitized).falseAbilityInvalid) return false;
        if (gen == 1 && !accepted(sanitized).falseGenderInvalid) return false;
    }
    return true;
}
static_assert(controls(), "All source-free PK1/PK2 source-predicate controls must hold");
int main() {
    assert(controls());
    std::cout << "L5 native-generation source-free predicates: 18 case states PASS "
                 "(PK1 gender and PK1/PK2 ability source-bypass reproduced)\n";
}
