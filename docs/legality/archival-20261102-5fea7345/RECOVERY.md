# PokeBank NX legality evidence recovery — 2026-11-02

This is a development/evidence archive, not an application release or release-acceptance certificate. No engine changes, application merge, device build, original-save write, branch deletion or history rewrite was performed during archival.

## Durable anchors

- Repository: https://github.com/GlitchedZeus/PokeBank-NX
- PR #314 engine: `2144a6820490fa81c932dd2d917b5153283feefc`.
- Evidence tag: `evidence/gen1-5-2144a682-20261102`, pointing directly to that commit.
- Evidence release: https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/evidence/gen1-5-2144a682-20261102
- Engine development branch: `claude/gen1-5-legality-campaign-w9mkom`. Verify its current head before doing development; the archive is intentionally frozen at 2144a682.
- Older independent audit snapshot: branch `audit/gen12-independent-20261013`, commit `2a681f2c` (full SHA in archival index). This preserves old QA findings and intentionally failing acceptance cases; it is not a current production fix or current green-CI assertion.
- `archive/local-reflog-20261102/*` tags preserve 61 orphaned reflog commits. Each was verified as an existing public GitHub commit before being pinned. See `ORPHAN_BACKUP_REFS.json` and `ORPHAN_REMOTE_REFS_VERIFIED.json`.

## Install host dependencies on EndeavourOS / Arch

Review the package transaction, then install the host toolchain:

```bash
sudo pacman -Syu --needed base-devel clang git python github-cli nlohmann-json curl icu krb5 libunwind openssl zlib
```

Official package references: https://archlinux.org/packages/extra/x86_64/clang/ and https://archlinux.org/packages/extra/any/nlohmann-json/ . Actual archival host versions are in `SOFTWARE_SOURCE_CORPUS.v1.json`; using current packages reproduces functionality, not necessarily identical compiler output.

Runtime dependency reference: https://archlinux.org/packages/extra/x86_64/dotnet-runtime/ . The SDK archive supplies its own dotnet host; the system libraries above cover the runtime dependencies.

The reference oracle requires .NET 10. Install SDK `10.0.100` into a new local dependency directory using Microsoft's version-pinned installer (https://learn.microsoft.com/en-us/dotnet/core/tools/dotnet-install-script):

```bash
mkdir -p legality-recovery-deps
curl -fL https://dot.net/v1/dotnet-install.sh -o legality-recovery-deps/dotnet-install.sh
bash legality-recovery-deps/dotnet-install.sh --version 10.0.100 --install-dir "$PWD/legality-recovery-deps/dotnet"
export PATH="$PWD/legality-recovery-deps/dotnet:$PATH"
dotnet --version
```

No Switch SDK, ROM or privately acquired save is needed for the host regression and evidence-recovery checks below. Device application acceptance remains separate work.

## Restore the exact source, independently of existing local worktrees

Use a new directory:

```bash
git clone --recurse-submodules https://github.com/GlitchedZeus/PokeBank-NX.git PokeBank-NX-recovered
cd PokeBank-NX-recovered
git checkout --detach evidence/gen1-5-2144a682-20261102
git submodule update --init --recursive
test "$(git rev-parse HEAD)" = 2144a6820490fa81c932dd2d917b5153283feefc
git submodule status --recursive
```

