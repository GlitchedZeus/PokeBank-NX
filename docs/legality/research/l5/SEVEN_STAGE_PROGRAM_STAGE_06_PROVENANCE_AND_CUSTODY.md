# L5 Seven-Stage Program — Stage 6 Original Provenance and Chain of Custody

Date: 2026-10-12. Existing L5-only PR #294 (draft, unmerged).
All work restricted to L5 audit-owned files and independent L5 workflow.
Original source saves, event payloads and commercial archives remain absent.
This stage is an evidence-gating implementation, **not** an assertion that
any original event card, native retail save or complete historical route
has been independently authenticated.

## Re-fetched authoritative source identities

- L5: `83d68a20626e3f90d5e1f9823762eaf47e26043b` at stage start.
- L1: `f7138ad95ed1e0cccab283d51e5ba87502bb3f79`.
- L2: `110372b2a877c41881eb5466392684d378b4a90d`.
- L3: `e6017edc18f053726dc2882e4caa69bcd572162a`.
- L4: `c8bc938018458d4eba5c308c5d96691cfed690ed`.
- Coordinator PR #298: `ad9c1c859f94c0321a5d17489c72b0e7dbb9f440`.
- Accepted legality: `d29eada5c087e201e00c749703f1a2de3ddfadeb`.

The four authentic L5 research registry **Git blob** pins, verified by
reading exact GitHub file objects, are:

| Manifest | Exact Git blob |
|---|---|
| Q07 247 indexed Gen IV event projections | `8261720c96cede640bff89aa091ea0ac5f1e4029` |
| Q08 redistribution restrictions | `7cd3d0a227dc9b008f7c304581a2d7e8d85f06e9` |
| Q09 synthetic fixture registry | `7b05973a077d044f46884719d41a91fe64372690` |
| Q10 paired original-source witness requests | `4376cd1dfc2fe53ae42a4bcfa1c6fb4720b4818c` |

Q07 records reflect structural PKHeX wc4 projection at pinned
`6501f0ab46e8f8ca048539dbaf8cae8cb104e722`, **not**
the original distribution payload. Source header Git blob reference
`6dd1fea4a9ba3669f957fbd4e70d076cb0f85529`
is recorded as a catalog identity, not as an original receipt.

## Actual research baseline after GitHub inspection

- 247 indexed event templates, 247 unique index/species/TID/SID/card
  identities; template indexes 0..246 and source lines 6..252.
- Only 70 distinct card IDs, with **219 templates bearing repeated
  card IDs** across 42 collision groups. Card ID alone is insufficient
  to match an archive or to issue a legality verdict.
- **0** verified original distribution receipts.
- **0** independently verified original archive payload SHA-256 digests.
- All **247** archive locations and digests are still null and
  archive provenance is explicitly unverified.
- Q10 has **22 missing original-stage requests**, **0** verified paired
  original saves, **0** event redemption receipts and **0** license-ready
  archive sources.
- Q08 has 14 third-party source entries without completed case-specific
  redistribution clearance. Third-party source importing, bulk retail
  table copying and original event payload redistribution remain blocked.

## Implemented executable engineering

1. `tools/audit_legality_l5_stage6_provenance.py`:
   strict Git-blob-pinned manifest verifier for all 247 WC4 records,
   source lines, unique compound identities, collisions, missing
   historical receipts, 14 license/source rows, 12 QA fixture entries
   and 22 original witness requests. It rejects fabricated archive
   hashes, source substitutions, false original/pair certifications,
   unsupported license claims and synthetic-to-retail promotion.
   Includes SHA-256 linked off-repository receipt-chain verification.
   A hash chain establishes integrity of a *provided* record sequence,
   **never** actual original capture or owner permission by itself.
   Every result retains release_gate=BLOCKED.

2. `tools/audit_legality_l5_stage6_receipt_chain.h`:
   bounded 16-record original-source / same-Pokémon / same-route model,
   reject-empty/duplicate/out-of-order records, lineage and trusted
   head requirements, non-forgeable-in-the-model approval booleans,
   separate original specimen, consent, distribution-authorization,
   mutable source and full route enumeration gates. It never generates
   global `Legal` or `Invalid` classification. This is QA-only and
   does not modify production engine verdicts.

