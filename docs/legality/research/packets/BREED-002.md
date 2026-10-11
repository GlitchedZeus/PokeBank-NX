# BREED-002 — Bind IV donors to daycare slots, not move-parent names

Research ID: **BREED-002**
Games: Ruby/Sapphire, Emerald, FireRed/LeafGreen ordinary daycare; three retail source representatives executed.
Engine revision: `fe3e3d054ef7d42c31df3e772e8370e2c7587c8a`.
Classification: **Confirmed candidate-helper integration defect**, not a demonstrated global production Invalid.
Confidence: **A for retail slot ordering and reproduced discrepancy**. Complexity: **Low/Medium**.

## Implementation brief

1. Missing: mapping between physical daycare slot 0/1 and the father/mother role IDs used by move inheritance.
2. Why: `Gen3JointEggCandidateEvidence::evaluate` passes father IVs as parent0 and mother IVs as parent1 to `verifyKnownStage`. Retail indexes `daycare->mons[whichParents[i]]`, independently of gender-derived move roles.
3. Sources: `E-PARENT-ROLES`, `NX-JOINT-EGG-REFRESH`, `NX-IV-STAGE-REFRESH`, and the three retail daycare functions already pinned in the registry.
4. Required change: preserve ordered daycare parent IDs in the IV-stage witness; resolve each slot ID to the same independently verified move/IV parent snapshot.
5. Integration: the call to `IV::verifyKnownStage` in `include/Legality/Gen3JointEggCandidateEvidence.h`. The lower-level IV replay is correct for ordered parent0/parent1 inputs.
6. Positive: deposit father in slot 0 and mother in slot 1; then reverse deposit order while keeping biological/role identity stable. Both should match their own retail outputs.
7. Negative: a declared slot mapping inconsistent with the source-attested deposit order excludes that candidate. Duplicate slot IDs, alien parent IDs or missing slot binding cannot create a verified joint route.
8. Unknown: both parent records and the final RNG state may be known while deposit order is missing. Do not silently assume father first.
9. Risk: present helper falsely contradicts some consistent conditional histories; global analyzer Invalid was not reproduced.
10. Acceptance: pass both orderings for all 24 fixed retail stages, preserve all old no-parent/no-stage safeguards, and explicitly test missing/contradictory slot mappings.

## Exact sources and current wiring

- [Joint helper](https://github.com/GlitchedZeus/PokeBank-NX/blob/fe3e3d054ef7d42c31df3e772e8370e2c7587c8a/include/Legality/Gen3JointEggCandidateEvidence.h): `evaluate`, call to `verifyKnownStage` passes `in.father.originalIVs`, then `in.mother.originalIVs`.
- [IV replay](https://github.com/GlitchedZeus/PokeBank-NX/blob/fe3e3d054ef7d42c31df3e772e8370e2c7587c8a/include/Legality/Gen3EggIVInheritanceStage.h): `replay` and `verifyKnownStage`, donor draw 0 selects parent0, 1 selects parent1.
- [Existing joint tests](https://github.com/GlitchedZeus/PokeBank-NX/blob/fe3e3d054ef7d42c31df3e772e8370e2c7587c8a/tests/test_gen3_joint_egg_candidate.cpp): generated expected IVs use the same father-first ordering, so they do not independently validate deposit order.
- Retail source revisions and file hashes are in `SOURCE_REGISTRY.json`; Emerald `InheritIVs`, `DetermineEggSpeciesAndParentSlots` and `_GiveEggFromDaycare` show the separation directly. Ruby and FireRed unchanged `InheritIVs` were independently hosted too.

A source search at the pinned engine revision found this joint helper in headers and tests, not a call from production `src/`. The production analyzer does not recover parents from a detached PK3. This finding therefore concerns the reusable conditional evidence API. Draft #270 at `1f7bde7972c92d881f36cea8b3aded74c4a7f80b` changes candidate/RNG domain checks and does not modify this header.

## Concrete reproduction

Run `python docs/legality/research/scripts/audit_parent_slot_binding.py SOURCE_ROOT`.

The script first executes the unchanged original Ruby, FireRed and Emerald inheritance functions against the independent Python model. It then compiles the actual pinned engine headers and checks the same 24 stages in both parent-role orderings. No production source is edited.

**48/48 lower-level IV replays match retail. 24/24 father-in-slot-0 joint histories match. 24/24 father-in-slot-1 histories are incorrectly returned as `IVCandidateContradicted`.** See [machine-readable cases](../evidence/parent-slot-binding-audit.json).

Minimal Emerald case: entry seed `00000000`, initial child IVs all 0, physical slot0 IVs all 30, slot1 IVs all 31. Retail selects HP, SpD, Spe with donor draws all 0, producing `[30,0,0,30,0,30]` in HP/Atk/Def/Spe/SpA/SpD order. Put the mother in slot0 and the father in slot1. The current joint helper instead supplies all-31 father IVs as parent0, so it contradicts the matching observed retail stage. The fixture supplies a simple independently asserted Bulbasaur move hypothesis `[Tackle,Growl,0,0]`; it is a conditional synthetic unit fixture, not a preserved egg or proof that the full PID/initial-IV construction occurred.

## Suggested implementation shape

Add `daycareSlot0ParentId`, `daycareSlot1ParentId` and an independently verified ordering flag to the shared IV-stage witness. Require the IDs to be distinct and exactly cover the two named parent records, and associate the mapping with the existing shared snapshot/stage token. Pass IVs in that order to `verifyKnownStage`.

If order is absent, return a dedicated Unknown or the existing unknown shared-parent-stage status. An optional search can report which of the two orderings is compatible, but that is a possible ordering, not source-attested historical order. Do not swap father/mother IDs to make the IVs fit: that changes the move donor hypothesis.

## Inputs, loss, exceptions and limits

PK3 stores child PID/IVs and origin metadata, not the parents' daycare slot order, identities or move snapshots. Names “father” and “mother” denote move roles; with Ditto, the donor-role assignment may differ from biological gender. Retail IV donor draws still use physical daycare slots. The stage mapping must come from external history or remain unknown. No native PK3 alone justifies a global hard-Invalid rule here. No Gen IV IV algorithm equivalence is claimed.

## License/reuse

Independently authored research and harness. Original retail functions are read into temporary compiler inputs only; no bodies or game assets are committed. Source/code import remains subject to the existing provenance review. This packet is a fix specification, not a production patch.
