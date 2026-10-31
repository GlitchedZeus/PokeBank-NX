// PokeBank NX application QA: synthetic save builders and checksum forgers (Thread 4, QA-owned).
//
// Every builder produces a SYNTHETIC save in code: no real save, ROM or key is read or committed.
// Layout constants are not invented here. Each one is either taken from the production header
// that owns it (Gen3::Detail, Gen5::LayoutInfo, Gen5::Crypto, Encryption4) or copied verbatim
// from the accepted regression fixture that already pins it, cited next to the value:
//   Gen I   tests/test_classic_verified_export.cpp gen1Save()  (PKSM-Core aa22d7a4 / PKHeX e15d2467 SAV1)
//   Gen II  tests/test_gsc_discovery.cpp IGS / IC tables       (PKHeX SAV2 international GS / Crystal)
//   Gen III build-host/frlg_fixture.sav from test_pksm_gen3_adapter --write-fixture (pinned SHA-256)
//   Gen IV  tests/test_gen4_staged_pokemon_editor.cpp spec()/makeSave()  (PKHeX SAV4 DP/Pt/HGSS)
//   Gen V   tests/test_gen5_staged_save_candidate.cpp fixture()  (PKHeX SaveBlockAccessor5BW/B2W2)
#pragma once

#include "Encryption/Encryption4.h"
#include "Encryption/Encryption5.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace qasynth {

using Bytes = std::vector<uint8_t>;

