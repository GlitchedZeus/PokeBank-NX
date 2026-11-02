// Feature A (owner Oct 28; owner's PKSE PR #128 + fa06a1b / aa4b72e / 18c2555 / 620c5f6): the standalone
// .pk1-.pk5 / .ek3-.ek5 decoder. Synthetic records only.
#include "PokemonFile/NativePokemonFile.h"
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Encryption/Encryption5.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace F = PokeBank::PokemonFile;
using Bytes = std::vector<uint8_t>;

namespace {
void w16(Bytes& b, size_t o, uint16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(v >> 8); }
void w32(Bytes& b, size_t o, uint32_t v) { w16(b, o, uint16_t(v)); w16(b, o + 2, uint16_t(v >> 16)); }
Bytes toBytes(const std::byte* p, size_t n) { return Bytes(reinterpret_cast<const uint8_t*>(p), reinterpret_cast<const uint8_t*>(p) + n); }

Bytes pk3(size_t size, uint16_t species) {
    Bytes b(size, 0);
    w32(b, 0, 0x89ABCDEFu); w32(b, 4, 0x12345678u);
    b[0x12] = 2;                                   // language
    w16(b, 0x20, species); w32(b, 0x24, 1000);     // growth: species, experience
    w16(b, 0x1C, Encryption::checksum3FRLG(std::as_bytes(std::span<const uint8_t>(b).first(80))));
    return b;
}
Bytes ek3(const Bytes& plain) {
    std::byte* e = Encryption::encryptArray3FRLG(std::as_bytes(std::span<const uint8_t>(plain)));
    Bytes out = toBytes(e, plain.size()); delete[] e; return out;
}
uint16_t sum45(const Bytes& b) { uint32_t s = 0; for (size_t o = 8; o < 136; o += 2) s += uint16_t(b[o] | (b[o + 1] << 8)); return uint16_t(s); }
Bytes pk45(int gen, size_t size, uint16_t species) {
    Bytes b(size, 0);
    w32(b, 0, 0x2468ACE0u);
    w16(b, 0x08, species);
    w32(b, 0x10, 5000);
    b[0x5F] = gen == 4 ? 10 : 20;                  // origin: Diamond / White
    if (gen == 5) w16(b, 0x80, 0x5555);            // a Gen V-only field shape (PKHeX GetFormat45)
    w16(b, 0x06, sum45(b));
    return b;
}
Bytes ek45(int gen, const Bytes& plain) {
    if (gen == 5) return PokeVault::Integration::Gen5::Crypto::encryptCandidate(plain);
    const auto e = Encryption::encryptArray4(std::as_bytes(std::span<const uint8_t>(plain)));
    Bytes out(e.size()); for (size_t i = 0; i < e.size(); ++i) out[i] = uint8_t(e[i]); return out;
}
Bytes list(int gen, bool japanese, uint8_t species, uint8_t listSpecies = 0) {
    const size_t body = gen == 1 ? 44 : 48, str = japanese ? 6 : 11;
    Bytes b(3 + body + 2 * str, 0x50);
    b[0] = 1; b[1] = listSpecies ? listSpecies : species; b[2] = 0xFF;
    std::fill(b.begin() + 3, b.begin() + 3 + static_cast<long>(body), 0);
    b[3] = species;
    b[3 + body] = 0x80;                            // OT "A", then 0x50 terminators
    b[3 + body + str] = 0x81;                      // nickname "B"
    return b;
}
bool ok(const Bytes& b, const char* name, std::string* why = nullptr) {
    std::string e; const auto d = F::decode(b, name, e); if (why) *why = e; return d.has_value();
}
}  // namespace

