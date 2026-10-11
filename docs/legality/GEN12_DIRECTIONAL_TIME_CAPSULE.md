# MOVE-002 — directional Time Capsule candidate history

The existing Gen I/II Time Capsule evidence separates **present-day PK2 eligibility** from a **possible historical** Gen I visit. The current stored Pokémon cannot prove an actual GB-era transfer, tradeback, stadium visit, or complete retention/relearning timeline.

The four-slot candidate-history solver now accepts an **independently verified hypothetical Gen II -> Gen I -> Gen II route**, with explicit entry/return stages, species at Gen I entry (National #1–151) and a separately verified language/retail policy. Unknown Korean/Stadium/console policy or missing transfer stages returns **Unresolved**.

A Gen II-only move ID >165 cannot be offered as a witness acquired before or during the Gen I visit, nor can the same individual re-inherit its own egg-only move after returning. Its later presence needs an independently verified, post-return acquisition witness (such as source-specific machine/level/tutor). A Gen I-era move ID <=165 may be retained subject to its normal independently verified method. Options with both invalid previsit retention and valid later acquisition still yield a compatible hypothetical candidate.

Tests exercise previsit egg-only contradiction, too-early teaching, post-return move, retained Gen I-era move, impossible second egg inheritance, unknown source/policy, unknown entry species, incorrect direction and mixed alternatives. These are **synthetic time-indexed candidate scenarios**, not proven species-specific native PK2 legalities or global Invalid decisions. The original Pokémon and save data remain immutable.

## One-time original encounter methods and forbidden fabricated returns

The model now distinguishes ordinary verified teaching *after* a confirmed return from Gen I from attempts to treat the same individual as an entirely **new original event gift or second egg**. Neither original move source can happen after a known Time Capsule entry, even if the move ID is ≤165. A Gen II-only move >165 cannot cross Gen I by pretending it was `TransferRetained` at a later stage. An independently verified post-return level/tutor/machine acquisition is still permitted. The new policy affects only an asserted fully specified hypothetical source history, never source-free native PK2 verdicts.
