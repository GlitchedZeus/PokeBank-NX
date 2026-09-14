# PokeBank NX Project Status

Last updated: 2026-09-14

For the shortest handoff, read `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md`.

## Project identity

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
Active PR: #68 — OPEN / DRAFT / NOT MERGED
Focused implementation head: 43b8563d4177b58ae0f341c1ebd58f86e2ee4fdb
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

`DEVICE ACCEPTED` is reserved for an exact artifact physically tested by the owner.

## Headline

PokeBank NX has moved beyond its original reader prototype:

- Generation I, II, and III legacy read-only paths are physically accepted.
- The classic staged inventory editor is hardware accepted.
- The Generation I boxed staged Pokémon editor is hardware accepted.
- Passive Gen I View Pokémon presentation is unified and hardware accepted.
- Generation II shared View/Create/Edit is implemented on PR #68 and is awaiting one exact final validation/package pass followed by physical Gold/Silver/Crystal testing.
- Live installed-game and emulator-source writes remain hard disabled.

## Accepted legacy read baselines

| Game | Status |
|---|---|
| Red GB | DEVICE ACCEPTED READ-ONLY |
| Blue GB | DEVICE ACCEPTED READ-ONLY |
| Yellow GB | DEVICE ACCEPTED READ-ONLY |
| Gold GBC | DEVICE ACCEPTED READ-ONLY |
| Silver GBC | DEVICE ACCEPTED READ-ONLY |
| Crystal GBC | DEVICE ACCEPTED READ-ONLY |
| FireRed GBA | DEVICE ACCEPTED READ-ONLY |
| LeafGreen GBA | DEVICE ACCEPTED READ-ONLY |
| Ruby GBA | DEVICE ACCEPTED READ-ONLY |
| Sapphire GBA | DEVICE ACCEPTED READ-ONLY |
| Emerald GBA | DEVICE ACCEPTED READ-ONLY |

Generation II Trainer semantics:

```text
Gold Trainer gender: Male
Silver Trainer gender: Male
Crystal Trainer gender: save-derived
Gen II SID: N/A — does not exist
```

## Accepted classic Inventory milestone

The shared staged Inventory experience is accepted on hardware with `A Edit / X Add / Y Remove / L/R Category / + Options / - Help / B Back` and exact-game catalog filtering. Original source bytes remain immutable and live writeback was not authorized.

## Accepted Generation I Pokémon editor milestone

Issue #62 is closed as completed.

```text
Source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

The accepted Gen I editor established the permanent capability-driven Pokémon editor contract. Exact save format controls field availability; unsupported modern fields are hidden rather than fabricated. Existing-Pokémon Edit uses a transactional session baseline, and source saves remain immutable.

PR #67 later unified Party/Storage/action-sheet View Pokémon into the same passive `DETAILS | VALUES | MOVES` visual language with no editable cursor and B-only navigation.

## Active Generation II shared editor milestone — PR #68

Current implementation head:

```text
43b8563d4177b58ae0f341c1ebd58f86e2ee4fdb
tree e791463ef438b5795da28c636de7bcac1105806f
```

Implemented in the focused branch:

- Gold/Silver and Crystal exact-game move compatibility using generated pinned offline data;
- passive View recalculation with green `OK` / orange `Unusual preserved`;
- legality explicitly deferred as `Not checked`;
- clean passive View labels;
- Gen II shared Details/Values/Moves editor;
- authentic DVs, derived HP DV, Stat Exp, six battle stats and six-axis radar;
- Held Item picker using the authentic Gen II item domain;
- Friendship and Pokérus support;
- Crystal-specific caught/met support;
- species + Attack-DV gender semantics;
- DV-derived shiny semantics;
- local Add/Edit drafts;
- transactional existing-Pokémon Edit keep/discard/continue;
- preservation of unusual existing move bytes on unrelated edits;
- separate Level and EXP applet invocation;
- permanent host/sanitizer surface and compatibility tests.

Not yet complete:

- one frozen exact final candidate SHA;
- complete latest-head host/regression run;
- ASan/UBSan latest-head proof;
- devkitA64 + final NRO link latest-head proof;
- embedded RomFS/source identity verification;
- exact artifact/NRO/ZIP hashes;
- physical Gold/Silver/Crystal acceptance.

PR #68 must not merge before that result.

## Permanent editor architecture

```text
exact game/save/revision capabilities
        ↓
shared View/Create/Edit shell
        ↓
generation-specific adapters and pickers
        ↓
local/staged mutations only
        ↓
strict serialize/finalize/reparse validation
        ↓
exported edited copy
        ↓
future separately approved write adapter
```

Examples:

- Gen I: no Held Item/Nature/Ability/Ribbons.
- Gen II: Held Item/Friendship/Pokérus yes; Nature/Ability/Ribbons no.
- Later generations add only fields genuinely stored by that format.

## Standalone runtime contract

```text
Runtime root: sdmc:/switch/PokeBank-NX/
Required /PKSE/: NO
Required PKSE.nro: NO
Required PKSM: NO
Required pkDex: NO
Required pkHouse: NO
Required JKSV: NO
Required Checkpoint: NO
Required PC PKHeX: NO
```

External tools may be references/oracles or optional migration helpers, but not required for normal advertised runtime use.

## Safety

- Original source bytes are sacred/immutable.
- Live installed-game writes are HARD DISABLED.
- Live RetroArch writes are HARD DISABLED.
- Live other-emulator writes are HARD DISABLED.
- Party mutation remains deferred where safety proof is incomplete.
- Unknown save variants fail closed.
- Savestates are not canonical battery-save sources.
- Read acceptance does not authorize writes.

## Immediate next milestone

Finish exact-head validation and package the first Gen II shared-editor hardware-test NRO. Do not begin the next major roadmap milestone until the owner reports the Gold/Silver/Crystal physical result.
