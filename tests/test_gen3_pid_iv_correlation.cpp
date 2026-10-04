#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen3WondercardEggEventTemplate.h"
#include "Legality/Gen3MethodHSlot.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3PidIv::Method;
    using Legality::Gen3PidIv::analyze;

    // Cross-check vectors from PKHeX Tests/PKHeX.Core.Tests/Entity/PIDIVTests.cs.
    assert(analyze(0xE97E0000u, {17,19,20,16,13,12}).method == Method::Method1);
    assert(analyze(0x5271E97Eu, {2,18,3,12,22,24}).method == Method::Method2);
    assert(analyze(0x3DD1BB49u, {23,12,31,9,3,3}).method == Method::Method3);
    assert(analyze(0x31B05271u, {2,18,3,5,30,11}).method == Method::Method4);

    assert(analyze(0x815549A2u, {2,26,30,30,11,26}, true).method == Method::Method1Unown);
    assert(analyze(0x8A7B5190u, {14,2,21,30,29,15}, true).method == Method::Method2Unown);
    assert(analyze(0xBB493DD1u, {23,12,31,9,3,3}, true).method == Method::Method3Unown);
    assert(analyze(0x5FA80D70u, {2,6,3,26,4,19}, true).method == Method::Method4Unown);

    // Pinned Gen III Wondercard event eggs accept their native Method 2
    // correlation and the documented Method 1 / Method 4 VBlank outcomes.
    namespace WcEgg = Legality::Gen3WondercardEggEvent;
    const auto m1 = analyze(0xE97E0000u, {17,19,20,16,13,12});
    const auto m2 = analyze(0x5271E97Eu, {2,18,3,12,22,24});
    const auto m4 = analyze(0x31B05271u, {2,18,3,5,30,11});

    WcEgg::Candidate farfetchd{
        83, 4, 1, 0, 255, 4, true, true, {281, 273, 0, 0}
    };
    assert(WcEgg::matches(farfetchd, m1));
    assert(WcEgg::matches(farfetchd, m2));
    assert(WcEgg::matches(farfetchd, m4));

    WcEgg::Candidate pokeParkPichu{
        172, 3, 1, 0, 255, 4, true, true, {84, 204, 266, 0}
    };
    assert(WcEgg::matches(pokeParkPichu, m2));

    auto wrongGame = farfetchd;
    wrongGame.originGame = 3; // PCNY/PCJP Wish eggs are FR/LG only.
    assert(!WcEgg::matches(wrongGame, m2));
    auto wrongMoves = pokeParkPichu;
    wrongMoves.moves = {84, 204, 273, 0};
    assert(!WcEgg::matches(wrongMoves, m2));
    auto notFateful = pokeParkPichu;
    notFateful.fateful = false;
    assert(!WcEgg::matches(notFateful, m2));
    auto wrongLanguage = farfetchd;
    wrongLanguage.language = 2;
    assert(!WcEgg::matches(wrongLanguage, m2));

    // One IV changed from the known Method 1 vector: no handheld 1/2/3/4 correlation.
    assert(analyze(0xE97E0000u, {17,19,21,16,13,12}).method == Method::None);

    // Deterministic Gen III truncated-roamer vector.
    // Origin seed 0x12345678 => PID 0x84EA0B71, generated IV32 low byte 0x8A.
    const auto roamer =
        Legality::Gen3PidIv::analyzeRoamer(0x84EA0B71u, {10,4,0,0,0,0});
    assert(roamer.method == Method::Method1Roamer);
    assert(roamer.originSeed == 0x12345678u);
    assert(Legality::Gen3PidIv::analyzeRoamer(
               0x84EA0B71u, {10,4,1,0,0,0}).method == Method::None);
    assert(Legality::Gen3PidIv::isRoamerSpecies(243));
    assert(Legality::Gen3PidIv::isRoamerSpecies(381));
    assert(!Legality::Gen3PidIv::isRoamerSpecies(150));

    // Pinned PKHeX SlotMethodH probabilities, with Super Rod resolved against
    // decompiled Emerald encounter data because PKHeX's inverse helper is stale.
    namespace HSlot = Legality::Gen3MethodHSlot;
    struct SlotShape {
        HSlot::Type type;
        std::array<uint8_t, 12> expected{};
        uint8_t count = 0;
    };
    constexpr std::array<SlotShape, 6> shapes{{
        {HSlot::Type::Grass,     {20,20,10,10,10,10,5,5,4,4,1,1}, 12},
        {HSlot::Type::Surf,      {60,30,5,4,1}, 5},
        {HSlot::Type::OldRod,    {70,30}, 2},
        {HSlot::Type::GoodRod,   {60,20,20}, 3},
        {HSlot::Type::SuperRod,  {40,40,15,4,1}, 5},
        {HSlot::Type::RockSmash, {60,30,5,4,1}, 5},
    }};

    for (const auto& shape : shapes) {
        std::array<uint8_t, 12> counts{};
        for (uint32_t roll = 0; roll < 100; ++roll) {
            const uint8_t slot = HSlot::get(shape.type, roll);
            assert(slot != HSlot::kInvalid);
            assert(slot < shape.count);
            ++counts[slot];
            assert(HSlot::range(shape.type, slot).contains(
                static_cast<uint8_t>(roll)));
        }
        for (uint8_t slot = 0; slot < shape.count; ++slot)
            assert(counts[slot] == shape.expected[slot]);
        assert(!HSlot::range(shape.type, shape.count).valid());
    }

    for (const auto type : {HSlot::Type::SwarmFish50,
                            HSlot::Type::SwarmGrass50}) {
        for (uint32_t roll = 0; roll < 100; ++roll) {
            const uint8_t slot = HSlot::get(type, roll);
            if (roll < 50)
                assert(slot == 0);
            else
                assert(slot == HSlot::kInvalid);
        }
        assert(HSlot::range(type, 0).min == 0);
        assert(HSlot::range(type, 0).max == 49);
        assert(!HSlot::range(type, 1).valid());
    }

    // SlotMethodH consumes the raw upper RNG half modulo 100, not a pre-reduced
    // byte. Exercise values beyond 99 to lock that source behavior.
    assert(HSlot::get(HSlot::Type::Grass, 100) == 0);
    assert(HSlot::get(HSlot::Type::Grass, 199) == 11);
    assert(HSlot::get(HSlot::Type::SuperRod, 170) == 1);
    assert(HSlot::get(HSlot::Type::SuperRod, 180) == 2);

    std::cout << "Gen III PID/IV handheld + roamer + Method H slot evidence: PASS\n";
}
