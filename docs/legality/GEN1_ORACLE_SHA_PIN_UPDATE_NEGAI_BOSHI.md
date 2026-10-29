# Gen I oracle workflow — coordinator `Legality.cpp` pin update (Negai Boshi lifetime fix)

`legality-gen1-exact-pkhex-oracle.yml` pins the coordinator tree's
`src/Legality/Legality.cpp` Git blob. This PR fixes a Gen III memory-safety
bug in that file, so the pin moves:

| | blob |
|---|---|
| before (integration head `5bf98b11`) | `aeede14171542247eb633451c50bd9b2074c49dc` |
| after (this fix) | `aedcff288860e2edc79944775dd049cd497d4d76` |

## What changed in `Legality.cpp`

Only `addGen3PidEvidence`, in the Gen III BA-CD event block:

- `Gen3NegaiBoshiEvent::Candidate` stores the OT name as a
  `std::u16string_view`.
- `negaiCandidate` was a named variable built from `pk.otName()`, which
  returns a `std::u16string` temporary. The temporary was destroyed at the
  end of the declaration.
- `matchesForceAntiShiny` and `matchesRestrictedTable2` then compared
  against the destroyed string.
- AddressSanitizer reports `stack-use-after-scope` in
  `Gen3NegaiBoshiEvent::commonFields` on a Japanese Negai Boshi Jirachi
  (Ruby, TID 30719 / SID 0, OT ネガイボシ, PID `F86484EA`, IVs
  10/12/22/0/7/29).
- The fix copies the OT name into a local `std::u16string` that outlives
  the candidate.
- No rule, table, message or verdict changes. The other Gen III event
  templates pass `pk.otName()` inside a single call expression, where the
  temporary lives long enough; they are unchanged.

The edit does not touch any Gen I path, so the Gen I oracle's expected
outputs are unaffected. Only the blob identity changes.

## Evidence

- `tests/test_gen3_legality_context.cpp` gains a Negai Boshi case through
  `Legality::analyze`.
  - On the unfixed base, its sanitizer build fails with the
    `stack-use-after-scope` above. The normal build passes only because the
    freed stack bytes happen to survive.
  - With the fix, both builds pass. The template still matches the
    distribution record and still rejects a renamed OT.
- `python3 tools/legality/verify_gen1_oracle_source_pins.py --self-test`
  and the plain run both pass with the new pin.
