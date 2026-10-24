# PokeBank NX engineering protocol (Claude and Codex)

This one protocol applies to every agent (Claude, Codex) and every human contributor. `CLAUDE.md` and
`AGENTS.md` at the repository root point here. If it conflicts with an older handoff, this file wins.

## GitHub is the source of truth

- **Resume from pushed code.** Start from the branch and SHA named in
  [`docs/handoffs/MAIN_ACTIVE_HANDOFF.md`](handoffs/MAIN_ACTIVE_HANDOFF.md). Do not start from a chat summary
  or a local workspace.
- **Fetch first.** Before working, run `git fetch origin` and compare your branch with its remote. If the
  remote has advanced, reconcile forward with a merge. If you cannot do that safely, push your work to a new
  `wip/...` branch and record the conflict in the handoff.
- **A push is not done until GitHub confirms it.** Check `git ls-remote origin <branch>` and record the full
  SHA. Never say work is "pushed" or "uploaded" without that check.

## Push early and often

- **Push after every working milestone.** Do not wait for the end of a campaign.
- **Checkpoint at least every 15–20 minutes** of active engineering, and before any long build or test run.
- **If usage is running low,** stop starting new work. Push what you have, update the handoff, and stop.

## Never lose work

- **Never rewrite history.** No force-push, no backward reset or rebase, and never overwrite newer commits.
- **Never discard untracked code.** If it is valuable, commit it, on a WIP branch if necessary.
- **Never commit sensitive files:** original Pokémon saves, ROMs, keys, personal data or credentials.

## Keep integration and WIP separate

- **#307** (`claude/gen5-integration-20261014`) is the application integration branch. Only finished work
  goes there: tested, and passing the gates below.
- **Unfinished or failing work** goes on a clearly named WIP branch: `wip/<lane>-<topic>-<yyyymmdd>`. Say in
  the handoff what is unfinished.
- **#314** (legality engine) is a separate lane. Do not merge #307 and #314 into each other, or into `main`,
  without the owner's explicit authorization.
- **Gen VI–IX lanes are separate.** Do not mix them into #307.
- **One writer per branch.** Claude and Codex must never modify the same branch at the same time. Whoever
  takes a branch records that in the handoff.
- **Do not overwrite another agent's changes.** Integrate them by SHA, with merge or cherry-pick.

## Product safety rules (never weaken)

- **Original saves are immutable.** This applies to emulator and installed-game saves alike.
  - The app never automatically overwrites an emulator's live save.
  - Export writes a separate copy only.
- **No automatic legality edits.** Never modify a Pokémon automatically because of a legality finding.
- **Missing evidence is not a verdict.** Missing legality evidence means Unknown / Incomplete, never Invalid.
- **Confirm destructive actions with Y.** The owner-directed Round 4 exception is Bag Remove: Y opens its explicit question, A confirms there, and B cancels. Pokémon Release and Backup Delete remain Y-only.
- **Every format rule must be source-backed.** That covers format rules, encryption schemes, checksums and
  structures; sources are pret disassemblies and PKHeX. Never invent one.
- **Never weaken a test to get CI green.** Do not hide, delete, skip or weaken failing assertions.

## Status words (use them exactly)

| Word | Means |
| --- | --- |
| Implemented | Code is written. |
| Tested | Host tests were actually run and passed. Name the command and the result. |
| Pushed | On GitHub, with the SHA verified by `git ls-remote`. |
| Integrated | On #307, with CI green on that head. |
| Hardware verified | The **owner** tested that exact NRO on a Switch and recorded PASS. Only the owner sets this. |

- Never call anything hardware verified without the owner's recorded result.
- Never call a bug fixed just because it was assigned to someone.
- Never call an emulator check passed without real emulator evidence.

## Gates before pushing to #307

Run these from clean build directories:

```sh
make -f Makefile.host -j"$(nproc)" host-test                       # GCC
make -f Makefile.host -j"$(nproc)" host-sanitize
make -f Makefile.host -j"$(nproc)" CXX=clang++ host-test
make -f Makefile.host -j"$(nproc)" CXX=clang++ host-sanitize
make -f Makefile.host -j"$(nproc)" qa-contracts qa-matrix
python3 tests/test_qa_cross_gen_contract.py --candidate --list     # must report 0 contract(s) failing
python3 tests/test_hw_round1_regression_gate.py                    # protects owner-accepted passes
python3 tests/test_ci_repository_safety_contract.py
git diff --check
make -j2                                                           # devkitA64 NRO build, when the toolchain is available
```

## Hardware candidate rules

- **One NRO per hardware round.** It is built by `.github/workflows/gen15-integrated-candidate.yml` on the
  exact #307 head.
- **Bind the build in a docs-only commit.** The binding goes in `docs/qa/CANDIDATE_UNDER_AUDIT.env`.
- **Audit the exact file.** The QA-F9 audit (`.github/workflows/app-qa-nro-audit.yml`) must report
  `NRO AUDIT: PASS` on the binding commit.
- **Not accepted until the owner tests it.** Until then the NRO is "NOT DEVICE TESTED".

## Handoff

- **Keep the handoff current.** Update [`docs/handoffs/MAIN_ACTIVE_HANDOFF.md`](handoffs/MAIN_ACTIVE_HANDOFF.md)
  at every significant checkpoint, and push it.
- **Leave enough to resume without the owner.** The next agent must be able to continue without asking the
  owner to reconstruct the session.
