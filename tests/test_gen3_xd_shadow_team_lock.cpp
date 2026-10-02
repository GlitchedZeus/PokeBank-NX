#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3XdShadowTeamLock.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    namespace CXD = Legality::Gen3CxdPidIv;
    namespace Lock = Legality::Gen3XdShadowTeamLock;

    static_assert(Lock::kTeamSetCount == 72);
    static_assert(Lock::kTeamVariantCount == 113);
    static_assert(Lock::kLockCount == 389);
    static_assert(Lock::kShadowIndexToTeamSet.size() == 84);

    // The source's "First" team has no prior locks and is therefore immediately valid.
    assert(Lock::validate(1, 0x12345678u) == Lock::Result::Matched);
    assert(Lock::validate(0, 0x12345678u) == Lock::Result::NotMatched);
    assert(Lock::validate(84, 0x12345678u) == Lock::Result::NotMatched);

    struct DirectVector {
        uint8_t index;
        uint32_t pid;
        std::array<uint8_t, 6> ivs;
    };
    constexpr std::array<DirectVector, 12> direct{{
        {82, 0xAF4E3161u, {11, 29, 25, 6, 23, 10}},
        {20, 0xC3A0F1E5u, {30, 3, 9, 10, 27, 30}},
        {3, 0xA459BF44u, {0, 11, 4, 28, 6, 13}},
        {22, 0x8E14DAB6u, {29, 24, 30, 16, 3, 18}},
        {11, 0x30E87CC7u, {22, 11, 8, 26, 4, 29}},
        {12, 0x9BECA2A6u, {31, 31, 25, 13, 22, 1}},
        {24, 0x77D87601u, {10, 27, 26, 13, 30, 19}},
        {9, 0x37F95B26u, {11, 8, 5, 10, 28, 14}},
        {36, 0x2E49AC34u, {15, 24, 7, 2, 11, 2}},
        {41, 0x1973FD07u, {13, 30, 3, 16, 20, 9}},
        {42, 0x33893D4Cu, {26, 25, 24, 28, 29, 30}},
        {7, 0x8CBD29DBu, {19, 29, 30, 0, 7, 2}},
    }};

    // Exact pinned PKHeX ShadowTests vectors: recover each CXD origin seed, then
    // prove at least one source team history can reverse-generate the encounter.
    for (const auto& vector : direct) {
        const auto cxd = CXD::analyze(vector.pid, vector.ivs);
        assert(cxd.matched);
        assert(Lock::validate(vector.index, cxd.originSeed) == Lock::Result::Matched);
    }

    struct TailVector {
        uint8_t index;
        uint32_t finalTeamPid;
    };
    constexpr std::array<TailVector, 20> tails{{
        {12, 0x31538B48u},
        {12, 0x3494CDA1u},
        {12, 0xC93DF897u},
        {12, 0x5F5380F4u},
        {12, 0x38DDE117u},
        {12, 0x1956D8B5u},
        {12, 0xF6EAD3E2u},
        {12, 0xBEADBDC3u},
        {12, 0x5EEF1076u},
        {12, 0x451FAE3Cu},
        {36, 0x0EC25CE5u},
        {36, 0x0A8C9738u},
        {36, 0x1D5AEC4Fu},
        {36, 0x55CE5E4Bu},
        {36, 0x9B2F5B53u},
        {36, 0x9334337Eu},
        {36, 0x92D31CC2u},
        {36, 0xCBA7A0C3u},
        {36, 0x9D1BDC4Au},
        {36, 0x0D949325u},
    }};

    // PKHeX ShadowTeamTests publishes 10 Delcatty and 10 Butterfree CPU-team
    // sequences. Reproduce its final-team-PID seed recovery and require at least
    // one candidate origin to satisfy the pinned recursive lock history.
    for (const auto& vector : tails) {
        const auto seeds = CXD::Detail::reversePid(
            vector.finalTeamPid & 0xFFFF0000u, vector.finalTeamPid << 16);
        bool matched = false;
        for (std::size_t i = 0; i < seeds.count; ++i) {
            uint32_t origin = seeds.values[i];
            origin = CXD::Detail::prev(CXD::Detail::prev(CXD::Detail::prev(origin)));
            if (Lock::validate(vector.index, origin) == Lock::Result::Matched) {
                matched = true;
                break;
            }
        }
        assert(matched);
    }

    // PKHeX's XD Mawile anti-shiny test: player TSV participates in the recursive
    // validation and still reaches a valid source history.
    const auto mawile = CXD::analyze(
        0x049F2F05u, {31, 30, 29, 31, 23, 27});
    assert(mawile.matched);
    assert(Lock::validateXd(18, mawile.originSeed, 12345, 51882) ==
           Lock::Result::Matched);

    // Guard exhaustion is deliberately distinct from a negative proof.
    const auto seedot = CXD::analyze(
        0x8CBD29DBu, {19, 29, 30, 0, 7, 2});
    assert(seedot.matched);
    assert(Lock::validate(7, seedot.originSeed,
                          Lock::kNoTrainerShinyValue, 1) ==
           Lock::Result::SearchLimit);

    std::cout << "Gen III XD recursive shadow team-lock evidence: PASS\n";
}