Pinned dependencies: PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`, memecrypto `bd3b16ee2c12bd9ee353cf3098cb998e7880638c`, pcg-cpp `428802d1a5634f96bcd0705fab379ff0113bcf13`.

## Download and check the evidence

In a new directory outside the checkout (run `gh auth login` if the CLI requests it; published assets also have public download URLs in the manifest/release page):

```bash
mkdir recovered-assets
gh release download evidence/gen1-5-2144a682-20261102 --repo GlitchedZeus/PokeBank-NX --dir recovered-assets
cd recovered-assets
sha256sum -c SHA256SUMS.v2.txt
```

`ARCHIVE_MANIFEST.v2.json.gz` maps each original path, byte count and SHA-256 to an asset and content-addressed member. The repository archival index pins the manifest checksum. Read `VERIFICATION_REPORT.v2.json` for the actual remote asset IDs/digests, independent full download hashes and recovered-content checks. Do not infer successful recovery from an upload's exit status.

Parts01–08, part09 (immutable witnesses) and part10 (additional source/build/probe files) preserve the broader historical evidence and immutable witnesses. They are independent gzip tar files, not byte-split fragments; do not concatenate them. Restore paths through the supplied helper into a **new** destination:

```bash
gzip -dc ARCHIVE_MANIFEST.v2.json.gz > ARCHIVE_MANIFEST.v2.json
python3 restore_evidence_v4.py ARCHIVE_MANIFEST.v2.json . ../restored-project-evidence
```

For a smaller initial restore, use a prefix:

```bash
python3 restore_evidence_v4.py ARCHIVE_MANIFEST.v2.json . ../restored-final-evidence --prefix outputs/legality-major-20261101/
```

The helper verifies each used asset and each content-addressed payload, refuses existing destination paths and recreates available safe relative aliases. For source-checkout directory aliases, pass `--source-checkout /absolute/path/PokeBank-NX-recovered`; the helper verifies that checkout is exactly the archived engine anchor. If an archived file alias points outside the selected restore prefix, its backed-up bytes are materialized instead. It keeps a `.blobs` directory as part of the restored evidence; no cleanup is performed.

The separate `legality-major-2144a682-PART_B-RECONSTITUTED.tar.gz` contains all 889 payload names from the original Part B. Its archive SHA is `4ec41ab2a2ff883d0510ffb9f96126837c3da3146a111b7db45cd3fa40f590ba`. Its **original manifest** SHA matches `4e79740c2a2d02676d8e70bd5b3798b1243437994aa5b0c7b49c3b03211ff937`. The original gzip archive disappeared after the initial read-only inventory, so the newly packaged gzip is explicitly a reconstruction, not the original sealed archive. Every payload byte/name matches. Original metadata and missing-file findings are retained. All 2846 + 688 payload bindings from the two older missing EABE archives were also located, with 48 immutable binaries added as part09; a byte-identical `part09-http1` duplicate from upload diagnosis is intentionally retained and independently verified; original compressed archive hashes are not claimed recovered.

The original exact-head CI log archive SHA is `026bde9a10b3472790d338388f126b4003ab96edd2256d55c8341ebe1e6e55f7`. `FINAL_REPORT.md` SHA is `40e0771e11ca6226a04338b425b279b7658e15fbd7b0175d429d16b12921a116`.

## Reproduce key regressions from source

From the recovered checkout:

```bash
make -f Makefile.host -j2 HOST_BUILD=recovery-build \
  recovery-build/test_gen34_ev_exp recovery-build/test_gen34_ev_exp_sanitize \
  recovery-build/test_gen35_coverage_integrity recovery-build/test_gen5_name_uncertainty \
  recovery-build/test_gen5_joint_breeding
