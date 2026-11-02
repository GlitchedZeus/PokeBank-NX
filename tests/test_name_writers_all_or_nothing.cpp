// Round 14 P0-2 / P0-3 / P1 (SHARED_UX_REQUIREMENTS.md): every name writer is all-or-nothing.
// For each writer, an over-long or unencodable input returns a refusal and leaves the destination
// bytes untouched; an accepted input decodes back to exactly the requested text. Before this:
//   * Pokemon3FRLG g3EncodeName stopped at the first unencodable glyph and at the cap (a FireRed
//     Egg OT "A×B" was stored as "A");
//   * Trainer3FRLG g3Encode stopped at the cap and reported success;
//   * Gen4TextCodec encodeGen4Field cut to the cap and wrote '?' (0x01AC) for a missing glyph, and
//     writeGen4Field cut to the cap;
//   * Utils::setStringChecked passed a text longer than the FIELD (but within maxUnits) on to the
//     truncating setString, and the legacy Gen VII-IX setters called setString directly, splitting
//     a surrogate pair at the 12-unit cap;
//   * the backup-name prompt silently dropped characters.
#include "Encryption/Encryption7LGPE.h"
#include "Encryption/Encryption8BDSP.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon7LGPE.h"
#include "Pokemon/Pokemon8BDSP.h"
#include "Text/TextFieldCapability.h"
#include "UI/BackupNameRule.h"
#include "Utils/Gen3Text.h"
#include "Utils/Gen4Text.h"
#include "Utils/Gen4TextCodec.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace T = PokeBank::Text;

namespace {
int failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}

std::vector<uint8_t> bytesOf(const Pokemon::Pokemon& p) {
    const auto d = p.getData();
    std::vector<uint8_t> out(d.size());
    std::memcpy(out.data(), d.data(), d.size());
    return out;
}

