#include "Encryption/Encryption4.h"
#include "Pokemon/Experience.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Names/Gen4HeldItemCatalog.h"
#include "Names/MoveInfo.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

void w16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}

void w32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
    b[o + 2] = static_cast<std::byte>(v >> 16);
    b[o + 3] = static_cast<std::byte>(v >> 24);
}

void copy(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(),
              dst.begin() + static_cast<std::ptrdiff_t>(off));
}

std::vector<std::byte> makeEntity(
    uint16_t species = 25, uint32_t pid = 0x12345678u,
    uint16_t ability = 9, uint8_t originVersion = 10) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    w32(d, 0x00, pid);
    w16(d, 0x08, species);
    w16(d, 0x0A, 1);
    w16(d, 0x0C, 12345);
    w16(d, 0x0E, 54321);
    w32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    d[0x15] = static_cast<std::byte>(ability);
    d[0x17] = std::byte{2};
    for (size_t i = 0; i < 6; ++i)
        d[0x18 + i] = static_cast<std::byte>(i + 1);
    w16(d, 0x28, 85);
    w16(d, 0x2A, 98);
    w16(d, 0x2C, 86);
    w16(d, 0x2E, 104);
    d[0x30] = std::byte{15};
    d[0x31] = std::byte{30};
    d[0x32] = std::byte{20};
    d[0x33] = std::byte{15};
    d[0x34] = std::byte{1};
    d[0x35] = std::byte{2};
    d[0x36] = std::byte{3};
    w32(d, 0x38, 31u | (30u << 5) | (29u << 10) | (28u << 15) |
                  (27u << 20) | (26u << 25));
    d[0x40] = static_cast<std::byte>(species == 292 ? 0x04 : 0x00); // Shedinja genderless; default fixture male
    auto nick = Utils::encodeGen4Field(u"PIKA", 11, 10, 2);
    copy(d, 0x48, nick);
    // Native entity strings may have post-terminator trash. Keep a recognizable tail.
    for (size_t i = 0x54; i < 0x5E; ++i)
        d[i] = static_cast<std::byte>(0x90 + (i - 0x54));
    d[0x5F] = static_cast<std::byte>(originVersion);
    auto ot = Utils::encodeGen4Field(u"ASH", 8, 7, 2);
    copy(d, 0x68, ot);
    d[0x82] = std::byte{0x21};
    d[0x83] = std::byte{4};
    d[0x84] = std::byte{25};
    return Encryption::encryptArray4(d);
}

std::vector<std::byte> makePartyEntity(
    uint16_t species = 25, uint32_t pid = 0x12345678u,
    uint16_t ability = 9, uint16_t currentHp = 20) {
    const auto stored = makeEntity(species, pid, ability);
    const auto decryptedStored = Encryption::decryptArray4(stored);
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    std::copy(decryptedStored.begin(), decryptedStored.end(), d.begin());
    d[0x8C] = std::byte{25};
    w16(d, 0x8E, currentHp);
    w16(d, 0x90, species == 292 ? 1 : 60);
    w16(d, 0x92, 35);
    w16(d, 0x94, 30);
    w16(d, 0x96, 55);
    w16(d, 0x98, 40);
    w16(d, 0x9A, 40);
    return Encryption::encryptArray4(d);
}

void testNoOpAndSimpleFields() {
    auto encrypted = makeEntity();
    std::string error;
    auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(
        encrypted, Enums::GameVersion::PT, &error);
    assert(editable && error.empty());
    assert(editable->encryptedBytes() == encrypted);

    static_assert(Names::isGen4HeldItemPresent(1, Enums::GameVersion::DP));
    static_assert(!Names::isGen4HeldItemPresent(5, Enums::GameVersion::DP));
    static_assert(!Names::isGen4HeldItemPresent(112, Enums::GameVersion::DP));
    static_assert(Names::isGen4HeldItemPresent(112, Enums::GameVersion::PT));
    static_assert(Names::isGen4HeldItemPresent(112, Enums::GameVersion::HGSS));
    static_assert(Names::isGen4HeldItemPresent(419, Enums::GameVersion::PT));
    static_assert(!Names::isGen4HeldItemPresent(420, Enums::GameVersion::PT));

    const uint16_t validHeldItem = 1;
    assert(editable->setHeldItem(validHeldItem));
    assert(editable->setFriendship(123));
    assert(editable->setEV(0, 252));
    assert(editable->setIV(5, 31));
    assert(Names::getMoveBasePP(105, Enums::GameVersion::PT) == 10); // Recover in Gen IV
    assert(Names::getMoveBasePP(467, Enums::GameVersion::PT) == 5);  // Shadow Force
    assert(editable->setMove(1, 237));
    assert(editable->pp()[1] == Names::getMoveBasePP(237, Enums::GameVersion::PT));
    assert(editable->ppUps()[1] == 0);
    assert(editable->setPP(1, 12));
    assert(editable->setPPUps(1, 3));
    assert(editable->setPokerus(0x43));
    assert(editable->setBall(16));
    assert(editable->setMetLevel(42));
    assert(editable->setLevel(50));

    Pokemon::Pokemon4ReadOnly parsed(
        editable->encryptedBytes(), Enums::GameVersion::PT);
    assert(parsed.valid());
    assert(parsed.heldItem() == validHeldItem);
    assert(parsed.friendship() == 123);
    assert(parsed.evs()[0] == 252);
    assert(parsed.ivs()[5] == 31);
    assert(parsed.moves()[1] == 237);
    assert(parsed.pp()[1] == 12);
    assert(parsed.ppUps()[1] == 3);
    assert(parsed.pokerusState() == 0x43);
    assert(parsed.ballDPPt() == 16);
    assert(parsed.metLevel() == 42);
    assert(Pokemon::getLevelFromExp(parsed.experience(), parsed.personal().growthRate) == 50);
}

