# PokeBank NX — verified archival and storage recovery report

Date: 2026-11-02. Engine anchor: `2144a6820490fa81c932dd2d917b5153283feefc`. Evidence release: https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/evidence/gen1-5-2144a682-20261102

## Inventory and preservation

Initial audited allocation: **27,232,681,984 bytes (27.23 GB)** under `/home/user/.codex`; **26,145,406,976 bytes (26.15 GB)** in this PokeBank project. The metadata inventory recorded **137,850 files**, without reading or publishing private conversation contents. Largest directories were outputs (16.77 GB), project caches (5.24 GB), and research sources (0.67 GB). Exact per-file classification and size reports remain local as `ALL_FILES_PRIVATE.jsonl`, `SIZE_REPORT_PRIVATE.json` and `CLASSIFIED_FILES_PRIVATE.v2.jsonl`.

The durable storage archive now maps **13,225 original regular-file paths**, **9,926 unique content-addressed blobs**, and **551 recorded aliases**. All archived regular-file payloads have been independently downloaded and rehashed. Initial failures, hard cases, raw reference errors and disagreements are retained; content deduplication does not remove any filename/hash binding or change measurement denominators. Runtime/source aliases and SDK dependencies have explicit recovery handling and remain outside cleanup.

The public safety audit resolved all original specimen flags through pinned public Event Gallery blobs or a synthetic save freshly generated from public code. The 177-path source/build/probe supplement also passed its separate audit. Credentials, private conversations, unproven saves and game dumps are excluded. `EVENTS_PUBLIC_FIXTURE_PROVENANCE.json`, `SYNTHETIC_SAVE_PROVENANCE.json` inside the recovery proof and the upload-clearance metadata document these decisions.

## Git and source recovery

PR #314's frozen engine commit is `2144a6820490fa81c932dd2d917b5153283feefc`. The evidence tag points directly to it; the prerelease is separate from official application releases and is not latest. No application merge, history rewrite, forced update or branch deletion occurred.

