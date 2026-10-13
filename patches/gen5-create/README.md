# Coordinator-only PR #300 fixes

These patch artifacts are not applied on the Create branch. They apply to
`5745f200bbc3e442bcee92e0b3945e4eb401c260`; inspect changes if integration advances.
They were applied and exercised only in a separate disposable checkout.

- `0001-baseline-compile.patch`: remove duplicate local declarations (`copySize`,
  `stale`), unused local `Slot` alias, explicitly initialize the appended pending
  certificate fields, and replace two pointer-truth static assertions with
  stronger native value assertions. GCC 16 rejected the pointer assertions in
  debug/ASan/UBSan configurations. No warning suppression is needed after patch.
- `0002-erratic-exp.patch`: restore the integer floor before multiplication for
  Erratic levels 69..98; 20 thresholds were wrong. For example level 70 was
  276915 instead of 276458, level 95 was 549291 instead of 548720, and level 98
  was 584166 instead of 583539. Update the incorrect compile-time expectations.
  This fixes shared level interpretation as well as EXP editing, independently
  of the Create engine's isolated corrected calculation.
- `0003-staged-field-expectations.patch`: update three stale assertions to match
  the current Gen V editable-field descriptor (Species, Ability, Held Item).
  Source-write, shiny and creation restrictions remain asserted unchanged.

Apply in order with `git apply --check` before `git apply`, in the coordinator's
chosen test/integration checkout. None changes accepted legality, MAIN or another
development lane. Applying a patch is not permission to merge or enable Create.
