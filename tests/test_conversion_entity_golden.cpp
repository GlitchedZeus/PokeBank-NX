#include "Conversion/Convert.h"
#include "Conversion/Fidelity.h"
#include "Conversion/Gen3PidSearch.h"
#include "Conversion/RouteEvidence.h"
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption7LGPE.h"
#include "Encryption/Encryption8SWSH.h"
#include "Encryption/Encryption8LA.h"
#include "Encryption/Encryption8BDSP.h"
#include "Encryption/Encryption9SV.h"
#include "Encryption/Encryption9LZA.h"
#include "Names/SpeciesNames.h"
#include "Names/ItemPresence.h"
#include "Names/MoveInfo.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/LearnsetTable.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon7LGPE.h"
#include "Pokemon/Pokemon8SWSH.h"
#include "Pokemon/Pokemon8LA.h"
#include "Pokemon/Pokemon8BDSP.h"
#include "Pokemon/Pokemon9SV.h"
#include "Pokemon/Pokemon9LZA.h"
#include "Utils/SHA256.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <vector>

// Pokemon7LGPE.cpp uses the historical Trainer namespace species-name shim.
namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
}

namespace {
using Enums::GameVersion;

void wr16(std::vector<std::byte>& b, std::size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v & 0xFF);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}
void wr32(std::vector<std::byte>& b, std::size_t o, uint32_t v) {
    wr16(b, o, static_cast<uint16_t>(v));
    wr16(b, o + 2, static_cast<uint16_t>(v >> 16));
}
void wr64(std::span<std::byte> b, std::size_t o, uint64_t v) {
    for (int i = 0; i < 8; ++i) b[o + static_cast<std::size_t>(i)] = static_cast<std::byte>(v >> (8 * i));
}
uint64_t rd64(std::span<const std::byte> b, std::size_t o) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(static_cast<uint8_t>(b[o + static_cast<std::size_t>(i)])) << (8 * i);
    return v;
}
uint32_t rd32(std::span<const std::byte> b, std::size_t o) {
    return static_cast<uint32_t>(static_cast<uint8_t>(b[o])) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b[o + 1])) << 8) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b[o + 2])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b[o + 3])) << 24);
}
void wr32s(std::span<std::byte> b, std::size_t o, uint32_t v) {
    for (int i = 0; i < 4; ++i)
        b[o + static_cast<std::size_t>(i)] = static_cast<std::byte>(v >> (8 * i));
}

std::array<uint8_t, 32> hashBytes(std::span<const std::byte> bytes) {
    Utils::SHA256 sha;
    sha.update(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    std::array<uint8_t, 32> out{};
    sha.finalize(out.data());
    return out;
}
std::string hexHash(const std::array<uint8_t, 32>& hash) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const uint8_t b : hash) out << std::setw(2) << static_cast<unsigned>(b);
    return out.str();
}

std::vector<std::byte> nativeBytes(const Pokemon::Pokemon& pk) {
    const auto data = pk.getData();
    std::byte* encrypted = nullptr;
    switch (pk.getGameGroup()) {
        case GameVersion::FRLG:
            encrypted = Encryption::encryptArray3FRLG(data);
            break;
        case GameVersion::GG:
            encrypted = Encryption::encryptArray7LGPE(data, pk.encryptionConstant());
            break;
        case GameVersion::SWSH:
            encrypted = Encryption::encryptArray8SWSH(data, pk.encryptionConstant());
            break;
        case GameVersion::BDSP:
            encrypted = Encryption::encryptArray8BDSP(data, pk.encryptionConstant());
            break;
        case GameVersion::PLA:
            encrypted = Encryption::encryptArray8LA(data, pk.encryptionConstant());
            break;
        case GameVersion::SV:
            encrypted = Encryption::encryptArray9SV(data, pk.encryptionConstant());
            break;
        case GameVersion::ZA:
            encrypted = Encryption::encryptArray9LZA(data, pk.encryptionConstant());
            break;
        default:
            assert(false && "fixture serializer does not cover this group");
    }
    assert(encrypted);
    std::vector<std::byte> out(encrypted, encrypted + pk.getDataSize());
    delete[] encrypted;
    return out;
}

std::unique_ptr<Pokemon::Pokemon> reparse(GameVersion group, const std::vector<std::byte>& native) {
    const std::span<const std::byte> bytes(native.data(), native.size());
    switch (group) {
        case GameVersion::FRLG: return std::make_unique<Pokemon::Pokemon3FRLG>(bytes);
        case GameVersion::GG:   return std::make_unique<Pokemon::Pokemon7LGPE>(bytes);
        case GameVersion::SWSH: return std::make_unique<Pokemon::Pokemon8SWSH>(bytes);
        case GameVersion::BDSP: return std::make_unique<Pokemon::Pokemon8BDSP>(bytes);
        case GameVersion::PLA:  return std::make_unique<Pokemon::Pokemon8LA>(bytes);
        case GameVersion::SV:   return std::make_unique<Pokemon::Pokemon9SV>(bytes);
        case GameVersion::ZA:   return std::make_unique<Pokemon::Pokemon9LZA>(bytes);
        default: return nullptr;
    }
}

void assertSerializedReparse(const Pokemon::Pokemon& candidate) {
    assert(candidate.checksumValid());
    const auto bytes = nativeBytes(candidate);
    auto parsed = reparse(candidate.getGameGroup(), bytes);
    assert(parsed);
    assert(parsed->checksumValid());

    assert(parsed->speciesID() == candidate.speciesID());
    assert(parsed->form() == candidate.form());
    assert(parsed->exp() == candidate.exp());
    assert(parsed->level() == candidate.level());
    assert(parsed->pid() == candidate.pid());
    assert(parsed->encryptionConstant() == candidate.encryptionConstant());
    assert(parsed->gender() == candidate.gender());
    assert(parsed->nature() == candidate.nature());
    assert(parsed->statNature() == candidate.statNature());
    assert(parsed->ability() == candidate.ability());
    assert(parsed->abilityNumber() == candidate.abilityNumber());
    assert(parsed->language() == candidate.language());
    assert(parsed->nickname() == candidate.nickname());
    assert(parsed->otName() == candidate.otName());
    assert(parsed->tid16() == candidate.tid16());
    assert(parsed->sid16() == candidate.sid16());
    assert(parsed->originGame() == candidate.originGame());
    assert(parsed->metLevel() == candidate.metLevel());
    assert(parsed->heldItem() == candidate.heldItem());
    assert(parsed->ball() == candidate.ball());

    assert(parsed->ivHP() == candidate.ivHP());
    assert(parsed->ivATK() == candidate.ivATK());
    assert(parsed->ivDEF() == candidate.ivDEF());
    assert(parsed->ivSPE() == candidate.ivSPE());
    assert(parsed->ivSPA() == candidate.ivSPA());
    assert(parsed->ivSPD() == candidate.ivSPD());
    assert(parsed->evHP() == candidate.evHP());
    assert(parsed->evATK() == candidate.evATK());
    assert(parsed->evDEF() == candidate.evDEF());
    assert(parsed->evSPE() == candidate.evSPE());
    assert(parsed->evSPA() == candidate.evSPA());
    assert(parsed->evSPD() == candidate.evSPD());
    for (int i = 0; i < 4; ++i) {
        assert(parsed->move(i) == candidate.move(i));
        assert(parsed->movePP(i) == candidate.movePP(i));
        assert(parsed->movePPUps(i) == candidate.movePPUps(i));
    }
}

