#include "Legality/Gen1OriginalGiftNativeEvidence.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L=Legality::Gen1OriginalGiftNativeEvidence;
namespace G=L::G;
namespace J=L::J;
namespace A=J::Archive;
using PK1=PokeVault::Integration::Gen1::PokemonRecord;
template<class T> constexpr G::Datum<T> yes(T v){return {true,v};}
constexpr G::Name name(uint8_t a) {G::Name n{};n[0]=a;n[1]=0x50;return n;}
struct Fixture {
    PK1 native{};
    L::Source source{};
    Fixture(G::Game edition=G::Game::Red,G::Kind kind=G::Kind::NPCTrade) {
        native.rawSpecies=84; // storage raw ID is not interpreted by this evidence layer
        native.species=30;native.level=20;native.trainerId=17389;
        native.catchRate=45;native.moves={1,2,3,0};native.dvs={0,15,10,7,12};
        native.ppUps={0,0,0,0};native.originalTrainer="trainer display only";
        native.nickname="nickname display only";native.rawBodySize=44;
        auto& c=source.hypothesis;
        c.original.game=edition;c.original.kind=kind;
        c.original.originalMechanicIndependentlyVerified=true;
        c.original.originalRowIndependentlyVerified=true;
        c.original.originalDistributionAttested=true;
        c.original.originalReceiptKnown=true;
        c.original.species=yes<uint16_t>(30);
        c.original.levelRule=kind==G::Kind::NPCTrade?G::LevelRule::OfferedTradeLevel:G::LevelRule::Fixed;
        c.original.level=yes<uint8_t>(20);
        // NPC trainer ID is random in pret original and known ONLY for this
        // synthetic independently documented individual receipt, not the ROM row.
        c.original.trainerId=yes<uint16_t>(native.trainerId);
        c.original.trainerName=yes(name('T'));
        c.original.nickname=yes(name('N'));
        c.original.moves=yes(G::Moves4{1,2,3,0});
        c.route.speciesHistory=G::History::Unchanged;
        c.route.trainerNameHistory=G::History::Unchanged;
        c.route.nicknameHistory=G::History::Unchanged;
        c.route.moveHistory=G::History::Unchanged;
        c.route.timeCapsule=G::Crossing::NoCrossingVerified;
        c.route.offeredTradeLevel=yes<uint8_t>(20);
        c.editionIdentityIndependentlyVerified=true;
        c.independentlyIdentifiedSourceEdition=edition;
        c.originalRawCharsetLinkIndependentlyVerified=true;
        c.archive.platform=J::platformFor(edition);
        c.archive.family=J::familyFor(kind);
        c.archive.grade=A::ArchiveGrade::OriginalAuthenticated;
        c.archive.exactSourceIdentityIndependentlyProven=true;
        c.archive.sourceMechanismIndependentlyProven=true;
        c.archive.originalPayloadDigestVerified=true;
        c.archive.originalPayloadSha256[0]=std::byte{0x73};
        c.archive.originalPayloadFieldsIndependentlyExtracted=true;
        c.archive.distributionReleaseIndependentlyCorroborated=true;
        c.archive.individualOriginalReceiptHistoryKnown=true;
        c.archiveRowLinkIndependentlyVerified=true;
        c.archiveRowOriginalDigestIndependentlyVerified=true;
        c.archiveRowOriginalDigest=c.archive.originalPayloadSha256;
        source.classification=kind==G::Kind::StadiumGift?
          L::SourceClass::OriginalStadiumMedia:L::SourceClass::OriginalCartridgeMechanic;
        source.originalCartridgeScriptIndependentlyPinned=true;
        source.exactOriginalNPCTradeRowIndependentlyPinned=true;
        source.englishRetailTradeTableIndependentlyVerified=true;
        source.originalNPCTradeRowIndex=0;
        source.originalOfferedSpeciesIndependentlyVerified=true;
        source.originalOfferedSpecies=33;
        source.observedOfferedSpeciesIndependentlyVerified=true;
        source.observedOfferedSpecies=33;
        source.sourceSaveEditionIndependentlyVerified=true;
        source.originalSaveEdition=edition;
        source.names.exactOriginalSaveBytesIndependentlyVerified=true;
        source.names.rawNamesIndependentlyExtractedFromThisRecord=true;
        source.names.originalEncodingIndependentlyReconciled=true;
        source.names.rawOT=name('T');source.names.rawNickname=name('N');
        source.catchHistory.originalAcquisitionRateIndependentlyAttested=true;
        source.catchHistory.originallyObservedRate=45;
        source.catchHistory.originalBytePreservedThroughCompleteHistory=true;
    }
    L::Result run()const {return L::evaluate(native,true,source);}
};
int main() {
    using S=L::Status;
    auto f=Fixture{};
    assert(f.run().status==S::ConditionalCandidate);
    assert(!f.run().globallyInvalid&&!f.run().authenticated);
    assert(!f.run().payloadEmbeddingPermitted);
    assert(L::evaluate(f.native,false,f.source).status==S::UnknownNative);
    {auto x=f;x.native.species=0;assert(x.run().status==S::UnknownNative);
      x.native.species=152;assert(x.run().status==S::UnknownNative);
      x.native.species=30;x.native.moves[0]=166;assert(x.run().status==S::UnknownNative);
      x.native.moves[0]=1;x.native.level=0;assert(x.run().status==S::UnknownNative);
      x.native.level=20;x.native.dvs[1]=16;assert(x.run().status==S::UnknownNative);
      x.native.dvs[1]=15;x.native.ppUps[1]=4;assert(x.run().status==S::UnknownNative);}
    {auto x=f;x.source.classification=L::SourceClass::ModernUnverifiedListing;
      x.native.trainerId++;assert(x.run().status==S::UnknownSource&&!x.run().contradicted);}
    {auto x=f;x.source.sourceSaveEditionIndependentlyVerified=false;
      x.native.trainerId++;assert(x.run().status==S::UnknownCartridgeVersion);}
    {auto x=f;x.source.originalOfferedSpeciesIndependentlyVerified=false;
      assert(x.run().status==S::UnknownOriginalNPCTrade);}
    {auto x=f;x.source.observedOfferedSpecies=34;
      assert(x.run().status==S::CandidateExcluded&&(x.run().contradicted&L::NPCTradeInput));}
    {auto x=f;x.source.names.rawNamesIndependentlyExtractedFromThisRecord=false;
      x.native.trainerId++;assert(x.run().status==S::UnknownHistoricalRoute);}
    {auto x=f;x.native.trainerId++;assert(x.run().status==S::CandidateExcluded);
      assert(x.run().contradicted&G::TrainerId);}
    {auto x=f;x.native.catchRate++;
      assert(x.run().status==S::CandidateExcluded &&(x.run().contradicted&L::CatchRate));}
    {auto x=f;x.source.catchHistory.originalAcquisitionRateIndependentlyAttested=false;
      x.native.catchRate++;assert(x.run().status==S::UnknownCatchRateHistory);
      assert(!(x.run().contradicted&L::CatchRate));}
    {auto x=f;x.source.catchHistory.originalBytePreservedThroughCompleteHistory=false;
      x.native.catchRate++;assert(x.run().status==S::UnknownCatchRateHistory);}
    {auto x=f;x.source.catchHistory.originalBytePreservedThroughCompleteHistory=false;
      x.source.catchHistory.originalTimeCapsuleRoundTripIndependentlyVerified=true;
      x.source.catchHistory.returnedHeldItemByteIndependentlyVerified=true;
      x.source.catchHistory.observedTradebackHeldItemByte=0x17;
      x.source.hypothesis.route.timeCapsule=G::Crossing::SnapshotVerified;
      x.source.hypothesis.route.crossingSpecies=yes<uint16_t>(30);
      x.source.hypothesis.route.crossingMoves=yes(G::Moves4{1,2,165,0});
      x.native.catchRate=0x17;assert(x.run().status==S::ConditionalCandidate);
      x.native.catchRate=0x18;assert(x.run().status==S::CandidateExcluded);
      x.native.catchRate=0x17;x.source.hypothesis.route.crossingMoves.value[2]=166;
      assert(x.run().status==S::CandidateExcluded);
      x.source.hypothesis.route.crossingMoves.value[2]=165;
      x.source.catchHistory.returnedHeldItemByteIndependentlyVerified=false;
      assert(x.run().status==S::UnknownCatchRateHistory);}
    {auto x=Fixture{G::Game::Yellow};
      x.native.species=51;
      x.source.hypothesis.original.species=yes<uint16_t>(51);
      x.source.originalOfferedSpecies=108;x.source.observedOfferedSpecies=108;
      assert(x.run().status==S::ConditionalCandidate);
      x.source.originalSaveEdition=G::Game::Red;
      assert(x.run().status==S::UnknownHistoricalRoute);
      x.source.hypothesis.route.sameGenerationEditionTransferIndependentlyVerified=true;
      assert(x.run().status==S::ConditionalCandidate);}
    {auto x=Fixture{G::Game::Red,G::Kind::StadiumGift};
      assert(x.run().status==S::ConditionalCandidate);
      x.source.hypothesis.archive.grade=A::ArchiveGrade::ReconstructedOrPlaceholder;
      assert(x.run().status==S::UnknownHistoricalRoute);
      x.source.hypothesis.archive.grade=A::ArchiveGrade::OriginalAuthenticated;
      x.source.hypothesis.archive.originalPayloadSha256[0]=std::byte{0x10};
      assert(x.run().status==S::UnknownHistoricalRoute);}
    uint64_t masks=0;
    for(unsigned mask=0;mask<65536;++mask){auto x=f;
      x.source.originalCartridgeScriptIndependentlyPinned=(mask&1)!=0;
      x.source.sourceSaveEditionIndependentlyVerified=(mask&2)!=0;
      x.source.exactOriginalNPCTradeRowIndependentlyPinned=(mask&4)!=0;
      x.source.originalOfferedSpeciesIndependentlyVerified=(mask&8)!=0;
      x.source.observedOfferedSpeciesIndependentlyVerified=(mask&16)!=0;
      x.source.names.exactOriginalSaveBytesIndependentlyVerified=(mask&32)!=0;
      x.source.names.rawNamesIndependentlyExtractedFromThisRecord=(mask&64)!=0;
      x.source.names.originalEncodingIndependentlyReconciled=(mask&128)!=0;
      x.source.catchHistory.originalAcquisitionRateIndependentlyAttested=(mask&256)!=0;
      x.source.catchHistory.originalBytePreservedThroughCompleteHistory=(mask&512)!=0;
      x.source.hypothesis.original.originalMechanicIndependentlyVerified=(mask&1024)!=0;
      x.source.hypothesis.original.originalRowIndependentlyVerified=(mask&2048)!=0;
      x.source.hypothesis.editionIdentityIndependentlyVerified=(mask&4096)!=0;
      x.source.hypothesis.originalRawCharsetLinkIndependentlyVerified=(mask&8192)!=0;
      x.source.hypothesis.route.offeredTradeLevel.known=(mask&16384)!=0;
      x.source.hypothesis.original.trainerId.known=(mask&32768)!=0;
      const auto r=x.run();
      assert((r.status==S::ConditionalCandidate)==(mask==65535));
      assert(!r.authenticated&&!r.globallyInvalid&&!r.payloadEmbeddingPermitted);
      ++masks;
    }
    std::cout<<"L4 Gen I R/B/Y source-gift native: "<<masks<<" proof masks, event + NPC + raw-name + Time Capsule + catch-rate: PASS\n";
}
