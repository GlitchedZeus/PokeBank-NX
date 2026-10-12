// L5: exercise the ACCEPTED production PK3/PK4 crypto implementation on
// synthetic native-format fixtures. Never opens saves and never writes originals.
//
// Build with src/Encryption/Encryption3FRLG.cpp and Encryption4.cpp.
// Stdin rows: id gen valid pid shuffle ev_hp egg checksum encrypted_hex
// Test success: 348 PASS (336 round-trips, 12 checksum negatives).
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <span>
#include <string>
#include <vector>

namespace {
uint8_t number(std::byte value) { return static_cast<uint8_t>(value); }
uint16_t rd16(std::span<const std::byte> b, size_t o) {
    return static_cast<uint16_t>(number(b[o]) |
        (static_cast<uint16_t>(number(b[o + 1])) << 8));
}
uint32_t rd32(std::span<const std::byte> b, size_t o) {
    return static_cast<uint32_t>(rd16(b,o)) |
        (static_cast<uint32_t>(rd16(b,o + 2)) << 16);
}
int nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
bool decodeHex(const std::string& hex, std::vector<std::byte>& out) {
    if (hex.size() % 2 != 0) return false;
    for (size_t i=0; i<hex.size(); i+=2) {
        const int hi=nibble(hex[i]), lo=nibble(hex[i+1]);
        if (hi<0 || lo<0) return false;
        out.push_back(static_cast<std::byte>((hi<<4)|lo));
    }
    return true;
}
bool process(const std::string& line, bool& wasValid, std::string& error) {
    std::istringstream input(line);
    std::string id, hex, excess;
    int gen=0,valid=0,shuffle=0,ev=0,egg=0,checksum=0;
    uint64_t pidValue=0;
    if (!(input>>id>>gen>>valid>>pidValue>>shuffle>>ev>>egg>>checksum>>hex)
         || (input>>excess)) {
        error="invalid tabular input"; return false;
    }
    if (valid != 0 && valid != 1) { error="invalid expected status";return false; }
    if (gen != 3 && gen != 4) { error="unknown format";return false; }
    if (pidValue>UINT32_MAX || ev<0 || ev>255 || egg<0 || egg>1 ||
        shuffle<0 || shuffle>= (gen==3?24:32) ||
        checksum<0 || checksum>65535) {
        error="out-of-range expected metadata"; return false;
    }
    std::vector<std::byte> raw;
    if (!decodeHex(hex,raw)) { error="malformed hex"; return false; }
    const size_t expectedLength=gen==3?80u:136u;
    if (raw.size()!=expectedLength) {error="bad native-record length";return false;}
    const std::vector<std::byte> saved=raw; // immutable input safety check
    std::vector<std::byte> plain;
    if (gen==3) {
        std::unique_ptr<std::byte[]> decrypted(
            Encryption::decryptArray3FRLG(std::span<const std::byte>(raw)));
        if (!decrypted) {error="PK3 decryption returned null";return false;}
        plain.assign(decrypted.get(),decrypted.get()+raw.size());
    } else {
        if (!Encryption::validRecordSize4(raw.size())) {
            error="PK4 record-length rejected";return false;
        }
        plain=Encryption::decryptArray4(std::span<const std::byte>(raw));
    }
    if (saved!=raw || plain.size()!=raw.size()) {
        error="source mutated or length drift";return false;
    }
    const std::span<const std::byte> d(plain);
    const uint16_t stored=rd16(d,gen==3?0x1cu:0x06u);
    const uint16_t calculated=gen==3
      ? Encryption::checksum3FRLG(d)
      : Encryption::checksum4(d);
    const bool checksumGood=stored==calculated;
    const bool sanityGood=gen==3 || rd16(d,4u)==0;
    if (valid==0) {
        if (checksumGood && sanityGood) {
            error="deliberately damaged encrypted fixture accepted";return false;
        }
        wasValid=false;
        return true;
    }
    if (!checksumGood || !sanityGood) {
        error="expected valid checksum/sanity rejected";return false;
    }
    if (stored!=static_cast<uint16_t>(checksum) ||
        rd32(d,0)!=static_cast<uint32_t>(pidValue)) {
        error="native header mismatch";return false;
    }
    const int index=gen==3
      ? static_cast<int>(rd32(d,0)%24u)
      : static_cast<int>((rd32(d,0)>>13)&31u);
    const int storedEv=number(d[gen==3?0x38u:0x18u]);
    const bool storedEgg=(rd32(d,gen==3?0x48u:0x38u)&0x40000000u)!=0;
    if (index!=shuffle || storedEv!=ev ||
        storedEgg!=(egg!=0) || rd16(d,gen==3?0x20u:0x08u)!=1u) {
        error="native species/shuffle/EV/egg mismatch";return false;
    }
    // This is a real accepted production codec round trip, not a mirror model.
    if (gen==3) {
        std::unique_ptr<std::byte[]> again(
            Encryption::encryptArray3FRLG(d));
        if (!again ||
            !std::equal(raw.begin(),raw.end(),again.get())) {
            error="PK3 byte-for-byte native roundtrip failed";return false;
        }
    } else if (Encryption::encryptArray4(d)!=raw) {
        error="PK4 byte-for-byte native roundtrip failed";return false;
    }
    wasValid=true;
    return true;
}
}
int main() {
    std::string line;
    size_t total=0,valid=0,corrupted=0;
    while (std::getline(std::cin,line)) {
        if (line.empty()) continue;
        std::string error;bool isValid=false;
        if (!process(line,isValid,error)) {
            std::cerr<<"L5 native crypto failure on case "<<total+1<<": "
                     <<error<<"\n";
            return 1;
        }
        ++total;
        if (isValid) ++valid; else ++corrupted;
    }
    if (total!=348 || valid!=336 || corrupted!=12) {
        std::cerr<<"L5 native crypto unexpected corpus counts: "<<total
                 <<" / "<<valid<<" / "<<corrupted<<"\n";
        return 1;
    }
    std::cout<<"L5 production crypto: "<<total
             <<" PASS ("<<valid<<" native byte roundtrips, "
             <<corrupted<<" corrupted checksum rejects)\n";
}