#include "Legality/Gen4EventArchiveAttestation.h"
// Exhaustive, bounded 10,368-state original Gen IV archive authenticity
// matrix. This NEVER authenticates real WC4 cards; every witness here is
// synthetic and demands release receipt, checked nonzero source digest,
// original/converted archive grade and three coherent original field hashes.
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace Legality::Gen4EventArchiveAttestation;
int main() {
    unsigned long long count=0, positive=0, failed=0;
    for (unsigned grade=0; grade<6; ++grade)
    for (unsigned digest=0; digest<2; ++digest)
    for (unsigned release=0; release<2; ++release)
    for (unsigned havePayload=0; havePayload<2; ++havePayload)
    for (unsigned fieldOrigins=0; fieldOrigins<27; ++fieldOrigins)
    for (unsigned fieldDigestErrors=0; fieldDigestErrors<8; ++fieldDigestErrors) {
        Source x{};x.projectionIndex=0;x.cardId=67;x.species=4;
        x.grade=static_cast<ArchiveGrade>(grade);
        x.digestIndependentlyChecked=digest!=0;
        x.releaseIndependentlyCorroborated=release!=0;
        if(havePayload)x.originalPayloadSha256[0]=std::byte{0x91};
        unsigned n=fieldOrigins;
        bool originalFields=true, consistentFields=true;
        for(unsigned f=0;f<3;++f) {
            x.fields[f].origin=static_cast<FieldOrigin>(n%3);n/=3;
            x.fields[f].payloadSha256=x.originalPayloadSha256;
            if(fieldDigestErrors&(1u<<f))
                x.fields[f].payloadSha256[1]=std::byte{0x43};
            if(x.fields[f].origin!=FieldOrigin::OriginalPayload)
                originalFields=false;
            else if(fieldDigestErrors&(1u<<f))
                consistentFields=false;
        }
        const auto out=evaluate(x,247,67,4);
        const bool expected=(grade==1||grade==2) && digest && release &&
            havePayload && originalFields && consistentFields;
        const bool actual=out.status==Status::ConditionalOriginalWitness;
        if(expected!=actual)++failed;
        if(actual)++positive;
        ++count;
    }
    assert(count==10368u && positive==2u && failed==0u);
    std::cout<<"L4 V07 exhaustive archive attestation: "<<count
             <<" states; "<<positive<<" strictly conditional; zero false promotions: PASS\n";
}