void testNicknamePreservesTrash() {
    auto encrypted = makeEntity();
    auto before = Encryption::decryptArray4(encrypted);
    auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(
        encrypted, Enums::GameVersion::PT);
    assert(editable);
    assert(editable->setNickname(u"PI"));
    const auto after = Encryption::decryptArray4(editable->encryptedBytes());
    Pokemon::Pokemon4ReadOnly parsed(
        editable->encryptedBytes(), Enums::GameVersion::PT);
    assert(parsed.valid() && parsed.nickname() == u"PI");

    // "PI" occupies two code units + terminator, so bytes after 0x4D are native trash
    // and must remain byte-exact rather than being pre-cleared.
    for (size_t i = 0x4E; i < 0x5E; ++i)
        assert(after[i] == before[i]);
}

void testPidCoupledEdits() {
    auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(), Enums::GameVersion::PT);
    assert(editable);
    const uint8_t startingGender = editable->gender();
    const bool startingShiny = editable->shiny();

    const uint8_t newNature = static_cast<uint8_t>((editable->nature() + 1) % 25);
    assert(editable->setNature(newNature));
    assert(editable->nature() == newNature);
    assert(editable->gender() == startingGender);
    assert(editable->shiny() == startingShiny);

    assert(editable->setGender(startingGender == 0 ? 1 : 0));
    const uint8_t toggledGender = editable->gender();
    assert(toggledGender != startingGender);
    assert(editable->nature() == newNature);
    assert(editable->shiny() == startingShiny);

    assert(editable->setShiny(!startingShiny));
    assert(editable->shiny() != startingShiny);
    assert(editable->gender() == toggledGender);
    assert(editable->nature() == newNature);

    Pokemon::Pokemon4ReadOnly parsed(
        editable->encryptedBytes(), Enums::GameVersion::PT);
    assert(parsed.valid());
    assert(parsed.gender() == toggledGender);
}

void testFixedGenderAndAbilitySlots() {
    // Nidoran F is fixed female.
    auto female = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(29), Enums::GameVersion::PT);
    assert(female);
    assert(female->setGender(1));
    const auto femaleBefore = female->encryptedBytes();
    assert(!female->setGender(0));
    assert(female->encryptedBytes() == femaleBefore);

    // Magnemite is genderless.
    auto genderless = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(81), Enums::GameVersion::PT);
    assert(genderless);
    assert(genderless->setGender(2));
    const auto genderlessBefore = genderless->encryptedBytes();
    assert(!genderless->setGender(1));
    assert(genderless->encryptedBytes() == genderlessBefore);

    // Pidgey has two distinct Gen IV ability slots; selecting the other one must
    // preserve nature, gender and shininess while keeping PID parity consistent.
    const auto& pidgey = Pokemon::getPersonalInfo4PT(16, 0);
    assert(pidgey.ability1 != 0 && pidgey.ability2 != 0 &&
           pidgey.ability1 != pidgey.ability2);
    auto dual = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(16, 0x12345678u, pidgey.ability1),
        Enums::GameVersion::PT);
    assert(dual);
    const uint8_t nature = dual->nature();
    const uint8_t gender = dual->gender();
    const bool shiny = dual->shiny();
    assert(dual->setAbilitySlot(1));
    assert(dual->abilitySlot() == 1);
    assert(dual->ability() == pidgey.ability2);
    assert(dual->nature() == nature);
    assert(dual->gender() == gender);
    assert(dual->shiny() == shiny);

    Pokemon::Pokemon4ReadOnly parsed(
        dual->encryptedBytes(), Enums::GameVersion::PT);
    assert(parsed.valid() && parsed.ability() == pidgey.ability2);
}

