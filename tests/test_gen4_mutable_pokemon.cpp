#include "Encryption/Encryption4.h"
#include "Pokemon/Experience.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
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
    uint16_t ability = 9) {
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
    d[0x5F] = std::byte{10};
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

    assert(editable->setHeldItem(200));
    assert(editable->setFriendship(123));
    assert(editable->setEV(0, 252));
    assert(editable->setIV(5, 31));
    assert(editable->setMove(1, 237));
    assert(editable->setPP(1, 12));
    assert(editable->setPPUps(1, 3));
    assert(editable->setPokerus(0x43));
    assert(editable->setBall(16));
    assert(editable->setMetLevel(42));
    assert(editable->setLevel(50));

    Pokemon::Pokemon4ReadOnly parsed(
        editable->encryptedBytes(), Enums::GameVersion::PT);
    assert(parsed.valid());
    assert(parsed.heldItem() == 200);
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

void testSpeciesAndTrainerFieldEdits() {
    std::string error;
    auto created = Pokemon::Pokemon4Mutable::createStored(
        393, Enums::GameVersion::PT, static_cast<uint8_t>(Enums::GameVersion::Pt),
        u"DAWN", 1111, 2222, 1, 2, 12, 0x13572468u, &error);
    assert(created && error.empty());
    const uint8_t beforeLevel = created->level();
    const uint8_t beforeNature = created->nature();
    const bool beforeShiny = created->shiny();

    assert(created->setSpecies(25));
    Pokemon::Pokemon4ReadOnly speciesChanged(created->encryptedBytes(), Enums::GameVersion::PT);
    assert(speciesChanged.valid() && speciesChanged.species() == 25);
    assert(speciesChanged.nickname() == u"Pikachu");
    assert(!speciesChanged.isNicknamed());
    assert(Pokemon::getLevelFromExp(speciesChanged.experience(), speciesChanged.personal().growthRate) == beforeLevel);
    assert(static_cast<uint8_t>(speciesChanged.pid() % 25u) == beforeNature);
    assert(created->shiny() == beforeShiny);

    assert(created->setNickname(u"SPARK"));
    assert(created->setSpecies(26));
    Pokemon::Pokemon4ReadOnly customName(created->encryptedBytes(), Enums::GameVersion::PT);
    assert(customName.valid() && customName.species() == 26);
    assert(customName.nickname() == u"SPARK" && customName.isNicknamed());

    const bool shinyBeforeIds = created->shiny();
    const uint8_t natureBeforeIds = created->nature();
    const uint8_t genderBeforeIds = created->gender();
    assert(created->setTID(3333));
    assert(created->setSID(4444));
    assert(created->setOriginalTrainerName(u"LUCAS"));
    assert(created->setOriginalTrainerGender(0));
    Pokemon::Pokemon4ReadOnly trainerChanged(created->encryptedBytes(), Enums::GameVersion::PT);
    assert(trainerChanged.valid());
    assert(trainerChanged.tid() == 3333 && trainerChanged.sid() == 4444);
    assert(trainerChanged.originalTrainerName() == u"LUCAS");
    assert(trainerChanged.originalTrainerGender() == 0);
    assert(created->shiny() == shinyBeforeIds);
    assert(created->nature() == natureBeforeIds);
    assert(created->gender() == genderBeforeIds);
}

void testStrictCreateFactory() {
    struct Case { Enums::GameVersion group; uint8_t origin; };
    const std::array<Case,3> cases{{
        {Enums::GameVersion::DP, static_cast<uint8_t>(Enums::GameVersion::D)},
        {Enums::GameVersion::PT, static_cast<uint8_t>(Enums::GameVersion::Pt)},
        {Enums::GameVersion::HGSS, static_cast<uint8_t>(Enums::GameVersion::HG)},
    }};
    for (const auto& tc : cases) {
        std::string error;
        auto created = Pokemon::Pokemon4Mutable::createStored(
            393, tc.group, tc.origin, u"ASH", 12345, 54321, 0, 2, 5,
            0x13572468u, &error);
        assert(created && error.empty() && !created->isParty());
        Pokemon::Pokemon4ReadOnly parsed(created->encryptedBytes(), tc.group);
        assert(parsed.valid() && !parsed.empty() && !parsed.isParty());
        assert(parsed.species() == 393);
        assert(parsed.tid() == 12345 && parsed.sid() == 54321);
        assert(parsed.language() == 2);
        assert(parsed.originVersion() == tc.origin);
        assert(parsed.originalTrainerName() == u"ASH");
        assert(parsed.nickname() == u"Piplup");
        assert(!parsed.isNicknamed());
        assert(parsed.metLevel() == 5);
        assert(parsed.originalTrainerGender() == 0);
        assert(Pokemon::getLevelFromExp(parsed.experience(), parsed.personal().growthRate) == 5);
        assert(parsed.friendship() == parsed.personal().baseFriendship);
        assert(parsed.ability() == parsed.personal().ability1 ||
               parsed.ability() == parsed.personal().ability2);
        if (tc.group == Enums::GameVersion::HGSS) assert(parsed.ballHGSS() == 4);
        else assert(parsed.ballDPPt() == 4);
        assert(parsed.moves() == std::array<uint16_t,4>{0,0,0,0});
        assert(parsed.pp() == std::array<uint8_t,4>{0,0,0,0});
        assert(parsed.ppUps() == std::array<uint8_t,4>{0,0,0,0});
    }

    std::string error;
    assert(!Pokemon::Pokemon4Mutable::createStored(
        494, Enums::GameVersion::PT, static_cast<uint8_t>(Enums::GameVersion::Pt),
        u"ASH", 1, 2, 0, 2, 5, 1, &error));
    assert(!error.empty());
    error.clear();
    assert(!Pokemon::Pokemon4Mutable::createStored(
        25, Enums::GameVersion::PT, static_cast<uint8_t>(Enums::GameVersion::Pt),
        u"ASH", 1, 2, 0, 9, 5, 1, &error)); // Chinese is not native to Gen IV.
    assert(!error.empty());
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
    testSpeciesAndTrainerFieldEdits();
    testStrictCreateFactory();
    testBadInputFailsClosed();
    std::cout << "Gen IV mutable PK4 core PASS\n";
    return 0;
}