inline void w16(Bytes& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
inline void w32(Bytes& b, size_t o, uint32_t v) {
    w16(b, o, static_cast<uint16_t>(v));
    w16(b, o + 2, static_cast<uint16_t>(v >> 16));
}
inline uint32_t r32(std::span<const uint8_t> b, size_t o) {
    return static_cast<uint32_t>(b[o]) | (static_cast<uint32_t>(b[o + 1]) << 8) |
           (static_cast<uint32_t>(b[o + 2]) << 16) | (static_cast<uint32_t>(b[o + 3]) << 24);
}

// ---------------------------------------------------------------------------------------------
// Gen I (international R/B/Y SRAM, 0x8000)
// ---------------------------------------------------------------------------------------------
namespace gen1 {
inline constexpr size_t kMain = 0x2598, kLen = 0x0F8B, kSum = 0x3523;

inline Bytes save(uint32_t moneyBcd = 0x012345, uint8_t partyCount = 0) {
    Bytes d(PokeVault::Integration::Gen1::kRawSaveSize, 0);
    const uint8_t will[] = {0x96, 0x88, 0x8B, 0x8B, 0x50};
    std::copy(std::begin(will), std::end(will), d.begin() + kMain);
    d[0x25F3] = static_cast<uint8_t>(moneyBcd >> 16);
    d[0x25F4] = static_cast<uint8_t>(moneyBcd >> 8);
    d[0x25F5] = static_cast<uint8_t>(moneyBcd);
    d[0x2605] = 0x12; d[0x2606] = 0x34;
    d[0x29C3] = 0x99;
    d[0x284C] = 2;
    d[0x2F2C] = partyCount; d[0x2F2D] = 0xFF;
    d[0x30C0] = 0; d[0x30C1] = 0xFF;
    d[0x2ED5] = 0xFF;
    d[kSum] = PokeVault::Integration::Gen1::calculateDiff8(std::span<const uint8_t>(d).subspan(kMain, kLen));
    return d;
}

inline void forge(Bytes& d) {
    if (d.size() < kSum + 1) return;
    d[kSum] = PokeVault::Integration::Gen1::calculateDiff8(std::span<const uint8_t>(d).subspan(kMain, kLen));
}
// Party count, box count bytes and the current-box list count.
inline const std::vector<size_t> kCounts = {0x2F2C, 0x30C0, 0x284C, 0x29C3};
} // namespace gen1

// ---------------------------------------------------------------------------------------------
// Gen II (international Gold/Silver and Crystal SRAM, 0x8000)
// ---------------------------------------------------------------------------------------------
namespace gen2 {
struct Layout {
    size_t money, idx, names, party, current, end, sum;
    uint8_t cap, boxes, str, split;
};
inline constexpr Layout IGS{0x23DB, 0x2724, 0x2727, 0x288A, 0x2D6C, 0x2D68, 0x2D69, 20, 14, 11, 7};
inline constexpr Layout IC{0x23DC, 0x2700, 0x2703, 0x2865, 0x2D10, 0x2B82, 0x2D0D, 20, 14, 11, 7};

inline size_t listLen(size_t cap, size_t body, size_t str) { return 1 + (cap + 1) + cap * body + 2 * cap * str; }
inline size_t stride(const Layout& l) { return listLen(l.cap, 32, l.str) + 2; }
inline size_t boxStart(const Layout& l, size_t b) {
    return b < l.split ? 0x4000 + b * stride(l) : 0x6000 + (b - l.split) * stride(l);
}
inline void name(Bytes& d, size_t ofs, size_t len, char c) {
    std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(ofs), static_cast<std::ptrdiff_t>(len), 0x50);
    if (c >= 'A' && c <= 'Z') d[ofs] = static_cast<uint8_t>(0x80 + c - 'A');
}
inline void pokemonList(Bytes& d, size_t ofs, size_t cap, size_t bodySize, size_t str) {
    constexpr uint8_t species = 25;
    d[ofs] = 1; d[ofs + 1] = species; d[ofs + 2] = 0xFF;
    const size_t body = ofs + 1 + (cap + 1);
    d[body] = species; d[body + 6] = 0x12; d[body + 7] = 0x34; d[body + 21] = 0x7A;
    d[body + 22] = 0xAA; d[body + 31] = 15;
    const size_t ot = body + cap * bodySize;
    const size_t nick = ot + cap * str;
    name(d, ot, str, 'O');
    name(d, nick, str, 'M');
}
inline void checksum(Bytes& d, const Layout& l) {
    if (d.size() < l.sum + 2) return;
    uint16_t s = 0;
    for (size_t i = 0x2009; i <= l.end; ++i) s = static_cast<uint16_t>(s + d[i]);
    d[l.sum] = static_cast<uint8_t>(s);
    d[l.sum + 1] = static_cast<uint8_t>(s >> 8);
}
inline Bytes save(const Layout& l, uint8_t moneyLow = 0x40) {
    Bytes d(0x8000);
    d[0x2009] = 0x12; d[0x200A] = 0x34; name(d, 0x200B, l.str, 'A');
    d[l.money] = 0x01; d[l.money + 1] = 0xE2; d[l.money + 2] = moneyLow; d[l.idx] = 0;
    for (size_t b = 0; b < l.boxes; ++b) {
        d[boxStart(l, b)] = 0; d[boxStart(l, b) + 1] = 0xFF;
        name(d, l.names + b * 9, 9, static_cast<char>('A' + (b % 26)));
    }
    pokemonList(d, boxStart(l, 0), l.cap, 32, l.str);
    d[l.current] = 0; d[l.current + 1] = 0xFF;
    pokemonList(d, l.party, 6, 48, l.str);
    checksum(d, l);
    return d;
}
// Crystal first: the Gold/Silver range (0x2009..0x2D68) covers Crystal's checksum word.
inline void forge(Bytes& d) {
    checksum(d, IC);
    checksum(d, IGS);
}
inline const std::vector<size_t> kCounts = {IGS.party, IC.party, IGS.current, IC.current,
                                            0x4000, IGS.idx, IC.idx};
} // namespace gen2

