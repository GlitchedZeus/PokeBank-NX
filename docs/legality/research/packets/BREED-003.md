# BREED-003 — Compute parent eligibility and move-donor roles explicitly

Research ID: **BREED-003**
Games: Emerald, Platinum, HGSS directly executed for eligibility; Emerald directly executed for role selection. Ruby/FireRed and DP generalization need their own source-stage validation.
Engine revision: `fe3e3d054ef7d42c31df3e772e8370e2c7587c8a`.
Classification: **Dataset/adapter gap beyond an implemented conditional API**.
Confidence: **A for the tested control flow; B for inspected Gen IV role correspondence**. Complexity: **Medium**, plus personal-data provenance work.

## Current implementation and compact handoff

`Gen34SameParentEggHistory::evaluate` checks game/species domains and requires `PairProof::IndependentlyCompatible`. It does not derive that proof from species egg groups, genders and trainer IDs. It then uses a caller-supplied father/mother role assignment. Implementing the eligibility adapter below replaces an unexplained boolean with a reproducible source-backed decision. Do not duplicate the existing same-parent move construction or claim this reconstructs absent parents.

Inputs: exact game, two externally known parent species/forms and correctly derived genders, each parent's two personal-data egg groups, full 32-bit OT ID, deposit slot indices, and verified parent snapshot identities. Source references and hashes: `E-PARENT-ROLES`, `PT-PARENT-ELIGIBILITY`, `HGSS-PARENT-ELIGIBILITY` in the registry.

## Retail eligibility decision table

Apply in this order. The retail code tests the first egg-group field for Undiscovered and Ditto; preserve the source personal-data invariant rather than inventing mixed terminal groups.

| Condition | Score | Interpretation |
|---|---:|---|
| Either parent's first group is Undiscovered | 0 | This proposed pair cannot breed |
| Both first groups are Ditto | 0 | This proposed pair cannot breed |
| Exactly one is Ditto, full OT IDs equal | 20 | Eligible, lower compatibility |
| Exactly one is Ditto, full OT IDs differ | 50 | Eligible, medium compatibility |
| No Ditto; same gender or either genderless | 0 | Ineligible |
| No Ditto; neither of the two groups overlaps | 0 | Ineligible |
| Eligible ordinary pair; same species, different OT IDs | 70 | Higher compatibility |
| Eligible ordinary pair; same species, same OT IDs | 50 | Medium compatibility |
| Eligible ordinary pair; different species, different OT IDs | 50 | Medium compatibility |
| Eligible ordinary pair; different species, same OT IDs | 20 | Lower compatibility |

These are returned compatibility scores. This harness does not execute step counters, RNG egg-generation thresholds or claim a directly measured encounter probability. OT names are not compared by these functions. “Different trainers” must not be approximated by TID alone: the accessor returns the full OT ID field.

## Move-donor role assignment is separate

First resolve the offspring lineage from the female parent, or the non-Ditto parent when Ditto is present. Retail then selects inheritance roles:

| Pair shape | Move “father” role | Move “mother” role |
|---|---|---|
| Ordinary female/male | Male | Female |
| Female with Ditto | Ditto | Female |
| Male with Ditto | Male | Ditto |
| Genderless with Ditto | Non-Ditto | Ditto |

This assignment is invariant to swapping the two deposited slots, but the numerical slot indices reverse. For a female paired with Ditto, do not promote the female's moves to father-only egg/TM inheritance merely because she is non-Ditto. Shared-level inheritance still depends on both actual movesets. The genderless role assignment does not guarantee that any special inherited move exists.

Emerald checks bit `0x8000` of the pending offspring personality for Nidoran-F→Nidoran-M and Illumise→Volbeat after base-lineage resolution. Platinum and HGSS additionally map Manaphy to Phione, then perform the same Ditto role swap. Incense changes and complete personal/form lineage data are separate stages; this harness deliberately does not replace them with an invented species database.

Physical IV donor slots must remain separate from these move roles: see [BREED-002](BREED-002.md).

## Independent reproduction and fixtures

`python docs/legality/research/scripts/reproduce_parent_eligibility.py SOURCE_ROOT` extracts unchanged retail functions into a temporary shared library, supplies explicit field-access/gender/personal-data stubs, and checks an independently written Python decision model.

- **17,496/17,496 eligibility comparisons passed:** 5,832 category combinations each in Emerald, Platinum and HGSS.
- **24/24 Emerald role cases passed:** six pair shapes × two deposit orders × two offspring-bit settings.
- The output stores source hashes, result counts, the ordered eligibility-case digest and individual role cases in [parent-eligibility-vectors.json](../evidence/parent-eligibility-vectors.json).

The categories systematically cover group overlap, second-group overlap, Undiscovered, Ditto, three gender classes, species equality and full OT-ID equality. They are **abstract algorithm inputs**, not 17,496 proven possible species/PID combinations. Role labels are harness-local numbers, never serialized National Dex or native species IDs. Species lineage lookup and PID→gender calculation are stubbed and therefore not independently verified by this harness.

Positive fixtures: ordinary opposite-gender overlapping groups; Ditto with a breedable genderless parent; swapped deposit order preserving role identity; a secondary-group-only overlap. Negative fixtures: two Ditto, Undiscovered+Ditto, same-sex non-Ditto, genderless non-Ditto and disjoint groups. Unknown fixtures: missing parent species/form, unverified group resource, missing actual gender or absent parent records. Missing OT IDs can leave the rate tier unknown while still proving pair eligibility from the other required fields.

## Integration sequence and acceptance

1. Add a source-versioned parent personal-data adapter and field-availability flags; pin its license/provenance and validate species/form lookup separately.
2. Implement pure eligibility with `Unknown / Incompatible / Compatible` plus optional score; exact game is mandatory.
3. Derive move-parent role IDs and separate physical daycare-slot IDs from the same source snapshot.
4. Feed a proven compatible result into the existing `PairProof` API; preserve unknowns. Feed slot IDs into the corrected joint-IV bridge.
5. Port category tests plus concrete native parent-reader fixtures. Add Ditto, genderless, baby/Undiscovered, second-group overlap and full OT-ID controls.

Acceptance requires agreement with all three tested retail functions, no caller-assumed father-slot ordering, and proof that the selected source/form personal data is correct. Pair incompatibility excludes the supplied pair only; the child can still have other unknown parents or an event origin. No global Invalid is warranted from missing history.

## Lost information and remaining research

PK3/PK4 do not preserve these two parent snapshots, daycare order, group fields or full inheritance history. Complete chain breeding still requires verifying each parent's own four-move history; this eligibility adapter is necessary but not sufficient. DP-specific retail execution, complete form/personal-data reconciliation, incense and lineage exceptions, and native save-derived parent fixtures remain explicit follow-ups.

## License/reuse

The algorithm specification and Python model are independently authored. Retail function bodies are hosted temporarily and not committed; no ROM or personal save data is needed. Any imported personal tables or copied implementation require provenance/license review before production integration.
