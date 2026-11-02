# Tranche 2 — engine refresh and independent breeding audit

The engine has moved well beyond the original research checkpoint. This tranche freezes accepted source at **`fe3e3d054ef7d42c31df3e772e8370e2c7587c8a`**, inspected from fetched Git objects. Draft #270 is `1f7bde7972c92d881f36cea8b3aded74c4a7f80b`; its domain hardening is separate from the donor-slot finding below. The machine-readable [refresh snapshot](evidence/engine-refresh-2.json) records PR ancestry. Ledger prose still contains older “not yet accepted” passages, so fetched branch ancestry governs this audit.

## What is now implemented

| Former recommendation | Current accepted capability | Remaining boundary |
|---|---|---|
| Candidate-scoped move evidence | `Gen14CandidateMoveHistory` and eight-candidate set coverage | External encounter grounding/exhaustiveness and full acquisition methods remain necessary |
| Emerald ordered history | Full pinned table plus `Gen3EmeraldLevelUpTimeline` and native ingress tests | Conditional same-species, same-game, no-other-mutations histories; other versions/stages still open |
| Pal Park move boundary | HM IDs, history gates, snapshot transitions and joint Emerald-to-Pal-Park path | Requires real paired snapshots and verified history; detached records do not recover them |
| Ordinary egg move construction | FIFO passes and one shared father moveset | Eligibility and donor roles still supplied externally |
| Ordinary egg IV stage | Version-specific replay plus combined move/IV parent hypothesis | Confirmed role-to-physical-slot binding defect in the joint bridge |
| Shedinja evolution moves | Dedicated Emerald evolution evidence | Full alternative acquisition closure remains separate |
| Event moves/ribbons/text | Unified candidate evidence and persisted raw-field capabilities | Original archive/payload attestation and unchanged text still required |
| RNG exhaustion | Explicit bounded-search result and 129-rejection coverage test | No complete unbounded source reconstruction implied |
| Time Capsule direction | Conditional historical edges and prevention of invented second origins | Actual transfer history remains absent from detached records |

These additions were source-inspected. This research did not rerun the entire newer production suite or substitute for the development lane's CI. Only the specific new harnesses described below were executed against the newer engine.

## New confirmed defect: move-parent identity is not daycare order

[BREED-002](packets/BREED-002.md) demonstrates that `Gen3JointEggCandidateEvidence` always passes father IVs as physical parent slot 0. Retail IV inheritance uses deposited daycare slots, independently of the role selection used for inherited moves.

**48 direct stage comparisons matched retail; all 24 reversed-deposit-order joint cases were incorrectly contradicted.** This is an independently reproduced defect in the conditional helper API, not a demonstrated global production Invalid. The helper is referenced by headers/tests, not a production `src/` call at the audited revision. The packet provides the exact mapping fields, unknown-state behavior, regression cases and acceptance criteria needed to fix it.

## New implementation-ready eligibility evidence

[BREED-003](packets/BREED-003.md) replaces vague parent compatibility research with a tested decision table and role-selection rules. **17,496 original-function/model eligibility comparisons passed** across Emerald, Platinum and HGSS, with **24 additional Emerald donor-role cases**. The harness explicitly stubs field access, derived gender and personal/lineage lookup; its cases test control flow, not complete native parent provenance.

The important separations are pair eligibility, move-donor roles, physical IV donor slots, and offspring species resolution. One `PairProof` flag does not establish all four. The new packet explains how to connect those layers without treating absent parents as invalid.

## READY FOR MAIN LEGALITY DEVELOPMENT — updated order

1. **Fix the joint egg slot binding** (Low/Medium): retain source-attested deposit-slot parent IDs, resolve them to the same parent snapshots, preserve Unknown when absent. Reuse the 48 concrete repro cases.
2. **Implement the eligibility/role adapter** (Medium): versioned personal data, full OT IDs, correctly derived genders, explicit Unknown fields. Reuse the three-retail-function decision table and category harness.
3. **Validate the personal-data and species/form adapter independently** (Medium/High): current harnesses intentionally do not claim those lookups. Add concrete native parent-reader fixtures before declaring the full adapter verified.
4. Continue the already-identified all-version acquisition and event archive work; do not rebuild accepted candidate/history abstractions.

## Artifacts, validation and boundaries

- Two new complete packets and two independently authored reproduction scripts.
- Machine-readable [slot-binding results](evidence/parent-slot-binding-audit.json) and [eligibility/role results](evidence/parent-eligibility-vectors.json).
- Expanded source registry: **76 entries**. Expanded test catalog: **143 rows**, including one aggregate eligibility suite row. Counts are not counts of legitimate Pokémon.
- Every new comparison is synthetic/conditional. No native parent save or complete retail egg specimen is claimed.
- Research branch remains based on the original production baseline; newer source is compiled temporarily from pinned Git objects. No production edits, accepted-branch advancement, main changes or merges of #103 are performed here.

Run the two scripts using the external pinned source directory, then the existing source/document validator. A completed run reproduces the defect; it is not a claim that the engine has been fixed. Exact source hashes and a deterministic category-result digest make the evidence reviewable.

## NEEDS FURTHER RESEARCH

Real paired parent/daycare snapshots; proof of source-specific personal/form data; full chain breeding; DP and Ruby/FireRed eligibility execution; Gen IV role-function execution; incense/offspring-lineage integration; and complete joint PID/IV/egg histories remain open. None is silently inferred from the synthetic stage comparisons.
