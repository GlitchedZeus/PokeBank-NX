# Historical evidence continuation — October25,2026

Starting PR314 head: `0b01ad6c614c1652a6c9591195ebff22cb761d8f` (all49 baseline checks green). Forward-only WIP `wip/legality-evidence-20261025`. Application PR307/main remain untouched.

Implemented checkpoint, not a historical authentication certificate:

- Downloaded and verified all81 `.wc5full` files at EventsGallery154d81be88453f6f78ec1d6d86e85fe0f2f5c240, tree5c6fda190f297c4cc80c0201fed0c078e732c46a. All81 match published Git blob/SHA256 and CRC16-CCITT-FALSE; XMODEM matches0. Inventory63 released/18 unreleased.
- Corrected the reader's real raw-mask bug: archive flags10/20/30/F0 are not catalog masks1/2/3/F. Single B/W archive witnesses establish10Black/20White; all-fourF0 is observed. Unobserved/unknown combinations, including isolated sequel bits, stay Unverified rather than invented.
- Source map/research swaps Italian/German: actual archive and pinned PKHeX LanguageID confirm4Italian/5German. One unreleased file labelled ITA is byte-identical to a Spanish file and has raw language7; filenames never determine eligibility.
- Added an explicit receipt-date-only payload relation. All63 released full-card PGFs match63 catalog payloads only after accounting for the documented blank day/month/year bytesAC..AF. Exact matching remains exact; every non-date byte and malformed-date refusal is protected. All18 unreleased/debug cards remain unmatched and excluded.
- Generated source-bound released-template evidence and wired INFORMATION ONLY into the real Gen5 gift candidate/report path. Archived full-card game/language claims remain per-card pairs; never combined across cards. No receipt authentication, no Unknown promotion, no changed legality verdict intended. The existing newer-catalog warning is retained.
- Added real81-file C++ replay, catalog mutation/refusal checks, generator controls rejecting even CRC-recomputed tampering, and four original retained English/Italian Karrablast/Meloetta PK5 records through production analysis. Existing six receiving regressions stay intact.

Sources: [archive](https://github.com/projectpokemon/EventsGallery/tree/154d81be88453f6f78ec1d6d86e85fe0f2f5c240), [field map](https://projectpokemon.org/home/docs/gen-5/5th-generation-wonder-card-full-map-r148/), [original format research](https://projectpokemon.org/home/forums/topic/43342-gen-5-full-wondercard-structure/), [PKHeX LanguageID](https://github.com/kwsch/PKHeX/blob/9c3391d017d91ba407ef53075b15581f1bec8f0d/PKHeX.Core/Game/Enums/LanguageID.cs), [PGF receipt-date layout](https://github.com/kwsch/PKHeX/blob/9c3391d017d91ba407ef53075b15581f1bec8f0d/PKHeX.Core/MysteryGifts/PGF.cs).

Checks complete: GCC and Clang each13 normal+13 ASan/UBSan configurations PASS,52 total, with leak detection and halt-on-error enabled. All81 original full cards replayed in four compiler/mode configurations. Generator regeneration and four source-forgery controls PASS. All135193 retained Gen5 native outputs are byte-identical to the prior engine, including4278 Incomplete. Python syntax,46 workflow YAML parses and diff-check PASS.

Next: recheck remote and forward the tested checkpoint to the existing draft PR314, then require exact-head CI. SOURCE_ASSESSMENT.md records all eight leads and their explicit limits. No application merge/deployment is authorized in this session. Original five Enigma cases and unauthenticated historical distributions remain Incomplete.
