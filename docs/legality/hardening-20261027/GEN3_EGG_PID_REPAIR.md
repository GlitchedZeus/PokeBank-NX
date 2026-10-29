# GenIII unhatched inheritance provenance and PID

The complete105-file public census found the published `Illegal/Gen3/0032 - NIDORAN♂ - 92A9F5C939C9 - edited issue4356.pk3` accepted clean. The exact original bytes and both results are retained; this is a published edited negative, not an authenticated acquired specimen.

The production native egg-move validator already finds a valid ordered source assignment: initial moves, paternal egg moves, paternal machines, both-parent level-up inheritance and Emerald Volt Tackle. Its caller discarded that assignment and marked every nonempty move `MEggSource`. The existing `verifyPid` therefore could not see `MInheritLevelUp` and did not enforce its Nidoran/Volbeat conditional gender-bit rule.

The repair returns the actual validated assignment and carries `ESParentLevelUp` forward as `MInheritLevelUp`. It adds no new PID restriction. The solver tries all lower-source phases before the level-up phase, preserving an available non-level-up route; initial and paternal-only sources do not gain an invented both-parent condition. Hatched move inference and unrelated encounter paths retain their existing code.

Original evidence: pret/pokeemerald `5eff78649e7170a877b961ef0b3da13b81a16038`, hash-identified in [NATIVE_GEN3_EGG_SOURCE_PINS.json](NATIVE_GEN3_EGG_SOURCE_PINS.json):

- `include/constants/daycare.h:18`: `EGG_GENDER_MALE` is0x8000.
- `src/daycare.c::DetermineEggSpeciesAndParentSlots` first selects the non-Ditto/female species source. NidoranFemale becomes NidoranMale only when this flag is set; Illumise similarly becomes Volbeat. A male parent with Ditto retains the male species without that female-parent bit requirement. The later slot swap makes Ditto the mother for move inheritance.
- `src/daycare.c::BuildEggMoveset` separately adds paternal egg/machine moves, then intersects father/mother move slots and tests the child's level-up list. Ditto's native level-up list contains Transform, not the inherited Nidoran moves. A both-parent level-up source excludes that Ditto explanation.
- `src/data/pokemon/level_up_learnsets.h` and `egg_moves.h` preserve the native source data for independent review. This evidence directly establishes the Emerald mechanism; broader original-version and recursive-ancestry proof remains a distinct obligation.

Permanent controls in `gen3_egg_pid.txt`: the unchanged published negative; a valid inherited case with only bit15 corrected; and initial-move-only male+Ditto-compatible cases with either bit value. Allfour are independently classified by the source-built official26.10.18 oracle. The144-case release suite also asserts exact PID diagnostics and immutable input. Native genus/source equivalence is not historical receipt authentication.
