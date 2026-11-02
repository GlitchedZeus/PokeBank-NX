# Scarlet/Violet and Legends: Z-A

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Ready for design, not certified exports

Current PKHeX has **PK9/SAV9SV** and **PA9/SAV9ZA**. NX already has legacy `Pokemon9SV`, `Pokemon9LZA`, `Encryption9SV`, `Encryption9LZA`, Trainer/Inventory classes and SC read gates. New strict staged adapters do not yet exist in the audited native lanes. Reuse readers/accessor knowledge, and put new editing behind the shared app interfaces.

## Scarlet / Violet

[SAV9SV](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV9SV.cs) uses SwishCrypto with full 0x158 party-format PK9 in all 32×30 box slots, and separate six-slot Party. SaveRevision is inferred from BlueberryPoints block presence or Kitakami raid data. `SaveUtil.IsSizeGen9SV` has historical fixed sizes **and bounded DLC ranges**; metadata includes raw size constants, not a replacement for the predicate. Patches/DLC not recognized by inspected schema stay UNKNOWN.

[block keys](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Access/SaveBlockAccessor9SV.cs) supplies distinct status, party, items, Pokédex and raid keys. Use generated SC key index; encrypted-file offsets are not stable field offsets. Identify Scarlet/Violet using MyStatus game's native value and schema, not `.main` name or assigned title alone.

[PK9](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PKM/PK9.cs) stores original nature and separate StatAlignment (mint result), HyperTrainFlags, tera original at `0x94` and override at `0x95`, handler/OT fields, Home tracker, form arguments, ribbon/mark flags, encounter metadata and move/TR records. These offsets are directly from accessors; item/width/string semantics still need native masks. TeraTypeOverride has a special none sentinel; unrestricted byte editing is unsafe. Hyper Training changes effective stats without overwriting genetic IVs. Preserve original nature when applying mints. Encounter-specific tera/shiny/scale rules require encounter/RNG proof, not generic shiny toggles.

Eggs/breeding and move chronology belong to SV learn/breed/encounter source rules. Do not infer a valid egg from an IsEgg bit, and do not clear HOME/handler fields to make an invalid record appear native. Native move list must reflect revision and species/form, including transferred move reset/history behavior. Existing memory/ribbon/mark bytes can remain read-only while coverage is incomplete.

Trainer `MyStatus9`, Bag `PlayerBag9`/InventoryItem9/Pouch9, `Zukan9` plus DLC dex components, `PersonalTable9SV`, and `Legality/Encounters/Generator/EncounterGenerator9` are mapped in the source index. Full production membership/quantity/evolution/encounter data extraction remains pending.

## Legends: Z-A: independently checked format identity

[PA9](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PKM/PA9.cs) implements Gen9a, Decrypt8-sized entity core, alpha/scaled size, separate original nature/StatAlignment, Hyper Training and **Plus** move flags. It is not PK9 and does not implement ITeraType. Do not map PK9 tera offsets onto PA9. An accessor inherited from the cross-game PKM model does not prove the mechanic is available natively.

[SAV9ZA](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV9ZA.cs) uses SwishCrypto and 32×30 boxes, with **0x40 bytes after each 0x158 entity** (`SIZE_BOXSLOT = 0x198`); party stride is `0x158+0x88 = 0x1E0`. `WriteSlotBox` sets a presence byte at `0x158` for nonzero SaveRevision, and `SetPartySlotAtIndex` updates slot footer. Exporting contiguous 0x158 records into this container would overwrite or misalign native metadata.

[KParty / KSaveRevision / KMyStatus / KItem](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Access/SaveBlockAccessor9ZA.cs) differs from SV: Party `0x3AA1A9AD`, status `0xE3E89BD1`, item `0x21C9BD44`, revision `0x0926555A` (UInt64). Box key is `0x0D66012C`. Current revision values in SAV9ZA are 0 Base, 1 Mega Dimension, 2 EOL as **upstream labels**, not a claim of hardware testing or comprehensive patch coverage. Known sizes are listed in the matrix; unknown revision/geometry must refuse edits even if hash verifies.

PA9 Plus flag arrays span `0xD6`/33 bytes and `0x94`/12 bytes (`PlusStart0/1`, `PlusCount0/1`); the latter overlaps PK9's tera area semantically. Alpha byte `0x23`, form `0x24`, and scale `0x4A` are separate native fields. Native encounter/evolution/form-changing/mega-state and shiny restrictions must be sourced from Gen9a-specific legality sources. Do not carry SV breeding or PLA mastery assumptions into ZA.

## Historical transfer restriction

The official Z-A HOME announcement says Z-A visits prevent return to older games. Current HOME page lists Z-A connectivity. These are current official service facts, not bytes proving an individual Pokémon's travel history: [official announcement](https://legends.pokemon.com/en-us/news/pokemon-home), [current HOME games](https://home.pokemon.com/en-ca/move/). Preserve unknown history as Unknown/Incomplete; no direct SV↔ZA route is invented.

## Readiness checklist

- Reuse strict SC crypto from #312 with per-family keys and geometry; compare existing legacy SC read gates before deduplicating.
- Build revision-aware catalog and native field maps; expose only reviewed fields, keep progression/raids/events read-only.
- Add stored/party PK9 and PA9 codec parity through existing readers; do not create a fourth shared Decrypt8 implementation.
- Add wrong-family same-size tests, ZA gap/footer/presence tests, SV tera sentinel/mint/Hyper Training tests, item representation and absent DLC blocks.
- Add exact diff containment and source immutability, independent reparse, companion preservation, bounded memory benchmarks.
- No authentic test corpus, Switch 2 save-equivalence proof, exhaustive DLC rules or native export acceptance is claimed. Device-produced diagnostics and private copy acceptance remain necessary.
