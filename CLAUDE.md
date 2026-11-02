# PokeBank NX — instructions for Claude

**READ FIRST:** [Binding project rules](docs/PROJECT_RULES.md), then [engineering protocol](docs/ENGINEERING_PROTOCOL.md), then [active handoff](docs/handoffs/MAIN_ACTIVE_HANDOFF.md). The project rules override older instructions where they conflict.

- No new per-task/round branches except under the binding rules. Stop treating a branch or PR per bug as progress.
- Work toward a single shared Gen I–IX application, with separate legality. Never report modern generations or Vault as complete without evidence.
- P0 save/profile failures take priority over new features; owner is not the default regression tester.
- Reproduce bugs before and after, run tests on exact head, verify push and record real CI. Don't call code hardware accepted without device evidence.
- Original saves stay immutable. No force-push, silent file replacements or test weakening. Respect one-writer-per-branch.
