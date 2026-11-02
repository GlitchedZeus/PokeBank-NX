# Shared UX requirements: one user experience, per-format codecs underneath

**Status: AUDIT + ENGINEERING PLAN. No product code changed. NOT DEVICE TESTED.**
Audited 2026-11-02 on `fix/round13-gen15-repairs` @ `3d9e979c` (Gen I–V) and read-only on
`origin/development/gen6-plus` @ `27c7c1b8` (Gen VI–IX, paused). Every claim below cites the code
it was read from. Line numbers are for those two SHAs.

Owner direction (authoritative, 2026-11): *stop implementing basic functionality separately for
every generation; one polished shared user experience with generation-specific save codecs,
validation and serialization underneath; never truncate, corrupt or silently replace characters;
do not invent unsupported fields or loosen save-safety rules; fix the shared component once and
add regression tests covering every applicable format.*

Contents

1. [Findings in one page](#1-findings-in-one-page)
2. [Target architecture](#2-target-architecture)
3. [Text-entry surface inventory (Gen I–IX)](#3-text-entry-surface-inventory-gen-iix)
4. [Native character repertoire per format vs. what the UI lets you type](#4-native-character-repertoire-per-format-vs-what-the-ui-lets-you-type)
5. [Shared-UX architecture map (all owner-listed areas)](#5-shared-ux-architecture-map-all-owner-listed-areas)
6. [Prioritized work list (P0 / P1 / P2)](#6-prioritized-work-list)
7. [What a format cannot safely support yet (exact missing implementation)](#7-what-a-format-cannot-safely-support-yet)
8. [Completion is not](#8-completion-is-not)
9. [Owner Round 13 hardware FAILs mapped to shared components](#9-owner-round-13-hardware-fails-9c301c48-mapped-to-shared-components)

---

## 1. Findings in one page

**Active P0 (data safety), reproduced on host:**

* **P0-1 Gen III Egg OT is silently truncated and exported.** `Gen3StagedPokemonEditor.cpp:571-578`
  validates a new OT with `Pokemon3FRLG::canStoreNickname` (the *record* language, which an Egg
  forces to Japanese, `Pokemon3FRLG.h:164-174`) but `Pokemon3FRLG::setOTName`
  (`Pokemon3FRLG.cpp:102-104`) encodes with `otTextLanguage()` (the *save* language for an Egg), and
  `g3EncodeName` (`Pokemon3FRLG.cpp:89-100`) stops at the first byte that table lacks (`break`). The
  edit path has no read-back check (Create has one, `:879`). Repro (host probe built from
  `tests/test_gen3_staged_pokemon_editor.cpp` fixtures, FireRed English save, Egg in box 1 slot 1):
  `stageBoxPokemonEdit(0,0,{otName="A×B"})` → accepted, no error, OT stored and exported as `"A"`;
  same for `"A…B"` and `"AいB"`; `"BOB"` is fine. `×` and `…` are on the in-app keyboard's
  Symbols page (`KeyboardDialog.cpp:229`), so no System keyboard is needed. On device: Gen III save →
  Egg → Edit → OT → `?123` page → type `A×B` → OK → Review shows `A` → Export writes `A`.

**Latent P0 (truncating writers that today rely on an upstream cap; one cap change away from
corruption):** `Pokemon3FRLG.cpp:93-95` (break at unencodable), `Trainer3FRLG.cpp:46` (break at
`maxChars`), `Pokemon4Mutable.cpp:394-395` (`substr` to max before the round-trip check, so the check
cannot see the cut), `Gen4TextCodec.h:64,57` (`min()` cut and `?` substitution), legacy
`Utils::setString` `StringHelpers.cpp:147-152` (cuts in UTF-16 units while every keyboard caps in code
points, so a non-BMP character splits a surrogate pair; 23 call sites; the checked
`setStringChecked` has zero callers), `Convert.cpp:868-869` (cross-group conversion writes a blank
Gen III OT when the OT is unrepresentable, with no `Report` adaptation), and
`Gen4ReadOnlyTrainer.h:27,38` (`supportsBoxNames()==true` with a no-op `updateBoxNameBlock`; only
the `SourceKind` gate stops a silent lost rename). One non-save P1: the backup-name prompt silently
strips characters (`TrainerViewScreenBase.inc:1770-1774`).

**Keyboard: one renderer, no format contract.** There is one in-app keyboard
(`src/UI/Dialogs/KeyboardDialog.cpp`, `Utils::promptText` family in `src/Utils/Keyboard.cpp`) and a
**second, separate keyboard** inside every chooser search (`include/UI/SharedSearchInput.h`, A–Z/0–9
only). The in-app keyboard knows nothing about the target format. It offers the same Latin, Symbols
and Accents pages for every field and caps length in code points. Script pages are switched by two
ad-hoc booleans (`KeyboardRequest::japaneseKana`, `koreanHangul`, `Keyboard.h:47-53`). Japanese kana
pages exist only for Gen I/II, and those are unreachable while
`JapaneseEditingGate.h:13` is `false`. Hangul exists only for Gen IV Pokémon names. Every writer
then re-decides what is legal, in 11 different encoders/validators (§4). Gen III Japanese records,
Gen IV Japanese records, Gen IV Korean trainer names, Gen V and all of Gen VI–IX get a Latin
keyboard although the format stores more. The keyboard is also **forked between branches**:
`gen6-plus` carries an older `KeyboardDialog.cpp` / `Keyboard.h` (147 lines different, no Hangul,
no `promptKoreanName`), so a fix on one branch does not reach the other.

**Duplication.** Gen I and Gen II each have a stack of layered editor `.inc` files compiled into one
translation unit with rename macros (`TrainerViewScreenCompositeOverlay.cpp:46-181`). Gen I name
prompts exist in 5 copies (4 live layers plus one compiled but unreachable file,
`Gen1PokemonEditorOverlay.cpp`). Gen II has 2 parallel paths (the shared wrapper and the preserved
`TrainerViewScreenGSCOverlay.inc` with direct `promptText`). Gen III has its own 2,822-line surface
(`Gen3SharedPokemonSurface.inc`) that re-implements what `SharedPokemonEditorSurface.inc` provides
to Gen IV/V/VI+. Gen V has its own trainer screen. Gen VI–IX has 5 copies of the same ASCII-only
`allowedName` validator.

**Surface counts (live text-entry surfaces; §3 has every row):**

| Gen | Live surfaces | SHARED + OK | DIVERGENT | ASCII-ONLY-BUT-FORMAT-SUPPORTS-MORE | TRUNCATES/REPLACES | Missing (real field, no surface) |
|---|---|---|---|---|---|---|
| I | 3 (nickname, OT, trainer) | 3 (Latin; kana gated) | 0 live, 4 dead call sites | 0 (JP gated) | 0 | rival name |
| II | 4 (nickname, OT, create nickname, trainer) | 3 | 4 legacy call sites (GSC overlay) | 0 (JP gated) | 0 | rival name, 14 box names |
| III | 3 (nickname, OT, trainer) | 1 (trainer) | 2 (own surface) | 2 (JP records: no kana page) | **1 (OT, P0-1)** | box names |
| IV | 3 (nickname, OT, trainer) | 3 (Hangul for KO Pokémon only) | 0 | 3 (JP records; KO trainer) | 0 active (1 latent) | box names (latent trap) |
| V | 3 (nickname, OT, trainer) | 0 | 1 (trainer screen) | 3 | 0 | box names |
| VI–IX (gen6-plus) | 3 (nickname, OT, save path) | 1 (path) | 5 validator copies + forked keyboard | 2 | 0 active | trainer name, box names |
| App-level (all gens) | 6 (species search, held-item search, bank box rename, backup name, Gen IV/V save path) | 2 (save paths) | 2 (search keyboard, two normalizers) | 1 (bank box) | 1 (backup name strips, non-save) | move / location / ability / bag-add / Vault search |
| Legacy PKSE Switch titles on this branch | 3 (nickname, trainer, save box) | 0 | 3 | 3 | latent (setString) | — |

---

## 2. Target architecture

```
           ┌──────────────────────────── ONE shared UX layer ─────────────────────────────┐
           │ Keyboard (Latin / Accents / Symbols / Kana / Hangul / full-width pages)       │
           │ Choosers (species, move, item, ability, nature, ball, met location, language) │
           │ Search (one normaliser, one key grid = the keyboard in search mode)           │
           │ Editor workspace, Party/Box grid + carry, Trainer page, Bag, Review, Export   │
           └───────────────▲───────────────────────────────▲───────────────────────────────┘
                           │ asks, never decides            │ stages, never writes source
           ┌───────────────┴──────────┐        ┌────────────┴───────────────────────────────┐
           │ Format capability layer  │        │ Session / backend contract                 │
           │ TextFieldCapability      │        │ (NativeSession on gen6-plus is the model:  │
           │ FieldState / maxima      │        │  view, fieldState, stageValue, stageText,  │
           │ chooser value domains    │        │  clone/move/remove/create support, bag,    │
           └───────────────▲──────────┘        │  reviewRows, export)                       │
                           │                   └────────────▲───────────────────────────────┘
           ┌───────────────┴───────────────────────────────┴───────────────────────────────┐
           │ Per-format codecs + validators + serializers (Gen1/2 charmaps incl. kana,     │
           │ Gen3 EN/JP tables, Gen4 16-bit table incl. Hangul, Gen5 UTF-16 + glyph rules, │
           │ Gen6/7 UTF-16 + PKHeX remaps, Gen8/9 UTF-16), strict reparse, export          │
           └───────────────────────────────────────────────────────────────────────────────┘
```

Rules:

1. **The UX layer never encodes and never decides legality.** It asks the capability for each
   field what is legal, and the capability answers from the same tables the writer uses.
2. **A writer never truncates, substitutes or drops.** Every name writer is all-or-nothing. It
   returns a refusal with a reason, and it re-reads what it wrote and compares, as
   `Gen1StagedPokemonEditor::encodeRegionName` (`Gen1StagedPokemonEditor.cpp:188-223`) and
   `Gen3StagedTrainerEditor::stageTrainerName` (`Gen3StagedTrainerEditor.h:135-165`) already do.
3. **One length unit per field.** The capability states the cap in the unit the game counts
   (characters as PKHeX `MaxStringLength*`). It also reports the native storage units, so the
   keyboard's counter, the writer's cap and the reparse all agree.
4. **The System keyboard is an input source, not an exemption.** Its result goes through the same
   `check()` before the prompt accepts it. Today `KeyboardDialog.cpp:800-815` accepts whatever swkbd
   returns.
5. **No RTTI, no exceptions.** Interfaces are abstract classes with virtual functions (already used
   by `Modern::NativeSession`) or `constexpr` tables. Errors are enums plus a static reason
   string. Formats are looked up by id (`sourceGameId`, language, region), never by `dynamic_cast`
   (see `ClassicJapaneseNames.h:19-31` for the established pattern).

### 2.1 Proposed interface: `include/Text/TextFieldCapability.h`

```cpp
#pragma once
// One question the shared keyboard / choosers ask a format about one text field.
// Device build: -fno-rtti -fno-exceptions. No allocation on the hot path except decode().
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace PokeBank::Text {

enum class FieldKind : uint8_t {
    Nickname, OriginalTrainer, TrainerName, RivalName, BoxName, // save-native
    BankBoxName, BackupName, SaveFilePath,                      // app-owned
    SearchQuery,                                                // never stored
};

enum class Refusal : uint8_t {
    None, Empty, TooLong, UnsupportedGlyph, ScriptNotForLanguage,  // e.g. Latin into JP Gen I
    ReleaseDependentGlyph,  // Gen II European accents (Gen12AccentText.h)
    RegionGateClosed,       // JapaneseEditingGate.h
    FieldReadOnly,          // e.g. JP Gen III trainer name
    TooManyDigits,          // PKHeX TrainerNameVerifier.ContainsTooManyNumbers
    RoundTripMismatch,      // encode -> decode != input
};

struct TextVerdict {
    Refusal refusal = Refusal::None;
    uint16_t characters = 0;     // game-visible characters (the unit maxCharacters() uses)
    uint16_t nativeUnits = 0;    // bytes (Gen I-III) or 16-bit units (Gen IV+) the encoding needs
    uint16_t firstBadIndex = 0;  // code-point index of the first refused character
    char32_t firstBad = 0;
    [[nodiscard]] bool ok() const noexcept { return refusal == Refusal::None; }
};

/// Which keyboard pages make sense. The keyboard still greys every key glyphAllowed() refuses.
struct KeyboardProfile {
    bool latin = true, accents = false, symbols = true;
    bool hiragana = false, katakana = false, fullWidthLatin = false, hangul = false;
    bool fullWidthSpace = false;           // Space types U+3000 (Gen I/II JP, Gen III JP)
    enum class Start : uint8_t { Latin, Kana, Hangul, Digits } start = Start::Latin;
};

class TextFieldCapability {
public:
    virtual ~TextFieldCapability() = default;
    [[nodiscard]] virtual FieldKind kind() const noexcept = 0;
    [[nodiscard]] virtual bool editable() const noexcept = 0;   // false -> FieldReadOnly / RegionGateClosed
    [[nodiscard]] virtual uint16_t maxCharacters() const noexcept = 0;
    [[nodiscard]] virtual uint16_t maxNativeUnits() const noexcept = 0;
    /// One character in isolation (drives key greying). Must agree with check().
    [[nodiscard]] virtual bool glyphAllowed(char32_t c) const noexcept = 0;
    /// Whole-string validation, no side effects. The keyboard calls it after every edit.
    [[nodiscard]] virtual TextVerdict check(std::string_view utf8) const noexcept = 0;
    /// All-or-nothing: on refusal `out` is untouched. On success decode(out) round-trips to
    /// `utf8` (modulo the format's documented remaps, which check() already applied).
    [[nodiscard]] virtual TextVerdict encode(std::string_view utf8, std::span<uint8_t> out) const noexcept = 0;
    [[nodiscard]] virtual std::string decode(std::span<const uint8_t> in) const = 0;
    [[nodiscard]] virtual KeyboardProfile keyboard() const noexcept = 0;
    [[nodiscard]] virtual const char* guide() const noexcept = 0;  // "1-5 kana", "1-10 Gen III characters"
    [[nodiscard]] static const char* reason(Refusal r) noexcept;   // one user-facing sentence each
};

/// The registry: the ONLY place a field's rules are chosen. `language` is the RECORD's own
/// language for Pokémon names (PKHeX rule, Gen3Text.h:15-22), the save's for trainer/box names;
/// `textLanguage` exists for Gen III Egg OTs (Pokemon3FRLG::otTextLanguage).
struct FieldContext {
    std::string_view sourceGameId;  // "firered_gba", "platinum_nds", "sv_switch", ...
    FieldKind kind;
    uint8_t language;               // Enums::LanguageID
    uint8_t textLanguage;           // usually == language
    bool japaneseLayout;            // Gen I/II/III region layout
};
[[nodiscard]] const TextFieldCapability& capabilityFor(const FieldContext&) noexcept;

}  // namespace PokeBank::Text
```

Keyboard integration (`include/Utils/Keyboard.h`): `KeyboardRequest` gains
`const PokeBank::Text::TextFieldCapability* field = nullptr;`. When set, it replaces `maxChars`,
`japaneseKana` and `koreanHangul`. Pages come from `field->keyboard()`, keys failing
`glyphAllowed()` are drawn disabled and refuse with the reason, and the counter shows
`characters/maxCharacters` from `check()`. OK is refused (field flashes, reason shown) while
`!check().ok()`. The System hand-off result goes through `check()` before `finish(true)`.
`Utils::promptName(const TextFieldCapability&, header, initial)` replaces `promptText` /
`promptJapaneseName` / `promptKoreanName` for every save-native name.

Writer integration: every existing writer (`encodeRegionName`, `encodePokemonName`,
`charToGen3` loops, `writeTextPreservingTrash`, `encodeAsciiName5`, `allowedName` ×5,
`setString`) is either reimplemented as a `TextFieldCapability` or called *by* one. Each writer
asserts `capability.check(text).ok()` before touching bytes, so the UI and the writer cannot
disagree.

Search integration: `SharedSearchInput` stops owning a key grid. It opens the same keyboard with
`FieldKind::SearchQuery` and `textChanged` (the incremental hook already exists,
`Keyboard.h:54-56`). It keeps one normaliser (`searchKey`) extended with kana/Hangul folding for
localized names, and `ItemPickerSearch::key` is deleted in favour of it.

---

## 3. Text-entry surface inventory (Gen I–IX)

Legend. **SHARED** = goes through the common keyboard wrapper for that need. **DIVERGENT** = own
call/flow/validator. **ASCII-ONLY+** = the UI cannot type characters the format stores. **TRUNC** =
truncates/replaces/corrupts. **OK** = refuses what it cannot store, with a reason. "Keyboard
pages" refers to `KeyboardDialog.cpp:planPage` (`:197-266`): *Latin* = `qwerty' - , . ?`;
*Symbols* = `0-9 ! ? & / : ; ( ) " # @ ♂ ♀ … × ~ * + = % _ $`; *Accents* = `à-ÿ À-Ÿ œ Œ ø Ø ß`;
*Kana* = Gen12Kana pages; *Hangul* = two-set jamo; *System* = swkbd hand-off (any script the
console IME offers, unvalidated by the dialog).

### 3.1 Gen I (RBY) — this branch

| # | Surface | Entry point (file:line) | Keyboard / pages | Encoder (refuse/replace) | Max source | Status |
|---|---|---|---|---|---|---|
| 1 | Nickname (Edit + Add draft) | `Gen1PokemonEditorOverlayUXCleanup3.inc:360` (live layer); same call also in `Gen1PokemonEditorOverlayUX.inc:476,583`, `…UXCleanup2.inc:521`, `…Foundation.inc:236` | `UI::ClassicJapaneseNames::prompt` (`ClassicJapaneseNames.h:38-45`) → `promptText` (Latin/Symbols/Accents/System) or `promptJapaneseName` (Kana) on a JP save | `StagedPokemonEditor::encodeRegionName` → `encodeName` (`Gen1StagedPokemonEditor.cpp:160-223`): A–Z a–z 0–9 space `- . ? !` + Gen I accents; JP: `Gen12Kana::encode`; refuses + round-trip | caller literal 10 (JP 5 via `Gen12Kana::kMaxNameChars`); writer `l.nicknameMax` | SHARED, OK. Gap: keyboard offers `' , ♂ ♀ × ( ) : ; /` which the Gen I font holds (PKHeX `StringConverter1.TableEN`) but `encodeName` refuses, so the user meets an avoidable refusal. JP kana unreachable (gate) |
| 2 | OT name (Edit + Add) | `…UXCleanup3.inc:378` (+ `UX.inc:533,636`, `UXCleanup2.inc:535`, `Foundation.inc:276`) | same | same (`trainerNameMax`) | 7 (JP 5) | SHARED, OK (same gap) |
| 3 | Trainer name | `ClassicTrainerSessionInput.h:111-115`, fields at `Gen1PokemonEditorOverlayFoundation.inc:778-781` | `promptText` / `promptJapaneseName` | `Gen1StagedTrainerEditor.h:141-144` → `encodeRegionName` | `editor->nameMax()` | SHARED, OK |
| 4 | Rival name | — | — | — | PKHeX `SAV1.Rival` | **Missing** (real field, no surface) |
| — | Dead copies | `Gen1PokemonEditorOverlay.cpp:337,408,444,469` (direct `promptText`) | — | — | — | DIVERGENT **dead**: compiled (Makefile `SOURCES` wildcard), but `UI::Gen1PokemonEditor::handleInput/drawOverlay` declared in `Gen1PokemonEditorOverlay.h:20-23` have no caller |
| — | Box names | RBY boxes are unnamed | — | — | — | N/A |

### 3.2 Gen II (GSC) — this branch

| # | Surface | Entry point | Keyboard / pages | Encoder | Max | Status |
|---|---|---|---|---|---|---|
| 1 | Nickname (Edit) | `Gen2UnifiedPokemonWorkspace.inc:481`, `Gen2PokemonEditorFoundation.inc:520` | `ClassicJapaneseNames::prompt` | `Gen2StagedEditor::encodePokemonName` (`Gen2StagedEditor.cpp:701-707`) → `encodeInternationalASCII` (`:322-410`: A–Z a–z 0–9 space `( ) : ; - ? ! & ' . / ,` ♂ ♀ ’ + all-release accents; refuses release-dependent accents and kana) or `encodeJapaneseKana` (`:415-437`) | 10 / JP 5 | SHARED, OK |
| 1b | Nickname (legacy overlay) | `TrainerViewScreenGSCOverlay.inc:211` | direct `promptText` (no JP routing) | same encoder | 10 | DIVERGENT (duplicate path) |
| 2 | OT name | `Gen2UnifiedPokemonWorkspace.inc:519`, `Gen2PokemonEditorFoundation.inc:536`; legacy `TrainerViewScreenGSCOverlay.inc:285` | wrapper / direct | `encodeOTName` (`:690-698`) | `kInternationalTrainerNameMax` (7) / JP 5 | SHARED, OK + DIVERGENT duplicate |
| 3 | Create nickname | `TrainerViewScreenGSCOverlay.inc:335` (chained raw-ID number prompts) | direct `promptText` | `encodePokemonName` at `:1459-1460` | 10 | DIVERGENT |
| 4 | Trainer name | `ClassicTrainerSessionInput.h:111-115` via `Gen2TrainerSessionOverlay.inc:24-27`; legacy `TrainerViewScreenGSCOverlay.inc:651` | wrapper / direct | `encodeTrainerName` (`:681-687`) | 7 / JP 5 | SHARED, OK + DIVERGENT duplicate |
| 5 | Rival name | — | — | — | PKHeX `SAV2.Rival` | **Missing** |
| 6 | Box names (14) | `GSCReadOnlyTrainer` inherits `getMaxBoxNameLength()==0` (`Trainer.h:443`) | — | — | PKHeX `SAV2` box names | **Missing** |

### 3.3 Gen III (RSE / FRLG GBA) — this branch

| # | Surface | Entry point | Keyboard / pages | Encoder | Max | Status |
|---|---|---|---|---|---|---|
| 1 | Nickname (Edit + Create) | `Gen3SharedPokemonSurface.inc:867` | `promptText` (Latin/Symbols/Accents/System) for every record language | `Gen3StagedPokemonEditor.cpp:510-521`: `canStoreNickname` (record language) + fold-width read-back; Create `:885-895` | caller 10 for all languages; writer `maxNicknameLength` = JP 5 / else 10 (`:120`, check at `:589`) | DIVERGENT surface; **ASCII-ONLY+** for Japanese records (GEN3_JP is a full kana table, `Gen3Text.h:66-83`; no kana page); OK (refuses) |
| 2 | OT name (Edit + Create) | `Gen3SharedPokemonSurface.inc:887` | `promptText` | Edit `:571-578`: `canStoreNickname` (wrong table for Eggs) then `setOTName` → `g3EncodeName` (breaks); no read-back. Create `:870-880` has read-back | 7 (JP 5) | **TRUNC — P0-1** (§1) |
| 3 | Trainer name | `ClassicTrainerSessionInput.h` via `Gen3SharedPokemonSurface.inc:2695-2697` | `promptText` | `Gen3StagedTrainerEditor.h:135-165` (international table only, read-back) | 7 | SHARED, OK. JP saves read-only (`:42-43,139`) |
| 4 | Box names | FRLG/RSE GBA trainer exposes none | — | (`Trainer3FRLG::canStoreBoxName` exists only for the legacy Switch path) | 8 | **Missing** |

### 3.4 Gen IV (DPPt / HGSS) — this branch

| # | Surface | Entry point | Keyboard / pages | Encoder | Max | Status |
|---|---|---|---|---|---|---|
| 1 | Nickname | `Gen4SharedPokemonSurface.inc:823-831` | Korean record → `promptKoreanName` (Hangul/ABC/?123/System); else `promptText` | `Pokemon4Mutable::setNickname` → `writeTextPreservingTrash` (`Pokemon4Mutable.cpp:389-415`): Gen 4 table incl. Hangul; round-trip refuses unsupported; **substr to 10 before the check** | 10 for every language (PKHeX caps JP/KO lower; verify at pin) | SHARED; **ASCII-ONLY+** for Japanese records (GEN4_INT holds hiragana/katakana/full-width, `Gen4Text.h:30-74`); OK (latent cut) |
| 2 | OT name | `Gen4SharedPokemonSurface.inc:847-854` | same | `setOriginalTrainerName` (`:510-521`, round-trip + reparse) | 7 | same |
| 3 | Trainer name | `ClassicTrainerSessionInput.h` via `Gen4SharedPokemonSurface.inc:2333-2335` | `promptText` (no Hangul for a Korean save, no kana for Japanese) | `Gen4StagedPokemonEditor::stageTrainerName` (`Gen4StagedPokemonEditor.cpp:1303-1320`, `gen4CanEncode` + reparse) | 7 | SHARED, OK; JP/KO saves read-only (`:1291-1301`) → ASCII-ONLY+ for those languages |
| 4 | Box names | `Gen4ReadOnlyTrainer.h:38-39` says supported (8) but `updateBoxNameBlock` is a no-op (`:27`); base `renameBox` refused by `requireMutableWorkspace` because classic sessions are `ExternalLegacy` (`UI.cpp:678-679`, `SourceMutationPolicy.h:29-32`) | — | none | 8 | **Missing** + latent trap (P0-L6) |

### 3.5 Gen V (BW / B2W2) — this branch

| # | Surface | Entry point | Keyboard / pages | Encoder | Max | Status |
|---|---|---|---|---|---|---|
| 1 | Nickname | `Gen5SharedReadOnlySurface.inc:438-451` (stored non-ASCII names seed an empty field) | `promptText` | `encodeAsciiName5` (`Gen5NativeText.h:83-103`): 0x20–0x7E only | 10 | **ASCII-ONLY+** (PK5 is UTF-16; blocked on glyph tables, `Gen5NativeText.h:20-27`); OK |
| 2 | OT name | same | `promptText` | same | 7 | ASCII-ONLY+, OK |
| 3 | Trainer name | `Gen5SharedReadOnlySurface.inc:1391-1394` → `Gen5TrainerScreen.h:204-206` | `promptText` | `Gen5StagedTrainer::setName` (`Gen5Trainer.h:201-214`, `trainerNameAccepted5`) | 7 | DIVERGENT (own trainer screen, not `ClassicTrainerSession`); ASCII-ONLY+ |
| 4 | Box names | `Gen5ReadOnlyTrainer.h:37` `supportsBoxNames()==false` | — | — | 8 | **Missing** |

### 3.6 Gen VI–IX — `origin/development/gen6-plus` (read-only)

| # | Surface | Entry point | Keyboard | Validator | Max | Status |
|---|---|---|---|---|---|---|
| 1 | Nickname | `src/UI/ModernSharedSurface.inc:416-422` ("1-12 ASCII characters") | `promptText` (older fork, no Hangul) | per family: `Gen6StagedSession.cpp:64-72`, `Gen7StagedSession.cpp:64`, `Gen7bStagedSession.cpp:31-39`, `Gen8RecordEdits.h:61-69` (Gen8 / BDSP / SV / Z-A via `Gen8Edits::applyText`), `Gen8aSession.cpp:44-52`: 0x20–0x7E only | 12 for every language | **ASCII-ONLY+** (PK6–PK9/PA8/PA9 are UTF-16); DIVERGENT (5 copies of one validator) |
| 2 | OT name | same | same | same | 12 | ASCII-ONLY+ |
| 3 | Trainer name | `Modern::NativeSession` exposes `trainerName()` only (`NativeSession.h:201`) | — | — | — | **Missing** |
| 4 | Box names | `NativeSession::boxName()` read-only (`:148`) | — | — | — | **Missing** |
| 5 | Save file path | `SaveSelectScreen.cpp:3187-3191` | `promptText` 240 | path inspection | 240 | OK |
| — | Gen I–V copies on gen6-plus | e.g. `Gen4SharedPokemonSurface.inc:822,845` plain `promptText` (no Hangul), `KeyboardDialog.cpp:750` | older keyboard | — | — | **Fork divergence**: shared component differs per branch |

### 3.7 App-level surfaces (every generation)

| # | Surface | Entry point | Implementation | Status |
|---|---|---|---|---|
| A1 | Species search | `SharedSpeciesPicker.h` + `SharedSearchInput.h` key grid; used by `Gen1PokemonEditorOverlayUXCleanup3.inc`, `Gen2PokemonEditorFoundation.inc`, `Gen2PokemonPickerOverlay.inc:30,186`, `Gen3SharedPokemonSurface.inc:123`, `SharedPokemonEditorSurface.inc:90,343-458` (Gen IV/V/VI+) | **second keyboard** (A–Z 0–9, `kMaxQuery` 12), prefix match on `searchKey` | DIVERGENT (own keyboard); OK for today's English names, cannot search Japanese/Korean names |
| A2 | Held-item search | `SharedHeldItemSearch.h` + `ItemPickerSearch.h`; `SharedPokemonEditorSurface.inc:289-305,423`, `Gen3SharedPokemonSurface.inc:1150-1162,1252`, `Gen2PokemonEditorFoundation.inc` | same grid; **two normalisers** (`ItemPickerSearch::key` contains-match keeping UTF-8 vs `SharedSearchInput::searchKey` prefix with accent folding) | DIVERGENT |
| A3 | Move / met-location / ability / bag-add search | none (`SharedBagAddPicker.h`, `MovePickerPresentation.h` have no query) | — | **Missing** (owner-listed) |
| A4 | Vault / bank search | none | — | **Missing** (no Vault, §5) |
| A5 | Bank box rename | `TrainerViewScreenBase.inc:1579-1599` | `promptText`, 24 code points; stored UTF-8 (`Bank.cpp:232-241`, parse cap 96 bytes `:406`) | ASCII-ONLY+ (any Unicode stores; only System types it); OK |
| A6 | Backup name | `TrainerViewScreenBase.inc:3290-3297` → `createNamedBackupDir` `:1767-1777` | `promptText` 40; **silently drops** every char outside `[A-Za-z0-9 _-]` | **REPLACES** (non-save P1): `Ash's PC` becomes `Ashs PC` without telling the user |
| A7 | Gen IV manual save path | `SaveSelectScreen.cpp:2386-2389` | `promptText` 240 | OK |
| A8 | Gen V manual save path | `SaveSelectScreen.cpp:2717-2720` | `promptText` 240 | OK |
| A9 | Export file name | none (auto-named) | — | N/A |

### 3.8 Legacy PKSE Switch-title surfaces still on this branch

`Save::TrainerVariant` (`GetSaveFileContents.h:26`: LGPE, SwSh, LZA, SV, LA, BDSP, Switch FRLG)
opens as `SourceKind::BackupOrStaged` (`UI.cpp:609-612`), which passes `requireMutableWorkspace`.
So these inherited surfaces are live for backups:

| # | Surface | Entry point | Validation | Writer | Status |
|---|---|---|---|---|---|
| L1 | Nickname (details) | `TrainerViewScreenBase.inc:3810-3832` | `Pokemon::canStoreNickname` (default `true`, `Pokemon.h:436`; FRLG overrides) | per-format setters | DIVERGENT; ASCII-ONLY+ |
| L2 | Trainer name | `TrainerViewScreenBase.inc:1659-1686` | `canStoreBoxName` (default `true`, `Trainer.h:471`) + digit cap | e.g. `Trainer8SWSH.cpp:808` `setString(...,12)` | DIVERGENT; latent TRUNC (P0-L4) |
| L3 | Save box rename | `TrainerViewScreenBase.inc:1539-1572` | `canStoreBoxName` | e.g. `Trainer8SWSH.cpp:502-503` `setString` | DIVERGENT; latent TRUNC |

---

## 4. Native character repertoire per format vs. what the UI lets you type

| Format | Native repertoire (codec in repo) | Max chars (game) | UI can type today | Gap |
|---|---|---|---|---|
| Gen I international | 1-byte charmap, `0x50` term; PKHeX `StringConverter1.TableEN` (Latin, digits, `' - . ? ! ( ) : ; [ ] / , ♂ ♀ ×`, ligatures, accents in `Gen12AccentText.h:25+`). Repo encoder subset: `Gen1StagedPokemonEditor.cpp:160-186` | 10 / 7 | Latin + Accents + Symbols (all pages) | Encoder narrower than font (`' , ♂ ♀ × ( ) : ; /` refused). Keyboard offers glyphs that will be refused |
| Gen I Japanese | `Gen12KanaText.h` (PKHeX TableJP): hiragana, katakana, `ー ・ … 「」『』`, full-width space | 5 | Kana pages exist (`KeyboardDialog.cpp:162-187`); **unreachable**, gate closed | By design until gate opens (§7) |
| Gen II international | `Gen2Text.h` (PKHeX StringConverter2), EN/FRE/ITA tables differ at 0xBE–0xBF, 0xC6–0xCC, 0xD0–0xD6 (`Gen12AccentText.h:5-14`) | 10 / 7 | all Latin pages | Release-dependent accents refused (correct). Encoder lacks `× [ ]` etc. |
| Gen II Japanese | `Gen12KanaText.h` (same table) | 5 | gated | §7 |
| Gen II Korean (KOR GS) | **not in repo** (no KOR table, no layout detection) | — | — | §7 |
| Gen III international | `GEN3_EN` (`Gen3Text.h:47-64`): Latin, accents, `¿ ¡ º ª & + = ; % ( ) “ ” ‘ ' ♂ ♀ $ , / : ← ↑ ↓ ►` and some kana glyphs | 10 / 7 | all Latin pages + System | Keyboard offers `… × ~ * _ # @ " !` subsets the table lacks (refused, except P0-1) |
| Gen III Japanese | `GEN3_JP` (`Gen3Text.h:66-83`): full kana, full-width Latin, `… 『』「」 円 × ／` | 5 | Latin pages only (half-width Latin is retried full-width, `:197-200`) | **No kana page** for JP records |
| Gen IV (all languages) | `Gen4Text.h` 16-bit code page: `GEN4_INT` (hiragana, katakana, full-width + half-width Latin, symbols) + `GEN4_KOR` 2,406 Hangul (`:75-76`); `Gen4TextCodec.h` gender remaps | 10 / 7 (PKHeX lower for JP/KO — verify) | KO Pokémon: Hangul page; all others Latin | No kana for JP records; no Hangul for KO trainer name; length not language-aware |
| Gen V | UTF-16 per character, `0xFFFF` term, ♂/♀ half-width remap (`Gen5NativeText.h:105-121`); glyph legality per language font **not in repo** | 10 / 7 | ASCII 0x20–0x7E only | Format stores more; blocked on glyph tables (§7) |
| Gen VI / VII | UTF-16 (PKHeX StringConverter6/7: gender-symbol remap; Gen VII CHS/CHT private-use remap) | 12 (PKHeX lower for JP/KO/ZH — verify) | ASCII only | §7 |
| Gen VII LGPE, VIII, IX | UTF-16 | 12 (same caveat) | ASCII only | §7 |
| Bank box name | UTF-8 (app-owned) | 24 code points | Latin pages + System | Kana/Hangul pages not offered |
| Backup name | FAT32-safe `[A-Za-z0-9 _-]` | 40 | everything, silently stripped | Must refuse, not strip |

---

## 5. Shared-UX architecture map (all owner-listed areas)

Generations: I, II, III, IV, V on this branch; VI+ = gen6-plus `ModernSharedSurface.inc`.

| Area | Shared component(s) that exist | Gens using it | Per-generation duplicates / divergence (file:function — what diverges) |
|---|---|---|---|
| Keyboard / text | `Utils/Keyboard.h`, `UI/Dialogs/KeyboardDialog.cpp`, `UI/ClassicJapaneseNames.h`, `Utils/HangulComposer.h`, `Utils/Gen12KanaText.h` | all | `SharedSearchInput.h` second key grid; script choice by booleans; 11 writer-side validators (§4); fork on gen6-plus (older keyboard); dead `Gen1PokemonEditorOverlay.cpp` prompts |
| Pokémon editor workspace / fields | `SharedPokemonEditorSurface.inc` (+ `SharedPokemonEditorContract.h`, `SharedPokemonEditorWorkspace.h`, `SharedEditorPresentation.h`, `SharedFieldState.h`, `SharedEditorChrome.h`, `SharedPokemonShell.h`) | IV, V, VI+ (`Gen4SharedPokemonSurface.inc:53`, `Gen5SharedReadOnlySurface.inc:45`, gen6-plus `ModernSharedSurface.inc:52`) | **Gen III** `Gen3SharedPokemonSurface.inc` (2,822 lines): own `editDetail` (`:851+`), picker state, move editor (`:1097-1405`), release (`:1926`), export (`:1817`). **Gen II** six layered files: `Gen2PokemonEditorFoundation.inc`, `Gen2PokemonPickerOverlay.inc`, `Gen2HardwarePickerFix.inc`, `Gen2SharedPokemonSurface.inc`, `Gen2UnifiedPokemonWorkspace.inc` (+`Gen2HardwareWorkspaceFix.inc`), `Gen2SharedSurfaceParity.inc` (+`Gen2HardwareFinalFix.inc`), plus legacy `TrainerViewScreenGSCOverlay.inc:editField` (numeric-ID prompts). **Gen I** `Gen1PokemonEditorOverlayUX.inc`, `…UXCleanup2.inc`, `…UXCleanup3.inc`, `…Foundation.inc`, `…FoundationHardwareFix.inc`, `…PassiveView.inc`, stacked by rename macros (`TrainerViewScreenCompositeOverlay.cpp:46-137`), plus dead `Gen1PokemonEditorOverlay.cpp` |
| Species chooser | `SharedSpeciesPicker.h` | I, II, III, IV, V, VI+ | Each surface re-wires state + input: `Gen2PokemonPickerOverlay.inc:30-186`, `Gen3SharedPokemonSurface.inc:123,859-864`, `SharedPokemonEditorSurface.inc:90,343-458`; gen6-plus Modern opens a plain value list (`ModernSharedSurface.inc:404-411`) |
| Move chooser | `MovePickerPresentation.h`, `MovePickerRowUI.h`, `SharedMovesPanel.h` | I (UXCleanup3), II (PickerOverlay), III, IV, V | `ClassicMovesPanel.h` (Gen I), `Gen2MovesPanel.h` (Gen II Unified), Gen III own move editor; no text search anywhere |
| Item chooser (held) | `SharedHeldItemPicker.h`, `SharedItemGridPicker.h`, `SharedHeldItemSearch.h`, `ItemPickerSearch.h`, `ItemPickerSort.h` | II, III, IV, V | `Gen2HeldItemPicker.h` (Gen II only); two search normalisers |
| Ability chooser | `SharedAbilityPicker.h` | III, IV, V | Gen III wires it separately from `SharedPokemonEditorSurface.inc` |
| Nature chooser | `NaturePickerLayout.h` | III, IV, V | Gen III separate wiring |
| Met location / encounter | `Integration/Encounter/MetLocationPolicy.h`; `SharedPokemonEditorSurface.inc:207 openContextualMetLocationPicker` + `EncounterContext45.h` | IV, V (contextual) | **Gen III** `Gen3EncounterPickerModel.h` + `kKeepStoredRow` (`Gen3SharedPokemonSurface.inc:826-832`); **Gen II** Crystal via `ExactGameMetLocations.h` in `Gen2PokemonPickerModel.h`; Gen I no field; Gen V also `Gen5EncounterLocations.h` |
| Party controls | `ClassicPartyActions.inc` (one Party sheet) | I–IV | Internally a `switch(generation)` over four per-gen states (`ClassicPartyActions.inc:13-20`), not one abstraction; Gen V in `Gen5SharedReadOnlySurface.inc`; VI+ in Modern |
| Box controls / carry / Move | `ClassicPackedMoveOverlay.inc` + `ClassicCarryGrid.h`, `ClassicCarryRules.h`, `ClassicPackedMultiSelect.h` | I–IV (`classicPackedMoveLayerAvailable`, `TrainerViewScreenCompositeOverlay.cpp:248-268`) | Gen V `Gen5BoxTouchNavigation.h` + own Move; VI+ Move via Clone destination keypads (`ModernSharedSurface.inc` header comment, lines 16-18) |
| Create | `CreateRequiredFields.h`, per-gen sessions | I–V | Gen I `ux3StageAdd` (`…UXCleanup3.inc:152`); Gen II `stageAddPokemon` (`TrainerViewScreenGSCOverlay.inc:328`, chained numeric prompts) + Unified path; Gen III `beginCreate` (`:336`); Gen IV `beginCreate` (`:586`); Gen V `beginSharedCreate` (`:136`); VI+ Create blocked by registry |
| Clone | per-gen staged editors | I–V, VI+ | Gen I `ux3DrawClone` (`…UXCleanup3.inc:637`); Gen II native byte clone (`Gen2StagedEditor.cpp:1326`); Gen III/IV in their surfaces; Gen V `actionUi.confirmClone`; VI+ `NativeSession::clone` |
| Release | — | — | Gen I and Gen II **two implementations in one file** (`ClassicReleaseActionFix.inc:12,98,173` and `:298,447`); Gen III `handleReleaseConfirm` (`:1926`); Gen IV `handleReleaseConfirm` (`:1118`); Gen V `actionUi.confirmRemoveBox`; VI+ `NativeSession::remove` |
| Move to Vault | **does not exist** | none | v1.0 blocker: `PBX-ACT-002`, `R12-011` (`docs/qa/MASTER_HARDWARE_ACCEPTANCE_LEDGER.md:50,483`); enum rows only in `ActionSheetModel.h` (NotYetSupported) |
| Trainer editing | `ClassicTrainerSessionInput.h` (`UI::ClassicTrainerSession::handle`), `ClassicTrainerPage.h`, `SharedTrainerPanel.h` | I, II, III, IV | **Gen V** `Gen5TrainerScreen.h` + `Gen5SharedReadOnlySurface.inc:1380-1405`; legacy `TrainerViewScreenBase.inc:editTrainerName`; legacy `TrainerViewScreenGSCOverlay.inc:640-675`; VI+ none |
| Inventory | `InventoryUIContract.h`, `SharedBagAddFlow.h`, `SharedBagAddPicker.h`, `SharedBagTopUp.h` | I–V | Gen I–III `ClassicInventoryOverlay.cpp` (own `SourceKind{Gen1,Gen2,Gen3}`, `:64,105`); Gen IV inside `Gen4SharedPokemonSurface.inc` (`stageGen4BagAdd :1745`, `drawItemAddPicker :2257`); Gen V `Gen5BagScreen.h`; VI+ Modern bag via `NativeSession::bag*` |
| Search | `SharedSearchInput.h` | species + held item, I–V | §3.7; no move/location/ability/bag/Vault search |
| Review | `ReviewEntry.h` (keys), `SharedReviewPresentation.h` (chrome), `ClassicBrowseReview.h` | I–V | Row builders differ: `Gen1ReviewRows.h`; Gen II review in `TrainerViewScreenGSCOverlay.inc` state; Gen III `reviewScroll` in its surface; Gen IV in its surface; `Gen5SharedReview.h`; `openStagedReview` has a 4-way `if` chain (`TrainerViewScreenCompositeOverlay.cpp:185-227`) |
| Backups | `BackupSelectionScreen.h`, `RecoveryManagementModel.h`, `SourceBackupClassification.h` | all | Backup naming only on legacy path (A6) |
| Safe export | `Integration/Common/VerifiedStagedExport.h` | I, III, IV (`Gen1/3/4ExportTransaction.cpp`) + Gen I–III bag (`ClassicInventoryOverlay.cpp`) | Gen II `Gen2ExportTransaction.cpp` (does not include it); Gen V `Gen5WorkingCopyExport.h`; UI exporters per gen: `Gen1PokemonEditorOverlayUX.inc:706`, `TrainerViewScreenGSCOverlay.inc:112`, `ClassicInventoryOverlay.cpp:393`, `Gen3…:1817`, `Gen4…:1145` |
| Import/Export to file | `PokemonFileDialog`, `PokemonFileMenus.inc` | I–V | — (already shared) |

---

## 6. Prioritized work list

Every item lists the files to change, the regression tests to add (all applicable formats, never
weakened), and acceptance measured against PKHeX/PKSE behaviour **and** a hardware check. "PKHeX"
means the pinned commit already cited in the repo (`8e3eb3d4`).

### P0: data safety (truncation / replacement / corruption)

| ID | Work | Files | Regression tests | Acceptance |
|---|---|---|---|---|
| **P0-1** | Gen III OT edit: validate with `otTextLanguage()` (add `Pokemon3FRLG::canStoreOTName`), add the fold-width read-back already used by Create (`:879`), apply the length rule of the text language | `src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:571-578,593`; `include/Pokemon/Pokemon3FRLG.h`; `src/Pokemon/Pokemon3FRLG.cpp:89-112` | `tests/test_gen3_staged_pokemon_editor.cpp`: Egg + non-Egg × {FR, LG, R, S, E} × save language {EN, FR, DE, IT, ES, JP} × corpus {`A×B`, `A…B`, `AいB`, `Aア`, `Aà`, `BOB`} → accepted iff every char is in the text-language table; refused edits leave `stagedBytes()` unchanged; accepted edits survive export → reparse byte-identical | Same verdicts as PKHeX `PK3.OriginalTrainerName` setter + `StringConverter3` for the same inputs; device: Egg OT `A×B` is refused with a reason, Review/Export unchanged |
| **P0-2** | Make every name writer all-or-nothing (return `bool`/`Refusal`, never `break`/`substr`/`?`/blank) | `Pokemon3FRLG.cpp:89-100`; `Trainer3FRLG.cpp:41-51`; `Pokemon4Mutable.cpp:389-415`; `include/Utils/Gen4TextCodec.h:52-75`; `src/Conversion/Convert.cpp:277-330,868-869` (OT blank → refuse or record a `Report` adaptation the UI must show) | New `tests/test_name_writers_all_or_nothing.cpp`: for each writer, over-length and unencodable inputs leave bytes untouched and return refusal; plus a contract test failing on new `break`-on-terminator loops in writers | No writer in the tree can store a prefix of the requested text; converter never blanks an OT silently |
| **P0-3** | Retire unchecked `Utils::setString` for user text; use `setStringChecked` (exists, 0 callers) with the cap in UTF-16 units from the capability | `src/Utils/StringHelpers.cpp:143-164`; 23 call sites incl. `Trainer8SWSH.cpp:502,808` and the other legacy `Trainer*`/`Pokemon*` | `tests/test_modern_text_writer.cpp`: 12 code points incl. one non-BMP char into a 12-unit field → refused, no lone surrogate | Matches PKHeX `StringConverter8.SetString` length behaviour; device: legacy SwSh backup rename with an over-long name is refused |
| **P0-4** | One length unit: keyboard counter/cap = capability `maxCharacters`, writer cap = same, and native units checked separately | `include/Utils/Keyboard.h`, `src/UI/Dialogs/KeyboardDialog.cpp:68-77,712-733`, every caller in §3 | `tests/test_ui_keyboard_capability_host.cpp` (extend existing host keyboard tests) | Keyboard never accepts text the writer refuses for length |
| **P0-5** | System keyboard result must pass `check()` before acceptance | `KeyboardDialog.cpp:800-815` | host test with a stubbed `promptSystemText` returning unsupported glyphs | Refused with reason; field keeps previous text |
| **P0-6** | `Gen4ReadOnlyTrainer::supportsBoxNames()` → `false` until a Gen IV box-name writer exists (no silent lost rename if the source policy ever widens) | `include/Legacy/Gen4ReadOnlyTrainer.h:38` | `tests/test_gen4_readonly_trainer.cpp` (or existing Gen IV UI contract): header not renamable | No UI path offers a rename that cannot be written |
| **P0-7** | Backup name: refuse (with the rule) instead of stripping; or show the sanitised name for confirmation | `TrainerViewScreenBase.inc:1767-1777,3290-3297` | `tests/test_backup_namespace_contract.cpp` extension | No silent character removal |

### P1: one shared keyboard with Japanese / Korean / Latin and native validation in every format

| ID | Work | Files | Regression tests | Acceptance |
|---|---|---|---|---|
| **P1-1** | Add `include/Text/TextFieldCapability.h` + `src/Text/*Capability.cpp` per format (Gen1 Intl/JP, Gen2 Intl/JP, Gen3 EN/JP, Gen4, Gen5, Gen6/7, Gen8/9, Bank, Backup, Path, Search) wrapping the existing codecs (no table copied) | new `include/Text/`, `src/Text/`; Makefile `SOURCES`; `Makefile.host.multilingual` | `tests/test_text_field_capability_matrix.cpp`: every descriptor × shared corpus (ASCII, accents, `♂♀…×`, hiragana, katakana, full-width, Hangul syllables + jamo, non-BMP, over-length, empty, digits-cap) asserting `glyphAllowed` ⇔ single-char `check`, `check` ⇔ `encode` verdict, round trip on accept, untouched buffer on refuse | Verdicts equal PKHeX `StringConverter{1,2,3,4,5,6,7,8}` + `Legal.GetMaxLengthNickname/OT` for the corpus (golden table generated from PKHeX at the pin, checked in) |
| **P1-2** | Keyboard consumes the capability: pages from `KeyboardProfile`, disabled keys, live counter, OK gating; delete `japaneseKana`/`koreanHangul` flags and `promptJapaneseName`/`promptKoreanName` | `Keyboard.h/.cpp`, `KeyboardDialog.h/.cpp`, `ClassicJapaneseNames.h` | extend `test_ui_kana_keyboard_host.cpp`, `test_ui_hangul_keyboard_host.cpp`, `test_ui_japanese_name_prompts_host.cpp` to every format row | Every key offered is storable in the target field |
| **P1-3** | Kana pages for Gen III JP records and Gen IV JP records (and Gen IV/V/VI+ JP when their gates open); full-width Latin where the table is full-width | `Gen12KanaText.h` page model generalised to a format-supplied glyph set | per-format page ⊆ table test | JP record nickname typed entirely in-app, stored, exported, reparsed |
| **P1-4** | Hangul for Gen IV Korean trainer name (after P1-1 decides the trainer field is editable) | `Gen4StagedPokemonEditor.cpp:1291-1320`, `ClassicTrainerSessionInput.h` | Gen IV KO trainer corpus | Matches PKHeX SAV4 KO trainer name |
| **P1-5** | Route every §3 surface through `Utils::promptName(capability, …)`; delete direct `promptText` name calls | §3 entry points (Gen1 4 layers, Gen2 wrapper + GSC overlay, Gen3 `:867,887`, Gen4 `:823-854`, Gen5 `:446,1391`, Modern `:419`, legacy base `:1548,1664,3813`) | python contract test: no `promptText(` with a name header outside `Keyboard.cpp` | One call path per field kind |
| **P1-6** | Encoder parity: widen Gen I/II international encoders to the full PKHeX tables (`' , ♂ ♀ × ( ) : ; [ ] /`), keeping release-dependent Gen II accents refused | `Gen1StagedPokemonEditor.cpp:160-186`, `Gen2StagedEditor.cpp:322-410` (generate from `Gen2Text.h` tables, not hand lists) | `test_rby_pokemon_editor`, `test_gsc_pokemon_editor` corpus vs PKHeX golden | No glyph PKHeX writes for the field is refused |
| **P1-7** | Language-aware lengths for Gen IV/V/VI+ (record language), replacing the fixed 10/7/12 | `Pokemon4Mutable.cpp`, `Gen5NativeText.h`, modern validators | golden lengths per language | Equal to PKHeX limits at the pin |
| **P1-8** | One keyboard on both branches: port the Gen I–V keyboard to gen6-plus when it resumes, then keep it in one place (shared header + host tests run on both) | gen6-plus `KeyboardDialog.cpp`, `Keyboard.h/.cpp` | same host tests on both branches | Zero diff in shared UI files between branches except intentional |

### P2: consolidate divergent per-generation UI into shared components

| ID | Work | Files | Tests | Acceptance |
|---|---|---|---|---|
| **P2-1** | Search uses the shared keyboard in search mode; one normaliser (`searchKey` + kana/Hangul folding); delete `ItemPickerSearch::key`; add move / met-location / ability / bag-add search | `SharedSearchInput.h`, `ItemPickerSearch.h`, `SharedSpeciesPicker.h`, `SharedHeldItemSearch.h`, move/location/bag pickers | `test_species_search_gen15.cpp`, `test_item_picker_search.cpp` + new move/location search tests, all gens | Same query finds the same thing in every chooser and every generation |
| **P2-2** | Gen III onto `SharedPokemonEditorSurface.inc` (adapter like Gen4/5), deleting duplicated pickers, move editor, release, review | `Gen3SharedPokemonSurface.inc` → thin adapter | existing Gen III host tests unchanged; shared-surface contract tests gain Gen III | Gen III editor behaviour identical to Gen IV/V for shared rows |
| **P2-3** | Gen I / Gen II: collapse the layered `.inc` stacks into one adapter each over the shared surface; delete `Gen1PokemonEditorOverlay.cpp` (dead) and `TrainerViewScreenGSCOverlay.inc` editor paths (`editField`, `stageAddPokemon`, trainer case 0) | `TrainerViewScreenCompositeOverlay.cpp:46-181` and the files it includes | Gen I/II hardware-acceptance guards (Round 11/12) stay green; new "no duplicate prompt" contract | One Gen I and one Gen II surface; no rename macros |
| **P2-4** | Gen V trainer page onto `ClassicTrainerSession` | `Gen5SharedReadOnlySurface.inc:1380-1405`, `Gen5TrainerScreen.h` | Gen V trainer tests | Same Trainer page UX as Gen I–IV |
| **P2-5** | One Release flow (shared confirm + per-format `removeSupport/remove`), one Review row builder interface, one export entry (`VerifiedStagedExport` for Gen II and V too) | `ClassicReleaseActionFix.inc`, Gen3/4/5 release, `openStagedReview`, `Gen2ExportTransaction.cpp`, `Gen5WorkingCopyExport.h` | per-gen release/review/export tests unchanged + shared contract | Release/Review/Export identical across gens |
| **P2-6** | One inventory surface over a per-format bag backend (`ClassicInventoryOverlay`, Gen IV bag in its surface, `Gen5BagScreen`, Modern bag) | listed files | existing bag tests per gen | Same Bag UX everywhere |
| **P2-7** | Missing real fields, each only with a byte-exact writer + reparse + PKHeX parity: Gen I/II rival name; Gen II/III/IV/V box names; Gen VI–IX trainer and box names | per-format staged editors + capability rows | writer round-trip + export reparse per game | Matches PKHeX field offsets and limits; device-verified in game |
| **P2-8** | Move to Vault (shared action row from `NarrowPokemonActionMenu.h` / `SharedPokemonEditorContract.h`, per-format remove + Vault deposit transaction) | Vault backend (PBX-VLT-001), action menus | transactional move tests per gen | Present in every Actions sheet; never loses a Pokémon on interruption (hardware power-cut test) |
| **P2-9** | Retire legacy PKSE Switch-title editing surfaces (§3.8) into the NativeSession / shared surface | `TrainerViewScreenBase.inc:1539-1686,3800-3835` | — | No second editor for any format |

**Top 10, in order:** P0-1, P0-2, P0-3, P0-5, P0-4, P1-1, P1-2, P1-5, P1-3, P2-1.

---

## 7. What a format cannot safely support yet

Each entry names the exact missing implementation. Until it exists the field stays refused or
read-only. Nothing here may be loosened to make a menu look complete.

1. **Japanese Gen I / II editing.** The gate stays closed (`JapaneseEditingGate.h:13`). It opens
   only after **(a)** strict parser + checksum + byte-preservation + export + reparse tests on a
   corpus of real Japanese R/G/B/Y and G/S/C saves run in default host CI (today opt-in,
   `Makefile.host.jp-gen12`); **(b)** a hardware session records kana entry → export → load in
   the original game/emulator for each title; **(c)** the owner flips the constant. The kana
   keyboard and encoders exist and are not the blocker.
2. **Korean Gold/Silver (Gen II KOR).** Missing: region/layout detection, a KOR charmap (PKHeX
   `StringConverter2KOR`), save offsets. Until then a Korean GS save must be refused at open, never
   decoded through the international table.
3. **Gen II European accents that differ by release** (`Gen12AccentText.h`). Missing: a per-save
   European release choice (or detection) feeding the table. Refused until then.
4. **Gen III Japanese trainer name** (`Gen3StagedTrainerEditor.h:42-43`). Missing: unambiguous
   5-character Japanese layout detection and a JP-table writer with read-back.
5. **Gen III Korean.** No such game. Never offer Hangul for Gen III.
6. **Gen IV Japanese / Korean trainer name** (`Gen4StagedPokemonEditor.cpp:1291-1301`). Missing:
   language-specific length + trash-byte rule verified against PKHeX SAV4 and real saves.
7. **Gen IV name lengths per language.** Writer allows 10/7 for every language. Missing: the
   PKHeX per-language limits in the capability (verify the pin) and tests.
8. **Gen V non-ASCII entry** (`Gen5NativeText.h:20-27`). Missing: verified per-language Gen V font
   glyph tables (which UTF-16 code units each language's font draws). Without them a writer cannot
   refuse a glyph the game would draw as garbage.
9. **Gen VI–IX non-ASCII entry** (5 × `allowedName`). Missing: PKHeX `StringConverter6/7`
   remaps (gender symbols; Gen VII CHS/CHT private-use area), per-language font repertoire and
   limits (6 for JP/KO/ZH — verify), and tests per family.
10. **Gen VI–IX trainer name / box names.** Missing: `NativeSession` stagers with byte-exact
    writers + export reparse per family.
11. **Box names Gen II / III (GBA) / IV / V; rival names Gen I / II.** Missing: writers. Do not add
    UI rows until the writer exists (owner: no invented fields).
12. **Search over Japanese/Korean names.** Missing: localized name tables in the UI and a
    normaliser that folds hiragana/katakana and composes jamo.
13. **Move to Vault.** Missing: the Vault itself (transactional deposit/withdraw), so no menu row.

---

## 8. Completion is not

A generation (or a shared component) is **not** complete because:

* its menu renders, a row appears, or a screenshot looks right;
* its host tests pass. Host tests prove the code does what the tests ask, not that the game accepts the
  bytes. Two P0s in this audit sit behind green suites;
* one generation's surface works and the others "use the same component" in name only;
* a keyboard opens, if it cannot type what the format stores or accepts what the format cannot;
* a writer "refuses most bad input" while any path still truncates, substitutes or blanks.

Completion **is** measured as: for every owner-listed area and every applicable format, the
shared component is the only implementation; every field's legal input equals PKHeX at the pinned
commit (golden tests); every write is all-or-nothing with export → reparse byte checks; and a
recorded hardware session shows the edit loading correctly in the real game or emulator. Every
status line in `CURRENT_STATUS.md` and the acceptance ledger must say which of these are proven.

---

## 9. Owner Round 13 hardware FAILs (9c301c48) mapped to shared components

Source: `origin/main:docs/hardware/round13/OWNER_RESULTS_9c301c48.json` (candidate
`PokeBank-NX-Gen1-5-Candidate-9c301c48`, NRO SHA-256 `0ecf8655…367d`, exported
2026-11-02T08:51Z). Result: **15 PASS / 12 FAIL**. The owner's notes are quoted verbatim. Each FAIL
is assigned to the **one** shared component that must fix it for every applicable generation. Fixing
it in a single generation's surface does not close it.

### 9.1 Owner P0: no workspace without a validated save

> **Owner P0:** "White/White 2 without assigned valid saves can enter editor/Party/Boxes. Route
> through Save Instances/validation before opening any workspace."

| ID | Owner note (verbatim) | Shared component that must fix it | Requirement and acceptance |
|---|---|---|---|
| **H01** (Install / identity) | "gen 5 i dont have a save for white 2 on purpose, i shouldnt be able to open a game save or the editor thing if the save isnt selected first, like i dont have a save for pokemon white but im able to click A on the main card and it opens the editor with parrty storage and all that, it should pop up with the choose save file thing or say no save accessible " | **Game-card open gate**, one function for every Gen I–V card kind: `SaveSelectScreen` card A/ZR handling + `UIManager::handleGen5View` (`UI.cpp:812-850`) and the Gen I–IV equivalents. `launchDescriptor.state=ChooseSource` (`SaveSelectScreen.cpp:1711-1713`) already models the right state but does not block the open | **P0-8.** No `TrainerViewScreen` (hub, Party, Boxes, Items, Trainer, editor) is constructed unless a Save Instance for *that exact game id and profile* resolved `Ready` and passed strict validation in this open. Otherwise open the Save Instance chooser, or show "No save assigned for Pokémon White". The root cause of the Gen V card opening without an assigned save **was not traced in this audit**, because `handleGen5View` requires a fingerprint. The sibling-binding path (`resolveFileForGame`, `SaveSelectScreen.cpp:1778-1786`) must be checked first: Black/White and B2/W2 saves share folders. Tests: a host model test for every exact Gen I–V game id × {no binding, binding to a sibling game's save, junk file, valid save} asserting the workspace opens only in the last case. Hardware: White and White 2 with no save → A shows the chooser or the message, never the hub |
| **R02** (Gen V new game, `white`) | "it shouldnt even go to that menu it should have the file menu pop up to assign a save since there is none" | same gate (P0-8) | same acceptance; "fresh save" means a *valid new-game save file*, never "no file" |

### 9.2 Keyboard / text input

| ID | Owner note (verbatim) | Shared component | Requirement |
|---|---|---|---|
| **R14** (Keyboards: Japanese/Korean & search) | "the nicknames and the name at the topm left changes but thats it , no keyboard changes or anyhitng" | `Utils::promptName` + `KeyboardDialog` driven by `TextFieldCapability` (§2.1); search via the same keyboard (P2-1) | The note confirms §3: changing Language changes the stored and displayed names (`R12-009` path), but the keyboard ignores the record language. Script pages are switched only by `japaneseKana` (gated) and `koreanHangul` (Gen IV Pokémon only). Fix = **P1-1, P1-2, P1-3, P1-5**. Acceptance on hardware for each of Gen III JP, Gen IV JP, Gen IV KO, and Gen I/II JP once the gate opens: set the record's language, open Nickname, and the keyboard **opens on** that script's page, offers only storable keys, and the name round-trips through Export. Gen V and VI+ show the documented "ASCII until glyph tables" reason (§7.8–7.9) instead of a silent Latin keyboard |
| **H10** (Names by Language, Gen III and V) | "dont know how to test make it it very simple for next round explaing what to do" | test-script, not code; covered by the capability matrix test (P1-1) | Next round's script must give exact steps: "Gen III: open a Pikachu, Language → Japanese, check the name reads ピカチュウ; Language → English, check PIKACHU; Export; reopen". Do not count as PASS or FAIL of code until it is run |

### 9.3 Footers and controls

| ID | Owner note (verbatim) | Shared component | Requirement |
|---|---|---|---|
| **R08** (Inventory Help: buttons and glyphs) | "gen 4 footer says edit quanitty not edit amout or what gen 1-3 have for the A button, \ngen 5 has almost none of the proper footers and im guessing controls" | One inventory footer/help source, `InventoryUIContract.h` + `InventoryHelp.h`, consumed by every bag surface (**P2-6**). Today labels are hard-coded per surface, e.g. `TrainerViewScreenBase.inc:5471` `"Edit Quantity"` while `BagTopUpModel.h:159` says `"Amount"`; Gen V bag is `Gen5BagScreen.h` | One `footerFor(bagState)` function returning identical glyph/label pairs for Gen I–V (and VI+); a host test enumerates every bag state × generation and asserts identical labels and that every listed button is handled. Hardware: same footer text in Gen I, II, III, IV, V Bag for A, X, Y, +, −, B |

### 9.4 Met location / met level verification

| ID | Owner note (verbatim) | Shared component | Requirement |
|---|---|---|---|
| **H05** (Met Location: verified catches first) | "pikachu has werid met locations" | Contextual met-location chooser: `SharedPokemonEditorSurface.inc:207 openContextualMetLocationPicker` + `Integration/Encounter/MetLocationPolicy.h` / `EncounterContext45.h` (Gen IV/V); Gen III must move onto it from `Gen3EncounterPickerModel.h`; Gen II Crystal from `Gen2PokemonPickerModel.h` | One verified-encounter source per game, with the same ordering everywhere (verified wild/static/gift for the species first; the UNVERIFIED list after, labelled). Golden tests per game: Pikachu's verified rows equal the PKHeX encounter tables at the pin (e.g. FRLG Viridian Forest / Power Plant; Platinum Trophy Garden; HGSS Viridian Forest; BW none wild → only gift/trade/event rows). The note does not name the game, so the next round must record it |
| **H06** (Met level: no too-high verified catch) | "" (FAIL, no note) | same component; level range is part of the encounter row | A verified row carries its legal level range; choosing it clamps/offers Met Level within it; the shared picker refuses a Met Level above the encounter's max. Test per game with PKHeX ranges |
| **R06** (Gen V event rows: distinct labels) | "all pokemon seem the have the same met location : mystery zone, link trade, PKMN Breedeer, other location, im pretty sure they all done have the same met location, this is so much fucking wasted time on the locations for gen 5 holy fuck" | same component, Gen V data `Gen5EncounterLocations.h` feeding `openContextualMetLocationPicker` | The Gen V verified list is not being built per species. Only the generic special rows show. Requirement: species-specific verified rows from the Gen V encounter tables (PKHeX `Encounters5*`), special rows (Mystery Zone / Link Trade / Pokémon Breeder / other) **after** them and only where legal. Host test: for each of several BW and B2W2 species with known wild locations, the first row is a real map location; for a species with none, the list says so |
| **R09** (Gen II Unown: Review visible) | "i didnt need to add a met location to ADD it to the box, which is wrong, met location should always be a part od the stuff before a pokemon is added, maybe this is cause its gen 2 but pokemon crystal" | `CreateRequiredFields.h` (shared Create rule) | Today `metLocationRequired(generation)` returns true only for Gen III–V (`CreateRequiredFields.h:46-49`), and Crystal caught data is deliberately optional (`:11-12`, "no caught data is native for a traded Pokemon"). The owner's direction overrides this for **Create** in Crystal: a created Pokémon must have a chosen location. The "traded / no caught data" case stays representable as an explicit chooser row, so no field is invented and the native "no data" stays possible. Gold/Silver have no met field and stay exempt. Change the rule to take the exact game (Crystal yes, GS no), and test Create in GS/C × chosen/not chosen. Owner decision recorded here; implement only with it |

### 9.5 Bag capacity / Top Up / Add Missing

| ID | Owner note (verbatim) | Shared component | Requirement |
|---|---|---|---|
| **H07** (Top Up: native capacity shown) | "again only adds some of the items see screen shots" | `SharedBagTopUp.h` + `BagTopUpModel.h` (used by `ClassicInventoryOverlay.cpp`, `Gen4SharedPokemonSurface.inc`, `Gen5SharedReadOnlySurface.inc`) | Before staging, Top Up must show exactly how many items fit (the pocket's native slot count and per-slot max, from the format backend). It either adds all of them or states, per item, why it did not ("Pocket full: 20 of 20 slots"). A partial result with no list of what was skipped is a silent drop. Host test per game × pocket: full, nearly full, empty; the reported count equals the staged count equals the reparse count |
| **H08** (Every TM/HM: Add Missing adds HMs) | "" (FAIL, no note) | same component (Add Missing) + per-format TM/HM pocket backend | Add Missing must include HMs in every game whose TM/HM pocket stores them (Gen I–IV HMs in the TM pocket; Gen V TM/HM pocket), refusing only with a stated native reason. Test: per game, after Add Missing every TM and HM id the game defines is present with the native quantity rule |

### 9.6 Not a shared-UI defect (recorded, out of scope here)

* **R04** (Launch and return, Quick Games): "retroacrch games so gens 1-3 crash on quit in retroarch, would be nice if we could have the return feature that we had but im not 100 hopeful, drastic wil always to to the launcher menu for drastic so no return to pokebank so not mad, dekopon we aint there yet and native games i dont care". This is emulator launch/return integration, not shared editor UX. It belongs to the launcher lane.

### 9.7 Effect on the work list

New P0: **P0-8** (validated Save Instance gate before any workspace, every generation). The work
items in §6 now carry these hardware IDs as acceptance:
P1-1/P1-2/P1-3/P1-5 ↔ **R14**; P2-6 ↔ **R08**; shared met-location chooser (new **P1-9**: one
verified-encounter source and ordering for Gen II Crystal, III, IV, V, with level ranges) ↔ **H05,
H06, R06**; `CreateRequiredFields` per exact game (new **P1-10**, owner decision) ↔ **R09**; Bag
Top Up / Add Missing capacity reporting (new **P0-9**: a partial add that silently skips items is a
silent drop) ↔ **H07, H08**.

**Top 10, revised:** P0-8, P0-1, P0-9, P0-2, P0-3, P0-5, P1-1 + P1-2 (shared keyboard), P1-9
(met location), P1-5, P2-6 (bag footers).
