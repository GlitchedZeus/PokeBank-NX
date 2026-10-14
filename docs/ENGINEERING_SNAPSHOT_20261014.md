# PokeBank NX — Engineering Snapshot (October 14, 2026)

> This is a **dated** audit of live GitHub PRs/CI, **not** a changelog of merged code or a physical-device acceptance certificate. Re-fetch each branch before making further decisions. Default `main` does not contain all of these development branches.

## Current acceptance rule

| State | Meaning |
|---|---|
| ✅ Owner accepted | A physical Nintendo Switch test was completed against the **exact** application build/NRO |
| 🧪 CI green, device pending | An actual development implementation passed normal/sanitizer/native checks; **hardware still unverified** |
| 🚧 In progress | Code/research exists, but CI, integration or review has blockers |
| ⏳ Planned | A menu entry, artwork, issue or experiment is not production editor/storage functionality |

**Last recorded accepted integrated Gen I–IV/Product Home checkpoint:** application `0760334fb850ca46b5171c55f690998d7680d4e7`, NRO SHA-256 `4f28d7f7ad38c2b62458642e014ae3c05e866755c4152bf4c2db0574c381ce91` (owner Oct 8). The owner confirmed launch / Quit & Return in that round. No newer NRO acceptance is established by CI alone.

## Application and editor lanes