The 47-file historical Gen I/II audit snapshot was committed and pushed to its existing audit branch at [`2a681f2cbfbc4d3d543cd882f93166e56109e639`](https://github.com/GlitchedZeus/PokeBank-NX/commit/2a681f2cbfbc4d3d543cd882f93166e56109e639). All 47 file hashes were verified after an independent remote fetch. This is a historical QA snapshot, not a current production fix or green-CI claim. Sixty-one orphaned reflog commits were independently checked as public GitHub commits, pinned by new `archive/local-reflog-20261102/*` backup tags, fetched into the fresh clone and OID-verified.

All **810 production/runtime file hashes** match the fresh GitHub checkout of the engine anchor. Pinned submodules restored successfully. The fresh clone passes EV/EXP, coverage, historical-name and recursive-breeding regressions, plus EV/EXP ASan/UBSan/LSan and the metrics/package regression tests. The sanitizer's initial wrapper restriction failure is retained; leak detection remained enabled for the successful rerun.

An immutable native BW2 witness restored from the actual downloaded part09 archive rehashed correctly, retained executable permissions and passed its **19,999-case** native-source check. Source-directory aliases were also restored against the exact fresh checkout. See `RECOVERY_PROOF.v1.tar.gz`, `REMOTE_GIT_RECOVERY_PROOF.v1.json` and `RECOVERY_GUIDE.v6.md`.

## Missing original compressed files

Three large archive files were present in the initial inventory and absent at the next safety check. No removal command was run by this agent. Their original compressed byte streams are **not claimed backed up**.

Part B was reconstructed under a new name. Every **889** payload-name/hash binding matches the original seal, whose manifest SHA-256 is `4e79740c2a2d02676d8e70bd5b3798b1243437994aa5b0c7b49c3b03211ff937`. Its new gzip SHA-256 is `4ec41ab2a2ff883d0510ffb9f96126837c3da3146a111b7db45cd3fa40f590ba`. Independent download recovered and checked all payloads. The original gzip SHA (`3bbe063dca428548239a5e6e89433dc29528c8e75becb7e9449df34ac9c2c5ac`) is preserved as historical metadata, not substituted with the reconstruction hash.

All **2,846 + 688** payload bindings from the two missing older EABE seals are present in download-verified storage parts; 48 additional immutable witnesses were preserved in part09. The original seal metadata remains archived. A byte-identical `part09-http1` duplicate from transfer diagnosis is intentionally retained and independently verified.

## Remote SHA-256 and recovery results

Each listed asset exists on GitHub in `uploaded` state, has the expected size/server digest, and was independently downloaded and recovered. The manifest and verification report record full remote destinations and asset IDs.

| Asset | SHA-256 | Bytes |
|---|---|---:|
| [legality-storage-evidence-v1-part01.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part01.tar.gz) | `9bd3340280e2fdbcb879a18e227534a42c16eef4654ec1c0dca1bfd40ac72cad` | 309295900 |
| [legality-storage-evidence-v1-part02.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part02.tar.gz) | `37bf806e576806bb6d510d49b606c03d6594bcbe84cbdef7ac338cf8b32d3a60` | 754946762 |
| [legality-storage-evidence-v1-part03.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part03.tar.gz) | `866995954999694c8bf7c781494f8252f7f98170c4831aaaba149e298c7530b7` | 693358590 |
| [legality-storage-evidence-v1-part04.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part04.tar.gz) | `0580b4c542bf37bcd7791413be4fa1842f4e969532a103bdfede307409c2e3d7` | 689539287 |
| [legality-storage-evidence-v1-part05.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part05.tar.gz) | `12f58eeb5a4bc1ff64daa46befa1d8b6272a4db5052caa747b903403a02fe363` | 554103683 |
| [legality-storage-evidence-v1-part06.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part06.tar.gz) | `50b6100a05953915aea8b53e26718f212a461bd451a7c0c6d0150507fc55bdc7` | 709780546 |
| [legality-storage-evidence-v1-part07.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part07.tar.gz) | `2ba9a24cb0d2f6bec6307f02269b51a9267c4ac1018a3dde2245b9d108dda926` | 940118406 |
| [legality-storage-evidence-v1-part08.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part08.tar.gz) | `1681b5ad45fca1254b5a79e8e36295345739a8ab056740dc56f64337654c9510` | 321593250 |
| [legality-storage-evidence-v1-part09.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part09.tar.gz) | `083eb5301e11a60d76950e63eb8107722ef7c18c5497bd1d664ab78d573ef434` | 47639611 |
| [legality-storage-evidence-v1-part10.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part10.tar.gz) | `50f9d15c8a8bcf7b1db3ee02c3e9f18fdc7ad29d5548ffb8eddce3a2a06945ee` | 1665644 |
| [legality-major-2144a682-PART_B-RECONSTITUTED.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-major-2144a682-PART_B-RECONSTITUTED.tar.gz) | `4ec41ab2a2ff883d0510ffb9f96126837c3da3146a111b7db45cd3fa40f590ba` | 1338891230 |
| [legality-major-2144a682-CI_LOGS.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-major-2144a682-CI_LOGS.tar.gz) | `026bde9a10b3472790d338388f126b4003ab96edd2256d55c8341ebe1e6e55f7` | 706357 |
| [FINAL_REPORT.md](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/FINAL_REPORT.md) | `40e0771e11ca6226a04338b425b279b7658e15fbd7b0175d429d16b12921a116` | 6282 |
| [legality-storage-evidence-v1-part09-http1.tar.gz](https://github.com/GlitchedZeus/PokeBank-NX/releases/download/evidence/gen1-5-2144a682-20261102/legality-storage-evidence-v1-part09-http1.tar.gz) | `083eb5301e11a60d76950e63eb8107722ef7c18c5497bd1d664ab78d573ef434` | 47639611 |

`ARCHIVE_MANIFEST.v2.json.gz` SHA-256: `331441c89014c649bacde16e2baeab9a6e9d0a15fa4c6c9e2c32c2e8e009eb2f`. The Git archival index pins it and the final checksum/control records. Final public retrieval checks are recorded separately after publication; no expiring Actions artifact is the sole backup.

## Cleanup — prepared only

Eligible file allocation: **21,571,874,816 bytes (21.572 GB)** across **5,818 exact file paths**. This includes verified original outputs, regenerable compiler intermediates/executables, local verified archive staging copies and duplicate downloads. Allocated file blocks are counted without overlapping inodes; actual filesystem free-space gain may differ. Directories and aliases remain.

**No cleanup was executed.** The guard first validates every path/size/hash. Its execution mode also rechecks remote assets/digests and the exact evidence tag, and refuses changed files, source paths, Git metadata and symlink traversal. The archival session ran only its default dry run.

Current-PC commands, after reviewing the full inventory:

```bash
python3 /home/user/.codex/.chatgpt-projects/g-p-6a9518ebef448191bfcc640287c6cb43/outputs/archival-20261102-5fea7345/cleanup_guard_v4.py /home/user/.codex/.chatgpt-projects/g-p-6a9518ebef448191bfcc640287c6cb43/outputs/archival-20261102-5fea7345/CLEANUP_INVENTORY.v2.json
```

Only when the user chooses to remove the verified files later:

```bash
python3 /home/user/.codex/.chatgpt-projects/g-p-6a9518ebef448191bfcc640287c6cb43/outputs/archival-20261102-5fea7345/cleanup_guard_v4.py /home/user/.codex/.chatgpt-projects/g-p-6a9518ebef448191bfcc640287c6cb43/outputs/archival-20261102-5fea7345/CLEANUP_INVENTORY.v2.json --execute
```

`SAFE_CLEANUP_COMMANDS.v2.txt` contains these exact commands. This frozen inventory describes the original PC, not a fresh-PC restore.

## Local-only material: UNSAFE TO DELETE

Global Codex authentication, configuration, hooks, installed skills/tools, unrelated projects, conversations/session history, databases/backups and private inventories remain local. No authorized private backup destination was supplied. Synced `sources/` stays read-only and retained. Useful source worktrees, original Git metadata, SDK/public-source dependency caches, unique fuzz/source-uncertain paths, the Downloads NRO and external transaction remnants are excluded from cleanup.

There remain **6,747 unreviewed unreferenced commit objects** in the original Git store. Named development heads and identified orphaned reflog tips are remote-recoverable, but these additional objects are not claimed separately externally backed up. The original Git store remains intact and outside every removal command. Do not treat this archive as a complete backup of private/global Codex state or every local Git object.

## Frozen legality limitations

No new legality rules were implemented during archival. The retained fresh sample has 143,934 rows and no exclusions: Gen III 23,991/24,567 agreement, 0 false Invalid, 0 false clean, 576 Incomplete; Gen IV 37,806/38,343, **21 raw false Invalid**, 0 false clean, 516 Incomplete; Gen V 80,344/81,024, 0 false Invalid, 0 false clean, 680 Incomplete and **344 reference execution errors**. Fishy counts and successful-reference denominators remain separate in the original reports. Gen I/II was not freshly benchmarked during this archival session.

Remaining release evidence includes event/cross-era/form/regional ancestry, complete native EV initial-state/encounter support including Orre/gifts, Ranch RNG/custody, historical name filters, exhaustive rule crosswalk, authenticated acquired specimens and twenty-title UI/device integration. The archive proves recoverability of the stated work, not 100% engine completeness or release acceptance.

## Current allocation and cleanup breakdown

The final size scan at 2026-11-02T07:55:59.925227+00:00 measured **30,047,510,528 allocated bytes (30.05 GB)** under Codex, including this session's backup staging. The verified cleanup list separates **15,162,486,784 bytes (15.162 GB) of existing outputs/compiler files** from **6,409,388,032 bytes (6.409 GB) of verified local archive staging and duplicate downloads**. Total: **21,571,874,816 bytes (21.572 GB)**. No cleanup was executed. Source/Git/global-private state and original local fuzz/corpus/probe files stay retained. See `PUBLIC_SPACE_BREAKDOWN.v1.json`.
