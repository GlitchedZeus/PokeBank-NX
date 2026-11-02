# PokeBank NX — binding project rules
**Effective: 2026-11-02. Applies to Claude, Codex, assistants, contributors and CI.**
These rules override older handoffs, the branch names in docs/ENGINEERING_PROTOCOL.md, and agent prompts where they conflict. No agent can waive them by claiming a task is urgent. Original-save safety rules are never weakened.

## 1. Stop branch sprawl, archive before pruning
- Target exactly **three long-lived branches**: `main` (reviewed release integration), `development/app-gen1-9` (one application integrating ALL generations), and `development/legality` (independent research and engine). These targets are NOT claims that consolidation or merges already happened.
- Do NOT create new branches for every task, bug, agent, test, NRO or QA round. Reuse the assigned app/legality branch when non-overlapping; temporary feature branches require a named owner, a PR, a close-out date and specific reason. Maximum **five** concurrent temporary task branches after cleanup, subject to protected active work.
- No agent deletes active PR heads/bases, frozen candidate refs, independent evidence, or unique commits. Before deleting any other remote branch, create and verify a FULL backup bundle outside GitHub's working tree; preserve unique tips with verified archive tags; confirm exact live tip and current PR ownership; use lease-protected deletes; log every deletion and recovery SHA. Never force-push.
- During the cleanup, *no new branches* except the designated cleanup branch, source-controlled migration target(s), and emergency safety fixes expressly approved by owner. No new NRO testing campaign.
- Legacy integration and agent branches may only be removed after audit and safe migration. Never equate fewer branches with completed application integration.

## 2. One application, shared features for Generations I–IX
- All generations use ONE product navigation model, Save Instances model, game-card gate, profile/assignment service, shared editor controls, modals, Party/Boxes/Items/Trainer, backup/export flow and Review model. Format adapters handle only real generation/game differences.
- Every feature added to any generation must have a parity matrix across all supported generations. No copy-pasted alternate UI or silent feature omissions. Incomplete Gen VI–IX functionality must be disabled and labelled, not portrayed as ready.
- Support actual emulator save roots and launch paths by documented provider discovery, not guessed locations; support RetroArch, mGBA, Tico, DraStic, melonDS and other investigated providers. Exact game and profile validation come before opening any workspace.

## 3. P0 save/profile guarantees before further features
- **No verified matching save = NO workspace.** A, +, dock Items, Quick Games, touch and any direct entry must go to an exact-game save chooser or a clear no-save message; no empty fake editor.
- Assign, change and UNASSIGN must work consistently in Gen I–IX; removing an app assignment must NEVER delete or alter source saves. Switching profiles must not expose or inherit another profile's assignment without explicit permitted linking. No side-effect unassignment when entering Items or navigating.
- A stale, missing, uninitialized, mismatched, cross-profile or invalid save fails closed. No sibling-game substitution or "best guess" save selection.
- Original saves are **byte-for-byte immutable**, writes are staged, exports create independent verified copies, and destructive operations require explicit confirmation. Stop editor features until the foundational save/profile P0s are fixed.

## 4. No repeat QA theatre
- Every owner FAIL becomes a reproducible test (when technically possible) that MUST fail before a fix and pass after it. Test the real call path through the UI, exact game, profile, save assignment, emulator provider and entry point; static string assertions alone are insufficient.
- A fix affecting shared code requires adjacent-generation regression coverage and negative cases. Existing owner PASS evidence is preserved and must be rechecked if affected.
- Do not send the owner another NRO with known P0 save/profile regressions. Green host tests are **not** Switch verification. Only owner-tested exact NRO+SHA may be called Hardware Accepted.
- Classify findings precisely: Reproduced FAIL, Blocked/Not Tested, Fixed in Code Pending Hardware, Hardware Accepted. Never transform an untestable case into proof of failure or success. Retain full raw owner evidence.

## 5. Truth, references and release scope
- Keep the legality engine separate and OUT of the NRO until evidence-backed quality is accepted. Unsupported evidence yields Incomplete, not clean/valid or automatically Invalid.
- Research all relevant credited sources, not only PKHeX/PKSE: native/disassembly sources, PKSM, PKForge, OpenHome/PokeHOME where applicable, Pokémon Bank/HOME documented behavior, independent save tooling and first-party documentation. Record provenance, version, confidence and licenses; do not redistribute prohibited assets.
- Do not call work implemented, integrated, built, pushed, device-tested or fixed without objective evidence and exact head/artefact identity. No invented checks or tests.
- New work first reads this document, `AGENTS.md`/`CLAUDE.md`, the active handoff and open PR heads. One writer per live branch; each agent reports touched files, commit SHA, CI, open blockers, and owner actions that are genuinely necessary.
- Feature expansion is paused while P0 save/profile problems remain. The owner's Nov 2 quick QA (`d160aa42`) is failed/blocked evidence, not release acceptance.

## 6. Enforcement and exceptions
- This file is the primary policy for scope, architecture, branches and acceptance. `docs/ENGINEERING_PROTOCOL.md` continues to govern technical build and source-safety requirements when not superseded here.
- Automated CI contracts should enforce these rules where possible. An AI summary is never a substitute for remote GitHub verification. Exceptions require an explicit owner decision documented in a PR or issue, with rationale and rollback.
