# Generation II GSC read-only oracle — 2026-09-11

This note freezes format facts for the PokeBank NX Gold/Silver/Crystal read-only milestone. It is not an authorization for writes.

## Independent references

- PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`: `Sav2`, `PK2`, Gen II inventory helpers.
- PKHeX `77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5`: `SAV2`, `SAV2Offsets`, `PokeList2`, `PK2`, `ItemStorage2`, `PlayerBag2`, `SaveUtil`, `SaveHandlerFooterRTC`.

## Structural identity

Gold and Silver share the same save structure. Neither oracle proves a reliable byte-level Gold-vs-Silver marker; both classify the family as GS. PokeBank NX therefore requires a trusted release hint to bind a structurally valid GS-family source as `gold_gbc` or `silver_gbc`; an unhinted GS-family save remains identity-ambiguous.

Crystal is structurally distinguishable by its party/current-box/list offsets and checksum layout. A Crystal hint must match Crystal structure; a GS hint must match GS-family structure.

Supported milestone regions are International and Japanese. Korean Gold/Silver is structurally recognized by both oracles but is deliberately unsupported until the Hangul text path is separately implemented and tested; Korean Crystal does not exist in the oracle layout.

## Normal payload / wrapper sizes

PKHeX recognizes Gen II raw SRAM payloads of `0x8000` and `0x10000` bytes. Japanese Crystal uses data at `0x8000`, so a 64 KiB payload is required for the Japanese Crystal layout in PokeBank NX.

Known appended emulator RTC footers are normalized read-only without altering source bytes when the footer is 7 bytes or an even size from `0x0C` through `0x30`, matching PKHeX `SaveHandlerFooterRTC`. Unknown trailing data remains unsupported rather than being silently trimmed.

## Core offsets

International Gold/Silver:

- trainer/TID `0x2009`; money `0x23DB`; current-box index `0x2724`; box names `0x2727`; party `0x288A`; current-box copy `0x2D6C`; checksum range ends `0x2D68`; primary checksum `0x2D69`; secondary checksum `0x7E6D`.
- inventory: TM/HM `0x23E6`, Items `0x241F`, Key Items `0x2449`, Balls `0x2464`, PC Items `0x247E`.

International Crystal:

- trainer/TID `0x2009`; money `0x23DC`; current-box index `0x2700`; box names `0x2703`; party `0x2865`; current-box copy `0x2D10`; trainer gender `0x3E3D`; checksum range ends `0x2B82`; primary checksum `0x2D0D`; secondary checksum `0x1F0D` per current PKHeX.
- inventory: TM/HM `0x23E7`, Items `0x2420`, Key Items `0x244A`, Balls `0x2465`, PC Items `0x247F`.

Japanese Gold/Silver:

- trainer/TID `0x2009`; money `0x23BC`; current-box index `0x2705`; box names `0x2708`; party `0x283E`; current-box copy `0x2D10`; checksum range ends `0x2C8B`; primary checksum `0x2D0D`; secondary checksum `0x7F0D`.
- inventory: TM/HM `0x23C7`, Items `0x2400`, Key Items `0x242A`, Balls `0x2445`, PC Items `0x245F`.

Japanese Crystal:

- trainer/TID `0x2009`; money `0x23BE`; current-box index `0x26E2`; box names `0x26E5`; party `0x281A`; current-box copy `0x2D10`; trainer gender `0x8000`; checksum range ends `0x2AE2`; primary checksum `0x2D0D`; secondary checksum `0x7F0D`.
- inventory: TM/HM `0x23C9`, Items `0x2402`, Key Items `0x242C`, Balls `0x2447`, PC Items `0x2461`.

The primary checksum is a little-endian 16-bit byte sum from `0x2009` through the layout-specific checksum-end offset inclusive. PokeBank NX validates the primary checksum as critical save evidence. Secondary/backup copies are preserved as evidence but are not used as a repair source.

## Boxes and party

- International: 14 boxes × 20 slots.
- Japanese: 9 boxes × 30 slots.
- Stored PK2 body: 32 bytes; party PK2 body: 48 bytes.
- Trainer/nickname string lengths: International 11 bytes; Japanese 6 bytes.
- Party capacity: 6.
- Stored box banks are authoritative. PKHeX explicitly warns that the separate current-box copy can be stale/desynchronized (for example via Stadium). PokeBank NX reads boxes from the stored banks and uses only the current-box index for selection; it does not duplicate the current-box copy into storage.
- Stored box bank split: International first 7 boxes at `0x4000`, remaining at `0x6000`; Japanese first 6 at `0x4000`, remaining at `0x6000`.

## Inventory

Real Gen II stores are:

1. TM/HM: 57 count bytes (no list count/terminator).
2. Items: capacity 20, `[count][id,qty]...0xFF`.
3. Key Items: capacity 26, `[count][id]...0xFF`, implicit quantity 1.
4. Balls: capacity 12, `[count][id,qty]...0xFF`.
5. PC Items: capacity 50, `[count][id,qty]...0xFF`.

Normal quantities are 1–99; HMs are effectively one. Crystal adds key-item IDs 70, 115, 116 and 129 relative to GS. Raw Gen II IDs must use dedicated Gen II naming; unused IDs must not fall through to modern item names.

## PK2

Stored fields include species, held item, four moves, TID, EXP, five Stat Exp values, 16-bit DVs, four packed PP/PP-Up bytes, friendship, Pokérus and Crystal caught-data. Party records append status, current/max HP and battle stats.

Shiny is DV-derived. Gender is species-ratio + Attack-DV-derived. Unown form is DV-derived. There is no stored SID, Nature, Ability, modern EV model, modern IV model, modern ribbons/marks or HOME tracker. The read-only presentation must not fabricate those fields.

## Safety

All parsing is bounds-checked and read-only. No checksum fixing, repair, source serialization or writeback is permitted. Malformed/truncated/unsupported sources fail safely. Optional inventory may be isolated from otherwise-valid Trainer/Party/Boxes only when the inventory decoder can fail without altering the critical save model.
