#include "Legality/Gen3SafariMethodHEvidence.h"

#include <array>
#include <cassert>

int main() {
    using namespace Legality;
    namespace MH = Gen3SafariMethodH;
    using Gen3PidIv::Method;

    // Ruby Safari Pikachu: positive Method-1 Hoenn Safari-block history.
    const auto rubyBlock = Gen3PidIv::analyze(
        0xAC2141C6u, {14,23,20,23,29,7});
    assert(rubyBlock.method == Method::Method1);
    assert(rubyBlock.originSeed == 0x00000001u);
    const auto pikachuBlock = MH::analyze(
        "ruby_gba", 25, 57, 25, 0, 0xAC2141C6u, rubyBlock);
    assert(pikachuBlock.resolution == MH::Resolution::FrameMatched);
    assert(pikachuBlock.path == MH::Path::HoennSafariBlock);
    assert(pikachuBlock.requiredBall == Gen3Safari::kSafariBall);
    assert(pikachuBlock.sourceSpecies == 25);
    assert(!pikachuBlock.evolved);
    assert(pikachuBlock.encounterType == 0);

    // Same accepted history remains valid after Pikachu evolves to Raichu.
    const auto raichuBlock = MH::analyze(
        "ruby_gba", 26, 57, 25, 0, 0xAC2141C6u, rubyBlock);
    assert(raichuBlock.resolution == MH::Resolution::FrameMatched);
    assert(raichuBlock.sourceSpecies == 25);
    assert(raichuBlock.evolved);

    // Ruby Safari Pikachu ordinary no-block path.
    const auto rubyRegular = Gen3PidIv::analyze(
        0x3FB2A527u, {16,19,27,3,9,12});
    assert(rubyRegular.method == Method::Method1);
    assert(rubyRegular.originSeed == 0x00000039u);
    const auto pikachuRegular = MH::analyze(
        "ruby_gba", 25, 57, 25, 0, 0x3FB2A527u, rubyRegular);
    assert(pikachuRegular.resolution == MH::Resolution::FrameMatched);
    assert(pikachuRegular.path == MH::Path::RegularNature);

    // FireRed does not use the Hoenn Safari nature-preference block.
    const auto frScyther = Gen3PidIv::analyze(
        0xC1354FF9u, {28,22,21,11,31,3});
    assert(frScyther.method == Method::Method1);
    assert(frScyther.originSeed == 0x00000009u);
    const auto scyther = MH::analyze(
        "firered_gba", 123, 136, 23, 0, 0xC1354FF9u, frScyther);
    assert(scyther.resolution == MH::Resolution::FrameMatched);
    assert(scyther.path == MH::Path::RegularNature);
    assert(scyther.sourceSpecies == 123);
    assert(scyther.encounterType == 0);
    assert(scyther.slot == 9);

    // Method 2 is still a classic Method-H correlation: the VBlank gap is in
    // the IV sequence, not between the two PID halves.
    const auto method2 = Gen3PidIv::analyze(
        0x5271E97Eu, {2,18,3,12,22,24});
    assert(method2.method == Method::Method2);
    assert(method2.originSeed == 0x00006073u);
    const auto oddishM2 = MH::analyze(
        "ruby_gba", 43, 57, 25, 0, 0x5271E97Eu, method2);
    assert(oddishM2.resolution == MH::Resolution::FrameMatched);
    assert(oddishM2.path == MH::Path::HoennSafariBlock);
    assert(oddishM2.sourceSpecies == 43);

    // Method 4 likewise preserves the normal sequential PID seed.
    const auto method4 = Gen3PidIv::analyze(
        0x31B05271u, {2,18,3,5,30,11});
    assert(method4.method == Method::Method4);
    assert(method4.originSeed == 0xE97E7B6Au);
    const auto oddishM4 = MH::analyze(
        "ruby_gba", 43, 57, 27, 0, 0x31B05271u, method4);
    assert(oddishM4.resolution == MH::Resolution::FrameMatched);
    assert(oddishM4.path == MH::Path::RegularNature);

    // Rock Smash has an additional source-backed encounter-rate proc.
    const auto rock = Gen3PidIv::analyze(
        0xF4080719u, {8,3,21,10,14,26});
    assert(rock.method == Method::Method1);
    assert(rock.originSeed == 0x00000004u);
    const auto geodude = MH::analyze(
        "ruby_gba", 74, 57, 10, 0, 0xF4080719u, rock);
    assert(geodude.resolution == MH::Resolution::FrameMatched);
    assert(geodude.encounterType == 5);

    // Emerald has the same Hoenn Safari nature-preference block, but pinned
    // Method-H can also search broader lead histories. This tranche proves only
    // a no-lead positive history. For gendered species, the first matching-
    // nature reversal count is the Emerald NoLead window.
    const auto emeraldNoLead = Gen3PidIv::analyze(
        0xE97E0000u, {17,19,20,16,13,12});
    assert(emeraldNoLead.method == Method::Method1);
    assert(emeraldNoLead.originSeed == 0x00000000u);
    const auto emeraldOddish = MH::analyze(
        "emerald_gba", 43, 57, 29, 0, 0xE97E0000u, emeraldNoLead);
    assert(emeraldOddish.resolution == MH::Resolution::FrameMatched);
    assert(emeraldOddish.path == MH::Path::HoennSafariBlock);
    assert(emeraldOddish.requiredBall == Gen3Safari::kSafariBall);
    assert(emeraldOddish.sourceSpecies == 43);
    assert(emeraldOddish.encounterType == 0);
    assert(emeraldOddish.slot == 3);

    // Emerald Synchronize-success path. This vector is intentionally isolated:
    // - Safari block roll is 95, so the block path cannot activate.
    // - p0 is even, so Synchronize succeeds.
    // - p0 nature (0) does not equal the generated PID nature (13), so the
    //   ordinary no-lead nature path cannot explain the encounter.
    // - reversal window is zero, so there are no later no-lead candidates.
    const auto emeraldSync = Gen3PidIv::analyze(
        0x8A3759F1u, {6,31,16,13,12,30});
    assert(emeraldSync.method == Method::Method1);
    assert(emeraldSync.originSeed == 0x000000E7u);
    const uint8_t emeraldSyncNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(emeraldSync.originSeed) % 25u);
    assert(emeraldSyncNature == 13u);
    assert(MH::Detail::reversalWindow(
        emeraldSync.originSeed, emeraldSyncNature) == 0u);
    const uint32_t emeraldSyncBlock =
        MH::Detail::hoennSafariBlockSeed(emeraldSync.originSeed);
    assert(!MH::Detail::hoennSafariBlockProc(emeraldSyncBlock));
    assert((MH::Detail::upper16(emeraldSyncBlock) % 100u) == 95u);
    assert((MH::Detail::upper16(emeraldSync.originSeed) & 1u) == 0u);
    assert((MH::Detail::upper16(emeraldSync.originSeed) % 25u) !=
           emeraldSyncNature);

    const auto emeraldPsyduckSync = MH::analyze(
        "emerald_gba", 54, 57, 31, 0, 0x8A3759F1u, emeraldSync);
    assert(emeraldPsyduckSync.resolution == MH::Resolution::FrameMatched);
    assert(emeraldPsyduckSync.path == MH::Path::EmeraldSynchronize);
    assert(emeraldPsyduckSync.requiredBall == Gen3Safari::kSafariBall);
    assert(emeraldPsyduckSync.sourceSpecies == 54);
    assert(!emeraldPsyduckSync.evolved);
    assert(emeraldPsyduckSync.encounterType == 1);
    assert(emeraldPsyduckSync.slot == 2);
    assert(emeraldPsyduckSync.pidSeed == emeraldSync.originSeed);
    assert(emeraldPsyduckSync.frameSeed ==
           Gen3PidIv::Detail::prev(emeraldSync.originSeed));

    // Emerald Synchronize-failure path: the Safari block and the earlier
    // no-lead/Synchronize-success paths are all excluded. The failed proc is
    // at -1, generated level at -2, encounter slot at -3.
    const auto emeraldSyncFail = Gen3PidIv::analyze(
        0xB9BC7402u, {4,3,8,22,31,9});
    assert(emeraldSyncFail.method == Method::Method1);
    assert(emeraldSyncFail.originSeed == 0x00014661u);
    const uint8_t failNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(emeraldSyncFail.originSeed) % 25u);
    assert(failNature == 1u);
    assert(MH::Detail::reversalWindow(
        emeraldSyncFail.originSeed, failNature) == 0u);
    assert(MH::Detail::upper16(emeraldSyncFail.originSeed) == 1u);
    const uint32_t failBlock =
        MH::Detail::hoennSafariBlockSeed(emeraldSyncFail.originSeed);
    assert(!MH::Detail::hoennSafariBlockProc(failBlock));
    assert((MH::Detail::upper16(failBlock) % 100u) == 98u);

    const uint32_t failFrame =
        Gen3PidIv::Detail::prev(emeraldSyncFail.originSeed);
    const uint32_t failedProc = Gen3PidIv::Detail::prev(failFrame);
    assert((MH::Detail::upper16(failedProc) & 1u) == 1u);
    const auto oddishSyncFail = MH::analyze(
        "emerald_gba", 43, 57, 27, 0, 0xB9BC7402u, emeraldSyncFail);
    assert(oddishSyncFail.resolution == MH::Resolution::FrameMatched);
    assert(oddishSyncFail.path == MH::Path::EmeraldSynchronizeFailed);
    assert(oddishSyncFail.requiredBall == Gen3Safari::kSafariBall);
    assert(oddishSyncFail.sourceSpecies == 43);
    assert(oddishSyncFail.encounterType == 0);
    assert(oddishSyncFail.slot == 1);
    assert(oddishSyncFail.frameSeed == failFrame);

    // Emerald Cute Charm failure consumes an RNG proc at -1. This vector is
    // isolated from every accepted no-lead/Synchronize result: the Hoenn block
    // does not activate, p0 is odd, the ordinary frame does not match the slot,
    // and the failed proc is divisible by 3 but even (SyncFail impossible).
    const auto emeraldCuteFail = Gen3PidIv::analyze(
        0x54231DF8u, {2,7,7,20,29,24});
    assert(emeraldCuteFail.method == Method::Method1);
    assert(emeraldCuteFail.originSeed == 0x000121B4u);
    const uint8_t cuteNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(emeraldCuteFail.originSeed) % 25u);
    assert(cuteNature == 1u);
    assert(MH::Detail::reversalWindow(
        emeraldCuteFail.originSeed, cuteNature) == 0u);
    assert(MH::Detail::upper16(emeraldCuteFail.originSeed) == 1u);
    const uint32_t cuteBlock =
        MH::Detail::hoennSafariBlockSeed(emeraldCuteFail.originSeed);
    assert(!MH::Detail::hoennSafariBlockProc(cuteBlock));
    assert((MH::Detail::upper16(cuteBlock) % 100u) == 87u);
    const uint32_t cuteFrame =
        Gen3PidIv::Detail::prev(emeraldCuteFail.originSeed);
    const uint16_t cuteProc = MH::Detail::upper16(
        Gen3PidIv::Detail::prev(cuteFrame));
    assert(cuteProc == 57024u);
    assert(cuteProc % 3u == 0u && (cuteProc & 1u) == 0u);

    const auto psyduckCuteFail = MH::analyze(
        "emerald_gba", 54, 57, 21, 0, 0x54231DF8u, emeraldCuteFail);
    assert(psyduckCuteFail.resolution == MH::Resolution::FrameMatched);
    assert(psyduckCuteFail.path == MH::Path::EmeraldCuteCharmFailed);
    assert(psyduckCuteFail.requiredBall == Gen3Safari::kSafariBall);
    assert(psyduckCuteFail.sourceSpecies == 54);
    assert(psyduckCuteFail.encounterType == 1);
    assert(psyduckCuteFail.slot == 1);
    assert(psyduckCuteFail.frameSeed == cuteFrame);

    // The same PID/IV frame is not an FR/LG or Ruby source: these remain
    // unresolved, not hard-invalid.
    assert(MH::analyze(
        "ruby_gba", 43, 57, 27, 0, 0xB9BC7402u, emeraldSyncFail
    ).resolution == MH::Resolution::Unresolved);

    // Method 3 uses A_CDE: the persisted PID skips one RNG frame between
    // halves. Pinned MethodH nevertheless derives its reversal nature from the
    // sequential A+B PID at the same OriginSeed. This vector deliberately has
    // different persisted and Method-H natures (5 vs 14) so using pid % 25
    // would fail to reproduce the source behavior.
    const auto method3 = Gen3PidIv::analyze(
        0x52710000u, {16,13,12,2,18,3});
    assert(method3.method == Method::Method3);
    assert(method3.originSeed == 0x00000000u);
    assert(MH::Detail::method3Pid(method3.originSeed) == 0x52710000u);
    assert((0x52710000u % 25u) == 5u);
    assert((MH::Detail::sequentialPid(method3.originSeed) % 25u) == 14u);

    const auto method3Safari = MH::analyze(
        "ruby_gba", 43, 57, 29, 0, 0x52710000u, method3);
    assert(method3Safari.resolution == MH::Resolution::FrameMatched);
    assert(method3Safari.path == MH::Path::HoennSafariBlock);
    assert(method3Safari.requiredBall == Gen3Safari::kSafariBall);
    assert(method3Safari.sourceSpecies == 43);
    assert(method3Safari.encounterType == 0);
    assert(method3Safari.slot == 3);

    // Reusing the Method-3 correlation with a sequential A+B PID is rejected;
    // the persisted PID shape still has to be A+C.
    const auto wrongMethod3Pid = MH::analyze(
        "ruby_gba", 43, 57, 29, 0,
        MH::Detail::sequentialPid(method3.originSeed), method3);
    assert(wrongMethod3Pid.resolution == MH::Resolution::Unresolved);

    // A correlation object cannot be re-used with a different persisted PID.
    const auto wrongPid = MH::analyze(
        "ruby_gba", 25, 57, 25, 0, 0xAC2141C7u, rubyBlock);
    assert(wrongPid.resolution == MH::Resolution::Unresolved);

    // Wrong region/location and unsupported identity remain unresolved.
    assert(MH::analyze(
        "ruby_gba", 25, 136, 25, 0, 0xAC2141C6u, rubyBlock).resolution ==
        MH::Resolution::Unresolved);
    assert(MH::analyze(
        "unknown", 25, 57, 25, 0, 0xAC2141C6u, rubyBlock).resolution ==
        MH::Resolution::Unresolved);

    return 0;
}
