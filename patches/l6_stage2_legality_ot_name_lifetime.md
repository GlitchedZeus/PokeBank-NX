# L6 Stage 2 coordinator patch: preserve Gen 3 event OT-name lifetime

**Target:** `src/Legality/Legality.cpp` in the coordinator integration branch.

## Finding

`Gen3NegaiBoshiEvent::Candidate` stores its `otName` as a
`std::u16string_view`.  The named `negaiCandidate` was initialized with the
temporary result of `pk.otName()`, then used in subsequent expressions.  Clang
correctly rejects this as a dangling view under the project's warning-as-error
policy.

This is a coordinator-owned production fix proposal, deliberately not applied
on the isolated Codex audit branch.

```diff
@@
                 const bool pomegCheck =
                     Legality::pomegGlitchCheck(
                         pk, true, pk.pid(), dexEntry, sourceGroup,
                         CheckIdentifier::PidRng);

+                const std::u16string otName = pk.otName();
+
                 const bool mystryMew =
@@
                 const Gen3NegaiBoshiEvent::Candidate negaiCandidate{
                     species, pk.tid16(), pk.sid16(), pk.originGame(),
                     pk.language(), pk.otGender(), pk.metLevel(),
                     pk.metLocation(), pk.ball(), pk.isEgg(),
                     pk.isFatefulEncounter(),
-                    pk.isShiny(pk.id32(), {}), pk.otName()
+                    pk.isShiny(pk.id32(), {}), otName
                 };
```

For consistency, the coordinator may also pass `otName` to the other
immediately-evaluated Gen 3 event candidate initializers in this block.  The
named `negaiCandidate` is the required safety fix because it survives the
full-expression that created it.
