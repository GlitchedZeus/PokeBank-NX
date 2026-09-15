# Issue #71 — Minimal safe P0 change list before Gen III

This is the implementation order after the audit. **It does not authorize Gen III Pokémon editing.**

## P0 — required before Gen III product work

1. Add pure, generation-neutral descriptor types for:
   - per-field state: `Hidden`, `Derived`, `ReadOnly`, `Editable`;
   - exact-format editor identity/capabilities;
   - native storage semantics (`PackedNative`, `SparseOwned`, `Other/AdapterDefined`);
   - stat presentation labels/count and stored-vs-derived status;
   - trainer-field support/editability;
   - move-compatibility query/result including `Compatible`, `PreserveExisting`, `Unsupported/Invalid`;
   - storage-operation capability sufficient to host future shared Move/Multi without direct legacy mutation.
2. Keep semantic actions separate from storage state: a DERIVED field such as Gen I/II Shiny may have a safe `Make Shiny` transformer without pretending Shiny is an independently stored editable field.
3. Add an adapter/provider interface that exposes those descriptors and normalized read-only/editor-view values without exposing PKSM/PK1/PK2 concrete implementation types.
4. Adapt **Gen I and Gen II only** to the descriptors first, as a no-product-change proof. Their device-accepted behavior must remain byte/UX-equivalent.
5. Add contract/host tests proving:
   - Gen I remains five-stat / one Special / packed-native;
   - Gen II remains its accepted DV/Stat Exp/derived-stat semantics / packed-native;
   - exact move compatibility remains game-specific;
   - unsupported capability fields remain hidden;
   - derived fields remain identified as derived even when semantic actions exist;
   - source mutation remains false for every current source kind;
   - no new Gen III Create/Edit route exists;
   - legacy Move/Multi mutation is not made a source-save bypass.
6. Keep `SaveEdit::Capabilities` separate from shared Pokémon-field capabilities; test the explicit bridge.
7. Re-run full host, ASan/UBSan and native compile/candidate regressions for accepted Gen I/II before declaring P0 complete.

## P1 — high value before/while opening the later Gen III milestone

- normalize the shared move-picker provider around exact game/species/form while retaining current Gen I/II data implementations;
- extract/refactor inherited PKSE Move/Multi carry/rectangle interaction into a PokeBank-owned storage controller, connected only to staged/native-safe providers;
- stabilize trainer presentation descriptor use in current read-only screens;
- add explicit Ball/Language/SID/Marks/collection capability vocabulary only where supported by exact adapters and shared UI;
- formalize provenance/legality read-only result descriptors (`Unknown` is valid);
- document/contract the Gen III read model -> shared View mapping before any edit mutation code.

## P2 — may wait

- dedicated Ribbon/Marks collection editor screens;
- expert PID correlation helpers;
- advanced legality/Quick Legal integration;
- conversion execution UI;
- performance cleanup of old unreachable compatibility overlay code;
- broad TrainerViewScreen architectural cleanup.

## DO NOT TOUCH

- accepted Gen I runtime/editor behavior;
- accepted Gen II runtime/editor behavior;
- Gen I/II packed native boxes;
- source-write hard locks;
- current Gen III read-only parser/product routing;
- live writes;
- party mutation;
- unrelated Vault/Banks issues;
- historical accepted/rejected artifact identities.

## P0 stop condition

P0 is complete only when the descriptor/provider foundation is green under permanent host/sanitizer/native tests and there is demonstrably **no new Gen III product editing behavior**. Then a separate, explicitly authorized Gen III milestone may consume that interface.