void testShedinjaPartyHpRule() {
    for (const auto group : {Enums::GameVersion::DP, Enums::GameVersion::PT,
                             Enums::GameVersion::HGSS}) {
        auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(
            makePartyEntity(292, 0x12345678u, 25, 1), group);
        assert(editable && editable->isParty());
        assert(editable->gender() == 2);

        const auto assertOneHp = [&]() {
            Pokemon::Pokemon4ReadOnly parsed(editable->encryptedBytes(), group);
            assert(parsed.valid() && parsed.isParty());
            assert(parsed.maxHP() == 1);
            assert(parsed.currentHP() <= 1);
        };

        assert(editable->setLevel(50));
        assertOneHp();
        assert(editable->setIV(0, 31));
        assertOneHp();
        assert(editable->setEV(0, 252));
        assertOneHp();
        const uint8_t nextNature = static_cast<uint8_t>((editable->nature() + 1) % 25);
        assert(editable->setNature(nextNature));
        assertOneHp();

        // Fainted state must stay fainted while the derived maximum remains the Shedinja constant.
        auto fainted = Pokemon::Pokemon4Mutable::fromEncrypted(
            makePartyEntity(292, 0x22345678u, 25, 0), group);
        assert(fainted && fainted->setLevel(60));
        Pokemon::Pokemon4ReadOnly faintedParsed(fainted->encryptedBytes(), group);
        assert(faintedParsed.valid() && faintedParsed.maxHP() == 1 &&
               faintedParsed.currentHP() == 0);
    }
}

void testSpeciesReconciliation() {
    const auto& pidgeyPersonal = Pokemon::getPersonalInfo4PT(16, 0);
    auto mon = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(16, 0x12345678u, pidgeyPersonal.ability1),
        Enums::GameVersion::PT);
    assert(mon);
    const uint8_t oldLevel = mon->level();
    const uint8_t oldNature = mon->nature();
    const bool oldShiny = mon->shiny();

    // Fixed-female and genderless targets must update both the PID-derived constraint
    // and the stored Gen IV gender bits.
    assert(mon->setSpecies(29)); // Nidoran F
    Pokemon::Pokemon4ReadOnly nidoranF(mon->encryptedBytes(), Enums::GameVersion::PT);
    assert(nidoranF.valid() && nidoranF.species() == 29 && nidoranF.gender() == 1);

    // Magnemite forces genderless + base form while preserving Level/Nature/Shiny.
    assert(mon->setSpecies(81));
    Pokemon::Pokemon4ReadOnly magnemite(mon->encryptedBytes(), Enums::GameVersion::PT);
    assert(magnemite.valid() && magnemite.species() == 81);
    assert(magnemite.gender() == 2);
    assert(magnemite.form() == 0);
    assert(static_cast<uint8_t>(magnemite.pid() % 25u) == oldNature);
    const uint16_t magPsv = static_cast<uint16_t>(
        (magnemite.pid() & 0xFFFFu) ^ (magnemite.pid() >> 16));
    assert((static_cast<uint16_t>(magnemite.tid() ^ magnemite.sid() ^ magPsv) < 8u) == oldShiny);
    assert(Pokemon::getLevelFromExp(
        magnemite.experience(), magnemite.personal().growthRate) == oldLevel);
    assert(magnemite.nickname() == u"MAGNEMITE");
    assert(!magnemite.isNicknamed());

    // A user nickname is intent, not a species-name cache; keep it across Species edits.
    auto nicknamed = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25), Enums::GameVersion::PT);
    assert(nicknamed && nicknamed->setNickname(u"SPARK"));
    assert(nicknamed->setSpecies(133));
    Pokemon::Pokemon4ReadOnly eevee(nicknamed->encryptedBytes(), Enums::GameVersion::PT);
    assert(eevee.valid() && eevee.species() == 133);
    assert(eevee.nickname() == u"SPARK" && eevee.isNicknamed());

    // Preserve a requested dual-ability slot where the target species also has two slots.
    auto dual = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(16, 0x12345678u, pidgeyPersonal.ability1),
        Enums::GameVersion::PT);
    assert(dual && dual->setAbilitySlot(1));
    const uint8_t slotBefore = dual->abilitySlot();
    assert(slotBefore == 1);
    assert(dual->setSpecies(133));
    assert(dual->abilitySlot() == 1);
    assert(dual->ability() == dual->abilityForSlot(1));

    // Party Species edits refresh live battle fields, including Shedinja's fixed 1 HP.
    auto party = Pokemon::Pokemon4Mutable::fromEncrypted(
        makePartyEntity(16, 0x12345678u, pidgeyPersonal.ability1, 20),
        Enums::GameVersion::PT);
    assert(party && party->setSpecies(292));
    Pokemon::Pokemon4ReadOnly shedinja(party->encryptedBytes(), Enums::GameVersion::PT);
    assert(shedinja.valid() && shedinja.isParty() && shedinja.species() == 292);
    assert(shedinja.maxHP() == 1 && shedinja.currentHP() <= 1);

    const auto stable = dual->encryptedBytes();
    assert(!dual->setSpecies(0));
    assert(dual->encryptedBytes() == stable);
    assert(!dual->setSpecies(494));
    assert(dual->encryptedBytes() == stable);
}


