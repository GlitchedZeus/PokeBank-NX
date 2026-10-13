#include "Legality/Gen1OriginalIndividualEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace I=Legality::Gen1OriginalIndividualEvidence;
constexpr I::Digest digest(uint8_t x) {I::Digest d{};d[0]=std::byte{x};return d;}
I::Link complete(){I::Link l{};
 l.preciseSourceRecordIndexIndependentlyVerified=true;l.originalRecordIndex=7;
 l.distributionArchiveSha256IndependentlyVerified=true;l.distributionArchiveSha256=digest(1);
 l.originalPK1RecordSha256IndependentlyVerified=true;l.originalPK1RecordSha256=digest(2);
 l.nativePK1RecordSha256IndependentlyVerified=true;l.nativePK1RecordSha256=digest(3);
 l.originalRecordBytesIndependentlyVerified=true;l.nativeRecordBytesIndependentlyVerified=true;
 l.originalToCurrentSameIndividualIndependentlyVerified=true;
 l.historicalOperationsExhaustivelyAttested=true;
 l.originalGameFontNameBytesIndependentlyIncluded=true;
 l.nativeGameFontNameBytesIndependentlyIncluded=true;return l;}
int main(){using B=I::Bound;using V=I::Verdict;using G=I::Group;
 auto full=complete();assert(I::evaluate(full)==B::ConditionalIndividual);
 std::uint32_t cases=0;
 for(unsigned mask=0;mask<4096;++mask){auto l=complete();
 l.preciseSourceRecordIndexIndependentlyVerified=bool(mask&1);
 l.distributionArchiveSha256IndependentlyVerified=bool(mask&2);
 l.originalPK1RecordSha256IndependentlyVerified=bool(mask&4);
 l.nativePK1RecordSha256IndependentlyVerified=bool(mask&8);
 l.originalRecordBytesIndependentlyVerified=bool(mask&16);
 l.nativeRecordBytesIndependentlyVerified=bool(mask&32);
 l.originalToCurrentSameIndividualIndependentlyVerified=bool(mask&64);
 l.historicalOperationsExhaustivelyAttested=bool(mask&128);
 l.originalGameFontNameBytesIndependentlyIncluded=bool(mask&256);
 l.nativeGameFontNameBytesIndependentlyIncluded=bool(mask&512);
 if(!(mask&1024))l.distributionArchiveSha256={};
 if(!(mask&2048))l.originalPK1RecordSha256={};
 const auto result=I::evaluate(l);
 assert((result==B::ConditionalIndividual)==(mask==4095));
 ++cases;
 }
 for(unsigned mask=0;mask<8;++mask){auto l=complete();
 if(mask&1)l.distributionArchiveSha256={};
 if(mask&2)l.originalPK1RecordSha256={};
 if(mask&4)l.nativePK1RecordSha256={};
 assert((I::evaluate(l)==B::ConditionalIndividual)==(mask==0));
 }
 // Even identical original and current PK1 bytes cannot prove that the
 // distribution/archive was authentic or that a historical receipt exists.
 {auto l=complete();assert(I::archiveIdentityMatches(l,l.distributionArchiveSha256));
  auto other=l.distributionArchiveSha256;other[31]=std::byte{0x9a};
  assert(!I::archiveIdentityMatches(l,other));
  l.distributionArchiveSha256IndependentlyVerified=false;
  assert(!I::archiveIdentityMatches(l,other));}
 auto identical=complete();identical.nativePK1RecordSha256=identical.originalPK1RecordSha256;
 identical.originalToCurrentSameIndividualIndependentlyVerified=false;
 assert(I::evaluate(identical)==B::UnknownHistoricalLineage);
 identical.originalToCurrentSameIndividualIndependentlyVerified=true;
 assert(I::evaluate(identical)==B::ConditionalIndividual);
 // All possible two- and three-candidate combinations, including malformed
 // enum values: an all-excluded set cannot globally invalidate anything.
 unsigned combinations=0;
 for(unsigned i=0;i<256;++i)for(unsigned j=0;j<256;++j){
 std::array<V,2> c{static_cast<V>(i),static_cast<V>(j)};
 auto out=I::summarize(c);assert(out.examined==2);
 assert(!out.globallyInvalid && !out.globallyLegal && !out.sourceAuthenticated);
 const bool hasCandidate=(i==2||j==2),allExcluded=i==1&&j==1;
 assert(out.group==(hasCandidate?G::HasConditionalCandidate:
     allExcluded?G::AllProvidedExcluded:G::UnknownAlternatives));
 ++combinations;
 }
 for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)for(unsigned c=0;c<3;++c){
 std::array<V,3> v{static_cast<V>(a),static_cast<V>(b),static_cast<V>(c)};
 const auto out=I::summarize(v);assert(out.examined==3);
 assert(out.unknown+out.conditional+out.excluded==3);
 }
 assert(I::summarize({}).group==G::NoCandidates);
 std::array<V,129> overflow{};assert(I::summarize(overflow).examined==0);
 std::cout<<"L4 Gen I individual digest/recipient identity: "<<cases
 <<" independent proof masks, "<<combinations<<" candidate verdict pairs PASS\n";
}
