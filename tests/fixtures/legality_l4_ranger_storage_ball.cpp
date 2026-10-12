#include "Legality/Gen4RangerStorageBallEvidence.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace B=Legality::Gen4RangerStorageBallEvidence;
int main() {
    uint64_t cases=0;
    // Exhaust the entire native byte pair domain. A stale or untrusted field
    // never overrides the active storage-family field on any value ordering.
    for(unsigned dp=0;dp<256;++dp)for(unsigned hg=0;hg<256;++hg) {
        auto a=B::select(B::Storage::DPPt,static_cast<uint8_t>(dp),static_cast<uint8_t>(hg));
        auto b=B::select(B::Storage::HGSS,static_cast<uint8_t>(dp),static_cast<uint8_t>(hg));
        auto u=B::select(B::Storage::Unknown,static_cast<uint8_t>(dp),static_cast<uint8_t>(hg));
        assert(a.storageKnown && a.ball==dp);
        assert(b.storageKnown && b.ball==hg);
        assert(!u.storageKnown && u.ball==0);
        cases+=3;
    }
    for(unsigned invalid=3;invalid<256;++invalid) {
        const auto unknown=B::select(static_cast<B::Storage>(invalid),4,16);
        assert(!unknown.storageKnown && unknown.ball==0);
        ++cases;
    }
    assert(B::select(B::Storage::DPPt,4,16).ball==4);
    assert(B::select(B::Storage::HGSS,16,4).ball==4);
    assert(cases==196861u);
    std::cout<<"L4 Ranger native storage Ball: "<<cases
             <<" active-field and unknown-tag checks: PASS\n";
}