void testOriginalTrainerIdentityEdits() {
    auto mon = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(16), Enums::GameVersion::PT);
    assert(mon);

    const uint16_t oldSid = mon->sid();
    const uint8_t oldNature = mon->nature();
    const uint8_t oldGender = mon->gender();
    const bool oldShiny = mon->shiny();
    const uint16_t oldAbility = mon->ability();

    assert(mon->setOriginalTrainerName(u"RED"));
    Pokemon::Pokemon4ReadOnly renamed(mon->encryptedBytes(), Enums::GameVersion::PT);
    assert(renamed.valid() && renamed.originalTrainerName() == u"RED");

    assert(mon->setTID(54321));
    Pokemon::Pokemon4ReadOnly retid(mon->encryptedBytes(), Enums::GameVersion::PT);
    assert(retid.valid() && retid.tid() == 54321 && retid.sid() == oldSid);
    assert(retid.originalTrainerName() == u"RED");
    assert(static_cast<uint8_t>(retid.pid() % 25u) == oldNature);
    const uint16_t psv = static_cast<uint16_t>(
        (retid.pid() & 0xFFFFu) ^ (retid.pid() >> 16));
    const bool stillShiny =
        static_cast<uint16_t>(retid.tid() ^ retid.sid() ^ psv) < 8u;
    assert(stillShiny == oldShiny);
    assert(retid.gender() == oldGender);
    assert(retid.ability() == oldAbility);
}

void testCatalogBackedFieldValidation() {
    auto pt = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(), Enums::GameVersion::PT);
    assert(pt);

    // EV edits enforce the native total cap, not only the per-stat byte range.
    for (size_t i = 0; i < 6; ++i) assert(pt->setEV(i, 0));
    assert(pt->setEV(0, 252));
    assert(pt->setEV(1, 252));
    assert(pt->setEV(2, 6)); // exactly 510 total
    const auto beforeTooMany = pt->encryptedBytes();
    assert(!pt->setEV(2, 7));
    assert(pt->encryptedBytes() == beforeTooMany);

    assert(pt->setHeldItem(112)); // Griseous Orb is holdable in Platinum/HGSS.
    assert(!pt->setHeldItem(420)); // HM01 is not a holdable PK4 item.
    assert(!pt->setHeldItem(65535));
    assert(pt->setMove(0, 1));   // Pound exists in Gen IV.
    assert(pt->pp()[0] == 35 && pt->ppUps()[0] == 0);
    assert(pt->setPPUps(0, 3));
    assert(pt->setPP(0, 56));
    assert(pt->setMove(0, 105)); // Recover is 10 PP in Gen IV, not modern fallback PP.
    assert(pt->pp()[0] == 10 && pt->ppUps()[0] == 0);
    assert(pt->setMove(0, 0));
    assert(pt->moves()[0] == 0 && pt->pp()[0] == 0 && pt->ppUps()[0] == 0);
    assert(pt->setMove(0, 467)); // Shadow Force is the final native Gen IV move.
    assert(pt->pp()[0] == 5 && pt->ppUps()[0] == 0);
    const auto nativeMoveBytes = pt->encryptedBytes();
    assert(!pt->setMove(0, 468)); // Hone Claws begins Generation V.
    assert(pt->encryptedBytes() == nativeMoveBytes);
    assert(!pt->setMove(0, 1000));

    assert(pt->setBall(16)); // Cherish Ball exists in D/P/Pt.
    const auto beforeInvalidBall = pt->encryptedBytes();
    assert(!pt->setBall(17)); // Apricorn balls are HGSS-only.
    assert(pt->encryptedBytes() == beforeInvalidBall);

    const uint8_t oldLanguage = pt->language();
    assert(!pt->setLanguage(9)); // Chinese is not a PK4 language.
    assert(pt->language() == oldLanguage);
    assert(pt->setLanguage(3));
    Pokemon::Pokemon4ReadOnly french(pt->encryptedBytes(), Enums::GameVersion::PT);
    assert(french.valid() && french.language() == 3);
    assert(!french.isNicknamed());

    auto hgss = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(), Enums::GameVersion::HGSS);
    assert(hgss);
    const auto beforeWrongOriginBall = hgss->encryptedBytes();
    assert(!hgss->setBall(24));
    assert(hgss->encryptedBytes() == beforeWrongOriginBall);
    assert(!hgss->setBall(25));
    // This fixture is Diamond-origin even though it currently lives in an HGSS
    // container; origin metadata therefore remains in the D/P field.
    assert(hgss->setMetLocation(16));
    Pokemon::Pokemon4ReadOnly hgssParsed(hgss->encryptedBytes(), Enums::GameVersion::HGSS);
    assert(hgssParsed.valid() && hgssParsed.metLocationDP() == 16);
    assert(hgssParsed.metLocationExtended() == 0);
}


