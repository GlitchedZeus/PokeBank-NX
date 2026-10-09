#include "Legality/Gen4LeadReportingEvidence.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    using namespace Legality;
    using Gen4LeadHistory::Path;

    Gen4LeadReporting::ReportEvidence none{};
    assert(!none.matched());
    assert(Gen4LeadReporting::describe(none).empty());
    assert(Gen4LeadReporting::describe(uint16_t{0}).empty());
    assert(Gen4LeadReporting::describeWildResult(
               0, "Method K Bug Contest (no lead / Sweet Scent)") ==
           "PID/IV spread and wild slot match Generation IV Method K Bug Contest (no lead / Sweet Scent); uncovered lead and method-specific branches remain incomplete");

    Gen4LeadReporting::ReportEvidence one{};
    one.history.add(Path::StaticSuccess);
    assert(one.matched());
    assert(Gen4LeadReporting::describe(one) ==
           "PID/IV spread and wild source match extended Generation IV Method J/K lead history: Static success; other unproven lead histories remain incomplete");
    assert(Gen4LeadReporting::describe(one.history.mask) ==
           Gen4LeadReporting::describe(one));
    assert(Gen4LeadReporting::describeWildResult(
               one.history.mask, "Method K (extended lead history)") ==
           Gen4LeadReporting::describe(one));

    Gen4LeadReporting::ReportEvidence several{};
    several.history.add(Path::PressureSuccess);
    several.history.add(Path::CuteCharmFailure);
    several.history.add(Path::IntimidateContinue);
    const std::string detail = Gen4LeadReporting::describe(several);
    assert(detail ==
           "PID/IV spread and wild source match extended Generation IV Method J/K lead history: Pressure/Hustle/Vital Spirit success, Cute Charm fail, Intimidate/Keen Eye encounter-continues; other unproven lead histories remain incomplete");
    assert(Gen4LeadReporting::describe(several.history.mask) == detail);

    Gen4LeadReporting::ReportEvidence all{};
    all.history.add(Path::IntimidateContinue);
    all.history.add(Path::StaticMagnetFailure);
    all.history.add(Path::PressureFailure);
    all.history.add(Path::CuteCharmFailure);
    all.history.add(Path::SynchronizeFailure);
    all.history.add(Path::PressureSuccess);
    all.history.add(Path::MagnetPullSuccess);
    all.history.add(Path::StaticSuccess);
    const std::string allDetail = Gen4LeadReporting::describe(all);
    assert(allDetail.find("Static success, Magnet Pull success, Pressure/Hustle/Vital Spirit success") != std::string::npos);
    assert(allDetail.find("Synchronize fail, Cute Charm fail, Pressure/Hustle/Vital Spirit fail") != std::string::npos);
    assert(allDetail.find("Static/Magnet Pull fail, Intimidate/Keen Eye encounter-continues") != std::string::npos);
    assert(Gen4LeadReporting::describe(all.history.mask) == allDetail);
    assert(Gen4LeadReporting::describeWildResult(
               all.history.mask, "ignored generic method") == allDetail);

    Gen4LeadReporting::ReportEvidence mixed{};
    mixed.history.add(Path::SynchronizeMixedSuccessThenFailure);
    assert(mixed.matched());
    assert(Gen4LeadReporting::describe(mixed) ==
           "PID/IV spread and wild source match extended Generation IV Method J/K lead history: Synchronize success then failure (BCC reroll); other unproven lead histories remain incomplete");
    assert(Gen4LeadReporting::describeWildResult(
               mixed.history.mask, "unmatched source") ==
           Gen4LeadReporting::describe(mixed));

    std::cout << "Gen IV lead reporting evidence: PASS\n";
}
