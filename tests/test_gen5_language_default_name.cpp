// M1 D3: Gen V stores an un-nicknamed Pokemon's species name in its own language. Changing the
// language of such a record must rewrite that name to the new language's default
// (PKHeX SpeciesName.GetSpeciesNameGeneration(species, language, 5)), written as PKHeX
// StringConverter5.SetString does with PK5.Nickname's StringConverterOption.None: UTF-16 units, a
// 0xFFFF terminator, half-width text storing ♂/♀ as U+246D/U+246E, and the units after the
// terminator left as they were. A nicknamed record keeps its name bytes; only 0x17 changes.
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace G = PokeVault::Integration::Gen5;
namespace C = G::Crypto;
using F = G::StagedPokemon5Record::Field;

std::vector<uint8_t> record(uint16_t species, const std::u16string& name, bool nicknamed) {
    std::vector<uint8_t> pk(C::StoredSize, 0);
    const uint32_t pid = 0x12340010u;
    C::write16(pk, 0, static_cast<uint16_t>(pid));
    C::write16(pk, 2, static_cast<uint16_t>(pid >> 16));
    C::write16(pk, 0x08, species);
    C::write16(pk, 0x0C, 12345);
    C::write16(pk, 0x0E, 54321);
    C::write16(pk, 0x10, 1000);
    pk[0x14] = 70;
    const auto personal = G::personal5(species, 0, false);
    assert(personal);
    pk[0x15] = G::abilityForSlot5(*personal.info, (pid >> 16) & 1);
    pk[0x17] = 2;                          // English
    pk[0x40] = 1 << 1;                     // female
    pk[0x41] = 3;
    pk[0x5F] = 21;
    pk[0x83] = 4;
    pk[0x84] = 0x85;
    if (nicknamed) { pk[0x3B] = 0x80; }    // IV32 bit 31: nicknamed
    for (size_t i = 0; i < 11; ++i) C::write16(pk, 0x48 + 2 * i, 0x5A5A);  // distinctive trash
    for (size_t i = 0; i < name.size(); ++i) C::write16(pk, 0x48 + 2 * i, name[i]);
    C::write16(pk, 0x48 + 2 * name.size(), 0xFFFF);
    return C::encryptCandidate(pk);
}

std::vector<uint16_t> units(const G::StagedPokemon5Record& r) {
    const auto view = r.current();
    const auto bytes = view.decodedBytes();
    std::vector<uint16_t> out;
    for (size_t i = 0; i < 11; ++i) out.push_back(C::read16(bytes, 0x48 + 2 * i));
    return out;
}
std::vector<uint16_t> expect(const std::u16string& text, const std::vector<uint16_t>& before) {
    auto out = before;  // units after the terminator keep their old values
    for (size_t i = 0; i < text.size(); ++i) out[i] = text[i];
    if (text.size() < out.size()) out[text.size()] = 0xFFFF;
    return out;
}
}  // namespace

int main() {
    std::string error;
    // Un-nicknamed English Pikachu -> Japanese / French / Korean default names.
    {
        auto r = G::StagedPokemon5Record::create(record(25, u"Pikachu", false), &error, false);
        assert(r);
        const auto before = units(*r);
        assert(r->stage(F::Language, 0, 1, &error));
        assert(units(*r) == expect(u"ピカチュウ", before));
        assert(r->current().decodedBytes()[0x17] == 1);
        assert((r->current().decodedBytes()[0x3B] & 0x80) == 0);   // still not nicknamed
        assert(r->stage(F::Language, 0, 3, &error));
        assert(units(*r) == expect(u"Pikachu", expect(u"ピカチュウ", before)));
        assert(r->stage(F::Language, 0, 8, &error));
        assert(units(*r)[0] == u'피');
    }
    // Nidoran♀ (29), half-width: ♀ is stored as U+246E; Japanese (full-width) keeps U+2640.
    {
        auto r = G::StagedPokemon5Record::create(record(29, u"Nidoran⑮", false), &error, false);
        assert(r);
        assert(r->stage(F::Language, 0, 3, &error));
        const auto fr = units(*r);
        assert(fr[7] == 0x246E && fr[8] == 0xFFFF);
        assert(r->stage(F::Language, 0, 1, &error));
        const auto jp = units(*r);
        assert(jp[0] == u'ニ' && jp[4] == 0x2640 && jp[5] == 0xFFFF);
    }
    // Nicknamed record: language changes, the name bytes do not.
    {
        auto r = G::StagedPokemon5Record::create(record(25, u"SPARKY", true), &error, false);
        assert(r);
        const auto beforeView = r->current();
        const auto before = beforeView.decodedBytes();
        const std::vector<uint8_t> beforeBytes(before.begin(), before.end());
        assert(r->stage(F::Language, 0, 1, &error));
        const auto afterView = r->current();
        const auto after = afterView.decodedBytes();
        for (size_t i = 0; i < beforeBytes.size(); ++i)
            if (i != 0x17 && i != 0x06 && i != 0x07 && after[i] != beforeBytes[i]) { std::cerr << "byte 0x" << std::hex << i << " " << int(beforeBytes[i]) << "->" << int(after[i]) << std::dec << "\n"; assert(false); }
    }
    std::cout << "Gen V language change refreshes an un-nicknamed default name (PKHeX rule): PASS\n";
}