void testLanguageTransitions() {
    auto custom = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(), Enums::GameVersion::PT);
    assert(custom);
    assert(custom->setNickname(u"SPARK"));
    const auto beforeCustom = std::vector<std::byte>(
        custom->decryptedBytes().begin(), custom->decryptedBytes().end());

    assert(custom->setLanguage(1)); // Japanese language ID.
    Pokemon::Pokemon4ReadOnly japanese(
        custom->encryptedBytes(), Enums::GameVersion::PT);
    assert(japanese.valid() && japanese.language() == 1);
    assert(japanese.nickname() == u"SPARK");
    assert(japanese.originalTrainerName() == u"ASH");

    const auto afterJapanese = custom->decryptedBytes();
    for (size_t i = 0; i < beforeCustom.size(); ++i) {
        if (i == 0x17) continue;
        assert(afterJapanese[i] == beforeCustom[i]);
    }

    assert(custom->setLanguage(8)); // Korean is supported by Gen IV.
    Pokemon::Pokemon4ReadOnly koreanCustom(
        custom->encryptedBytes(), Enums::GameVersion::PT);
    assert(koreanCustom.valid() && koreanCustom.language() == 8);
    assert(koreanCustom.nickname() == u"SPARK");
    assert(koreanCustom.originalTrainerName() == u"ASH");

    const auto stable = custom->encryptedBytes();
    assert(!custom->setLanguage(9)); // Chinese is not a PK4 language.
    assert(custom->encryptedBytes() == stable);

    auto nativeName = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(), Enums::GameVersion::PT);
    assert(nativeName);
    assert(nativeName->setLanguage(8));
    Pokemon::Pokemon4ReadOnly koreanDefault(
        nativeName->encryptedBytes(), Enums::GameVersion::PT);
    assert(koreanDefault.valid() && koreanDefault.language() == 8);
    assert(!koreanDefault.isNicknamed());
    assert(!koreanDefault.nickname().empty());
    assert(koreanDefault.originalTrainerName() == u"ASH");
}

void testPokerusModes() {
    auto mon = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(), Enums::GameVersion::PT);
    assert(mon);
    assert(mon->setPokerus(0x43));

    const auto before = std::vector<std::byte>(
        mon->decryptedBytes().begin(), mon->decryptedBytes().end());
    assert(mon->setPokerusMode(Pokemon::PokerusMode::Infected));
    assert(mon->pokerus() == 0x43);
    assert(std::equal(mon->decryptedBytes().begin(), mon->decryptedBytes().end(),
                      before.begin()));

    assert(mon->setPokerusMode(Pokemon::PokerusMode::Cured));
    assert(mon->pokerus() == 0x40);
    const auto cured = mon->decryptedBytes();
    for (size_t i = 0; i < before.size(); ++i)
        if (i != 0x82) assert(cured[i] == before[i]);

    assert(mon->setPokerusMode(Pokemon::PokerusMode::Infected));
    assert(mon->pokerus() == 0x41);
    assert(mon->setPokerusMode(Pokemon::PokerusMode::None));
    assert(mon->pokerus() == 0x00);
    assert(mon->setPokerusMode(Pokemon::PokerusMode::Cured));
    assert(mon->pokerus() == 0x10);
    assert(mon->setPokerusMode(Pokemon::PokerusMode::Infected));
    assert(mon->pokerus() == 0x11);

    const auto stable = mon->encryptedBytes();
    assert(!mon->setPokerusMode(static_cast<Pokemon::PokerusMode>(99)));
    assert(mon->encryptedBytes() == stable);
}

