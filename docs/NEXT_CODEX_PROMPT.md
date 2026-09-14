# NEXT CODEX PROMPT — GENERATION II SHARED POKÉMON EDITOR

Continue PokeBank NX on `feature/pokebank-playable`.

## ACCEPTED BASELINE

PR #66 is merged. Generation I boxed staged Pokémon editing is now physically DEVICE ACCEPTED.

Accepted Gen I hardware candidate:

```text
Application source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
Application tree:   b0ec6d42a426b4bba4bff87ad7eb948ccafc4fe9
NRO:                PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
NRO SHA-256:        3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
Acceptance record:  docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md
Issue #62:          CLOSED / DEVICE ACCEPTED
```

PR #66 merge commit baseline:

```text
22333d370f4c06d334946e8ff913f884a205722f
```

Recover the latest remote production head before editing; do not assume the baseline above is still HEAD if newer documentation commits exist.

## NEXT MILESTONE

Bring the existing Generation II Gold/Silver/Crystal boxed-Pokémon staged editor onto the same reusable PKSE-style View/Create/Edit foundation now accepted for Gen I, then produce an exact NRO for physical hardware acceptance.

Do NOT rewrite the existing Gen II parser/serializer/editor core unless a concrete failing test proves a defect. Reuse the already validated staged Gen II backend.

## PERMANENT EDITOR RULE

Read and obey:

`docs/POKEMON_EDITOR_UI_CONTRACT.md`

If a field genuinely exists in the exact current game/save format, expose it in the shared editor. If it does not exist, hide it rather than fabricate it. Exact game/save/revision capability wins over species debut generation.

Gen II should therefore expose truthful PK2 concepts such as:

- Species
- Nickname
- Level / EXP
- OT
- TID
- Held Item
- Moves / PP / PP Ups
- Attack / Defense / Speed / Special DVs
- derived/read-only HP DV
- Stat Exp
- Friendship
- Pokérus
- caught/met data where the exact GSC format supports it
- shiny state derived from DVs
- gender derived from species + Attack DV where applicable
- real calculated Generation II battle stats

Do NOT add Gen III+ concepts to GSC:

- Nature
- Ability
- Ribbons
- Marks
- modern personality/PID semantics
- later-generation-only met fields

Gold/Silver trainer gender remains fixed Male. Crystal trainer gender remains save-derived. Gen II SID does not exist.

## SHARED UI EXPECTATION

Use the same permanent three-panel visual language accepted for Gen I:

```text
DETAILS | VALUES | MOVES
```

Create, Edit and View should share one generation-aware shell.

Preserve the accepted controller model:

```text
D-pad / Left Stick   Navigate current panel
A                    Edit / Select
Y                    Generation-appropriate quick action only when explicitly shown
L                    Previous panel
R                    Next panel
B                    Back / transactional exit
```

Footer hints must always describe the currently active screen/modal.

For existing-Pokémon Edit, preserve the accepted transactional-session rule:

- opening Edit snapshots the exact current staged Pokémon;
- previews may update inside the editor;
- B after a real change opens confirmation;
- A keeps this session's staged edits;
- X discards only this Edit session and restores the exact state present when Edit opened;
- B on confirmation returns to editing;
- opening Edit and changing nothing exits normally without pointless confirmation;
- discard must never erase unrelated earlier staged work.

## GEN II SUPPLEMENTAL DATA PANE

Use the lower-left supplemental pane beside the radar for real Gen II data rather than empty filler.

Good candidates include compact truthful values such as:

- Held Item
- Friendship
- Pokérus
- caught/met information where supported
- source game
- EXP growth / next-level information

Held Item must be interactive and use an exact-game-valid picker. Do not expose later-generation items that cannot exist in GSC.

There are no Ribbons in Gen II, so do not show a Ribbons row.

## GEN II BATTLE STATS / RADAR

Generation II battle stats are six numeric stats because Special Attack and Special Defense are separate battle stats, even though the stored DV is one shared Special DV.

Do not blindly reuse the Gen I five-axis labels if they would misrepresent Gen II.

