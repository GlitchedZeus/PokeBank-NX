#include "Legality/Gen3ChannelPidIvCorrelation.h"
#include "Legality/Gen3ChannelEventTemplate.h"

#include <cassert>
#include <iostream>

int main() {
    using Legality::Gen3ChannelPidIv::analyze;

    // Canonical PKHeX Pokémon Channel Jirachi vector.
    const auto channel = analyze(
        0x264750D9u,
        {6, 31, 14, 27, 5, 27},
        45819,
        2, // Ruby
        1  // female OT
    );
    assert(channel.matched);

    // The pinned Channel Jirachi distribution has additional persistent fields.
    using Legality::Gen3ChannelEvent::matchesTemplate;
    assert(matchesTemplate(385, 40122, 2, 0, 255, 4, false, false, u"CHANNEL"));
    assert(matchesTemplate(385, 40122, 1, 0, 255, 4, false, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40121, 2, 0, 255, 4, false, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 3, 0, 255, 4, false, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 2, 1, 255, 4, false, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 2, 0, 0, 4, false, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 2, 0, 255, 3, false, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 2, 0, 255, 4, true, false, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 2, 0, 255, 4, false, true, u"CHANNEL"));
    assert(!matchesTemplate(385, 40122, 2, 0, 255, 4, false, false, u"WISHMKR"));

    // Wrong origin version, OT gender, SID, or IVs must break the correlation.
    assert(!analyze(0x264750D9u, {6,31,14,27,5,27}, 45819, 1, 1).matched);
    assert(!analyze(0x264750D9u, {6,31,14,27,5,27}, 45819, 2, 0).matched);
    assert(!analyze(0x264750D9u, {6,31,14,27,5,27}, 45818, 2, 1).matched);
    assert(!analyze(0x264750D9u, {6,31,14,27,5,26}, 45819, 2, 1).matched);

    std::cout << "Gen III Channel Jirachi PID/IV correlation: PASS\n";
}
