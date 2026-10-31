/**
 * ModernBall.h - the encounter ball rule shared by the Gen VI-IX analyzers.
 *
 * BallVerifier.VerifyBall, made encounter-independent: for each format and national species,
 * the union of the balls every encounter EncounterMovesetGenerator yields for any form of the
 * species (wild ball sets per game, fixed balls, egg inheritance, Dream World, GO restrictions,
 * plus the Shedinja and Legends: Arceus -> Sword/Shield replacements). A ball outside that union
 * has no encounter at all. Generated from the pinned PKHeX.Core by
 * tools/pkhex_gen67_diff/oracle ("dumpballs") into ModernBallData.inc.
 */
#pragma once

#include <cstdint>

#include "Legality/ModernLegality.h"

namespace Legality::ModernBall {

/// False only when no encounter of the species in this format can carry the ball. A species
/// without any generated encounter, or outside the table, is never decided here.
bool possible(Modern::Format format, uint16_t species, uint8_t ball) noexcept;

} // namespace Legality::ModernBall
