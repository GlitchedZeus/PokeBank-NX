# PokeBank NX — Next Session Recovery Launcher

Copy/paste the prompt below into the coding session.

```text
POKEBANK NX — SESSION 2.6 — RECOVERY + SAFETY/CRASH FINISH

Use HIGH reasoning.

Open:
GlitchedZeus/PokeBank-NX

Development branch:
feature/pokebank-playable

PKSE is upstream only.
NEVER push PokeBank NX changes to upstream.
Do NOT merge this work to main before the replacement artifact is physically tested and accepted.

FIRST: preserve and recover any interrupted local work before editing anything.

Run and inspect:

pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -40
git reflog -30
git stash list
git worktree list
git diff
git diff --cached

Do NOT run git reset --hard, git clean, discard changes, overwrite untracked files, or otherwise destroy local work until you understand exactly what is present.

The useful known Session 2.5 application checkpoint is:
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input

The exact old physically tested application source is:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Later feature-branch commits are mostly docs/research/tooling. Do not confuse branch HEAD with application-source identity.

If interrupted/uncommitted runtime source exists locally, preserve it first and determine whether it contains work newer than 361c6f55 before continuing.
If useful source is missing from the working tree, recover it from Git history/reflog/stashes/worktrees/branches when possible rather than recreating it from memory.

Then read, in this order:

PROJECT_STATUS.md
docs/FINAL_HANDOFF_2026-09-03.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md

Also read every file required by docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md.

Then execute:

docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md

EXACTLY.

Start working immediately after the recovery/read phase.
Do not ask me to repeat project history that is already in the repository.
Do not redo the external-project research unless an actual blocker cannot be solved from the recorded references.
Do not start Master Vault, PKSM-Core integration, DS/3DS adapters, Colosseum/XD, Stadium, true Move, full 3D/model work, event-system work, Android development, PokeBank NX Link/USB-C transfer, LAN sync, Historical Transfer Workspace, Prepare for HOME, HOME tracker research, or any other post-v1/#39/#40 work in this session.

Mission for this session:

1. Finish #23 installed-source mutation UI safety and persistence classification.
2. Harden #24 old/malformed/unsupported Legends: Arceus failure paths so they return safely instead of crashing.
3. Preserve #19 real Left Stick implementation from Session 2.5.
4. Preserve #13/#16 visible PokeBank NX shell/branding work.
5. Run #37 sprite/RomFS asset generation and device-asset preflight so View Pokemon artwork is actually packaged.
6. Run host tests, ASan/UBSan, git diff --check, and native Switch build.
7. If runtime/application source changes, create and push a NEW exact application-source commit. Do not label modified code as 361c6f55.
8. Clean-rebuild from that exact application commit.
9. Package/hash/preserve the replacement .nro with the repository artifact helper.
10. Populate the second-device checklist with exact application SHA, filename, byte size and SHA-256, leaving hardware PASS/FAIL fields blank for me.

Keep live installed-game writes HARD DISABLED.
Do not weaken the low-level safety lock.
Installed source must present as read-only.
Backup/staged mutation and legacy app Storage must be clearly distinguished from the installed title and from the future Master Vault.

Do not claim the old PLA hardware crash is DEVICE FIXED unless the exact failing save is physically retested. If the save is unavailable to the coding runtime, harden the path, add defensive tests, and leave #24 open for my physical retest.

Only an exact binary physically run by me can become DEVICE TESTED.

The session's required end state is:

READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED

At the end, give me the actual replacement .nro (and ZIP fallback if useful) plus:

Application source full SHA
Application source commit message
Embedded short SHA/version
Artifact filename
Artifact byte size
Artifact SHA-256
Host-test result
Sanitizer result
git diff --check result
Native-build result
Device-asset preflight result
GitHub branch/CI result
Device tested: NO

Start now.
```
