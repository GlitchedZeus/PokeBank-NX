// Stage 1 L1: malformed source game/scene bytes must never certify a
// GameCube origin. Test the actual read-only original-stage helper, not a
// reconstructed model. Synthetic candidates are not retail proof.
#include "Legality/Gen3GameCubeOriginalNativeStage.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen3GameCubeOriginalNativeStage;

static G::Candidate evidence() {
    G::Candidate c{};
    c.originalGame=G::Game::Colosseum;
    c.originalRoute=G::Route::ShadowPurified;
    c.originalPersonality=0x51234abc;
    c.originalTrainerId=0x12345678;
    c.originalSpecies=196;
    c.native.personality=c.originalPersonality;
    c.native.originalTrainerId=c.originalTrainerId;
    c.native.species=c.originalSpecies;
    c.native.originGame=15;
    c.native.checksumAndDecryptionVerified=true;
    c.originalGameIndependentlyAttested=true;
    c.originalSceneIndependentlyAttested=true;
    c.originalCapturedPidSpeciesOtIndependentlyAttested=true;
    c.originalPurificationIndependentlyAttested=true;
    c.originalRecipientSaveIndependentlyAttested=true;
    return c;
}

int main() {
    unsigned checks=0,knownRoutes=0;
    static_assert(G::recognizedGame(G::Game::Colosseum));
    static_assert(G::recognizedGame(G::Game::XD));
    static_assert(!G::recognizedGame(G::Game::Unknown));
    static_assert(!G::recognizedGame(static_cast<G::Game>(255)));
    static_assert(!G::recognizedRoute(static_cast<G::Route>(255)));
    // 65,536 game/scene byte combinations: no fall-through positives for
    // malformed enum values that can be read from corrupted source metadata.
    for(unsigned gameByte=0;gameByte<256;++gameByte) {
        for(unsigned routeByte=0;routeByte<256;++routeByte) {
            auto c=evidence();
            c.originalGame=static_cast<G::Game>(gameByte);
            c.originalRoute=static_cast<G::Route>(routeByte);
            c.native.originGame=routeByte==3 ? 3 : 15;
            const G::Status expected=
                (gameByte!=1 && gameByte!=2) ? G::Status::UnknownOriginalGame :
                (routeByte<1 || routeByte>3) ? G::Status::UnknownOriginalScene :
                                              G::Status::ConditionalGameCubeOriginHistory;
            const auto result=G::evaluate(c);
            assert(result.status==expected);
            assert(result.recipientOriginIsGba==
                (expected==G::Status::ConditionalGameCubeOriginHistory && routeByte==3));
            ++checks;
            if(expected!=G::Status::ConditionalGameCubeOriginHistory)continue;
            ++knownRoutes;
            auto bad=c;
            bad.native.checksumAndDecryptionVerified=false;
            assert(G::evaluate(bad).status==G::Status::UnknownOriginalNative);++checks;
            bad=c;bad.originalCapturedPidSpeciesOtIndependentlyAttested=false;
            assert(G::evaluate(bad).status==G::Status::UnknownOriginalNative);++checks;
            bad=c;bad.originalSceneIndependentlyAttested=false;
            assert(G::evaluate(bad).status==G::Status::UnknownOriginalScene);++checks;
            bad=c;bad.native.personality^=1;
            assert(G::evaluate(bad).status==G::Status::CandidateNativeIdentityContradicted);++checks;
            bad=c;bad.native.originGame=0;
            assert(G::evaluate(bad).status==G::Status::CandidateNativeOriginContradicted);++checks;
            bad=c;bad.originalSpecies=0;
            assert(G::evaluate(bad).status==G::Status::UnknownOriginalNative);++checks;
            if(routeByte==1) {
                bad=c;bad.originalPurificationIndependentlyAttested=false;
                assert(G::evaluate(bad).status==G::Status::UnknownPurificationHistory);++checks;
            }
            if(routeByte==3) {
                bad=c;bad.originalRecipientSaveIndependentlyAttested=false;
                assert(G::evaluate(bad).status==G::Status::UnknownRecipientHistory);++checks;
            }
        }
    }
    assert(knownRoutes==6 && checks==65576);
    std::cout<<"L1 GameCube source enum domain: "<<checks
             <<" PASS (unknown values never certify original histories)\n";
}