void testExactBallSemantics() {
    auto dpInHgss = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 10), Enums::GameVersion::HGSS);
    assert(dpInHgss);
    const auto dpBeforeSport = dpInHgss->encryptedBytes();
    assert(!dpInHgss->setBall(24));
    assert(dpInHgss->encryptedBytes() == dpBeforeSport);
    assert(dpInHgss->setBall(16));
    Pokemon::Pokemon4ReadOnly dpBall(
        dpInHgss->encryptedBytes(), Enums::GameVersion::HGSS);
    assert(dpBall.valid() && dpBall.ballDPPt() == 16 && dpBall.ballHGSS() == 0);
    assert(dpInHgss->ball() == 16);

    auto hgInDp = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 7), Enums::GameVersion::DP);
    assert(hgInDp && hgInDp->setBall(4));
    Pokemon::Pokemon4ReadOnly hgPoke(
        hgInDp->encryptedBytes(), Enums::GameVersion::DP);
    assert(hgPoke.valid() && hgPoke.ballDPPt() == 4 && hgPoke.ballHGSS() == 4);
    assert(hgInDp->ball() == 4);

    assert(hgInDp->setBall(24));
    Pokemon::Pokemon4ReadOnly hgSport(
        hgInDp->encryptedBytes(), Enums::GameVersion::DP);
    assert(hgSport.valid() && hgSport.ballDPPt() == 4 && hgSport.ballHGSS() == 24);
    assert(hgInDp->ball() == 24);
    const auto hgStable = hgInDp->encryptedBytes();
    assert(!hgInDp->setBall(25));
    assert(hgInDp->encryptedBytes() == hgStable);

    auto ptInHgss = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 12), Enums::GameVersion::HGSS);
    assert(ptInHgss && ptInHgss->setBall(16));
    const auto ptStable = ptInHgss->encryptedBytes();
    assert(!ptInHgss->setBall(17));
    assert(ptInHgss->encryptedBytes() == ptStable);
}

void testExactMetLocationSemantics() {
    // D/P native locations occupy only the original 0..111 bank and do not use
    // the Pt/HGSS extended field.
    auto dp = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 10), Enums::GameVersion::DP);
    assert(dp && dp->setMetLocation(16));
    Pokemon::Pokemon4ReadOnly dpParsed(dp->encryptedBytes(), Enums::GameVersion::DP);
    assert(dpParsed.valid() && dpParsed.metLocationDP() == 16);
    assert(dpParsed.metLocationExtended() == 0);
    const auto dpBeforeInvalid = dp->encryptedBytes();
    assert(!dp->setMetLocation(112));
    assert(dp->encryptedBytes() == dpBeforeInvalid);

    // Platinum mirrors common locations, but Platinum-only 112..125 locations use
    // Faraway Place in D/P's legacy field plus the real extended value.
    auto pt = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 12), Enums::GameVersion::PT);
    assert(pt && pt->setMetLocation(111));
    Pokemon::Pokemon4ReadOnly ptCommon(pt->encryptedBytes(), Enums::GameVersion::PT);
    assert(ptCommon.valid() && ptCommon.metLocationDP() == 111);
    assert(ptCommon.metLocationExtended() == 111);
    assert(pt->setMetLocation(125));
    Pokemon::Pokemon4ReadOnly ptUnique(pt->encryptedBytes(), Enums::GameVersion::PT);
    assert(ptUnique.valid() && ptUnique.metLocationDP() == 3002);
    assert(ptUnique.metLocationExtended() == 125);
    const auto ptBeforeInvalid = pt->encryptedBytes();
    assert(!pt->setMetLocation(126));
    assert(pt->encryptedBytes() == ptBeforeInvalid);

    // HG/SS native locations are 126..233 and are represented through the extended
    // field with Faraway Place for D/P compatibility.
    auto hg = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 7), Enums::GameVersion::HGSS);
    assert(hg);
    const auto hgBeforeInvalid = hg->encryptedBytes();
    assert(!hg->setMetLocation(125));
    assert(hg->encryptedBytes() == hgBeforeInvalid);
    assert(hg->setMetLocation(182)); // Route 34 / canonical HGSS hatch area.
    Pokemon::Pokemon4ReadOnly hgParsed(hg->encryptedBytes(), Enums::GameVersion::HGSS);
    assert(hgParsed.valid() && hgParsed.metLocationDP() == 3002);
    assert(hgParsed.metLocationExtended() == 182);
    const auto hgStable = hg->encryptedBytes();
    assert(!hg->setMetLocation(234));
    assert(hg->encryptedBytes() == hgStable);

    // Current save family must not overwrite origin semantics: a Platinum-origin
    // PK4 stored in a D/P save still uses Platinum's exact field representation.
    auto ptInDp = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(25, 0x12345678u, 9, 12), Enums::GameVersion::DP);
    assert(ptInDp && ptInDp->setMetLocation(125));
    Pokemon::Pokemon4ReadOnly ptInDpParsed(
        ptInDp->encryptedBytes(), Enums::GameVersion::DP);
    assert(ptInDpParsed.valid() && ptInDpParsed.metLocationDP() == 3002);
    assert(ptInDpParsed.metLocationExtended() == 125);
}

