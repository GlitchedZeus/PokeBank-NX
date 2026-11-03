# Master Vault v1 — Implementation Plan (Phase 1)

Status: **Phase 1 implemented on `feature/master-vault-v1` behind a build flag that is OFF by default.**
Nothing here is hardware-validated. Status of every item below is at best *Fixed in Code — Awaiting
Hardware*. This branch stays isolated from the Gen I–V repair branches (`fix/*`) until the owner
validates it; it gets its own PR.

Sources reconciled: `docs/MASTER_VAULT_SPEC.md` (spec), `docs/TRANSFER_MODEL.md` (Copy / Move /
Clone semantics), `docs/qa/V1_RELEASE_FEATURE_GAPS.md` (`PBX-ACT-002`, `PBX-VLT-001`, `PBX-VLT-002`,
`PBX-BNK-003`, `PBX-BNK-004`, `PBX-BNK-005`), `docs/architecture/SHARED_UX_REQUIREMENTS.md` (§A4,
P2-8: "Move to Vault does not exist"), Round 12 intake `R12-011`.

## 1. Owner requirements (v1.0 blockers, never dropped)

| # | Requirement | Phase 1 | Remaining |
|---|---|---|---|
| 1 | Move to Vault in every Gen I–V Actions list | "Copy to Vault" row in every Gen I–V box sheet and the Gen I–III Party sheet; Gen IV/V Party via their own sheets. Flag OFF by default | "Move to Vault" (= Copy + staged Release, explicit confirmation) — owner decision D1 |
| 2 | Persistent Vault deposit and withdrawal (survives restart) | Storage core + deposit + withdraw-as-copy; persistence and fault-injection host tests | Hardware read/write test on the SD card (spec §22 last two boxes) |
| 3 | Vault search with highlighted matches | `Vault/VaultSearch.h` (AND terms, case-insensitive, spans per field) + highlighted rows in the Vault browser | Home-screen Vault browser (outside a save); item search needs held-item names in the cache |
| 4 | Cross-generation transfer later | Refused explicitly ("the Vault does not convert between generations yet") | Transfer Lab after the Vault (owner intake Q4) |
| 5 | In-save box renaming, separate | Not touched (`PBX-BNK-004`) | Separate lane |

Safety rules kept: the original emulator / Switch save is never written (Copy to Vault reads the
staged workspace's export; Withdraw only stages into the workspace, and the existing Review / verified
export path is the only way out); no injection or writeback; cross-game true Move stays locked;
Japanese Gen I/II stays read-only (no staged editor ⇒ no file capability ⇒ no Vault rows).

## 2. Existing Bank / Legacy Storage code: reuse vs. replace

| Existing piece | Decision | Why |
|---|---|---|
| `Trainer::Bank` / `bank.dat` (PKSE Legacy Storage, `include/Trainer/Bank.h`) | **Not reused as the Vault; left untouched.** | It stores PKSM `Pokemon` objects tagged by game group (GG/SWSH/BDSP/PLA/SV/ZA/FRLG only); it cannot hold PK1/PK2/PK4/PK5, has no stable IDs, no SHA-256, no provenance, and a fixed slot grid. `PBX-BNK-003` says "do not extend bank.dat tags". It stays the Legacy Storage it is today (owner decision D3 on migration). |
| `BankLoadPolicy` / `BankRecordValidation` / Q01 fault-injection test | **Pattern reused** | Same fail-closed rules: never treat an unreadable store as empty; quarantine, never delete; newer layouts read-only. |
| `Utils/DurableFile` (temp → reread → validate → promote → reread, `.previous.N`) | **Reused for every Vault write** | Already hardware-reviewed primitive; journal, record and index all go through it. |
| `Utils/SHA256` | **Reused** | Record and index checksums, raw-payload hash. |
| `PokemonFile/NativePokemonFile` (PK1–PK5 decode) | **Reused as the Vault's native-format validator** | A Vault record is exactly the plaintext record Export to file writes; the same decoder checks it on deposit, on read and on withdraw. |
| `UI/PokemonFileMenus` export bytes + per-generation import callbacks | **Reused as the deposit source and the withdraw sink** | One shared path: Copy to Vault deposits what `exportBytes` produces; Withdraw stages through `genNImport` — the very callbacks Import from file uses, so every capacity / occupied / wrong-generation / Japanese refusal is unchanged. |
| `ActionSheetModel` `AddToMasterVault` enum (Gen VI+ generic sheet, NotYetSupported) | Left alone | Gen VI+ is out of scope; Gen I–V use the shared narrow menu. |

## 3. Storage format

Root: `sdmc:/switch/PokeBank-NX/vault` (`PokeBank::Paths::vaultRoot()`), app-owned, never inside a
game save folder.

```
vault/
  records/<vault-id>.pbvr     authoritative, one per entity, written once, never rewritten
  index.pbvi                  metadata cache (rebuildable from records/)
  index.pbvi.previous.N       newest preserved generation only (cache)
  journal/<vault-id>.pending  exists only while a deposit is in flight
  quarantine/                 anything not trusted (never deleted automatically)
```

