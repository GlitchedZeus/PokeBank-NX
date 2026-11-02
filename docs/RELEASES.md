# Releases and candidate builds

**PokeBank NX has no stable release.** Every build listed here is an alpha development or hardware-test
candidate. The GitHub pre-releases tagged `nro/<sha8>` are archives of those candidates so their exact bytes
survive after the Actions artifacts expire; they are not recommended downloads and none is marked "latest".

A build's status only ever describes what the owner recorded for **that exact file** (matching SHA-256) on a
real Switch. A passing CI run, a newer commit or an earlier accepted build never changes it.

## Status labels

| Label | Meaning |
|---|---|
| **Accepted on hardware** | The owner tested this exact NRO and accepted it **for the scope of that milestone**. It is superseded and is not a release. |
| **Partial** | Owner-tested with recorded results that include both passes and failures. |
| **Failed hardware candidate** | Owner-tested and rejected. |
| **Experimental build** | Built for development; not offered for acceptance as-is. |
| **Not yet tested** | No owner hardware result is recorded. Either awaiting testing or superseded before it was tested. |

## Current candidate

| Build | Status | NRO SHA-256 |
|---|---|---|
| Round 13, `PokeBank-NX-Gen1-5-Candidate-9c301c48.nro` (commit `9c301c483af97acf60058546c9b35bfec45c7b0e`, build run [41392463888](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41392463888), 177,118,897 bytes) | **Not yet tested**: awaiting owner hardware acceptance | `0ecf8655bcbef6490e73e2367801d972a8a9339396e73e16a4afe8f6ca76367d` |

