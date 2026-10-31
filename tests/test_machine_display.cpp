#include "Names/MachineDisplay.h"
#include "Names/MoveNames.h"
#include "Names/TMMoves.h"
#include "UI/ClassicMachineArtwork.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    using Enums::GameVersion;
    using Names::MachineKind;

    // Gen II exact PKHeX PersonalInfo2 mapping: TM01 Dynamic Punch, HM03 Surf.
    assert(Names::getTMMove(GameVersion::GSC, 191) == 223);
    assert(Names::getTMMove(GameVersion::GSC, 245) == 57);
    assert(Names::getTMMove(GameVersion::GSC, 195) == 0); // unused gap must not alias a machine

    auto tm01gsc = Names::getMachineDescriptor(GameVersion::GSC, 191);
    assert(tm01gsc && tm01gsc.kind == MachineKind::TM && tm01gsc.number == 1 && tm01gsc.moveId == 223);
    auto hm03gsc = Names::getMachineDescriptor(GameVersion::GSC, 245);
    assert(hm03gsc && hm03gsc.kind == MachineKind::HM && hm03gsc.number == 3 && hm03gsc.moveId == 57);
    assert(Names::machineIdentifier(tm01gsc) == "TM01");
    assert(Names::machineIdentifier(hm03gsc) == "HM03");

    // Same machine number, different generation/game mapping.
    assert(Names::getTMMove(GameVersion::FRLG, 289) == 264);
    assert(Names::getTMMove(GameVersion::FRLG, 289) != Names::getTMMove(GameVersion::GSC, 191));
    auto frLeaf = Names::getMachineDescriptor(GameVersion::FR, 289);
    assert(frLeaf && frLeaf.kind == MachineKind::TM && frLeaf.number == 1 && frLeaf.moveId == 264);
    auto swLeaf = Names::getMachineDescriptor(GameVersion::SW, 1145);
    assert(swLeaf && swLeaf.kind == MachineKind::TR && swLeaf.number == 15 && swLeaf.moveId == 126);

    // Generation IV keeps TM01-TM92 at items 328-419. HM05 differs:
    // Defog in D/P/Pt, Whirlpool in HG/SS.
    assert(Names::getTMMove(GameVersion::DP, 328) == 264);
    assert(Names::getTMMove(GameVersion::PT, 419) == 433);
    assert(Names::getTMMove(GameVersion::DP, 424) == 432);
    assert(Names::getTMMove(GameVersion::HGSS, 424) == 250);
    auto tm01g4 = Names::getMachineDescriptor(GameVersion::PT, 328);
    assert(tm01g4 && tm01g4.kind == MachineKind::TM && tm01g4.number == 1 && tm01g4.moveId == 264);
    auto hm05dp = Names::getMachineDescriptor(GameVersion::DP, 424);
    assert(hm05dp && hm05dp.kind == MachineKind::HM && hm05dp.number == 5 && hm05dp.moveId == 432);
    const std::string tm53g4 = Names::machineDisplayLabel(GameVersion::PT, 380, "TM53");
    assert(tm53g4.find("TM53") != std::string::npos);
    assert(tm53g4.find(Names::getMoveName(412)) != std::string::npos);

    // Existing reliable project mappings remain available through the global descriptor layer.
    auto tr15 = Names::getMachineDescriptor(GameVersion::SWSH, 1145);
    assert(tr15 && tr15.kind == MachineKind::TR && tr15.number == 15 && tr15.moveId == 126);
    assert(Names::getMachineDescriptor(GameVersion::PLA, 328).kind == MachineKind::None);

    const uint16_t stored = 191;
    const std::string display = Names::machineDisplayLabel(GameVersion::GSC, stored, "TM01");
    assert(display.find("TM01") != std::string::npos);
    assert(display.find(Names::getMoveName(223)) != std::string::npos);
    assert(stored == 191); // presentation must never rewrite the save-format item id

    assert(Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "TM01"));
    assert(Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "1"));
    assert(Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "dynamic"));
    assert(!Names::machineDisplayMatches(GameVersion::GSC, 191, "TM01", "surf"));

    // Image art is selected by the move the native machine TEACHES,
    // not its visible TM number. This covers the screenshot regressions.
    using UI::ClassicMachineArtwork::artKey;
    assert(artKey(GameVersion::RBY, 201) == "tm-normal"); // TM01 Mega Punch
    assert(artKey(GameVersion::RBY, 206) == "tm-poison"); // TM06 Toxic
    assert(artKey(GameVersion::RBY, 198) == "hm-water"); // HM03 Surf
    assert(artKey(GameVersion::GSC, 191) == "tm-fighting"); // TM01 Dynamic Punch
    assert(artKey(GameVersion::GSC, 193) == "tm-unknown"); // TM03 Curse = ??? type
    assert(artKey(GameVersion::GSC, 245) == "hm-water"); // HM03 Surf
    assert(artKey(GameVersion::FRLG, 289) == "tm-fighting"); // TM01 Focus Punch
    assert(artKey(GameVersion::PT, 328) == "tm-fighting");
    assert(artKey(GameVersion::DP, 424) == "hm-flying"); // HM05 Defog
    assert(artKey(GameVersion::HGSS, 424) == "hm-water"); // HM05 Whirlpool
    assert(artKey(GameVersion::GSC, 195).empty()); // gap must stay untyped
    assert(artKey(GameVersion::RBY, 1).empty()); // not a machine
    static_assert(UI::ClassicMachineArtwork::Gen1Types[2] == 1);
    static_assert(UI::ClassicMachineArtwork::Gen2Types[2] == 2);

    // Complete native machine matrix, not only a few iconic TMs. No valid
    // Gen I-IV machine may regress to an empty type icon or a later-game move.
    for(uint16_t id=196;id<=250;++id)
        assert(!artKey(GameVersion::RBY,id).empty()); // 5 HMs + 50 TMs
    for(uint16_t id:UI::ClassicMachineArtwork::Gen2MachineItemIDs) {
        const auto machine=Names::getMachineDescriptor(GameVersion::GSC,id);
        assert(machine && machine.moveId>0 && machine.moveId<=251);
        assert(!artKey(GameVersion::GSC,id).empty()); // Curse uses ??? icon
        const auto label=Names::machineDisplayLabel(GameVersion::GSC,id,"");
        assert(label.find(" — ")!=std::string::npos);
    }
    for(uint16_t id=289;id<=346;++id) {
        const auto machine=Names::getMachineDescriptor(GameVersion::FRLG,id);
        assert(machine && machine.moveId>0 && machine.moveId<=354);
        assert(!artKey(GameVersion::FRLG,id).empty());
        assert(Names::machineDisplayLabel(GameVersion::FRLG,id,"").find(" — ")!=std::string::npos);
    }
    for(const auto game:{GameVersion::DP,GameVersion::PT,GameVersion::HGSS}) {
        for(uint16_t id=328;id<=427;++id) {
            const auto machine=Names::getMachineDescriptor(game,id);
            assert(machine && machine.moveId>0 && machine.moveId<=467);
            assert(machine.number==(id<=419 ? id-327 : id-419));
            assert(!artKey(game,id).empty());
            assert(Names::machineDisplayLabel(game,id,"").find(" — ")!=std::string::npos);
        }
    }

    std::cout << "machine display tests passed\n";
    return 0;
}