3. `tests/fixtures/legality_l5_stage6_receipt_chain_fuzz.cpp`:
   200,000 deterministic bounded mutation cases plus directed
   controls, guarding missing consent/signatures, altered lineage,
   source mismatches, untrusted digest, duplicate identity, wrong
   chronology, absent original witnesses, research catalog projection
   vs original receipt, and authorized vs unauthorized redistribution.

4. `.github/workflows/l5-independent-provenance-stage6.yml`:
   exact-head fail-closed L5-only CI; verifies original compiled C++
   fixture Git blobs and actual 247-row local repository JSON/CSV
   manifests, all 31 synthetic tamper checks, compiler/sanitizer matrix
   and explicit no-source-save-mutation requirements.

## Exactly executed local tests

The following tests compiled the exact C++ file bytes whose Git blobs
are now committed to L5:
- `tools/audit_legality_l5_stage6_receipt_chain.h`:
  `63a8beff9f5b2bcca86d83eb9575b59789015229`
- `tests/fixtures/legality_l5_stage6_receipt_chain_fuzz.cpp`:
  `d07351f551cdb51afc59bcdd4e7e22fe09c31cb4`
- `tools/audit_legality_l5_stage6_provenance.py`:
  `f1119cbf37a42ca4faea520d2afeb3708b6148a3`

| Executed command family | Status | Checks |
|---|---|---:|
| GCC 14.2 C++20 O2, strict -Wall/-Wextra/-Wpedantic/-Werror | PASS | 930,275 |
| GCC 14.2 O1 -g -fsanitize=address,undefined | PASS | 930,275 |
| Clang 17 C++20 O2, strict Werror | PASS | 930,275 |
| Clang 17 O1 -g -fsanitize=address,undefined | PASS | 930,275 |
| Python 3 validator --self-test | PASS | 31 negative/attestation controls |
| Python py_compile | PASS | Syntax |
| GCC -DNDEBUG negative build | Expected compile rejection | Assertions preserved |

Total **3,721,100 actually executed C++ assertions**. ASan was set
to detect_leaks=0 and halt_on_error=1; UBSan halt_on_error=1 and
print_stacktrace=1. An initial GCC signedness -Werror failure was
fixed in the L5-owned header and the final complete four-mode matrix
rerun successfully.

The GitHub connector independently inspected the live full Q07,
Q08 and Q10 files, verifying **all 247** compound template IDs,
source lines, absent original metadata, license restrictions and the
22 unmet history rows: **zero source-structure mismatches observed**.
This is *independent structured source inspection*, not execution
of the new Python verifier against a full local checkout.

## Explicit blocked acceptance gate and coordinator handoff

The complete repository is still unavailable locally because
GitHub DNS resolution fails. Therefore the committed Python validator
has **NOT YET** been executed against its actual four manifest files
in a local full checkout. The independent Stage 6 CI workflow was
submitted for GitHub-hosted execution; only completed green logs from
its exact commit can close that code-level gate. No whole production
`Legality::analyze`, real original retail save, signed archive
receipt, live PKHeX verifier, or 336-cell complete-game parity was
independently authenticated.

A source hash identifies a catalog; it is not an archived distribution
digest. A self-asserted signature/consent flag is not proof of owner
permission. An original retail event comparison requires an
independently trusted external witness, separately obtained original
payload digest, release/time provenance, original game/save identity,
complete competing historical routes and case-specific redistribution
permission before any higher-level claim is even considered.

**Release decision: BLOCKED.** Do not merge the L5 branch into MAIN,
accepted legality or coordinator solely from this result.

The **next and final separate continuation is Stage 7**:
reproducible 336-cell dashboard, false-Invalid emissions audit, CI
provenance reconciliation and final coordinator handoff. Stop expansion
after Stage 7.
