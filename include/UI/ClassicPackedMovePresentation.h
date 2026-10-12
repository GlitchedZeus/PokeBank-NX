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
} // namespace ClassicPackedMove
} // namespace UI

#endif
