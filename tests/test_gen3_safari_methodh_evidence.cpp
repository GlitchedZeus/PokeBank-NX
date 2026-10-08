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

    // Pressure/Hustle/Vital Spirit failed proc reduces the encounter level by
    // one when the RNG level bias is nonzero. Normal frame level is 24 here,
    // while the same -2 level frame after a failed proc yields level 23.
    // This is a source-distinct history, not an ordinary no-lead frame.
    const auto emeraldPressureFail = Gen3PidIv::analyze(
        0xF64C4D11u, {20,26,24,24,29,22});
    assert(emeraldPressureFail.method == Method::Method1);
    assert(emeraldPressureFail.originSeed == 0x000101B0u);
    const uint8_t pressureNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(emeraldPressureFail.originSeed) % 25u);
    assert(pressureNature == 1u);
    assert(MH::Detail::reversalWindow(
        emeraldPressureFail.originSeed, pressureNature) == 8u);
    assert(MH::Detail::upper16(emeraldPressureFail.originSeed) == 1u);
    const uint32_t pressureBlock =
        MH::Detail::hoennSafariBlockSeed(emeraldPressureFail.originSeed);
    assert(!MH::Detail::hoennSafariBlockProc(pressureBlock));
    assert((MH::Detail::upper16(pressureBlock) % 100u) == 86u);
    const uint32_t pressureFrame =
        Gen3PidIv::Detail::prev(emeraldPressureFail.originSeed);
    const uint16_t pressureProc = MH::Detail::upper16(
        Gen3PidIv::Detail::prev(pressureFrame));
    assert(pressureProc == 55890u);
    assert((pressureProc & 1u) == 0u); // lead ability failed

    const auto psyduckPressureFail = MH::analyze(
        "emerald_gba", 54, 57, 23, 0, 0xF64C4D11u, emeraldPressureFail);
    assert(psyduckPressureFail.resolution == MH::Resolution::FrameMatched);
    assert(psyduckPressureFail.path == MH::Path::EmeraldPressureHustleFailed);
    assert(psyduckPressureFail.requiredBall == Gen3Safari::kSafariBall);
    assert(psyduckPressureFail.sourceSpecies == 54);
    assert(psyduckPressureFail.encounterType == 1);
    assert(psyduckPressureFail.slot == 0);
    assert(psyduckPressureFail.frameSeed == pressureFrame);

    // Successful Pressure/Hustle/Vital Spirit proc forces maximum level for
    // non-grass Safari slots. Pinned EncounterSlot3.PressureLevel returns
    // LevelMax for Surf/fishing, but grass may use parent-area pressure max.
    // This Surf vector deliberately differs from both regular RNG level (25)
    // and the shifted failed-proc level (26); only success forces level 30.
    const auto emeraldPressureSuccess = Gen3PidIv::analyze(
        0xB9BC7402u, {4,3,8,22,31,9});
    assert(emeraldPressureSuccess.method == Method::Method1);
    assert(emeraldPressureSuccess.originSeed == 0x00014661u);
    const uint32_t pressureSuccessBlock =
        MH::Detail::hoennSafariBlockSeed(emeraldPressureSuccess.originSeed);
    assert(!MH::Detail::hoennSafariBlockProc(pressureSuccessBlock));
    assert((MH::Detail::upper16(pressureSuccessBlock) % 100u) == 98u);
    const uint32_t pressureSuccessFrame =
        Gen3PidIv::Detail::prev(emeraldPressureSuccess.originSeed);
    const uint32_t pressureSuccessProcSeed =
        Gen3PidIv::Detail::prev(pressureSuccessFrame);
    assert((MH::Detail::upper16(pressureSuccessProcSeed) & 1u) == 1u);

    const auto psyduckPressureSuccess = MH::analyze(
        "emerald_gba", 54, 57, 30, 0, 0xB9BC7402u,
        emeraldPressureSuccess);
    assert(psyduckPressureSuccess.resolution == MH::Resolution::FrameMatched);
    assert(psyduckPressureSuccess.path == MH::Path::EmeraldPressureHustleSuccess);
    assert(psyduckPressureSuccess.requiredBall == Gen3Safari::kSafariBall);
    assert(psyduckPressureSuccess.sourceSpecies == 54);
    assert(psyduckPressureSuccess.encounterType == 1); // Surf, not Grass.
    assert(psyduckPressureSuccess.slot == 0);
    assert(psyduckPressureSuccess.frameSeed == pressureSuccessFrame);

    // Area-aware grass Pressure success: Emerald Oddish grass slot 1 has
    // LevelMax=27 in area 182, but the *same area* has another Oddish slot
    // reaching 29. Pinned EncounterArea3.GetPressureMax therefore forces 29.
    // Distinct areas sharing Safari location 57 must NOT be collapsed together.
    const Gen3Safari::Entry* area180Oddish = nullptr;
    const Gen3Safari::Entry* area182Oddish = nullptr;
    for (const auto& row : Gen3Safari::kGen3SafariEntries) {
        if (row.game != static_cast<uint8_t>(Gen3Safari::Game::Emerald) ||
            row.species != 43 || row.method != 0)
            continue;
        if (row.areaIndex == 180 && row.slot == 0)
            area180Oddish = &row;
        if (row.areaIndex == 182 && row.slot == 1)
            area182Oddish = &row;
    }
    assert(area180Oddish != nullptr && area182Oddish != nullptr);
    assert(area180Oddish->maxLevel == 25);
    assert(MH::Detail::grassPressureLevel(*area180Oddish) == 27);
    assert(area182Oddish->maxLevel == 27);
    assert(MH::Detail::grassPressureLevel(*area182Oddish) == 29);

    // Reuse an independently proven PID/IV seed, now for a distinct grass
    // encounter source: the -3 RNG slot resolves to Oddish slot 1 while
    // the -1 Pressure proc succeeds. Ordinary slot level is 27; grass-area
    // Pressure raises it to 29, unlike the non-grass slot-level path.
    const auto oddishGrassPressure = MH::analyze(
        "emerald_gba", 43, 57, 29, 0, 0xB9BC7402u,
        emeraldPressureSuccess);
    assert(oddishGrassPressure.resolution == MH::Resolution::FrameMatched);
    assert(oddishGrassPressure.path == MH::Path::EmeraldPressureHustleSuccess);
    assert(oddishGrassPressure.requiredBall == Gen3Safari::kSafariBall);
    assert(oddishGrassPressure.encounterType == 0);
    assert(oddishGrassPressure.slot == 1);
    assert(oddishGrassPressure.sourceSpecies == 43);
    assert(oddishGrassPressure.frameSeed == pressureSuccessFrame);

    // An intermediate level 28 cannot be explained by the same successful
    // proc, nor can another encounter area be borrowed to invent that level.
    assert(MH::analyze(
        "emerald_gba", 43, 57, 28, 0, 0xB9BC7402u,
        emeraldPressureSuccess).resolution == MH::Resolution::Unresolved);

    // Emerald Safari Static success consumes a proc at -3, redirects a slot
    // at -2 and generates an ordinary level at -1 (relative to MethodH's
    // failed-Safari-block nature frame). The unmodified ESV picks grass slot
    // 7, not Pikachu's Static-eligible slot 8. This is a genuinely distinct
    // positive lead history with zero prior reversal candidates.
    const auto emeraldStatic = Gen3PidIv::analyze(
        0xA2B5D929u, {8,27,6,4,25,3});
    assert(emeraldStatic.method == Method::Method1);
    assert(emeraldStatic.originSeed == 0x00017734u);
    const uint8_t staticNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(emeraldStatic.originSeed) % 25u);
    assert(staticNature == 1u);
    assert(MH::Detail::reversalWindow(
        emeraldStatic.originSeed, staticNature) == 0u);
    assert((MH::Detail::upper16(emeraldStatic.originSeed) & 1u) == 1u);
    assert(!MH::Detail::hoennSafariBlockProc(
        MH::Detail::hoennSafariBlockSeed(emeraldStatic.originSeed)));
    const uint32_t staticFrame = Gen3PidIv::Detail::prev(
        emeraldStatic.originSeed);
    const uint32_t staticLevelSeed = Gen3PidIv::Detail::prev(staticFrame);
    const uint32_t staticSlotSeed = Gen3PidIv::Detail::prev(staticLevelSeed);
    const uint32_t staticProcSeed = Gen3PidIv::Detail::prev(staticSlotSeed);
    assert(MH::Detail::upper16(staticLevelSeed) == 8908u);
    assert(MH::Detail::upper16(staticSlotSeed) == 7888u);
    assert(MH::Detail::upper16(staticProcSeed) == 30914u);
    assert((MH::Detail::upper16(staticProcSeed) & 1u) == 0u);
    assert((MH::Detail::upper16(staticSlotSeed) % 2u) == 0u);

    const auto pikachuStatic = MH::analyze(
        "emerald_gba", 25, 57, 25, 0, 0xA2B5D929u, emeraldStatic);
    assert(pikachuStatic.resolution == MH::Resolution::FrameMatched);
    assert(pikachuStatic.path == MH::Path::EmeraldStaticSuccess);
    assert(pikachuStatic.requiredBall == Gen3Safari::kSafariBall);
    assert(pikachuStatic.sourceSpecies == 25);
    assert(pikachuStatic.encounterType == 0);
    assert(pikachuStatic.slot == 8);
    assert(pikachuStatic.frameSeed == staticFrame);

    // StaticIndex 1 instead of 0 is not satisfied by the same -2 ESV,
    // so the same PID/IV cannot justify Pikachu's fixed level 27 slot 10.
    assert(MH::analyze(
        "emerald_gba", 25, 57, 27, 0, 0xA2B5D929u,
        emeraldStatic).resolution == MH::Resolution::Unresolved);

    // Emerald Safari 300-call nature-block Pressure-success. Oddish slot 1
    // is level 27, but its original EncounterArea3 #182 also has slot 3
    // Oddish level 29. PressureLevel therefore forces 29, overriding -2.
    // The -3 block slot roll 33 selects slot 1. p0 is odd and nature 1
    // differs from the PID nature 22: neither no-block Synchronize nor
    // ordinary no-block nature can account for this frame.
    const auto grassBlockPressure = Gen3PidIv::analyze(
        0xD5343194u, {4,16,19,1,0,13});
    assert(grassBlockPressure.method == Method::Method1);
    assert(grassBlockPressure.originSeed == 0x00010080u);
    const uint8_t blockPressureNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(grassBlockPressure.originSeed) % 25u);
    assert(blockPressureNature == 22u);
    assert(MH::Detail::upper16(grassBlockPressure.originSeed) == 1u);
    assert(MH::Detail::reversalWindow(
        grassBlockPressure.originSeed, blockPressureNature) == 0u);
    const uint32_t grassBlockPressureFrame =
        MH::Detail::hoennSafariBlockSeed(grassBlockPressure.originSeed);
    assert(grassBlockPressureFrame == 0x06182B34u);
    assert(MH::Detail::hoennSafariBlockProc(grassBlockPressureFrame));
    assert(MH::Detail::upper16(grassBlockPressureFrame) % 100u == 60u);
    // Pinned MethodH.IsSlotValidHustleVital: -1 successful proc (odd),
    // -2 level call consumed but overridden, -3 ordinary slot.
    const uint32_t grassBlockProcSeed =
        Gen3PidIv::Detail::prev(grassBlockPressureFrame);
    const uint32_t grassBlockLevelSeed =
        Gen3PidIv::Detail::prev(grassBlockProcSeed);
    const uint32_t grassBlockSlotSeed =
        Gen3PidIv::Detail::prev(grassBlockLevelSeed);
    assert(MH::Detail::upper16(grassBlockProcSeed) == 12795u);
    assert((MH::Detail::upper16(grassBlockProcSeed) & 1u) == 1u);
    assert(MH::Detail::upper16(grassBlockLevelSeed) == 29390u);
    assert(MH::Detail::upper16(grassBlockSlotSeed) == 433u);
    assert(MH::Detail::upper16(grassBlockSlotSeed) % 100u == 33u);

    bool foundPressureArea = false;
    for (const auto& row : Gen3Safari::kGen3SafariEntries) {
        if (row.game == 2 && row.species == 43 &&
            row.areaIndex == 182 && row.slot == 1 && row.method == 0) {
            assert(row.maxLevel == 27);
            assert(MH::Detail::grassPressureLevel(row) == 29);
            foundPressureArea = true;
        }
    }
    assert(foundPressureArea);

    const auto oddishBlockPressure = MH::analyze(
        "emerald_gba", 43, 57, 29, 0, 0xD5343194u,
        grassBlockPressure);
    assert(oddishBlockPressure.resolution == MH::Resolution::FrameMatched);
    assert(oddishBlockPressure.path ==
           MH::Path::EmeraldSafariBlockPressureSuccess);
    assert(oddishBlockPressure.requiredBall == Gen3Safari::kSafariBall);
    assert(oddishBlockPressure.sourceSpecies == 43);
    assert(oddishBlockPressure.encounterType == 0);
    assert(oddishBlockPressure.slot == 1);
    assert(oddishBlockPressure.frameSeed == grassBlockPressureFrame);

    // No matching source slot forces Oddish level 28 in this frame.
    // Unsupported competing histories remain Unresolved, never Invalid.
    assert(MH::analyze(
        "emerald_gba", 43, 57, 28, 0, 0xD5343194u,
        grassBlockPressure).resolution == MH::Resolution::Unresolved);

    // Hoenn Safari-block Static-success is a distinct MethodH context from
    // no-block Static. The pinned 300-call rewind yields block frame 0x7B4A6F2E;
    // the ordinary -2 ESV would select grass slot 1, NOT Pikachu slot 8.
    // p0=1 (odd), but p0 %25=1 differs from sequential PID nature 14;
    // neither ordinary-nature nor no-block Static can prove this encounter.
    const auto blockStatic = Gen3PidIv::analyze(
        0xF1116E24u, {26,26,14,10,24,23});
    assert(blockStatic.method == Method::Method1);
    assert(blockStatic.originSeed == 0x0001005Au);
    const uint8_t blockStaticNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(blockStatic.originSeed) % 25u);
    assert(blockStaticNature == 14u);
    assert(MH::Detail::upper16(blockStatic.originSeed) == 1u);
    assert(MH::Detail::reversalWindow(
        blockStatic.originSeed, blockStaticNature) == 0u);
    const uint32_t blockStaticFrame = MH::Detail::hoennSafariBlockSeed(
        blockStatic.originSeed);
    assert(blockStaticFrame == 0x7B4A6F2Eu);
    assert(MH::Detail::hoennSafariBlockProc(blockStaticFrame));
    assert(MH::Detail::upper16(blockStaticFrame) % 100u == 62u);
    const uint32_t blockLevelSeed = Gen3PidIv::Detail::prev(blockStaticFrame);
    const uint32_t blockSlotSeed = Gen3PidIv::Detail::prev(blockLevelSeed);
    const uint32_t blockProcSeed = Gen3PidIv::Detail::prev(blockSlotSeed);
    assert(MH::Detail::upper16(blockLevelSeed) == 7904u);
    assert(MH::Detail::upper16(blockSlotSeed) == 37234u);
    assert(MH::Detail::upper16(blockProcSeed) == 6082u);
    assert((MH::Detail::upper16(blockProcSeed) & 1u) == 0u);
    assert((MH::Detail::upper16(blockSlotSeed) % 2u) == 0u);
    assert(MH::Detail::upper16(blockSlotSeed) % 100u == 34u);

    const auto pikachuBlockStatic = MH::analyze(
        "emerald_gba", 25, 57, 25, 0, 0xF1116E24u, blockStatic);
    assert(pikachuBlockStatic.resolution == MH::Resolution::FrameMatched);
    assert(pikachuBlockStatic.path == MH::Path::EmeraldSafariBlockStaticSuccess);
    assert(pikachuBlockStatic.requiredBall == Gen3Safari::kSafariBall);
    assert(pikachuBlockStatic.sourceSpecies == 25);
    assert(pikachuBlockStatic.encounterType == 0);
    assert(pikachuBlockStatic.slot == 8);
    assert(pikachuBlockStatic.frameSeed == blockStaticFrame);

    // StaticIndex 1 (Pikachu slot 10, level 27) fails the even -2 index.
    // No other proven history can justify that level with this same frame.
    assert(MH::analyze(
        "emerald_gba", 25, 57, 27, 0, 0xF1116E24u,
        blockStatic).resolution == MH::Resolution::Unresolved);

    // Emerald Safari Intimidate/Keen Eye not-repelled encounter check.
    // Pinned MethodH.IsSlotValidIntimidate only allows the encounter when
    // the -1 level-adequacy proc is even. The ordinary -1 level is 21,
    // and Pressure-failure -2 level would also be 21, while the actual
    // -2 level here is 22. The -3 Surf ESV yields Psyduck slot 0.
    const auto emeraldIntimidate = Gen3PidIv::analyze(
        0x87D5DA8Cu, {5,30,18,7,14,1});
    assert(emeraldIntimidate.method == Method::Method1);
    assert(emeraldIntimidate.originSeed == 0x0001356Bu);
    const uint8_t intimNature = static_cast<uint8_t>(
        MH::Detail::sequentialPid(emeraldIntimidate.originSeed) % 25u);
    assert(intimNature == 1u);
    assert(MH::Detail::reversalWindow(
        emeraldIntimidate.originSeed, intimNature) == 0u);
    const uint32_t intimBlock =
        MH::Detail::hoennSafariBlockSeed(emeraldIntimidate.originSeed);
    assert(!MH::Detail::hoennSafariBlockProc(intimBlock));
    assert((MH::Detail::upper16(intimBlock) % 100u) == 88u);
    const uint32_t intimFrame = Gen3PidIv::Detail::prev(
        emeraldIntimidate.originSeed);
    const uint32_t intimProcSeed = Gen3PidIv::Detail::prev(intimFrame);
    const uint32_t intimLevelSeed = Gen3PidIv::Detail::prev(intimProcSeed);
    const uint32_t intimSlotSeed = Gen3PidIv::Detail::prev(intimLevelSeed);
    assert(MH::Detail::upper16(intimProcSeed) == 32638u);
    assert((MH::Detail::upper16(intimProcSeed) & 1u) == 0u);
    assert((MH::Detail::upper16(intimProcSeed) % 3u) != 0u);
    assert(MH::Detail::upper16(intimLevelSeed) == 7537u);
    assert(MH::Detail::upper16(intimSlotSeed) == 17910u);
    assert(MH::Detail::upper16(intimSlotSeed) % 100u < 60u);

    const auto psyduckIntimidate = MH::analyze(
        "emerald_gba", 54, 57, 22, 0, 0x87D5DA8Cu,
        emeraldIntimidate);
    assert(psyduckIntimidate.resolution == MH::Resolution::FrameMatched);
    assert(psyduckIntimidate.path ==
           MH::Path::EmeraldIntimidateKeenEyeCheckFailed);
    assert(psyduckIntimidate.requiredBall == Gen3Safari::kSafariBall);
    assert(psyduckIntimidate.sourceSpecies == 54);
    assert(psyduckIntimidate.encounterType == 1);
    assert(psyduckIntimidate.slot == 0);
    assert(psyduckIntimidate.frameSeed == intimFrame);

    // This source frame does not prove a different met level. Unknown
    // competing history remains Unresolved, never a hard Invalid verdict.
    assert(MH::analyze(
        "emerald_gba", 54, 57, 23, 0, 0x87D5DA8Cu,
        emeraldIntimidate).resolution == MH::Resolution::Unresolved);

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
