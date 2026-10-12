# Gen III joint ordinary egg witness — parental move history + inherited IVs

**Problem solved (bounded):** independent four-slot egg moves and six-draw IV inheritance witnesses must NOT be joined when they refer to two different hypothetical fathers or mothers. Pinned generated game-specific egg move tables, parental four-slot ordering, Light Ball controls, and the distinct RS/FRLG-versus-Emerald LCRNG inheritance behavior are each retained by delegating to the existing audited stage helpers.

`Gen3JointEggCandidateEvidence` now accepts **one explicit pair** of parent IDs, original four moves, original six-stat IVs, and an independently supplied post-inheritance six-draw RNG state. It requires the same father/mother IDs in both subproofs. Missing egg group/parent species eligibility, one missing parent-IV snapshot, unverified RNG state, or unsupported Gen IV egg-IV algorithm all produce distinct Unknown/Unsupported outcomes; no alternate generation formula is invented.

The conditional checker separates impossible hypothetical move lists, impossible known IV-stage observations and out-of-domain candidate inputs. A result of `KnownCandidateConsistent` means that **one fully supplied scenario** matches, **not** that the historical parents and RNG state truly existed or can be recovered from the stored child. A native PK3 contains none of those full historical parent identities or RNG stages.

Host and sanitizer tests cover same-parent moves/IVs, switched father and mother IDs, partial evidence, source game routing, inherited-IV mismatch, malformed >31 values, and moveset contradiction. Existing 24 forward/reverse synthetic retail vectors remain independent evidence. This interface is read-only and never issues global Invalid or mutates saves.

## Parent-pair timestamp / snapshot coupling

Parent IDs alone can accidentally combine a father's moveset from one time with an incompatible earlier/later original IV snapshot. The joint checker now requires an **independently attested nonzero shared parent-stage token** for both evidence inputs. Missing stage linkage is Unknown; mismatched stage tokens mark the asserted joint witness incoherent. Tokens are hypothetical supplied provenance, **not** stored native PK3 fields or newly inferred historical proof.

## BREED-002 physical daycare deposit order (2026-10-11)

The joint helper now requires an independently verified mapping of daycare **slot 0** and **slot 1** to the already verified parent IDs, rather than silently treating the move-donor father as physical slot 0. Unknown or missing deposit order returns `UnknownDaycareSlotOrder`; duplicated, alien or contradictory IDs return `IncoherentDaycareSlotOrder` for that proposed history. Source-attested reversed deposit order is valid and preserves the original father/mother move inheritance roles.

The regression checks 24 source-cross-checked Gen III six-draw IV stage vectors (Emerald, Ruby, FireRed) with both possible role placements, plus explicit missing, duplicate and alien slot mappings: 48 positive conditional joint histories, 48 each of four negative/unknown controls. This is **not** proof that real parent identities or a final RNG state can be reconstructed from a detached PK3, and no new global Invalid decision is connected to this helper.
