#include "Legality/Gen4WildRngCorrelation.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t minimum, uint8_t maximum,
                           uint8_t rate = 0) {
    return (static_cast<uint64_t>(minimum) << 17) |
           (static_cast<uint64_t>(maximum) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
}
}

int main() {
    using namespace Legality::Gen4WildRng;

    // PKHeX MethodJChecks: ModestFlawless (TimidFlawless ^ 0x80000000)
    // can reach Grass slot 6 with LeadRequired.None.
    constexpr uint32_t methodJSeed = 0x469FB838u;
    constexpr uint32_t methodJPid = sequentialPid(methodJSeed);
    constexpr uint64_t jSlot6 = makeRow(0, 6, 5, 5);
    constexpr auto j = matchNoLeadRow(false, jSlot6, methodJSeed, methodJPid, 5);
    static_assert(j.method == Method::MethodJNoLead);
    static_assert(j.slot == 6);

    // PKHeX marks slot 3 impossible for the same Method J pre-PID seed.
    constexpr uint64_t jSlot3 = makeRow(0, 3, 5, 5);
    static_assert(!matchNoLeadRow(false, jSlot3, methodJSeed, methodJPid, 5).matched());

    // Deterministic HG/SS Method K no-lead vector. Seed 0x0000000D has
    // nature roll 0 and maps the preceding Grass slot roll to slot 4.
    constexpr uint32_t methodKSeed = 0x0000000Du;
    constexpr uint32_t methodKPid = sequentialPid(methodKSeed);
    static_assert((methodKPid % 25u) == 0);
    constexpr uint64_t kSlot4 = makeRow(0, 4, 5, 5);
    constexpr auto k = matchNoLeadRow(true, kSlot4, methodKSeed, methodKPid, 5);
    static_assert(k.method == Method::MethodKNoLead);
    static_assert(k.slot == 4);

    constexpr uint64_t kWrong = makeRow(0, 5, 5, 5);
    static_assert(!matchNoLeadRow(true, kWrong, methodKSeed, methodKPid, 5).matched());

    // Successful Synchronize is a separate frame branch: the ordinary nature roll
    // does not equal the PID nature, but the 50% Synchronize proc passes.
    constexpr uint32_t syncJSeed = 0u;
    constexpr uint32_t syncJPid = sequentialPid(syncJSeed);
    static_assert(syncJPid == 0xE97E0000u);
    constexpr uint64_t syncJGrass = makeRow(0, 0, 5, 5);
    static_assert(!matchNoLeadRow(false, syncJGrass,
                                  syncJSeed, syncJPid, 5).matched());
    constexpr auto syncJ =
        matchSynchronizeRow(false, syncJGrass, syncJSeed, syncJPid, 5);
    static_assert(syncJ.method == Method::MethodJSynchronize);
    static_assert(syncJ.slot == 0);

    constexpr uint32_t syncKSeed = 1u;
    constexpr uint32_t syncKPid = sequentialPid(syncKSeed);
    static_assert(syncKPid == 0xAC2141C6u);
    constexpr uint64_t syncKGrass = makeRow(0, 1, 5, 5);
    static_assert(!matchNoLeadRow(true, syncKGrass,
                                  syncKSeed, syncKPid, 5).matched());
    constexpr auto syncK =
        matchSynchronizeRow(true, syncKGrass, syncKSeed, syncKPid, 5);
    static_assert(syncK.method == Method::MethodKSynchronize);
    static_assert(syncK.slot == 1);

    // Surf adds the random-level frame while retaining the same Synchronize proc.
    constexpr uint64_t syncJSurf = makeRow(1, 0, 5, 10);
    constexpr auto syncJS =
        matchSynchronizeRow(false, syncJSurf, syncJSeed, syncJPid, 8);
    static_assert(syncJS.method == Method::MethodJSynchronize);
    static_assert(!matchSynchronizeRow(false, syncJSurf,
                                       syncJSeed, syncJPid, 7).matched());

    constexpr uint64_t syncKSurf = makeRow(1, 1, 5, 10);
    constexpr auto syncKS =
        matchSynchronizeRow(true, syncKSurf, syncJSeed, syncJPid, 8);
    static_assert(syncKS.method == Method::MethodKSynchronize);

    // D/P/Pt Old Rod Synchronize: seed 1 reaches slot 0 / level 6 and the
    // 25% hook activation succeeds. The no-lead branch does not prove it.
    constexpr uint64_t syncJOldRod = makeRow(2, 0, 5, 10);
    static_assert(!matchNoLeadRow(false, syncJOldRod,
                                  syncKSeed, syncKPid, 6).matched());
    static_assert(matchSynchronizeRow(false, syncJOldRod,
                                      syncKSeed, syncKPid, 6).method ==
                  Method::MethodJSynchronize);

    // HG/SS Old Rod seed 0 reaches slot 2 / level 8 on the normal activation
    // path; Synchronize cannot claim the separate Suction Cups fallback.
    constexpr uint64_t syncKOldRod = makeRow(2, 2, 5, 10);
    static_assert(!matchNoLeadRow(true, syncKOldRod,
                                  syncJSeed, syncJPid, 8).matched());
    static_assert(matchSynchronizeRow(true, syncKOldRod,
                                      syncJSeed, syncJPid, 8).method ==
                  Method::MethodKSynchronize);

    // HG/SS normal Rock Smash activation is compatible with Synchronize.
    // Seed 6's activation roll is 9 against rate 20, so Illuminate is not needed.
    constexpr uint32_t syncRockSeed = 6u;
    constexpr uint32_t syncRockPid = sequentialPid(syncRockSeed);
    static_assert(syncRockPid == 0x794E8AA6u);
    constexpr uint64_t syncRock = makeRow(5, 0, 5, 10, 20);
    static_assert(!matchNoLeadRow(true, syncRock,
                                  syncRockSeed, syncRockPid, 5).matched());
    static_assert(matchSynchronizeRow(true, syncRock,
                                      syncRockSeed, syncRockPid, 5).method ==
                  Method::MethodKSynchronize);

    // Headbutt has no additional encounter-activation proc after slot+level.
    constexpr uint64_t syncHeadbutt = makeRow(6, 2, 5, 10);
    static_assert(!matchNoLeadRow(true, syncHeadbutt,
                                  syncJSeed, syncJPid, 8).matched());
    static_assert(matchSynchronizeRow(true, syncHeadbutt,
                                      syncJSeed, syncJPid, 8).method ==
                  Method::MethodKSynchronize);

    // Honey Tree ESV is pre-determined; Synchronize only adds the nature proc
    // around the existing Method J level frame.
    constexpr uint64_t syncHoney = makeRow(9, 0, 5, 15);
    static_assert(!matchNoLeadRow(false, syncHoney,
                                  syncJSeed, syncJPid, 5).matched());
    static_assert(matchSynchronizeRow(false, syncHoney,
                                      syncJSeed, syncJPid, 5).method ==
                  Method::MethodJSynchronize);

    // Keep unsupported reroll/special-combination branches fail-closed.
    constexpr uint64_t syncContestUnsupported = makeRow(8, 0, 7, 18, 25);
    static_assert(!matchSynchronizeRow(true, syncContestUnsupported,
                                       syncJSeed, syncJPid, 8).matched());
    constexpr uint64_t syncSafariUnsupported = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchSynchronizeRow(true, syncSafariUnsupported,
                                       syncJSeed, syncJPid, 15).matched());

    // D/P/Pt Old Rod positive vector for the current reversal-window interpretation:
    // pre-PID seed 69 rolls slot 0, level 7, and passes the 25% rod activation frame.
    constexpr uint32_t fishingJSeed = 69u;
    constexpr uint32_t fishingJPid = sequentialPid(fishingJSeed);
    constexpr uint64_t fishingJ = makeRow(2, 0, 5, 10);
    constexpr auto jf = matchNoLeadRow(false, fishingJ, fishingJSeed, fishingJPid, 7);
    static_assert(jf.method == Method::MethodJFishingNoLead);
    static_assert(jf.slot == 0);
    static_assert(!matchNoLeadRow(false, fishingJ, fishingJSeed, fishingJPid, 8).matched());

    // HG/SS Old Rod positive vector for the current reversal-window interpretation:
    // pre-PID seed 20 rolls slot 0, level 8, and passes the normal/following-Pokemon
    // rod activation path without a lead ability.
    constexpr uint32_t fishingKSeed = 20u;
    constexpr uint32_t fishingKPid = sequentialPid(fishingKSeed);
    constexpr uint64_t fishingK = makeRow(2, 0, 5, 10);
    constexpr auto kf = matchNoLeadRow(true, fishingK, fishingKSeed, fishingKPid, 8);
    static_assert(kf.method == Method::MethodKFishingNoLead);
    static_assert(kf.slot == 0);

    // HG/SS Old Rod can require Suction Cups / Sticky Hold after the
    // following-Pokemon +50 rate bonus. Seed 0 reaches slot 0 / level 6, but
    // its activation roll is 87: normal Old Rod (75%) fails and Suction Cups passes.
    constexpr uint32_t suctionSeed = 0u;
    constexpr uint32_t suctionPid = sequentialPid(suctionSeed);
    static_assert(suctionPid == 0xE97E0000u);
    constexpr uint64_t suctionFishing = makeRow(2, 0, 5, 10);
    constexpr auto kfs = matchNoLeadRow(true, suctionFishing,
                                        suctionSeed, suctionPid, 6);
    static_assert(kfs.method == Method::MethodKFishingSuctionCups);
    static_assert(kfs.slot == 0);

    // The same deterministic Method K seed maps Headbutt's 23% slot roll to slot 0
    // and the preceding level roll to level 8.
    constexpr uint64_t headbutt = makeRow(6, 0, 5, 10);
    constexpr auto kh = matchNoLeadRow(true, headbutt, fishingKSeed, fishingKPid, 8);
    static_assert(kh.method == Method::MethodKHeadbuttNoLead);
    static_assert(kh.slot == 0);
    constexpr uint64_t headbuttSpecial = makeRow(7, 0, 5, 10);
    static_assert(matchNoLeadRow(true, headbuttSpecial,
                                 fishingKSeed, fishingKPid, 8).matched());
    static_assert(!matchNoLeadRow(false, headbutt,
                                  fishingKSeed, fishingKPid, 8).matched());

    // HG/SS Rock Smash requires the area encounter rate in addition to slot + level.
    // Seed 20 yields slot 0 and level 8; rate 100 guarantees the normal no-lead trigger.
    constexpr uint64_t rockSmash = makeRow(5, 0, 5, 10, 100);
    constexpr auto kr = matchNoLeadRow(true, rockSmash,
                                       fishingKSeed, fishingKPid, 8);
    static_assert(kr.method == Method::MethodKRockSmashNoLead);
    static_assert(kr.slot == 0);
    constexpr uint64_t impossibleRockSmashRate = makeRow(5, 0, 5, 10, 0);
    static_assert(!matchNoLeadRow(true, impossibleRockSmashRate,
                                  fishingKSeed, fishingKPid, 8).matched());
    static_assert(!matchNoLeadRow(false, rockSmash,
                                  fishingKSeed, fishingKPid, 8).matched());

    // HG/SS Rock Smash can use Illuminate when the normal area-rate roll
    // fails but remains below twice the encounter rate. Seed 13 gives roll 33
    // against rate 20, so this is positive Illuminate-only evidence.
    constexpr uint32_t illuminateSeed = 13u;
    constexpr uint32_t illuminatePid = sequentialPid(illuminateSeed);
    static_assert(illuminatePid == 0xCBC05712u);
    constexpr uint64_t illuminateRock = makeRow(5, 0, 5, 10, 20);
    constexpr auto kri = matchNoLeadRow(true, illuminateRock,
                                        illuminateSeed, illuminatePid, 5);
    static_assert(kri.method == Method::MethodKRockSmashIlluminate);
    static_assert(kri.slot == 0);

    // D/P/Pt Honey Trees do not use the ordinary encounter-slot roll.
    // Deterministic Method J vector: seed 29 has a single valid nature-reversal
    // candidate whose Honey Tree level roll is 9 (5 + 26507 / 0x1745).
    constexpr uint32_t honeySeed = 29u;
    constexpr uint32_t honeyPid = sequentialPid(honeySeed);
    constexpr uint64_t honeyTree = makeRow(9, 0, 5, 15);
    constexpr auto jh = matchNoLeadRow(false, honeyTree,
                                       honeySeed, honeyPid, 9);
    static_assert(jh.method == Method::MethodJHoneyTreeNoLead);
    static_assert(!matchNoLeadRow(false, honeyTree,
                                  honeySeed, honeyPid, 10).matched());
    static_assert(!matchNoLeadRow(true, honeyTree,
                                  honeySeed, honeyPid, 9).matched());

    // Actual D/P Honey Tree Munchlax rows additionally require one of the
    // trainer-ID-selected group-C trees. ID32=0 allows locations 20/21/22,
    // but not location 23.
    const auto munchlaxAllowed = analyzeNoLead(
        "diamond_nds", 446, 20, 9, 0, 0u, honeySeed, honeyPid);
    assert(munchlaxAllowed.method == Method::MethodJHoneyTreeNoLead);
    assert(!analyzeNoLead(
        "diamond_nds", 446, 23, 9, 0, 0u, honeySeed, honeyPid).matched());

    // HG/SS Bug Catching Contest: slot table is reversed and the accepted
    // current candidate must contain at least one 31 IV unless the full four-attempt
    // exhaustion history is proven. Seed 8 has a direct 31-IV Method-1 candidate.
    constexpr uint32_t contestSeed = 8u;
    constexpr uint32_t contestPid = sequentialPid(contestSeed);
    static_assert(contestPid == 0xFE930E32u);
    static_assert(directMinimum31Satisfied(contestSeed));
    constexpr uint64_t bugContest = makeRow(8, 0, 7, 18, 25);
    constexpr auto kb = matchNoLeadRow(true, bugContest, contestSeed, contestPid, 14);
    static_assert(kb.method == Method::MethodKBugContestNoLead);
    static_assert(kb.slot == 0);
    constexpr uint64_t wrongContestSlot = makeRow(8, 1, 7, 18, 25);
    static_assert(!matchNoLeadRow(true, wrongContestSlot,
                                  contestSeed, contestPid, 14).matched());

    // Shared slot helper must preserve the Safari modulo-10 rule too,
    // even though matchNoLeadRow handles Safari before the generic fishing branch.
    static_assert(fishingSlot(true, 12, 28111) == 1);
    static_assert(fishingSlot(false, 12, 28111) == 0xFF);

    // HG/SS Safari uses rand % 10 for all five Safari method types and does not
    // consume a random-level frame. The pinned resource stores fixed levels.
    constexpr uint64_t safariGrass = makeRow(10, 1, 15, 15, 6);
    constexpr auto ks = matchNoLeadRow(true, safariGrass,
                                       contestSeed, contestPid, 15);
    static_assert(ks.method == Method::MethodKSafariNoLead);
    static_assert(ks.slot == 1);
    static_assert(!matchNoLeadRow(true, safariGrass,
                                  contestSeed, contestPid, 16).matched());

    // Safari Old Rod uses the same fixed-level Safari slot frame; activation is
    // immediately before the ESV because there is no random-level call.
    constexpr uint32_t safariFishingSeed = 22u;
    constexpr uint32_t safariFishingPid = sequentialPid(safariFishingSeed);
    static_assert(safariFishingPid == 0xA377A70Bu);
    static_assert(directMinimum31Satisfied(safariFishingSeed));
    constexpr uint64_t safariOldRod = makeRow(12, 3, 12, 12, 5);
    constexpr auto ksf = matchNoLeadRow(true, safariOldRod,
                                        safariFishingSeed, safariFishingPid, 12);
    static_assert(ksf.method == Method::MethodKSafariFishingNoLead);
    static_assert(ksf.slot == 3);

    // Safari Old Rod can hit the same Suction Cups / Sticky Hold branch.
    // Seed 8's activation roll is 88, above the normal 75% path but below the
    // compounded threshold; Safari still uses the fixed-level / rand%10 slot layout.
    constexpr uint64_t safariSuctionRod = makeRow(12, 1, 12, 12, 5);
    constexpr auto ksfs = matchNoLeadRow(true, safariSuctionRod,
                                         contestSeed, contestPid, 12);
    static_assert(ksfs.method == Method::MethodKSafariFishingSuctionCups);
    static_assert(ksfs.slot == 1);

    // Seed 12 has no 31 IV but otherwise reaches a valid Bug Contest slot+level
    // and Safari slot. Such a result can be legal only as the exhausted fourth
    // reroll; until that prior three-attempt chain is proven, stay unresolved.
    constexpr uint32_t no31Seed = 12u;
    constexpr uint32_t no31Pid = sequentialPid(no31Seed);
    static_assert(no31Pid == 0x091D154Cu);
    static_assert(!directMinimum31Satisfied(no31Seed));
    constexpr uint64_t no31Contest = makeRow(8, 4, 7, 18, 25);
    static_assert(!matchNoLeadRow(true, no31Contest,
                                  no31Seed, no31Pid, 15).matched());
    constexpr uint64_t no31Safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchNoLeadRow(true, no31Safari,
                                  no31Seed, no31Pid, 15).matched());

    // Mt. Coronet B1F consumes an extra tile-check RNG frame for every
    // fishing encounter. Seed 7 proves both the regular-species path and Feebas's
    // additional 50% replacement branch using the pinned rate=255 sentinel.
    constexpr uint32_t coronetSeed = 7u;
    constexpr uint32_t coronetPid = sequentialPid(coronetSeed);
    static_assert(coronetPid == 0x3BF0CC6Cu);

    constexpr uint64_t coronetRegular =
        makeRow(3, 1, 15, 20, 0xFF) | static_cast<uint64_t>(129u);
    constexpr auto jrCoronet =
        matchNoLeadRow(false, coronetRegular, coronetSeed, coronetPid, 17);
    static_assert(jrCoronet.method == Method::MethodJFishingNoLead);
    static_assert(jrCoronet.slot == 1);

    constexpr uint64_t feebasFishing =
        makeRow(3, 1, 10, 20, 0xFF) | static_cast<uint64_t>(349u);
    constexpr auto jfCoronet =
        matchNoLeadRow(false, feebasFishing, coronetSeed, coronetPid, 18);
    static_assert(jfCoronet.method == Method::MethodJFishingNoLead);
    static_assert(jfCoronet.slot == 1);

    // Seed 9 reaches a valid Good Rod slot/level and hook roll, but its tile
    // replacement bit is clear, so it cannot positively prove Feebas.
    constexpr uint32_t badTileSeed = 9u;
    constexpr uint32_t badTilePid = sequentialPid(badTileSeed);
    static_assert(badTilePid == 0xC1354FF9u);
    constexpr uint64_t badTileFeebas =
        makeRow(3, 0, 10, 20, 0xFF) | static_cast<uint64_t>(349u);
    static_assert(!matchNoLeadRow(false, badTileFeebas,
                                  badTileSeed, badTilePid, 16).matched());

    // The packed wild table reader must retain slot bits emitted by the generator.
    constexpr uint64_t packedSample = 0x20000060c260aULL;
    static_assert(Legality::Gen4Wild::slot(packedSample) == 8);
    constexpr uint64_t rateSample = packedSample | (static_cast<uint64_t>(25) << 50);
    static_assert(Legality::Gen4Wild::rate(rateSample) == 25);

    std::cout << "Gen IV Method J/K no-lead wild RNG evidence: PASS\n";
}
