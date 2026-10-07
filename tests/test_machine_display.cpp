#include "Names/MachineDisplay.h"
#include "Names/MoveNames.h"
#include "Names/TMMoves.h"

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

    std::cout << "machine display tests passed\n";
    return 0;
}