// ---------------------------------------------------------------------------------------------
// Gen III (rotating-sector Flash, 0x20000)
// ---------------------------------------------------------------------------------------------
namespace gen3 {
namespace D = PokeVault::Integration::Gen3::Detail;

// The accepted deterministic FR/LG fixture (pinned SHA-256 in test_pksm_gen3_adapter.cpp),
// produced by the existing host target. Returns empty when it was not built.
inline Bytes fixture() {
    const char* env = std::getenv("POKEBANK_QA_GEN3_FIXTURE");
    const std::filesystem::path path = env && *env ? env : "build-host/frlg_fixture.sav";
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return Bytes(std::istreambuf_iterator<char>(in), {});
}

// Sector-valid skeleton: correct ids, signatures, counters and checksums, zero payload. It is
// expected to be rejected after sector validation (no recognizable family); it exercises the
// slot selection path deterministically without depending on any build output.
inline Bytes skeleton(uint32_t counterA = 5, uint32_t counterB = 4) {
    Bytes d(D::kSaveSize, 0);
    for (uint8_t slot = 0; slot < 2; ++slot)
        for (size_t s = 0; s < D::kSectorCount; ++s) {
            const size_t off = D::kSlotBases[slot] + s * D::kSectorSize;
            w16(d, off + 0xFF4, static_cast<uint16_t>((s + slot * 3) % D::kSectorCount));
            w32(d, off + 0xFF8, D::kSectorSignature);
            w32(d, off + 0xFFC, slot == 0 ? counterA : counterB);
        }
    return d;
}

inline void forge(Bytes& d) {
    if (d.size() < D::kSaveSize) return;
    for (size_t slotBase : D::kSlotBases)
        for (size_t s = 0; s < D::kSectorCount; ++s) {
            const size_t off = slotBase + s * D::kSectorSize;
            const uint16_t id = static_cast<uint16_t>(d[off + 0xFF4] | (d[off + 0xFF5] << 8));
            if (id >= D::kSectorCount) continue;
            w16(d, off + 0xFF6, D::sectorChecksum(std::span<const uint8_t>(d).subspan(off, D::kChunkLengths[id])));
        }
}
} // namespace gen3

// ---------------------------------------------------------------------------------------------
// Gen IV (DP / Pt / HGSS NDS battery, 0x80000, two 0x40000 partitions)
// ---------------------------------------------------------------------------------------------
namespace gen4 {
inline constexpr size_t kSaveSize = 0x80000;
inline constexpr size_t kPartition = 0x40000;
inline constexpr uint32_t kMagic = 0x20060623;

struct Spec {
    size_t generalSize, storageStart, storageSize, footerSize, trainer, party, boxData, boxStride;
};
enum class Kind : uint8_t { DiamondPearl, Platinum, HeartGoldSoulSilver };
inline Spec spec(Kind k) {
    if (k == Kind::DiamondPearl) return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 0xFF0};
    if (k == Kind::Platinum) return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 0xFF0};
    return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000};
}

inline void copyBytes(Bytes& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i) dst[off + i] = static_cast<uint8_t>(src[i]);
}
inline void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}
inline void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    wb16(b, o, static_cast<uint16_t>(v));
    wb16(b, o + 2, static_cast<uint16_t>(v >> 16));
}

inline std::vector<std::byte> decryptedStored(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    wb32(d, 0x00, 0x12345678u);
    wb16(d, 0x08, species);
    wb16(d, 0x0A, 1);
    wb16(d, 0x0C, 12345);
    wb16(d, 0x0E, 54321);
    wb32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    d[0x15] = std::byte{9};
    d[0x17] = std::byte{2};
    const auto nickname = Utils::encodeGen4Field(u"PIKA", 11, 10, 2);
    std::copy(nickname.begin(), nickname.end(), d.begin() + 0x48);
    const auto ot = Utils::encodeGen4Field(u"ASH", 8, 7, 2);
    std::copy(ot.begin(), ot.end(), d.begin() + 0x68);
    d[0x83] = std::byte{4};
    d[0x84] = std::byte{25};
    d[0x86] = std::byte{17};
    return d;
}
inline std::vector<std::byte> stored(uint16_t species = 25) {
    return Encryption::encryptArray4(decryptedStored(species));
}
inline std::vector<std::byte> party(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    const auto decoded = decryptedStored(species);
    std::copy(decoded.begin(), decoded.end(), d.begin());
    d[0x8C] = std::byte{20};
    wb16(d, 0x8E, 35); wb16(d, 0x90, 35); wb16(d, 0x92, 25);
    wb16(d, 0x94, 20); wb16(d, 0x96, 30); wb16(d, 0x98, 25); wb16(d, 0x9A, 20);
    return Encryption::encryptArray4(d);
}

inline void stampBlock(Bytes& save, size_t off, size_t len, size_t footer, uint32_t major) {
    const size_t end = off + len;
    w32(save, end - 0x14, major);
    w32(save, end - 0x10, 1);
    w32(save, end - 0x0C, static_cast<uint32_t>(len));
    w32(save, end - 0x08, kMagic);
    w16(save, end - 2, ::Utils::crc16ccitt(save.data() + off, len - footer));
}

