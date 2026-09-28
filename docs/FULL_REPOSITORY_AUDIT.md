# Full Repository Forensic Audit

Status: IN PROGRESS

## Exact state

- Repository: GlitchedZeus/PokeBank-NX
- Audit branch: `audit/full-repository-line-by-line-20260928`
- Primary MAIN tree audited: PR #92 head `dc8a64158f2ebaf22a7b456e52eb8ee320e9c1f4`
- PR #92 branch: `feature/gen4-full-editor-20260928`
- Sibling UI overlay: PR #97 head `1a59feeb10b7826f945f19df171661740d723697` (delta will be audited separately)
- Integration parent: PR #90 head `8b3bcc16c804247bfe8d1314b686974ce73051d8`
- Hardening parent: PR #79 head `00ee7a6ed7ac1b5a93c43246d70c252e135acec0`
- Default branch main: `aca2bf41c83d81084886a46d53195f6cead81ccc`

## Inventory

- Git tree truncated: false
- Tracked non-directory paths: 725
- Text/unknown candidates: 692
- Binary candidates: 32
- Symlinks: 0
- Submodule entries: 1
- Audit ledger: `docs/FULL_REPOSITORY_AUDIT_LEDGER.tsv`

## Coverage

- Audited tracked paths: 84 / 725
- Fully read text files: 50 / 692 (classification still provisional)n provisional until content inspection)
- Binary/non-text inspected: 34 / 34 currently identified by exact extension/manifest scan

## Findings

No findings are recorded here until supported by direct evidence from the frozen tree or active-overlay delta.


## Confirmed findings — repository metadata / CI

### AUDIT-001 — destructive mutable PKSE import workflow
- Severity: P1
- Confidence: CONFIRMED
- Area: CI / repository integrity
- File: `.github/workflows/import-pkse.yml`
- Evidence: `contents: write`; trigger on a push to `main` changing this workflow; fetches unpinned `pkse/master`; runs `git checkout "$upstream_sha" -- .`; commits and pushes to `main`. The workflow calls this “PKSE 1.1.3” without pinning a 1.1.3 tag/SHA.
- Why it matters: editing this workflow can replace the PokeBank NX default-branch tree with whatever PKSE master contains at run time.
- Recommended fix: retire it after the historical import, or make it manual/read-only and pin an immutable upstream revision. Never overlay upstream onto `.` and push directly to `main`.
- Owner: dedicated cleanup / MAIN.

### AUDIT-002 — no general unfiltered native PR compile gate
- Severity: P2
- Confidence: HIGH
- Area: CI / Switch-native coverage
- Files: `.github/workflows/**`
- Evidence: `host-tests.yml` is unfiltered across PRs but host-only. devkitA64 jobs are branch/path scoped or historical/manual; shared native-sensitive files outside those path filters can change without a Switch compile/link job.
- Why it matters: a host-green PR can still break libnx/device-only compilation or packaging.
- Recommended fix: add one broad PR native compile/link gate; keep specialized filtered jobs as supplemental checks.
- Owner: MAIN / CI.

### AUDIT-003 — mutable native toolchain image
- Severity: P3
- Confidence: CONFIRMED
- Area: CI reproducibility
- Files: multiple native workflows
- Evidence: builds use `devkitpro/devkita64:latest` rather than an immutable digest.
- Why it matters: identical source SHAs can build differently after the image advances.
- Recommended fix: pin a tested digest/version for candidate/release builds and update deliberately.
- Owner: MAIN / CI.

### AUDIT-004 — stale theme choices in physical bug template
- Severity: P4
- Confidence: CONFIRMED
- Area: docs/templates
- File: `.github/ISSUE_TEMPLATE/device_bug_report.md`
- Evidence: it only lists `OLED Black / Dark / Light` even though current builds support additional themes.
- Recommended fix: make the field free-form or list the current set.
- Owner: docs-only / UI-QoL.

### AUDIT-005 — historical branch-specific workflows remain tracked
- Severity: P4
- Confidence: CONFIRMED
- Area: CI maintainability
- Files: old Gen I/II/III candidate/retest/recovery workflows
- Evidence: several jobs are permanently tied to `feature/pokebank-playable` or frozen historical SHAs while live work now flows through PR #79 → #90 with #92/#97 overlays.
- Recommended fix: preserve historical evidence in docs/releases, then retire or clearly mark historical/manual workflows.
- Owner: dedicated cleanup.