| PR | Exact HEAD observed | Real work / acceptance boundary |
|---|---|---|
| [#92 — Gen I–IV parent](https://github.com/GlitchedZeus/PokeBank-NX/pull/92) | `ddaff2470947ed3ca579ec22eb25c7564e215a8c` | Accepted earlier UI/editor ancestor; parent remains draft/unmerged |
| [#278 — narrow actions](https://github.com/GlitchedZeus/PokeBank-NX/pull/278) / [#280 — touch parity](https://github.com/GlitchedZeus/PokeBank-NX/pull/280) | `d0a45909` / `dcd855a7` | Shared narrow greyed-out actions; Gen III/IV empty-slot tap/A parity; owner hardware retest outstanding |
| [#306 — Claude Gen I–IV finalization](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) | `6f3b4ddf86f1cd8751025c4cde41fd1be07820de` | 10 region backdrops / 20 trainer portraits, all 16 Gen I–IV covers and in-NRO RomFS package verification, fair DS per-root discovery, GB/GBA/NDS ROM-header checks. **Gen IV candidate CI failed at last check; other checks active.** |
| [#300 — Gen V shared application](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) | `58bd564ba479745f6129f57167aeac6616297753` | Exact BW/B2W2 Save Instances, source/profile assignment, validated primary/backup copies, native Trainer/Party/24 Boxes, staged PK5 View/Edit/Create/Review, working-copy export, source diagnostics and artwork. **All six exact-head CI checks passed. Owner hardware pending.** |
| [#304 — independent Gen V source-hardening](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) | `4d8c267e8b86df15aeb0002df89207cad0516211` | Persisted exact copy/snapshot/profile identity, alias and moved-file guards, equitable provider source budgets, manual repair and focus/scroll refinements. Stacked on #300; review/reconciliation pending. |
| [#305 — Gen IV Ball correction](https://github.com/GlitchedZeus/PokeBank-NX/pull/305) | `b8eb006ebb85900684ce4caac4d75a18ebb5a8be` | PK4 capture Ball `max(0x83,0x86)`; separate unmerged change. |

### Gen V current CI/device evidence

- At `58bd564ba479745f6129f57167aeac6616297753`, Gen V Focused Host (including ASan/UBSan), Full Host Tests, Native PR Gate, Touch Input Gating and both Gen I/II packed-move gates **all succeeded**.
- [Native NRO Actions run 38523683607](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38523683607) produced **`PokeBank-NX-Gen5-Save-Instances-38523683607`**. This is a development artifact, **not owner-device accepted**.
- Previous owner report: Gen V saves already exist in the **same emulator folders as the working Gen IV saves** but did not appear; region backgrounds, covers and trainers were missing in the tested build. The next exact-head Switch test—not another guess about save locations—must determine whether the issues are fixed.
- New assignment UX: on active profile, Source/Game File → **Assign Save to This Profile** → choose emulator or manual source → return to Games; the normal A/Open route opens the validated shared editor separately. The original save remains read-only.
- A normal emulator save update must not invalidate a remembered FAT/exFAT profile link (new normalized path identity). A new explicit editor-opening session **still** requires exact native validation and a matching selected-content fingerprint. Copy choice remains explicit. Pre-fix remembered assignments may need conscious relinking.

## Legality and research

| PR | Observed HEAD | Status |
|---|---|---|
| [#103 — isolated Gen I–IV accepted foundation](https://github.com/GlitchedZeus/PokeBank-NX/pull/103) | `d29eada5c087e201e00c749703f1a2de3ddfadeb` | Read-only evidence-aware baseline; not default main |
| [#298 — five-lane coordinator](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) | `5bf98b115ee9c3ff9189cccdfb7d9fd8040577c1` | L1 encounters [#293](https://github.com/GlitchedZeus/PokeBank-NX/pull/293), L2 breeding [#295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295), L3 moves [#296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296), L4 events [#292](https://github.com/GlitchedZeus/PokeBank-NX/pull/292), L5 independent QA [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294); coordinator exact-head checks have failures |
| [#303 — Claude legality follow-up](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) | `bc9d8c205b05a55bfbad3c913c0a52bedce0b71a` | Gen I original PKHeX differential jobs green; full Host Tests failing |
| [#301 — Gen V–IX domain groundwork](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) | `809a31841fc2ad43dda02d4412cb9891f29a482f` | Native-envelope/storage-domain research, **not complete Gen V–IX history legality** |

Original-source routes, decisions, authenticated distribution provenance and exhaustive candidate histories are often unavailable. **Unknown/Incomplete does not mean globally Invalid, and a green synthetic test does not certify PKHeX parity.** Gen I does not have native natures, abilities, gender or breeding mechanics.

## Product progression / honest remaining gates

| Area | Verified progress | Remaining |
|---|---|---|
| Gen I–III | Owner-accepted editor/save foundation | One combined Gen I–IV UI/touch/launch regression NRO |
| Gen IV | Owner-accepted prior foundation; advanced edit/items work in draft | #306 native/host CI repair; DS save discovery fairness; actual NRO artwork, Items/exit and touch smoke |
| Gen V | Extensive native/shared PK5 editor, Create/Review/export, six green CI gates | **Owner** must prove discovery in existing RetroArch/DraStic/melonDS directories; profile binding, fresh reopen, primary/backup, edits, source immutability |
| UI | HOME-style art, Party/Trainer previews, Game Sources, touch/focus foundation | Verify in-NRO assets and new controller/touch flows on Switch |
| Legality | Five independent lines plus original PKHeX differential scaffolding | Combined exact-head tests, cross-lane reconciliation, pinned-source evidence and trustworthy Unknown handling |
| Later gens | Identity/transport/read-only legality foundation | Native save support, generation-aware shared editor and device acceptance |
| Vault/Banks/Search and direct source writes | Design, cards/backlog and safety contract | **Not enabled**; independent persistent storage and transactional write-gate proof needed |

## Safety and branch governance

- No destructive A shortcuts, no generic installed-title or emulator source writeback, no unrestricted Inject Save, no unauthorized True Move or persistent Master Vault.
- App-owned, explicitly confirmed working-copy export is **not** permission to overwrite a source SAV.
- Never reset/rebase backward, force-push, discard someone else's commits or merge draft lanes just to make the roadmap look complete.
- Claude continues code on its lanes. This documentation checkpoint does **not** touch #303/#306 code or authorize merges.
- Owner preference: **one NRO per hardware round**, with exact source SHA/NRO hash, source/provider details and a concise verification checklist.

**Live sources:** [GitHub open PRs](https://github.com/GlitchedZeus/PokeBank-NX/pulls) · [Current Status](../CURRENT_STATUS.md) · [Game Support Matrix](GAME_SUPPORT_MATRIX.md) · [Build Record](BUILD_RECORD.md)