inline Bytes save(Kind kind, uint8_t romCode = 7, int newerGeneral = 0, int newerStorage = 0) {
    const auto sp = spec(kind);
    Bytes s(kSaveSize, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto occupied = stored();
    const auto partyRecord = party();
    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * kPartition;
        std::fill(s.begin() + static_cast<std::ptrdiff_t>(base),
                  s.begin() + static_cast<std::ptrdiff_t>(base + sp.generalSize), 0);
        const size_t storage = base + sp.storageStart;
        std::fill(s.begin() + static_cast<std::ptrdiff_t>(storage),
                  s.begin() + static_cast<std::ptrdiff_t>(storage + sp.storageSize), 0);
        copyBytes(s, base + sp.trainer, Utils::encodeGen4Field(u"Kylie", 8, 7, 2));
        w16(s, base + sp.trainer + 0x10, 12150);
        w16(s, base + sp.trainer + 0x12, 22558);
        s[base + sp.trainer + 0x18] = 1;
        s[base + sp.trainer + 0x19] = 2;
        s[base + sp.trainer + 0x1C] = romCode;
        s[base + sp.party - 4] = 1;
        copyBytes(s, base + sp.party, partyRecord);
        for (size_t box = 0; box < 18; ++box) {
            const size_t boxBase = storage + sp.boxData + box * sp.boxStride;
            for (size_t slot = 0; slot < 30; ++slot)
                copyBytes(s, boxBase + slot * Encryption::SIZE_STORED4, blank);
            if (kind == Kind::HeartGoldSoulSilver)
                std::fill(s.begin() + static_cast<std::ptrdiff_t>(boxBase + 0xFF0),
                          s.begin() + static_cast<std::ptrdiff_t>(boxBase + 0x1000),
                          static_cast<uint8_t>(0xC0 + box));
        }
        copyBytes(s, storage + sp.boxData, occupied);
        if (kind == Kind::HeartGoldSoulSilver) s[storage + 0x12000] = 0;
        else s[storage] = 0;
        stampBlock(s, base, sp.generalSize, sp.footerSize, partition == newerGeneral ? 20 : 10);
        stampBlock(s, storage, sp.storageSize, sp.footerSize, partition == newerStorage ? 30 : 15);
    }
    return s;
}

// Re-forge the CRC of every block whose footer still declares that layout's block size.
inline void forge(Bytes& s) {
    if (s.size() < kSaveSize) return;
    for (Kind kind : {Kind::DiamondPearl, Kind::Platinum, Kind::HeartGoldSoulSilver}) {
        const auto sp = spec(kind);
        for (size_t partition = 0; partition < 2; ++partition) {
            const size_t base = partition * kPartition;
            for (const auto& [off, len] : {std::pair{base, sp.generalSize},
                                          std::pair{base + sp.storageStart, sp.storageSize}}) {
                const size_t end = off + len;
                if (r32(s, end - 0x0C) != len) continue;
                w16(s, end - 2, ::Utils::crc16ccitt(s.data() + off, len - sp.footerSize));
            }
        }
    }
}

inline std::vector<size_t> countOffsets() {
    std::vector<size_t> out;
    for (Kind kind : {Kind::DiamondPearl, Kind::Platinum, Kind::HeartGoldSoulSilver}) {
        const auto sp = spec(kind);
        for (size_t partition = 0; partition < 2; ++partition) {
            out.push_back(partition * kPartition + sp.party - 4);           // party count
            out.push_back(partition * kPartition + sp.storageStart);        // current box (DP/Pt)
            out.push_back(partition * kPartition + sp.storageStart + 0x12000); // current box (HGSS)
        }
    }
    return out;
}
} // namespace gen4

