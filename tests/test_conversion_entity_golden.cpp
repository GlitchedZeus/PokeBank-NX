#include "Conversion/Convert.h"
#include "Conversion/Fidelity.h"
#include "Conversion/Gen3PidSearch.h"
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption8SWSH.h"
#include "Encryption/Encryption8BDSP.h"
#include "Encryption/Encryption9SV.h"
#include "Encryption/Encryption9LZA.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon8SWSH.h"
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
        case GameVersion::SWSH:
            encrypted = Encryption::encryptArray8SWSH(data, pk.encryptionConstant());
            break;
        case GameVersion::BDSP:
            encrypted = Encryption::encryptArray8BDSP(data, pk.encryptionConstant());
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
        case GameVersion::SWSH: return std::make_unique<Pokemon::Pokemon8SWSH>(bytes);
        case GameVersion::BDSP: return std::make_unique<Pokemon::Pokemon8BDSP>(bytes);
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

            Report upReport;
            auto roundTrip = convert(*pk3, GameVersion::SWSH, result,
                                     static_cast<uint8_t>(GameVersion::SW), &upReport);
            assert(result == Result::Ok && roundTrip);
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
        source->getData()[0x4B] = std::byte{1}; // Z-A divergent field
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
        assert(static_cast<uint8_t>(sv->getData()[0x4B]) == 0);
        assert(static_cast<uint8_t>(sv->getData()[0x94]) != 0);
        assertSerializedReparse(*sv);

        std::cout << "fixture f09-za-sv-alpha source-sha256=" << hexHash(sourceHash) << "\n";
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

    std::cout << "F05-F13 production conversion entity goldens: PASS\n";
}
