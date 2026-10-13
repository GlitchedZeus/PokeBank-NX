# Original PKHeX.Core source pin verification — 2026-10-12

**Scope:** authoritative owner-uploaded \`PKHeX-master.zip\` source-reference
manifest, \`PKHEX_USER_ARCHIVE_SOURCE_MAP.json\`. This is a host-only license
and source-identity checkpoint; it does **not** imply native legality parity.

- Owner's original archive SHA-256: \`b90420f2dc1bceeadde318601f194a819b049af893e4318243786f04047abd9b\`.
- Original zip has **no authenticated Git commit metadata**.
- Independently selected public candidate: \`kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e\`.
- Method: fetch each exact repository file at the pinned public Git commit,
  compute SHA-256 of the UTF-8 file content, and compare with the independently
  recorded SHA-256 of the owner-uploaded ZIP member. The SHA-256 computation
  was self-checked against the standard \`abc\` test vector before comparison.
- **Result: 29/29 manifest-listed files matched exactly; 0 differences,
  0 unreadable entries.** The two checks were run in batches of 13 and 16.

## Files authenticated

| Group | Files checked | Result |
| --- | ---: | --- |
| Core legality analysis and legal info | 2 | 2/2 |
| Original encounter generators: G5, G6, G7, LGPE, G8, PLA, BDSP, G9, Z-A | 9 | 9/9 |
| LearnGroup G5, G6, G7, G8, G9, G9a and HOME | 7 | 7/7 |
| Breeding, transfer, ball, Mystery Gift and ability verifiers | 5 | 5/5 |
| PK5, PK6, PK7, PK8, PK9 and PA9 format classes | 6 | 6/6 |
| **Total** | **29** | **29/29** |

This is a **per-file source match**, not proof that the entire Git tree equals
the owner ZIP. Keep the archive's commit metadata explicitly unverified.
The CI source-pin script must verify these 29 hashes again against the actual
GitHub Actions checkout; do not disable or downgrade that check.

## Acceptance boundaries

1. The original .NET 10 PKHeX.Core is used in the **host-only oracle**; the
   native Switch app cannot directly execute it. Native C++ remains a minimal
   audited format adapter and must retain source file hashes and GPL obligations.
2. The native PK5, PK6 and PK7 analyzers do not authenticate original game,
   encounter, Mystery Gift, transfer, breeding, ownership or custody history.
   Unknown history remains **Incomplete**.
3. Synthetic normal/sanitizer test success is distinct from authentic original
   PKHeX.Core differential execution and original retail save vectors.
4. The GitHub Actions exact-head run, full original C# compiler/legality-oracle
   execution, full checkout and authentic retail parity have **not** passed as
   part of this source verification. They are independent blocking gates.
5. PR #301 remains OPEN/DRAFT; no merge to accepted engine, main app or
   product/editor, and no original save-write authorization.