**Record (`.pbvr`)**: `"PBNXVREC"`, u32 schema (=1), u32 metadata length, u32 raw length, metadata
(`key=value` lines, escaped), the raw native record, SHA-256(raw), SHA-256(entire file before it).
Self-describing and self-verifying, so the index can always be rebuilt from records alone.

**Raw payload**: the generation's PKHeX plaintext record exactly as Export to file writes it (`.pk1` /
`.pk2` single-entry list, 80/100-byte PK3, 136/236 PK4, 136/220 PK5). It is decrypted and in canonical
substructure order for Gen III–V; this canonicalisation is *recorded* (`canonicalization=pkhex-plaintext`)
rather than hidden (spec §5). Deposit refuses anything that is not an exact plaintext record of the
declared generation (`decode(raw).plain == raw`).

**Metadata** (spec §5–7): `id` (UUID v4 text, random 128-bit, independent of species/slot/hash),
`sequence` (deposit order, never reused), `generation`, `format`, `party`, `japanese`, `raw_size`,
`raw_sha256`, display cache (`species`, `species_name`, `nickname`, `ot_name`, `level`, `shiny`, `egg`
— not authoritative) and provenance (`source_kind`, `source_game`, `source_label`, `source_box`,
`source_slot`, `source_party`, `staged_edits` — the copy came from a workspace with unsaved staged
changes, `deposited_at`, `app_version`). No paths with personal data, no console identifiers.

**Index (`index.pbvi`)**: `"PBNXVIDX"`, schema, count, next sequence, the records' metadata blocks,
SHA-256 trailer. Browsing and search read only this (no raw scan at launch, spec §21).

**Identity**: the same bytes deposited twice are two entities with two IDs (spec §9/§14: never
silently de-duplicated); `rawSha256` lets a later UI show "byte duplicate".

## 4. Transactions, recovery, corruption

Deposit (spec §18):

1. validate the native record (decode, generation, exact plaintext) and allocate a fresh ID;
2. **journal** `journal/<id>.pending` (durable write, reread);
3. **record** `records/<id>.pbvr` (durable write; refused if the file exists; validator re-decodes and
   checks ID + SHA-256);
4. **index** rewrite (durable; validator re-decodes);
5. remove the journal; prune old index generations.

On any failure before 4 completes nothing is listed: temp / failed generations of that ID and a
published-but-unindexed record are **moved to quarantine** (never deleted), the journal is removed, the
in-memory list is unchanged, and the user sees "Nothing was copied: …".

Open / crash recovery (spec §19):

* index missing / corrupt / truncated → quarantined, rebuilt from `records/` (every record fully
  verified while rebuilding; damaged records are quarantined);
* journal present → if the record re-reads and verifies it is **rolled forward** into the index,
  otherwise **rolled back** (torn record quarantined);
* records not in the index (e.g. crash between steps 3 and 4) are adopted only after verification;
* indexed entries whose record vanished are not listed and are reported (never invented);
* a newer schema (index or record) opens **read-only**; nothing is rewritten;
* stray files in `records/` are quarantined.

Read / withdraw re-verify the whole record (file SHA-256, raw SHA-256, metadata vs. index, native
decode) every time, so silent SD corruption is detected before a copy is ever staged.

Not yet covered (hardware gate, spec §22): directory-entry durability of `rename` on the Switch SD
filesystem, real power loss, and a genuinely full SD card (host tests inject the failure at each write).

## 5. UI flows through the SHARED UX

One vocabulary, one order, one dispatcher per generation — no per-generation Vault UI.

* `SharedPokemonEditor::Action` gains `CopyToVault` and `WithdrawFromVault` (appended; existing values
  unchanged); `ActionCapabilities::vault` defaults to the build flag. `actionsForSlot` lists
  **Copy to Vault** right after Export to file and **Withdraw from Vault** right after Import from file,
  only when the slot has that staged-editor file capability *and* the flag is on. Gen I's native set
  and Gen IV's own list follow the same rule with the same shared labels
  (`NarrowPokemonMenu::kCopyToVaultLabel` / `kWithdrawFromVaultLabel`).
* Every Gen I–V dispatcher (Gen I/II live sheets in `ClassicReleaseActionFix.inc`, Gen III, Gen IV,
  Gen V, Gen I–III Party sheet) calls the two shared entry points in `UI/PokemonFileMenus`:
  * `copyToVault(screen, gen, party, box, slot)` — the record as the open staged workspace holds it
    (`exportBytes`, the Export to file record), deposited under a new Vault ID; the workspace and the save
    are unchanged; status "Copied X to the Vault; this save is unchanged".
  * `beginWithdraw(screen, gen, box, slot)` — opens **the one Vault browser** (`UI/VaultDialog`), modal
    like the Import from file dialog: lists the Vault's records of this generation, **Y = Search** (live
    as typed, `VaultSearch`), matched text highlighted in species / nickname / OT / game / ID, **A =
    Withdraw Copy** → `withdrawCopy` → the generation's own import callback (Gen I/II packed boxes: first
    empty slot; Gen III–V: the chosen empty slot). Refusals keep the browser open with the editor's own
    reason. The Vault keeps its record; Review lists the staged import like any other; Discard undoes it.