### AUDIT-006 — LeakSanitizer is disabled even where comments say CI keeps it enabled
- Severity: P3
- Confidence: CONFIRMED
- Area: test/sanitizer coverage
- File: `Makefile.host.base`
- Exact symbol: `host-sanitize`
- Evidence: every sanitizer executable is launched with `ASAN_OPTIONS=detect_leaks=0`. The adjacent comment says leak checks “remain enabled in unrestricted CI builds,” but the reviewed CI workflows invoke this same target and therefore inherit `detect_leaks=0`.
- Why it matters: ASan/UBSan still provide useful coverage, but leak detection is absent and the comment overstates the gate.
- Recommended fix: either enable leak detection in Linux CI with an environment override, or correct the comment and add a separate leak-capable job if practical.
- Owner: MAIN / CI.

### AUDIT-007 — top-level README materially understates current Gen IV implementation
- Severity: P4
- Confidence: CONFIRMED
- Area: public documentation
- File: `README.md`
- Evidence: it is dated 2026-09-27 and still labels Gen IV as a read-only preview with editing/Create disabled and hardware smoke pending, while current authoritative status documents and PR #92 describe the later G4-03 accepted staged View/Edit milestone and active G4-04 Create/editor work.
- Why it matters: the public front page gives contributors/users the wrong current support boundary.
- Recommended fix: refresh only the human-facing current-status sections after the active Gen IV lane reaches the intended documentation checkpoint.
- Owner: docs-only / MAIN.

### AUDIT-008 — checked-in Visual Studio metadata is stale PKSE-era configuration
- Severity: P4
- Confidence: CONFIRMED
- Area: developer tooling / stale code metadata
- Files: `CppProperties.json`, `PKSE.sln`, `PKSE.vcxproj`, `PKSE.vcxproj.filters`
- Evidence: the project remains named `PKSE`, expects `PKSE.nro` while the Makefile target is `PokeBankNX`, carries `PKSE_VERSION="1.1.3"` in IntelliSense configuration, hard-codes a devkitA64 GCC 16.1.0 Windows include layout, and enumerates an old subset of sources that omits the newer Gen I/II/IV integration tree.
- Why it matters: the files are vestigial/IDE-facing rather than the authoritative build, but they can mislead contributors and produce incorrect IDE diagnostics/up-to-date expectations.
- Recommended fix: either regenerate/rename the VS metadata for PokeBank NX or remove it and document the supported editor setup.
- Owner: dedicated cleanup.

### AUDIT-009 — recovery metadata still describes the repository as private / old production branch
- Severity: P4
- Confidence: CONFIRMED
- Area: recovery documentation
- Files: `recovery/RECOVERY_STATE.json`, `recovery/assets_snapshot/README.md`
- Evidence: recovery state is dated 2026-09-10 and points at `feature/pokebank-playable`; the snapshot README states “The project repository is private” and instructs pushing recovery material to that historical branch. The repository is currently public and active development has moved to the #79/#90/#92/#97 hierarchy.
- Why it matters: recovery bytes themselves remain pinned and usable, but operational instructions are stale.
- Recommended fix: separate immutable historical snapshot identity from current recovery/publishing instructions.
- Owner: docs-only / dedicated cleanup.

## Completed build-system checks at this checkpoint
- The native Makefile directory wildcard covers all 148 tracked production `.cpp/.c/.s` files under `src/` and `nanovg/`.
- The 94 tracked C/C++ files outside those native source directories are tests; no production translation unit was found silently omitted from the Switch source list.
- All 32 tracked image assets were inspected by decoded binary signature and dimensions. Game-card PNG Git blob IDs match the manifest where listed; the icon is a valid 256x256 JPEG; all 14 screenshots are valid 1280x720 JPEGs; the banner is a valid 848x208 PNG.
- The two 80-MiB-class recovery tar parts are accounted for indirectly through the tracked manifest: their tree sizes match the manifest and the manifest records SHA-256 for each part and the reconstructed archive.
