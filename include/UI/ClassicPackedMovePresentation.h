#ifndef UI_CLASSIC_PACKED_MOVE_PRESENTATION_H
#define UI_CLASSIC_PACKED_MOVE_PRESENTATION_H

// Read-only presentation hint for the classic shared Y-carry overlay.
// Gen IV deliberately defers edits to PK4 until a validated Y drop. The
// source slot still exists in the staged save during carry, but the box grid
// and summary must present it as empty, like Gen I-III's picked-up source.
// This NEVER deletes, relocates or writes the source record.
namespace UI {
class TrainerViewScreen;
namespace ClassicPackedMove {
bool hidesGen4CarriedSource(const TrainerViewScreen& screen, int box, int slot);
// R2-CHECK-009: Gen I Box Actions -> Move. Starts the SAME Boxes-view carry as a Y tap, with this
// Pokemon picked up (Y drops, B cancels). False when no carry could start.
bool beginGen1FromAction(TrainerViewScreen& screen, int box, int slot);
// Round 11 ACTIONS: the same for Gen II (packed box) and Gen III (sparse box) Box Actions -> Move.
bool beginGen2FromAction(TrainerViewScreen& screen, int box, int slot);
bool beginGen3FromAction(TrainerViewScreen& screen, int box, int slot);
} // namespace ClassicPackedMove
} // namespace UI

#endif