Audit the existing Gen II stat model and choose a truthful readable presentation. The shared radar architecture may require a generation-specific six-axis variant for Gen II+.

Requirements:

- use actual calculated GSC battle stats;
- make clear that Sp. Atk and Sp. Def derive from one stored Special DV in Gen II;
- do not fabricate independent SpA/SpD DVs;
- keep calculated battle stats read-only/unfocusable;
- editor and normal Summary must agree.

## PRESERVE EXISTING GEN II BACKEND

The existing staged Gen II editor already has host/native coverage for boxed Pokémon edits, including semantic round-trip validation and derived-gender normalization. Preserve that work.

Do not weaken:

- strict serialize -> finalize -> reparse -> semantic verification;
- source immutability;
- exact game handling;
- unsupported Japanese/edit layouts fail-closed behavior;
- staged Trainer / Money / Inventory behavior;
- machine display mappings;
- existing Gen I / Gen III read regressions.

Party mutation remains DEFERRED.

## SAFETY — NON-NEGOTIABLE

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE RETROARCH WRITES: HARD DISABLED
LIVE INSTALLED-GAME WRITES: HARD DISABLED
LIVE OTHER-EMULATOR WRITES: HARD DISABLED
PARTY EDIT: DEFERRED
```

Implementing the Gen II UI does not authorize live writeback.

## FIRST TASK

1. Recover exact current `feature/pokebank-playable` HEAD/tree.
2. Confirm PR #66 merge and Gen I acceptance record are in history.
3. Run the existing Gen II focused staged-editor tests before changing UI.
4. Audit which Gen II backend fields already exist and map them into the permanent shared editor capability model.
5. Reuse the accepted Gen I UI primitives rather than creating another generation-specific editor from scratch.
6. Keep Gen I DEVICE ACCEPTED behavior frozen unless a new regression proves otherwise.

## REQUIRED REGRESSION COVERAGE

At minimum cover:

- exact Gen II field/capability set;
- Held Item present and editable;
- no Nature/Ability/Ribbons;
- HP DV derived/read-only;
- one stored Special DV feeding truthful Gen II battle-stat presentation;
- Friendship / Pokérus / caught data where supported;
- derived gender behavior;
- shiny DV behavior;
- transactional Edit keep/discard/no-change semantics;
- Add remains explicit staged-draft flow;
- exact-game held-item validity;
- calculated stats unfocusable;
- contextual picker/modal ownership;
- source immutability and all live-write locks.

## VALIDATION BEFORE DEVICE NRO

Require:

```text
git diff --check
focused Gen II staged editor tests
shared Pokémon editor capability/UI tests
Gen I accepted editor regressions
GSC read-only regressions
RBY regressions
FRLG/RSE regressions
inventory regressions
machine-display regressions
source mutation policy
full permanent host suite
ASan
UBSan
device asset preflight
devkitA64 clean compile
final NRO link
embedded application identity
embedded RomFS verification
exact artifact packaging
```

Do not weaken tests to make the candidate green.

## HARDWARE HANDOFF

When the exact candidate is fully green, download the CI artifact and independently verify its manifest and hashes. Report exact application SHA/tree, workflow IDs, NRO filename/bytes/SHA-256, packaged ZIP, artifact ID/digest and RomFS count.

Then provide the actual `.nro` and STOP for owner hardware testing.

Primary physical test targets:

1. Gold/Silver/Crystal boxed Pokémon open in the shared editor.
2. View is read-only.
3. Create/Edit share the same workspace.
4. Held Item picker works and only offers exact-game-valid items.
5. Friendship, Pokérus and supported caught/met fields are truthful/editable.
6. DVs and derived HP DV behave correctly.
7. Gen II gender derivation remains correct.
8. shiny behavior remains DV-derived.
9. battle stats/Summary agree and clearly handle split SpA/SpD with one stored Special DV.
10. transactional B exit keep/discard/no-change works.
11. discard restores exact pre-edit staged state.
12. source `.srm` remains untouched.

## STOP CONDITION

Do not start Gen III boxed editing, Master Vault, DS/3DS, live writes or another roadmap milestone until the Generation II shared editor has an exact hardware-test candidate and the owner reports the device result.