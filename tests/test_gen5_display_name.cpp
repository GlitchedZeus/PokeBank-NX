// Round 11 H27: the Gen V editor header, Details rows and review/summary rows show the name the
// record actually stores, in every Gen V language, through ONE display decoder shared with the
// trainer-name screens (Gen5TrainerName.h displayGen5Text). Before this, the header used the ASCII
// edit gate (asciiName5), so a Japanese, Korean or accented French default name fell back to the
// English species name and cycling Language looked like it did nothing.
//   * an un-nicknamed record staged to JA / EN / FR / DE / KO shows exactly the language default
//     (Names::defaultSpeciesName) as UTF-8, and JA / KO / French accents differ from English;
//   * the half-width gender signs Gen V stores (U+246D / U+246E) display as ♂ / ♀, for Pokemon names
//     AND trainer names (the shared core's previously missing mapping);
//   * the name ends at 0xFFFF or 0x0000, a full-width name with no terminator is read to capacity,
//     and a lone surrogate, control or private-use unit yields no label (the caller's fallback);
//   * the edit gate is unchanged: asciiName5 still refuses a Japanese / Korean name, so a localized
//     default never becomes free-text-editable, and staging Language never touches the OT bytes.
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"
#include "Integration/Gen5/Gen5NativeText.h"
#include "Integration/Gen5/Gen5TrainerName.h"
#include "Names/DefaultSpeciesName.h"
#include "Names/SpeciesNames.h"
#include "Utils/StringHelpers.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace G = PokeVault::Integration::Gen5;
namespace C = G::Crypto;
using F = G::StagedPokemon5Record::Field;

std::vector<uint8_t> record(uint16_t species, const std::u16string& name, bool nicknamed,
                            const std::u16string& ot = u"NX") {
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
    pk[0x40] = 1 << 1;
    pk[0x41] = 3;
    pk[0x5F] = 21;
    pk[0x83] = 4;
    pk[0x84] = 0x85;
    if (nicknamed) pk[0x3B] = 0x80;
    for (size_t i = 0; i < 11; ++i) C::write16(pk, 0x48 + 2 * i, 0xFFFF);
    for (size_t i = 0; i < name.size(); ++i) C::write16(pk, 0x48 + 2 * i, name[i]);
    for (size_t i = 0; i < 8; ++i) C::write16(pk, 0x68 + 2 * i, 0xFFFF);
    for (size_t i = 0; i < ot.size(); ++i) C::write16(pk, 0x68 + 2 * i, ot[i]);
    return C::encryptCandidate(pk);
}

std::string header(const G::StagedPokemon5Record& r) {
    const auto view = r.current();
    return G::displayName5(view.decodedBytes(), true).value_or("");
}

std::vector<uint8_t> plainName(const std::vector<uint16_t>& units, bool nickname) {
    std::vector<uint8_t> plain(C::StoredSize, 0);
    const size_t off = G::nameOffset5(nickname);
    for (size_t i = 0; i < G::nameCapacity5(nickname); ++i)
        C::write16(plain, off + 2 * i, i < units.size() ? units[i] : 0x5A5A);
    return plain;
}
}  // namespace