void gen3Writers() {
    const uint8_t english = 2, japanese = 1;
    // The capability itself.
    {
        const T::Gen3NameField ot(T::FieldKind::OriginalTrainer, english, 7, 7);
        std::array<uint8_t, 7> out{};
        out.fill(0xAB);
        const auto before = out;
        for (const char16_t* bad : {u"A×B", u"A…B", u"ABCDEFGH", u"A😀"}) {
            const auto v = ot.encode16(bad, out);
            check(!v.ok() && out == before, "Gen III OT refuses without writing");
        }
        check(ot.check16(u"A×B").refusal == T::Refusal::UnsupportedGlyph &&
              ot.check16(u"A×B").firstBadIndex == 1, "Gen III OT names the first bad glyph");
        check(ot.check16(u"ABCDEFGH").refusal == T::Refusal::TooLong, "Gen III OT over 7 is TooLong");
        check(ot.check(std::string("A\xFF" "B")).refusal == T::Refusal::MalformedText, "malformed UTF-8 refused");
        const auto ok = ot.encode16(u"BOB", out);
        check(ok.ok() && ot.decode(out) == u"BOB" && out[3] == Utils::GEN3_TERMINATOR && out[4] == 0xAB,
              "Gen III OT stores BOB + one terminator, trash after it kept");
        const auto full = ot.encode16(u"ABCDEFG", out);
        check(full.ok() && ot.decode(out) == u"ABCDEFG", "Gen III OT stores 7 characters exactly");
        // Japanese table: Latin is stored full-width (the same bytes); folds back equal.
        const T::Gen3NameField jp(T::FieldKind::OriginalTrainer, japanese, 5, 7);
        std::array<uint8_t, 7> jpOut{};
        check(jp.encode16(u"RED", jpOut).ok() && Utils::gen3FoldWidth(jp.decode(jpOut)) == u"RED",
              "Gen III Japanese OT stores Latin full-width");
        check(!jp.check16(u"ABCDEF").ok(), "Gen III Japanese OT over 5 refused");
        // Box name field (9 bytes, 8 characters), the shape Trainer3FRLG::g3Encode writes.
        const T::Gen3NameField box(T::FieldKind::BoxName, english, 8, 9);
        std::array<uint8_t, 9> boxOut{};
        boxOut.fill(0xCD);
        const auto boxBefore = boxOut;
        check(!box.encode16(u"ABCDEFGHI", boxOut).ok() && boxOut == boxBefore, "Gen III box name over 8 refused");
        check(box.encode16(u"ABCDEFGH", boxOut).ok() && box.decode(boxOut) == u"ABCDEFGH" &&
              boxOut[8] == Utils::GEN3_TERMINATOR, "Gen III box name of 8 stored with terminator");
    }
    // Pokemon3FRLG: an Egg in an English save (the host probe).
    {
        std::array<uint8_t, 80> blank{};
        Pokemon::Pokemon3FRLG egg(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
        egg.setSaveLanguage(english);
        egg.setSpecies(25);
        egg.setLanguage(english);
        egg.setOTName(u"RED");
        egg.setEgg(true);
        check(egg.language() == japanese && egg.otTextLanguage() == english, "Egg: JP record, EN OT text");
        const auto before = bytesOf(egg);
        for (const char16_t* bad : {u"A×B", u"A…B", u"AいB", u"ABCDEFGH"}) {
            check(!egg.canStoreOTName(bad), "Egg OT refused by canStoreOTName");
            egg.setOTName(bad);
            check(bytesOf(egg) == before && egg.otName() == u"RED", "Egg OT setter leaves bytes untouched");
        }
        check(egg.canStoreOTName(u"ABCDEFG"), "Egg OT of 7 in the save's (English) table is storable");
        egg.setOTName(u"ABCDEFG");
        check(egg.otName() == u"ABCDEFG" && egg.checksumValid(), "Egg OT of 7 stored exactly");
    }
    {
        std::array<uint8_t, 80> blank{};
        Pokemon::Pokemon3FRLG mon(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
        mon.setSpecies(25);
        mon.setLanguage(english);
        mon.setNickname(u"PIKACHU");
        const auto before = bytesOf(mon);
        mon.setNickname(u"ABCDEFGHIJK");   // 11 > 10
        check(bytesOf(mon) == before && mon.nickname() == u"PIKACHU", "Gen III nickname over 10 leaves bytes");
        check(!mon.canStoreNickname(u"ABCDEFGHIJK"), "canStoreNickname refuses 11 characters");
    }
}

void gen4Writers() {
    const uint8_t english = 2;
    check(Utils::encodeGen4Field(u"ABCDEFGHIJK", 11, 10, english).empty(), "Gen IV codec refuses 11 > 10");
    check(Utils::encodeGen4Field(u"AB😀", 11, 10, english).empty(), "Gen IV codec refuses non-BMP");
    check(Utils::encodeGen4Field(u"A€B", 11, 10, english).empty() ||
              Utils::decodeGen4Field(Utils::encodeGen4Field(u"A€B", 11, 10, english)) == u"A€B",
          "Gen IV codec never substitutes '?'");
    const auto ok = Utils::encodeGen4Field(u"ROCKY", 11, 10, english);
    check(ok.size() == 22 && Utils::decodeGen4Field(ok) == u"ROCKY", "Gen IV codec stores ROCKY exactly");
    std::array<uint8_t, 16> field{};
    field.fill(0x5A);
    const auto before = field;
    check(!Utils::writeGen4Field(field.data(), 8, u"ABCDEFGH", 7) && field == before,
          "Gen IV writeGen4Field refuses 8 > 7 without writing");
    check(!Utils::writeGen4Field(field.data(), 8, u"A😀", 7) && field == before,
          "Gen IV writeGen4Field refuses an unencodable glyph without writing");
    check(Utils::writeGen4Field(field.data(), 8, u"ABCDEFG", 7), "Gen IV writeGen4Field stores 7");
    const T::Gen4NameField nick(T::FieldKind::Nickname, english, 10, 11);
    check(nick.check16(u"ABCDEFGHIJ").ok() && !nick.check16(u"ABCDEFGHIJK").ok(), "Gen IV capability caps at 10");
}

void utf16Writers() {
    // 12 code units in a 26-byte field; one emoji costs two units.
    std::array<uint8_t, 26> dest{};
    dest.fill(0x77);
    const auto before = dest;
    const std::u16string elevenPlusEmoji = u"ABCDEFGHIJK😀";   // 12 code points, 13 units
    check(Utils::setStringChecked(dest.data(), 26, elevenPlusEmoji, 12) == Utils::CheckedTextResult::Overlength &&
          dest == before, "setStringChecked: 12 code points / 13 units refused, no lone surrogate written");
    check(Utils::setStringChecked(dest.data(), 26, u"ABCDEFGHIJKLM", 20) == Utils::CheckedTextResult::Overlength &&
          dest == before, "setStringChecked: within maxUnits but over the field is refused, not cut");
    check(Utils::setStringChecked(dest.data(), 26, std::u16string{u'A', static_cast<char16_t>(0xD83D)}, 12) == Utils::CheckedTextResult::MalformedUtf16 &&
          dest == before, "setStringChecked: lone surrogate refused");
    check(Utils::setStringChecked(dest.data(), 26, u"ABCDEFGHIJ😀", 12) == Utils::CheckedTextResult::Accepted,
          "setStringChecked: 12 units incl. a pair accepted");
    check(Utils::getString(dest.data(), 26) == u"ABCDEFGHIJ😀", "setStringChecked round trip exact");
    const T::Utf16NameField box(T::FieldKind::BoxName, 16, 34);
    check(box.check(std::string(16, 'A')).ok() && !box.check(std::string(17, 'A')).ok(), "UTF-16 box name caps in units");

    // Legacy Gen VII-IX setters (formerly Utils::setString): refuse, never cut.
    {
        constexpr std::size_t PARTY = 0x158;
        std::vector<std::byte> dec(PARTY, std::byte{0});
        dec[0] = std::byte{0x51};
        std::byte* enc = Encryption::encryptArray8BDSP(dec, 0x51u);
        Pokemon::Pokemon8BDSP bdsp(std::span<const std::byte>(enc, dec.size()));
        delete[] enc;
        bdsp.setNickname(u"PIKA");
        bdsp.setOTName(u"ASH");
        const auto snap = bytesOf(bdsp);
        check(!bdsp.canStoreNickname(elevenPlusEmoji) && !bdsp.canStoreOTName(elevenPlusEmoji), "BDSP canStore refuses 13 units");
        bdsp.setNickname(elevenPlusEmoji);
        bdsp.setOTName(elevenPlusEmoji);
        bdsp.setHTName(elevenPlusEmoji);
        check(bytesOf(bdsp) == snap && bdsp.nickname() == u"PIKA" && bdsp.otName() == u"ASH",
              "BDSP setters leave bytes untouched on refusal");
        bdsp.setNickname(u"ABCDEFGHIJ😀");
        check(bdsp.nickname() == u"ABCDEFGHIJ😀", "BDSP nickname with a pair stored exactly");
    }
    {
        std::vector<std::byte> dec(Encryption::SIZE_PARTY7_LGPE, std::byte{0});
        dec[0] = std::byte{0x40};
        std::byte* enc = Encryption::encryptArray7LGPE(dec, 0x40u);
        Pokemon::Pokemon7LGPE lgpe(std::span<const std::byte>(enc, dec.size()));
        delete[] enc;
        lgpe.setOTName(u"ASH");
        const auto snap = bytesOf(lgpe);
        lgpe.setOTName(elevenPlusEmoji);
        lgpe.setNickname(elevenPlusEmoji);
        check(bytesOf(lgpe) == snap && lgpe.otName() == u"ASH", "LGPE setters leave bytes untouched on refusal");
    }
}

void backupNames() {
    namespace M = PokeBank::UIModel;
    check(M::backupFolderName("Run 2").ok && M::backupFolderName("Run 2").folder == "Run 2", "plain backup name kept");
    for (const char* bad : {"Run #2", "\xC3\x89VA", "a/b", "..", "", " lead", "trail "}) {
        const auto v = M::backupFolderName(bad);
        check(!v.ok && !v.reason.empty(), std::string("backup name refused with reason: ") + bad);
    }
    check(M::backupNameSuggestion("Pok\xC3\xA9mon FireRed") == "Pokmon FireRed", "suggestion uses allowed characters");
    check(M::backupFolderName(M::backupNameSuggestion("Pok\xC3\xA9mon FireRed")).ok, "suggestion passes the rule");
    check(M::backupNameSuggestion("\xC3\x89\xC3\x89") == "Backup", "empty suggestion falls back");
}
}  // namespace

int main() {
    gen3Writers();
    gen4Writers();
    utf16Writers();
    backupNames();
    if (failures) {
        std::cerr << failures << " name-writer failure(s)\n";
        return 1;
    }
    std::cout << "Name writers all-or-nothing (Gen III / Gen IV / UTF-16 / backup names): PASS\n";
    return 0;
}
