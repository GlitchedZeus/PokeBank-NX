# L4 Stage 2 — Gen III original distributions and encrypted native evidence

**Scope:** independent Gen III candidate-local helpers and compiled native regressions. No actual historical archive payloads are included. All distribution profile inputs in the tests are explicitly synthetic and do not identify a valid real-world event or possession receipt.

## Implemented logic

- `Gen3DistributionWitnessEvidence.h`: typed original-source hypotheses for WISHMKR, Channel, PCNY/PCJP, Mystry Mew, GBA Wondercards, Berry Fix, GameCube gift, Shadow encounters, and event-island encounters. Gift / service / original in-game encounter / Shadow mechanisms are distinct. Berry Fix is a service mechanism, not proof of a gifted Pokémon. GBA Wondercards may be direct gifts *or* unlock original encounters, subject to separately verified original mechanisms. No per-version distribution availability is inferred from family names.
- Independent original archive identity, nonzero verified original payload digest, independently attested extracted original fields, release corroboration, and individual historical receipt are separate gates. Candidate observations may be compared only where original source fields and specific unchanged or fully verified changed histories are independently known. Template match proves neither original historical distribution nor an individual Pokémon's provenance. No user data is modified.
- `Gen3DistributionNativePK3Evidence.h`: read-only strict 80-/100-byte encrypted PK3 detection (Bad Egg, zero-species, checksum quarantines), decryption and canonical field projections: internal species, PID, TID/SID, original trainer gender, language, raw native nickname and OT bytes, origin game, met level/location, ball, four moves, held item, egg, fateful and ribbon word. The 16-dimensional result keeps Unknown, conditional match, and candidate mismatch distinct; missing history, changed moves/item/species, egg/trade character encoding and language transformations remain Unknown unless there is independently verified final-state evidence. No globally Invalid/Valid determination.
- Original event claims such as PCNY/PCJP, WISHMKR/Channel, Manaphy (Gen IV, later stage), and specific Wondercards require independently reviewed per-distribution primary payload / original source history; this stage does **not** create a synthetic authenticated event table.

## Source references (mechanics, not license grants)

- Existing PokeBank NX `include/Pokemon/Pokemon3FRLG.h` and `src/Encryption/Encryption3FRLG.cpp`, original gen-3 canonical PK3 substructure, PID%24 permutations, XOR with PID^OTID32, native header and 48-byte data checksum; exact committed encryption source Git blob `eec27b73a15d6652371ed2e066690ef0fa9fb7ef`.
- Decompiled original Emerald Mystery Gift card handling (source mechanics only): https://github.com/pret/pokeemerald/blob/master/src/mystery_gift.c
- Original Emerald Mystery Event script operation: https://github.com/pret/pokeemerald/blob/master/src/mystery_event_script.c
- Original FireRed Mystery Gift card and received-gift handling: https://github.com/pret/pokefirered/blob/master/src/mystery_gift.c

These are cited solely to audit mechanics. No retail ROMs, event archives, external event payloads, or third-party source code are copied into the L4 PR. Original archive redistribution rights and individual historical distribution receipts remain unverified.

## Actual compiled tests (focused Stage 2)

Two new standalone tests:

1. `test_legality_l4_gen3_distribution_matrix.cpp`: **28,160** family/mechanism/original-source/provenance combinations, **16** per-field candidate negatives, partial release/receipt, unknown history, explicit verified-final alternatives, legal encoding changes, archive trust and digest negatives, no-global-Invalid invariants.
2. `test_legality_l4_gen3_distribution_native_pk3.cpp`: **48** real Gen III encrypted permutations/sizes, **768** encrypted native original-field negatives, **18,432** one-bit encrypted payload checks, empty box/party, bad egg, checksum tamper and short/oversized record negatives, source buffer immutability.

Ran both with GCC/Clang C++20 `-Wall -Wextra -Wpedantic -Werror` in normal O2 and with AddressSanitizer + UndefinedBehaviorSanitizer O1: eight program executions, all PASS locally. The copied encryption implementation's exact Git blob matches the upstream repository. The focused compile uses only a declaration-only stand-in for the unused `Encryption/Encryption.h` save-layer header, not a substituted encryption implementation. The full GitHub checkout's 18-program L4 suite normal and sanitizer execution, and shared CI, **remain to be verified by coordinator**. PR is draft and unmerged.

## Integration holds

Do not use synthetic candidate-attestation booleans as factual original receipts. Do not mark globally Invalid from a single disproven or unsupported distribution template. The native PK3 projection is factual only for a structurally valid encrypted record; original event sources require independently verified payload, release and history. No modification to `Legality.cpp`, other lanes, accepted branch, #298, shared CI, or MAIN.
