# Live development reconciliation

**Follow-up refresh:** the newer accepted engine is `fe3e3d05`. Read [Tranche 2](TRANCHE_2_ENGINE_REFRESH.md) before acting on the original backlog below. It credits the implemented work and supplies a confirmed parent-slot integration defect plus independently executed eligibility evidence.


Delivery source freeze: **4d3026e0bbe90f2eb1505c107b1c74498c051549**, October 11, 2026. Research working-tree production remains the original `fecfbca9` baseline. Later sources were inspected through Git objects; no accepted checkout or branch was moved by this research.

The earlier `github-final-checkpoint.json` is a historical intermediate checkpoint, despite its name. Use [github-delivery-checkpoint.json](evidence/github-delivery-checkpoint.json) for delivery status. Development continued during this audit; these findings apply to the named revisions, not an unbounded future head.

## Already implemented — do not duplicate

- Detached Ranger, WC4/event, wild and move reporting from #222–#230 is included in the accepted history. Some PRs remain open even when their exact head is an ancestor; use the captured ancestry flag.
- `Gen12JointMoveEvidence`, `Gen3JointMoveEvidence`, and `Gen4JointMoveEvidence` evaluate all four slots against game-family pools. Gen III/IV enumerate finite covering subsets and report minimum family counts. Gen I/II use intersection/union bounds. This is substantive joint **pool coverage**, not four independent checks. It does not model an encounter candidate, four-slot capacity over time, parent compatibility, or ordered acquisition/deletion.
- `Gen34PalParkMoves` combines Gen III/IV pool evidence, checks structural transfer fields and ball bytes before positive provenance, distinguishes post-migration move-ID bounds, and uses `Gen34PalParkSpecies` ancestry for Gen IV evolutions. GameCube origin does not imply a named original GBA game. Do not duplicate these guards or report all retained-move evidence missing.
- Adult source-free move reports exclude unhatched eggs. Existing tests cover native-format synthetic records; this audit did not execute the entire newer accepted suite.

Exact files, hashes and symbols: source registry IDs `NX-LIVE-JOINT12`, `NX-LIVE-JOINT3`, `NX-LIVE-JOINT4`, `NX-LIVE-PALPARK`, `NX-LIVE-PALPARK-SPECIES`.

## Active draft work — coordinate before implementing

| Draft | Inspected head | Scope and remaining boundary |
|---|---|---|
| #244 | `12a6db6550ff22976683cdd867435c95c546c704` | Conditional PK2 Time Capsule return-stage move/evolution bounds; does not supply a complete directional acquisition history |
| #245 | `916e9931250476af4ffa1e5b6f08baacc5488a80` | Out-of-domain PK2 move-ID handling; do not call invalid IDs legitimate Gen II learning |
| #246 | `6c0259bff9780d5008abbf89ed5c11ffa46cbd1b` | GameCube-origin source-neutral GBA pool coverage; actual transfer route remains unknown |
| #247 | `b12f4225ae6af9eeb4aa4db9766adc10d69bc51b` | Full 386-species Emerald level-up resource and positive minimum-level evidence. Do not duplicate this extraction |
| #248 | `06eda48b4ea366dcd062cfd34602af8aee32c11c` | Ledger reconciliation; inspect actual ancestry before interpreting PR status |
| #249 | `79a5bad43c56cc09643d41ab6d8972af4d640d1e` | Gen IV egg-move game-family evidence using retained original game. Counts origin-family/other-family/unmodeled slots; explicitly does not prove parents or chronology |

The #249 branch is not simply the tip of the #248 stack. Compare each PR with its own base; a cross-branch diff can misleadingly look like deletion of unrelated newer work.

## New independent cross-source check

The research script [check_emerald_pending.py](scripts/check_emerald_pending.py) compares the complete pending Emerald table at #247 with the exact pinned PKHeX binary level-up resource. **386 species, 3,947 ordered move/level pairs, zero differences.** Both byte hashes are captured in [emerald-levelup-crosscheck.json](evidence/emerald-levelup-crosscheck.json). This validates agreement between sources and ordering, not independent retail execution; the sources may share underlying provenance. It does not establish feasibility of a four-move history or clear dataset licensing.

## Corrected remaining work

Extend accepted family-cover evidence with candidate identity and stage/method witnesses. Extend pending Emerald evidence to other versions and compatible predecessor stages. Build ordinary unhatched egg ordering and parent feasibility beyond #249 membership. Preserve Unknown for unsupported candidate alternatives. These are the backlog's actual targets; rebuilding broad pools is not.
