# SAV7 signature blocker: findings and experiment

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Status

**Production SAV7 export remains UNSUPPORTED.** The claimed PKHeX/PKSM tail disagreement is materially weakened by a newly reproduced vendor defect. Do not flip `ExportSupported`, do not call `reverseCrypt` to construct a signing payload, and do not change vendor files or suppressions in this research branch.

## Exact coverage and construction: ORIGINAL + PKHEX

| Family | Signature bytes | SHA256 input | Reference |
|---|---|---|---|
| SM | `[0x6BB00,0x6BB80)` | `[0x6BC00,0x6BD40)` (0x140 bytes) | MemeCrypto.SignInPlace / SavSUMO::resign |
| USUM | `[0x6C100,0x6C180)` | `[0x6CA00,0x6CB50)` (0x150 bytes) | MemeCrypto.SignInPlace / SavUSUM::resign |

Sources: [SignInPlace / SignMemeDataInPlace](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Encryption/MemeCrypto/MemeCrypto.cs); [include/Integration/Gen7/Gen7SaveLayout.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/2f065ee2b0768d01bf621f969aef01120c6231e3/include/Integration/Gen7/Gen7SaveLayout.h); PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c` `source/sav/SavSUMO.cpp` and `SavUSUM.cpp`. Current PKSM-Core `d9bca6fcdf622760a0aa963565b102276e2ae2a3` retains the same resign construction.

Meme signature region is 128 bytes, while RSA-transformed portion is the last 96 bytes. Payload begins with 32-byte SHA256(checksum-table). Last 8 bytes are truncated SHA1 of the preceding 120 payload bytes. AES key derives from SHA1 of public key DER plus unencrypted prefix; specialized AES mixing operates on the 96-byte tail, sign bit is cleared, RSA private operation signs. Verification tries both sign-bit cases, decrypts and compares the 8-byte SHA1. See `MemeKey.cs`, `MemeCrypto.cs`, and unchanged vendor `memecrypto.c::memecrypto_sign/verify`. This is not ordinary “RSA sign the whole save”.

All block CRCs are finalized first, with the signature area cleared for block36's CRC. The table digest and signature follow. PKHeX SAV7 clears the signature region on load; its signing tail therefore starts as zeros. Preserve original signed bytes separately for immutability.

## Reproduced defect: INDEPENDENT

Pinned `FlagBrew/memecrypto@bd3b16ee2c12bd9ee353cf3098cb998e7880638c` `reverseCrypt`:

1. Allocates `dec[0x80]` uninitialized.
2. AES helper writes only `MEME_LEN = 0x60` bytes.
3. Hashes `dec` over `0x80` bytes at line 218 (and again on fallback), reading an uninitialized tail.
4. Copies to caller output only if an internal check succeeds; returns `void`, so caller cannot learn failure.
5. PKSM `resign` passes an uninitialized `decryptedSignature[0x80]`, then overwrites only its first 32 bytes before signing.

`tools/signature_probe.c` fills output with two distinct sentinels and calls `reverseCrypt` on a zero signature region, matching the caller's cleared-region input. On GCC 16.2.1 and Clang 23.1.1 both arrays stayed unchanged. This is an observed no-output path, **not a deterministic mathematical property under UB**. Clang MemorySanitizer separately reported `use-of-uninitialized-value` at `memecrypto.c:218:9`; see `evidence/signature-msan.log` and TEST_RESULTS.

Therefore the observed/nonzero “PKSM tail” may be stack contents, not a meaningful preserved payload. Do not infer from that output that the retail game requires it. A proposed fix is to define initialized payload construction and return status in a separately reviewed vendor lane, or use an independently specified signing construction. No patch is applied here.

## What differs, and what remains unknown

Both an initialized zero-tail payload and an initialized 0xA5-tail payload signed by the unchanged vendor verify, and one-bit prefix tampering fails, under both compilers. Payload tail bytes `[0x20,0x78)` differ; `[0x78,0x80)` is regenerated SHA1 and must be excluded from “zero tail” diagnostics. Existing Gen7Save does exclude those 8 bytes.

Verifier acceptance proves cryptographic self-consistency and table binding. It cannot prove whether the game checks additional tail semantics. Different valid ciphertext/signature bytes are an implementation detail only **if the game accepts their decoded payloads equivalently**; this acceptance is UNKNOWN. Neither agreement with a synthetic signer nor one retail sample's tail alone establishes that.

## Existing vendor hygiene: ORIGINAL

`sha1.c:203` signed left shift is already exempted with `-fno-sanitize=shift-base` on vendor objects only. `rsa.c` initializes mpz temporaries without clearing them; LSan suppressions are limited to rsa_encrypt/rsa_decrypt. ASan/UBSan does not detect uninitialized reads, so passing existing suites does not contradict the MSan finding. Preserve exemptions until a licensed upstream/submodule fix is reviewed. Repeated discovery re-verifies signatures and can accumulate the known leak: avoid polling full parse loops.

## Evidence plan and acceptance gates

1. Run existing strict reader on locally retained untouched game-written SM **and** USUM saves, recording family/patch, file length, CRC counts, signature/table binding and tail category only. Do not publish signatures or Trainer data.
2. Generate both initialized signing constructions on app-owned test copies after a tiny known field edit; compare public-key verification using **independent PKHeX and vendor** verifiers. Current probe uses only the vendor verifier; PKHeX executable differential remains pending (.NET unavailable here).
3. Reparse all CRCs and identities; require exact decrypted field mask, affected CRCs, and signature-only whole-file changes. Keep source hashes fixed.
4. On expendable local backups, explicitly authorized hardware test each output in the matching game/patch; load, observe edit, save again, export and strictly reparse. A successful load is necessary but save/reopen is stronger. Retain rollback backups and record which construction was accepted.
5. Separate vendor UB/leak remediation from game-format/export acceptance; re-run normal, ASan/UBSan, LSan and MSan with reviewed exemption changes.

Until these gates, unsigned staging is reusable engineering work and no SAV7 file is published as a usable export.

## Separate Gen VII Bag defect found during this audit

Both `PlayerBag7SM.GetMaxCount` and `PlayerBag7USUM.GetMaxCount` at **both** inspected PKHeX pins explicitly allow item797 (Z-Ring) count2. `Gen7Bag.h::decodeBag7` instead checks the pocket default max1 and rejects the whole Bag. The isolated `tests/reproduce_gen7_zring.cpp` confirms x1 accepted and x2 rejected for both families under GCC/Clang normal and ASan/UBSan. This is a reproduced source-reference mismatch, not new retail-save acceptance evidence. Repair should introduce a shared per-item quantity rule used by read, staged validation and independent verifier; keep story-item Add disabled. Test x2 accepted, x3 rejected, other key items capped1 and untouched slot metadata. No lane code was changed.
