#include "Legality/Gen4FormEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Form;

    // Diamond/Pearl predate the appliance Rotom and Origin/Sky form data.
    assert(formCount("diamond_nds", 479) == 1);
    assert(!isFormValid("diamond_nds", 479, 1));
    assert(formCount("diamond_nds", 487) == 1);
    assert(!isFormValid("diamond_nds", 487, 1));
    assert(formCount("diamond_nds", 492) == 1);
    assert(!isFormValid("diamond_nds", 492, 1));

    // Platinum and HG/SS expose the expanded native Gen IV form tables.
    assert(formCount("platinum_nds", 479) == 6);
    assert(isFormValid("platinum_nds", 479, 5));
    assert(!isFormValid("platinum_nds", 479, 6));
    assert(formCount("platinum_nds", 487) == 2);
    assert(isFormValid("platinum_nds", 487, 1));
    assert(formCount("heartgold_nds", 492) == 2);
    assert(isFormValid("heartgold_nds", 492, 1));

    // Origin-form Dialga/Palkia are later-generation forms, not native PK4 forms.
    assert(formCount("platinum_nds", 483) == 1);
    assert(!isFormValid("platinum_nds", 483, 1));
    assert(formCount("soulsilver_nds", 484) == 1);
    assert(!isFormValid("soulsilver_nds", 484, 1));

    // Arceus's plate/type forms are native to the Gen IV personal table.
    assert(formCount("platinum_nds", 493) == 18);
    assert(isFormValid("platinum_nds", 493, 17));
    assert(!isFormValid("platinum_nds", 493, 18));

    assert(!isFormValid("unknown", 479, 0));
    assert(!isFormValid("platinum_nds", 494, 0));

    std::cout << "Gen IV exact-game form evidence: PASS\n";
}
