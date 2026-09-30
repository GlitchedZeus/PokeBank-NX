#include "Legality/Gen34EggState.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen34EggState;

    {
        const auto r = analyze(3, true, 0, 0);
        assert(r.applies());
        assert(!r.invalid());
        assert(r.evidence == Evidence::Consistent);
    }
    {
        const auto r = analyze(3, true, 0, 5);
        assert(r.invalid());
        assert(r.evidence == Evidence::InvalidMetLevel);
    }
    {
        const auto r = analyze(3, false, 0, 0);
        assert(!r.applies()); // hatched PK3 origin cannot be proven from these fields alone.
    }
    {
        const auto r = analyze(4, true, 2000, 0);
        assert(r.applies());
        assert(!r.invalid());
    }
    {
        const auto r = analyze(4, false, 2000, 0);
        assert(r.applies()); // hatched PK4 keeps EggLocation.
        assert(!r.invalid());
    }
    {
        const auto r = analyze(4, true, 0, 0);
        assert(r.invalid());
        assert(r.evidence == Evidence::MissingEggLocation);
    }
    {
        const auto r = analyze(4, false, 2000, 1);
        assert(r.invalid());
        assert(r.evidence == Evidence::InvalidMetLevel);
    }
    {
        const auto r = analyze(2, true, 0, 0);
        assert(!r.applies());
    }

    std::cout << "Gen III/IV egg-state legality evidence: PASS\n";
}