void testNativeStoredCreateDraft() {
    struct Case { Enums::GameVersion group; uint8_t origin; };
    for (const auto tc : {Case{Enums::GameVersion::DP, 10},
                          Case{Enums::GameVersion::PT, 12},
                          Case{Enums::GameVersion::HGSS, 7}}) {
        Pokemon::Pokemon4CreateDefaults defaults;
        defaults.species = 393; // Piplup
        defaults.tid = 12150;
        defaults.sid = 22558;
        defaults.language = 2;
        defaults.otGender = 1;
        defaults.originVersion = tc.origin;
        defaults.level = 5;
        defaults.ball = 4;
        defaults.metLevel = 5;
        defaults.otName = u"Kylie";

        std::string error;
        auto draft = Pokemon::Pokemon4Mutable::createStored(
            defaults, tc.group, &error);
        assert(draft && error.empty() && !draft->isParty());

        Pokemon::Pokemon4ReadOnly parsed(draft->encryptedBytes(), tc.group);
        assert(parsed.valid() && !parsed.empty() && !parsed.isParty());
        assert(parsed.species() == 393);
        assert(parsed.tid() == 12150 && parsed.sid() == 22558);
        assert(parsed.language() == 2);
        assert(parsed.originVersion() == tc.origin);
        assert(parsed.originalTrainerName() == u"Kylie");
        assert(parsed.originalTrainerGender() == 1);
        assert(parsed.nickname() == u"PIPLUP");
        assert(!parsed.isNicknamed());
        assert(draft->nickname() == u"PIPLUP");
        assert(parsed.heldItem() == 0);
        assert(parsed.ballDPPt() == 4);
        if (tc.group == Enums::GameVersion::HGSS)
            assert(parsed.ballHGSS() == 4);
        else
            assert(parsed.ballHGSS() == 0);
        assert(Pokemon::getLevelFromExp(
            parsed.experience(), parsed.personal().growthRate) == 5);
        assert(parsed.friendship() == parsed.personal().baseFriendship);
        assert(parsed.ability() == parsed.personal().ability1);
        assert((parsed.pid() & 1u) == 0);
        assert(!draft->shiny());
    }

    Pokemon::Pokemon4CreateDefaults bad;
    bad.species = 494;
    bad.otName = u"ASH";
    std::string error;
    assert(!Pokemon::Pokemon4Mutable::createStored(
        bad, Enums::GameVersion::PT, &error));
    assert(!error.empty());

    bad.species = 25;
    bad.originVersion = 10; // Diamond origin does not match Platinum target group.
    error.clear();
    assert(!Pokemon::Pokemon4Mutable::createStored(
        bad, Enums::GameVersion::PT, &error));
    assert(!error.empty());
}

void testExactGameForms() {
    auto dp = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(479, 0x12345678u, 26), Enums::GameVersion::DP);
    assert(dp);
    assert(dp->formCount() == 1);
    assert(dp->form() == 0);
    const auto dpBefore = dp->encryptedBytes();
    assert(!dp->setForm(1));
    assert(dp->encryptedBytes() == dpBefore);

    for (const auto group : {Enums::GameVersion::PT, Enums::GameVersion::HGSS}) {
        auto rotom = Pokemon::Pokemon4Mutable::fromEncrypted(
            makeEntity(479, 0x12345678u, 26), group);
        assert(rotom);
        assert(rotom->formCount() == 6);
        assert(rotom->setForm(1));
        Pokemon::Pokemon4ReadOnly heat(rotom->encryptedBytes(), group);
        assert(heat.valid() && heat.species() == 479 && heat.form() == 1);
        assert(heat.ability() == 26);
        assert(rotom->setForm(5));
        Pokemon::Pokemon4ReadOnly mow(rotom->encryptedBytes(), group);
        assert(mow.valid() && mow.form() == 5);
        const auto beforeBadForm = rotom->encryptedBytes();
        assert(!rotom->setForm(6));
        assert(rotom->encryptedBytes() == beforeBadForm);
        assert(rotom->setForm(0));
        Pokemon::Pokemon4ReadOnly normal(rotom->encryptedBytes(), group);
        assert(normal.valid() && normal.form() == 0);
    }
}

