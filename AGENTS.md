# PokeBank NX: instructions for Codex and other agents

Follow [`docs/ENGINEERING_PROTOCOL.md`](docs/ENGINEERING_PROTOCOL.md). It is shared with Claude; see `CLAUDE.md`.

Before any work, read [`docs/handoffs/MAIN_ACTIVE_HANDOFF.md`](docs/handoffs/MAIN_ACTIVE_HANDOFF.md) and resume from the pushed SHA it names, not from a chat summary.

- Push every working milestone, verify the remote SHA, and update the handoff.
- Never force-push or reset backward.
- Unfinished work goes on `wip/...` branches.
- Never weaken a test.
- Original saves are immutable.
- Keep #307 (application) and #314 (legality) separate.
- Never edit a branch another agent is working on at the same time.