int main() {
    std::string e;
    // Extensions.
    for (const char* n : {"a.pk1", "a.PK2", "x/y.pk3", "a.ek3", "a.pk4", "a.ek4", "a.pk5", "a.Ek5"}) assert(F::supportedFileName(n));
    for (const char* n : {"a.pk6", "a.ek1", "a.ek2", "a.pk", "a", "dir.pk3/file", "a.sav"}) assert(!F::supportedFileName(n));
    assert(F::plainExtension(4) == ".pk4" && F::plainExtension(6).empty());

    // Gen III: plain and encrypted, both stored and party; content decides, not the name.
    for (const size_t size : {size_t(80), size_t(100)}) {
        const Bytes plain = pk3(size, 25), enc = ek3(plain);
        assert(enc != plain);
        for (const char* name : {"p.pk3", "p.ek3"}) {
            auto d = F::decode(plain, name, e); assert(d && !d->wasEncrypted && d->plain == plain && d->party == (size == 100));
            d = F::decode(enc, name, e);        assert(d && d->wasEncrypted && d->plain == plain);
        }
        Bytes bad = enc; bad[0x30] ^= 0x5A;
        assert(!ok(bad, "p.ek3", &e) && e.find("Checksum") != std::string::npos);
        Bytes egg = pk3(size, 25); egg[0x13] |= 1; w16(egg, 0x1C, Encryption::checksum3FRLG(std::as_bytes(std::span<const uint8_t>(egg).first(80))));
        assert(!ok(egg, "p.pk3", &e) && e.find("Bad Egg") != std::string::npos);
    }
    assert(!ok(pk3(80, 0), "p.pk3"));                  // empty record
    assert(!ok(pk3(80, 500), "p.pk3"));                // out-of-range species
    assert(!ok(Bytes(81, 0), "p.pk3", &e) && e.find("size") != std::string::npos);

    // Gen IV / V.
    for (const int gen : {4, 5}) {
        const char* plainName = gen == 4 ? "m.pk4" : "m.pk5";
        const char* encName = gen == 4 ? "m.ek4" : "m.ek5";
        const char* otherName = gen == 4 ? "m.pk5" : "m.pk4";
        for (const size_t size : {size_t(136), size_t(gen == 4 ? 236 : 220)}) {
            const Bytes plain = pk45(gen, size, 133), enc = ek45(gen, plain);
            auto d = F::decode(plain, plainName, e); assert(d && !d->wasEncrypted && d->plain == plain && d->generation == gen);
            d = F::decode(enc, encName, e);           assert(d && d->wasEncrypted && d->plain == plain);
            d = F::decode(enc, plainName, e);         assert(d && d->wasEncrypted);   // mislabelled: still detected
            Bytes bad = enc; bad[0x40] ^= 0x11;
            assert(!ok(bad, encName));
        }
        // A stored record whose content belongs to the other generation is refused (PKHeX GetFormat45).
        assert(!ok(pk45(gen, 136, 133), otherName, &e) && e.find("content") != std::string::npos);
        // Battle Revolution sanity, empty and out-of-range species.
        Bytes br = pk45(gen, 136, 133); w16(br, 0x04, 1); w16(br, 0x06, sum45(br));
        assert(!ok(br, plainName, &e) && e.find("Battle Revolution") != std::string::npos);
        assert(!ok(pk45(gen, 136, 0), plainName));
        assert(!ok(pk45(gen, 136, gen == 4 ? 494 : 650), plainName));
    }
    assert(!ok(pk45(4, 236, 133), "m.pk5", &e));     // 236 bytes is Gen IV only
    assert(!ok(pk45(5, 220, 133), "m.pk4", &e));     // 220 bytes is Gen V only

    // Gen I / II lists, both regions; aa4b72e: a record whose party fields are zero is accepted.
    for (const int gen : {1, 2})
        for (const bool jp : {false, true}) {
            const char* name = gen == 1 ? "r.pk1" : "r.pk2";
            const Bytes b = list(gen, jp, 0x99);
            auto d = F::decode(b, name, e);
            assert(d && d->generation == gen && d->japanese == jp && d->plain == b);
            Bytes noTerm = b; noTerm[2] = 0x00;          assert(!ok(noTerm, name));
            Bytes two = b; two[0] = 2;                   assert(!ok(two, name));
            assert(!ok(list(gen, jp, 0x99, 0x98), name, &e) && e.find("disagree") != std::string::npos);
            Bytes noName = b; std::fill(noName.end() - 11 + (jp ? 5 : 0), noName.end(), 0x80);
            assert(!ok(noName, name, &e) && e.find("terminator") != std::string::npos);
            assert(!ok(list(gen, jp, 0), name));
        }
    assert(ok(list(2, false, 0x99, 0xFD), "egg.pk2"));  // Gen II Egg: list 0xFD, body keeps the species
    assert(!ok(list(1, false, 0x99, 0xFD), "egg.pk1"));
    assert(!ok(list(2, false, 252), "r.pk2"));
    assert(!ok(Bytes(70, 0), "r.pk1", &e) && e.find("size") != std::string::npos);
    assert(!ok(list(1, false, 0x99), "r.pk2"));          // a .pk1 body named .pk2: wrong size
    std::cout << "Standalone Pokemon file decoder (.pk1-.pk5, .ek3-.ek5): PASS\n";
    return 0;
}
