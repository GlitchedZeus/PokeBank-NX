# HGSS egg PID history

`Gen4HGSSEggPidHistory` joins two source-attested stages for one **hypothetical, independently observed** HeartGold/SoulSilver egg event: `GenerateEggPID` creation and the later `SetBreedEggStats` collection transform. It first replays the exact MT/Everstone pending PID, then requires an equal nonzero egg-event token at creation and collection, and finally requires that same pending PID as input to the collection-time language/trainer-ID Masuda replay.

A detached PK4 has none of the parent held-item snapshot, LCRNG/MT transcript, parent-language snapshot, trainer ID at collection, or the two event identity tokens. Missing evidence is `Unknown*` or `CreationStageNotReplayed`; a mismatched event or pending PID contradicts only this proposed history. The helper never reconstructs parents or assigns a global invalid verdict.

The regression covers ordinary and Masuda paths plus absent creation/collection evidence, absent and divergent event identities, a borrowed pending PID, and a final-PID mismatch. The underlying creation and collection stages retain their direct original-source comparison suites.
