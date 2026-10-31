/**
 * ModernEncounter.h - the encounter ball rule shared by the Gen VI-IX analyzers.
 *
 * BallVerifier.VerifyBall, made encounter-independent: for each format and national species,
 * the union of the balls every encounter EncounterMovesetGenerator yields for any form of the
 * species (wild ball sets per game, fixed balls, egg inheritance, Dream World, GO restrictions,
 * plus the Shedinja and Legends: Arceus -> Sword/Shield replacements). A ball outside that union
 * has no encounter at all. The same pass records whether any encounter takes the fateful flag
 * and whether any takes it clear. Generated from the pinned PKHeX.Core by
 * tools/pkhex_gen67_diff/oracle ("dumpencounters") into ModernEncounterData.inc.
 */
#pragma once

#include <cstdint>

#include "Legality/ModernLegality.h"

namespace Legality::ModernEncounter {

/// False only when no encounter of the species in this format can carry the ball. A species
/// without any generated encounter, or outside the table, is never decided here.
bool ballPossible(Modern::Format format, uint16_t species, uint8_t ball) noexcept;

/// False only when no encounter of the species in this format accepts the fateful flag in this
/// state (set, or clear). Undecided cases as for ballPossible.
bool fatefulPossible(Modern::Format format, uint16_t species, bool fateful) noexcept;

} // namespace Legality::ModernEncounter
