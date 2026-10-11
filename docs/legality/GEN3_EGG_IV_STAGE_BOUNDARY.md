# Gen III retail ordinary-egg IV inheritance — bounded stage

This production evidence foundation implements only the **six LCRNG draws** starting from a known `InheritIVs` entry state. It requires all **six baseline child IVs** and the **two six-stat parent IV arrays** to be explicitly supplied, and distinguishes known from unknown parental history.

The Ruby/Sapphire and FireRed/LeafGreen stat-removal behavior differs from Emerald. The retail behaviors can repeat selected stats. The forward model deliberately preserves the stale compacted tail and avoids an assumed universal three-distinct-inherited-stats rule.

## Test provenance and limits

- Source research: Codex research-only #252, packet `RNG-001`, `evidence/inheritance-vectors.json`. It records **24 executed, independently corroborated synthetic stage vectors** for Emerald/Ruby/FireRed (8 seeds each).
- New fixture: `tests/test_gen3_egg_iv_inheritance_stage.cpp` checks **all 24** selected-stat sequences, donor bits, six RNG draws, resulting IVs and final RNG seed. It separately tests Emerald repeated inheritance, source family boundaries and unknown parental history.
- **Not a native PK3 egg specimen or a complete egg PID/IV reconstruction**. Original parental IVs, pending egg PID, Everstone, Masuda and source-save provenance are absent from an ordinary stored child. The analyzer MUST NOT translate a missing or contradicted single-stage witness into global `Invalid`.
- No PKHeX GPL code, retail decompilation body or game assets have been imported. This independently written bounded algorithm is based on behavior recorded in the cited research packet; license/source provenance remains separate.

Further work needs parent-candidate enumeration, exact original-game encounters, persisted/known parental identities or explicit existential alternatives, game-specific PID generation stages, and native fixtures.
