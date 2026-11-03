# Gen I UX Cleanup Pass #3 — physical Switch follow-up

Cleanup #2 was physically tested on a real Nintendo Switch from exact production source `6ecd9c28dbe12a4b5c4b2810a8e31cb07c0485ac` using `PokeBank-NX-Gen1-UX2-Retest-6ecd9c28.nro` (SHA-256 `b08de66795902e0cef01273dca5883ab823e21df5f844844ff1cf9abaf0ab012`).

## Hardware result

- Functional editor: **PASS**
- Cleanup #2 UX direction: **strongly accepted**
- Final Gen I device acceptance: **not yet — Cleanup #3 retest required**

## Focused Cleanup #3 requirements

1. Add Left Stick Up/Down navigation with comfortable held repeat to long Gen I lists/pickers while preserving D-pad navigation and existing L/R page jumps. Neutral/deadzone input must not drift.
2. Replace the normal Add/Create move picker’s full 0–165 enumeration with a reusable, game-aware Generation I species/move compatibility model for Red/Blue/Yellow. New drafts must fail closed at Stage Add if any retained move is incompatible. Existing/imported unusual moves must be preserved unless explicitly changed.
3. Reuse/refactor the mature inherited PKSE-style Pokémon detail experience for the normal read-only `View Pokémon` path, adapted truthfully to Gen I. Preserve the new Cleanup #2 five-stat radar Summary as a reusable PokeBank overview component rather than deleting it.
4. Add cached Pokémon sprite/icon rendering to occupied Clone destination slots while preserving the accepted same-save contiguous-safe destination rules and browse/cancel no-mutation behavior.
5. Make Pending Changes content-aware: compact for one change, moderately sized for a few changes, capped + scrolling for long lists.

## Product philosophy

All work remains governed by `PHILOSOPHY.md` and `docs/PRODUCT_UI_PHILOSOPHY.md`:

> Nintendo-level polish. Homebrew-level freedom. PKHeX-class power. Kid-safe by default.

Normal users should not need move IDs, save record layout, compatibility bitfields, checksums, or serializer knowledge to use the app safely.

## Safety invariants

- Original RetroArch source: **IMMUTABLE**
- Live RetroArch write: **HARD DISABLED**
- Live installed-game write: **HARD DISABLED**
- Live other-emulator source write: **HARD DISABLED**
- Party edit: **DEFERRED**
- Add before explicit Stage Add: **NO STAGED MUTATION**
- Species hover: **NO STAGED MUTATION**
- Move-picker hover/navigation: **NO STAGED MUTATION**
- Clone browsing/cancel: **NO STAGED MUTATION**
- Unsupported/unknown editable layouts: **FAIL CLOSED**
- Full encounter legality engine: **DEFERRED**

## Acceptance

Do not mark Gen I `DEVICE ACCEPTED` until the owner physically tests and accepts a new exact source-addressed Cleanup #3 NRO.
