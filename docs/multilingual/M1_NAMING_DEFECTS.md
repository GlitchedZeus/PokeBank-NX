# Multilingual Gen I–V: milestone M1 (D1–D4 naming defects)

**Status: NOT DEVICE TESTED.** Host tests only. Branch `feature/multilingual-gen15`, from the safety
branch `wip/pkse-1.2.x-gen15-audit` at `6ec5ccb9`. Japanese save editing stays read-only; nothing here
lifts that gate. No NRO has been built from this branch.

Each fix landed with a regression test that fails without it (shown in the commit message).

| # | Defect | Fix | Source | Tests |
|---|---|---|---|---|
| D1 | Gen III: changing Language and Nickname together applied the nickname in the old language's encoding | Language is applied first. On a language-only change a default name becomes the new language's default; a custom name is kept only if it reads back unchanged in the new encoding, otherwise the edit is refused and nothing is written | PKHeX `SpeciesName.GetSpeciesNameGeneration`, `StringConverter3` | `test_gen3_staged_pokemon_editor` (EN→FR→JP, DE→ES, refused JP with unchanged bytes) |
| D2 | Gen III/IV Create used the English default name for every language | `Names::defaultSpeciesName(species, language, generation)`: Japanese/Korean as listed; others upper case; French E/I accents stripped; Farfetch'd with a straight apostrophe | PKHeX `SpeciesName.cs:114-200` | `test_default_species_names` (386×6 Gen III names encodable and in limits), Create defaults in `test_gen3_staged_pokemon_editor` |
| D3 | Gen V: changing Language on an unnicknamed Pokémon kept the old language's name | The nickname becomes the default name in the new language, written by `writeUnicodeName5` (UTF-16, 0xFFFF terminator, ♂/♀ half-width remap, trailing units kept). A nicknamed record changes only byte 0x17. Keep and Review replay Language before names, so the derived name is not mistaken for a name edit | PKHeX `PK5`, `StringConverter5.SetString` | `test_gen5_language_default_name`; `test_gen5_staged_save_candidate` (language plus explicit nickname, both orders, Keep and Review); `test_gen5_pk5_readonly` updated to the PKHeX rule with every other byte pinned |
| D4 | Gen I/II international names refused every accented letter | Gen I: all accents of its single international table. Gen II: only the accents whose byte is the same in the English, French/German and Italian/Spanish tables (àèéù ÄÖÜäöü ÙÚá). The others are refused with a reason, because a Gen II save does not record which European release wrote it. Gen I length now counts characters, not UTF-8 bytes | PKHeX `StringConverter1.TableEN`; `StringConverter2.TableEN/TableFRE/TableITA` | `test_rby_pokemon_editor`, `test_gsc_pokemon_editor` |

## Known limits carried into M2

- Gen II decoding uses the English table only. On a French/German save, bytes 0xBE–0xBF, 0xC6–0xCC and
  0xD0–0xDF display as the English-table letter. M2 adds a per-save European table choice.
- Gen I/II localized default names are not covered yet (M2/M3).
- Gen V Unicode nickname entry is still ASCII-only in the editor; only the language-derived default name
  uses the full UTF-16 writer. Unicode entry waits for the glyph rules (owner decision).
- D4 covers the native encoders. The on-screen keyboard does not offer accented keys yet; target-aware
  keys, limits and a kana page are a later phase (owner-approved order).

## Round 12 (R12-008 / R12-009): language changes, Gen III–V

**Status: NOT DEVICE TESTED.** Owner H19: "gen 4 should be how all gens work". Gen IV's rule is the model:
a Language change rewrites only the language byte and the names; an un-nicknamed name becomes that
language's default; PID, IVs, EVs, stats and every other byte stay; a name the language cannot hold
is refused with a message and nothing is staged.

| Gen | What changed | Tests |
|---|---|---|
| III | Create in Japanese failed its strict reparse because a Latin OT reads back full-width (`ＲＥＤ`), so the exact preview failed and the screen fell back to the unbuilt draft: PID 0, stats "—", radar 0, name "Pikachu" (R12-008). Names are now compared with `Utils::gen3FoldWidth` (the Latin bytes are the same in both tables). An Edit's language change also re-stores the OT in the new table and refuses an OT it cannot hold (before, an accented OT was silently reread as other glyphs). The Language cycle offers only a language whose exact preview succeeds, skipping the others with the reason. Korean is never offered and the cycle says so on the Spanish → Japanese wrap | `test_gen3_staged_pokemon_editor` `languageStabilityGame` (all five games, Create and Edit, EN→JA→EN, byte compare, export → reparse, OT refusals); `test_shared_editor_unit` |
| IV | No failure mode: `Pokemon4Mutable::setLanguage` writes only 0x17 and the nickname and verifies the OT | `test_gen4_mutable_pokemon` (every Platinum language and back) |
| V | Create always set the nickname flag (ASCII "Pikachu"), so the M1 D3 rename never ran on a created Pokémon. Create now makes an un-nicknamed record (`CreateRequest5::defaultNickname`) | `test_gen5_party_box_parity` `languageFollowsDefaultName5` (BW/B2W2, all seven languages, byte compare, Keep → Review replay → export → reparse) |

What each format cannot store:

- **Gen III (PK3):** languages Japanese, English, French, Italian, German, Spanish only (no Korean game, no
  Hangul in either table). Two 256-byte tables: Japanese (kana, full-width Latin) and international (Latin
  with accents). A name moves between them only if every character exists in the target table; Latin
  letters, digits and basic punctuation do (drawn half-width or full-width), accents and kana do not.
  Japanese names: nickname and OT 5 characters; others 10 / 7.
- **Gen IV (PK4):** one table for every language; Korean exists (Hangul) in DP/Pt/HGSS. Names keep their
  characters across a language change.
- **Gen V (PK5):** UTF-16, every language including Korean. A default name follows the language. Typed
  nicknames are ASCII-only until the Gen V glyph tables are verified (owner decision); display shows any
  stored name.
