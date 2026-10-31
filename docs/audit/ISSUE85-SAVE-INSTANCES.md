# Issue 85: provider-neutral Save Instances

## Starting point and audit

The live PR #79 head was `17d551a11845adec8079de1fa91eae4e2d9551d6`, ahead of the
handoff's `b007e125191463bcb12556f318883b53283be27c`. All later work was retained.
The owner's hardware acceptance remains attached to
`d49efd0c16433aaa1aa171501671a6e811c9da64`; it does not automatically accept this
later runtime candidate. PR #77 is outside this tranche.

Before editing, the audit found that the live head already introduced
`Source::SaveInstance`, classic row aliases, a Gen IV adapter, shared sorting and
row rendering. Its host build failed because three classic adapters partially
aggregate-initialized the expanded model under `-Werror`. It also discarded
remembered-source/alias metadata when deduplicating, used a path identity for
Gen IV physical dedupe, and left the Gen IV safety contract checking the old
inline renderer/sorter. These were completed forward, not replaced wholesale.

| Responsibility | Shared boundary | Generation-specific responsibility retained |
| --- | --- | --- |
| Provider/source metadata | `Source::SaveInstance`: exact game, platform, provider, paths, physical and path-stable identities, time, size, fingerprint, summary, validation/access/diagnostic state, claim and remembered state | Native parsed-save handles and validation indices |
| Classic discovery | Bounded results project to shared rows; alias identities survive physical dedupe | Existing RBY, GSC, FRLG/RSE format checks and exact-game qualification |
| Gen IV discovery | `toSaveInstance` projects candidate metadata, physical identity and validated payload fingerprint | Strict DP/Pt/HGSS parser and raw/DSV container qualification |
| Presentation | One `drawSaveInstanceRows` component, shared ordering and visibility; classic Source Details consumes the shared row | Gen IV Source Setup and classic Details retain their accepted actions |
| Profile claims | Existing binding database applies claims across observed aliases and still-present bound physical paths | Existing explicit setup and transactional persistence |
| Refresh/open | Shared validated-snapshot comparison after reinspection; no substitute selection | Typed classic catalog refresh and Gen IV file reinspection/open bridges |
| UIManager | Receives the selected exact source | Existing typed read-only trainer bridges; no giant parser or generic unsafe cast |

The historical `FRLGDiscoveryResult` name remains an internal aggregate for
classic parsed handles. Renaming it adds no safety or presentation benefit in
this tranche. The shared metadata boundary prevents the UI renderer, ordering
and visibility rules from depending on those native handle types.

## Behavior and constraints

- One exact-game card leads to Save Instances, including remembered Gen IV files.
- Shared dedupe retains the first provider/validation handle, remembered status
  and all observed path-stable aliases. Separate byte-identical files stay separate.
- Physical keys use device/inode where available, otherwise normalized paths.
  They are runtime dedupe keys, not persistent ownership identities across copies
  or arbitrary renames. Existing bindings remain path-stable and format-compatible.
- Conflicting profile or exact-game claims fail closed. Alias claims are saved
  transactionally and restored in memory on persistence failure. Unassigned
  validated instances remain visible.
- Opening revalidates the selected file and compares game, identity, path,
  physical key, timestamp, size and content fingerprint with the displayed row.
  A changed/missing source returns to review/refresh instead of silently opening
  changed bytes or selecting a different source.
- Classic refresh still uses all configured providers. RetroArch, configured
  mGBA battery storage and only Tico `sdmc:/tico/saves/gb`, `/gbc`, `/gba` remain
  supported. No Tico DS root or arbitrary ROM-directory scan was added.
- Gen IV preserves RetroArch, DraStic cartridge backups, melonDS and explicitly
  remembered Manual paths outside normal roots. `.dss` remains unsupported.
- Strict parser implementations, source-write locks, generation scope, joystick
  parity/repeat, quiet modal surfaces and the accepted Gen II border fix are unchanged.
- Generated tables remain committed; normal builds do not regenerate them or
  fetch upstream data. The preserved regeneration driver is maintenance-only.

## Permanent regression evidence

`test_save_instance_model` covers mixed providers, physical dedupe, remembered
metadata, aliases, newest-first ordering, unknown timestamps, claim conflicts,
read-only metadata and changed validated snapshots. The initial remembered
metadata assertion reproduced the defect before the implementation fix.

Classic discovery tests cover overlap and hard-link aliases for RBY/GSC/FRLG,
provider-complete refresh, configured roots and unchanged source bytes.
Classic browser tests cover mixed-provider cards, profile visibility and stale
resolution. Binding tests cover alias persistence, cross-profile denial,
transaction rollback and cross-game hard-link assignment rejection.

The Gen IV foundation suite covers known/Manual hard-link dedupe, remembered
state, profile claims across aliases, valid-but-changed payload rejection,
strict family mismatch, unsupported savestates and source immutability.
Architecture/safety contracts follow the shared renderer/model while preserving
no remembered auto-open, typed bridges, bounded roots and all write locks.
Existing strict Gen I-IV parser suites remain in the full host and sanitizer gates.

Required delivery gates are full host, focused source tests, ASan/UBSan and an
exact-head Actions devkitA64 compile/link. Results and artifact identities belong
in Issue #85 after those runs finish. Native workflow path filters now include
the shared source model and its test gate.

## Hardware gate

A fresh Actions-built NRO is required: shared row rendering was introduced after
the accepted hardware baseline, and this tranche changes stale-row and alias
claim handling. Automated evidence cannot label it DEVICE ACCEPTED. The owner
should test Save Instances/provider labels, refresh, changed/missing source
messages, profile isolation and ordinary read-only opening. No new generation,
source write, Master Vault, True Move or PR merge is authorized by this refactor.