std::unique_ptr<Pokemon::Pokemon3FRLG> blankPK3(uint32_t pid, uint32_t id32) {
    std::vector<std::byte> dec(Encryption::SIZE_PARTY3_FRLG, std::byte{0});
    wr32(dec, 0x00, pid);
    wr32(dec, 0x04, id32);
    wr16(dec, 0x1C, Encryption::checksum3FRLG(dec));
    std::byte* enc = Encryption::encryptArray3FRLG(dec);
    auto out = std::make_unique<Pokemon::Pokemon3FRLG>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

std::unique_ptr<Pokemon::Pokemon7LGPE> blankGG(uint32_t ec = 0x0A0B0C0Du) {
    std::vector<std::byte> dec(0x104, std::byte{0});
    wr32(dec, 0x00, ec);
    std::byte* enc = Encryption::encryptArray7LGPE(dec, ec);
    auto out = std::make_unique<Pokemon::Pokemon7LGPE>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

std::unique_ptr<Pokemon::Pokemon8SWSH> blankSWSH(uint32_t ec = 0x10203040u) {
    std::vector<std::byte> dec(Encryption::SIZE_PARTY8_SWSH, std::byte{0});
    wr32(dec, 0x00, ec);
    std::byte* enc = Encryption::encryptArray8SWSH(dec, ec);
    auto out = std::make_unique<Pokemon::Pokemon8SWSH>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

std::unique_ptr<Pokemon::Pokemon8BDSP> blankBDSP(uint32_t ec = 0x21314151u) {
    constexpr std::size_t PARTY = 0x158;
    std::vector<std::byte> dec(PARTY, std::byte{0});
    wr32(dec, 0x00, ec);
    std::byte* enc = Encryption::encryptArray8BDSP(dec, ec);
    auto out = std::make_unique<Pokemon::Pokemon8BDSP>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

std::unique_ptr<Pokemon::Pokemon8LA> blankPLA(uint32_t ec = 0x55667788u) {
    std::vector<std::byte> dec(Encryption::SIZE_PARTY8_LA, std::byte{0});
    wr32(dec, 0x00, ec);
    std::byte* enc = Encryption::encryptArray8LA(dec, ec);
    auto out = std::make_unique<Pokemon::Pokemon8LA>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

std::unique_ptr<Pokemon::Pokemon9SV> blankSV(uint32_t ec = 0x31415926u) {
    std::vector<std::byte> dec(Encryption::SIZE_PARTY9_SV, std::byte{0});
    wr32(dec, 0x00, ec);
    std::byte* enc = Encryption::encryptArray9SV(dec, ec);
    auto out = std::make_unique<Pokemon::Pokemon9SV>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

std::unique_ptr<Pokemon::Pokemon9LZA> blankZA(uint32_t ec = 0x27182818u) {
    std::vector<std::byte> dec(Encryption::SIZE_PARTY9_LZA, std::byte{0});
    wr32(dec, 0x00, ec);
    std::byte* enc = Encryption::encryptArray9LZA(dec, ec);
    auto out = std::make_unique<Pokemon::Pokemon9LZA>(
        std::span<const std::byte>(enc, dec.size()));
    delete[] enc;
    assert(out->checksumValid());
    return out;
}

void configurePK3(Pokemon::Pokemon3FRLG& pk, uint16_t species, uint32_t pid, uint32_t id32,
                  const std::u16string& nickname = u"PIKACHU") {
    pk.setSpecies(species);
    pk.setId32(id32);
    pk.setLanguage(2);
    pk.setOriginGame(static_cast<uint8_t>(GameVersion::FR));
    pk.setOTName(u"ASH");
    pk.setNickname(nickname);
    pk.setBall(4);
    pk.setMetLevel(5);
    pk.setMetLocation(1);
    pk.setLevel(25);
    for (int i = 0; i < 6; ++i) {
        pk.setIV(i, 20);
        pk.setEV(i, 0);
    }
    pk.setPID(pid); // last: fixtures intentionally exercise exact PID-derived traits
    pk.refreshChecksum();
    assert(pk.checksumValid());
}

void configureModern(Pokemon::Pokemon& pk, uint16_t species, uint8_t form,
                     uint32_t pid, uint32_t id32, uint8_t origin,
                     const std::u16string& nickname, bool nicknamed) {
    pk.setSpecies(species);
    pk.setForm(form);
    pk.setId32(id32);
    pk.setPID(pid);
    pk.setNature(static_cast<uint8_t>(pid % 25));
    pk.setStatNature(static_cast<uint8_t>(pid % 25));
    const auto& pi = Pokemon::getPersonalInfo(species, form);
    pk.setGender(Conversion::Gen3PidSearch::genderForPid(pid, pi.genderRatio));
    pk.setAbility(pi.ability1);
    pk.setAbilityNumber(1);
    pk.setLanguage(2);
    pk.setOriginGame(origin);
    pk.setOTName(u"ASH");
    pk.setNickname(nickname);
    pk.setIsNicknamed(nicknamed);
    pk.setBall(4);
    pk.setMetLevel(5);
    pk.setMetLocation(1);
    pk.setLevel(25);
    for (int i = 0; i < 6; ++i) {
        pk.setIV(i, 20);
        pk.setEV(i, 0);
    }
    pk.refreshChecksum();
    assert(pk.checksumValid());
}

void proveSourceUnchanged(const Pokemon::Pokemon& source,
                          const std::vector<std::byte>& before,
                          const std::array<uint8_t, 32>& beforeHash) {
    const auto after = nativeBytes(source);
    assert(after == before);
    assert(hashBytes(after) == beforeHash);
}
}

int main() {
    using namespace Conversion;

    // F05: real PK3 -> PK8 -> PK3 entity paths across the Gen III/modern shiny threshold.
    {
        struct ShinyCase { uint32_t xorValue; bool shiny; bool adjusted; const char* name; };
        const ShinyCase cases[] = {
            {0u,  true,  false, "xor-0-shiny"},
            {8u,  false, true,  "xor-8-boundary"},
            {16u, false, false, "xor-16-nonshiny"},
        };
        for (const auto& c : cases) {
            const uint32_t id32 = 0x12345678u;
            const uint32_t pid = id32 ^ c.xorValue;
            auto source = blankPK3(pid, id32);
            configurePK3(*source, 25, pid, id32);
            assert(source->isShiny(id32, "") == c.shiny);

            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report upReport;
            Result result = Result::Unsupported;
            auto modern = convert(*source, GameVersion::SWSH, result,
                                  static_cast<uint8_t>(GameVersion::SW), &upReport);
            assert(result == Result::Ok && modern);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(modern->speciesID() == source->speciesID());
            assert(modern->nature() == source->nature());
            assert(modern->gender() == source->gender());
            assert(modern->ability() == source->ability());
            assert(modern->isShiny(id32, "") == c.shiny);
            assert(upReport.hasAdaptation(Adaptation::PidAdjustedForShinyThreshold) == c.adjusted);
            assert(modern->pid() == (c.adjusted ? (pid ^ 0x80000000u) : pid));
            assert(modern->encryptionConstant() == pid);
            assertSerializedReparse(*modern);

            const auto modernBefore = nativeBytes(*modern);
            const auto modernHash = hashBytes(modernBefore);
            Report downReport;
            auto roundTrip = convert(*modern, GameVersion::FRLG, result,
                                     static_cast<uint8_t>(GameVersion::FR), &downReport);
            assert(result == Result::Ok && roundTrip);
            proveSourceUnchanged(*modern, modernBefore, modernHash);
            assert(roundTrip->speciesID() == source->speciesID());
            assert(roundTrip->nature() == source->nature());
            assert(roundTrip->gender() == source->gender());
            assert(roundTrip->ability() == source->ability());
            assert(roundTrip->isShiny(id32, "") == c.shiny);
            assertSerializedReparse(*roundTrip);

            std::cout << "fixture f05-pk3-swsh-" << c.name
                      << " source-sha256=" << hexHash(sourceHash) << "\n";
        }
    }

    // F06: multiple real modern Unown forms -> PK3 PID-derived form -> modern round trip.
    {
        for (const int wantedForm : {0, 1, 13, 27}) {
            const uint32_t pid = Gen3PidSearch::stampUnownForm(
                0x13579BDFu + static_cast<uint32_t>(wantedForm) * 0x10101u, wantedForm);
            const uint32_t id32 = pid ^ 0x00000100u; // non-shiny under both thresholds
            auto source = blankBDSP(0x21314151u + static_cast<uint32_t>(wantedForm));
            configureModern(*source, 201, static_cast<uint8_t>(wantedForm), pid, id32,
                            static_cast<uint8_t>(GameVersion::BD), u"UNOWN", false);
            assert(source->form() == wantedForm);

            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report downReport;
            Result result = Result::Unsupported;
            auto pk3 = convert(*source, GameVersion::FRLG, result,
                               static_cast<uint8_t>(GameVersion::FR), &downReport);
            assert(result == Result::Ok && pk3);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pk3->speciesID() == 201);
            assert(pk3->form() == wantedForm);
            assert(pk3->nature() == source->nature());
            assert(pk3->gender() == 2);
            assert(pk3->isShiny(id32, "") == source->isShiny(id32, ""));
            assert(downReport.hasLoss(Loss::OriginGameRestamped));
            assertSerializedReparse(*pk3);

            const auto pk3Before = nativeBytes(*pk3);
            const auto pk3Hash = hashBytes(pk3Before);
            Report upReport;
            auto roundTrip = convert(*pk3, GameVersion::BDSP, result,
                                     static_cast<uint8_t>(GameVersion::BD), &upReport);
            assert(result == Result::Ok && roundTrip);
            proveSourceUnchanged(*pk3, pk3Before, pk3Hash);
            assert(roundTrip->speciesID() == 201);
            assert(roundTrip->form() == wantedForm);
            assert(roundTrip->nature() == pk3->nature());
            assert(roundTrip->isShiny(id32, "") == pk3->isShiny(id32, ""));
            assertSerializedReparse(*roundTrip);

            std::cout << "fixture f06-bdsp-unown" << wantedForm
                      << "-pk3 source-sha256=" << hexHash(sourceHash) << "\n";
        }
    }

    // F08: production ability ID + slot/number semantics, including duplicate and hidden ability.
    {
        // Ralts has distinct Gen III normal abilities (Synchronize / Trace): both slots must round-trip.
        const auto& raltsG3 = Pokemon::getPersonalInfoG3(280);
        assert(raltsG3.ability1 != 0 && raltsG3.ability2 != 0 && raltsG3.ability1 != raltsG3.ability2);
        for (const uint8_t slot : {uint8_t{1}, uint8_t{2}}) {
            const uint32_t pid = 0x2468ACE0u + slot;
            const uint32_t id32 = pid ^ 0x00000100u;
            auto source = blankSWSH(0x10203040u + slot);
            configureModern(*source, 280, 0, pid, id32,
                            static_cast<uint8_t>(GameVersion::SW), u"RALTS", false);
            const uint16_t ability = slot == 1 ? raltsG3.ability1 : raltsG3.ability2;
            source->setAbility(ability);
            source->setAbilityNumber(slot);
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);

            Report downReport;
            Result result = Result::Unsupported;
            auto pk3 = convert(*source, GameVersion::FRLG, result,
                               static_cast<uint8_t>(GameVersion::FR), &downReport);
            assert(result == Result::Ok && pk3);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pk3->ability() == ability);
            assertSerializedReparse(*pk3);

            const auto pk3Before = nativeBytes(*pk3);
            const auto pk3Hash = hashBytes(pk3Before);
            Report upReport;
            auto roundTrip = convert(*pk3, GameVersion::SWSH, result,
                                     static_cast<uint8_t>(GameVersion::SW), &upReport);
            assert(result == Result::Ok && roundTrip);
            proveSourceUnchanged(*pk3, pk3Before, pk3Hash);
            assert(roundTrip->ability() == ability);
            assert(roundTrip->abilityNumber() == slot);
            assertSerializedReparse(*roundTrip);

            std::cout << "fixture f08-ralts-slot" << static_cast<unsigned>(slot)
                      << "-pk3 source-sha256=" << hexHash(sourceHash) << "\n";
        }

        // Pikachu has the same Gen III ability in both native slots. Slot 2 can be represented in PK3,
        // but a later modern representation normalizes the redundant selector to slot 1 and declares it.
        const auto& pikachuG3 = Pokemon::getPersonalInfoG3(25);
        assert(pikachuG3.ability1 == pikachuG3.ability2 && pikachuG3.ability1 != 0);
        {
            const uint32_t pid = 0x2468ACF2u;
            const uint32_t id32 = pid ^ 0x00000100u;
            auto source = blankSWSH(0x10203052u);
            configureModern(*source, 25, 0, pid, id32,
                            static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
            source->setAbility(pikachuG3.ability2);
            source->setAbilityNumber(2);
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report downReport;
            Result result = Result::Unsupported;
            auto pk3 = convert(*source, GameVersion::FRLG, result,
                               static_cast<uint8_t>(GameVersion::FR), &downReport);
            assert(result == Result::Ok && pk3);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pk3->ability() == pikachuG3.ability1);
            assertSerializedReparse(*pk3);

            const auto pk3Before = nativeBytes(*pk3);
            const auto pk3Hash = hashBytes(pk3Before);
            Report upReport;
            auto roundTrip = convert(*pk3, GameVersion::SWSH, result,
                                     static_cast<uint8_t>(GameVersion::SW), &upReport);
            assert(result == Result::Ok && roundTrip);
            proveSourceUnchanged(*pk3, pk3Before, pk3Hash);
            assert(roundTrip->ability() == pikachuG3.ability1);
            assert(roundTrip->abilityNumber() == 1);
            assert(upReport.hasLoss(Loss::AbilitySlotNormalized));
            assertSerializedReparse(*roundTrip);
            std::cout << "fixture f08-pikachu-duplicate-slot2-pk3 source-sha256="
                      << hexHash(sourceHash) << "\n";
        }

        // Hidden ability has no Gen III representation: fail closed and keep exact source bytes.
        {
            const uint32_t pid = 0x2468ACE0u;
            const uint32_t id32 = pid ^ 0x00000100u;
            auto source = blankSWSH();
            configureModern(*source, 25, 0, pid, id32,
                            static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
            const auto& pi = Pokemon::getPersonalInfo(25, 0);
            source->setAbility(pi.abilityHidden);
            source->setAbilityNumber(4);
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);

            Report report;
            Result result = Result::Ok;
            auto failed = convert(*source, GameVersion::FRLG, result,
                                  static_cast<uint8_t>(GameVersion::FR), &report);
            assert(!failed);
            assert(result == Result::AbilityNotRepresentable);
            proveSourceUnchanged(*source, before, sourceHash);

            std::cout << "fixture f08-swsh-hidden-pk3-fail source-sha256="
                      << hexHash(sourceHash) << "\n";
        }
    }

    // F09: S/V -> Z-A drops Tera semantics explicitly and reparses as a valid PA9 entity.
    {
        const uint32_t pid = 0x11223344u;
        const uint32_t id32 = pid ^ 0x00000100u;
        auto source = blankSV();
        configureModern(*source, 25, 0, pid, id32,
                        static_cast<uint8_t>(GameVersion::SL), u"PIKACHU", false);
        source->getData()[0x94] = std::byte{12};
        source->getData()[0x95] = std::byte{12};
        source->refreshChecksum();

        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto za = convert(*source, GameVersion::ZA, result,
                          static_cast<uint8_t>(GameVersion::ZA), &report);
        assert(result == Result::Ok && za);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::TeraDataDropped));
        assert(static_cast<uint8_t>(za->getData()[0x94]) == 0);
        assert(static_cast<uint8_t>(za->getData()[0x95]) == 0);
        assertSerializedReparse(*za);

        std::cout << "fixture f09-sv-za-tera source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // F09 reverse: Z-A Alpha/divergent state is declared lost; S/V receives a target-native Tera.
    {
        const uint32_t pid = 0x55667788u;
        const uint32_t id32 = pid ^ 0x00000100u;
        auto source = blankZA();
        configureModern(*source, 25, 0, pid, id32,
                        static_cast<uint8_t>(GameVersion::ZA), u"PIKACHU", false);
        source->getData()[0x23] = std::byte{1}; // Alpha
        source->getData()[0x94] = std::byte{1}; // Z-A Plus-side divergent field
        source->refreshChecksum();

        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto sv = convert(*source, GameVersion::SV, result,
                          static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && sv);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::ZAAlphaDropped));
        assert(report.hasLoss(Loss::DivergentGameDataDropped));
        assert(report.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
        assert(static_cast<uint8_t>(sv->getData()[0x23]) == 0);
        assert(static_cast<uint8_t>(sv->getData()[0x94]) != 0); // source Plus byte was discarded; target Tera synthesized
        assertSerializedReparse(*sv);

        std::cout << "fixture f09-za-sv-alpha-plus source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // F09 hub path: Z-A Plus-side divergent data must also be declared when normalizing through PK8.
    {
        const uint32_t pid = 0x66778899u;
        const uint32_t id32 = pid ^ 0x00000100u;
        auto source = blankZA(0x27182819u);
        configureModern(*source, 25, 0, pid, id32,
                        static_cast<uint8_t>(GameVersion::ZA), u"PIKACHU", false);
        source->getData()[0x94] = std::byte{1};
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);

        Report report;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::DivergentGameDataDropped));
        assertSerializedReparse(*swsh);
        std::cout << "fixture f09-za-swsh-plus-hub source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // F10: real PK3 -> PK8 EV policy, including legacy 253/254/255 bytes.
    {
        const uint32_t id32 = 0x33445566u;
        const uint32_t pid = id32 ^ 0x00000100u;
        auto source = blankPK3(pid, id32);
        configurePK3(*source, 25, pid, id32);
        const uint8_t values[6] = {252, 253, 254, 255, 1, 0};
        for (int i = 0; i < 6; ++i) source->setEV(i, values[i]);

        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto modern = convert(*source, GameVersion::SWSH, result,
                              static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && modern);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::Gen3EVClamped));
        assert(modern->evHP() == 252);
        assert(modern->evATK() == 252);
        assert(modern->evDEF() == 252);
        assert(modern->evSPE() == 252);
        assert(modern->evSPA() == 1);
        assert(modern->evSPD() == 0);
        assertSerializedReparse(*modern);

        std::cout << "fixture f10-pk3-ev-252-255 source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // F10 reverse: modern raw EV bytes 252/253/254/255 are representable by PK3 and are not
    // silently clamped on down-conversion. Each fixture uses one nonzero stat so the destination total
    // remains within the Gen III 510-EV gameplay limit; this distinguishes format/game validity from
    // the later-generation per-stat 252 legality convention.
    {
        for (const uint8_t value : {uint8_t{252}, uint8_t{253}, uint8_t{254}, uint8_t{255}}) {
            const uint32_t pid = 0x44556600u + value;
            const uint32_t id32 = pid ^ 0x00000100u;
            auto source = blankSWSH(0x40506000u + value);
            configureModern(*source, 25, 0, pid, id32,
                            static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
            source->getData()[0x26] = static_cast<std::byte>(value); // raw HP EV byte in PK8 layout
            for (int i = 1; i < 6; ++i) source->getData()[0x26 + i] = std::byte{0};
            source->refreshChecksum();
            assert(source->evHP() == value);

            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto pk3 = convert(*source, GameVersion::FRLG, result,
                               static_cast<uint8_t>(GameVersion::FR), &report);
            assert(result == Result::Ok && pk3);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pk3->evHP() == value);
            assert(pk3->evATK() == 0 && pk3->evDEF() == 0 && pk3->evSPE() == 0
                   && pk3->evSPA() == 0 && pk3->evSPD() == 0);
            assert(!report.hasLoss(Loss::Gen3EVClamped));
            assertSerializedReparse(*pk3);

            std::cout << "fixture f10-swsh-raw-ev-" << static_cast<unsigned>(value)
                      << "-pk3 source-sha256=" << hexHash(sourceHash) << "\n";
        }
    }

    // F11/F13: representable custom nickname survives modern -> PK3 -> modern; modern origin
    // is restamped only in the PK3 representation and explicitly declared as provenance loss.
    {
        const uint32_t pid = 0x34567890u;
        const uint32_t id32 = pid ^ 0x00000100u;
        auto source = blankSWSH();
        configureModern(*source, 25, 0, pid, id32,
                        static_cast<uint8_t>(GameVersion::SW), u"SPARKY", true);

        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto pk3 = convert(*source, GameVersion::FRLG, result,
                           static_cast<uint8_t>(GameVersion::FR), &report);
        assert(result == Result::Ok && pk3);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(pk3->nickname() == u"SPARKY");
        assert(pk3->otName() == u"ASH");
        assert(pk3->language() == 2);
        assert(pk3->originGame() == static_cast<uint8_t>(GameVersion::FR));
        assert(report.sourceOriginVersion == static_cast<uint8_t>(GameVersion::SW));
        assert(report.destinationEntityOriginVersion == static_cast<uint8_t>(GameVersion::FR));
        assert(report.hasLoss(Loss::OriginGameRestamped));
        assertSerializedReparse(*pk3);

        const auto pk3Before = nativeBytes(*pk3);
        const auto pk3Hash = hashBytes(pk3Before);
        Report roundReport;
        auto roundTrip = convert(*pk3, GameVersion::SWSH, result,
                                 static_cast<uint8_t>(GameVersion::SW), &roundReport);
        assert(result == Result::Ok && roundTrip);
        proveSourceUnchanged(*pk3, pk3Before, pk3Hash);
        assert(roundTrip->nickname() == u"SPARKY");
        assert(roundTrip->isNicknamed());
        assert(roundTrip->language() == 2);
        assert(roundTrip->otName() == u"ASH");
        assertSerializedReparse(*roundTrip);

        std::cout << "fixture f11-f13-swsh-pk3-sparky source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // F11 explicit failures: unsupported text and language never produce a lossy PK3 candidate.
    {
        const uint32_t pid = 0x456789A0u;
        const uint32_t id32 = pid ^ 0x00000100u;
        auto textSource = blankSWSH();
        configureModern(*textSource, 25, 0, pid, id32,
                        static_cast<uint8_t>(GameVersion::SW), u"Ω", true);
        auto before = nativeBytes(*textSource);
        const auto hash = hashBytes(before);
        Report report;
        Result result = Result::Ok;
        auto failed = convert(*textSource, GameVersion::FRLG, result,
                              static_cast<uint8_t>(GameVersion::FR), &report);
        assert(!failed && result == Result::TextNotRepresentable);
        proveSourceUnchanged(*textSource, before, hash);

        auto languageSource = blankSWSH(0x50607080u);
        configureModern(*languageSource, 25, 0, pid, id32,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        languageSource->setLanguage(1); // Japanese requires a separate Gen III table not implemented here.
        before = nativeBytes(*languageSource);
        const auto languageHash = hashBytes(before);
        failed = convert(*languageSource, GameVersion::FRLG, result,
                         static_cast<uint8_t>(GameVersion::FR), &report);
        assert(!failed && result == Result::LanguageNotRepresentable);
        proveSourceUnchanged(*languageSource, before, languageHash);
    }

    // Route-corpus regression: modern HOME tracker bytes must never disappear silently when the
    // destination format (PB7 / PK3) has no tracker field. The loss bit is intentionally referenced
    // by its next stable mask value so this test compiles before the production enum is extended.
    {
        const uint64_t tracker = 0x1122334455667788ULL;
        auto source = blankSWSH(0x70000001u);
        configureModern(*source, 25, 0, 0x1234ABCDu, 0x1234AACDu,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        wr64(source->getData(), 0x135, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);

        Report ggReport;
        Result result = Result::Unsupported;
        auto gg = convert(*source, GameVersion::GG, result,
                          static_cast<uint8_t>(GameVersion::GP), &ggReport);
        assert(result == Result::Ok && gg);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(ggReport.hasLoss(Loss::HomeTrackerDropped));
        assertSerializedReparse(*gg);

        Report g3Report;
        auto pk3 = convert(*source, GameVersion::FRLG, result,
                           static_cast<uint8_t>(GameVersion::FR), &g3Report);
        assert(result == Result::Ok && pk3);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(g3Report.hasLoss(Loss::HomeTrackerDropped));
        assertSerializedReparse(*pk3);

        std::cout << "fixture route-home-tracker-to-trackerless source-sha256="
                  << hexHash(sourceHash) << "\n";
    }

    // Route-corpus regression: Gen III ribbon bits other than Fateful Encounter are not represented
    // by the current PK3 -> modern remap, so they must be declared rather than silently erased.
    {
        auto source = blankPK3(0x2468ACE0u, 0x2468ADE0u);
        configurePK3(*source, 25, 0x2468ACE0u, 0x2468ADE0u);
        source->getData()[0x4C] = std::byte{0x02}; // non-fateful Gen III ribbon bit
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::RibbonDataDropped));
        assertSerializedReparse(*swsh);
        std::cout << "fixture route-pk3-ribbon-loss source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // Route corpus foundation: LGPE real entities participate in encrypted serialization/reparse.
    // This fixture also covers an Alolan form, AV reset, event/fateful state, and round-trip form custody.
    {
        auto source = blankGG();
        configureModern(*source, 37, 1, 0x10293847u, 0x10293947u,
                        static_cast<uint8_t>(GameVersion::GP), u"VULPIX", false);
        source->setAV(0, 120);
        source->setFatefulEncounter(true);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report upReport;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &upReport);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(swsh->speciesID() == 37 && swsh->form() == 1);
        assert(swsh->isFatefulEncounter());
        assert(swsh->evHP() == 0);
        assert(upReport.hasLoss(Loss::StatTrainingReset));
        assert(upReport.hasLoss(Loss::RibbonDataDropped));
        assertSerializedReparse(*swsh);

        const auto swBefore = nativeBytes(*swsh);
        const auto swHash = hashBytes(swBefore);
        Report downReport;
        auto roundTrip = convert(*swsh, GameVersion::GG, result,
                                 static_cast<uint8_t>(GameVersion::GP), &downReport);
        assert(result == Result::Ok && roundTrip);
        proveSourceUnchanged(*swsh, swBefore, swHash);
        assert(roundTrip->speciesID() == 37 && roundTrip->form() == 1);
        assert(roundTrip->isFatefulEncounter());
        assert(roundTrip->avHP() == 0);
        assert(downReport.hasLoss(Loss::StatTrainingReset));
        assert(downReport.hasLoss(Loss::RibbonDataDropped));
        assertSerializedReparse(*roundTrip);
        std::cout << "fixture route-lgpe-alolan-vulpix-swsh source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // LGPE destination semantics: held items and modern EV training are intentionally unavailable.
    {
        auto source = blankSWSH(0x70000002u);
        configureModern(*source, 25, 0, 0x22334455u, 0x22334555u,
                        static_cast<uint8_t>(GameVersion::SW), u"SPARKY", true);
        source->setHeldItem(1);
        source->setEV(0, 100);
        source->getData()[0x40] = std::byte{0x04};
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto gg = convert(*source, GameVersion::GG, result,
                          static_cast<uint8_t>(GameVersion::GP), &report);
        assert(result == Result::Ok && gg);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(gg->heldItem() == 0);
        assert(gg->evHP() == 0 && gg->avHP() == 0);
        assert(report.hasLoss(Loss::HeldItemDropped));
        assert(report.hasLoss(Loss::StatTrainingReset));
        assert(report.hasLoss(Loss::MarkDataDropped));
        assertSerializedReparse(*gg);
        std::cout << "fixture route-swsh-lgpe-helditem-stat-reset source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // LGPE mythical/genderless route coverage.
    {
        auto source = blankGG(0x0A0B0C0Eu);
        configureModern(*source, 809, 0, 0x33445566u, 0x33445566u,
                        static_cast<uint8_t>(GameVersion::GP), u"MELMETAL", false);
        assert(source->isShiny(source->id32(), ""));
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(swsh->speciesID() == 809 && swsh->gender() == 2);
        assert(swsh->isShiny(swsh->id32(), ""));
        assertSerializedReparse(*swsh);
        std::cout << "fixture route-lgpe-melmetal-swsh source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // PLA -> PK8: HOME tracker, common ribbons/marks and Fateful Encounter survive; Alpha/PLA-only
    // state is explicitly loss-classified instead of leaking overlapping bytes into PK8.
    {
        const uint64_t tracker = 0x8877665544332211ULL;
        auto source = blankPLA();
        configureModern(*source, 25, 0, 0x44556677u, 0x44556677u,
                        static_cast<uint8_t>(GameVersion::PLA), u"ALPHA", true);
        assert(source->isShiny(source->id32(), ""));
        source->setFatefulEncounter(true);
        uint16_t commonMove = 0;
        uint16_t swordOnlyMove = 0;
        for (uint16_t move = 1; move <= Pokemon::LEARN_MAX_MOVE_ID; ++move) {
            const bool sw = Pokemon::isLearnable(25, 0, GameVersion::SWSH, move);
            const bool sv = Pokemon::isLearnable(25, 0, GameVersion::SV, move);
            if (sw && sv && commonMove == 0) commonMove = move;
            if (sw && !sv && swordOnlyMove == 0) swordOnlyMove = move;
            if (commonMove != 0 && swordOnlyMove != 0) break;
        }
        assert(commonMove != 0 && swordOnlyMove != 0);
        source->setMove(0, commonMove);
        source->setMovePP(0, 1);
        source->setMovePPUps(0, 0);
        source->setMove(1, swordOnlyMove);
        source->setMovePP(1, 1);
        source->setMovePPUps(1, 0);
        auto d = source->getData();
        d[0x16] = static_cast<std::byte>(static_cast<uint8_t>(d[0x16]) | 0x20); // Alpha
        d[0x34] = std::byte{0x02}; // common ribbon bit
        d[0x40] = std::byte{0x04}; // common mark bit
        d[0xF8] = std::byte{1};    // affixed ribbon index
        wr64(d, 0x14D, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::PLAExclusiveDataDropped));
        assert(swsh->isShiny(swsh->id32(), ""));
        assert((static_cast<uint8_t>(swsh->getData()[0x16]) & 0x20) == 0);
        assert(static_cast<uint8_t>(swsh->getData()[0x34]) == 0x02);
        assert(static_cast<uint8_t>(swsh->getData()[0x40]) == 0x04);
        assert(static_cast<uint8_t>(swsh->getData()[0xE8]) == 1);
        assert(rd64(swsh->getData(), 0x135) == tracker);
        assert(swsh->isFatefulEncounter());
        assertSerializedReparse(*swsh);
        std::cout << "fixture route-pla-alpha-ribbon-mark-tracker-swsh source-sha256="
                  << hexHash(sourceHash) << "\n";
    }

    // PK8 -> PLA: tracker/ribbons/marks survive while unsupported held items are explicitly dropped.
    {
        const uint64_t tracker = 0x0102030405060708ULL;
        auto source = blankSWSH(0x70000003u);
        configureModern(*source, 25, 0, 0x55667788u, 0x55667688u,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        source->setHeldItem(1);
        auto d = source->getData();
        d[0x34] = std::byte{0x02};
        d[0x40] = std::byte{0x04};
        d[0xE8] = std::byte{1};
        wr64(d, 0x135, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto pla = convert(*source, GameVersion::PLA, result,
                           static_cast<uint8_t>(GameVersion::PLA), &report);
        assert(result == Result::Ok && pla);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(pla->heldItem() == 0);
        assert(report.hasLoss(Loss::HeldItemDropped));
        assert(static_cast<uint8_t>(pla->getData()[0x34]) == 0x02);
        assert(static_cast<uint8_t>(pla->getData()[0x40]) == 0x04);
        assert(static_cast<uint8_t>(pla->getData()[0xF8]) == 1);
        assert(rd64(pla->getData(), 0x14D) == tracker);
        assertSerializedReparse(*pla);
        std::cout << "fixture route-swsh-pla-ribbon-mark-tracker source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // A Hisuian form absent from Sword must fail closed instead of flattening to the base form.
    {
        auto source = blankPLA(0x55667789u);
        configureModern(*source, 570, 1, 0x66778899u, 0x66778999u,
                        static_cast<uint8_t>(GameVersion::PLA), u"ZORUA", false);
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Ok;
        auto failed = convert(*source, GameVersion::SWSH, result,
                              static_cast<uint8_t>(GameVersion::SW), &report);
        assert(!failed && result == Result::NotInDex);
        proveSourceUnchanged(*source, before, sourceHash);
        std::cout << "fixture route-pla-hisuian-zorua-swsh-fail source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // PK8 <-> PK9 route coverage: HOME tracker, common ribbon/mark and fateful metadata survive,
    // while destination-native Tera is synthesized entering S/V and explicitly lost on the return.
    {
        const uint64_t tracker = 0xA1A2A3A4A5A6A7A8ULL;
        auto source = blankSWSH(0x70000004u);
        configureModern(*source, 25, 0, 0x778899AAu, 0x778898AAu,
                        static_cast<uint8_t>(GameVersion::SW), u"EVENT", true);
        source->setFatefulEncounter(true);
        uint16_t commonMove = 0;
        uint16_t swordOnlyMove = 0;
        for (uint16_t move = 1; move <= Pokemon::LEARN_MAX_MOVE_ID; ++move) {
            const bool sw = Pokemon::isLearnable(25, 0, GameVersion::SWSH, move);
            const bool sv = Pokemon::isLearnable(25, 0, GameVersion::SV, move);
            if (sw && sv && commonMove == 0) commonMove = move;
            if (sw && !sv && swordOnlyMove == 0) swordOnlyMove = move;
            if (commonMove != 0 && swordOnlyMove != 0) break;
        }
        assert(commonMove != 0 && swordOnlyMove != 0);
        source->setMove(0, commonMove);
        source->setMovePP(0, 1);
        source->setMovePPUps(0, 0);
        source->setMove(1, swordOnlyMove);
        source->setMovePP(1, 1);
        source->setMovePPUps(1, 0);
        auto d = source->getData();
        d[0x34] = std::byte{0x02};
        d[0x40] = std::byte{0x04};
        d[0xE8] = std::byte{1};
        wr64(d, 0x135, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report toSV;
        Result result = Result::Unsupported;
        auto sv = convert(*source, GameVersion::SV, result,
                          static_cast<uint8_t>(GameVersion::SL), &toSV);
        assert(result == Result::Ok && sv);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(toSV.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
        assert(rd64(sv->getData(), 0x127) == tracker);
        assert(static_cast<uint8_t>(sv->getData()[0x34]) == 0x02);
        assert(static_cast<uint8_t>(sv->getData()[0x40]) == 0x04);
        assert(static_cast<uint8_t>(sv->getData()[0xD4]) == 1);
        assert(sv->isFatefulEncounter());
        assert(sv->move(0) == commonMove && sv->movePP(0) == 1 && sv->movePPUps(0) == 0);
        assert(sv->move(1) == 0 && sv->movePP(1) == 0 && sv->movePPUps(1) == 0);
        assert(toSV.hasLoss(Loss::MoveDropped));
        assert(sv->metLevel() == source->metLevel());
        assert(sv->metLocation() == source->metLocation());
        assert(sv->originGame() == source->originGame());
        assertSerializedReparse(*sv);

        const auto svBefore = nativeBytes(*sv);
        const auto svHash = hashBytes(svBefore);
        Report back;
        auto swsh = convert(*sv, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &back);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*sv, svBefore, svHash);
        assert(back.hasLoss(Loss::TeraDataDropped));
        assert(rd64(swsh->getData(), 0x135) == tracker);
        assert(static_cast<uint8_t>(swsh->getData()[0x34]) == 0x02);
        assert(static_cast<uint8_t>(swsh->getData()[0x40]) == 0x04);
        assert(static_cast<uint8_t>(swsh->getData()[0xE8]) == 1);
        assert(swsh->isFatefulEncounter());
        assertSerializedReparse(*swsh);
        std::cout << "fixture route-swsh-sv-event-tracker-ribbon source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // PK8 <-> Z-A route coverage with no Alpha/divergent payload: tracker survives both directions.
    {
        const uint64_t tracker = 0xB1B2B3B4B5B6B7B8ULL;
        auto source = blankSWSH(0x70000005u);
        configureModern(*source, 25, 0, 0x8899AABBu, 0x8899ABBBu,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        wr64(source->getData(), 0x135, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report toZA;
        Result result = Result::Unsupported;
        auto za = convert(*source, GameVersion::ZA, result,
                          static_cast<uint8_t>(GameVersion::ZA), &toZA);
        assert(result == Result::Ok && za);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(rd64(za->getData(), 0x127) == tracker);
        assertSerializedReparse(*za);

        const auto zaBefore = nativeBytes(*za);
        const auto zaHash = hashBytes(zaBefore);
        Report back;
        auto swsh = convert(*za, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &back);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*za, zaBefore, zaHash);
        assert(!back.hasLoss(Loss::ZAAlphaDropped));
        assert(rd64(swsh->getData(), 0x135) == tracker);
        assertSerializedReparse(*swsh);
        std::cout << "fixture route-swsh-za-tracker source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // G8 sibling route: BDSP <-> Sword preserves the HOME tracker even though TR flags are target-specific.
    {
        const uint64_t tracker = 0xC1C2C3C4C5C6C7C8ULL;
        auto source = blankBDSP(0x70000006u);
        configureModern(*source, 25, 0, 0x99AABBCCu, 0x99AABACCu,
                        static_cast<uint8_t>(GameVersion::BD), u"PIKACHU", false);
        wr64(source->getData(), 0x135, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(rd64(swsh->getData(), 0x135) == tracker);
        assertSerializedReparse(*swsh);
        std::cout << "fixture route-bdsp-swsh-tracker source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // Gen III international language corpus: every currently supported table id must round-trip;
    // Japanese/Korean/Chinese remain explicit fail-closed until actual tables exist.
    {
        for (const uint8_t language : {uint8_t{2}, uint8_t{3}, uint8_t{4}, uint8_t{5}, uint8_t{7}}) {
            auto source = blankSWSH(0x71000000u + language);
            configureModern(*source, 25, 0, 0xAABBCC00u + language, 0xAABBCD00u + language,
                            static_cast<uint8_t>(GameVersion::SW), u"SPARKY", true);
            source->setLanguage(language);
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto pk3 = convert(*source, GameVersion::FRLG, result,
                               static_cast<uint8_t>(GameVersion::FR), &report);
            assert(result == Result::Ok && pk3);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pk3->language() == language);
            assertSerializedReparse(*pk3);
            std::cout << "fixture route-language-" << static_cast<unsigned>(language)
                      << "-swsh-pk3 source-sha256=" << hexHash(sourceHash) << "\n";
        }
        for (const uint8_t language : {uint8_t{1}, uint8_t{8}, uint8_t{9}, uint8_t{10}}) {
            auto source = blankSWSH(0x72000000u + language);
            configureModern(*source, 25, 0, 0xBBCCDD00u + language, 0xBBCCDC00u + language,
                            static_cast<uint8_t>(GameVersion::SW), u"SPARKY", true);
            source->setLanguage(language);
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Ok;
            auto failed = convert(*source, GameVersion::FRLG, result,
                                  static_cast<uint8_t>(GameVersion::FR), &report);
            assert(!failed && result == Result::LanguageNotRepresentable);
            proveSourceUnchanged(*source, before, sourceHash);
        }
    }

    // BDSP <-> S/V route slice: the PK8/PK9 tracker survives the hub transform while target-native
    // Tera is synthesized entering S/V and explicitly reported as lost on the return.
    {
        const uint64_t tracker = 0xD1D2D3D4D5D6D7D8ULL;
        auto source = blankBDSP(0x73000001u);
        configureModern(*source, 25, 0, 0xCCDDEEFFu, 0xCCDDEFFFu,
                        static_cast<uint8_t>(GameVersion::BD), u"PIKACHU", false);
        wr64(source->getData(), 0x135, tracker);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report toSV;
        Result result = Result::Unsupported;
        auto sv = convert(*source, GameVersion::SV, result,
                          static_cast<uint8_t>(GameVersion::SL), &toSV);
        assert(result == Result::Ok && sv);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(toSV.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
        assert(rd64(sv->getData(), 0x127) == tracker);
        assertSerializedReparse(*sv);

        const auto svBefore = nativeBytes(*sv);
        const auto svHash = hashBytes(svBefore);
        Report back;
        auto bdsp = convert(*sv, GameVersion::BDSP, result,
                            static_cast<uint8_t>(GameVersion::BD), &back);
        assert(result == Result::Ok && bdsp);
        proveSourceUnchanged(*sv, svBefore, svHash);
        assert(back.hasLoss(Loss::TeraDataDropped));
        assert(rd64(bdsp->getData(), 0x135) == tracker);
        assertSerializedReparse(*bdsp);
        std::cout << "fixture route-bdsp-sv-tracker source-sha256=" << hexHash(sourceHash) << "\n";
    }

    // Exact shared preflight must report the same candidate viability/losses as production conversion
    // without inventing a second rules engine or mutating the source.
    {
        auto source = blankSWSH(0x74000001u);
        configureModern(*source, 25, 0, 0xDDEEFF00u, 0xDDEEFE00u,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        source->setHeldItem(1);
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        const auto pre = preflightConvert(*source, GameVersion::GG,
                                          static_cast<uint8_t>(GameVersion::GP));
        proveSourceUnchanged(*source, before, sourceHash);
        assert(pre.candidateAvailable && pre.result == Result::Ok);
        assert(pre.report.hasLoss(Loss::HeldItemDropped));
        assert(pre.report.hasLoss(Loss::StatTrainingReset));

        const auto& pi = Pokemon::getPersonalInfo(25, 0);
        source->setAbility(pi.abilityHidden);
        source->setAbilityNumber(4);
        source->refreshChecksum();
        const auto hiddenBefore = nativeBytes(*source);
        const auto hiddenHash = hashBytes(hiddenBefore);
        const auto hidden = preflightConvert(*source, GameVersion::FRLG,
                                             static_cast<uint8_t>(GameVersion::FR));
        proveSourceUnchanged(*source, hiddenBefore, hiddenHash);
        assert(!hidden.candidateAvailable && hidden.result == Result::AbilityNotRepresentable);
    }

    // F13 data contract: historical origin/native format/current store/profile are intentionally
    // distinct, and any declared loss requires acknowledgement before future retirement policy could pass.
    {
        RouteEvidence evidence;
        evidence.sourceFormat = GameVersion::SWSH;
        evidence.destinationFormat = GameVersion::SV;
        evidence.historicalOriginVersion = static_cast<uint8_t>(GameVersion::SW);
        evidence.fidelity.sourceOriginVersion = static_cast<uint8_t>(GameVersion::SW);
        evidence.fidelity.destinationEntityOriginVersion = static_cast<uint8_t>(GameVersion::SW);
        evidence.fidelity.addLoss(Loss::HeldItemDropped);
        evidence.sourceStore.type = PokeBank::Storage::MoveTx::StoreType::Bank;
        evidence.sourceStore.fileId = "bank.dat";
        evidence.destinationStore.type = PokeBank::Storage::MoveTx::StoreType::MutableWorkspaceSingleFile;
        evidence.destinationStore.profile = "account-11111111111111112222222222222222";
        evidence.destinationStore.gameId = "scarlet_switch";
        evidence.destinationStore.workspace = "Working";
        evidence.destinationStore.fileId = "main";
        evidence.sourcePayload[0] = 0x11;
        evidence.destinationPayload[0] = 0x22;

        assert(evidence.historicalOriginVersion == static_cast<uint8_t>(GameVersion::SW));
        assert(evidence.fidelity.destinationEntityOriginVersion == static_cast<uint8_t>(GameVersion::SW));
        assert(evidence.destinationFormat == GameVersion::SV);
        assert(evidence.destinationStore.gameId == "scarlet_switch");
        assert(evidence.destinationStore.profile.find("account-") == 0);
        assert(evidence.requiresLossAcknowledgement());
        assert(!evidence.lossPolicySatisfied());
        evidence.lossesShownToUser = true;
        assert(!evidence.lossPolicySatisfied());
        evidence.lossesAcknowledged = true;
        assert(evidence.lossPolicySatisfied());
    }

    // HOME tracker format matrix: PA8/PK9/PA9 sources explicitly declare loss entering PB7/PK3,
    // while tracker-less PB7 and PK3 sources do not synthesize a tracker when entering PK8.
    {
        const uint64_t plaTracker = 0x1111222233334444ULL;
        auto plaSource = blankPLA(0x75000001u);
        configureModern(*plaSource, 25, 0, 0x12344321u, 0x12344221u,
                        static_cast<uint8_t>(GameVersion::PLA), u"PIKACHU", false);
        wr64(plaSource->getData(), 0x14D, plaTracker);
        plaSource->refreshChecksum();
        const auto plaBefore = nativeBytes(*plaSource);
        const auto plaHash = hashBytes(plaBefore);
        Report plaReport;
        Result result = Result::Unsupported;
        auto gg = convert(*plaSource, GameVersion::GG, result,
                          static_cast<uint8_t>(GameVersion::GP), &plaReport);
        assert(result == Result::Ok && gg);
        proveSourceUnchanged(*plaSource, plaBefore, plaHash);
        assert(plaReport.hasLoss(Loss::HomeTrackerDropped));
        assertSerializedReparse(*gg);

        const uint64_t svTracker = 0x2222333344445555ULL;
        auto svSource = blankSV(0x75000002u);
        configureModern(*svSource, 25, 0, 0x23455432u, 0x23455532u,
                        static_cast<uint8_t>(GameVersion::SL), u"PIKACHU", false);
        wr64(svSource->getData(), 0x127, svTracker);
        svSource->refreshChecksum();
        const auto svBefore = nativeBytes(*svSource);
        const auto svHash = hashBytes(svBefore);
        Report svReport;
        auto pk3 = convert(*svSource, GameVersion::FRLG, result,
                           static_cast<uint8_t>(GameVersion::FR), &svReport);
        assert(result == Result::Ok && pk3);
        proveSourceUnchanged(*svSource, svBefore, svHash);
        assert(svReport.hasLoss(Loss::HomeTrackerDropped));
        assertSerializedReparse(*pk3);

        const uint64_t zaTracker = 0x3333444455556666ULL;
        auto zaSource = blankZA(0x75000003u);
        configureModern(*zaSource, 25, 0, 0x34566543u, 0x34566443u,
                        static_cast<uint8_t>(GameVersion::ZA), u"PIKACHU", false);
        wr64(zaSource->getData(), 0x127, zaTracker);
        zaSource->refreshChecksum();
        const auto zaBefore = nativeBytes(*zaSource);
        const auto zaHash = hashBytes(zaBefore);
        Report zaReport;
        auto ggFromZA = convert(*zaSource, GameVersion::GG, result,
                                static_cast<uint8_t>(GameVersion::GP), &zaReport);
        assert(result == Result::Ok && ggFromZA);
        proveSourceUnchanged(*zaSource, zaBefore, zaHash);
        assert(zaReport.hasLoss(Loss::HomeTrackerDropped));
        assertSerializedReparse(*ggFromZA);

        auto ggSource = blankGG(0x75000004u);
        configureModern(*ggSource, 25, 0, 0x45677654u, 0x45677754u,
                        static_cast<uint8_t>(GameVersion::GP), u"PIKACHU", false);
        const auto ggBefore = nativeBytes(*ggSource);
        const auto ggHash = hashBytes(ggBefore);
        Report ggUp;
        auto swFromGG = convert(*ggSource, GameVersion::SWSH, result,
                                static_cast<uint8_t>(GameVersion::SW), &ggUp);
        assert(result == Result::Ok && swFromGG);
        proveSourceUnchanged(*ggSource, ggBefore, ggHash);
        assert(rd64(swFromGG->getData(), 0x135) == 0);
        assertSerializedReparse(*swFromGG);

        auto pk3Source = blankPK3(0x56788765u, 0x56788665u);
        configurePK3(*pk3Source, 25, 0x56788765u, 0x56788665u);
        const auto pk3Before = nativeBytes(*pk3Source);
        const auto pk3Hash = hashBytes(pk3Before);
        Report pk3Up;
        auto swFromPK3 = convert(*pk3Source, GameVersion::SWSH, result,
                                 static_cast<uint8_t>(GameVersion::SW), &pk3Up);
        assert(result == Result::Ok && swFromPK3);
        proveSourceUnchanged(*pk3Source, pk3Before, pk3Hash);
        assert(rd64(swFromPK3->getData(), 0x135) == 0);
        assertSerializedReparse(*swFromPK3);

        std::cout << "fixture route-home-tracker-format-matrix: PASS\n";
    }

    // Shared production preflight also exposes text/language fidelity failures exactly, with no source mutation.
    {
        auto source = blankSWSH(0x76000001u);
        configureModern(*source, 25, 0, 0x67899876u, 0x67899976u,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        source->setLanguage(1);
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        const auto pre = preflightConvert(*source, GameVersion::FRLG,
                                          static_cast<uint8_t>(GameVersion::FR));
        proveSourceUnchanged(*source, before, sourceHash);
        assert(!pre.candidateAvailable);
        assert(pre.result == Result::LanguageNotRepresentable);
    }

    // SWSH <-> S/V exact-pair audit: known format-specific fields must never disappear silently,
    // and status condition has a destination-native field in both formats so it must relocate rather
    // than be reset. This is intentionally a focused regression before the broader route corpus.
    {
        auto source = blankSWSH(0x77000001u);
        configureModern(*source, 25, 0, 0x11223344u, 0x11223244u,
                        static_cast<uint8_t>(GameVersion::SW), u"PIKACHU", false);
        auto d = source->getData();
        d[0x16] = static_cast<std::byte>(static_cast<uint8_t>(d[0x16]) | 0x10u); // CanGigantamax
        wr32s(d, 0x48, 0x01020304u); // Sociability
        d[0x90] = std::byte{7};       // Dynamax level
        wr32s(d, 0x94, 0x00000008u); // Status condition
        wr32s(d, 0x98, 0x00000001u); // PK8 Palma/source-only field
        d[0xCE] = std::byte{1};       // PokeJob flag
        d[0xDC] = std::byte{2};       // Fullness
        d[0x127] = std::byte{1};      // TR record flag
        source->refreshChecksum();

        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto sv = convert(*source, GameVersion::SV, result,
                          static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && sv);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::DivergentGameDataDropped));
        // G8 has no independent Scale or ObedienceLevel fields. Entering S/V deterministically
        // synthesizes Scale from Height and ObedienceLevel from MetLevel; reserve adaptation bits
        // 5 and 6 for those explicit representation changes.
        assert((report.adaptations & (1u << 5)) != 0);
        assert((report.adaptations & (1u << 6)) != 0);
        assert(rd32(sv->getData(), 0x90) == 0x00000008u);
        assertSerializedReparse(*sv);
    }

    {
        auto source = blankSV(0x77000002u);
        configureModern(*source, 25, 0, 0x55667788u, 0x55667688u,
                        static_cast<uint8_t>(GameVersion::SL), u"PIKACHU", false);
        auto d = source->getData();
        wr32s(d, 0x90, 0x00000010u); // Status condition
        d[0x4A] = std::byte{77};      // Scale distinct from height
        d[0x11F] = std::byte{42};     // Obedience level distinct from met level
        d[0x12F] = std::byte{1};      // PK9 base TM-record flag
        source->refreshChecksum();

        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto swsh = convert(*source, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && swsh);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::TeraDataDropped));
        assert(report.hasLoss(Loss::DivergentGameDataDropped));
        assert(rd32(swsh->getData(), 0x94) == 0x00000010u);
        assertSerializedReparse(*swsh);
    }


    // PK8/PK9 closure regressions: each fixture changes ONE semantic so silent loss cannot hide
    // behind an unrelated loss bit. These are intentionally written before the production fixes.
    {
        // PK8 Flag2 (0x22 bit1) has no PK9 representation. If a legitimate/nonzero source carries
        // it, the conversion must at least declare the generation-specific semantic loss.
        auto source = blankSWSH(0x77100001u);
        configureModern(*source, 25, 0, 0x10101010u, 0x10101110u,
                        static_cast<uint8_t>(GameVersion::SW), u"FLAG2", true);
        auto raw = source->getData();
        raw[0x22] = static_cast<std::byte>(static_cast<uint8_t>(raw[0x22]) | 0x02u);
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto candidate = convert(*source, GameVersion::SV, result,
                                 static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && candidate);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::DivergentGameDataDropped));
        assertSerializedReparse(*candidate);
    }

    {
        // PK8's party-only DynamaxType (0x156-0x157) has no PK9 counterpart. A nonzero value must
        // be declared lost rather than merely zeroed while constructing the PK9 party tail.
        auto source = blankSWSH(0x77100002u);
        configureModern(*source, 25, 0, 0x20202020u, 0x20202120u,
                        static_cast<uint8_t>(GameVersion::SW), u"DMAXTYPE", true);
        auto raw = source->getData();
        assert(raw.size() >= 0x158);
        raw[0x156] = std::byte{1};
        raw[0x157] = std::byte{0};
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto candidate = convert(*source, GameVersion::SV, result,
                                 static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && candidate);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(report.hasLoss(Loss::DivergentGameDataDropped));
        assertSerializedReparse(*candidate);
    }

    {
        // S/V can represent ball ids beyond SWSH's PK8 domain (PK8 ends at Beast Ball = 26).
        // The converter must fail closed instead of copying a destination-invalid ball byte.
        auto source = blankSV(0x77100003u);
        configureModern(*source, 25, 0, 0x30303030u, 0x30303130u,
                        static_cast<uint8_t>(GameVersion::SL), u"BALL27", true);
        source->setBall(27); // Strange Ball: valid PK9 id, not representable by PK8/SWSH.
        source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
        source->getData()[0x4A] = source->getData()[0x48];
        source->refreshChecksum();
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto candidate = convert(*source, GameVersion::SWSH, result,
                                 static_cast<uint8_t>(GameVersion::SW), &report);
        assert(!candidate);
        assert(result == Result::BallNotRepresentable);
        proveSourceUnchanged(*source, before, sourceHash);

        const auto pre = preflightConvert(*source, GameVersion::SWSH,
                                          static_cast<uint8_t>(GameVersion::SW));
        assert(!pre.candidateAvailable);
        assert(pre.result == Result::BallNotRepresentable);
        proveSourceUnchanged(*source, before, sourceHash);
    }

    {
        // AffixedRibbon is an index into the shared 128-bit ribbon/mark set. Preserve a valid mark
        // affix, but clear a dangling one instead of leaving a title that the Pokemon does not own.
        auto valid = blankSWSH(0x77100004u);
        configureModern(*valid, 25, 0, 0x40404040u, 0x40404140u,
                        static_cast<uint8_t>(GameVersion::SW), u"AFFIXOK", true);
        valid->getData()[0x40] = std::byte{0x01}; // index 64 owned
        valid->getData()[0xE8] = std::byte{64};
        valid->refreshChecksum();
        const auto validBefore = nativeBytes(*valid);
        const auto validHash = hashBytes(validBefore);
        Report report;
        Result result = Result::Unsupported;
        auto validSV = convert(*valid, GameVersion::SV, result,
                               static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && validSV);
        proveSourceUnchanged(*valid, validBefore, validHash);
        assert(static_cast<uint8_t>(validSV->getData()[0xD4]) == 64);
        assertSerializedReparse(*validSV);

        auto dangling = blankSWSH(0x77100005u);
        configureModern(*dangling, 25, 0, 0x50505050u, 0x50505150u,
                        static_cast<uint8_t>(GameVersion::SW), u"AFFIXBAD", true);
        dangling->getData()[0xE8] = std::byte{64}; // index 64, but 0x40 bit0 is NOT owned
        dangling->refreshChecksum();
        const auto danglingBefore = nativeBytes(*dangling);
        const auto danglingHash = hashBytes(danglingBefore);
        auto normalizedSV = convert(*dangling, GameVersion::SV, result,
                                    static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && normalizedSV);
        proveSourceUnchanged(*dangling, danglingBefore, danglingHash);
        assert(static_cast<uint8_t>(normalizedSV->getData()[0xD4]) == AFFIXED_RIBBON_NONE);
        assertSerializedReparse(*normalizedSV);

        auto danglingPK9 = blankSV(0x77100006u);
        configureModern(*danglingPK9, 25, 0, 0x60606060u, 0x60606160u,
                        static_cast<uint8_t>(GameVersion::SL), u"AFFIXPK9", true);
        danglingPK9->getData()[0xD4] = std::byte{64};
        danglingPK9->refreshChecksum();
        assert(normalizeAffixedRibbon(*danglingPK9));
        assert(static_cast<uint8_t>(danglingPK9->getData()[0xD4]) == AFFIXED_RIBBON_NONE);
        assert(danglingPK9->checksumValid());
    }

    // Special-form closure: destination presence alone is not sufficient. The pinned transfer
    // oracle classifies battle-only/transient forms as untradable states, so a shared species must
    // not carry one of those transient form values across saves as if it were an ordinary form.
    {
        for (const auto& [species, form] : std::array<std::pair<uint16_t, uint8_t>, 2>{{
                 {778, 1}, // Mimikyu Busted
                 {875, 1}, // Eiscue Noice
             }}) {
            auto source = blankSWSH(0x77110000u + species);
            configureModern(*source, species, form, 0x61610000u + species, 0x61610100u + species,
                            static_cast<uint8_t>(GameVersion::SW), u"TRANSIENT", true);
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Result result = Result::Unsupported;
            Report report;
            auto candidate = convert(*source, GameVersion::SV, result,
                                     static_cast<uint8_t>(GameVersion::SL), &report);
            assert(!candidate);
            assert(result == Result::FormNotTransferable);
            proveSourceUnchanged(*source, before, sourceHash);
            const auto pre = preflightConvert(*source, GameVersion::SV,
                                              static_cast<uint8_t>(GameVersion::SL));
            assert(!pre.candidateAvailable);
            assert(pre.result == Result::FormNotTransferable);
            proveSourceUnchanged(*source, before, sourceHash);
        }
    }

    // PK9 -> PK8 history representation closure. HOME does not store Scarlet/Violet's raw
    // Version/met/egg fields verbatim inside PK8: it maps them to target-valid SWSH representation
    // while external/core provenance retains the historical S/V origin.
    {
        struct HistoryCase {
            uint8_t sourceVersion;
            uint8_t destinationVersion;
            uint8_t expectedPk8Version;
            uint16_t expectedMet;
        };
        const HistoryCase cases[] = {
            {static_cast<uint8_t>(GameVersion::SL), static_cast<uint8_t>(GameVersion::SW),
             static_cast<uint8_t>(GameVersion::SW), 59997},
            {static_cast<uint8_t>(GameVersion::VL), static_cast<uint8_t>(GameVersion::SH),
             static_cast<uint8_t>(GameVersion::SH), 59996},
        };
        for (size_t i = 0; i < std::size(cases); ++i) {
            const auto& hc = cases[i];
            auto source = blankSV(0x77120000u + static_cast<uint32_t>(i));
            configureModern(*source, 25, 0, 0x71717171u + static_cast<uint32_t>(i),
                            0x71717071u + static_cast<uint32_t>(i), hc.sourceVersion,
                            u"HISTORYMAP", true);
            auto raw = source->getData();
            wr16(raw, 0x120, 5678); // S/V egg location that cannot stay raw in PK8.
            wr16(raw, 0x122, 1234); // S/V met location that cannot stay raw in PK8.
            raw[0x11F] = raw[0x125];
            raw[0x4A] = raw[0x48];
            source->refreshChecksum();

            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            const auto pre = preflightConvert(*source, GameVersion::SWSH, hc.destinationVersion);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pre.candidateAvailable && pre.result == Result::Ok);

            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, GameVersion::SWSH, result, hc.destinationVersion, &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(candidate->originGame() == hc.expectedPk8Version);
            assert(candidate->metLocation() == hc.expectedMet);
            assert(rd32(candidate->getData(), 0x120) ==
                   (static_cast<uint32_t>(hc.expectedMet) << 16 | 65534u));
            assert(report.sourceOriginVersion == hc.sourceVersion);
            assert(report.destinationEntityOriginVersion == hc.expectedPk8Version);
            assert((report.adaptations & (1u << 7)) != 0);
            assert(pre.report.losses == report.losses);
            assert(pre.report.adaptations == report.adaptations);
            assertSerializedReparse(*candidate);
        }
    }

    // SWSH <-> S/V exact-pair route-completion corpus.
    // The personal table is generated from production PKHeX resources and carries game-presence
    // bits per species+form. Pin the exact current intersection rather than calling the pair
    // "compatible" from a small sample.
    {
        size_t sharedBase = 0, swshOnlyBase = 0, svOnlyBase = 0, neitherBase = 0;
        size_t sharedForms = 0, swshOnlyForms = 0, svOnlyForms = 0, neitherForms = 0;
        for (uint16_t sp = 1; sp <= Pokemon::PERSONAL_MAX_SPECIES; ++sp) {
            const auto& base = Pokemon::getPersonalInfo(sp, 0);
            const bool sw = (base.presence & Pokemon::PERSONAL_GAME_SWSH) != 0;
            const bool sv = (base.presence & Pokemon::PERSONAL_GAME_SV) != 0;
            if (sw && sv) ++sharedBase;
            else if (sw) ++swshOnlyBase;
            else if (sv) ++svOnlyBase;
            else ++neitherBase;

            for (uint8_t form = 1; form < base.formCount; ++form) {
                const auto& pi = Pokemon::getPersonalInfo(sp, form);
                const bool fsw = (pi.presence & Pokemon::PERSONAL_GAME_SWSH) != 0;
                const bool fsv = (pi.presence & Pokemon::PERSONAL_GAME_SV) != 0;
                if (fsw && fsv) ++sharedForms;
                else if (fsw) ++swshOnlyForms;
                else if (fsv) ++svOnlyForms;
                else ++neitherForms;
            }
        }
        assert(sharedBase == 420);
        assert(swshOnlyBase == 244);
        assert(svOnlyBase == 313);
        assert(neitherBase == 48);
        assert(sharedForms == 71);
        assert(swshOnlyForms == 50);
        assert(svOnlyForms == 175);
        assert(neitherForms == 169);

        auto expectNotInDex = [&](std::unique_ptr<Pokemon::Pokemon> source,
                                  GameVersion destinationGroup,
                                  uint8_t destinationVersion) {
            const auto before = nativeBytes(*source);
            const auto hash = hashBytes(before);
            const auto pre = preflightConvert(*source, destinationGroup, destinationVersion);
            assert(!pre.candidateAvailable);
            assert(pre.result == Result::NotInDex);
            proveSourceUnchanged(*source, before, hash);
            Result result = Result::Ok;
            Report report;
            auto candidate = convert(*source, destinationGroup, result, destinationVersion, &report);
            assert(!candidate);
            assert(result == Result::NotInDex);
            proveSourceUnchanged(*source, before, hash);
        };

        // Base species absent from the destination.
        {
            auto source = blankSWSH(0x78000001u);
            configureModern(*source, 10, 0, 0x11112222u, 0x11112322u,
                            static_cast<uint8_t>(GameVersion::SW), u"CATERPIE", false);
            expectNotInDex(std::move(source), GameVersion::SV,
                           static_cast<uint8_t>(GameVersion::SL));
        }
        {
            auto source = blankSV(0x78000002u);
            configureModern(*source, 23, 0, 0x22223333u, 0x22223233u,
                            static_cast<uint8_t>(GameVersion::SL), u"EKANS", false);
            source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
            source->refreshChecksum();
            expectNotInDex(std::move(source), GameVersion::SWSH,
                           static_cast<uint8_t>(GameVersion::SW));
        }

        // Regional forms absent from the destination must not flatten to form 0.
        {
            auto source = blankSWSH(0x78000003u);
            configureModern(*source, 77, 1, 0x33334444u, 0x33334544u,
                            static_cast<uint8_t>(GameVersion::SH), u"PONYTA", false);
            assert(source->form() == 1);
            expectNotInDex(std::move(source), GameVersion::SV,
                           static_cast<uint8_t>(GameVersion::VL));
        }
        {
            auto source = blankSV(0x78000004u);
            configureModern(*source, 128, 1, 0x44445555u, 0x44445455u,
                            static_cast<uint8_t>(GameVersion::VL), u"TAUROS", false);
            source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
            source->refreshChecksum();
            assert(source->form() == 1);
            expectNotInDex(std::move(source), GameVersion::SWSH,
                           static_cast<uint8_t>(GameVersion::SH));
        }

        // Shared regional/permanent forms survive as the same form through production serialization.
        for (const auto& [sp, form] : std::array<std::pair<uint16_t, uint8_t>, 3>{{
                 {37, 1},   // Alolan Vulpix
                 {52, 2},   // Galarian Meowth
                 {479, 1},  // Rotom form
             }}) {
            auto source = blankSWSH(0x78100000u + sp + form);
            configureModern(*source, sp, form, 0x55550000u + sp, 0x55550100u + sp,
                            static_cast<uint8_t>(GameVersion::SW), u"FORMTEST", true);
            const auto before = nativeBytes(*source);
            const auto hash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto sv = convert(*source, GameVersion::SV, result,
                              static_cast<uint8_t>(GameVersion::SL), &report);
            assert(result == Result::Ok && sv);
            proveSourceUnchanged(*source, before, hash);
            assert(sv->speciesID() == sp && sv->form() == form);
            assertSerializedReparse(*sv);
        }
    }

    // Every exact title direction uses the same PK8<->PK9 format transform but keeps exact
    // source/destination identity outside the format group. Exercise all eight identities rather
    // than inferring Sword=Shield or Scarlet=Violet.
    {
        struct ExactRoute {
            bool sourceG8;
            uint8_t sourceVersion;
            uint8_t destinationVersion;
            const char* label;
        };
        const ExactRoute routes[] = {
            {true,  static_cast<uint8_t>(GameVersion::SW), static_cast<uint8_t>(GameVersion::SL), "sword-scarlet"},
            {true,  static_cast<uint8_t>(GameVersion::SW), static_cast<uint8_t>(GameVersion::VL), "sword-violet"},
            {true,  static_cast<uint8_t>(GameVersion::SH), static_cast<uint8_t>(GameVersion::SL), "shield-scarlet"},
            {true,  static_cast<uint8_t>(GameVersion::SH), static_cast<uint8_t>(GameVersion::VL), "shield-violet"},
            {false, static_cast<uint8_t>(GameVersion::SL), static_cast<uint8_t>(GameVersion::SW), "scarlet-sword"},
            {false, static_cast<uint8_t>(GameVersion::SL), static_cast<uint8_t>(GameVersion::SH), "scarlet-shield"},
            {false, static_cast<uint8_t>(GameVersion::VL), static_cast<uint8_t>(GameVersion::SW), "violet-sword"},
            {false, static_cast<uint8_t>(GameVersion::VL), static_cast<uint8_t>(GameVersion::SH), "violet-shield"},
        };

        uint16_t commonMove = 0;
        for (uint16_t move = 1; move <= Pokemon::LEARN_MAX_MOVE_ID; ++move) {
            if (Pokemon::isLearnable(25, 0, GameVersion::SWSH, move) &&
                Pokemon::isLearnable(25, 0, GameVersion::SV, move)) {
                commonMove = move;
                break;
            }
        }
        assert(commonMove != 0);

        uint16_t commonItem = 0;
        for (uint16_t item = 1; item <= Names::ITEM_PRESENCE_MAX_ID; ++item) {
            if (Names::isHeldItemPresent(item, GameVersion::SWSH) &&
                Names::isHeldItemPresent(item, GameVersion::SV)) {
                commonItem = item;
                break;
            }
        }
        assert(commonItem != 0);

        for (size_t r = 0; r < std::size(routes); ++r) {
            const auto& route = routes[r];
            std::unique_ptr<Pokemon::Pokemon> source;
            if (route.sourceG8) source = blankSWSH(0x79000000u + static_cast<uint32_t>(r));
            else source = blankSV(0x79000000u + static_cast<uint32_t>(r));

            const uint32_t pid = 0x60001000u + static_cast<uint32_t>(r) * 0x101u;
            const uint32_t id32 = pid ^ 0x00000100u;
            configureModern(*source, 25, 0, pid, id32, route.sourceVersion, u"PAIRTEST", true);
            source->setOTName(u"ROUTEOT");
            source->setLanguage(2);
            source->setBall(16); // Cherish Ball, representative event ball
            source->setHeldItem(commonItem);
            source->setMove(0, commonMove);
            source->setMovePPUps(0, 1);
            source->setMovePP(0, Names::getMoveMaxPP(commonMove, 1,
                route.sourceG8 ? GameVersion::SWSH : GameVersion::SV));
            source->setRelearnMove(0, commonMove);
            source->setNature(3);
            source->setStatNature(10);
            source->setFriendship(187);
            for (int i = 0; i < 6; ++i) {
                source->setIV(i, static_cast<uint8_t>(20 + i));
                source->setEV(i, static_cast<uint8_t>(4 * i));
            }
            auto raw = source->getData();
            raw[0x22] = static_cast<std::byte>(static_cast<uint8_t>(raw[0x22]) | 0x01u); // fateful
            raw[0x37] = std::byte{0x14}; // representative event ribbons
            raw[0x3A] = std::byte{0x24}; // Galar ribbon + encounter mark
            raw[0x40] = std::byte{0x11}; // representative mark bits
            raw[0x126] = std::byte{0x15}; // hyper-training flags
            raw[0x32] = std::byte{0x21};  // Pokerus state
            const uint64_t tracker = 0xABCDEF0000000000ULL + static_cast<uint64_t>(r + 1);
            if (route.sourceG8) {
                wr64(raw, 0x135, tracker);
                wr32s(raw, 0x94, 0x00000008u + static_cast<uint32_t>(r));
            } else {
                wr64(raw, 0x127, tracker);
                wr32s(raw, 0x90, 0x00000008u + static_cast<uint32_t>(r));
                raw[0x11F] = static_cast<std::byte>(source->metLevel()); // reversible obedience case
                raw[0x4A] = raw[0x48]; // reversible scale case
            }
            source->refreshChecksum();

            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            const GameVersion destinationGroup = route.sourceG8 ? GameVersion::SV : GameVersion::SWSH;

            const auto pre = preflightConvert(*source, destinationGroup, route.destinationVersion);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pre.candidateAvailable && pre.result == Result::Ok);

            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, destinationGroup, result, route.destinationVersion, &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(pre.report.losses == report.losses);
            assert(pre.report.adaptations == report.adaptations);
            const uint8_t expectedNativeVersion = route.sourceG8
                ? route.sourceVersion
                : static_cast<uint8_t>(route.sourceVersion == static_cast<uint8_t>(GameVersion::VL)
                                           ? GameVersion::SH : GameVersion::SW);
            assert(candidate->originGame() == expectedNativeVersion);
            assert(report.sourceOriginVersion == route.sourceVersion);
            assert(report.destinationEntityOriginVersion == expectedNativeVersion);
            assert(candidate->pid() == source->pid());
            assert(candidate->encryptionConstant() == source->encryptionConstant());
            assert(candidate->speciesID() == 25 && candidate->form() == 0);
            assert(candidate->language() == source->language());
            assert(candidate->nickname() == source->nickname());
            assert(candidate->otName() == source->otName());
            assert(candidate->ball() == source->ball());
            assert(candidate->heldItem() == source->heldItem());
            assert(candidate->nature() == source->nature());
            assert(candidate->statNature() == source->statNature());
            assert(candidate->friendship() == source->friendship());
            assert(candidate->move(0) == commonMove);
            assert(candidate->relearnMove(0) == commonMove);
            assert(candidate->isFatefulEncounter());
            assert(static_cast<uint8_t>(candidate->getData()[0x37]) == 0x14);
            assert(static_cast<uint8_t>(candidate->getData()[0x3A]) == 0x24);
            assert(static_cast<uint8_t>(candidate->getData()[0x40]) == 0x11);
            assert(static_cast<uint8_t>(candidate->getData()[0x126]) == 0x15);
            assert(static_cast<uint8_t>(candidate->getData()[0x32]) == 0x21);
            const std::array<uint8_t, 6> candidateIVs{
                candidate->ivHP(), candidate->ivATK(), candidate->ivDEF(),
                candidate->ivSPE(), candidate->ivSPA(), candidate->ivSPD()};
            const std::array<uint8_t, 6> sourceIVs{
                source->ivHP(), source->ivATK(), source->ivDEF(),
                source->ivSPE(), source->ivSPA(), source->ivSPD()};
            const std::array<uint8_t, 6> candidateEVs{
                candidate->evHP(), candidate->evATK(), candidate->evDEF(),
                candidate->evSPE(), candidate->evSPA(), candidate->evSPD()};
            const std::array<uint8_t, 6> sourceEVs{
                source->evHP(), source->evATK(), source->evDEF(),
                source->evSPE(), source->evSPA(), source->evSPD()};
            assert(candidateIVs == sourceIVs);
            assert(candidateEVs == sourceEVs);
            if (route.sourceG8) {
                assert(rd64(candidate->getData(), 0x127) == tracker);
                assert(report.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
                assert(report.hasAdaptation(Adaptation::TargetScaleSynthesized));
                assert(report.hasAdaptation(Adaptation::TargetObedienceLevelSynthesized));
                assert(rd32(candidate->getData(), 0x90) == 0x00000008u + static_cast<uint32_t>(r));
            } else {
                assert(rd64(candidate->getData(), 0x135) == tracker);
                assert(report.hasLoss(Loss::TeraDataDropped));
                assert(!report.hasLoss(Loss::DivergentGameDataDropped));
                assert(report.hasAdaptation(Adaptation::TargetHistoryRepresentationRemapped));
                assert(candidate->metLocation() ==
                    (route.sourceVersion == static_cast<uint8_t>(GameVersion::VL) ? 59996 : 59997));
                assert(rd32(candidate->getData(), 0x94) == 0x00000008u + static_cast<uint32_t>(r));
            }
            assertSerializedReparse(*candidate);
            std::cout << "fixture exact-pair-" << route.label
                      << " source-sha256=" << hexHash(sourceHash) << "\n";
        }
    }


    // Historical origin is entity history, not the current store. A BDSP-origin entity currently
    // living in a Sword workspace and converted to Scarlet must remain BDSP-origin.
    {
        auto source = blankSWSH(0x79F00001u);
        configureModern(*source, 25, 0, 0x31415926u, 0x31415826u,
                        static_cast<uint8_t>(GameVersion::BD), u"OLDORIGIN", true);
        assert(source->originGame() == static_cast<uint8_t>(GameVersion::BD));
        const auto before = nativeBytes(*source);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto candidate = convert(*source, GameVersion::SV, result,
                                 static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && candidate);
        proveSourceUnchanged(*source, before, sourceHash);
        assert(candidate->originGame() == static_cast<uint8_t>(GameVersion::BD));
        assert(report.sourceOriginVersion == static_cast<uint8_t>(GameVersion::BD));
        assert(report.destinationEntityOriginVersion == static_cast<uint8_t>(GameVersion::BD));
        assertSerializedReparse(*candidate);
    }

    // Modern shiny/PID/EC semantics are shared by PK8 and PK9. Cover both shiny classes and an
    // ordinary non-shiny without allowing conversion to regenerate identity.
    {
        for (const bool fromG8 : {true, false}) {
            for (const uint16_t shinyXor : {uint16_t{0}, uint16_t{15}, uint16_t{16}}) {
                const uint32_t pid = 0x13572468u + shinyXor;
                const uint32_t id32 = pid ^ shinyXor;
                std::unique_ptr<Pokemon::Pokemon> source;
                if (fromG8) source = blankSWSH(0x7A000000u + shinyXor);
                else source = blankSV(0x7A100000u + shinyXor);
                configureModern(*source, 25, 0, pid, id32,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                u"SHINYTEST", true);
                if (!fromG8) {
                    source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                    source->getData()[0x4A] = source->getData()[0x48];
                    source->refreshChecksum();
                }
                const bool expectedShiny = Fidelity::isModernShiny(pid, id32);
                assert(expectedShiny == (shinyXor < 16));
                const uint32_t sourceEC = source->encryptionConstant();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                Report report;
                Result result = Result::Unsupported;
                auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
                assert(result == Result::Ok && candidate);
                proveSourceUnchanged(*source, before, sourceHash);
                assert(candidate->pid() == pid);
                assert(candidate->encryptionConstant() == sourceEC);
                assert(candidate->isShiny(id32, "") == expectedShiny);
                assertSerializedReparse(*candidate);
            }
        }
    }

    // Ability slots are byte-compatible for tested shared species. Exercise normal slot 1,
    // distinct slot 2, duplicate slot semantics, and hidden ability without substitution.
    {
        const uint16_t species[] = {280, 25}; // Ralts has distinct normal slots; Pikachu duplicates
        for (const uint16_t sp : species) {
            const auto& pi = Pokemon::getPersonalInfo(sp, 0);
            assert((pi.presence & Pokemon::PERSONAL_GAME_SWSH) != 0);
            assert((pi.presence & Pokemon::PERSONAL_GAME_SV) != 0);
            const std::array<std::pair<uint16_t, uint8_t>, 3> abilities{{
                {pi.ability1, 1},
                {pi.ability2, 2},
                {pi.abilityHidden, 4},
            }};
            for (const bool fromG8 : {true, false}) {
                for (const auto& [ability, slot] : abilities) {
                    auto source = fromG8
                        ? std::unique_ptr<Pokemon::Pokemon>(blankSWSH(0x7B000000u + sp + slot).release())
                        : std::unique_ptr<Pokemon::Pokemon>(blankSV(0x7B100000u + sp + slot).release());
                    configureModern(*source, sp, 0, 0x24680000u + sp + slot, 0x24680100u + sp + slot,
                                    static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                    u"ABILITY", true);
                    source->setAbility(ability);
                    source->setAbilityNumber(slot);
                    if (!fromG8) {
                        source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                        source->getData()[0x4A] = source->getData()[0x48];
                    }
                    source->refreshChecksum();
                    const auto before = nativeBytes(*source);
                    const auto sourceHash = hashBytes(before);
                    Report report;
                    Result result = Result::Unsupported;
                    auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                        result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
                    assert(result == Result::Ok && candidate);
                    proveSourceUnchanged(*source, before, sourceHash);
                    assert(candidate->ability() == ability);
                    assert(candidate->abilityNumber() == slot);
                    assertSerializedReparse(*candidate);
                }
            }
        }
    }


    // Two shared species have a real slot-2 ability-table change between PK8 and PK9.
    // Until an official-style remap policy is deliberately implemented, fail closed rather than
    // preserving an ability id that is invalid in the destination game.
    {
        struct AbilityDivergence {
            uint16_t species;
            uint16_t swshSlot2;
            uint16_t svSlot2;
        };
        const AbilityDivergence cases[] = {
            {275, 48, 274}, // Shiftry
            {475, 80, 292}, // Gallade
        };
        for (const auto& x : cases) {
            for (const bool fromG8 : {true, false}) {
                std::unique_ptr<Pokemon::Pokemon> source;
                if (fromG8) source = blankSWSH(0x7B800000u + x.species);
                else source = blankSV(0x7B900000u + x.species);
                configureModern(*source, x.species, 0, 0x76540000u + x.species,
                                0x76540100u + x.species,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                u"ABILITYFAIL", true);
                source->setAbility(fromG8 ? x.swshSlot2 : x.svSlot2);
                source->setAbilityNumber(2);
                if (!fromG8) {
                    source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                    source->getData()[0x4A] = source->getData()[0x48];
                }
                source->refreshChecksum();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                const auto pre = preflightConvert(*source,
                    fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW));
                assert(!pre.candidateAvailable);
                assert(pre.result == Result::AbilityNotRepresentable);
                proveSourceUnchanged(*source, before, sourceHash);
                Report report;
                Result result = Result::Ok;
                auto candidate = convert(*source,
                    fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    result,
                    static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW),
                    &report);
                assert(!candidate);
                assert(result == Result::AbilityNotRepresentable);
                proveSourceUnchanged(*source, before, sourceHash);
            }
        }
    }

    // Moves/relearn moves: derive route classes from production learnsets rather than hard-coding
    // stale move ids. Shared moves survive; source-only moves and relearns are explicitly dropped.
    {
        std::vector<uint16_t> common, swOnly, svOnly;
        for (uint16_t move = 1; move <= Pokemon::LEARN_MAX_MOVE_ID; ++move) {
            const bool sw = Pokemon::isLearnable(25, 0, GameVersion::SWSH, move);
            const bool sv = Pokemon::isLearnable(25, 0, GameVersion::SV, move);
            if (sw && sv) common.push_back(move);
            else if (sw) swOnly.push_back(move);
            else if (sv) svOnly.push_back(move);
        }
        assert(common.size() >= 2 && swOnly.size() >= 2 && svOnly.size() >= 2);

        auto exercise = [&](bool fromG8,
                            const std::vector<uint16_t>& sourceOnly) {
            std::unique_ptr<Pokemon::Pokemon> source;
            if (fromG8) source = blankSWSH(fromG8 ? 0x7C000001u : 0x7C100001u);
            else source = blankSV(0x7C100001u);
            configureModern(*source, 25, 0, 0x33445566u, 0x33445466u,
                            static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                            u"MOVETEST", true);
            const uint16_t moves[4] = {common[0], sourceOnly[0], common[1], sourceOnly[1]};
            for (int i = 0; i < 4; ++i) {
                source->setMove(i, moves[i]);
                source->setMovePPUps(i, static_cast<uint8_t>(i % 2));
                source->setMovePP(i, Names::getMoveMaxPP(moves[i], source->movePPUps(i),
                    fromG8 ? GameVersion::SWSH : GameVersion::SV));
                source->setRelearnMove(i, moves[i]);
            }
            if (!fromG8) {
                source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                source->getData()[0x4A] = source->getData()[0x48];
            }
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(report.hasLoss(Loss::MoveDropped));
            assert(report.hasLoss(Loss::RelearnMoveDropped));
            assert(candidate->move(0) == common[0]);
            assert(candidate->move(1) == common[1]);
            assert(candidate->move(2) == 0 && candidate->move(3) == 0);
            assert(candidate->relearnMove(0) == common[0]);
            assert(candidate->relearnMove(1) == 0);
            assert(candidate->relearnMove(2) == common[1]);
            assert(candidate->relearnMove(3) == 0);
            assertSerializedReparse(*candidate);
        };
        exercise(true, swOnly);
        exercise(false, svOnly);
    }

    // Held-item route classes from the generated per-game presence tables.
    {
        uint16_t common = 0, swOnly = 0, svOnly = 0;
        for (uint16_t item = 1; item <= Names::ITEM_PRESENCE_MAX_ID; ++item) {
            const bool sw = Names::isHeldItemPresent(item, GameVersion::SWSH);
            const bool sv = Names::isHeldItemPresent(item, GameVersion::SV);
            if (sw && sv && common == 0) common = item;
            if (sw && !sv && swOnly == 0) swOnly = item;
            if (!sw && sv && svOnly == 0) svOnly = item;
        }
        assert(common != 0 && swOnly != 0 && svOnly != 0);

        auto exercise = [&](bool fromG8, uint16_t item, bool expectDrop) {
            std::unique_ptr<Pokemon::Pokemon> source;
            if (fromG8) source = blankSWSH(0x7D000000u + item);
            else source = blankSV(0x7D100000u + item);
            configureModern(*source, 25, 0, 0x44556677u, 0x44556777u,
                            static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                            u"ITEMTEST", true);
            source->setHeldItem(item);
            if (!fromG8) {
                source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                source->getData()[0x4A] = source->getData()[0x48];
            }
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(report.hasLoss(Loss::HeldItemDropped) == expectDrop);
            assert(candidate->heldItem() == (expectDrop ? 0 : item));
            assertSerializedReparse(*candidate);
        };
        exercise(true, common, false);
        exercise(false, common, false);
        exercise(true, swOnly, true);
        exercise(false, svOnly, true);
    }


    // SWSH and S/V changed the base stats of five shared species/form entries. Cresselia is the
    // simplest proof that the cached party-stat tail must be recalculated with destination-native
    // base stats instead of copied across generations.
    {
        auto sw = blankSWSH(0x7D800001u);
        configureModern(*sw, 488, 0, 0x10203040u, 0x10203140u,
                        static_cast<uint8_t>(GameVersion::SW), u"CRESSELIA", false);
        sw->setNature(0);
        sw->setStatNature(0);
        for (int s = 0; s < 6; ++s) {
            sw->setIV(s, 31);
            sw->setEV(s, 0);
        }
        sw->setLevel(50);
        sw->recalculateStats();
        sw->refreshChecksum();
        assert(sw->baseDEF() == 120);
        assert(sw->baseSPD() == 130);
        assert(sw->statDEF() == 140);
        assert(sw->statSPD() == 150);

        const auto before = nativeBytes(*sw);
        const auto sourceHash = hashBytes(before);
        Report report;
        Result result = Result::Unsupported;
        auto sv = convert(*sw, GameVersion::SV, result,
                          static_cast<uint8_t>(GameVersion::SL), &report);
        assert(result == Result::Ok && sv);
        proveSourceUnchanged(*sw, before, sourceHash);
        assert(sv->baseDEF() == 110);
        assert(sv->baseSPD() == 120);
        assert(sv->statDEF() == 130);
        assert(sv->statSPD() == 140);
        assertSerializedReparse(*sv);

        auto reverse = blankSV(0x7D800002u);
        configureModern(*reverse, 488, 0, 0x20304050u, 0x20304150u,
                        static_cast<uint8_t>(GameVersion::SL), u"CRESSELIA", false);
        reverse->setNature(0);
        reverse->setStatNature(0);
        for (int s = 0; s < 6; ++s) {
            reverse->setIV(s, 31);
            reverse->setEV(s, 0);
        }
        reverse->setLevel(50);
        reverse->getData()[0x11F] = static_cast<std::byte>(reverse->metLevel());
        reverse->getData()[0x4A] = reverse->getData()[0x48];
        reverse->recalculateStats();
        reverse->refreshChecksum();
        assert(reverse->baseDEF() == 110);
        assert(reverse->baseSPD() == 120);
        assert(reverse->statDEF() == 130);
        assert(reverse->statSPD() == 140);

        const auto reverseBefore = nativeBytes(*reverse);
        const auto reverseHash = hashBytes(reverseBefore);
        auto back = convert(*reverse, GameVersion::SWSH, result,
                            static_cast<uint8_t>(GameVersion::SW), &report);
        assert(result == Result::Ok && back);
        proveSourceUnchanged(*reverse, reverseBefore, reverseHash);
        assert(back->baseDEF() == 120);
        assert(back->baseSPD() == 130);
        assert(back->statDEF() == 140);
        assert(back->statSPD() == 150);
        assertSerializedReparse(*back);
    }

    // Shared ribbon/mark storage, HOME tracker zero/nonzero, balls and modern languages/text.
    {
        const uint8_t languages[] = {1,2,3,4,5,7,8,9,10};
        const uint8_t balls[] = {1,4,16,17,25,26};
        for (const bool fromG8 : {true, false}) {
            for (const uint8_t language : languages) {
                std::unique_ptr<Pokemon::Pokemon> source;
                if (fromG8) source = blankSWSH(0x7E000000u + language);
                else source = blankSV(0x7E100000u + language);
                configureModern(*source, 25, 0, 0x55667700u + language, 0x55667600u + language,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                u"ABCDEFGHIJKL", true);
                source->setOTName(u"TRAINER12345");
                source->setLanguage(language);
                auto raw = source->getData();
                raw[0x34] = std::byte{0x8B};
                raw[0x37] = std::byte{0xFC};
                raw[0x3A] = std::byte{0xFC};
                raw[0x40] = std::byte{0xA5};
                raw[0x44] = std::byte{0x73};
                raw[0x45] = std::byte{0x67};
                if (!fromG8) {
                    raw[0x11F] = static_cast<std::byte>(source->metLevel());
                    raw[0x4A] = raw[0x48];
                }
                source->refreshChecksum();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                Report report;
                Result result = Result::Unsupported;
                auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
                assert(result == Result::Ok && candidate);
                proveSourceUnchanged(*source, before, sourceHash);
                assert(candidate->language() == language);
                assert(candidate->nickname() == source->nickname());
                assert(candidate->otName() == source->otName());
                for (const size_t o : {size_t{0x34}, size_t{0x37}, size_t{0x3A},
                                       size_t{0x40}, size_t{0x44}, size_t{0x45}})
                    assert(candidate->getData()[o] == source->getData()[o]);
                assertSerializedReparse(*candidate);
            }

            for (const uint8_t ball : balls) {
                std::unique_ptr<Pokemon::Pokemon> source;
                if (fromG8) source = blankSWSH(0x7E200000u + ball);
                else source = blankSV(0x7E300000u + ball);
                configureModern(*source, 25, 0, 0x66778800u + ball, 0x66778900u + ball,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                u"BALLTEST", true);
                source->setBall(ball);
                if (!fromG8) {
                    source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                    source->getData()[0x4A] = source->getData()[0x48];
                }
                source->refreshChecksum();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                Result result = Result::Unsupported;
                Report report;
                auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
                assert(result == Result::Ok && candidate);
                proveSourceUnchanged(*source, before, sourceHash);
                assert(candidate->ball() == ball);
                assertSerializedReparse(*candidate);
            }

            for (const uint64_t tracker : std::array<uint64_t, 3>{
                     uint64_t{0}, uint64_t{0x0102030405060708ULL}, uint64_t{0xFFEEDDCCBBAA9988ULL}}) {
                std::unique_ptr<Pokemon::Pokemon> source;
                if (fromG8) source = blankSWSH(static_cast<uint32_t>(0x7E400000u + (tracker & 0xFF)));
                else source = blankSV(static_cast<uint32_t>(0x7E500000u + (tracker & 0xFF)));
                configureModern(*source, 25, 0, 0x77889900u, 0x77889800u,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                u"TRACKER", true);
                if (fromG8) wr64(source->getData(), 0x135, tracker);
                else {
                    wr64(source->getData(), 0x127, tracker);
                    source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                    source->getData()[0x4A] = source->getData()[0x48];
                }
                source->refreshChecksum();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                Result result = Result::Unsupported;
                Report report;
                auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
                assert(result == Result::Ok && candidate);
                proveSourceUnchanged(*source, before, sourceHash);
                assert(rd64(candidate->getData(), fromG8 ? 0x127 : 0x135) == tracker);
                assertSerializedReparse(*candidate);
            }
        }
    }


    // Closure audit: the pinned PKHeX PK8/G8PKM + PK9 models account for the complete 0x158
    // party record. Every byte is classified exactly once so a future layout edit cannot hide an
    // unexplained hole behind a broad memcpy.
    {
        struct LayoutRange { size_t first; size_t last; const char* meaning; };
        const std::vector<LayoutRange> pk8{
            {0x000,0x016,"header/identity/ability"}, {0x017,0x017,"alignment"},
            {0x018,0x019,"markings"}, {0x01A,0x01B,"alignment"},
            {0x01C,0x022,"PID/nature/fateful/Flag2/gender"}, {0x023,0x023,"alignment"},
            {0x024,0x032,"form/EV/contest/Pokerus"}, {0x033,0x033,"padding"},
            {0x034,0x03D,"ribbons/memory counts"}, {0x03E,0x03F,"padding"},
            {0x040,0x047,"ribbons/marks"}, {0x048,0x04B,"Sociability"},
            {0x04C,0x04F,"alignment"}, {0x050,0x051,"height/weight"},
            {0x052,0x057,"alignment"}, {0x058,0x08F,"nickname/moves/current HP/IV"},
            {0x090,0x090,"DynamaxLevel"}, {0x091,0x093,"alignment"},
            {0x094,0x09B,"status/Palma"}, {0x09C,0x0A7,"alignment"},
            {0x0A8,0x0C4,"HT name/gender/language/current handler"}, {0x0C5,0x0C5,"alignment"},
            {0x0C6,0x0DD,"HT id/memory/PokeJob/fullness/enjoyment"},
            {0x0DE,0x0DF,"origin/BattleVersion"}, {0x0E0,0x0E1,"legacy region extra bytes"},
            {0x0E2,0x0E2,"language"}, {0x0E3,0x0E3,"alignment"},
            {0x0E4,0x0E8,"FormArgument/AffixedRibbon"}, {0x0E9,0x0F7,"padding"},
            {0x0F8,0x114,"OT name/friendship/memory"}, {0x115,0x115,"alignment"},
            {0x116,0x11E,"OT memory/dates"}, {0x11F,0x11F,"alignment"},
            {0x120,0x126,"locations/ball/met/HyperTraining"}, {0x127,0x134,"TR records"},
            {0x135,0x13C,"HOME tracker"}, {0x13D,0x147,"alignment"},
            {0x148,0x148,"party level"}, {0x149,0x149,"alignment"},
            {0x14A,0x155,"party battle stats"}, {0x156,0x157,"DynamaxType"},
        };
        const std::vector<LayoutRange> pk9{
            {0x000,0x016,"header/identity/ability"}, {0x017,0x017,"alignment"},
            {0x018,0x019,"markings"}, {0x01A,0x01B,"alignment"},
            {0x01C,0x022,"PID/nature/fateful/gender"}, {0x023,0x023,"alignment"},
            {0x024,0x032,"form/EV/contest/Pokerus"}, {0x033,0x033,"padding"},
            {0x034,0x03D,"ribbons/memory counts"}, {0x03E,0x03F,"padding"},
            {0x040,0x047,"ribbons/marks"}, {0x048,0x04A,"height/weight/Scale"},
            {0x04B,0x057,"DLC TM records"}, {0x058,0x08F,"nickname/moves/current HP/IV"},
            {0x090,0x095,"status/Tera"}, {0x096,0x0A7,"padding"},
            {0x0A8,0x0C4,"HT name/gender/language/current handler"}, {0x0C5,0x0C5,"alignment"},
            {0x0C6,0x0D5,"HT id/memory/origin/BattleVersion/FormArgument/AffixedRibbon/language"},
            {0x0D6,0x0F7,"padding"}, {0x0F8,0x114,"OT name/friendship/memory"},
            {0x115,0x115,"alignment"}, {0x116,0x11E,"OT memory/dates"},
            {0x11F,0x11F,"ObedienceLevel"}, {0x120,0x126,"locations/ball/met/HyperTraining"},
            {0x127,0x12E,"HOME tracker"}, {0x12F,0x147,"TM records"},
            {0x148,0x148,"party level"}, {0x149,0x149,"alignment"},
            {0x14A,0x155,"party battle stats"}, {0x156,0x157,"unused PK9 party tail"},
        };
        auto proveAccounting = [](const std::vector<LayoutRange>& ranges) {
            std::array<uint8_t, 0x158> covered{};
            for (const auto& r : ranges) {
                assert(r.first <= r.last && r.last < covered.size());
                assert(r.meaning && r.meaning[0] != '\0');
                for (size_t o = r.first; o <= r.last; ++o)
                    assert(++covered[o] == 1);
            }
            for (const uint8_t count : covered) assert(count == 1);
        };
        proveAccounting(pk8);
        proveAccounting(pk9);
    }

    // All 128 ribbon/mark storage bits occupy the same byte/index space in the pinned PK8 and PK9
    // reference models. Preserve the complete shared payload, including currently unnamed RIB bits;
    // 0x3E-0x3F are authoritative padding and stay zero.
    {
        for (const bool fromG8 : {true, false}) {
            std::unique_ptr<Pokemon::Pokemon> source =
                fromG8 ? std::unique_ptr<Pokemon::Pokemon>(blankSWSH(0x7E180000u))
                       : std::unique_ptr<Pokemon::Pokemon>(blankSV(0x7E190000u));
            configureModern(*source, 25, 0, 0x12344321u, 0x12344221u,
                            static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                            u"RIBBONBITS", true);
            auto raw = source->getData();
            for (size_t o = 0x34; o <= 0x3D; ++o)
                raw[o] = static_cast<std::byte>(0xA5u ^ static_cast<uint8_t>(o));
            raw[0x3E] = raw[0x3F] = std::byte{0};
            for (size_t o = 0x40; o <= 0x47; ++o)
                raw[o] = static_cast<std::byte>(0x5Au ^ static_cast<uint8_t>(o));
            raw[fromG8 ? 0xE8 : 0xD4] = std::byte{AFFIXED_RIBBON_NONE};
            if (!fromG8) {
                raw[0x11F] = static_cast<std::byte>(source->metLevel());
                raw[0x4A] = raw[0x48];
            }
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                                     result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW),
                                     &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            for (size_t o = 0x34; o <= 0x3D; ++o)
                assert(candidate->getData()[o] == source->getData()[o]);
            assert(candidate->getData()[0x3E] == std::byte{0});
            assert(candidate->getData()[0x3F] == std::byte{0});
            for (size_t o = 0x40; o <= 0x47; ++o)
                assert(candidate->getData()[o] == source->getData()[o]);
            assertSerializedReparse(*candidate);
        }
    }

    // Complete current PK8 ball domain (1..26) survives both directions. PK9-only ids 27..37 must
    // fail closed when targeting SWSH, and a malformed PK8 source outside its own domain is refused.
    {
        for (uint8_t ball = 1; ball <= 26; ++ball) {
            for (const bool fromG8 : {true, false}) {
                std::unique_ptr<Pokemon::Pokemon> source =
                    fromG8 ? std::unique_ptr<Pokemon::Pokemon>(blankSWSH(0x7E1A0000u + ball))
                           : std::unique_ptr<Pokemon::Pokemon>(blankSV(0x7E1B0000u + ball));
                configureModern(*source, 25, 0, 0x70700000u + ball, 0x70700100u + ball,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                u"BALLDOMAIN", true);
                source->setBall(ball);
                if (!fromG8) {
                    source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                    source->getData()[0x4A] = source->getData()[0x48];
                }
                source->refreshChecksum();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                Report report;
                Result result = Result::Unsupported;
                auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                                         result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW),
                                         &report);
                assert(result == Result::Ok && candidate);
                proveSourceUnchanged(*source, before, sourceHash);
                assert(candidate->ball() == ball);
                assertSerializedReparse(*candidate);
            }
        }

        for (uint8_t ball = 27; ball <= 37; ++ball) {
            auto source = blankSV(0x7E1C0000u + ball);
            configureModern(*source, 25, 0, 0x71710000u + ball, 0x71710100u + ball,
                            static_cast<uint8_t>(GameVersion::SL), u"PK9BALL", true);
            source->setBall(ball);
            source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
            source->getData()[0x4A] = source->getData()[0x48];
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Result result = Result::Unsupported;
            Report report;
            auto candidate = convert(*source, GameVersion::SWSH, result,
                                     static_cast<uint8_t>(GameVersion::SW), &report);
            assert(!candidate && result == Result::BallNotRepresentable);
            proveSourceUnchanged(*source, before, sourceHash);
        }

        auto noBall = blankSV(0x7E1C0000u);
        configureModern(*noBall, 25, 0, 0x71710000u, 0x71710100u,
                        static_cast<uint8_t>(GameVersion::SL), u"NOBALL", true);
        noBall->setBall(0);
        noBall->getData()[0x11F] = static_cast<std::byte>(noBall->metLevel());
        noBall->getData()[0x4A] = noBall->getData()[0x48];
        noBall->refreshChecksum();
        const auto noBallBefore = nativeBytes(*noBall);
        const auto noBallHash = hashBytes(noBallBefore);
        Result noBallResult = Result::Unsupported;
        Report noBallReport;
        auto noBallCandidate = convert(*noBall, GameVersion::SWSH, noBallResult,
                                       static_cast<uint8_t>(GameVersion::SW), &noBallReport);
        assert(!noBallCandidate && noBallResult == Result::BallNotRepresentable);
        proveSourceUnchanged(*noBall, noBallBefore, noBallHash);

        auto malformed = blankSWSH(0x7E1D001Bu);
        configureModern(*malformed, 25, 0, 0x7272001Bu, 0x7272011Bu,
                        static_cast<uint8_t>(GameVersion::SW), u"BADPK8BALL", true);
        malformed->setBall(27);
        malformed->refreshChecksum();
        const auto before = nativeBytes(*malformed);
        const auto sourceHash = hashBytes(before);
        Result result = Result::Unsupported;
        Report report;
        auto candidate = convert(*malformed, GameVersion::SV, result,
                                 static_cast<uint8_t>(GameVersion::SL), &report);
        assert(!candidate && result == Result::BallNotRepresentable);
        proveSourceUnchanged(*malformed, before, sourceHash);
    }

    // Handler/history fields shared at identical offsets must survive untouched; Version,
    // BattleVersion and FormArgument are relocated explicitly between PK8 and PK9. This is a
    // representation-preserving conversion, not a synthetic HOME trade-handler update.
    {
        for (const bool fromG8 : {true, false}) {
            std::unique_ptr<Pokemon::Pokemon> source =
                fromG8 ? std::unique_ptr<Pokemon::Pokemon>(blankSWSH(0x7E1E0001u))
                       : std::unique_ptr<Pokemon::Pokemon>(blankSV(0x7E1F0001u));
            configureModern(*source, 869 /*Alcremie*/, 0, 0x81818181u, 0x81818081u,
                            static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                            u"ALCREMIE", true);
            const auto& pi = Pokemon::getPersonalInfo(869, 0);
            assert((pi.presence & Pokemon::PERSONAL_GAME_SWSH) != 0);
            assert((pi.presence & Pokemon::PERSONAL_GAME_SV) != 0);
            auto raw = source->getData();
            raw[0xC2] = std::byte{1}; raw[0xC3] = std::byte{2}; raw[0xC4] = std::byte{1};
            raw[0xC6] = std::byte{0x34}; raw[0xC7] = std::byte{0x12};
            raw[0xC8] = std::byte{177}; raw[0xC9] = std::byte{3}; raw[0xCA] = std::byte{7};
            raw[0xCB] = std::byte{4}; raw[0xCC] = std::byte{0x78}; raw[0xCD] = std::byte{0x56};
            raw[0x112] = std::byte{166}; raw[0x113] = std::byte{2}; raw[0x114] = std::byte{6};
            raw[0x116] = std::byte{0xBC}; raw[0x117] = std::byte{0x9A}; raw[0x118] = std::byte{5};
            raw[0x119] = std::byte{24}; raw[0x11A] = std::byte{7}; raw[0x11B] = std::byte{14};
            raw[0x11C] = std::byte{25}; raw[0x11D] = std::byte{8}; raw[0x11E] = std::byte{15};
            raw[0x120] = std::byte{0x11}; raw[0x121] = std::byte{0x22};
            raw[0x122] = std::byte{0x33}; raw[0x123] = std::byte{0x44};
            raw[0x124] = std::byte{16}; raw[0x125] = std::byte{25}; raw[0x126] = std::byte{0x15};
            const size_t versionOffset = fromG8 ? 0xDE : 0xCE;
            const size_t battleOffset = fromG8 ? 0xDF : 0xCF;
            const size_t formArgOffset = fromG8 ? 0xE4 : 0xD0;
            const size_t affixOffset = fromG8 ? 0xE8 : 0xD4;
            raw[versionOffset] = static_cast<std::byte>(
                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL));
            raw[battleOffset] = static_cast<std::byte>(
                static_cast<uint8_t>(fromG8 ? GameVersion::SH : GameVersion::VL));
            wr32s(raw, formArgOffset, 3u); // Alcremie decoration/form argument is meaningful.
            raw[0x34] = std::byte{0x01};
            raw[affixOffset] = std::byte{0};
            if (!fromG8) {
                raw[0x11F] = raw[0x125];
                raw[0x4A] = raw[0x48];
            }
            source->refreshChecksum();

            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                                     result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW),
                                     &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            const auto dst = candidate->getData();
            for (size_t o = 0xC2; o <= 0xCD; ++o)
                assert(dst[o] == raw[o]);
            for (size_t o = 0x112; o <= 0x11E; ++o)
                if (o != 0x115) assert(dst[o] == raw[o]);
            for (size_t o = 0x120; o <= 0x126; ++o)
                assert(dst[o] == raw[o]);
            const size_t dstVersion = fromG8 ? 0xCE : 0xDE;
            const size_t dstBattle = fromG8 ? 0xCF : 0xDF;
            const size_t dstFormArg = fromG8 ? 0xD0 : 0xE4;
            const size_t dstAffix = fromG8 ? 0xD4 : 0xE8;
            assert(dst[dstVersion] == raw[versionOffset]);
            assert(dst[dstBattle] == raw[battleOffset]);
            assert(rd32(dst, dstFormArg) == 3u);
            assert(static_cast<uint8_t>(dst[dstAffix]) == 0);
            assertSerializedReparse(*candidate);
        }
    }

    // Route-specific PP drift: dynamically find a shared species+move whose source-generation
    // maximum exceeds the destination-generation maximum, then prove the converter clamps only
    // the destination candidate and reports the deterministic adaptation.
    {
        struct PPCandidate {
            uint16_t species = 0;
            uint16_t move = 0;
            uint8_t sourceMax = 0;
            uint8_t destinationMax = 0;
        };
        auto findPPDrop = [](GameVersion sourceGroup, GameVersion destinationGroup) {
            PPCandidate found{};
            for (uint16_t sp = 1; sp <= Pokemon::PERSONAL_MAX_SPECIES && found.species == 0; ++sp) {
                const auto& pi = Pokemon::getPersonalInfo(sp, 0);
                const bool presentSource =
                    sourceGroup == GameVersion::SWSH
                        ? (pi.presence & Pokemon::PERSONAL_GAME_SWSH) != 0
                        : (pi.presence & Pokemon::PERSONAL_GAME_SV) != 0;
                const bool presentDestination =
                    destinationGroup == GameVersion::SWSH
                        ? (pi.presence & Pokemon::PERSONAL_GAME_SWSH) != 0
                        : (pi.presence & Pokemon::PERSONAL_GAME_SV) != 0;
                if (!presentSource || !presentDestination) continue;
                for (uint16_t move = 1; move <= Pokemon::LEARN_MAX_MOVE_ID; ++move) {
                    if (!Pokemon::isLearnable(sp, 0, sourceGroup, move) ||
                        !Pokemon::isLearnable(sp, 0, destinationGroup, move))
                        continue;
                    const uint8_t srcMax = Names::getMoveMaxPP(move, 0, sourceGroup);
                    const uint8_t dstMax = Names::getMoveMaxPP(move, 0, destinationGroup);
                    if (srcMax > dstMax && dstMax != 0) {
                        found = PPCandidate{sp, move, srcMax, dstMax};
                        break;
                    }
                }
            }
            return found;
        };

        const std::array<std::pair<GameVersion, GameVersion>, 2> directions{{
            {GameVersion::SWSH, GameVersion::SV},
            {GameVersion::SV, GameVersion::SWSH},
        }};
        size_t provenDirections = 0;
        for (const auto& [sourceGroup, destinationGroup] : directions) {
            const auto pp = findPPDrop(sourceGroup, destinationGroup);
            if (pp.species == 0) continue; // only require a fixture where current tables contain drift
            ++provenDirections;
            const bool fromG8 = sourceGroup == GameVersion::SWSH;
            std::unique_ptr<Pokemon::Pokemon> source;
            if (fromG8) source = blankSWSH(0x7E600000u + pp.species);
            else source = blankSV(0x7E700000u + pp.species);
            configureModern(*source, pp.species, 0, 0x99000000u + pp.species,
                            0x99000100u + pp.species,
                            static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                            u"PPCLAMP", true);
            source->setMove(0, pp.move);
            source->setMovePPUps(0, 0);
            source->setMovePP(0, pp.sourceMax);
            if (!fromG8) {
                source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                source->getData()[0x4A] = source->getData()[0x48];
            }
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);
            Report report;
            Result result = Result::Unsupported;
            auto candidate = convert(*source, destinationGroup, result,
                static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*source, before, sourceHash);
            assert(report.hasAdaptation(Adaptation::MovePPClamped));
            assert(candidate->move(0) == pp.move);
            assert(candidate->movePP(0) == pp.destinationMax);
            assertSerializedReparse(*candidate);
        }
        assert(provenDirections >= 1);
    }

    // Both formats use the same modern UTF-16 text representation. Exercise non-Latin scripts
    // for the supported modern language IDs so language/text coverage is not ASCII-only.
    {
        struct TextCase {
            uint8_t language;
            const char16_t* nickname;
            const char16_t* ot;
        };
        const TextCase textCases[] = {
            {1,  u"ピカチュウ", u"トレーナー"},
            {8,  u"피카츄",     u"트레이너"},
            {9,  u"皮卡丘",     u"训练家"},
            {10, u"皮卡丘",     u"訓練家"},
        };
        for (const bool fromG8 : {true, false}) {
            for (const auto& tc : textCases) {
                std::unique_ptr<Pokemon::Pokemon> source;
                if (fromG8) source = blankSWSH(0x7E800000u + tc.language);
                else source = blankSV(0x7E900000u + tc.language);
                configureModern(*source, 25, 0, 0xAA000000u + tc.language,
                                0xAA000100u + tc.language,
                                static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                                tc.nickname, true);
                source->setOTName(tc.ot);
                source->setLanguage(tc.language);
                if (!fromG8) {
                    source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                    source->getData()[0x4A] = source->getData()[0x48];
                }
                source->refreshChecksum();
                const auto before = nativeBytes(*source);
                const auto sourceHash = hashBytes(before);
                Report report;
                Result result = Result::Unsupported;
                auto candidate = convert(*source, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                    result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW), &report);
                assert(result == Result::Ok && candidate);
                proveSourceUnchanged(*source, before, sourceHash);
                assert(candidate->language() == tc.language);
                assert(candidate->nickname() == source->nickname());
                assert(candidate->otName() == source->otName());
                assertSerializedReparse(*candidate);
            }
        }
    }

    // Modern text closure: 12 UTF-16 code units plus NUL is valid, while raw fields with no
    // terminator or an unpaired surrogate fail preflight/conversion without touching the source.
    {
        for (const bool fromG8 : {true, false}) {
            std::unique_ptr<Pokemon::Pokemon> maxText =
                fromG8 ? std::unique_ptr<Pokemon::Pokemon>(blankSWSH(0x7EA00001u))
                       : std::unique_ptr<Pokemon::Pokemon>(blankSV(0x7EA10001u));
            configureModern(*maxText, 25, 0, 0xABABABABu, 0xABABAAABu,
                            static_cast<uint8_t>(fromG8 ? GameVersion::SW : GameVersion::SL),
                            u"ABCDEFGHIJKL", true); // exactly 12
            maxText->setOTName(u"TRAINER12345"); // exactly 12
            if (!fromG8) {
                maxText->getData()[0x11F] = static_cast<std::byte>(maxText->metLevel());
                maxText->getData()[0x4A] = maxText->getData()[0x48];
            }
            maxText->refreshChecksum();
            const auto before = nativeBytes(*maxText);
            const auto sourceHash = hashBytes(before);
            Result result = Result::Unsupported;
            Report report;
            auto candidate = convert(*maxText, fromG8 ? GameVersion::SV : GameVersion::SWSH,
                                     result, static_cast<uint8_t>(fromG8 ? GameVersion::SL : GameVersion::SW),
                                     &report);
            assert(result == Result::Ok && candidate);
            proveSourceUnchanged(*maxText, before, sourceHash);
            assert(candidate->nickname() == maxText->nickname());
            assert(candidate->otName() == maxText->otName());
            assertSerializedReparse(*candidate);
        }

        auto unterminated = blankSWSH(0x7EA20001u);
        configureModern(*unterminated, 25, 0, 0xBCBCBCBCu, 0xBCBCBDBCu,
                        static_cast<uint8_t>(GameVersion::SW), u"VALID", true);
        auto u = unterminated->getData();
        for (size_t i = 0; i < 13; ++i) {
            u[0x58 + i * 2] = std::byte{'A'};
            u[0x58 + i * 2 + 1] = std::byte{0};
        }
        unterminated->refreshChecksum();
        const auto ub = nativeBytes(*unterminated);
        const auto uh = hashBytes(ub);
        auto pre = preflightConvert(*unterminated, GameVersion::SV,
                                    static_cast<uint8_t>(GameVersion::SL));
        assert(!pre.candidateAvailable && pre.result == Result::TextNotRepresentable);
        proveSourceUnchanged(*unterminated, ub, uh);

        auto surrogate = blankSV(0x7EA30001u);
        configureModern(*surrogate, 25, 0, 0xCDCDCDCDu, 0xCDCDCCCDu,
                        static_cast<uint8_t>(GameVersion::SL), u"VALID", true);
        auto s = surrogate->getData();
        s[0x58] = std::byte{0x00}; s[0x59] = std::byte{0xD8}; // lone high surrogate U+D800
        s[0x5A] = std::byte{0}; s[0x5B] = std::byte{0};
        s[0x11F] = static_cast<std::byte>(surrogate->metLevel());
        s[0x4A] = s[0x48];
        surrogate->refreshChecksum();
        const auto sb = nativeBytes(*surrogate);
        const auto sh = hashBytes(sb);
        pre = preflightConvert(*surrogate, GameVersion::SWSH,
                               static_cast<uint8_t>(GameVersion::SW));
        assert(!pre.candidateAvailable && pre.result == Result::TextNotRepresentable);
        proveSourceUnchanged(*surrogate, sb, sh);
    }

    // Requested exact round trips. Equality is required only for shared semantics; Tera and
    // destination-native synthesized fields remain explicitly explained by the fidelity reports.
    {
        struct RoundTrip {
            bool startG8;
            uint8_t sourceVersion;
            uint8_t middleVersion;
        };
        const RoundTrip cases[] = {
            {true,  static_cast<uint8_t>(GameVersion::SW), static_cast<uint8_t>(GameVersion::SL)},
            {true,  static_cast<uint8_t>(GameVersion::SH), static_cast<uint8_t>(GameVersion::VL)},
            {false, static_cast<uint8_t>(GameVersion::SL), static_cast<uint8_t>(GameVersion::SW)},
            {false, static_cast<uint8_t>(GameVersion::VL), static_cast<uint8_t>(GameVersion::SH)},
        };
        for (size_t i = 0; i < std::size(cases); ++i) {
            const auto& rt = cases[i];
            std::unique_ptr<Pokemon::Pokemon> source;
            if (rt.startG8) source = blankSWSH(0x7F000000u + static_cast<uint32_t>(i));
            else source = blankSV(0x7F100000u + static_cast<uint32_t>(i));
            configureModern(*source, 25, 0, 0x88990000u + static_cast<uint32_t>(i),
                            0x88990100u + static_cast<uint32_t>(i), rt.sourceVersion,
                            u"ROUNDTRIP", true);
            const uint64_t tracker = 0x1234000000000000ULL + i;
            if (rt.startG8) wr64(source->getData(), 0x135, tracker);
            else {
                wr64(source->getData(), 0x127, tracker);
                source->getData()[0x11F] = static_cast<std::byte>(source->metLevel());
                source->getData()[0x4A] = source->getData()[0x48];
            }
            source->refreshChecksum();
            const auto before = nativeBytes(*source);
            const auto sourceHash = hashBytes(before);

            Result result = Result::Unsupported;
            Report first;
            auto middle = convert(*source, rt.startG8 ? GameVersion::SV : GameVersion::SWSH,
                                  result, rt.middleVersion, &first);
            assert(result == Result::Ok && middle);
            proveSourceUnchanged(*source, before, sourceHash);
            const auto middleBefore = nativeBytes(*middle);
            const auto middleHash = hashBytes(middleBefore);

            Report second;
            auto back = convert(*middle, rt.startG8 ? GameVersion::SWSH : GameVersion::SV,
                                result, rt.sourceVersion, &second);
            assert(result == Result::Ok && back);
            proveSourceUnchanged(*middle, middleBefore, middleHash);
            assert(back->speciesID() == source->speciesID());
            assert(back->form() == source->form());
            assert(back->pid() == source->pid());
            assert(back->encryptionConstant() == source->encryptionConstant());
            if (rt.startG8) {
                assert(back->originGame() == source->originGame());
            } else {
                const uint8_t remapped = static_cast<uint8_t>(
                    rt.sourceVersion == static_cast<uint8_t>(GameVersion::VL)
                        ? GameVersion::SH : GameVersion::SW);
                // A standalone PK8 payload stores the HOME remap, not the original S/V native
                // location/version. F13 retains the true source origin, but a later reverse
                // conversion cannot reconstruct the original raw S/V met location from PK8 alone.
                assert(middle->originGame() == remapped);
                assert(back->originGame() == remapped);
                assert(first.sourceOriginVersion == rt.sourceVersion);
                assert(first.destinationEntityOriginVersion == remapped);
                assert(first.hasAdaptation(Adaptation::TargetHistoryRepresentationRemapped));
            }
            assert(back->nickname() == source->nickname());
            assert(back->otName() == source->otName());
            assert(back->id32() == source->id32());
            assert(back->exp() == source->exp());
            assert(back->nature() == source->nature());
            assert(back->statNature() == source->statNature());
            assert(rd64(back->getData(), rt.startG8 ? 0x135 : 0x127) == tracker);
            if (rt.startG8) {
                assert(first.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
                assert(second.hasLoss(Loss::TeraDataDropped));
            } else {
                assert(first.hasLoss(Loss::TeraDataDropped));
                assert(first.hasAdaptation(Adaptation::TargetHistoryRepresentationRemapped));
                assert(second.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
            }
            assertSerializedReparse(*back);
        }
    }

    std::cout << "F05-F13 production conversion entity goldens: PASS\n";
}