* Default: **Copy** (source unchanged). "Move to Vault" is not offered in Phase 1 (D1).

Build flag: `include/Vault/VaultFeature.h` (`POKEBANK_ENABLE_VAULT`, default 0); device test build
`make POKEBANK_VAULT=1`. With the flag off every sheet is byte-for-byte the current one (pinned by
`tests/test_vault_actions_contract.cpp` default build and the existing `NarrowPokemonActionMenu.h`
static asserts, which now state `vault = false` explicitly).

Gen VI–IX applicability: the storage core is format-neutral (generation 1–9 accepted by the metadata
parser) but deposit is limited to Gen I–V natives in v1; the Gen VI+ generic sheet keeps its
NotYetSupported Master Vault rows.

## 6. Tests (host)

| Test | Covers |
|---|---|
| `tests/test_vault_store.cpp` (+ `_sanitize`) | create/open; deposit PK1/PK2/PK3(80/100)/PK4(136/236)/PK5(136/220) with UUID v4 IDs; byte-identical read-back; duplicate bytes → new ID; refusals (encrypted, wrong gen, Gen VI, bad checksum, empty) write nothing; withdraw-as-copy (wrong gen refused, Vault unchanged); persistence across reopen + sequence continuity; search spans/AND/case/generation filter/ID; full SD at journal / record / index; power loss after journal / record / index; torn record; corrupt / truncated / missing index; payload and metadata tampering detected on read; stray files quarantined; index generations bounded; newer schema read-only |
| `tests/test_pokemon_file_gen12.cpp`, `gen3`, `gen4`, `gen5` (+ `_sanitize`) | per generation with the REAL staged editors: export → deposit → restart → withdraw → occupied / full / out-of-range refusals identical → empty slot receives the exact record (Gen V: candidate + strict reparse); the source save never changes |
| `tests/test_vault_actions_contract.cpp` (default and `_on`, + `_sanitize`) | flag off: no Vault rows anywhere; flag on: one row each in the shared order, none without a staged editor (read-only / Japanese), Party sheet, Gen I native set |
| `tests/test_vault_wiring_contract.py` | Makefile default off; every Gen I–V dispatcher + Party sheet reach the shared entry points; the browser is modal; copyToVault never stages / releases / writes; withdraw uses the Import from file callbacks; no exceptions/RTTI |

## 7. Remaining for v1.0

1. Owner validation on hardware with a `POKEBANK_VAULT=1` build (spec §22: physical read-only test,
   then deposit/withdraw); real power-cut and full-SD tests.
2. **Move to Vault** (D1): Copy + staged Release in the same generation's workspace with explicit
   confirmation, completed only by the existing verified export; transactional ordering (Vault commit
   first, then staged removal) and per-generation tests.
3. A Vault browser reachable from the Home screen (Master Vault tile, today "Coming soon"), with
   View (read-only summary from the record) and Origin/provenance; named Banks over Vault IDs (spec §15).
4. Held-item / move search terms (needs those names in the metadata cache) — `PBX-VLT-002` scope (D4).
5. Spec §22 items not in Phase 1: Clone-with-parent provenance inside the Vault, Banks, explicit
   delete-with-backup semantics, schema-migration tooling.
6. Cross-generation transfer (Transfer Lab), after the owner's route table (intake Q4).
7. Shared UX docs: `SHARED_UX_REQUIREMENTS.md` P2-8 / §A4 rows to be updated when the flag is turned on.

## 8. Owner decisions needed

* **D1 Move vs Copy.** Phase 1 ships Copy to Vault only. Should "Move to Vault" (copy + staged Release,
  explicit confirmation, original save untouched until the user exports) be added for v1.0, and is the
  row label "Move to Vault" (owner wording) or "Copy to Vault" (safety default) — or both?
* **D2 Turning the flag on.** Which candidate first carries `POKEBANK_VAULT=1`, and what hardware
  checklist (deposit, restart, withdraw, search, SD-removal / power-cut) gates it.
* **D3 Legacy Storage.** Keep `bank.dat` as Legacy Storage (current plan) or migrate it into the Vault
  (intake VAULT Q6), and the user-facing name ("Master Vault"?).
* **D4 Search scope.** Fields beyond species / nickname / OT / game / ID (held item, moves, shiny,
  level ranges)?
* **D5 Withdraw target.** Withdraw is into the open save's same generation only (no conversion). Confirm
  that a Japanese Gen I/II save stays excluded (read-only) and that Party withdraw is not needed.
