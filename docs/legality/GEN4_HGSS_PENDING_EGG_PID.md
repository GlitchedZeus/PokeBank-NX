# HGSS pending daycare PID creation

`Gen4HGSSPendingEggPid.h` implements the HGSS `Daycare_EverstoneCheck` and `GenerateEggPID` stages. It accepts source-attested creation-time parent item/PID facts, physical daycare order, an LCRNG entry state when needed, and a verified transcript of exact consecutive MT outputs. It returns the **pending** PID and consumed RNG counts. It does not reconstruct MT state, authenticate a transcript, or add an analyzer Invalid verdict.

This work starts from accepted `f6a17e15`, separately from Platinum PR #283. The refreshed accepted branch contains newer IV/parent-history implementations; this change does not overwrite them or reuse the older divergent #273 headers. PR #273's latest published commit passed its Host tests workflow, but that does not certify this new branch.

## Source-backed behavior

Pinned retail source: [pret/pokeheartgold get_egg.c at 9d8b7591f09b65804da2fb2dfd56f320633e0d36](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/get_egg.c), `Daycare_EverstoneCheck`, `GenerateEggPID`; `ITEM_EVERSTONE=229` in the pinned item constants.

- No Everstone: consume no LCRNG draws and retain one MT output directly.
- One Everstone in either physical slot: consume one LCRNG roll. Inherit that holder's nature only for roll `<0x7FFF`; exactly 32767 fails.
- Two Everstones: first roll chooses holder (odd→slot0, even→slot1), second applies the same threshold. Gender/Ditto mother-priority rules from Platinum must not be substituted here.
- If inheritance applies, accept the first **nonzero** MT PID with the selected parent's nature (`PID % 25`). Otherwise the game keeps trying.
- After 2,401 attempts, the game retains the last PID even if it is zero or has the wrong nature. This is a retail fallback, not the helper's search budget. A transcript shorter than that with no match is Incomplete, not proof of impossibility.
- If a transcript includes extra draws after the first accepted PID, it contradicts the caller's exact-consumed-count claim. More than 2,401 claimed consumed outputs is malformed for this stage.

`retailRetryLimitReached` identifies retention caused by exhaustion, rather than successful nature matching on the last allowed draw. The result's LCRNG endpoint is meaningful as known history only when the input endpoint is known; no-stone execution does not require or infer that otherwise-unused state.

## Required provenance and limits

The two item positions are physical daycare slots at PID **creation**, not necessarily the later egg-collection snapshots. Parent/MT stage tokens must be nonzero and equal, with explicit independent attestation. Missing item/order, creation-stage, needed LCRNG, needed parent nature or transcript evidence has a distinct Unknown status.

Parent PIDs are needed only when the Everstone gate succeeds. The caller must still establish that the parent identities/items and MT outputs belong to the same event; flags and tokens do not verify their own authenticity. Held item IDs are interpreted by the retail Everstone switch, not certified as a legal inventory by this helper.

HGSS applies international-language ARNG transformations later in `SetBreedEggStats`, at collection. This helper deliberately does **not** compare its pending result directly to the final encrypted PK4 PID or infer language history. Pending zero values are faithfully replayed as source-stage outputs; downstream daycare state handling and whole-egg feasibility remain separate. Parent eligibility, lineage, final PID transformation, IV generation and native history linkage are outside this stage.

## Final Masuda PID transform

`Gen4HGSSMasudaPidTransform.h` models the later `SetBreedEggStats` collection-time transform as a separate pure stage. When the two source-attested physical parent language bytes differ and the pending PID is not shiny for the source-attested trainer ID, it applies `PRandom(pid) = pid * 0x6C078965 + 1` up to four times, retaining the first shiny result or the fourth non-shiny result. Equal language bytes and an already-shiny pending PID retain the PID without a PRandom call.

The helper requires collection-time language and trainer-ID evidence and compares an explicitly supplied final PID. It does not treat a derived current PK4 language or trainer field as historical proof, and it does not reconstruct the pending PID or connect the creation and collection snapshots by itself. Its permanent normal and ASan/UBSan tests cover equal/different languages, already-shiny and four-failed-retry paths, both HGSS titles, mismatch, and unavailable inputs. This stage must be joined with the pending PID helper only after an external witness proves the same daycare/collection history.

## Independent verification

`python tools/check_hgss_pending_egg_pid.py /path/to/pokeheartgold` requires the exact pinned checkout. It reads original source through Git objects, extracts the two unchanged retail functions into a temporary compiler input, supplies explicit accessor/RNG-stream stubs and compares against the compiled implementation. It checks pending PID, final LCRNG state, LCRNG call count and MT consumption.

**903 comparisons pass**, covering both donor slots, all 25 nature classes, no/one/two Everstones, exact threshold seeds, the zero rejection, and the 2,401st-output fallback/last-draw match. These are synthetic stage/control-flow inputs; the supplied test transcripts are not asserted to be preserved retail RNG histories. No original function bodies, ROMs or personal save files are committed.

Permanent host and sanitizer targets are `test_gen4_hgss_pending_egg_pid` and `test_gen4_hgss_pending_egg_pid_sanitize`. They cover the boundaries above plus missing flags/tokens, mismatched stages, unsupported games, extra/empty/oversized transcripts and the SoulSilver alias. Normal and ASan/UBSan tests passed locally. Leak detection is disabled locally because this environment runs under ptrace; LeakSanitizer was not validated. Full repository/RSE/platform checks are left to exact-head PR CI before any acceptance.

No accepted branch movement, main change, save write or #103 merge is included.