void testItemDrivenAndStorageFormRules() {
    // Diamond/Pearl do not expose Giratina Origin Forme in their personal table.
    auto dpGiratina = Pokemon::Pokemon4Mutable::fromEncrypted(
        makeEntity(487, 0x12345678u, 46), Enums::GameVersion::DP);
    assert(dpGiratina && dpGiratina->formCount() == 1);
    assert(!dpGiratina->setForm(1));

    for (const auto group : {Enums::GameVersion::PT, Enums::GameVersion::HGSS}) {
        auto giratina = Pokemon::Pokemon4Mutable::fromEncrypted(
            makeEntity(487, 0x12345678u, 46), group);
        assert(giratina && giratina->formCount() == 2);
        assert(giratina->setForm(1));
        assert(giratina->form() == 1 && giratina->heldItem() == 112);
        // Editing the held item away from Griseous Orb must return Altered Forme.
        assert(giratina->setHeldItem(1));
        assert(giratina->heldItem() == 1 && giratina->form() == 0);
        assert(giratina->setHeldItem(112));
        assert(giratina->heldItem() == 112 && giratina->form() == 1);

        auto arceus = Pokemon::Pokemon4Mutable::fromEncrypted(
            makeEntity(493, 0x12345678u, 121), group);
        assert(arceus && arceus->formCount() == 18);
        assert(arceus->setForm(1));
        assert(arceus->form() == 1 && arceus->heldItem() == 303);
        const auto beforeCurse = arceus->encryptedBytes();
        assert(!arceus->setForm(9)); // Gen IV's unused ???-type slot has no Plate.
        assert(arceus->encryptedBytes() == beforeCurse);
        assert(arceus->setForm(10));
        assert(arceus->form() == 10 && arceus->heldItem() == 298);
        assert(arceus->setHeldItem(299));
        assert(arceus->heldItem() == 299 && arceus->form() == 11);
        assert(arceus->setHeldItem(1));
        assert(arceus->heldItem() == 1 && arceus->form() == 0);

        // Sky Forme is Party-only in Gen IV; a boxed PK4 must fail closed.
        auto boxedShaymin = Pokemon::Pokemon4Mutable::fromEncrypted(
            makeEntity(492, 0x12345678u, 30), group);
        assert(boxedShaymin && boxedShaymin->formCount() == 2);
        const auto boxBefore = boxedShaymin->encryptedBytes();
        assert(!boxedShaymin->setForm(1));
        assert(boxedShaymin->encryptedBytes() == boxBefore);

        auto partyShaymin = Pokemon::Pokemon4Mutable::fromEncrypted(
            makePartyEntity(492, 0x12345678u, 30), group);
        assert(partyShaymin && partyShaymin->isParty());
        assert(partyShaymin->setForm(1));
        assert(partyShaymin->form() == 1);
    }
}

void testBadInputFailsClosed() {
    std::vector<std::byte> empty(Encryption::SIZE_STORED4, std::byte{0});
    std::string error;
    assert(!Pokemon::Pokemon4Mutable::fromEncrypted(
        empty, Enums::GameVersion::PT, &error));
    assert(!error.empty());

    auto corrupt = makeEntity();
    corrupt[0x20] ^= std::byte{1};
    error.clear();
    assert(!Pokemon::Pokemon4Mutable::fromEncrypted(
        corrupt, Enums::GameVersion::PT, &error));
    assert(!error.empty());
}

} // namespace

int main() {
    testNoOpAndSimpleFields();
    testNicknamePreservesTrash();
    testPidCoupledEdits();
    testFixedGenderAndAbilitySlots();
    testShedinjaPartyHpRule();
    testSpeciesReconciliation();
    testOriginalTrainerIdentityEdits();
    testCatalogBackedFieldValidation();
    testLanguageTransitions();
    testPokerusModes();
    testExactBallSemantics();
    testExactMetLocationSemantics();
    testExactGameForms();
    testItemDrivenAndStorageFormRules();
    testNativeStoredCreateDraft();
    testBadInputFailsClosed();
    std::cout << "Gen IV mutable PK4 core PASS\n";
    return 0;
}
