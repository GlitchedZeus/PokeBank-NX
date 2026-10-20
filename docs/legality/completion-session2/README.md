# Gen I–V engineering checkpoint, session2

The finite Gen II joint-parent verifier, actual Gen III save/workspace evidence bridge and Gen V receiving-metadata check are production code. This is **not**100% Gen I–V parity, an exhaustive historical-distribution audit or a release certificate.

The forward commit preserves contributor60b4bce954b1e0404cb0dc57210cb979ff546728 (Hordel’s Elekid fixed-nickname behavior across Gen IV/V). Application PR307 is unchanged; a five-file integration patch is prepared and hash-bound to c3b0ac4812ef9d584de5d74109189c3ac897fe24.

## Concrete changes

Further source review corrected omitted Crystal tutor inheritance in the ancestry adapter. Three valid unhatched eggs combining an egg move with Ice Beam, Flamethrower or Thunderbolt had been rejected; all now pass with a compatible joint donor. `TUTOR_INHERITANCE.md` records the original-game evidence, production regressions, negative controls,150000 byte-identical existing GB replays and an additional declared30000-record holdout. Frozen campaign metrics are unchanged.

- `FiniteBreedingChain.h` saturates joint subsets of up to four moves through one father, compatible maternal groups/sex/Ditto, evolution, native learning, previous-era acquisition and distinct gift seeds. Finite saturation has no depth/time cutoff. Different event individuals are never merged. `GBLegality.cpp::evaluate` applies the joint constraint per encounter candidate and continues alternative origins. A native witness is preferred; a catalog-only historical-gift witness remains Incomplete.
- All38 original raw false-clean records were loaded byte-for-byte and tested through the production analyzer.26 concern Japanese punctuation and alternate Gen II ancestry;12 are original Japanese Crystal GS Ball Celebi. All38 have specific source-backed explanations and remain clean. Zero defects were found in these38 bytes; zero were hidden or removed. The evidence limits of the official VC naming manual and unreconstructed Japanese disassembly are explicit in `SOURCES.md`.
- Original12 newer-PKHeX chain cases:8 Invalid,2 native-source-backed clean,2 Incomplete. IDs7517,7553,7582,9123,9429,9484,9869,9891 are Invalid.9615 uses Lapras’s Gen I Skull Bash followed by Gen II Safeguard;9744 uses Oddish’s Gen I Mega Drain followed by Gen II Moonlight.7527/7575 require unverified PCNY gift ancestry and Korean-Western trade. See `BREEDING12.json` for every raw record, verdict and witness.
- `Gen3WorkspaceAdapter.h` selects the Pokémon and berry evidence from the same strict validated save or current staged workspace. It preserves title, slot, active copy and save counter. `Gen3BerryContext` accepts recognized emulator footers, distinguishes malformed berry evidence from absence, and reports integrity failures as incomplete evidence.280 native title/rotation/presence/integrity/save-language tests cover party/box selection, edits, ownership and immutability. The isolated app patch adds save-owned context with explicit Bank ownership checks; native app deployment remains pending.
- The newer Gen V PGF resource appends709 paired receiving-language/game bytes to the **identical**709 payloads. The generator verifies byte identity/order and both resource hashes. Unsupported receipt cannot certify a Pokémon; a fully supported alternate remains available.3515 former clean certifications now remain Incomplete. No new historical payload was independently authenticated. Two old positive fixtures used an English trainer for Japanese distributions: Valid assertions are preserved with corrected Japanese bytes, independently accepted by the newer oracle; original bytes are retained as Incomplete regressions.

## Observed rates on the same retained inputs

These are corpus rates, with every raw disagreement included. Baseline reference6501f0ab46e8f8ca048539dbaf8cae8cb104e722; newer reference6042efa241a9925ce57e645329ea7f976deabdd3. Raw exact agreement counts only Invalid/Invalid and clean/Valid. Three-state agreement additionally compares Fishy to Incomplete; it does not count Incomplete as clean. Synthetic records and correlated mutations do not authenticate historical events.

### Baseline reference

