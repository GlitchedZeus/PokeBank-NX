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

    // Emerald is intentionally outside this tranche because pinned Method-H
    // uses the broader lead-ability search there.
    const auto emerald = MH::analyze(
        "emerald_gba", 25, 57, 25, 0, 0xAC2141C6u, rubyBlock);
    assert(emerald.resolution == MH::Resolution::Unresolved);

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
