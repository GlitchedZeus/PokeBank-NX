# Tranche 3 — verified Gen IV egg-IV inheritance stages

The accepted engine remains `fe3e3d054ef7d42c31df3e772e8370e2c7587c8a`. The fetched active hardening draft advanced to `33d74ee0`; it adds transfer/RNG work but does not modify the joint egg helper or add ordinary Gen IV inheritance replay. The [refresh snapshot](evidence/engine-refresh-3.json) records the exact refs. The donor-slot defect from Tranche 2 therefore remains present at these inspected revisions; no newer fix is assumed absent beyond this freeze.

## New evidence ready for implementation

[Packet RNG-003](packets/RNG-003.md) supplies the exact Platinum/HGSS stage algorithms, Power item IDs, fixed-seed examples, variable-length reversal, integration points and Unknown boundaries. The independently authored script hosts unchanged original retail functions temporarily and compares them against a separate Python model.

- **25,382 forward comparisons passed:** child IVs, final RNG state and draw count.
- **25,382 known-branch inverse checks passed.** These rewind a known stage endpoint; they do not recover unique seeds from child IVs.
- **138 detailed fixtures** cover all 49 item pairs at seed 0 plus selected branch controls at four other seeds. The full generated matrix is reproducible and has deterministic result digests.
- Both parent IV arrays and initial child IVs vary with the seed. Fixtures are synthetic stage inputs, not legitimate full Pokémon claims.

The practical implementation differences are substantial:

| Stage | RNG calls | Stat selection |
|---|---:|---|
| HGSS, no Power item | 6 | Three distinct stats |
| HGSS, one Power item | 4 | Holder's stat plus two other distinct stats |
| HGSS, two Power items | 5 | Choose one holder first, then two other stats |
| Platinum, all tested item pairs | 6 | Indexed-removal behavior can repeat stats |

For HGSS with two items, odd first output selects slot0, even selects slot1. Even two identical Power items consume this selection draw. Physical daycare slots are not move-parent roles. Reusing the Gen III helper's unconditional six-draw rewind is incorrect for HGSS's one/two-Power-item branches.

## Updated immediate queue

1. Fix the confirmed Gen III joint donor-slot binding from [BREED-002](packets/BREED-002.md).
2. Implement ordinary Gen IV IV-stage replay from [RNG-003](packets/RNG-003.md), keeping Platinum and HGSS separate.
3. Add a known-branch inverse only after held-item and slot availability are explicit; unknown historical items imply unknown branch length.
4. Use [BREED-003](packets/BREED-003.md) to derive parent eligibility/roles from independently verified personal data.
5. Connect joint move/IV histories only after source snapshots and daycare slots refer to the same parents and same stage.

These tasks have concrete reproducible algorithm evidence. DP retail corroboration, pending PID/Everstone/international stages, source personal/form adapters and real paired native histories remain open. No new global Invalid rule is justified by these stage comparisons.

## Delivery and reproduction

Run `python docs/legality/research/scripts/reproduce_gen4_egg_ivs.py SOURCE_ROOT`, then the source/document validator. See [the fixture data](evidence/gen4-egg-iv-vectors.json). Source hashes and item constants are checked; original functions and compiler products exist only in temporary directories.

The research set now contains **13 packets, 79 source entries and 145 catalog rows**. Two new catalog rows index the aggregate Gen IV test suites; the 138 detailed vectors reside in their own JSON artifact. Historical tranche counts remain labeled by their checkpoint. Production code, main, the accepted branch and #103 are unchanged by this research.
