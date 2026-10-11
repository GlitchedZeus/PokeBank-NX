# Gen III joint ordinary egg witness — parental move history + inherited IVs

**Problem solved (bounded):** independent four-slot egg moves and six-draw IV inheritance witnesses must NOT be joined when they refer to two different hypothetical fathers or mothers. Pinned generated game-specific egg move tables, parental four-slot ordering, Light Ball controls, and the distinct RS/FRLG-versus-Emerald LCRNG inheritance behavior are each retained by delegating to the existing audited stage helpers.

`Gen3JointEggCandidateEvidence` now accepts **one explicit pair** of parent IDs, original four moves, original six-stat IVs, and an independently supplied post-inheritance six-draw RNG state. It requires the same father/mother IDs in both subproofs. Missing egg group/parent species eligibility, one missing parent-IV snapshot, unverified RNG state, or unsupported Gen IV egg-IV algorithm all produce distinct Unknown/Unsupported outcomes; no alternate generation formula is invented.

The conditional checker separates impossible hypothetical move lists, impossible known IV-stage observations and out-of-domain candidate inputs. A result of `KnownCandidateConsistent` means that **one fully supplied scenario** matches, **not** that the historical parents and RNG state truly existed or can be recovered from the stored child. A native PK3 contains none of those full historical parent identities or RNG stages.

Host and sanitizer tests cover same-parent moves/IVs, switched father and mother IDs, partial evidence, source game routing, inherited-IV mismatch, malformed >31 values, and moveset contradiction. Existing 24 forward/reverse synthetic retail vectors remain independent evidence. This interface is read-only and never issues global Invalid or mutates saves.
