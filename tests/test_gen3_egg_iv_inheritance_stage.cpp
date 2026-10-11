#include "Legality/Gen3EggIVInheritanceStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

// These 24 are synthetic RNG/IV-stage vectors from Codex #252 RNG-001:
// unchanged retail-stage C corroborated by independently modeled Python.
// NOT native PK3 specimens, encounter candidates, or complete egg PID tests.
int main(){
    namespace G=Legality::Gen3EggIVInheritanceStage;
    constexpr std::array<uint8_t,6> initial{0,0,0,0,0,0};
    constexpr std::array<uint8_t,6> low{30,30,30,30,30,30};
    constexpr std::array<uint8_t,6> high{31,31,31,31,31,31};
    static_assert(G::family("sapphire_gba")==G::Family::RubySapphire);
    static_assert(G::family("leafgreen_gba")==G::Family::FireRedLeafGreen);
    static_assert(G::family("platinum_nds")==G::Family::Unknown);
    static_assert(G::replay("platinum_nds",0,initial,low,high,true).status==
                  G::Status::UnsupportedGame);
    static_assert(G::replay("emerald_gba",0,initial,low,high,false).status==
                  G::Status::UnknownParentHistory);
    { // Executed independent pinned stage-vector 1/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x00000000u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{0,5,3});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,0});
        static_assert(r.draws==std::array<uint16_t,6>{0,59774,21105,12720,36418,58060});
        static_assert(r.childIVs==std::array<uint8_t,6>{30,0,0,30,0,30});
        static_assert(r.finalSeed==0xE2CCA5EEu);
        assert(r.matches(std::array<uint8_t,6>{30,0,0,30,0,30}));
    }
    { // Executed independent pinned stage-vector 2/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x00000001u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{2,1,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,1,0});
        static_assert(r.draws==std::array<uint16_t,6>{16838,44065,53998,8119,31203,46760});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,31,31,0,30,0});
        static_assert(r.finalSeed==0xB6A8FDE7u);
        assert(r.matches(std::array<uint8_t,6>{0,31,31,0,30,0}));
    }
    { // Executed independent pinned stage-vector 3/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x00000002u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{4,1,1});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{33676,28355,21356,3517,25988,35461});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,31,0,0,31,0});
        static_assert(r.finalSeed==0x8A8555E0u);
        assert(r.matches(std::array<uint8_t,6>{0,31,0,0,31,0}));
    }
    { // Executed independent pinned stage-vector 4/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x00000003u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{1,2,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{50515,12646,54250,64452,20774,24161});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,30,30,0,31,0});
        static_assert(r.finalSeed==0x5E61ADD9u);
        assert(r.matches(std::array<uint8_t,6>{0,30,30,0,31,0}));
    }
    { // Executed independent pinned stage-vector 5/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x0000002Au,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{3,5,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{51849,55339,26386,16065,14008,42227});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,0,0,31,31,30});
        static_assert(r.finalSeed==0xA4F314C8u);
        assert(r.matches(std::array<uint8_t,6>{0,0,0,31,31,30}));
    }
    { // Executed independent pinned stage-vector 6/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x0000FFFFu,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{5,2,1});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{3239,49476,36520,49851,60126,26345});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,31,30,0,0,31});
        static_assert(r.finalSeed==0x66E94DF5u);
        assert(r.matches(std::array<uint8_t,6>{0,31,30,0,0,31}));
    }
    { // Executed independent pinned stage-vector 7/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0x12345678u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{1,2,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{2929,34026,55690,62688,9860,38967});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,30,30,0,31,0});
        static_assert(r.finalSeed==0x983788A6u);
        assert(r.matches(std::array<uint8_t,6>{0,30,30,0,31,0}));
    }
    { // Executed independent pinned stage-vector 8/24 (pokeemerald)
        constexpr auto r=G::replay("emerald_gba",0xFFFFFFFFu,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{2,3,5});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,1,0});
        static_assert(r.draws==std::array<uint16_t,6>{48698,9947,53747,17322,41633,3824});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,0,30,31,0,30});
        static_assert(r.finalSeed==0x0EF04DF5u);
        assert(r.matches(std::array<uint8_t,6>{0,0,30,31,0,30}));
    }
    { // Executed independent pinned stage-vector 9/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x00000000u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{0,5,2});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,0});
        static_assert(r.draws==std::array<uint16_t,6>{0,59774,21105,12720,36418,58060});
        static_assert(r.childIVs==std::array<uint8_t,6>{30,0,30,0,0,30});
        static_assert(r.finalSeed==0xE2CCA5EEu);
        assert(r.matches(std::array<uint8_t,6>{30,0,30,0,0,30}));
    }
    { // Executed independent pinned stage-vector 10/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x00000001u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{2,0,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,1,0});
        static_assert(r.draws==std::array<uint16_t,6>{16838,44065,53998,8119,31203,46760});
        static_assert(r.childIVs==std::array<uint8_t,6>{31,0,31,0,30,0});
        static_assert(r.finalSeed==0xB6A8FDE7u);
        assert(r.matches(std::array<uint8_t,6>{31,0,31,0,30,0}));
    }
    { // Executed independent pinned stage-vector 11/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x00000002u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{4,0,1});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{33676,28355,21356,3517,25988,35461});
        static_assert(r.childIVs==std::array<uint8_t,6>{30,31,0,0,31,0});
        static_assert(r.finalSeed==0x8A8555E0u);
        assert(r.matches(std::array<uint8_t,6>{30,31,0,0,31,0}));
    }
    { // Executed independent pinned stage-vector 12/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x00000003u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{1,2,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{50515,12646,54250,64452,20774,24161});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,30,30,0,31,0});
        static_assert(r.finalSeed==0x5E61ADD9u);
        assert(r.matches(std::array<uint8_t,6>{0,30,30,0,31,0}));
    }
    { // Executed independent pinned stage-vector 13/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x0000002Au,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{3,5,2});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{51849,55339,26386,16065,14008,42227});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,0,31,31,0,30});
        static_assert(r.finalSeed==0xA4F314C8u);
        assert(r.matches(std::array<uint8_t,6>{0,0,31,31,0,30}));
    }
    { // Executed independent pinned stage-vector 14/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x0000FFFFu,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{5,1,0});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{3239,49476,36520,49851,60126,26345});
        static_assert(r.childIVs==std::array<uint8_t,6>{31,30,0,0,0,31});
        static_assert(r.finalSeed==0x66E94DF5u);
        assert(r.matches(std::array<uint8_t,6>{31,30,0,0,0,31}));
    }
    { // Executed independent pinned stage-vector 15/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0x12345678u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{1,2,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{2929,34026,55690,62688,9860,38967});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,30,30,0,31,0});
        static_assert(r.finalSeed==0x983788A6u);
        assert(r.matches(std::array<uint8_t,6>{0,30,30,0,31,0}));
    }
    { // Executed independent pinned stage-vector 16/24 (pokeruby)
        constexpr auto r=G::replay("ruby_gba",0xFFFFFFFFu,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{2,3,5});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,1,0});
        static_assert(r.draws==std::array<uint16_t,6>{48698,9947,53747,17322,41633,3824});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,0,30,31,0,30});
        static_assert(r.finalSeed==0x0EF04DF5u);
        assert(r.matches(std::array<uint8_t,6>{0,0,30,31,0,30}));
    }
    { // Executed independent pinned stage-vector 17/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x00000000u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{0,5,2});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,0});
        static_assert(r.draws==std::array<uint16_t,6>{0,59774,21105,12720,36418,58060});
        static_assert(r.childIVs==std::array<uint8_t,6>{30,0,30,0,0,30});
        static_assert(r.finalSeed==0xE2CCA5EEu);
        assert(r.matches(std::array<uint8_t,6>{30,0,30,0,0,30}));
    }
    { // Executed independent pinned stage-vector 18/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x00000001u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{2,0,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,1,0});
        static_assert(r.draws==std::array<uint16_t,6>{16838,44065,53998,8119,31203,46760});
        static_assert(r.childIVs==std::array<uint8_t,6>{31,0,31,0,30,0});
        static_assert(r.finalSeed==0xB6A8FDE7u);
        assert(r.matches(std::array<uint8_t,6>{31,0,31,0,30,0}));
    }
    { // Executed independent pinned stage-vector 19/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x00000002u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{4,0,1});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{33676,28355,21356,3517,25988,35461});
        static_assert(r.childIVs==std::array<uint8_t,6>{30,31,0,0,31,0});
        static_assert(r.finalSeed==0x8A8555E0u);
        assert(r.matches(std::array<uint8_t,6>{30,31,0,0,31,0}));
    }
    { // Executed independent pinned stage-vector 20/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x00000003u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{1,2,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{50515,12646,54250,64452,20774,24161});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,30,30,0,31,0});
        static_assert(r.finalSeed==0x5E61ADD9u);
        assert(r.matches(std::array<uint8_t,6>{0,30,30,0,31,0}));
    }
    { // Executed independent pinned stage-vector 21/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x0000002Au,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{3,5,2});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{51849,55339,26386,16065,14008,42227});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,0,31,31,0,30});
        static_assert(r.finalSeed==0xA4F314C8u);
        assert(r.matches(std::array<uint8_t,6>{0,0,31,31,0,30}));
    }
    { // Executed independent pinned stage-vector 22/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x0000FFFFu,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{5,1,0});
        static_assert(r.parentChoices==std::array<uint8_t,3>{1,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{3239,49476,36520,49851,60126,26345});
        static_assert(r.childIVs==std::array<uint8_t,6>{31,30,0,0,0,31});
        static_assert(r.finalSeed==0x66E94DF5u);
        assert(r.matches(std::array<uint8_t,6>{31,30,0,0,0,31}));
    }
    { // Executed independent pinned stage-vector 23/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0x12345678u,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{1,2,4});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,0,1});
        static_assert(r.draws==std::array<uint16_t,6>{2929,34026,55690,62688,9860,38967});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,30,30,0,31,0});
        static_assert(r.finalSeed==0x983788A6u);
        assert(r.matches(std::array<uint8_t,6>{0,30,30,0,31,0}));
    }
    { // Executed independent pinned stage-vector 24/24 (pokefirered)
        constexpr auto r=G::replay("firered_gba",0xFFFFFFFFu,initial,low,high,true);
        static_assert(r.status==G::Status::ReplayedStage);
        static_assert(r.selectedStats==std::array<uint8_t,3>{2,3,5});
        static_assert(r.parentChoices==std::array<uint8_t,3>{0,1,0});
        static_assert(r.draws==std::array<uint16_t,6>{48698,9947,53747,17322,41633,3824});
        static_assert(r.childIVs==std::array<uint8_t,6>{0,0,30,31,0,30});
        static_assert(r.finalSeed==0x0EF04DF5u);
        assert(r.matches(std::array<uint8_t,6>{0,0,30,31,0,30}));
    }
    // Emerald can repeat inheritance of the same stat due to the retail
    // compaction bug: there need not be three DISTINCT inherited IVs.
    constexpr auto repeat=G::replay("emerald_gba",2,initial,low,high,true);
    static_assert(repeat.selectedStats==std::array<uint8_t,3>{4,1,1});
    static_assert(repeat.distinctInheritedStats==2);
    static_assert(!repeat.matches({0,0,0,0,0,0}));
    // An alternate known-parent history changes only that candidate stage.
    // It cannot be generalized to all possible parents/eggs.
    constexpr auto changedParent=G::replay(
        "emerald_gba",2,initial,high,low,true);
    static_assert(changedParent.childIVs!=repeat.childIVs);
    std::cout << "Gen III retail ordinary egg IV stage: 24/24 pinned synthetic vectors PASS\\n";
}