| Gen | Invalid detected | Valid fully certified | Raw exact agreement | Three-state agreement | Invalid→Incomplete | Valid→Invalid | Invalid→clean |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 86.4808% | 98.2511% | 95.1360% | unmeasured | 1788 | 269 | 1 |
| 2 | 91.6300% | 97.5741% | 95.8200% | unmeasured | 1198 | 507 | 37 |
| 3 | 99.9697% | 96.0261% | 98.0156% | 99.1757% | 5 | 0 | 0 |
| 4 | 100.0000% | 96.9688% | 98.6195% | 97.8370% | 0 | 45 | 0 |
| 5 | 99.5778% | 93.6440% | 96.8356% | 87.1598% | 307 | 0 | 0 |

### Newer reference

| Gen | Invalid detected | Valid fully certified | Raw exact agreement | Three-state agreement | Invalid→Incomplete | Valid→Invalid | Invalid→clean |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 86.4946% | 98.3073% | 95.1760% | unmeasured | 1789 | 249 | 1 |
| 2 | 91.6438% | 98.0579% | 96.1420% | unmeasured | 1205 | 340 | 43 |
| 3 | 99.9697% | 96.0261% | 98.0156% | 99.1757% | 5 | 0 | 0 |
| 4 | 99.6519% | 96.9777% | 98.4391% | 97.6586% | 5 | 45 | 92 |
| 5 | 94.9179% | 99.2293% | 96.7964% | 86.8418% | 3824 | 0 | 53 |

The newer reference still has92 Gen IV and53 Gen V raw clean disagreements, including5 Gen V oracle analysis errors. They are retained individually in `CURRENT_GEN4_FALSE_CLEAN.json` and `CURRENT_GEN5_FALSE_CLEAN.json`; they are **not** approved exceptions. Nine Gen V cases involve the newer chain rule. Current-reference parity remains failing. The45 retained stricter Gen IV met-field results were recomputed directly from pinned native setters/ranges; all45 and17 fresh records contradict native paired-field writes. See `GEN4_MET_LOCATION_REVIEW.json`. The307 original Gen V invalid-but-Incomplete cases remain unresolved:298 unmatched fateful-event histories and9 other cases. See `GEN5_INCOMPLETE_CLUSTERS.json`.

The38 original GB differences remain in all raw denominators. The GB dual-reference audit accounts for every discrepancy on the retained/final fresh sets, using unchanged old predicates plus narrowly checked newer inherited-move rules or a constructive pret-only witness. It rejects unrelated complaints and rejects a new Invalid when an independent native construction exists. It never rewrites either oracle’s verdict.

The first published-head GB CI also found one additional raw Valid→Invalid result: an unhatched Smoochum with Meditate and Blizzard. The native single-donor/sex/TM invariant supports the stricter result; the newer oracle omits inherited TM moves from its joint check. `CI_SMOOCHUM.md` retains the exact artifact hash, source review, two clean positive controls and guarded differential adjudication. It is additional CI evidence and does not change these frozen retained-corpus rates. Production's constraint is preserved.

## Evidence and remaining scope

`OBSERVED_METRICS.json` contains exact numerators, denominators, file hashes, per-title/origin rows and fresh-set measurements. `DATASETS.json` identifies every retained/fresh input. All355997 original record evaluations were replayed; final declared seeds add163176 records, including correlated adversarial mutations. The development50k seed8174003 was retired before final817500x seeds were declared. Full byte streams, warning streams, proofs, mismatches and logs are packaged in the workspace evidence archive; no record is silently excluded. Gen I/II source-save title identity is unavailable in these corpora, so per-title accuracy is unmeasured.

Semantic-rule completeness, original-source coverage, historical-event completeness and deployed production coverage remain **unmeasured for every generation**. Their applicability/evidence denominators have not been independently audited. New absolute evidence consists of1 first-party historical GS Ball availability statement,2 constructive native breeding routes,709 catalog receiving annotations and0 newly authenticated historical distribution payloads/specimens. API/host reachability is tested; native UI/hardware deployment is not.

`HANDOFF.md` identifies every substantial remaining category, missing condition, source evidence, code entry point, retained reproduction and next engineering action. Gen III–V general father-chain integration, Gen IV newer HGSS rules, authentic event history and actual source provenance remain open. Independent human/agent QA and native NRO/device acceptance have not occurred. Final exact-head CI must be read from the published commit; earlier green runs are not attributed to this checkpoint.
