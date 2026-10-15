# PokeBank NX Project Status

## CURRENT PRIORITY — October 15, 2026

**Freeze application scope at Generations I–V** until a complete code audit and owner Switch hardware torture test pass. Active [Gen I–V PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) at `35cbf163` passed all nine exact-head CI checks and published a combined NRO, **not yet device accepted**. Earlier combined `fa9cfcea` failed physical Gen V testing; its hub, blank-slot Create and trainer-state regressions are now mandatory retest items.

Owner acceptance = **all 20 Gen I–V games** plus every reachable app action (button and touch, supported source/emulator, profile, staged edit, Export Copy, B/confirmation/exit) tested on one exact frozen NRO with artifact SHA-256. [Full 106-checkpoint starter matrix](docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md) · [Single acceptance tracker #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313).

**Gen VI/VII/VIII backend drafts are preserved but on hold for active Switch app integration**; only when the owner accepts Gen I–V do we start the different Gen VI emulator phase. The project cleaned four superseded PRs and moved 18 post-v1 ideas to [deferred index](docs/FUTURE_AFTER_GEN15_BACKLOG.md), leaving 28 open PRs/28 open issues. No original save writing or mainline draft merges were authorized.

---

**Last checked: October 14, 2026.** This is a milestone summary, not a fictitious completion percentage. [Current engineering state](CURRENT_STATUS.md) · [Full cross-lane snapshot](docs/ENGINEERING_SNAPSHOT_20261014.md).

## Product identity
PokeBank NX is a native Nintendo Switch, offline, C++20, HOME-inspired Pokémon save manager/editor. Status: **active alpha**. Core principles: real generation-native data, shared staged editor, explicit profile/source identity, immutable original external saves and exact-NRO hardware testing. Default `main` is not the combined development release branch.

## Milestone progress
| Area | Implementation | Acceptance / outstanding |
|---|---|---|
| Gen I Red/Blue/Yellow | ✅ Full accepted native/editor foundation | Earlier Switch hardware passed; new integrated UI retest |
| Gen II Gold/Silver/Crystal | ✅ DV, Stat Exp, Pokémon/Items foundation | Earlier Switch hardware passed; new integrated UI retest |
| Gen III R/S/E/FR/LG | ✅ Strict sources, Trainer/Party/Boxes and staged editor | Earlier Switch hardware passed; latest source/launch/touch retest |
| Gen IV D/P/Pt/HG/SS | ✅ Accepted prior integrated milestone; advanced editor/Items | Claude [#306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) CI and hardware fixes ongoing |
| Gen V B/W/B2/W2 | 🧪 [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) implemented staged PK5 View/Edit/Create/Review/export + active-profile Save Instances | ✅ Six CI gates green; ❌ owner hardware still pending |
| Modern Home / Touch | ✅ Earlier working UX; additional narrow Actions/assets/touch | New exact candidate must be tested |
| Legality Gen I–IV | 🔬 Five specialist lanes with Gen I-first original-game research | [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) + Claude [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) combined CI blockers |
| Gen V–IX legality | 🔬 [#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) structural foundations | Full histories/PKHeX parity not available |
| Master Vault, Banks, Search | ⏳ Entry points / backlog only | Persistent Vault and global collection engine future work |
| Original emulator / installed-save writes | 🔒 Forbidden by default | No broad Inject Save or unsafe writes |

## Active branches and milestones
- [Gen I–IV finalization #306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) — Claude works on packaged NRO art verification, per-root DS discovery fairness, ROM header identity and combined hardware polish; its candidate/native/Gen IV/touch gates have passed at `3ac0ae29`, with the full host suite still running and real-device acceptance pending.
- [Gen V integration #300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) — `58bd564ba479745f6129f57167aeac6616297753` passed six checks, NRO [run 38523683607](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38523683607). Still need Switch check of existing emulator-folder save discovery, per-profile assignment, primary/backup copies, shared editor, and original-save immutability.
- [Gen V source-hardening #304](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) — stronger persistent copy/identity and provider/focus UI handling; separately tested stacked draft, awaiting reconciliation.
- [Gen IV Ball #305](https://github.com/GlitchedZeus/PokeBank-NX/pull/305) — unmerged `max(0x83,0x86)` correction.
- [Read-only legality #103](https://github.com/GlitchedZeus/PokeBank-NX/pull/103), [coordinator #298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), [Claude #303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) and [later-gen #301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) — isolated, incomplete, and subject to exact source/CI review.

## Release priorities
1. Complete #306 full host CI, then supply **one exact Gen I–IV NRO** for owner to test artwork, source selection, Items, input/touch, launch and Quit & Return.
2. Test #300's already CI-green Gen V NRO with the owner's **existing** save layout. Assignment to Switch profile must persist after emulator save updates; no file relocation required by design.
3. Complete touch-only parity and one trustworthy artifact-hash record per accepted round.
4. Continue legality independently, integrate only verified subsets, use Unknown/Incomplete whenever authentic original-game evidence is absent.
5. Later: generation-by-generation format extension, persistent app-owned Vault/Banks, broader adapters, and separately authorized safe write transactions.

**No progress percentages are inferred from PR, file or test counts.** Passed CI, source implementation, and physical Switch acceptance are different milestones.