recovery-build/test_gen34_ev_exp
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 recovery-build/test_gen34_ev_exp_sanitize
recovery-build/test_gen35_coverage_integrity
recovery-build/test_gen5_name_uncertainty
recovery-build/test_gen5_joint_breeding
python3 tests/test_release_metrics_errors.py
python3 tests/test_reference_evidence_package.py
```

The archived recovery proof ran these C++ regressions from a fresh GitHub clone. LeakSanitizer initially rejected the execution wrapper's process-inspection restrictions; rerunning outside that wrapper passed with leak detection still enabled. The initial failure log is preserved. No test requirement was weakened.

The synthetic FR/LG save can be regenerated without any private save:

```bash
make -f Makefile.host build-host/frlg_fixture.sav
sha256sum build-host/frlg_fixture.sav
```

Expected SHA: `c0aec024fd8f0b2c3b99a8e93853f63c013d223b8fd2c08a790156bc147ae0a0`, 131072 bytes. Fresh regeneration during archival matched the old fixture exactly.

## Preserve and replay the same oracle inputs

PKHeX release `26.10.18` is pinned to `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`. Some older CI captures use primary `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` plus official-release secondary replay. Those captures and full official-release local samples are distinct experiments; do not combine their denominators or relabel them.

```bash
git clone https://github.com/kwsch/PKHeX.git PKHeX-reference
git -C PKHeX-reference checkout --detach bfe89c74a3f5265805e364b79bc04ad38ebbc5ea
dotnet build tools/pkhex_gen34_diff/oracle/PKHeXGen34Oracle.csproj -c Release -p:PKHEX_ROOT="$PWD/PKHeX-reference" -o oracle-release-gen34
dotnet build tools/pkhex_gen5_diff/oracle/PKHeXGen5Oracle.csproj -c Release -p:PKHEX_ROOT="$PWD/PKHeX-reference" -o oracle-release-gen5
```

The retained `.cache/legality-major-20261101/ctx_*` drivers and source/command manifests explain production binding; code and makefile recipes are recoverable from the storage manifest. The permanent source-aware driver is `tools/pkhex_gen34_diff/gen34_diff.cpp`; Gen V uses `tools/pkhex_gen5_diff/gen5_diff.cpp`. The checked-in workflow source lists/commands rebuild them. Use the **retained** `gen34-reference.txt` and `gen5-reference.txt` for identical-input comparisons. Seed alone does not guarantee identical bytes because parts of the reference generator use other random sources.

Recalculate raw metrics without dropping difficult rows, with a **new** output filename:

```bash
python3 tools/legality_completion/release_metrics.py /absolute/path/restored-project-evidence/outputs/legality-major-20261101/source-context-repair/fresh-independent --groups gen34 gen5 --output /absolute/path/new-recovery-metrics.json
```

Full source-built differential CI commands remain in `.github/workflows/legality-gen34-pkhex-diff.yml`, `.github/workflows/legality-gen5-pkhex-diff.yml` and `.github/workflows/legality-completion.yml`. Use explicit new WORK directories so raw streams and failures survive. Never modify oracle source or historical raw records to improve agreement.

## Frozen findings and next work

Fresh 143934-row sample (zero exclusions; raw counts, not rule-completeness estimates):

| Generation | Raw agreement / all rows | False Invalid | False clean | Incomplete | Reference Fishy | Reference errors |
|---|---:|---:|---:|---:|---:|---:|
| III | 23991 / 24567 | 0 | 0 | 576 | 2321 | 0 |
| IV | 37806 / 38343 | 21 | 0 | 516 | 2385 | 0 |
| V | 80344 / 81024 | 0 | 0 | 680 | 19084 | 344 |

All 21 raw Gen IV stricter cases remain in raw false-Invalid counts despite separate original-game adjudication. All 344 Gen V reference execution errors remain in the total, separately reproduced with a diagnostic-only copy. Strict Invalid / NoProblemsFound / Incomplete semantics and historical name-filter uncertainty must survive continuation.

Remaining work includes event/cross-era/form/regional breeding ancestry, complete EV encounter/initial-state evidence (including Orre/gifts), Ranch RNG/custody, historical filters, exhaustive rule-family/branch crosswalk, acquired specimens with custody, and full twenty-title UI/device integration. No 100% claim is supported. The archived final report, `docs/legality/major-20261101/CONTINUATION.md`, and `docs/legality/hardening-20261027/RULE_GAPS.md` contain the detailed blockers.

Resume the existing engine branch only after checking the latest remote head. Keep PR #314 separate from application/main; PR #327 owns independent Gen I/II work and PR #328 owns modern generations. The archival tag and historical snapshot are recovery anchors, not instructions to merge branches.

## Privacy and cleanup

Global Codex authentication/configuration, conversations/session history, synced `sources/`, unrelated projects, unproven private saves, original Git metadata and the old Downloads NRO remain local. No private backup endpoint was provided; excluded data is explicitly **UNSAFE TO DELETE**. Local-only sensitive inventories are not published.

Read the final cleanup inventory and its guarded command file. No cleanup commands were executed during archival. Do not use a broad `rm -rf ~/.codex`, delete source worktrees, or prune Git metadata on the strength of this evidence release.
