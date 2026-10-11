# Gen III/IV ordinary egg: simultaneous parent-history candidate

The earlier ordinary-egg builder modeled base, egg, machine, and shared-level passes but accepted separate hypothetical move vectors for each pass. That could inadvertently union moves held by different fathers.

This bounded companion accepts **one identified hypothetical father and mother**, each with exactly four stored moves at the alleged egg creation. The child's direct egg moves come from the pinned exact-game egg-move data. Verified machine flags are only supplied by a caller with independent method evidence. Shared-level moves require a verified method AND presence on the same proposed mother; all three passes use the same father's original four slots. FIFO, duplicate suppression and Emerald/Gen IV Light Ball are delegated to the accepted constructor.

The test is a **synthetic conditional pair**, not a real PK3/PK4 parent source. Missing parents remain Unknown; a contradictory pair excludes only that pair, **never globally Invalid**. Full egg-group, parent eligibility, trade, event, chain breeding and parent-moveset reconstruction remain unresolved. No source save changes or automated fixes.

## Breeding-pair provenance gate

Nonzero invented parent IDs and four-slot move snapshots **do not prove** ordinary daycare eligibility. The model now requires both parent species to be valid for the stated game (Gen III up to #386, Gen IV up to #493) and a separate, independently verified parent-pair compatibility result. Missing egg-group, gender, Ditto, incense, mother-species, or other breeding evidence returns `UnknownPairEligibility` rather than a compatible route. An explicitly verified incompatible pair contradicts only the single hypothetical parent-pair candidate, not every possible source of the stored Pokémon.

New regression cases exercise unknown versus compatible versus explicitly incompatible source pairs, missing parent species, generation-specific species bounds, and unchanged Emerald Pichu / Light Ball uncertainty. The helper does not invent a comprehensive egg-group database or imply actual parent histories survive in native Pokémon records.
