#pragma once

#include "Legality/Gen4FormEvidence.h"

#include <cstddef>
#include <cstdint>

namespace Legality::Gen4EventTemplate {

struct EventTemplate {
    uint16_t species;
    uint16_t tid;
    uint16_t sid;
    uint32_t pid; // 1 = random anti-shiny; >1 = fixed PID
    uint8_t metLevel;
    uint16_t metLocation;
    uint8_t ball;
    uint8_t form;
    uint8_t language; // 0 = unrestricted
    uint8_t version;
    uint8_t otGender;
    bool fateful;
    uint16_t cardId;
};

#include "Legality/Gen4EventTemplateData.inc"

struct Candidate {
    uint16_t species = 0;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint32_t pid = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    uint8_t form = 0;
    uint8_t language = 0;
    uint8_t version = 0;
    uint8_t otGender = 0;
    bool fateful = false;
};

struct MatchResult {
    bool matched = false;
    uint16_t cardId = 0;
    bool fixedPid = false;
};

constexpr bool shinyForTrainer(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return (static_cast<uint16_t>(pid) ^ static_cast<uint16_t>(pid >> 16) ^ tid ^ sid) < 8u;
}

constexpr bool matches(const EventTemplate& t, const Candidate& c) noexcept {
    if (t.species != c.species || t.tid != c.tid || t.sid != c.sid)
        return false;
    if (t.metLevel != c.metLevel || t.metLocation != c.metLocation ||
        t.ball != c.ball)
        return false;
    if (!Gen4Form::formCompatible(t.species, t.form, c.form))
        return false;
    if (t.language != 0 && t.language != c.language)
        return false;
    if (t.version != c.version || t.otGender != c.otGender || t.fateful != c.fateful)
        return false;

    if (t.pid > 1)
        return c.pid == t.pid;

    // PID=1 wondercards generate a random PID and reject shiny outcomes.
    if (t.pid == 1)
        return c.pid > 1 && !shinyForTrainer(c.pid, t.tid, t.sid);

    return false;
}

constexpr MatchResult matchDirect(const Candidate& c) noexcept {
    for (const auto& t : kGen4EventTemplates) {
        if (matches(t, c))
            return {true, t.cardId, t.pid > 1};
    }
    return {};
}

inline constexpr std::size_t kGen4EventTemplateCount =
    sizeof(kGen4EventTemplates) / sizeof(kGen4EventTemplates[0]);

} // namespace Legality::Gen4EventTemplate
