# PKHeX verified improvements, Generations I-V

Each entry is classified as **Verified improvement** (a primary source, a witness record, a
reproduction, PKHeX's verdict and ours), **Verified parity** (PKHeX's behaviour reproduced and
confirmed against its own test records or oracle), **Unresolved difference** or **Unsupported
claim**. Rates are measured on the differential corpora and are not completeness figures.
PKHeX.Core is `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

## Gen III

### Colosseum/XD origins judged in the Gen III analyzer (Verified parity)

- **Before.** A PK3 with version 15 got the Warning "Colosseum/XD origin; Gen III cartridge
  rules cannot verify this Pokemon" and the older GameCube evidence in `Legality.cpp`; it
  could never be certified and a relabelled GBA record was never Invalid.
- **Now.** `src/Legality/Gen3Legality.cpp` runs PKHeX's `EncounterGenerator3GC` path (shadow,
  starter, gift, trade, Poke Spot and e-Reader templates, team locks, name-screen trainer IDs,
  CXDVerifier, fateful, National/Earth Ribbon, ball, ability rules; file:line in
  `GEN3_PKHEX_RULE_INVENTORY.md` section 7) on a version-15 PK3 held in any GBA save.
- **Witnesses.** PKHeX's own test records converted to PK3 (hex in
  `tests/test_legality_gen3.cpp`, rows `pkhex-*`): Vaporeon split evolution VALID / certified;
  Shuckle `.xk3` without fateful VALID / certified; Italian HO-OH long OT VALID / certified;
  Shuckle `.ck3` without fateful INVALID / Invalid; Ultra Ball Teddiursa INVALID / Invalid;
  Tyranitar missing Winning Ribbon Fishy / Incomplete, with it INVALID / Invalid. Oracle-built
  records: 3914 (seed 1), 3928 (seed 2), 3908 (seed 3) GameCube records in five cartridges,
  0 false Invalid, 0 false Valid, 100% of PKHeX-invalid caught, 97.2-97.7% of PKHeX-valid
  certified (the rest Fishy in PKHeX too).

### Team lock search terminates (Verified parity with a guard; see PKHEX_DISAGREEMENT_REVIEW.md 15-A, 16-A)

- `TeamLockResult.GetSingleLock` has a branch that never advances (`TeamLockResult.cs:118-123`);
  the port advances, and every GameCube RNG search is budgeted (`CxdGuard`), reporting Unknown
  at the cap instead of hanging. No corpus record reached a cap. This is not a stricter rule:
  a cap demotes Invalid findings to Warnings. Classified parity because PKHeX never enters the
  branch on the published templates; the improvement is robustness, not a new verdict.

### Oracle coverage: GameCube records in every GBA cartridge (Verified parity)

- `tools/pkhex_gen34_diff/oracle/Program.cs` builds each Colosseum/XD template as CK3/XK3 ->
  PK3 with a name-screen-valid TID/SID (`CxdTrainer`) once per holding cartridge, plus
  mutations and growth. Before, the Gen III corpora had no version-15 record at all, so the
  GameCube path had no oracle measurement.

### Unresolved differences (none promoted)

- e-Reader shadows (Japanese bonus disc) carry fixed all-zero IVs; PKHeX grades them Fishy
  (IVAllEqual_0) and so do we (Incomplete). A primary source for the fixed IVs (the disc's
  data) would allow certification; none is available in this environment.
- English XD TID/SID without a name-screen seed: PKHeX Fishy (PAL copies skip the ball
  sequence, `TrainerIDVerifier.cs:86-101`); kept as a Warning. No decompilation exists to
  decide the PAL behaviour independently.
