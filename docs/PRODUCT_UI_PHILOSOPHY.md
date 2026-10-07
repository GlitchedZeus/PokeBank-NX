# PokeBank NX — Product & UI Philosophy

This document is the durable product-design standard for PokeBank NX. It applies to new UI, editor workflows, Vault/Banks, discovery, transfer, legality, recovery, touch, controller behavior, and future features.

## Core product mantra

> **Nintendo-level polish. Homebrew-level freedom. PKHeX-class power. Kid-safe by default.**

Long form:

> **The engine underneath can be as powerful as the best Pokémon homebrew tools ever made. The interface on top should feel as polished, safe, and intuitive as if Nintendo built it for everyday players — but designed with the freedom, practicality, preservation mindset, and real-world feature ideas of the Pokémon homebrew community.**
>
> **PokeBank NX should feel like Nintendo and the Pokémon homebrew community collaborated on the ultimate Pokémon management app: simple enough for a young player to use safely, powerful enough for advanced users to do almost anything they need, and smart enough to keep dangerous complexity out of the way until it is actually wanted.**

Another useful shorthand:

> **PokeBank NX should feel like Nintendo built Pokémon HOME together with the homebrew community: polished for normal players, powerful for experts, and designed around the features people actually want.**

This is a design aspiration, not a claim of affiliation with or endorsement by Nintendo, The Pokémon Company, GAME FREAK, or Creatures.

---

## The child-safety test

For every normal workflow, ask:

> **Could a young Pokémon player use this without understanding save formats, checksums, DVs/IV internals, emulator folders, source adapters, or corruption risks — and still avoid damaging their save?**

If the answer is no, the UX needs another layer of protection, explanation, progressive disclosure, or a safer default.

The user should not need expert knowledge to avoid expert-level mistakes.

---

## Power underneath, simplicity on top

PokeBank NX may internally contain extremely technical systems:

- exact generation-specific save parsing;
- checksums and rotating save blocks;
- DVs, Stat Exp, IVs, EVs, PID-derived properties;
- conversion rules;
- provenance graphs;
- legality engines;
- transaction journals;
- source fingerprints;
- rollback/recovery;
- emulator and native Switch source adapters.

Normal users should not be forced to think in those terms unless they deliberately enter an advanced view.

The app should present Pokémon concepts first:

```text
Pokémon
Moves
Stats
Trainer
Boxes
Items
Banks
Transfers
Backups
Pending Changes
```

—not raw save-structure concepts.

Advanced controls are not removed; they are placed where they logically belong.

---

## Progressive disclosure, not feature removal

PokeBank NX is not intended to become "simple" by deleting useful power-user features.

Instead:

```text
common action
    ↓
clear normal UI
    ↓
logical detail editor
    ↓
advanced/technical controls when requested
```

Examples:

- Show four Move rows in the main Pokémon editor; put PP and PP Ups inside the selected Move editor.
- Show a clean Stats section; put DVs/IVs/EVs/Stat Exp inside generation-appropriate detail editors.
- Show a simple legality status; expose evidence/details separately.
- Let normal users choose a destination visually; keep source IDs and transaction diagnostics in advanced/provenance views.

A powerful feature should be easy to discover without making every screen look like a developer tool.

---

## Safe by default

Safety is part of the product experience, not merely a backend concern.

Default rules:

- original source saves are sacred;
- previewing or hovering never silently commits data;
- Cancel means cancel;
- Back means leave safely;
- edits are staged before destructive actions are considered;
- Pending Changes clearly explains what will happen;
- unsupported capabilities are hidden or truthfully disabled;
- unknown formats fail closed;
- risky actions use plain-language confirmation;
- recovery/backups are built into future approved write workflows;
- no global "enable all writes" switch.

A user should be protected from mistakes even if they do not understand why the protection exists.

---

## Controller-native and touch-native

The app should feel like a console application, not a desktop form squeezed onto a Switch.

Controller conventions should remain predictable:

- D-pad / Left Stick: navigate;
- A: select/edit/confirm;
- B: back/cancel;
- X/Y: obvious contextual actions where useful;
- L/R: boxes, categories, tabs, or sections only when that mapping makes sense;
- footer hints: always describe the currently active screen or modal.

When a modal opens, its controls replace the underlying screen's controls. Closing it restores the previous controls.

Touch should follow the same conceptual actions rather than becoming a separate UI design.

---

## Visual, direct manipulation over numeric navigation

Whenever the user is choosing a Pokémon, Box, Bank, source, save, or destination, prefer showing the thing visually.

Examples:

- species picker shows `001 - Bulbasaur` with live sprite preview;
- Clone selects an actual destination box and empty slot;
- future Transfer/Copy chooses Game/Save → Box/Bank → destination visually;
- Vault/Banks show Pokémon, not opaque slot IDs;
- source selection shows recognizable game/source identity rather than filesystem paths as the primary interface.

Technical identifiers can remain available in provenance/diagnostics.

---

## Responsive previews without hidden mutation

The UI should respond immediately where it is safe to do so.

Examples:

- hovering Charizard in a species picker previews Charizard immediately;
- changing Level/DVs/Stat Exp can update calculated stats live when exact formulas are known;
- selecting a Move can preview PP/effect information;
- changing a destination highlights exactly where the Pokémon would go.

But preview state and committed/staged state are separate.

```text
hover / preview
    ≠
draft selection
    ≠
staged mutation
    ≠
source write
```

The user should get rich feedback without accidental state changes.

---

## Generation-aware, never fabricated

The same overall PokeBank NX experience should span generations, while each game exposes only fields that truly exist or are explicitly marked derived.

Examples:

- Gen I: HP / Attack / Defense / Speed / Special; no fake Sp. Atk/Sp. Def split.
- Gen I/II: no fabricated SID.
- Gen I: no native Nature, Ability, Held Item, modern met fields, PID, ribbons, or native shiny/gender fields.
- later generations expose their real fields through the same coherent design language.

The UI should adapt to the game rather than forcing every game into one modern schema.

---

## Truthful legality and provenance

PokeBank NX should never claim more certainty than the engine has proved.

Good states include:

```text
Legal
Illegal
Unknown
Not checked yet
Unavailable in current build
Structurally valid; encounter legality not checked
```

Do not label a generated Pokémon "legal" merely because it can be serialized.

Provenance should remain understandable to normal users while retaining expert evidence underneath.

---

## Compact when simple, spacious when useful

Dialog size should match content.

Four actions do not need a nearly full-screen panel.
A Box grid or full Pokémon editor can justify a large workspace.

General rule:

```text
small content  → compact action sheet/dialog
medium content → focused modal
complex editor → full intentional workspace
```

Avoid dead space, overlapping text, clipped rows, debug-looking dumps, and giant generic rectangles reused for every interaction.

---

## One PokeBank NX design language

Game-specific save engines remain separate internally, but the user should encounter a consistent app across:

- GB/GBC/GBA;
- DS/3DS;
- installed Switch games;
- RetroArch and standalone emulators;
- Master Vault;
- Named Banks;
- backups;
- transfers;
- legality/provenance.

The player should learn PokeBank NX once, not relearn a different editor for every generation.

Shared concepts should use shared components where practical:

- Pokémon Summary;
- Moves;
- Stats;
- Trainer;
- Boxes;
- Inventory;
- Pending Changes;
- destination pickers;
- confirmations;
- contextual footers;
- touch/controller action dispatch.

---

## Homebrew-community values

The project should take advantage of what homebrew can do better than a locked commercial product:

- offline-first operation;
- user-owned local data;
- preservation of older games and saves;
- broad emulator/source support;
- interoperability;
- transparent provenance;
- backups and recovery;
- optional advanced tooling;
- no artificial feature restrictions merely to mirror an official service;
- community-discovered quality-of-life ideas;
- practical workflows for real collections;
- clear documentation and open implementation.

Freedom does not mean recklessness. More capability should come with stronger safeguards.

---

## Design review checklist

Before accepting a new user-facing feature, ask:

1. Can a normal Pokémon player understand what this does?
2. Could a young player use it safely without save-format knowledge?
3. Is the common path simpler than the advanced path?
4. Are advanced capabilities still available when genuinely useful?
5. Does Cancel truly leave state unchanged?
6. Does preview stay separate from staged mutation?
7. Is the source save protected?
8. Are unsupported capabilities hidden or honestly explained?
9. Is the screen generation-correct?
10. Does it look like a polished console UI rather than a debug/editor struct dump?
11. Are controller and touch semantics obvious?
12. Does the active footer match the active screen/modal?
13. Is the dialog/workspace sized for its actual content?
14. Can the user choose important destinations visually instead of manipulating opaque numbers?
15. Does this fit the same PokeBank NX design language as the rest of the app?

If several answers are no, the feature is not finished even if the backend works.

---

## Final standard

The technical engine can be extraordinarily powerful.

The player-facing experience should make that power feel safe, obvious, and enjoyable.

**PokeBank NX is not trying to choose between Nintendo-style polish and homebrew power. The goal is to combine them: the usability and confidence of a first-party Pokémon tool, the freedom and preservation mindset of homebrew, and the depth expected from serious Pokémon save tooling.**
