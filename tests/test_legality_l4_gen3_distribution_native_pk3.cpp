#include "Legality/Gen3DistributionNativePK3Evidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
namespace N=Legality::Gen3DistributionNativePK3Evidence;
namespace W=Legality::Gen3DistributionWitnessEvidence;
using H=W::History;using F=W::Field;using S=W::Status;
void w16(std::vector<std::byte>& data,size_t o,uint16_t value) {
    data[o]=std::byte(value&0xffu);data[o+1]=std::byte(value>>8);
}
void w32(std::vector<std::byte>& data,size_t o,uint32_t value) {
    w16(data,o,static_cast<uint16_t>(value));w16(data,o+2,static_cast<uint16_t>(value>>16));
}
std::vector<std::byte> makeCanonical(size_t size,uint32_t pid) {
    std::vector<std::byte> d(size);
    w32(d,0x00,pid);w16(d,0x04,0x4321);w16(d,0x06,0x1234);
    d[0x12]=std::byte{2};d[0x13]=std::byte{2};
    d[0x08]=std::byte{0xbb};d[0x09]=std::byte{0xff};d[0x0a]=std::byte{0x8a};
    d[0x14]=std::byte{0xc1};d[0x15]=std::byte{0xff};d[0x16]=std::byte{0x72};
    w16(d,0x20,385);w16(d,0x22,175);
    w16(d,0x2c,10);w16(d,0x2e,20);w16(d,0x30,30);w16(d,0x32,40);
    d[0x45]=std::byte{42};
    // Level 5, Ruby origin 1, ball 4, OT gender 1.
    w16(d,0x46,static_cast<uint16_t>(5u|(1u<<7)|(4u<<11)|(1u<<15)));
    w32(d,0x48,0x00118421u);w32(d,0x4c,0x81000001u);
    w16(d,0x1c,Encryption::checksum3FRLG(d));
    return d;
}
std::vector<std::byte> encrypt(const std::vector<std::byte>& data) {
    std::unique_ptr<std::byte[]> encoded(Encryption::encryptArray3FRLG(data));
    return {encoded.get(),encoded.get()+data.size()};
}
template<class T> void take(W::Witness<T>& w,const T& observed) {
    w.independentlyKnown=true;w.original=observed;
    w.history=H::IndependentlyVerifiedUnchanged;
}
W::OriginalCandidate profile(const W::Native& n) {
    W::OriginalCandidate c{};
    c.family=W::Family::Wondercard;c.mechanism=W::Mechanism::PokemonGift;
    c.originalSource=W::OriginalSource::Ruby;
    c.exactRouteIndependentlyVerified=true;
    c.originalArchiveIndependentlyAuthenticated=true;
    c.originalPayloadDigestVerified=true;c.originalPayloadSha256[0]=std::byte{0x7a};
    c.originalTemplateIndependentlyAttested=true;
    c.historicalReleaseIndependentlyCorroborated=true;
    c.individualHistoricalReceiptKnown=false; // synthetic! never receipt proof
    take(c.speciesInternal,n.speciesInternal);take(c.pid,n.pid);
    take(c.ids,n.ids);take(c.trainerGender,n.trainerGender);
    take(c.language,n.language);take(c.nickname,n.nickname);take(c.otName,n.otName);
    take(c.originGame,n.originGame);take(c.metLevel,n.metLevel);
    take(c.metLocation,n.metLocation);take(c.ball,n.ball);
    take(c.moves,n.moves);take(c.heldItem,n.heldItem);
    take(c.egg,n.egg);take(c.fateful,n.fateful);
    take(c.ribbonBits,n.ribbonBits);
    return c;
}
int main() {
    uint64_t sets=0,fieldNegatives=0,bitNegatives=0;
    for(const auto size:{Encryption::SIZE_STORED3_FRLG,Encryption::SIZE_PARTY3_FRLG})
    for(uint32_t sv=0;sv<24;++sv) {
        auto decoded=makeCanonical(size,sv+24u);
        const auto raw=encrypt(decoded);
        assert(raw.size()==size && raw!=decoded);
        const auto originalBytes=raw;
        const auto n=N::inspectEncrypted(raw);
        assert(n.available && n.pid==sv+24 && n.speciesInternal==385);
        assert(n.ids.tid==0x4321 && n.ids.sid==0x1234);
        assert(n.language==2 && n.nickname[2]==std::byte{0x8a});
        assert(n.otName[2]==std::byte{0x72} && n.heldItem==175);
        assert(n.originGame==1 && n.metLevel==5 && n.metLocation==42);
        assert(n.ball==4 && n.trainerGender==1);
        assert((n.moves==W::Moves{10,20,30,40}));
        assert(!n.egg && n.fateful && n.ribbonBits==0x81000001);
        const auto c=profile(n);
        const auto r=N::evaluateEncrypted(raw,c);
        assert(r.source==W::SourceStatus::MissingReceipt);
        assert(r.conditionalMatchMask==0xffffu && r.unknownMask==0);
        assert(!r.authenticated && !r.globallyInvalid && r.mismatchMask==0);
        assert(raw==originalBytes); // decryptor cannot modify staged source
        // Native projection cannot itself prove a distribution.
        assert(N::evaluateEncrypted(raw).mismatchMask==0);
        // A source without independent payload attestation cannot compare.
        {auto a=c;a.originalArchiveIndependentlyAuthenticated=false;
         auto x=N::evaluateEncrypted(raw,a);
         assert(x.conditionalMatchMask==0 && x.mismatchMask==0);}
        // Original language / game-font bytes: source claims are per-field.
        {auto a=c;a.nickname.original[2]^=std::byte{1};
         assert(N::evaluateEncrypted(raw,a).mismatchMask==(1u<<5));
         a.nicknameOrOTReencodingPossible=true;
         assert(N::evaluateEncrypted(raw,a).field(F::Nickname)==S::UnknownHistory);}
        {auto a=c;a.language.original=7;
         assert(N::evaluateEncrypted(raw,a).mismatchMask==(1u<<4));
         a.languageConversionPossible=true;
         assert(N::evaluateEncrypted(raw,a).mismatchMask==0);}
        // Original-source witness differs for each of the 16 immutable or
        // mutable fields. No synthetic row turns into a global verdict.
        for(unsigned i=0;i<W::FIELD_COUNT;++i){
            auto a=c;
            switch(static_cast<F>(i)) {
            case F::Species: a.speciesInternal.original^=1;break;
            case F::PID: a.pid.original^=1;break;
            case F::TrainerIDs: a.ids.original.tid^=1;break;
            case F::TrainerGender: a.trainerGender.original^=1;break;
            case F::Language: a.language.original^=1;break;
            case F::Nickname: a.nickname.original[0]^=std::byte{1};break;
            case F::OTName: a.otName.original[0]^=std::byte{1};break;
            case F::OriginGame: a.originGame.original^=1;break;
            case F::MetLevel: a.metLevel.original^=1;break;
            case F::MetLocation: a.metLocation.original^=1;break;
            case F::Ball: a.ball.original^=1;break;
            case F::Moves: a.moves.original[2]^=1;break;
            case F::HeldItem: a.heldItem.original^=1;break;
            case F::Egg: a.egg.original=!a.egg.original;break;
            case F::Fateful: a.fateful.original=!a.fateful.original;break;
            case F::RibbonBits: a.ribbonBits.original^=1;break;
            case F::Count: assert(false);break;
            }
            auto x=N::evaluateEncrypted(raw,a);
            assert(x.mismatchMask==(1u<<i));
            assert(!x.globallyInvalid && !x.authenticated && raw==originalBytes);
            ++fieldNegatives;
        }
        // Synthetic native checksum-consistent species zero is empty,
        // including nonempty data elsewhere.
        {auto d=decoded;w16(d,0x20,0);w16(d,0x1c,Encryption::checksum3FRLG(d));
         assert(!N::inspectEncrypted(encrypt(d)).available);}
        {std::vector<std::byte> blank(size);
         assert(!N::inspectEncrypted(blank).available);}
        {auto bad=raw;bad[0x13]|=std::byte{1};
         assert(!N::inspectEncrypted(bad).available);}
        {auto corrupt=raw;corrupt[0x20]^=std::byte{1};
         assert(!N::inspectEncrypted(corrupt).available);}
        {auto corrupt=raw;corrupt[0x1c]^=std::byte{1};
         assert(!N::inspectEncrypted(corrupt).available);}
        // Every flipped bit of the XOR-encrypted block changes the 16-bit
        // word-sum checksum in these single-bit adversarial mutations.
        for(size_t byte=0x20;byte<0x50;++byte)
        for(unsigned bit=0;bit<8;++bit) {
            auto corrupt=raw;
            corrupt[byte]^=std::byte{static_cast<unsigned char>(1u<<bit)};
            assert(!N::inspectEncrypted(corrupt).available);
            assert(!N::evaluateEncrypted(corrupt,c).hasCandidateMismatch());
            ++bitNegatives;
        }
        ++sets;
    }
    assert(sets==48 && fieldNegatives==48u*16u && bitNegatives==48u*48u*8u);
    for(const size_t size:{0u,1u,79u,81u,99u,101u,136u}) {
        std::vector<std::byte> bad(size);
        assert(!N::inspectEncrypted(bad).available);
    }
    std::cout<<"L4 Gen III native PK3: "<<sets<<" encrypted layout+size fixtures, "
             <<fieldNegatives<<" native field negatives, "<<bitNegatives
             <<" one-bit ciphertext rejection cases: PASS\n";
}
