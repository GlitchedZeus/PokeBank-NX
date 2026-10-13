# Gen I Red/Blue/Yellow coordinator acceptance ledger — October 13, 2026

**Owner-authorized coordinator:** [draft PR #298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298).
This is a category- and evidence-based decision, **not** acceptance of whole
Pokémon original retail history, original PKHeX parity or an NRO shipment.
Five independent specialist PRs remain paused/open/draft and are not reset,
force-pushed or merged into product main/accepted legality #103.

## Source accepted, runtime acceptance still blocked

| Discipline | Source-level coordinator disposition | Native/PKHeX runtime acceptance |
| --- | --- | --- |
| L1 original encounters | Original original-map/fishing/static/trade slot data integrated, candidate-only; 1,512 new RBY physical rows and legacy 149 indexed rows retained | **Blocked**: exact combined host/sanitize, original slot/catch RNG and acquisition-event witnesses |
| L2 native DVs and Stat Experience | RBY four DVs + derived HP DV + five 16-bit StatExp source mechanics, single/divided battle/limited vitamin & alternate candidate histories integrated | **Blocked**: full native parser and cross-generation ABI/sanitizer gates |
| L3 original level-up | **Source-verified**: exact original pret/pokered 728 and pret/pokeyellow 755 level-up offers plus identical 72 original evolution entries for all 151 species. Independently re-parsed immutable original assembly and compared each generated packed string byte-for-byte | **Blocked**: real retail four-slot path/candidate completeness, exact-head host/san and original PKHeX.Core executable comparison |
| L3 TM/HM compatibility | **Source-verified**: all **151 species × 50 TMs + 5 HMs** from original per-species base_stats, 3,037 R/B and 3,043 Yellow source-allowed pairs. 128 base-stat files have identical original Git blob IDs, 23 changed Yellow originals read independently; eight species differ in compatibility. Original source blob manifest + automated CI source replay committed | **Blocked**: item/history receipts, event-only machine exceptions, real four-slot acquisition, exact-head normal + ASan/UBSan |
| L4 gifts/NPC/OT/raw native | Original cartridge source-linked receipts and candidate-local preflight/raw file evidence integrated; Yellow original trade scenes distinguish unused source table entries; no modern Gen I ability/gender/nature demands | **Blocked**: original archive payloads, authentic per-individual provenance, current full-native join |
| L5 original PK1/PKHeX | Independent genuine original PKHeX.Core C# host harness and 72 synthetic test vectors integrated. Source review identifies accepted baseline F3/F4 absent Gen I ability and gender risks; coordinator analyzer already gates modern verifier by native generation | **Blocked**: original C# vs actual two native executable differential, native all-151 × six source labels, normal/sanitizer and audit. **0/21** complete R/B/Y × 7 historical-method certificates |

## Separately accepted evidence

- Original `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`
  and `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`
  exact source identities and per-file Git SHA for original level-up,
  evolution and machine tables. National Dex indices mapped from
  original include order; R/B Mew is a separately defined final species,
  while Yellow Mew is included in its `base_stats.asm`.
- Original Mew supports all 55 machine flags; the unused 56th Red/Blue
  padding flag is excluded. Ditto's machine bitmap is zero. Standard
  Yellow Charizard Fly differs from original Red/Blue; event-only
  Surfing Pikachu is **not** inferred from its normal species bitmap.
- The original source data was independently parsed and compared using
  live GitHub file contents and immutable Git blob identities.
  **Source-pass does not equal full host-test-pass.** Re-run
  `tools/legality/verify_gen1_original_machines.py` against exact
  pinned original Git checkouts in GitHub Actions.

## Exact-head runtime gates still awaiting execution

- Coordinator native original-machine tests: all three original game
  namespaces × 151 species × 55 machines, exact Red/Blue vs Yellow
  masks, invalid method enum bytes 3..255, malformed game/species/number;
  normal and ASan/UBSan under GCC and Clang.
- The all-lane Gen I coordinator 20-suite focused smoke, full L1–L5
  native suite register, full host/RSE native matrices, normal and
  ASan/UBSan; no NRO or original-save write.
- Actual original pinned .NET 10 `PKHeX.Core` PK1 legal analyzer against
  real accepted/coordinator original-source PK1 wrappers, plus original
  save pipeline, real save bytes and Gen II–IV regression controls.
  The workflow `legality-gen1-exact-pkhex-oracle.yml` references the
  *current exact coordinator PR head*, not a stale old SHA.
- Independently authenticated source version/history from an original
  Red, Blue or Yellow save. Detached PK1 has no native original-game
  provenance bit. **Never** promote a failed acquisition candidate into
  an entire-Pokémon Invalid when unenumerated alternatives remain.

This ledger approves the provenance and original-game source TABLES as
reviewed evidence in **draft coordinator**, not merge into accepted
legality #103, main app or 1.0. Historic Unknown/Incomplete stays so.
Original source saves are immutable; no writes permitted by this work.
