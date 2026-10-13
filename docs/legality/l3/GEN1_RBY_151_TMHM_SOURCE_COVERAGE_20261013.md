# Original Gen I TM/HM completeness — source evidence, not trainer acquisition
Source pins: `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`,
`pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`.

This exact original-source implementation covers **all 151 species and all 50
Gen I TMs plus five HMs**, preserving separate Red/Blue versus Yellow. The
ordered original `data/pokemon/base_stats.asm` and each `base_stats/*.asm`
were read by exact Git blob identity. Original `constants/item_constants.asm`
and `constants/move_constants.asm` establish machine order and native move
IDs; no modern learnset union or unverified distribution was used.

- Red/Blue: **3,037** species-machine compatibility pairs.
- Yellow: **3,043** compatibility pairs.
- **128** original species base-stat files are byte-identical across
  original revisions; **23** Yellow files differ, of which **8** have
  TM/HM-compatibility differences: Charizard, Butterfree, Venonat,
  Venomoth, Diglett, Dugtrio, Kabutops and Mewtwo.
- See `docs/legality/research/l3/GEN1_RBY_MACHINE_SOURCE_BLOBS.json` for
  exact 151+151 source Git blobs; the Python verifier requires those
  original commits and independently reconstructs every packed machine.
- `Gen1OriginalMachineTables.h` is integrated by existing
  `Gen1RbyOriginalJointHistory::sourceMachine` and therefore the
  all-151-species joint chronology solver; this is **source compatibility
  only**. An unattested version, TM receipt, prior acquisition/evolution,
  trainer overwrite choice, event-special move or Time Capsule crossing
  remains Unknown/Incomplete.
- `tests/test_legality_l3_gen1_original_all_machines.cpp` tests all
  151×55×3 original titles, boundary/Unknown statuses and source-known
  version differences. Normal+ASan/UBSan must pass at the exact
  coordinator head; original .NET PKHeX oracle parity remains blocked
  until actually executed.

Only coordinator draft PR #298 is modified. Original saves untouched,
no product/main/accepted merge or global-Pokémon Invalid claim.