Round 13 is built from `fix/round12-gen15-repairs` (draft PR #333, stacked on #332) and is not on `main`.
Checklist: [`RETEST_9c301c48.md`](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/hardware/RETEST_9c301c48.md).
Pre-release archive: [`nro/9c301c48`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F9c301c48).

## Gen I–V hardware rounds

Results are in [`docs/hardware/`](hardware/) (and on `fix/round12-gen15-repairs` for Rounds 11–13). Counts are
PASS / FAIL / NOT TESTED as the owner recorded them for that round's checklist; they are not a full-game
acceptance.

| Round | Commit | NRO SHA-256 | Owner result | Status | Archive |
|---|---|---|---|---|---|
| 1 | `7fea1797` | `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5` | 42 / 6 / 6 not verified ([results](hardware/ROUND1_RESULTS_7fea1797.md)) | **Partial** | not archived (release creation was refused, HTTP 403); Actions artifact valid until 2027-01-14 |
| 2 | `e10f307f` | `1dd518b756274cfe70d7a87b06a2e30f027389aeaddd9640f03ef3c5a2eb7ac0` | 6 PASS / 6 not verified ([results](hardware/ROUND2_RESULTS_e10f307f.md)) | **Partial** | not archived (HTTP 403); artifact valid until 2027-01-16 |
| 3 | `62a2f8c3` | `0c7a6fb01af2a1356676beedf3aec9da79842a499c7c63833fb683f42f332078` | 70 / 37 / 3 ([results](hardware/ROUND3_RESULTS_62a2f8c3.md)) | **Partial** | [`nro/62a2f8c3`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F62a2f8c3) |
| 4 | `c60d4da0` | none recorded | Oct 23–24 retests on this commit ([repairs and retest](hardware/ROUND4_REPAIRS_AND_RETEST.md), [Oct 24](hardware/OCT24_REPAIRS_AND_RETEST.md)) | **Partial** | no separate candidate; repairs shipped in Round 5 |
| 5 | `1ca2faec` | `b8966cdbd108530ae1c056a07e318d2f05d0cdee30d910b361ab29f381e41511` | 31 / 8 / 3 ([results](hardware/ROUND5_RESULTS_1ca2faec.md)) | **Partial** | [`nro/1ca2faec`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F1ca2faec) |
| 6 | `26186464` | `5b1a85c878e2999f0f8212a583f037643c3b42147a21fef631ea11a832d2a3bf` | 49 / 9 / 1 ([results](hardware/ROUND6_RESULTS_26186464.md)) | **Partial** | [`nro/26186464`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F26186464) |
| 7 | `e88957f2` | `9e1b6a314fd7643901b6bb157dc69bfa82de61b9b4c45dc97214021f3db813b5` | none recorded ([checklist](hardware/RETEST_e88957f2.md)) | **Not yet tested** (superseded) | [`nro/e88957f2`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2Fe88957f2) |
| 8 | `6ec5ccb9` | `49d95c555636e3ed3f461c080c168d06c963415801d8ad02b0f7ae5f87eee186` | none recorded ([checklist](hardware/RETEST_6ec5ccb9.md)) | **Not yet tested** (superseded) | [`nro/6ec5ccb9`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F6ec5ccb9) |
| 9 | `d6a20a2a` | `57b2c1186942473ee53f78df6f44bc5ab430acb0b86f26ecddc8326c87c0494a` | none recorded ([checklist](hardware/RETEST_d6a20a2a.md)) | **Not yet tested** (superseded) | [`nro/d6a20a2a`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2Fd6a20a2a) |
| 10 | `3634d1e6` | `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9` | 34 / 13 / 1, recorded as the Round 11 worksheet ([checklist](hardware/RETEST_3634d1e6.md), [worksheet](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/hardware/ROUND11_OWNER_WORKSHEET.md)) | **Partial** | [`nro/3634d1e6`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F3634d1e6) |
| 12 | `066e1db9` | `ba6aea09f9032fcab52b397cdd9c9d04fdf5b9b9cb4babf2bfa2f0fa7bd63c03` | 20 / 4 / 0 on the owner's own 24-item checklist ([results](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/hardware/round12/README.md)) | **Partial** | [`nro/066e1db9`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F066e1db9) |
| 13 | `9c301c48` | `0ecf8655bcbef6490e73e2367801d972a8a9339396e73e16a4afe8f6ca76367d` | awaiting ([checklist](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/hardware/RETEST_9c301c48.md)) | **Not yet tested** | [`nro/9c301c48`](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/nro%2F9c301c48) |

Round 11 was not a new build: it is the owner's worksheet for the Round 10 file. Run IDs, artifact IDs and sizes
for Rounds 1–10 are in the [candidate NRO archive](hardware/NRO_ARCHIVE.md).

## Earlier milestone builds (September–October 2026)

These predate the numbered rounds. Their acceptance was for a **narrower, read-only milestone** and does not
carry over to any later build.

| Build | Commit | NRO SHA-256 | Status |
|---|---|---|---|
| Integrated Gen I–IV / Product Home (Oct 8) | `0760334fb850ca46b5171c55f690998d7680d4e7` | `4f28d7f7ad38c2b62458642e014ae3c05e866755c4152bf4c2db0574c381ce91` | **Accepted on hardware** for the integrated Gen I–IV / Product Home checkpoint (including legacy launch and Quit & Return); superseded |
| `PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro` | `50dac31f53907143f48884681056f8d582813b76` | `b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b` | **Accepted on hardware** for read-only Red/Blue/Yellow browsing and Items; superseded |
| `PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro` | `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954` | `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e` | **Accepted on hardware** for the read-only Gen III milestone; superseded |
| `PokeBank-NX-RBY-Retest-d9077e2d.nro` | `d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7` | `b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664` | **Failed hardware candidate** (Items category defect, wrong platform label for Yellow) |
| Gen I–V integration run on `fa9cfcea` (Oct 15) | `fa9cfcea` | none recorded | **Failed hardware candidate** (Gen V routing, blank Create, new-game trainer) |

Sources: [build / NRO record](BUILD_RECORD.md). Other October CI artifacts (for example the Gen V Save Instances
build `58bd564b` and the combined `35cbf163` candidate) have no extracted NRO hash on file, so they are not listed
as identifiable builds.

## Gen VI–IX (paused)

Gen VI–IX work is paused on `development/gen6-plus` and is not part of `main`.

| Build | Commit | NRO SHA-256 | Status |
|---|---|---|---|
| `PokeBank-NX-Gen6Plus-Experimental-5895e6b9` (run 41272664794) | `5895e6b9` | `dc6427244aff9cee3016ddd930bf65075059dc152e126878b039e3faa9204b41` (as reported by the build lane) | **Failed hardware candidate**: Pokémon Y save rejected, Alpha Sapphire disputed, other titles limited or read-only |
| `PokeBank-NX-Gen6Plus-Experimental-43c296dc` (run 41378109195) | `43c296dc` | `fd67dd1fdb21f835b6b6e392dc25bdf73a041b07aca77cc711de0716b124e251` | **Experimental build**, not device tested |

Details: [`ACCEPTANCE_STATUS.md` on `development/gen6-plus`](https://github.com/GlitchedZeus/PokeBank-NX/blob/development/gen6-plus/docs/gen6-plus/ACCEPTANCE_STATUS.md).

## Other pre-releases

`evidence/gen1-5-2144a682-20261102` is a legality-engine evidence archive (test corpora and logs), not an
application build. The `archive/local-reflog-20261102/*` tags preserve commits and carry no release.

## What a v1.0 release requires

No build is called stable until a single exact NRO passes the full Gen I–V hardware gate across all 20 releases
and the v1.0 features exist (Master Vault, Move to Vault, deposit/withdraw, Vault search, cross-generation
transfer, in-save box renaming, source-backed gift/static encounter data). See [STATUS.md](STATUS.md).
