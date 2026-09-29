# Full Audit Remediation Matrix

Created from frozen forensic evidence `143c5e5c341d4f85af30e013808a37d6719560fe` and reconciled forward to live MAIN / PR #92 head `ea6e9079bd8c5278ecd27fd9f81c18279d7c807d`.

The forensic audit is closed: 746/746 tracked paths accounted, 711/711 text files fully read, 35/35 non-text entries inspected, 0 pending ledger entries. This file is a remediation ledger, not a continuation of repository coverage.

## Baseline and invariants

- Original severity counts: P1 2, P2 17, P3 20, P4 5.
- Current MAIN reconciliation point: PR #92 branch `feature/gen4-full-editor-20260928` at `ea6e9079bd8c5278ecd27fd9f81c18279d7c807d`. PR #101 preserves its existing remediation history; the one newer MAIN delta was documentation-only and its exact file contents were synced forward without reset/rebase.
- Frozen evidence branch remains untouched at `143c5e5c341d4f85af30e013808a37d6719560fe`.
- Current baseline CI note: upstream MAIN `58a56f8d…` still has Product UI Native red before remediation; Host Tests were in progress and the Gen IV gate was cancelled by newer lane movement when this ledger was refreshed. Treat this as pre-existing UI-lane drift, not a new audit finding.
- Source saves remain immutable. Emulator/installed-title live writes remain disabled. Cross-game True Move and source injection remain locked. No Gen V or Master Vault backend work is in scope.

Status semantics: **OPEN** = not implemented; **IN PROGRESS** = active fix; **FIXED** = implementation exists but exact-head validation is incomplete; **VERIFIED** = focused + relevant broader validation proves the fix; **DEFERRED WITH JUSTIFICATION** = explicit blocker/prerequisite recorded.

## Proposed remediation order

Order is risk-based, not numeric: repository-integrity/native-CI guardrails first; memory/parser/save-integrity next; staged-move and data-fidelity next; launch/UI/tooling/docs last. P3 memory-safety findings 025/026 are intentionally elevated ahead of lower-risk P2 correctness work.

| Order | Finding | Sev | Status | Short title |
|---:|---|:---:|---|---|
| 1 | AUDIT-001 | P1 | FIXED | destructive mutable PKSE import workflow |
| 2 | AUDIT-032 | P1 | FIXED | PR #97 removes AppShellSection::Collections but still references it |
| 3 | AUDIT-002 | P2 | FIXED | no general unfiltered native PR compile gate |
| 4 | AUDIT-020 | P2 | FIXED | SV/Z-A MyStatus size guard permits an out-of-bounds gender read |
| 5 | AUDIT-031 | P2 | FIXED | SC SHA-256 message decoding uses signed-shift undefined behavior |
| 6 | AUDIT-025 | P3 | FIXED | defaulted Pokémon move operations duplicate raw-buffer ownership |
| 7 | AUDIT-026 | P3 | FIXED | Gen III and modern entity constructors do not fully enforce/normalize native record length |
| 8 | AUDIT-021 | P2 | FIXED | SWSH/SV/Z-A authenticate the SC container but do not validate required game layout |
| 9 | AUDIT-017 | P2 | FIXED | FRLG mutable workspace selects rotating slot before checksum validation |
| 10 | AUDIT-023 | P2 | FIXED | LGPE durable validator rejects the authentic 1 MiB save image |
| 11 | AUDIT-018 | P2 | FIXED | LGPE mutable workspace rewrites CRCs before validating pre-existing block integrity |
| 12 | AUDIT-022 | P3 | FIXED | BDSP pre-open validation ignores its stored whole-file MD5 |
| 13 | AUDIT-024 | P2 | FIXED | Gen IX inventory decoder leaves persisted flags indeterminate |
| 14 | AUDIT-039 | P2 | FIXED | Backup save can serialize while held-Pokémon rollback failed |
| 15 | AUDIT-015 | P2 | FIXED | Failed backup creation can leave a partial folder surfaced as a backup |
| 16 | AUDIT-029 | P2 | FIXED | Gen II finalization can serialize an in-progress packed move with carried Pokémon omitted |
| 17 | AUDIT-030 | P2 | FIXED | Gen I finalization can serialize an in-progress packed move |
| 18 | AUDIT-037 | P2 | FIXED | Gen I/II group pickup can strand an active staged move after presentation-refresh failure |
| 19 | AUDIT-013 | P2 | FIXED | Legacy Bank migration skips checksum validation used by normal Bank load |
| 20 | AUDIT-028 | P2 | OPEN | Gen VIII/IX base-stat form routing returns zero/wrong rows and can index beyond valid arrays |
| 21 | AUDIT-027 | P2 | OPEN | LGPE Meltan/Melmetal base-stat rows are unreachable and edits rewrite party stats from base 0 |
| 22 | AUDIT-014 | P2 | OPEN | Session-wide source read-only gate disables app-owned Bank mutation |
| 23 | AUDIT-019 | P3 | OPEN | modern encrypted blank slots are parsed as live species-0 objects |
| 24 | AUDIT-012 | P3 | OPEN | Settings persistence truncates in place and ignores write/close failure |
| 25 | AUDIT-040 | P3 | OPEN | RetroArch playlist auto-match can accept wrong-family game content with the same basename |
| 26 | AUDIT-016 | P3 | OPEN | RetroArch launch matching is basename-only and first-match wins |
| 27 | AUDIT-043 | P3 | OPEN | Device-observed Gen IV save rows can lose trainer-name presentation despite synthetic parser coverage |
| 28 | AUDIT-033 | P3 | OPEN | Gen IV move-stat presentation uses HGSS values for Diamond/Pearl |
| 29 | AUDIT-034 | P3 | OPEN | HD sprite recovery/preflight can accept corrupt existing PNGs |
| 30 | AUDIT-035 | P3 | OPEN | personal/learnset regeneration still searches the removed SPECIES_NAMES symbol |
| 31 | AUDIT-044 | P3 | OPEN | Fontstash allocation failures can become null-pointer crashes during text rendering |
| 32 | AUDIT-006 | P3 | OPEN | LeakSanitizer is disabled even where comments say CI keeps it enabled |
| 33 | AUDIT-003 | P3 | OPEN | mutable native toolchain image |
| 34 | AUDIT-036 | P3 | FIXED | Product Home Help overlay advertises stale controller actions |
| 35 | AUDIT-011 | P3 | OPEN | Search preview vertical wrap changes columns and the test blesses it |
| 36 | AUDIT-038 | P3 | OPEN | Legacy Save Instances scroll logic assumes one more visible row than the renderer draws |
| 37 | AUDIT-041 | P3 | OPEN | Standalone runtime contract falsely says RetroArch is never invoked |
| 38 | AUDIT-010 | P3 | OPEN | canonical engineering authority chain points to obsolete work |
| 39 | AUDIT-042 | P3 | OPEN | Active session/roadmap docs route work through obsolete project state |
| 40 | AUDIT-004 | P4 | OPEN | stale theme choices in physical bug template |
| 41 | AUDIT-005 | P4 | OPEN | historical branch-specific workflows remain tracked |
| 42 | AUDIT-007 | P4 | FIXED | top-level README materially understates current Gen IV implementation |
| 43 | AUDIT-008 | P4 | OPEN | checked-in Visual Studio metadata is stale PKSE-era configuration |
| 44 | AUDIT-009 | P4 | OPEN | recovery metadata still describes the repository as private / old production branch |

## Detailed finding ledger

### 1. AUDIT-001 — destructive mutable PKSE import workflow

- **Severity:** P1
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `.github/workflows/import-pkse.yml`
- **Concrete failure mode:** editing this workflow can replace the PokeBank NX default-branch tree with whatever PKSE master contains at run time.
- **Reachable in current product paths:** CI/repository only; triggered by workflow changes/push conditions
- **Save-integrity impact:** Indirect HIGH repository-integrity risk; not a runtime save path
- **Security / memory-safety impact:** No direct process-memory bug
- **User-visible impact:** Could replace project tree/default-branch code
- **Recommended fix:** retire it after the historical import, or make it manual/read-only and pin an immutable upstream revision. Never overlay upstream onto `.` and push directly to `main`.
- **Regression-test strategy:** Run the affected workflow/target on the exact remediation head and assert the intended gate executes.
- **Dependency / sequencing:** None
- **Proposed remediation order:** 1
- **Status:** FIXED
- **Current-code reconciliation:** `.github/workflows/import-pkse.yml` is retired on PR #101; `tests/test_ci_repository_safety_contract.py` is wired into Host Tests to block reintroduction of the destructive import/direct-main-push pattern. Exact-head Host Tests are still running, so this is not VERIFIED yet.

### 2. AUDIT-032 — PR #97 removes AppShellSection::Collections but still references it

