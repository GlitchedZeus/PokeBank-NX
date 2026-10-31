// PK6 codec: Decrypt67/Encrypt67, block shuffle, checksum and strict decode.
// Differentially checked against PKSM-Core's independent header-only
// implementation (vendor/PKSM-Core include/utils/crypto.hpp @ aa22d7a4).
#include "Integration/Gen6/Gen6PK6.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

#include "utils/crypto.hpp"

#include <algorithm>
#include <random>

namespace PK6 = PokeVault::Integration::Gen6::PK6;

namespace {

// PKSM-Core's PK6::decrypt sequence (source/pkx/PK6.cpp) expressed with its crypto.hpp helpers.
std::vector<uint8_t> pksmDecrypt(std::vector<uint8_t> data) {
    const uint32_t ec = PK6::read32(data, 0);
    pksm::crypto::pkm::crypt<PK6::StoredSize - 8>(data.data() + 8, ec);
    if (data.size() == PK6::PartySize)
        pksm::crypto::pkm::crypt<PK6::PartySize - PK6::StoredSize>(data.data() + PK6::StoredSize, ec);
    pksm::crypto::pkm::blockShuffle<PK6::BlockSize>(data.data() + 8, static_cast<uint8_t>((ec >> 13) & 31));
    return data;
}
std::vector<uint8_t> pksmEncrypt(std::vector<uint8_t> data) {
    const uint32_t ec = PK6::read32(data, 0);
    pksm::crypto::pkm::blockShuffle<PK6::BlockSize>(
        data.data() + 8, pksm::crypto::pkm::InvertedBlockPositions[(ec >> 13) & 31]);
    pksm::crypto::pkm::crypt<PK6::StoredSize - 8>(data.data() + 8, ec);
    if (data.size() == PK6::PartySize)
        pksm::crypto::pkm::crypt<PK6::PartySize - PK6::StoredSize>(data.data() + PK6::StoredSize, ec);
    return data;
}

void testTablesMatchPksm() {
    for (size_t i = 0; i < PK6::BlockPosition.size(); ++i)
        GEN6_CHECK(PK6::BlockPosition[i] == pksm::crypto::pkm::BlockPositions[i]);
    for (size_t i = 0; i < PK6::BlockPositionInvert.size(); ++i)
        GEN6_CHECK(PK6::BlockPositionInvert[i] == pksm::crypto::pkm::InvertedBlockPositions[i]);
    // Duplicated rows 24..31 in both references equal rows 0..7, which is what % 24 relies on.
    for (size_t sv = 24; sv < 32; ++sv) {
        for (size_t i = 0; i < 4; ++i)
            GEN6_CHECK(pksm::crypto::pkm::BlockPositions[sv * 4 + i] == PK6::BlockPosition[(sv % 24) * 4 + i]);
        GEN6_CHECK(pksm::crypto::pkm::InvertedBlockPositions[sv] == PK6::BlockPositionInvert[sv % 24]);
    }
    // Every inverse row really inverts its permutation.
    for (uint32_t sv = 0; sv < 24; ++sv) {
        std::vector<uint8_t> blocks(PK6::BlockSize * 4);
        for (size_t i = 0; i < blocks.size(); ++i) blocks[i] = static_cast<uint8_t>(i);
        const auto before = blocks;
        PK6::shuffle(blocks, sv);
        PK6::shuffle(blocks, PK6::BlockPositionInvert[sv]);
        GEN6_CHECK(blocks == before);
    }
}

void testDifferentialCrypto() {
    std::mt19937 rng(6);
    for (uint32_t sv = 0; sv < 32; ++sv) {
        for (const size_t size : {PK6::StoredSize, PK6::PartySize}) {
            std::vector<uint8_t> plain(size);
            for (auto& b : plain) b = static_cast<uint8_t>(rng());
            // Force the shuffle value under test into bits 13..17 of the EC.
            uint32_t ec = static_cast<uint32_t>(rng());
            ec = (ec & ~(31u << 13)) | (sv << 13);
            PK6::write32(plain, 0, ec);
            const auto mine = PK6::encrypt(plain);
            GEN6_CHECK(mine == pksmEncrypt(plain));
            GEN6_CHECK(PK6::decrypt(mine) == plain);
            GEN6_CHECK(pksmDecrypt(mine) == plain);
            GEN6_CHECK(PK6::decrypt(mine) == pksmDecrypt(mine));
            // The 8-byte header is never encrypted.
            GEN6_CHECK(std::equal(mine.begin(), mine.begin() + 8, plain.begin()));
        }
    }
}

void testStrictDecodeAndRoundTrip() {
    Gen6Fixture::MonSpec spec;
    for (const bool party : {false, true}) {
        const auto encrypted = Gen6Fixture::encryptedMon(spec, party);
        PK6::DecodeStatus status{};
        const auto mon = PK6::Pokemon6::decode(encrypted, &status);
        GEN6_CHECK(mon.has_value());
        GEN6_CHECK(status == PK6::DecodeStatus::Ok);
        if (!mon) continue;
        // decrypt -> parse -> serialize -> re-encrypt is byte-identical.
        GEN6_CHECK(mon->serialize() == encrypted);
        GEN6_CHECK(mon->species() == 25);
        GEN6_CHECK(mon->tid() == 12345 && mon->sid() == 54321);
        GEN6_CHECK(mon->pid() == 0x9ABCDEF0 && mon->encryptionConstant() == 0x12345678);
        GEN6_CHECK(mon->nickname() == u"SYNTH");
        GEN6_CHECK(mon->originalTrainerName() == u"TESTER");
        GEN6_CHECK(mon->iv(0) == 31 && mon->iv(1) == 16 && mon->iv(5) == 30);
        GEN6_CHECK(mon->ev(0) == 4 && mon->ev(1) == 8);
        GEN6_CHECK(mon->ball() == 4 && mon->metLevel() == 5 && mon->language() == 2);
        GEN6_CHECK(mon->partyForm() == party);
        GEN6_CHECK(mon->statLevel() == (party ? 12 : 0));
        GEN6_CHECK(!mon->isEgg());

        // Every single-bit corruption of the encrypted body fails strictly.
        for (const size_t at : {size_t{0x08}, size_t{0x40}, size_t{0x90}, size_t{0xE7}}) {
            auto bad = encrypted;
            bad[at] ^= 0x01;
            GEN6_CHECK(!PK6::Pokemon6::decode(bad, &status));
            GEN6_CHECK(status == PK6::DecodeStatus::BadChecksum || status == PK6::DecodeStatus::BadSanity);
        }
        auto badChecksum = encrypted;
        badChecksum[0x06] ^= 0xFF;
        GEN6_CHECK(!PK6::Pokemon6::decode(badChecksum, &status) && status == PK6::DecodeStatus::BadChecksum);
        auto plain = Gen6Fixture::plainMon(spec, party);
        PK6::write16(plain, 0x04, 1);
        PK6::write16(plain, 0x06, PK6::checksum(plain));
        GEN6_CHECK(!PK6::Pokemon6::decode(PK6::encrypt(plain), &status) && status == PK6::DecodeStatus::BadSanity);
    }
    // Unknown sizes are rejected, never padded or truncated.
    PK6::DecodeStatus status{};
    for (const size_t size : {size_t{0}, size_t{136}, size_t{220}, size_t{231}, size_t{233}, size_t{259},
                              size_t{261}, size_t{0x148}}) {
        std::vector<uint8_t> raw(size, 0);
        GEN6_CHECK(!PK6::Pokemon6::decode(raw, &status));
        GEN6_CHECK(status == PK6::DecodeStatus::UnknownSize);
        GEN6_CHECK(PK6::decrypt(raw).empty() && PK6::encrypt(raw).empty());
    }
    // Raw zero bytes are NOT a valid PK6 (pv 0 still decrypts to keystream).
    GEN6_CHECK(!PK6::Pokemon6::decode(std::vector<uint8_t>(PK6::StoredSize, 0)));
    // The encrypted blank decodes to an empty, all-zero record.
    const auto blank = PK6::Pokemon6::decode(PK6::Pokemon6::blank().serialize());
    GEN6_CHECK(blank && blank->empty() && blank->allZero());
}

void testFieldMutatorsAndDerived() {
    Gen6Fixture::MonSpec spec;
    auto mon = *PK6::Pokemon6::decode(Gen6Fixture::encryptedMon(spec, false));
    const auto before = std::vector<uint8_t>(mon.plain().begin(), mon.plain().end());
    mon.setIv(3, 7);
    GEN6_CHECK(mon.iv(3) == 7 && mon.iv(2) == 15 && mon.iv(4) == 2 && !mon.isEgg() && !mon.isNicknamed());
    mon.setNicknamedFlag(true);
    GEN6_CHECK(mon.isNicknamed() && mon.iv(5) == 30);
    mon.setRawName(PK6::NicknameOffset, u"ABCDEFGHIJKLMNOP");  // capped at 12
    GEN6_CHECK(mon.nickname() == u"ABCDEFGHIJKL");
    GEN6_CHECK(PK6::read16(mon.plain(), 0x58) == 0);  // terminator word stays zero
    mon.setOriginalTrainerGender(1);
    GEN6_CHECK(mon.originalTrainerGender() == 1 && mon.metLevel() == 5);
    mon.setFatefulEncounter(true);
    GEN6_CHECK(mon.fatefulEncounter() && mon.form() == 0 && mon.gender() == 0);
    // Serialize recomputes the checksum and stays strictly decodable.
    const auto again = PK6::Pokemon6::decode(mon.serialize());
    GEN6_CHECK(again && again->iv(3) == 7 && again->nickname() == u"ABCDEFGHIJKL");
    GEN6_CHECK(std::vector<uint8_t>(mon.plain().begin(), mon.plain().end()) != before);

    // Shiny: PSV == TSV (xor < 16), G6PKM.
    spec.tid = 0x1234; spec.sid = 0x0000; spec.pid = 0x12340005;  // xor = 0x1234^0x1234^0x0005 = 5
    GEN6_CHECK(PK6::Pokemon6::decode(Gen6Fixture::encryptedMon(spec, false))->shiny());
    spec.pid = 0x12340010;  // xor = 16
    GEN6_CHECK(!PK6::Pokemon6::decode(Gen6Fixture::encryptedMon(spec, false))->shiny());

    // Party -> boxed keeps exactly the 232-byte stored prefix.
    const auto party = *PK6::Pokemon6::decode(Gen6Fixture::encryptedMon(Gen6Fixture::MonSpec{}, true));
    const auto boxed = party.boxed();
    GEN6_CHECK(boxed.size() == PK6::StoredSize && !boxed.partyForm());
    GEN6_CHECK(std::equal(boxed.plain().begin(), boxed.plain().end(), party.plain().begin()));
    GEN6_CHECK(PK6::Pokemon6::decode(boxed.serialize()).has_value());
}

} // namespace

int main() {
    testTablesMatchPksm();
    testDifferentialCrypto();
    testStrictDecodeAndRoundTrip();
    testFieldMutatorsAndDerived();
    return Gen6Test::finish("test_gen6_pk6");
}