int main() {
    std::string error;
    // Owner H27: cycling Language visibly changes an un-nicknamed name, as in Gen I-IV.
    struct Case { uint16_t species; uint8_t language; };
    const Case cases[] = {{25, 1}, {25, 8}, {4, 3}, {4, 5}, {495, 3}, {495, 1}, {495, 8}};
    for (const auto& c : cases) {
        auto r = G::StagedPokemon5Record::create(
            record(c.species, Names::defaultSpeciesName(c.species, 2, 5), false), &error, false);
        assert(r);
        assert(header(*r) == std::string(Names::getSpeciesName(c.species)));
        assert(r->stage(F::Language, 0, c.language, &error));
        const std::string expected = Utils::utf16ToUtf8(Names::defaultSpeciesName(c.species, c.language, 5));
        if (header(*r) != expected) {
            std::cerr << "species " << c.species << " language " << int(c.language) << ": header '"
                      << header(*r) << "' expected '" << expected << "'\n";
            assert(false);
        }
        assert(!header(*r).empty() && header(*r) != std::string(Names::getSpeciesName(c.species)));
        // JA, KO and accented FR names are exactly the ones the ASCII gate hid; the edit gate is
        // unchanged, so such a default name is still not free-text-editable.
        if (c.language != 5) assert(!G::asciiName5(r->current().decodedBytes(), true));
        // Language never rewrites the OT name.
        assert(G::displayName5(r->current().decodedBytes(), false).value_or("") == "NX");
    }
    assert(Utils::utf16ToUtf8(Names::defaultSpeciesName(495, 3, 5)) == "Vip\xC3\xA9lierre");

    // Nicknamed record: the nickname is shown in every language and never replaced.
    {
        auto r = G::StagedPokemon5Record::create(record(25, u"SPARKY", true, u"サトシ"), &error, false);
        assert(r);
        for (const uint8_t lang : {1, 3, 8, 2}) {
            assert(r->stage(F::Language, 0, lang, &error));
            assert(header(*r) == "SPARKY");
            assert(G::displayName5(r->current().decodedBytes(), false).value_or("") ==
                   "\xE3\x82\xB5\xE3\x83\x88\xE3\x82\xB7");   // サトシ
        }
    }

    // Nidoran♀ staged to French stores U+246E (half-width); it displays as ♀, not ⑮.
    {
        auto r = G::StagedPokemon5Record::create(record(29, u"Nidoran♀", false), &error, false);
        assert(r);
        assert(r->stage(F::Language, 0, 3, &error));
        assert(C::read16(r->current().decodedBytes(), 0x48 + 2 * 7) == 0x246E);
        assert(header(*r) == "Nidoran\xE2\x99\x80");
        assert(r->stage(F::Language, 0, 1, &error));             // full-width keeps U+2640
        assert(header(*r) == Utils::utf16ToUtf8(Names::defaultSpeciesName(29, 1, 5)));
    }
    {
        // Nidoran♂ half-width U+246D -> ♂.
        const auto plain = plainName({u'N', u'i', u'd', u'o', 0x246D, 0xFFFF}, true);
        assert(G::displayName5(plain, true).value() == "Nido\xE2\x99\x82");
    }

    // Terminators and capacity.
    {
        assert(G::displayName5(plainName({u'A', u'B', 0xFFFF, u'C'}, true), true).value() == "AB");
        assert(G::displayName5(plainName({u'A', u'B', 0x0000, u'C'}, true), true).value() == "AB");
        // Ten characters fill the field; the 11th unit is the terminator slot and is never shown.
        std::vector<uint16_t> ten;
        for (char16_t c : std::u16string(u"ピカチュウピカチュウ"))
            ten.push_back(c);
        ten.push_back(u'X');
        assert(G::displayName5(plainName(ten, true), true).value() ==
               Utils::utf16ToUtf8(u"ピカチュウピカチュウ"));
        // OT: seven characters, the 8th unit is never shown.
        assert(G::displayName5(plainName({u'A', u'B', u'C', u'D', u'E', u'F', u'G', u'H'}, false), false).value() ==
               "ABCDEFG");
        // Empty name and short buffer: no label.
        assert(!G::displayName5(plainName({0xFFFF}, true), true));
        assert(!G::displayName5(std::vector<uint8_t>(0x50, 0), true));
        // Malformed units: no label, so the header keeps its species-name fallback.
        assert(!G::displayName5(plainName({u'A', 0xD800, u'B', 0xFFFF}, true), true));
        assert(!G::displayName5(plainName({0xDC00, 0xFFFF}, true), true));
        assert(!G::displayName5(plainName({u'A', 0x0007, 0xFFFF}, true), true));
        assert(!G::displayName5(plainName({0xE000, 0xFFFF}, true), true));
        // Hangul passes through as Hangul.
        assert(G::displayName5(plainName({0xD53C, 0xCE74, 0xCE04, 0xFFFF}, true), true).value() ==
               "\xED\x94\xBC\xEC\xB9\xB4\xEC\xB8\x84");   // 피카츄
    }

    // Trainer names share the core and gain the same gender-sign mapping (skeptic correction 2).
    assert(G::displayTrainerName(u"AB⑮").value() == "AB\xE2\x99\x80");
    assert(G::displayTrainerName(u"AB⑭").value() == "AB\xE2\x99\x82");
    assert(G::displayTrainerName(u"NX").value() == "NX");
    assert(!G::displayTrainerName(u"ABCDEFGH"));                 // over the 7-unit trainer field
    assert(!G::displayTrainerName(std::u16string(1, char16_t(0xE000))));

    std::cout << "Gen V names display in their stored language (header, details, trainer): PASS\n";
}
