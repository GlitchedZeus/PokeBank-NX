# RNG-002 — retained Gen IV PID + 129 rejected pairs with honest search budgets

The Gen IV nature-rejection stage may reject more than 128 generated PIDs. An internal reverse-helper budget of 128 is NOT a retail bound, and exhaustion cannot show an impossible encounter. The existing `Gen4NatureReversal::search` already has a distinct `SearchExhausted` status; this milestone adds `verifyKnownCandidate` requiring an **independently verified pre-PID state**, exact forward-reproduced *retained PID*, matching nature, then the bounded reverse nature-rejection search. It keeps missing original RNG stage, wrong retained PID, invalid nature and insufficient budget distinct.

Pinned synthetic HeartGold-compatible stage `0xE0D14A4F` produces PID `0xEA0705DB`, nature 20, and an independently enumerated **129 rejected PID pairs**; budget 128 is SearchExhausted, budget 256 produces 130 inspected pairs and conditional success. One and two rejection pinned specimens remain positive. Tests run under host/ASan/UBSan and verify unknown original stage, altered PID, wrong nature, invalid nature and zero budget.

A conditional matched stage does NOT automatically prove a full native PK4, encounter area, lead ability, slot, frame, origin game or source policy. Its match is not global Legal; a mismatch is not global Invalid. Source saves and staged Pokémon are not mutated.
