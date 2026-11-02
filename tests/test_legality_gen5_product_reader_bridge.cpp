// Exact Product PK5 crypto/reader shared with read-only legality.
#include "Legality/Legality.h"
#include "Legality/Gen5ProductNativeEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
#ifdef NDEBUG
#error "Gen V bridge regression requires active assertions"
#endif
namespace P=PokeVault::Integration::Gen5;
namespace N=Legality::Gen5ProductNativeEvidence;
static std::vector<uint8_t> fixture(size_t size, uint32_t pid,
    uint16_t species=649, uint8_t nature=24, uint16_t ev=510) {
    std::vector<uint8_t> plain(size,0);
    plain[0]=uint8_t(pid);plain[1]=uint8_t(pid>>8);
    plain[2]=uint8_t(pid>>16);plain[3]=uint8_t(pid>>24);
    P::Crypto::write16(plain,8,species);
    plain[0x41]=nature;
    plain[0x18]=uint8_t(ev>255?255:ev);
    const auto remaining=uint16_t(ev-plain[0x18]);
    plain[0x19]=uint8_t(remaining>255?255:remaining);
    plain[0x1A]=uint8_t(remaining-plain[0x19]);
    plain[0x17]=2;
    P::Crypto::write16(plain,0x28,33);
    if(size==P::Crypto::PartySize)plain[0x8C]=50;
    return P::Crypto::encryptCandidate(plain);
}
int main() {
    size_t checked=0, corrupted=0;
    for(size_t size : {P::Crypto::StoredSize,P::Crypto::PartySize}) {
        for(uint32_t order=0;order<24;++order) {
            const uint32_t pid=(0x05612345u & ~(31u<<13)) | (order<<13);
            auto bytes=fixture(size,pid);
            const auto source=bytes;
            auto evidence=N::inspect(bytes);
            assert(evidence.status==N::Status::Parsed);
            assert(evidence.species==649 && evidence.nature==24);
            assert(evidence.evTotal==510 && evidence.checksumVerified);
            assert(!evidence.sourceHistoryAuthenticated);
            assert(evidence.partyRecord==(size==P::Crypto::PartySize));
            auto report=Legality::analyzePk5Record(bytes);
            assert(report.verdict()==Legality::Verdict::Incomplete);
            assert(report.coverage.structure==Legality::CoverageLevel::Complete);
            assert(!report.hasInvalid());
            assert(bytes==source);
            for(size_t byte=8;byte<P::Crypto::StoredSize;++byte)
                for(unsigned bit=0;bit<8;++bit) {
                    auto tampered=bytes;
                    tampered[byte]^=uint8_t(1u<<bit);
                    auto bad=N::inspect(tampered);
                    assert(bad.status==N::Status::InvalidChecksum);
                    assert(!bad.checksumVerified);
                    assert(tampered[byte]!=bytes[byte]);
                    ++corrupted;
                }
            auto badSanity=bytes;badSanity[4]=1;
            assert(N::inspect(badSanity).status==N::Status::InvalidSanity);
            assert(Legality::analyzePk5Record(badSanity).hasInvalid());
            auto badSpecies=fixture(size,pid,650);
            assert(N::inspect(badSpecies).status==N::Status::InvalidSpecies);
            auto badNature=fixture(size,pid,649,25);
            assert(N::inspect(badNature).status==N::Status::InvalidNature);
            auto badEv=fixture(size,pid,649,24,511);
            assert(N::inspect(badEv).status==N::Status::InvalidEVTotal);
            ++checked;
        }
        std::vector<uint8_t> zeros(size,0);
        assert(N::inspect(zeros).status==N::Status::EmptySlot);
        assert(Legality::analyzePk5Record(zeros).verdict()==Legality::Verdict::Incomplete);
        if(size==P::Crypto::PartySize) {
            auto bytes=fixture(size,0x11224455);
            bytes.back()^=0x80;
            assert(N::inspect(bytes).status==N::Status::Parsed);
        }
    }
    for(size_t len=0;len<300;++len) {
        if(P::Crypto::recordSizeValid(len))continue;
        std::vector<uint8_t> bytes(len,0xFF);
        assert(N::inspect(bytes).status==N::Status::UnknownLength);
        assert(!Legality::analyzePk5Record(bytes).hasInvalid());
    }
    assert(checked==48 && corrupted==48u*128u*8u);
    std::cout<<"Gen V reused product PK5: "<<checked<<" records, "
             <<corrupted<<" encrypted-body bit flips rejected; "
             <<"length/sanity/checksum/species/nature/EV/immutability PASS\n";
}