// ---------------------------------------------------------------------------------------------
// Gen V (BW / B2W2 NDS battery, 0x80000, adjacent primary/backup copies)
// ---------------------------------------------------------------------------------------------
namespace gen5 {
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace C = PokeVault::Integration::Gen5::Crypto;
using Family = PokeVault::Integration::Gen5::SaveFamily;

inline size_t copySize(Family f) { return f == Family::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize; }
inline std::span<const L::SaveBlock> blocks(Family f) {
    return f == Family::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                   : std::span<const L::SaveBlock>(L::Black2White2);
}
inline void stamp(std::span<uint8_t> copy, std::span<const L::SaveBlock> list) {
    for (const auto& b : list) {
        if (b.offset + b.length > copy.size() || b.checksumOffset + 2 > copy.size() ||
            b.mirrorOffset + 2 > copy.size())
            continue;
        const uint16_t crc = ::Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}

inline Bytes pk5(uint32_t pid, uint16_t species, bool partyRecord) {
    Bytes r(partyRecord ? C::PartySize : C::StoredSize, 0);
    w32(r, 0, pid);
    w16(r, 8, species);
    w16(r, 0x0C, 4321);
    if (partyRecord) r[0x8C] = 54;
    r[0x84] = 0x80 | 35;
    return C::encryptCandidate(r);
}

// Items block contents for the selected copy: a few native stacks per pocket
// (ids taken from the production BagCatalog5 tables, never invented).
inline void writeBag(std::span<uint8_t> copy, Family f) {
    using namespace PokeVault::Integration::Gen5;
    const size_t at = itemsBlockOffset(f);
    auto put = [&](size_t pocket, size_t slot, uint16_t id, uint16_t count) {
        const auto spec = bagPocketLayout5(f, pocket);
        const size_t pos = at + spec.offset + slot * 4;
        C::write16(copy, pos, id);
        C::write16(copy, pos + 2, count);
    };
    put(0, 0, BagCatalog5::kGeneral[0], 3);
    put(0, 1, BagCatalog5::kGeneral[5], 999);
    put(1, 0, f == Family::BlackWhite ? BagCatalog5::kKeyBW[0] : BagCatalog5::kKeyB2W2[0], 1);
    put(2, 0, BagCatalog5::kMachine[0], 1);
    put(3, 0, BagCatalog5::kMedicine[0], 12);
    put(4, 0, BagCatalog5::kBerry[0], 5);
}

inline void fillCopy(Bytes& sav, Family f, size_t base, uint8_t version, uint32_t pid) {
    sav[base + 0x19400 + 0x1F] = version;
    sav[base + L::PartyOffset + 4] = 1;
    const auto party = pk5(pid, 25, true);
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8));
    const auto boxed = pk5(pid ^ 0x12340000u, 133, false);
    std::copy(boxed.begin(), boxed.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset));
    writeBag(std::span<uint8_t>(sav).subspan(base, copySize(f)), f);
    stamp(std::span<uint8_t>(sav).subspan(base, copySize(f)), blocks(f));
}

// copies: 1 = primary only, 2 = backup only, 3 = both identical, 4 = both different
inline Bytes save(Family f, uint8_t version, int copies = 1, uint32_t pid = 0x01020304u) {
    Bytes sav(L::FullSaveSize, 0);
    if (copies == 1 || copies >= 3) fillCopy(sav, f, 0, version, pid);
    if (copies == 2 || copies >= 3) fillCopy(sav, f, copySize(f), version, copies == 4 ? pid + 1 : pid);
    return sav;
}

inline void forge(Bytes& sav) {
    if (sav.size() != L::FullSaveSize) return;
    for (const Family f : {Family::BlackWhite, Family::Black2White2}) {
        for (const size_t base : {size_t{0}, copySize(f)}) {
            const uint8_t v = sav[base + 0x19400 + 0x1F];
            const bool bw = v == 20 || v == 21;
            const bool b2 = v == 22 || v == 23;
            if ((f == Family::BlackWhite && bw) || (f == Family::Black2White2 && b2))
                stamp(std::span<uint8_t>(sav).subspan(base, copySize(f)), blocks(f));
        }
    }
}

inline std::vector<size_t> countOffsets() {
    return {L::PartyOffset + 4, L::BlackWhiteCopySize + L::PartyOffset + 4,
            L::Black2White2CopySize + L::PartyOffset + 4, 0x19400 + 0x1F,
            L::BlackWhiteCopySize + 0x19400 + 0x1F, L::Black2White2CopySize + 0x19400 + 0x1F};
}
} // namespace gen5

} // namespace qasynth
