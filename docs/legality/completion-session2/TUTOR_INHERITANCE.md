# Crystal tutor moves require the same compatible donor

Further native-source review found a false-Invalid defect in the new ancestry adapter: it omitted Crystal's three tutor entries from the inheritance mask and queried egg's admission check. The existing egg-order verifier already recognized this source. This was an implementation defect, not a deliberate stricter interpretation.

The [Crystal TMHMMoves table](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/data/moves/tmhm_moves.asm) appends Flamethrower, Thunderbolt and Ice Beam before the zero sentinel. [GetEggMove](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/pokemon/breeding.asm) scans that complete table and checks the child's compatibility bit. The donor may therefore pass a tutor move together with its other inherited moves. [Gold/Silver's table](https://github.com/pret/pokegold/blob/ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8/data/moves/tmhm_moves.asm) terminates before those entries. The [selection menu](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/items/tmhm.asm) rejects Eggs; this is inheritance from a donor, not teaching the unhatched egg directly.

Production now includes tutor compatibility under Crystal's existing joint-donor proof. No queried move is dropped. Korean-only source restrictions remain. No failed proof is automatically converted to clean or Incomplete.

| Exact fixture ID / egg | Joint inherited pair | Native construction | Before | Both raw oracles | After |
|---|---|---|---|---|---|
| 800001 Squirtle | Mirror Coat243 + Ice Beam58 | Corsola learns Mirror Coat at37 and receives the compatible Crystal tutor; shared Water1 | Invalid | Valid | Clean |
| 800002 Ponyta | Hypnosis95 + Flamethrower53 | Compatible Field donor can carry both; recorded witness includes native Smeargle/Vulpix ancestry | Invalid | Valid | Clean |
| 800003 Pichu | Encore227 + Thunderbolt85 | Fairy-compatible Clefairy learns Encore and receives the tutor; a later rebred/evolved Pikachu is also a donor | Invalid | Valid | Clean |

The exact synthetic native-format bytes are in `crystal_parent_tutors.txt`. The production test uses Pokemon2ReadOnly and Legality::analyze. Both PKHeX revisions accept all three, and the separate pret-only checker constructs all three ancestries. These are constructed regression examples, not authenticated historical specimens.

Direct proof tests cover all three tutor types, exclusion when constrained to GS birth or native Korean-only sources, and the negative Smoochum Meditate + Ice Beam combination where no compatible donor exists. Generic entity analysis still considers alternate birth histories. Accepting a tutor never bypasses the donor requirement. The original12/38 cases and stricter Smoochum Blizzard controls are unchanged.

All40 focused configurations were rebuilt and rerun: ten suites × GCC/Clang × normal/ASan+UBSan, preserving the original24. Leak detection and halt-on-error were enabled. `TUTOR_REPLAY.json` records150000 existing GB evaluations (retained, final fresh and the original current-reference probe): **every output byte is identical** after the correction. Frozen campaign metrics and denominators are unchanged; these new positive cases expose a gap in the earlier generated sets.

An additional holdout was declared before generation: Gen I seed8176001 with10000 records and Gen II seed8176003 with20000. Both raw oracles and production evaluated every input. Source adjudication found zero unexplained outcomes. `TUTOR_HOLDOUT.json` retains every raw disagreement count and file hash. No failed seed was renamed or discarded. These additional synthetic/correlated evaluations remain separate from frozen campaign metrics; the three focused regressions are not presented as independent holdouts.

The preceding95a3cfcee4b7783ff874c09a2d81f7f6a0992e90 checkpoint passed49/49 checks. Its CI results must not substitute for the final correction's exact-head CI. Independent reviewer/hardware acceptance remains unperformed, and semantic/historical/deployed coverage remains unmeasured.
