# PokeBank NX — Gen VI–IX first owner hardware intake (experimental 5895e6b9)

**Assessment: NOT HARDWARE ACCEPTED. REJECT AS RELEASE CANDIDATE.**

- Branch: `development/gen6-plus`; candidate code `5895e6b9`; artifact `PokeBank-NX-Gen6Plus-Experimental-5895e6b9` (GitHub Actions run `41272664794`), NRO SHA-256 reported by build lane: `dc6427244aff9cee3016ddd930bf65075059dc152e126878b039e3faa9204b41`.
- Owner observed real Nintendo Switch UI, including Pokémon Y, Alpha Sapphire, Let's Go Pikachu, Legends: Z-A.
- **Evidence quality:** screenshots of actual app, owner verbal feedback, and repository capability matrix. No original saves supplied, no byte-for-byte source comparison, no current debug log. Do not claim a parser defect is conclusively proven where only the UI symptom is confirmed.
- All owner screenshots are included, unmodified. Find by filename in `SCREENSHOT_INDEX.md`.

## Owner's report (key substance)

> "gen 6-8 is a joke not even worth the check list i cant open pokemon y even tho i have it installedm alpha sapphire is broken, no proper trainer stuff or editor, all other gens like 7/8/9 either dont work or are read only"

## Confirmed UI and workflow observations

1. **Y rejects a candidate**: save selector says `Inspected 1 save file(s); none validated for this game. Example rejection: Gen VI save footer has no BEEF block-metadata magic`. This is direct hardware evidence of failed save opening, **not** proof that the precise rejected file was the installed game's valid save. Find path/provenance, identify copy format and compare with emulator bytes. Do not simply disable validation.
2. **Alpha Sapphire**: HOME card has `Trainer Billy`, source `Nintendo 3DS • Dekopon`, `Seen 0 / 721 • Owned 0 / 721`, and `No active party Pokémon`. Owner reports it broken. Verify expected party and Pokédex against actual emulator game; this might be stale assignment, wrong input, incorrect decoder, or a genuinely empty save. No root cause confirmed.
3. **Gen VI HOME art**: generic large Poké Ball artwork instead of title-specific cover. This is consistent with previous HOME-card art complaints.
4. **Let's Go Pikachu**: trainer facts displayed (name, IDs, gender, language, Pokémon stored `72/1000`) but Pokédex unavailable. Bag says `Installed-title reader shows pouches read-only; staged Bag needs a verified MyItem (block 0) decoder`. Missing native capabilities, not device-ready parity.
5. **Legends: Z-A**: visible 32-box grid and Pokémon previews, detailed Pokémon editor with correct-looking stats/IVs/EVs, title/source labelled `Unknown provider • Pokémon Legends Z-A • Read-only source`, and `LIVE LOCKED`. This demonstrates *some* valid read handling.
6. **Z-A editor gaps**: species row explicitly read-only, moves explicitly read-only because of missing verified learnset/PP writer, origin and met location unknown; trainer facts read-only; Pokédex unavailable; battle teams unreadable, box modifications refused. `Not check...` status labels are clipped.
7. **Z-A staging semantics**: STAGED and Keep/Reroll IVs/EVs are displayed while the *source* is live-locked. This can be a valid immutable-source design, but the UI must clearly state whether export is permitted, what fields are supported, whether the staged payload passes round-trip verification, and never claim live save is modified. No export or original save mutation shown.
8. **Unverified games**: these screenshots don't establish status of every Gen VII–IX game individually. Do not mark them FAIL or PASS without owner evidence. They remain NOT TESTED or LIMITED per implementation.

## Required repair sequence

**P0 Gate A — Gen VI access + data correctness.** Reproduce Y discovery/validation with the exact emulator save layout. Distinguish installed game card, valid save-file assignment, and wrong candidate. Verify Alpha Sapphire party/trainer/dex with a genuine owner-provided or independently obtained retail-compatible fixture. Preserve strict fail-closed parsing and original-save immutability.

**P0 Gate B — Modern box safety.** Z-A `Battle teams: Unreadable (box changes refused)` is correctly fail-closed today. Resolve parsing only with verified native evidence. Do not weaken team locks to make buttons work. Audit status of prior Sonnet D3 symlink-safe discovery and PKSE 1.2.2 modern writer safety tests at this exact NRO commit.

**P1 Gate C — Core shared editor parity.** Implement title-native species, forms, moves, PP, supported data fields, trainer, bag and native mechanics where safe; add real screenshots/records, field-diff and round-trip tests. Create, Move, Clone, Release, Review and Export Copy must have clearly stated, verifiable capability for *each title*. Do not imply all 18 are completed just because card and box views exist.

**P1 Gate D — Source/error UX.** For every unsupported operation distinguish `unsupported`, `read-only source`, `staged only`, `export blocked`, `missing fixture`, `validation failed`. Preserve full error text without clipping, correct provider labels and cover art, and eliminate ambiguity of LIVE LOCKED + STAGED.

**P1 Gate E — Truthful acceptance matrix.** Separate four distinct states: (1) host tests pass, (2) owner hardware opened authentic save, (3) exact feature parity implemented, (4) exported copy validated in game. Never equate 1 with 2–4. Reconcile every older Gen I–V requirement, especially common Actions menu and Vault transfer, into Gen VI–IX inheritance plan. Those are v1.0 feature blockers until implemented.

## Safe QA protocol

- Freeze experimental `5895e6b9` as failed hardware candidate, keep screenshots and issues; don't silently relabel it accepted.
- Do not test risky live writes. Source saves stay immutable; test exports only using copies with independent checksums and game-level reopen verification.
- No PR merge to main or Round 12 before acceptance. Keep legality engine separate until parity gate.
- New candidate must provide exact commit, SHA-256, GitHub CI artifact, repeatable fixture tests and **specific owner retest items**, not a generic 40-item checklist covering features which don't yet work.
- Do not demand that the owner publicly upload personal save contents; accept private test fixture consent or owner-only diagnostics.

## Release acceptance

Overall outcome: **NO-GO for Gen VI–IX as a v1.0-ready editor.** A successfully built NRO and passing host tests remain useful engineering milestones but are not completion evidence.

See `ISSUE_LEDGER.csv` for stable findings and `SCREENSHOT_INDEX.md` for screenshot traceability.
