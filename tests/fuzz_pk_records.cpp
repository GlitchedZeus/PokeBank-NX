// QA fuzz target: raw PK3 / PK4 / PK5 record decoders (stored and party sizes, and every other
// size, which must fail closed). PK1 and PK2 have no raw-record entry point in the app; their
// records are decoded by the SAV1/SAV2 loaders and wrapped by Pokemon1ReadOnly/Pokemon2ReadOnly,
// which fuzz_gen1_sav / fuzz_gen2_sav exercise.
//
// Round-trip contract: a record that decodes as checksum-valid re-encrypts to the exact input bytes.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Encryption/Encryption5.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon5ReadOnly.h"

#include <array>
#include <memory>

namespace G3 = PokeVault::Integration::Gen3;
namespace G5 = PokeVault::Integration::Gen5;

namespace {

std::span<const std::byte> asBytes(std::span<const uint8_t> data) {
    return std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()), data.size());
}

void pk3(std::span<const uint8_t> data) {
    Pokemon::Pokemon3FRLG mon(asBytes(data));
    const bool shape = data.size() == Encryption::SIZE_STORED3_FRLG || data.size() == Encryption::SIZE_PARTY3_FRLG;
    QA_CHECK(mon.inputValid() == shape);
    if (!shape) QA_CHECK(!mon.checksumValid());
    (void)mon.speciesID();
    (void)mon.level();
    (void)mon.nickname();
    if (shape && mon.checksumValid()) {
        qafuzz::noteAccepted();
        const auto canonical = mon.getData();
        std::unique_ptr<std::byte[]> again(Encryption::encryptArray3FRLG(canonical));
        QA_CHECK(std::equal(again.get(), again.get() + data.size(), asBytes(data).begin()));
    }
    std::array<uint8_t, 80> raw{};
    std::copy_n(data.begin(), std::min<size_t>(data.size(), raw.size()), raw.begin());
    std::string error;
    if (G3::StagedPokemonEditor::decodeRecord(raw, error)) qafuzz::noteAccepted();
}

void pk4(std::span<const uint8_t> data) {
    for (const auto group : {Enums::GameVersion::Invalid, Enums::GameVersion::DP, Enums::GameVersion::Pt,
                             Enums::GameVersion::HGSS}) {
        Pokemon::Pokemon4ReadOnly mon(asBytes(data), group);
        const bool shape = data.size() == Encryption::SIZE_STORED4 || data.size() == Encryption::SIZE_PARTY4;
        if (!shape) QA_CHECK(!mon.sizeValid() && !mon.valid());
        (void)mon.nickname();
        (void)mon.originalTrainerName();
        (void)mon.battleStats();
        (void)mon.ivs();
        if (mon.valid()) {
            qafuzz::noteAccepted();
            if (!mon.empty()) (void)mon.personal();
            // An all-zero slot is the accepted blank-slot convention (Pokemon4ReadOnly keeps it
            // unencrypted, like PK5); it has no ciphertext to round-trip to.
            const bool allZero = std::all_of(data.begin(), data.end(), [](uint8_t b) { return b == 0; });
            const auto again = Encryption::encryptArray4(mon.decryptedBytes());
            if (!allZero)
                QA_CHECK(std::equal(again.begin(), again.end(), asBytes(data).begin(), asBytes(data).end()));
        }
    }
}

void pk5(std::span<const uint8_t> data) {
    G5::Pokemon5ReadOnly mon(data);
    const bool shape = G5::Crypto::recordSizeValid(data.size());
    if (!shape) QA_CHECK(!mon.valid() && mon.pid() == 0 && mon.species() == 0);
    (void)mon.shiny();
    (void)mon.ivs();
    (void)mon.partyLevel();
    if (mon.valid()) {
        qafuzz::noteAccepted();
        const auto decoded = mon.decodedBytes();
        const auto again = G5::Crypto::encryptCandidate(std::vector<uint8_t>(decoded.begin(), decoded.end()));
        const bool allZero = std::all_of(data.begin(), data.end(), [](uint8_t b) { return b == 0; });
        if (!allZero) QA_CHECK(again.size() == data.size() && std::equal(again.begin(), again.end(), data.begin()));
    }
}

void runOne(std::span<const uint8_t> data) {
    pk3(data);
    pk4(data);
    pk5(data);
}

qafuzz::Bytes toBytes(std::span<const std::byte> in) {
    qafuzz::Bytes out(in.size());
    for (size_t i = 0; i < in.size(); ++i) out[i] = static_cast<uint8_t>(in[i]);
    return out;
}

qafuzz::Bytes pk3Seed(bool party) {
    std::vector<std::byte> canonical(party ? Encryption::SIZE_PARTY3_FRLG : Encryption::SIZE_STORED3_FRLG,
                                     std::byte{0});
    auto put16 = [&](size_t o, uint16_t v) {
        canonical[o] = std::byte(v & 0xFF);
        canonical[o + 1] = std::byte(v >> 8);
    };
    put16(0x00, 0x5678); put16(0x02, 0x1234);   // PID
    put16(0x04, 12345); put16(0x06, 54321);     // OT ID
    canonical[0x13] = std::byte{0x02};          // language: English
    put16(0x20, 25);                            // canonical Growth block species
    put16(0x1C, Encryption::checksum3FRLG(canonical));
    std::unique_ptr<std::byte[]> encrypted(Encryption::encryptArray3FRLG(canonical));
    return toBytes(std::span<const std::byte>(encrypted.get(), canonical.size()));
}

std::vector<qafuzz::Bytes> seeds() {
    return {
        pk3Seed(false), pk3Seed(true),
        toBytes(qasynth::gen4::stored()), toBytes(qasynth::gen4::party()),
        qasynth::gen5::pk5(0xCAFEF00Du, 25, false), qasynth::gen5::pk5(0x01020304u, 494, true),
    };
}

// Forge: recompute whichever record checksum matches the input length, then re-encrypt.
void forge(qafuzz::Bytes& data) {
    if (data.size() == Encryption::SIZE_STORED4 || data.size() == Encryption::SIZE_PARTY4) {
        // Reuse the production checksum (Pokemon4ReadOnly::calculatedChecksum), never a re-derived one.
        const Pokemon::Pokemon4ReadOnly mon(asBytes(data));
        const auto decrypted = mon.decryptedBytes();
        if (decrypted.size() != data.size()) return;
        std::vector<std::byte> plain(decrypted.begin(), decrypted.end());
        const uint16_t sum = mon.calculatedChecksum();
        plain[6] = std::byte(sum & 0xFF);
        plain[7] = std::byte(sum >> 8);
        data = toBytes(Encryption::encryptArray4(plain));
    } else if (G5::Crypto::recordSizeValid(data.size())) {
        auto plain = G5::Crypto::decrypt(data);
        if (plain.size() != data.size()) return;
        plain[4] = plain[5] = 0;
        G5::Crypto::write16(plain, 6, G5::Crypto::checksum(plain));
        data = G5::Crypto::encryptCandidate(plain);
    }
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "pk-records", runOne, seeds, forge, {0x08, 0x09, 0x8C, 0x54}, {{0x08, 0x20, 0x08}, {0x20, 0x38, 0x18}},
    };
    return target;
}
