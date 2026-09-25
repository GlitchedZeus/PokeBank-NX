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

    std::cout << "F05-F13 production conversion entity goldens: PASS\n";
}