- **Severity:** P1
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/UI/AppShellModel.h`, `src/UI/AppShellScreen.cpp`, `.github/workflows/product-ui-native.yml`
- **Concrete failure mode:** PR #97 head `5f19fd14628c182db135038864036b6eb1b86c49` removes `Collections` from `AppShellSection` when replacing the old eight-area dashboard, but `AppShellScreen::update()` still contains `infoSection == PokeBank::UIModel::AppShellSection::Collections` while selecting an organization-preview kind.
- **Reachable in current product paths:** Historical compile blocker; implementation no longer present in current MAIN
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Would block native Product UI build; code fix already exists
- **Recommended fix:** remove the obsolete `AppShellSection::Collections` branch from `OrganizationPreview` handling or replace it with the intended `Pokedex` behavior, then run the native product-UI workflow and host contract tests at the exact PR head.
- **Regression-test strategy:** `tests/test_app_shell_model.cpp` validates the revised enum/navigation model but does not compile `AppShellScreen.cpp`, so it cannot catch this stale screen-level enum reference. `tests/test_game_hub_contract.py` is also text-contract based.
- **Dependency / sequencing:** Verification-only; current Product UI Native is red for unrelated dock contract
- **Proposed remediation order:** 2
- **Status:** FIXED
- **Current-code reconciliation:** live MAIN has no `AppShellSection::Collections` reference. Exact-head Product UI Native did not reach native compile because an unrelated dock contract failed first, so this is not VERIFIED yet.

### 3. AUDIT-002 — no general unfiltered native PR compile gate

- **Severity:** P2
- **Confidence:** HIGH
- **Affected files / subsystem:** `.github/workflows/**`
- **Concrete failure mode:** a host-green PR can still break libnx/device-only compilation or packaging.
- **Reachable in current product paths:** Active for every PR because the missing gate is the issue
- **Save-integrity impact:** None directly
- **Security / memory-safety impact:** None
- **User-visible impact:** Host-green changes can fail Switch build/package
- **Recommended fix:** add one broad PR native compile/link gate; keep specialized filtered jobs as supplemental checks.
- **Regression-test strategy:** Run the affected workflow/target on the exact remediation head and assert the intended gate executes.
- **Dependency / sequencing:** Do early so later C++ fixes get native coverage
- **Proposed remediation order:** 3
- **Status:** FIXED
- **Current-code reconciliation:** PR #101 adds `.github/workflows/native-pr-build.yml`, triggered on every pull request with no path filter, and it performs a clean devkitA64 compile/link of the exact PR application head. Exact-head Native PR Gate is running, so this is not VERIFIED yet.

### 4. AUDIT-020 — SV/Z-A MyStatus size guard permits an out-of-bounds gender read

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`
- **Concrete failure mode:** both functions reject only when `block.data.size() < 4`, read ID32, conditionally guard the OT-name field, and then unconditionally read `block.data[0x05]` for trainer gender. A 4- or 5-byte MyStatus object therefore passes the initial guard and indexes past the vector.
- **Reachable in current product paths:** Malformed SV/Z-A MyStatus path
- **Save-integrity impact:** None unless malformed candidate later edited; open should fail
- **Security / memory-safety impact:** HIGH OOB read / memory-safety issue
- **User-visible impact:** Malformed save can crash parser
- **Recommended fix:** require the complete minimum region actually consumed by the parser before any field access (at minimum through gender byte, preferably the full supported MyStatus layout), and make that requirement part of a pre-open game-specific layout validator.
- **Regression-test strategy:** authenticated SC fixtures with MyStatus sizes 4 and 5 must be rejected cleanly without constructing a Trainer; size 6 must not read beyond bounds; full expected native MyStatus geometry should pass.
- **Dependency / sequencing:** Defense-in-depth with AUDIT-021, but fix independently first
- **Proposed remediation order:** 4
- **Status:** FIXED
- **Current-code reconciliation:** both `Trainer9SV::parseMyStatusBlock()` and `Trainer9LZA::parseMyStatusBlock()` now use shared `Gen9MyStatus::hasCoreFields()` and reject blocks shorter than six bytes before the unconditional gender read. `tests/test_gen9_my_status_guard.cpp` is included in normal Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 5. AUDIT-031 — SC SHA-256 message decoding uses signed-shift undefined behavior

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Utils/SHA256.cpp`, `include/Utils/SHA256.h`; consumed by the SC save encryption/decryption layer.
- **Concrete failure mode:** the first sixteen message-schedule words are assembled with expressions such as `buffer[i * 4] << 24`. Each `buffer[]` element is a `uint8_t`, which undergoes integer promotion to signed `int` before the shift. For a first byte >= 0x80, shifting that promoted positive int by 24 produces a value outside the representable signed-int range, which is undefined behavior in C++.
- **Reachable in current product paths:** Every modern SC SHA-256 operation with high-bit input
- **Save-integrity impact:** HIGH integrity primitive correctness
- **Security / memory-safety impact:** MEDIUM signed-shift UB
- **User-visible impact:** Hash behavior is formally undefined on common save bytes
- **Recommended fix:** cast every byte to `uint32_t` before shifting, e.g. `(static_cast<uint32_t>(buffer[n]) << 24)`, or centralize through an unsigned big-endian 32-bit reader. Add known-answer tests and a real SC fixture.
- **Regression-test strategy:** standard SHA-256 vectors (empty string, `abc`, multi-block input), a block containing bytes >= 0x80 in every word position, and a captured SC-container hash fixture under UBSan/host CI.
- **Dependency / sequencing:** Fix very early; verify with UBSan and known vectors
- **Proposed remediation order:** 5
- **Status:** FIXED
- **Current-code reconciliation:** `SHA256::transform()` casts each schedule byte to `uint32_t` before left shifting. `tests/test_sha256.cpp` covers standard known-answer vectors plus a 64-byte high-bit vector and is wired into normal Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 6. AUDIT-025 — defaulted Pokémon move operations duplicate raw-buffer ownership

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Pokemon/Pokemon.h`; `Pokemon1ReadOnly.h`, `Pokemon2ReadOnly.h`, `Pokemon3FRLG.h`; directly verified modern declarations in `Pokemon7LGPE.h`, `Pokemon8SWSH.h`, `Pokemon8BDSP.h`, `Pokemon8LA.h`, `Pokemon9SV.h`, `Pokemon9LZA.h`
- **Concrete failure mode:** `Pokemon` owns `buffer` as a raw `std::byte*` and its destructor executes `delete[] buffer`. A compiler-generated move of a raw pointer copies the pointer value; it does not null the source. The defaulted move also copies the `std::span`, so after a move both source and destination refer to the same allocation.
- **Reachable in current product paths:** Any concrete Pokémon object that is move-constructed/assigned
- **Save-integrity impact:** Indirect save risk through crash/corrupted ownership during operations
- **Security / memory-safety impact:** HIGH double-free/UAF ownership bug
- **User-visible impact:** Potential crash or corrupted entity lifetime
- **Recommended fix:** implement custom base move construction/assignment that transfers `buffer`, rebuilds `data` to the transferred allocation, copies `dataSize`, and clears the source's pointer/span/size. Alternatively replace the raw allocation with `std::unique_ptr<std::byte[]>` and still ensure the span is rebound after moves.
- **Regression-test strategy:** move-construct and move-assign each concrete format under ASan; destroy the moved-from object before reading the moved-to object; verify data remains valid and exactly one owner frees the allocation.
- **Dependency / sequencing:** Fix early before broad sanitizer verification
- **Proposed remediation order:** 6
- **Status:** FIXED
- **Current-code reconciliation:** the base `Pokemon` move constructor/assignment now transfer sole `buffer` ownership, rebuild the span, release any prior destination allocation, and clear the moved-from object. `dataSize` now defaults to zero for non-buffer wrappers. `tests/test_pokemon_move_ownership.cpp` exercises move-construction after source destruction, move-assignment over an existing allocation, and self-move under normal and sanitizer suites. Exact-head CI is pending, so this is not VERIFIED yet.

### 7. AUDIT-026 — Gen III and modern entity constructors do not fully enforce/normalize native record length

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Pokemon/Pokemon3FRLG.h`; `include/Pokemon/Pokemon7LGPE.h`, `include/Pokemon/Pokemon8SWSH.h`, `include/Pokemon/Pokemon8BDSP.h`, `include/Pokemon/Pokemon8LA.h`, `include/Pokemon/Pokemon9SV.h`, `include/Pokemon/Pokemon9LZA.h`; corresponding entity encryption implementations; shared `src/Encryption/Encryption.cpp`; stat implementations for SWSH/BDSP/SV/Z-A.
- **Concrete failure mode:** the modern entity constructors accept an arbitrary `std::span<const std::byte>` and their decryptors immediately read the first four bytes and then operate on a fixed header + four-block region without verifying that the supplied span contains it. `cryptPokemon()` likewise forms a fixed `subspan(8, blockSize * blockCount)`. Gen III's decryptor is safer and returns a copied short buffer when input is below 0x50, but `Pokemon3FRLG` still binds that short result as a live entity and its ordinary fixed-offset accessors then assume the native header/data region exists. Truncated entity input can therefore reach out-of-bounds access before checksum validation rejects it.
- **Reachable in current product paths:** Malformed/truncated Gen III-modern entity input
- **Save-integrity impact:** Indirect; invalid entity must fail before save model use
- **Security / memory-safety impact:** HIGH OOB access risk
- **User-visible impact:** Malformed record can crash before checksum rejection
- **Recommended fix:** make entity length an enforced boundary before decryption. Accept only documented native stored/party sizes (or an explicitly justified superset), and normalize valid stored entities to an owned party-sized buffer before any party-stat API is exposed. Return failure/invalid state for malformed lengths instead of relying on callers to be perfect.
- **Regression-test strategy:** Gen III and every modern entity constructor should reject spans shorter than the native stored size without exposing a usable entity; exact stored-size and exact party-size records should construct deterministically; SWSH/BDSP/SV/Z-A stored-size objects should support level/stat getters and stat-affecting edits under ASan without an out-of-bounds access; unexpected intermediate/oversized lengths should have an explicit policy.
- **Dependency / sequencing:** Pairs naturally with sanitizer tranche after AUDIT-025
- **Proposed remediation order:** 7
- **Status:** FIXED
- **Current-code reconciliation:** affected Gen III/modern constructors now accept only their documented stored/party record sizes, reject malformed lengths into an explicitly invalid full-sized safe buffer, and modern stored records normalize into owned party-sized buffers before party-stat APIs are exposed. `Encryption::cryptPokemon()` now bounds-checks its fixed block region before taking a subspan. Commit `d183fa5b172678cb9d4d3d44247b3154a84e716c` adds `fixture entity-native-length-boundary` to the existing conversion-entity golden suite, covering Gen III plus LGPE/SWSH/BDSP/PLA/SV/Z-A malformed/intermediate/oversized rejection, valid stored/party construction, stored-to-party normalization, representative stat reads, stat-affecting edits, and the short-buffer crypto guard. That target is already wired to Host Tests and ASan/UBSan. Exact-head workflows are queued, so this is not VERIFIED yet.

### 8. AUDIT-021 — SWSH/SV/Z-A authenticate the SC container but do not validate required game layout

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Save/GetSaveFileContents.cpp`, `src/Save/Block.cpp`, `src/Trainer/Trainer8SWSH.cpp`, `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`
- **Concrete failure mode:** unlike PLA, Sword/Shield, Scarlet/Violet and Z-A have no per-game semantic layout gate. Their open preflight returns success without validating the SC file, and their post-write validator verifies only the SC hash plus complete syntactic block parsing. It does not require unique keys or required block types/sizes. The generic parser also permits duplicate keys.
- **Reachable in current product paths:** Current SWSH/SV/Z-A open/write validation
- **Save-integrity impact:** HIGH structural/save-integrity gate for app-owned workspaces
- **Security / memory-safety impact:** MEDIUM malformed-layout exposure
- **User-visible impact:** Malformed authenticated container can reach trainers/writers
- **Recommended fix:** add game-specific SWSH/SV/Z-A read/layout validators modeled on `validatePLAReadLayout()`: unique keys, exact/minimum supported type and size for every consumed block, Pokémon checksum/basic-domain validation for occupied slots, and fail-closed open/write validation. Do not use resize as recovery for a malformed required native block.
- **Regression-test strategy:** duplicate required keys; missing required blocks; short Party/Box/MyStatus blocks with a valid outer hash; unsupported block type for a required key; all must fail before Trainer construction and before any serializer mutation.
- **Dependency / sequencing:** Build after immediate OOB fix; validator covers several downstream paths
- **Proposed remediation order:** 8
- **Status:** FIXED

- **Current-code reconciliation:** PR #101 now provides `include/Save/SCReadValidation.h` with unique-key checks, required block type/size checks, and checksum/basic-domain validation for occupied SWSH/SV/Z-A party/box records. `validateTrainerSaveForOpen()` and durable `validateSCWorkspace()` both call the same semantic layout validator. `tests/test_pla_read_validation.cpp` covers valid layouts plus duplicate keys, missing required blocks, truncation, wrong type, and corrupted Pokémon records, and the target is wired into Host Tests and ASan/UBSan. Exact-head Host Tests are still running and the Native PR Gate is red, so this is not VERIFIED yet.

### 9. AUDIT-017 — FRLG mutable workspace selects rotating slot before checksum validation

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Trainer/Trainer3FRLG.cpp`, `src/Save/GetSaveFileContents.cpp`; contrast with `tests/test_gen3_staged_pokemon_editor.cpp`
- **Concrete failure mode:** the legacy mutable FRLG trainer chooses the active rotating save slot from save counters and sector-id presence only. It does not verify the selected slot's 14 sector checksums before parsing trainer/party/boxes. The generic pre-open validation path special-cases BDSP and PLA but currently returns success for FRLG without running a checksum/slot-recovery gate.
- **Reachable in current product paths:** Current mutable FRLG backup/workspace open path
- **Save-integrity impact:** HIGH: corrupt newest slot may be parsed/edited; original source stays immutable
- **Security / memory-safety impact:** None
- **User-visible impact:** Wrong/corrupt slot can be shown as current save
- **Recommended fix:** reuse the checksum-aware Gen III slot validator/selection policy (or one shared lower-level implementation) before constructing mutable FRLG state. Make FRLG participate in `validateTrainerSaveForOpen()` and keep the original corrupted candidate only as recovery evidence, never auto-repair it on ordinary save.
- **Regression-test strategy:** corrupt newest/valid older slot must select the older valid slot; both invalid slots must refuse open; no mutation may occur merely to make an invalid selected slot checksum-valid.
- **Dependency / sequencing:** Share checksum-aware Gen III slot policy
- **Proposed remediation order:** 9
- **Status:** FIXED

- **Current-code reconciliation:** `Trainer3FRLG::selectActiveSlot()` now uses the shared checksum/signature/counter-aware `Gen3SaveValidation::Detail::validateSlot()` policy and only accepts a checksum-valid FireRed/LeafGreen slot. `validateTrainerSaveForOpen()` now applies the same fail-closed policy before mutable FRLG construction. `tests/test_frlg_mutable_slot_validation.cpp` synthesizes both rotating slots and proves valid-newest selection, corrupt-newest fallback to the older valid slot, and both-corrupt refusal; it is wired into normal Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 10. AUDIT-023 — LGPE durable validator rejects the authentic 1 MiB save image

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Save/GetSaveFileContents.h`, `src/Save/GetSaveFileContents.cpp`, `include/Trainer/Trainer7LGPE.h`
- **Concrete failure mode:** the read/write code correctly treats LGPE `savedata.bin` as a full file containing an active `0xB8800` region plus trailing data, but the durable validator requires `bytes.size() == SAVE_SIZE7_LGPE`, where `SAVE_SIZE7_LGPE` is `0xB8800`. Both the generic workspace builder and the direct LGPE save function preserve the FULL input file and pass that full image to `validateLGPEWorkspace()`.
- **Reachable in current product paths:** Current LGPE durable validation/build path
- **Save-integrity impact:** MEDIUM-HIGH app-owned save availability/integrity boundary
- **Security / memory-safety impact:** None
- **User-visible impact:** Valid 1 MiB LGPE workspace can be rejected
- **Recommended fix:** introduce separate constants for physical LGPE file size (`0x100000`) and active Beluga region size (`0xB8800`). Validate the physical workspace shape, slice only the active region for block/CRC verification, and preserve the trailing region byte-for-byte during build/persist.
- **Regression-test strategy:** full `0x100000` LGPE fixture must validate after block patching while preserving bytes `0xB8800..0xFFFFF` exactly; active-region checksum corruption must fail; define explicitly whether cropped `0xB8800` images are supported or rejected rather than conflating active-region size with physical-file size.
- **Dependency / sequencing:** Do before/with AUDIT-018 to establish correct sizes
- **Proposed remediation order:** 10
- **Status:** FIXED

- **Current-code reconciliation:** the LGPE workspace validator now accepts only the intentional `0xB8800` active-region fixture geometry or the authentic `0x100000` full `savedata.bin` geometry. Full files expose exactly the first active region for the existing block/CRC round-trip check, so trailing bytes are preserved and excluded from Beluga CRC validation. `tests/test_lgpe_workspace_geometry.cpp` proves active/full acceptance, arbitrary intermediate/oversized rejection, and exact active-region slicing; it is wired into normal Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 11. AUDIT-018 — LGPE mutable workspace rewrites CRCs before validating pre-existing block integrity

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Trainer/Trainer7LGPE.cpp`, `src/Save/GetSaveFileContents.cpp`
- **Concrete failure mode:** the LGPE read/open path extracts fixed-offset blocks from a correctly-sized file but does not verify the existing BEEF footer CRC for those blocks before constructing mutable trainer/storage state. The generic pre-open gate does not currently perform LGPE integrity validation.
- **Reachable in current product paths:** Current mutable LGPE backup open path
- **Save-integrity impact:** HIGH app-owned workspace integrity; source immutable
- **Security / memory-safety impact:** None
- **User-visible impact:** Corrupt block can be accepted then rechecksummed on save
- **Recommended fix:** add a read-only LGPE integrity validator that verifies every consumed Beluga block against its existing BEEF footer CRC before constructing `Trainer7LGPE`; invoke it from `validateTrainerSaveForOpen()` and from any direct LGPE load path. Keep rechecksum-on-write only after a valid source has been established.
- **Regression-test strategy:** corrupt one covered LGPE block while leaving the old footer CRC unchanged and require open to fail; verify an untouched valid fixture passes; prove save never repairs an invalid source merely as a side effect of ordinary editing.
- **Dependency / sequencing:** Coordinate with AUDIT-023 size/layout constants
- **Proposed remediation order:** 11
- **Status:** FIXED

- **Current-code reconciliation:** `include/Save/LGPEReadValidation.h` now performs read-only CRC-16/ARC verification for every Beluga block consumed by PokeBank before mutable state exists. `validateTrainerSaveForOpen()` applies this preflight to Let's Go, and the direct `readTrainerInfoLetsGo()` path independently refuses invalid CRCs before constructing `Trainer7LGPE`. The durable validator also shares this preflight. `tests/test_lgpe_read_validation.cpp` proves untouched active/full files pass, covered-block corruption with the old CRC fails, footer corruption fails, and unsupported size fails; it is wired into Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 12. AUDIT-022 — BDSP pre-open validation ignores its stored whole-file MD5

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Save/BDSPReadValidation.h`, `src/Save/GetSaveFileContents.cpp`, `src/Trainer/Trainer8BDSP.cpp`, `tests/test_bdsp_layout_guard.cpp`
- **Concrete failure mode:** BDSP is documented and implemented as a flat blob guarded by a 16-byte whole-file MD5. The open gate and Trainer constructor verify only that the file extends through that hash field; neither compares the stored digest with a computed digest before parsing trainer/party/box data.
- **Reachable in current product paths:** Current BDSP pre-open path
- **Save-integrity impact:** MEDIUM: corrupted flat save can be accepted; writes remain blocked
- **Security / memory-safety impact:** None
- **User-visible impact:** Corrupt BDSP may look valid
- **Recommended fix:** implement a read-only whole-file MD5 verifier using the same zero-the-hash-field convention as `recomputeHash()`, call it from the BDSP pre-open gate and Trainer validity boundary, and add fixture tests before enabling multi-file writeback.
- **Regression-test strategy:** flip one covered byte in a valid BDSP fixture without updating its stored digest and require open refusal; confirm a correct digest passes; confirm the validator itself does not mutate the candidate while checking.
- **Dependency / sequencing:** Independent; keep BDSP writeback disabled
- **Proposed remediation order:** 12
- **Status:** FIXED

- **Current-code reconciliation:** `BDSPReadValidation::wholeFileHashValid()` now copies the candidate, preserves the stored 16-byte digest, zeroes only the copy's hash field, computes MD5 over the complete flat file with the same convention as `Trainer8BDSP::recomputeHash()`, and compares without mutating input. Both `validateTrainerSaveForOpen()` and the `Trainer8BDSP` constructor require a valid stored MD5 before parsing fixed-offset state. `tests/test_bdsp_layout_guard.cpp` now builds a valid digest fixture, proves validation is non-mutating, flips a covered byte to require failure, and asserts both production gates precede parsing; normal and sanitizer targets link the real MD5 implementation. BDSP writeback remains blocked. Exact-head CI is pending, so this is not VERIFIED yet.

### 13. AUDIT-024 — Gen IX inventory decoder leaves persisted flags indeterminate

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Trainer/Inventory9SV.h`, `include/Trainer/Inventory9LZA.h`, `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`, `include/Trainer/Trainer.h`
- **Concrete failure mode:** `InventoryItem9SV item;` / `InventoryItem9LZA item;` default-initialize an aggregate whose inherited `InventoryItem::isNew` and `isFavorite` booleans have no default member initializers. `fromBytes()` decodes the native `flags` word but never maps it into those inherited booleans. The derived object is then copied into `Trainer::items`, whose element type is the base `InventoryItem`, preserving the indeterminate boolean state while discarding the decoded `flags` member.
- **Reachable in current product paths:** Current SV/Z-A inventory decode/write path
- **Save-integrity impact:** HIGH round-trip correctness for app-owned edits
- **Security / memory-safety impact:** MEDIUM uninitialized state + signed-shift UB
- **User-visible impact:** Flags/counts can vary or write back incorrectly
- **Recommended fix:** value-initialize the record (`InventoryItem9SV item{}` / `InventoryItem9LZA item{}`), decode flags into `isNew` and `isFavorite` explicitly, and replace manual signed-shift assembly with `readUInt32LittleEndian()` or `static_cast<uint32_t>(data[n]) << shift`. Add exact byte round-trip tests for both games.
- **Regression-test strategy:** native records with NEW clear/set and FAVORITE clear/set must decode deterministically; parse→write with no edits must be byte-identical; repeated runs under UBSan/ASan-compatible host builds must not depend on stack contents; high-bit 32-bit fields must decode using defined unsigned operations.
- **Dependency / sequencing:** Run under UBSan; coordinate with AUDIT-021 fixtures
- **Proposed remediation order:** 13
- **Status:** FIXED

- **Current-code reconciliation:** `InventoryItem` now has deterministic zero/false defaults. Both `InventoryItem9SV::fromBytes()` and `InventoryItem9LZA::fromBytes()` value-initialize their records, assemble native 32-bit fields with defined unsigned shifts, and map flags bit 0/1 into `isNew`/`isFavorite` before slicing into `Trainer::items`. `tests/test_gen9_inventory_decode.cpp` exercises all NEW/FAVORITE combinations, high-bit pouch/count/flag values, and base slicing for both games; it runs in normal Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 14. AUDIT-039 — Backup save can serialize while held-Pokémon rollback failed

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/UI/TrainerViewScreenBase.inc`
- **Concrete failure mode:** `returnHeldToOrigin()` deliberately plans the entire rollback before moving any carried Pokémon. If no complete destination plan exists, it posts `Could not return the held Pokemon safely - custody retained.` and returns with `moveMon` still owning the carried records. The regular game-save confirmation path calls `returnHeldToOrigin()` immediately before serialization, but it does not inspect a success result and does not check `carrying()` afterward; it can continue through the safety destination check and call `performSave(destDir)`.
- **Reachable in current product paths:** Regular backup save while a carried block cannot be returned
- **Save-integrity impact:** HIGH app-owned backup can serialize with Pokémon omitted
- **Security / memory-safety impact:** None
- **User-visible impact:** Backup reports success while custody remains outside trainer boxes
- **Recommended fix:** make `returnHeldToOrigin()` report success/failure, or explicitly test `carrying()` after the call. If custody remains, abort the save, keep the dialog/session active, and surface the custody-retained error. Add tests for both failed return => no save and successful return => save proceeds.
- **Regression-test strategy:** there is no regression test that exhausts/locks all return destinations, leaves custody active, then attempts a game backup save and proves serialization is refused.
- **Dependency / sequencing:** Fix before backup durability work or in same safety checkpoint
- **Proposed remediation order:** 14
- **Status:** FIXED

- **Current-code reconciliation:** `returnHeldToOrigin()` now returns a success result while preserving its existing all-or-nothing custody plan. Failure leaves `moveMon` untouched and returns false; success clears custody and returns true. The regular game-save confirmation path now requires a successful return and additionally checks `carrying()` before any destination/write logic; otherwise it posts `Save blocked - held Pokemon custody could not be restored.` and returns with the save dialog/session active. `tests/test_storage_custody_contract.cpp` binds the failure-return semantics and proves the custody gate appears before `performSave(destDir)`. The existing test is already part of normal Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 15. AUDIT-015 — Failed backup creation can leave a partial folder surfaced as a backup

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Utils/FileUtilities.cpp`, `src/UI/TrainerViewScreenBase.inc`, `src/UI/BackupSelectionScreen.cpp`, `tests/test_backup_namespace_contract.cpp`, `tests/test_backup_workspace_durability.cpp`
- **Concrete failure mode:** backup creation writes directly into the final destination directory. If any recursive file copy fails, the function returns failure but leaves the partially populated destination directory behind. User-named and timestamped folders are then enumerated as ordinary editable backups solely because they are directories under the scoped game root.
- **Reachable in current product paths:** Current named/timestamped backup creation under I/O failure
- **Save-integrity impact:** HIGH app-owned backup completeness risk; source immutable
- **Security / memory-safety impact:** None
- **User-visible impact:** Partial folder can appear as editable backup
- **Recommended fix:** create backups under a unique temporary/incomplete directory, copy + close/readback/validate the required game file set there, then promote/rename to the final visible backup name only after success. On failure retain evidence under an explicitly non-browsable failed/incomplete name or remove it only when safe. The picker should ignore transaction temp/failed markers and/or require a completed manifest.
- **Regression-test strategy:** ENOSPC/short-write/read/close failure while copying; timestamped and named backup failure cleanup/quarantine; incomplete backup excluded from picker; Working-copy recovery semantics; multi-file backup completeness checks.
- **Dependency / sequencing:** Useful durable temp/promote primitive for AUDIT-012
- **Proposed remediation order:** 15
- **Status:** FIXED

- **Current-code reconciliation:** backup creation now uses `copyDirectoryTransactional()`: every file copy must write, flush/close, and pass byte-for-byte readback inside a unique `.incomplete.*` sibling before promotion. Failed copies are retained only under non-browsable `.failed.*`/`.incomplete.*` evidence names. Reusable `Working` is rotated to `.previous.*` until the completed copy promotes, with rollback on promotion failure; successful old generations are deleted best-effort and remain non-browsable if cleanup fails. Automatic/timestamped and user-named backup creation both use this primitive, while `listBackupDirectories()` excludes all transaction artifact markers. Timestamp collisions receive a distinct workspace name rather than overwriting prior history. `test_backup_namespace_contract.cpp` and `test_backup_workspace_durability.cpp` bind the temp/promote, marker filtering, close/readback, and named/automatic routing contracts. Exact-head CI is pending, so this is not VERIFIED yet.

### 16. AUDIT-029 — Gen II finalization can serialize an in-progress packed move with carried Pokémon omitted

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Integration/Gen2/Gen2StagedEditor.h`, `src/Integration/Gen2/Gen2PackedMove.cpp`, `src/Integration/Gen2/Gen2StagedEditor.cpp`, `src/Integration/Gen2/Gen2ExportTransaction.cpp`
- **Concrete failure mode:** beginning a packed move immediately removes the selected Pokémon from its source box and compacts the staged retail list, while keeping the removed records only in the in-memory `packedMove_.carried` state. `finalizedBytes()` does not test `packedMove_.active`; it repairs checksums/mirrors and strictly reparses the already-compacted bytes. The export transaction then accepts those bytes because the file is structurally valid.
- **Reachable in current product paths:** Gen II staged packed move + export
- **Save-integrity impact:** HIGH app-owned export can omit carried Pokémon
- **Security / memory-safety impact:** None
- **User-visible impact:** Export can silently lose carried records
- **Recommended fix:** make `finalizedBytes()` fail closed whenever `packedMove_.active` is true, with a clear “place or cancel carried Pokémon first” error. Also consider rejecting unrelated staged mutations while a carry is active, matching the Gen III transaction boundary.
- **Regression-test strategy:** begin a packed move with another unrelated pending edit, call `finalizedBytes()` / `publishVerifiedStagedEditorExport()`, and require failure until place or cancel; verify cancel restores byte-identical pre-carry staged state and placement enables finalization.
- **Dependency / sequencing:** Close with AUDIT-030 shared invariant, before UI recovery AUDIT-037
- **Proposed remediation order:** 16
- **Status:** FIXED

- **Current-code reconciliation:** `Gen2::StagedEditor::finalizedBytes()` now fails closed while a packed move is active, with a clear place-or-cancel error before any checksum repair/export bytes are produced. The shared `tests/test_gen12_packed_multimove.cpp` behavior test starts an active group carry, requires finalization failure while custody is out of the packed box list, cancels and proves the pre-carry bytes are restored, then proves finalization succeeds again. Existing successful placement/finalization coverage remains. This is one shared invariant fix across AUDIT-029 and AUDIT-030. Exact-head CI is pending, so this is not VERIFIED yet.\n\n### 17. AUDIT-030 — Gen I finalization can serialize an in-progress packed move

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Integration/Gen1/Gen1StagedPokemonEditor.h`, `src/Integration/Gen1/Gen1PackedMove.cpp`, `src/Integration/Gen1/Gen1StagedPokemonEditor.cpp`
- **Concrete failure mode:** `beginPackedGroupMove()` saves the pre-carry bytes, then implements pickup by calling `stageRemove()` for each selected slot. Those removals are immediately committed into the staged view and rebuild the pending-change list. The carried Pokémon exist only in `packedMove_.beforeBytes` plus source-slot metadata until placement/cancel. `finalizedBytes()` never checks `packedMove_.active`.
- **Reachable in current product paths:** Gen I staged packed move + finalization
- **Save-integrity impact:** HIGH app-owned export can omit carried Pokémon
- **Security / memory-safety impact:** None
- **User-visible impact:** Finalized staged image can miss carried records
- **Recommended fix:** fail closed at the top of `finalizedBytes()` when `packedMove_.active`, and consider blocking unrelated staged mutations while a carry is active. Apply the same transaction invariant to Gen I and II.
- **Regression-test strategy:** begin single/group carry and require `finalizedBytes()` failure before placement/cancel; ensure cancel restores byte-identical pre-carry bytes and pending changes; ensure successful placement allows finalization and preserves exact carried records.
- **Dependency / sequencing:** Close with AUDIT-029 shared invariant
- **Proposed remediation order:** 17
- **Status:** FIXED

- **Current-code reconciliation:** `Gen1::StagedPokemonEditor::finalizedBytes()` now fails closed while a packed move is active, with a clear place-or-cancel error before any checksum repair/export bytes are produced. The shared `tests/test_gen12_packed_multimove.cpp` behavior test starts an active group carry, requires finalization failure while custody is out of the packed box list, cancels and proves the pre-carry bytes are restored, then proves finalization succeeds again. Existing successful placement/finalization coverage remains. This is one shared invariant fix across AUDIT-029 and AUDIT-030. Exact-head CI is pending, so this is not VERIFIED yet.\n\n### 18. AUDIT-037 — Gen I/II group pickup can strand an active staged move after presentation-refresh failure

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/UI/ClassicPackedMoveOverlay.inc`, `src/Integration/Gen1/Gen1PackedMove.cpp`, `src/Integration/Gen2/Gen2PackedMove.cpp`, `src/Legacy/RBYReadOnlyTrainer.cpp`, `src/Legacy/GSCReadOnlyTrainer.cpp`
- **Concrete failure mode:** if the subsequent bridge/presentation refresh fails, the UI clears its multi-select/carry state while the backend still owns an active packed move and staged boxes remain in the post-removal state. A later pickup is rejected as “already being carried”; the normal B-cancel route is no longer reachable through this overlay. The original source remains immutable, but the app-owned working copy can be left missing the carried records until a broader discard/reopen path restores it.
- **Reachable in current product paths:** Gen I/II group pickup when post-pickup refresh fails
- **Save-integrity impact:** MEDIUM-HIGH staged working-copy custody risk; source immutable
- **Security / memory-safety impact:** None
- **User-visible impact:** Backend remains in active carry while UI forgets it
- **Recommended fix:** mirror the single-Pokémon pickup path: on Gen I/II group refresh failure, call `cancelPackedMove()`, refresh presentation again from the restored staged state, preserve the first failure for diagnostics, and only clear UI state after rollback has been attempted. Add a fault-injection contract test for both generations.
- **Regression-test strategy:** backend tests cover begin/place/cancel and failed destination placement, but there is no UI/bridge fault-injection test proving that a failed post-pickup refresh rolls the group transaction back.
- **Dependency / sequencing:** Do after AUDIT-029/030 finalization gate
- **Proposed remediation order:** 18
- **Status:** FIXED

- **Current-code reconciliation:** `beginSelectedGroup()` now treats post-pickup presentation refresh as part of the packed-move transaction for both Gen I and Gen II. If refresh fails after `beginPackedGroupMove()`, it preserves the first refresh failure for diagnostics, calls `cancelPackedMove()`, and refreshes again from the restored staged bytes. If backend cancellation itself fails, the overlay deliberately keeps `state.active`, the attempted generation, carried visuals, and holding presentation so B can retry rather than forgetting backend custody. Existing backend packed-move tests already prove cancel restores pre-carry bytes; `tests/test_classic_packed_move_recovery_contract.cpp` binds both UI rollback paths and the retained-custody fallback and runs in Host Tests/ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 19. AUDIT-013 — Legacy Bank migration skips checksum validation used by normal Bank load

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Trainer/Bank.cpp`, `tests/test_bank_recovery_contract.cpp`, `tests/test_bank_format_policy.cpp`
- **Concrete failure mode:** normal unified Bank loading requires a non-empty Pokémon whose stored checksum equals `calculateChecksum()`. The legacy per-group migration path accepts any record for which `makePokemon(...)` succeeds and `speciesID() != 0`, then places it into the in-memory Bank without the checksum test.
- **Reachable in current product paths:** Reachable only when legacy per-group Bank migration runs
- **Save-integrity impact:** MEDIUM app-owned Bank integrity; legacy sources stay untouched
- **Security / memory-safety impact:** None
- **User-visible impact:** Corrupt legacy slot can appear as migrated Pokémon
- **Recommended fix:** apply the same nonempty + checksum validation predicate before `placeNext`; count/log rejected corrupt migration records separately from capacity overflow.
- **Regression-test strategy:** corrupted-checksum legacy record is skipped, valid record migrates, mixed valid/corrupt legacy records preserve only valid entries, and migration never mutates the legacy source file.
- **Dependency / sequencing:** None
- **Proposed remediation order:** 19
- **Status:** FIXED

- **Current-code reconciliation:** normal unified Bank load and legacy per-group migration now share `Trainer::BankRecordValidation::accept()`, which rejects species 0 and requires stored checksum equality with the calculated checksum. Legacy migration separately counts/logs corrupt rejected records instead of conflating them with capacity drops. `tests/test_bank_record_validation.cpp` proves the acceptance predicate and binds both production paths to the same gate; it is wired into Host Tests and ASan/UBSan. Exact-head CI is pending, so this is not VERIFIED yet.

### 20. AUDIT-028 — Gen VIII/IX base-stat form routing returns zero/wrong rows and can index beyond valid arrays

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Pokemon/BaseStatsGen89.h`, `src/Pokemon/BaseStatsGen89.cpp`, and consumers `src/Pokemon/Pokemon8SWSH.cpp`, `Pokemon8BDSP.cpp`, `Pokemon8LA.cpp`, `Pokemon9SV.cpp`, `Pokemon9LZA.cpp`
- **Concrete failure mode:** the modern base-stat table deliberately places all-zero placeholders in the dense species array for many form-driven species and relies on `getBaseStatsGen89()` to redirect them into dedicated form arrays. That handwritten routing is incomplete and contains wrong offsets/raw-form indexing.
- **Reachable in current product paths:** Affected Gen VIII/IX alternate forms and stat edits
- **Save-integrity impact:** MEDIUM-HIGH data-fidelity risk on edited forms
- **Security / memory-safety impact:** MEDIUM possible out-of-bounds form-array indexing
- **User-visible impact:** Wrong/zero stats on valid forms; malformed form may index badly
- **Recommended fix:** retire the handwritten form-routing table in favor of generated per-game personal/base-stat data using the same form-index redirection model already used by `PersonalInfo`. If a compatibility helper remains, every lookup must first validate `form < formCount` and every dedicated array must have explicit, tested mapping rather than raw-form arithmetic. Preserve SWSH's historical stat differences from later generations through per-game generated data, not ad-hoc post-fixes.
- **Regression-test strategy:** iterate every `PersonalInfo` species/form pair present in each supported game and require a nonzero, correct base-stat record with matching species id; compare representative alternate forms against the corresponding game personal table; run every form under bounds sanitizers; verify stat-affecting edit round-trips for the affected valid forms.
- **Dependency / sequencing:** Prefer generated personal/base-stat routing
- **Proposed remediation order:** 20
- **Status:** OPEN

### 21. AUDIT-027 — LGPE Meltan/Melmetal base-stat rows are unreachable and edits rewrite party stats from base 0

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Pokemon/BaseStatsGen7.h`, `src/Pokemon/BaseStatsGen7.cpp`, `src/Pokemon/Pokemon7LGPE.cpp`, `include/Pokemon/Pokemon7LGPE.h`
- **Concrete failure mode:** the LGPE base-stat table is mostly a dense 0..151 array, then appends Meltan (#808) and Melmetal (#809) as the final two records. The getter does not search by the record's `id`; after regional-form handling it rejects any `speciesId >= BASE_STATS_COUNT_GEN7` and otherwise indexes `BASE_STATS_TABLE_GEN7[speciesId]`. The array count is therefore only the number of rows, not the highest supported species id. Species 808/809 always return the all-zero fallback and the appended rows are unreachable.
- **Reachable in current product paths:** LGPE Meltan/Melmetal editing/stat recalculation
- **Save-integrity impact:** MEDIUM edited-stat correctness
- **Security / memory-safety impact:** None
- **User-visible impact:** Stats/CP can collapse toward zero-base results
- **Recommended fix:** stop treating this sparse table as a dense dex-indexed array. Either special-case/search sparse IDs, split the 0..151 dense table from Meltan/Melmetal, or generate a true id-indexed table large enough for 809. Add explicit compile-time/runtime coverage for 808/809.
- **Regression-test strategy:** `getBaseStatsGen7(808,0)` and `(809,0)` must return the native Meltan/Melmetal rows; editing Level/IV/AV on fixture PB7s must produce the same party stats/CP as PKHeX and must not collapse them toward base-0 results.
- **Dependency / sequencing:** After table/form routing AUDIT-028 if shared generator work helps
- **Proposed remediation order:** 21
- **Status:** OPEN

### 22. AUDIT-014 — Session-wide source read-only gate disables app-owned Bank mutation

- **Severity:** P2
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/UI/TrainerViewScreenBase.h`, `src/UI/TrainerViewScreenBase.inc`, `include/UI/ExactFormatEditorProvider.h`, `tests/test_source_mutation_policy.cpp`
- **Concrete failure mode:** mutation permission is decided from the session's source kind rather than from the actual mutation target. When a RetroArch/DraStic/melonDS/manual or installed source is read-only, the same gate also blocks operations whose target is PokeBank-owned `Bank` storage.
- **Reachable in current product paths:** Current read-only-source + Bank sessions
- **Save-integrity impact:** No source-write risk if fixed correctly; target is app-owned Bank
- **Security / memory-safety impact:** None
- **User-visible impact:** Valid Bank edit/rename/sort blocked
- **Recommended fix:** replace the session-wide mutation gate with a target-aware capability decision. Save/Party/SaveBox targets inherit the source/workspace capability; Bank targets use `SourceKind::AppOwnedStorage`. Keep cross-store True Move source retirement behind its existing transaction/evidence gates and do not turn this cleanup into emulator source writing.
- **Regression-test strategy:** immutable source + mutable Bank target; Bank rename/edit/sort during RetroArch/ExternalLegacy browsing; source-box mutation remains blocked; cross-store source-retiring Move remains blocked; copy/import into Bank must not imply source retirement.
- **Dependency / sequencing:** Must preserve source-retiring Move locks
- **Proposed remediation order:** 22
- **Status:** OPEN

### 23. AUDIT-019 — modern encrypted blank slots are parsed as live species-0 objects

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Trainer/Trainer8SWSH.cpp`, `src/Trainer/Trainer8LA.cpp`, `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`, corresponding Trainer headers, `src/UI/TrainerViewScreenBase.inc`
- **Concrete failure mode:** these parsers decide emptiness from whether the RAW encrypted slot bytes are all zero. Their own write paths document the opposite native fact: a legitimate empty slot is normally a non-zero encrypted blank which decrypts to species 0. The parser therefore constructs live Pokémon objects for native empty slots instead of representing them as empty/null.
- **Reachable in current product paths:** Current SWSH/PLA/SV/Z-A parsing
- **Save-integrity impact:** LOW-MEDIUM logical model correctness
- **Security / memory-safety impact:** None
- **User-visible impact:** Native encrypted blanks become ghost Pokémon objects
- **Recommended fix:** decrypt/validate each slot before deciding occupancy and store only species-nonzero entities in the logical model (or explicitly preserve positional empties as null). Where the save has an authoritative party count, parse and validate it rather than deriving party size from raw slot nonzeroness. Keep species-aware UI occupancy checks as defense in depth.
- **Regression-test strategy:** a native party with two real Pokémon plus four encrypted blanks must parse to two real party entries; native encrypted empty box slots must become null/empty model cells; Y on a visually empty box cell must remain a no-op; round-trip must preserve valid native blank bytes/count semantics.
- **Dependency / sequencing:** Best after layout validators (AUDIT-021)
- **Proposed remediation order:** 23
- **Status:** OPEN

### 24. AUDIT-012 — Settings persistence truncates in place and ignores write/close failure

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/Utils/Settings.cpp`
- **Concrete failure mode:** the authoritative settings file is truncated before the replacement content is known to be complete, and write/close failures are not observed.
- **Reachable in current product paths:** Active Settings save path
- **Save-integrity impact:** Settings only; no Pokémon/save corruption
- **Security / memory-safety impact:** None
- **User-visible impact:** Settings can truncate/disappear on I/O failure
- **Recommended fix:** serialize settings to memory and use the app-owned durable replacement primitive (or an equivalent narrow config transaction), validate/read back the promoted text, and return/report failure.
- **Regression-test strategy:** injected write failure, close failure, interrupted/truncated file recovery, and successful round-trip of all current keys.
- **Dependency / sequencing:** Can reuse durable replace primitive from AUDIT-015 if generalized
- **Proposed remediation order:** 24
- **Status:** OPEN

### 25. AUDIT-040 — RetroArch playlist auto-match can accept wrong-family game content with the same basename

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/UI/GameLauncher.cpp`, `include/UI/GameLaunchModel.h`, `tests/test_game_launch_model.cpp`
- **Concrete failure mode:** manual/stored launch linking calls `gameLaunchContentSupported(gameId, content)` before persisting a game file, and the in-app browser only lists compatible extensions. The automatic RetroArch playlist path instead matches an entry solely by `normalizedLaunchStem(content) == normalizedLaunchStem(sourcePath)`, then checks only that the content path is a regular file and that the playlist supplies a usable core. It never applies `gameLaunchContentSupported(gameId, content)` to the matched playlist entry.
- **Reachable in current product paths:** Current RetroArch playlist auto-match
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Same-stem wrong-family content can launch
- **Recommended fix:** after normalizing the playlist path and before accepting it, require `gameLaunchContentSupported(gameId, content)`; skip incompatible same-stem entries and continue searching. Add a resolver fixture containing both a wrong-family same-stem entry and a correct one.
- **Regression-test strategy:** `test_game_launch_model.cpp` proves the pure extension helper rejects mismatched families (for example Platinum vs a `.gba` file) but no resolver-level test proves RetroArch playlist auto-match actually calls that helper before returning Ready.
- **Dependency / sequencing:** Pair with AUDIT-016 resolver hardening
- **Proposed remediation order:** 25
- **Status:** OPEN

### 26. AUDIT-016 — RetroArch launch matching is basename-only and first-match wins

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/UI/GameLauncher.cpp`, `include/UI/GameLaunchModel.h`, `tests/test_game_launch_model.cpp` on sibling UI PR #97
- **Concrete failure mode:** the resolver reduces the validated save path and each playlist content path to an alphanumeric basename stem, then returns the first playlist entry whose stem matches. Directory, extension, core, exact game identity and content hash are not part of the match key.
- **Reachable in current product paths:** Current RetroArch launch resolver
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Ambiguous same-basename playlist can launch wrong game
- **Recommended fix:** gather all viable matches first. If exactly one remains, launch it. If multiple remain, require an explicit stored content binding or chooser keyed by exact source/game identity; never pick by directory iteration order.
- **Regression-test strategy:** two valid playlist entries with the same normalized stem but different paths; ambiguity must not silently select one.
- **Dependency / sequencing:** Pair with AUDIT-040 in one resolver tranche if tests prove shared root
- **Proposed remediation order:** 26
- **Status:** OPEN

### 27. AUDIT-043 — Device-observed Gen IV save rows can lose trainer-name presentation despite synthetic parser coverage

- **Severity:** P3
- **Confidence:** DEVICE-OBSERVED / CODE-PATH CONFIRMED, root cause not yet isolated
- **Affected files / subsystem:** `src/Integration/Gen4/Gen4SourceDiscovery.cpp`, `src/Integration/Gen4/Gen4ReadOnlySave.cpp`, `src/Legacy/Gen4ReadOnlyTrainer.cpp`
- **Concrete failure mode:** Device-observed Gen IV save rows can lose trainer-name presentation despite synthetic parser coverage
- **Reachable in current product paths:** Device-observed Gen IV Save Instances/trainer presentation
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Real save can show no trainer name despite valid content
- **Recommended fix:** retain a disposable failing Gen IV source, trace selected General partition/layout/language/name bytes through `decodeGen4Field`, and add a regression fixture from the failing structure (sanitized if needed). Do not paper over the failure by substituting the filename as trainer name.
- **Regression-test strategy:** Add a focused regression reproducing the original failure, then run the nearest broader host/native suite.
- **Dependency / sequencing:** Evidence-first; requires failing disposable fixture before code change
- **Proposed remediation order:** 27
- **Status:** OPEN
- **Evidence gate:** do not patch by filename fallback. First preserve/reproduce the failing real Gen IV layout and convert it into a sanitized regression fixture.

### 28. AUDIT-033 — Gen IV move-stat presentation uses HGSS values for Diamond/Pearl

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/Names/MoveBattleStats.h`, `include/UI/MovePickerPresentation.h`, `tools/gen_move_battle_stats.py`, `src/UI/Gen4SharedPokemonSurface.inc`
- **Concrete failure mode:** the generator sources one Generation IV move table exclusively from pinned `pret/pokeheartgold` and `getMoveBattleStats()` returns that same table for Diamond/Pearl, Platinum and HGSS. Generation IV is not fully uniform across those releases.
- **Reachable in current product paths:** Diamond/Pearl Gen IV move-stat display
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Displayed Acc/Pwr/PP can be wrong (e.g. Hypnosis)
- **Recommended fix:** generate distinct DP and Pt/HGSS move-stat tables (or patch the known intra-generation differences explicitly), dispatch by exact game/version rather than only `GameVersion::DP/PT/HGSS` to one common array, and add a Hypnosis regression asserting DP=70 and Pt/HGSS=60.
- **Regression-test strategy:** Add a focused regression reproducing the original failure, then run the nearest broader host/native suite.
- **Dependency / sequencing:** Gen IV correctness tranche
- **Proposed remediation order:** 28
- **Status:** OPEN

### 29. AUDIT-034 — HD sprite recovery/preflight can accept corrupt existing PNGs

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `tools/gen_hdsprites.py`, `tools/check_device_assets.py`, `tools/recover_workspace.py`; final packaging consumes the preflight in `tools/package_device_build.py`.
- **Concrete failure mode:** an existing HD sprite is considered reusable by `gen_hdsprites.py` solely because its path exists. Recovery decides the HD tree is complete primarily from the number of `.png` filenames. The device asset preflight likewise builds a filename set and checks required names; it reports directory size totals but does not require each PNG to be non-empty or decodable.
- **Reachable in current product paths:** Recovery/package asset pipeline
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Corrupt PNG can pass preflight into hardware package
- **Recommended fix:** validate every required HD asset as a non-empty decodable PNG (at minimum PNG signature + dimensions; preferably Pillow verification where available), and have recovery regenerate any invalid file rather than using filename count as completeness. The final preflight should independently repeat the validity check before packaging.
- **Regression-test strategy:** zero-byte required HD sprite; non-PNG bytes under a `.png` name; truncated PNG; recovery with correct filename count but one corrupt required file; preflight must fail all of them.
- **Dependency / sequencing:** Tooling tranche
- **Proposed remediation order:** 29
- **Status:** OPEN

### 30. AUDIT-035 — personal/learnset regeneration still searches the removed SPECIES_NAMES symbol

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `tools/gen_personal.py`, `tools/gen_learnsets.py`, `tools/gen_speciesnames.py`, `src/Names/SpeciesNames.cpp`, `tools/regenerate.py`
- **Concrete failure mode:** `gen_personal.py` and `gen_learnsets.py` still parse the checked-in species source with the regex `SPECIES_NAMES\[\]\s*=...`. The multilingual species-name generator no longer emits that symbol: it emits `SPECIES_NAMES_EN[]` (plus the other eight language arrays) and `SPECIES_NAMES_BY_LANGUAGE[]`.
- **Reachable in current product paths:** Generated data refresh tooling
- **Save-integrity impact:** Indirect future data drift risk
- **Security / memory-safety impact:** None
- **User-visible impact:** Regeneration fails after species-symbol refactor
- **Recommended fix:** make both generators consume the shared language helper/current English symbol explicitly (or better, consume PKHeX's source species resource directly rather than reparsing another generated C++ file). Update the asset preflight's parser at the same time. Add a regeneration smoke test so future symbol refactors cannot silently break the generator chain.
- **Regression-test strategy:** run every discovered table generator against the current tree in a pinned/offline-cache CI job; at minimum invoke both `gen_personal.py` and `gen_learnsets.py` after regenerating species names and require successful no-drift output.
- **Dependency / sequencing:** Tooling tranche; pair with generated-data CI
- **Proposed remediation order:** 30
- **Status:** OPEN

### 31. AUDIT-044 — Fontstash allocation failures can become null-pointer crashes during text rendering

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `nanovg/fontstash.h`, `nanovg/nanovg.c`
- **Concrete failure mode:** (1) `fons__allocGlyph()` returns `NULL` if its glyph-array `realloc` fails, but `fons__getGlyph()` immediately writes through that pointer without checking it. (2) `fonsResetAtlas()` assigns `realloc()` directly back to `stash->texData` and returns 0 on failure, while NanoVG's `nvg__allocTextAtlas()` ignores that return and reports success before the text iterator retries against the failed atlas state.
- **Reachable in current product paths:** Text rendering under allocation failure
- **Save-integrity impact:** None
- **Security / memory-safety impact:** LOW-MEDIUM null-deref availability crash only
- **User-visible impact:** App can crash under memory pressure
- **Recommended fix:** check the `fons__allocGlyph()` result before dereference; preserve the old atlas pointer across `realloc`; propagate `fonsResetAtlas()` failure through `nvg__allocTextAtlas()`; and let the existing higher-level text paths skip/fail the draw. Preserve vendored-source attribution/patch marking.
- **Regression-test strategy:** no constrained-allocation/fault-injection regression covers Fontstash glyph-cache growth.
- **Dependency / sequencing:** Bounded vendor/call-boundary fix only; no renderer rewrite
- **Proposed remediation order:** 31
- **Status:** OPEN

### 32. AUDIT-006 — LeakSanitizer is disabled even where comments say CI keeps it enabled

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `Makefile.host.base`
- **Concrete failure mode:** ASan/UBSan still provide useful coverage, but leak detection is absent and the comment overstates the gate.
- **Reachable in current product paths:** Active host sanitizer target
- **Save-integrity impact:** None directly
- **Security / memory-safety impact:** Leak detection gap only
- **User-visible impact:** Leaks can escape CI
- **Recommended fix:** either enable leak detection in Linux CI with an environment override, or correct the comment and add a separate leak-capable job if practical.
- **Regression-test strategy:** Run the affected workflow/target on the exact remediation head and assert the intended gate executes.
- **Dependency / sequencing:** After higher-risk memory fixes; then enable/clarify LSan
- **Proposed remediation order:** 32
- **Status:** OPEN

### 33. AUDIT-003 — mutable native toolchain image

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** multiple native workflows
- **Concrete failure mode:** identical source SHAs can build differently after the image advances.
- **Reachable in current product paths:** Active native CI/release builds
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Non-reproducible build output across time
- **Recommended fix:** pin a tested digest/version for candidate/release builds and update deliberately.
- **Regression-test strategy:** Run the affected workflow/target on the exact remediation head and assert the intended gate executes.
- **Dependency / sequencing:** After core correctness; coordinate with CI pin update
- **Proposed remediation order:** 33
- **Status:** OPEN

### 34. AUDIT-036 — Product Home Help overlay advertises stale controller actions

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/UI/SaveSelectScreen.cpp`
- **Concrete failure mode:** the reachable Help overlay opened with Minus still says `L / R   Previous or next Switch user` and `B   Return to the PokeBank NX Main Menu`. Those instructions describe an older navigation model and contradict the live handler on the same screen.
- **Reachable in current product paths:** Current MAIN help copy now corrected by newer UI merge
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Previously told users wrong controls
- **Recommended fix:** make Help use `ZL` for profile switching, `L/R` for game switching, and `B` for Exit; add a contract assertion tying the visible Help strings to those current actions.
- **Regression-test strategy:** `tests/test_game_hub_contract.py` proves that L/R exists for game switching but does not assert that Help copy matches the handler/nav hint, so this drift can remain test-green.
- **Dependency / sequencing:** Verification-only; add contract assertion
- **Proposed remediation order:** 34
- **Status:** FIXED
- **Current-code reconciliation:** live MAIN now displays `L / R Previous / next game` and `B Exit PokeBank NX from Product Home`; a dedicated help-copy contract is still missing, so this is not VERIFIED yet.

### 35. AUDIT-011 — Search preview vertical wrap changes columns and the test blesses it

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `include/UI/OrganizationPreviewModel.h`, `tests/test_organization_preview_model.cpp` on sibling UI PR #97
- **Concrete failure mode:** Search renders seven filters in a two-column grid. Vertical movement adds/subtracts the column count and then wraps the flat index modulo seven. Because seven is not divisible by two, vertical wrapping changes columns: index 6 (bottom-left) + Down becomes index 1 (top-right); Up from index 0 similarly lands on index 5 (bottom-right).
- **Reachable in current product paths:** Current Product UI model in MAIN
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Controller wrap jumps columns unexpectedly
- **Recommended fix:** perform row/column navigation geometrically. On vertical wrap, preserve the current column and choose the nearest valid row entry; for the one-item final Search row, bottom-left should wrap to top-left. Update the regression expectations accordingly.
- **Regression-test strategy:** column-preserving wrap for incomplete rows, including Search 0 + Up and 6 + Down.
- **Dependency / sequencing:** UI tranche; current MAIN reproduces it
- **Proposed remediation order:** 35
- **Status:** OPEN

### 36. AUDIT-038 — Legacy Save Instances scroll logic assumes one more visible row than the renderer draws

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `src/UI/SaveSelectScreen.cpp`
- **Concrete failure mode:** the Legacy Save Instances draw path declares `visibleRows = 5` and passes that value to `drawSaveInstanceRows()`, so only five save rows are rendered. The matching `Overlay::LegacyInstances` input path separately declares `visibleRows = 6` when deciding whether to advance `legacyInstanceScroll`.
- **Reachable in current product paths:** Current Legacy Save Instances with >5 rows
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Focused sixth row can be off-screen
- **Recommended fix:** define the visible-row count once and share it between draw and input, or derive scrolling from the same layout constant used by the renderer. Add a navigation test that walks across the fifth/sixth-row boundary and asserts the focused row remains rendered.
- **Regression-test strategy:** no contract currently binds the Legacy Save Instances renderer's visible-row count to the input scroll-window count.
- **Dependency / sequencing:** UI navigation tranche
- **Proposed remediation order:** 36
- **Status:** OPEN
- **Current-code reconciliation:** still CURRENT at `4bdb82db247f9d0d6bc4393624c40db32a923b40`: input uses six visible rows while the renderer draws five.

### 37. AUDIT-041 — Standalone runtime contract falsely says RetroArch is never invoked

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `docs/STANDALONE_RUNTIME.md`, `src/UI/GameLauncher.cpp`
- **Concrete failure mode:** the active standalone-runtime contract classifies RetroArch only as an external SAVE SOURCE and states `RetroArch is not invoked as a helper`. Current MAIN deliberately supports RetroArch game shortcuts: `GameLauncher` resolves the known RetroArch executable/core, prepares content, and requests launch from Product Home.
- **Reachable in current product paths:** Active runtime documentation
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Security/support docs falsely say RetroArch is never invoked
- **Recommended fix:** update the runtime classification to distinguish `RetroArch save discovery = external read-only source` from `RetroArch executable = optional user-invoked game-launch target`; retain the statement that PokeBank NX does not require RetroArch for normal standalone operation.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** Docs tranche after launch fixes
- **Proposed remediation order:** 37
- **Status:** OPEN

### 38. AUDIT-010 — canonical engineering authority chain points to obsolete work

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `docs/PROJECT_RESOURCE_INDEX.md`, `docs/RESEARCH_CURRENT_INDEX.md`, `docs/CODEX_SESSION.md`, `docs/NEXT_CODEX_PROMPT.md`, `docs/NEXT_SESSION_PLAN.md`, `docs/RECOVERY_CONTRACT.md`
- **Concrete failure mode:** unlike dated reports explicitly kept as history, these files advertise themselves as the fast/current authority path. A fresh agent or contributor following repository instructions can resume an obsolete milestone or write to an obsolete branch.
- **Reachable in current product paths:** Active engineering handoff docs
- **Save-integrity impact:** Indirect workflow risk if stale branch is followed
- **Security / memory-safety impact:** None
- **User-visible impact:** Agents/contributors can resume obsolete lane
- **Recommended fix:** keep one tiny live handoff/authority file that starts by re-fetching GitHub and records only the current integration hierarchy; move obsolete prompts to `docs/history/` or mark them historical at the top; make recovery branch-agnostic where possible.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** Refresh after functional remediation order is established
- **Proposed remediation order:** 38
- **Status:** OPEN

### 39. AUDIT-042 — Active session/roadmap docs route work through obsolete project state

- **Severity:** P3
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `docs/SESSION_RUNBOOK.md`, `docs/V1_ROADMAP.md`, `docs/UPSTREAM_AUDIT.md`, `docs/RETROARCH_SOURCE_NAMING.md`, `docs/UI_FLOW.md`, `docs/UI_STYLE_GUIDE.md`, `docs/UI_OWNERSHIP_STATUS.md`
- **Concrete failure mode:** these non-archive documents still present old state as current: the runbook instructs work and pushes on `feature/pokebank-playable`; the v1 roadmap says Gen II is current/hardware-pending and Gen III is next; the upstream audit's current-context block still describes the early 23-identity/second-device stage; the RetroArch naming note describes the immediate FRLG blocker/current FRLG-only production scope; `UI_FLOW.md` still describes Home primarily as a source-card list and its old top-level Vault shell; `UI_STYLE_GUIDE.md` still treats the pre-Product-Home Select Game grid/source-badge design as the target Home direction; and `UI_OWNERSHIP_STATUS.md` still describes Home/Game selection as mixed/incomplete. Live GitHub instead has the active MAIN development lane in PR #92 at `fb7b1d8c…`, device-accepted Gen I–III work, provider-neutral Save Instances, active Gen IV work, and integrated Product Home/launch UI.
- **Reachable in current product paths:** Active runbook/roadmap/UI docs
- **Save-integrity impact:** Indirect workflow risk
- **Security / memory-safety impact:** None
- **User-visible impact:** Agents/contributors can follow obsolete project state
- **Recommended fix:** either refresh the live-state/routing sections to current lanes and milestones or explicitly mark the files historical/reference-only and point to the current authority. Preserve dated research conclusions separately from mutable project status.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** Docs tranche late
- **Proposed remediation order:** 39
- **Status:** OPEN

### 40. AUDIT-004 — stale theme choices in physical bug template

- **Severity:** P4
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `.github/ISSUE_TEMPLATE/device_bug_report.md`
- **Concrete failure mode:** it only lists `OLED Black / Dark / Light` even though current builds support additional themes.
- **Reachable in current product paths:** Issue-report workflow only
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Users see stale theme choices
- **Recommended fix:** make the field free-form or list the current set.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** None
- **Proposed remediation order:** 40
- **Status:** OPEN

### 41. AUDIT-005 — historical branch-specific workflows remain tracked

- **Severity:** P4
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** old Gen I/II/III candidate/retest/recovery workflows
- **Concrete failure mode:** several jobs are permanently tied to `feature/pokebank-playable` or frozen historical SHAs while live work now flows through PR #79 → #90 with #92/#97 overlays.
- **Reachable in current product paths:** Tracked CI workflow set; mostly historical/manual
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Contributor/maintenance confusion
- **Recommended fix:** preserve historical evidence in docs/releases, then retire or clearly mark historical/manual workflows.
- **Regression-test strategy:** Run the affected workflow/target on the exact remediation head and assert the intended gate executes.
- **Dependency / sequencing:** After active CI remediation
- **Proposed remediation order:** 41
- **Status:** OPEN

### 42. AUDIT-007 — top-level README materially understates current Gen IV implementation

- **Severity:** P4
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `README.md`
- **Concrete failure mode:** the public front page gives contributors/users the wrong current support boundary.
- **Reachable in current product paths:** Public README
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Users/contributors see stale Gen IV support state
- **Recommended fix:** refresh only the human-facing current-status sections after the active Gen IV lane reaches the intended documentation checkpoint.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** Do late after remediation state stabilizes
- **Proposed remediation order:** 42
- **Status:** FIXED
- **Current-code reconciliation:** live MAIN now states active Gen IV Diamond/Pearl/Platinum/HeartGold/SoulSilver staged Party/Box editing, Create, native fields, move/form support, checksum repair, strict reparse and rollback. That current human-facing README was synced unchanged into PR #101 while preserving remediation history. Documentation consistency still needs exact-head review before VERIFIED.

### 43. AUDIT-008 — checked-in Visual Studio metadata is stale PKSE-era configuration

- **Severity:** P4
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `CppProperties.json`, `PKSE.sln`, `PKSE.vcxproj`, `PKSE.vcxproj.filters`
- **Concrete failure mode:** the files are vestigial/IDE-facing rather than the authoritative build, but they can mislead contributors and produce incorrect IDE diagnostics/up-to-date expectations.
- **Reachable in current product paths:** Developer IDE metadata only
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Misleading IntelliSense/project expectations
- **Recommended fix:** either regenerate/rename the VS metadata for PokeBank NX or remove it and document the supported editor setup.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** Cleanup-only
- **Proposed remediation order:** 43
- **Status:** OPEN

### 44. AUDIT-009 — recovery metadata still describes the repository as private / old production branch

- **Severity:** P4
- **Confidence:** CONFIRMED
- **Affected files / subsystem:** `recovery/RECOVERY_STATE.json`, `recovery/assets_snapshot/README.md`
- **Concrete failure mode:** recovery bytes themselves remain pinned and usable, but operational instructions are stale.
- **Reachable in current product paths:** Recovery docs/metadata only
- **Save-integrity impact:** None
- **Security / memory-safety impact:** None
- **User-visible impact:** Recovery operator can follow stale branch/privacy instructions
- **Recommended fix:** separate immutable historical snapshot identity from current recovery/publishing instructions.
- **Regression-test strategy:** Docs/source consistency check against current branch hierarchy and supported behavior; no runtime claim beyond reviewed text.
- **Dependency / sequencing:** Docs cleanup
- **Proposed remediation order:** 44
- **Status:** OPEN

## Checkpoints

- **Checkpoint A:** all P1 findings VERIFIED (or fixed historical item verified) and all high-risk memory/parser/source-boundary findings needed to make further remediation safe.
- **Checkpoint B:** all P2 findings VERIFIED.
- **Checkpoint C:** all P3/P4 findings VERIFIED or explicitly DEFERRED WITH JUSTIFICATION.
- Final exit requires full Host Tests, ASan, UBSan, relevant generation/editor tests, relevant native/devkitA64 builds, exact final application/tree SHAs, and no weakened safety invariant.
