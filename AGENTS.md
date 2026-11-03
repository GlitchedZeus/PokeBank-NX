# PokeBank NX — instructions for Codex and every non-Claude agent

**READ FIRST:** [Binding project rules](docs/PROJECT_RULES.md), then [engineering protocol](docs/ENGINEERING_PROTOCOL.md), then [active handoff](docs/handoffs/MAIN_ACTIVE_HANDOFF.md). The project rules override older instructions where they conflict.

- Do not create a new branch unless the binding branch policy explicitly allows it; no new QA-round branches by default.
- The application target is ONE Gen I–IX branch; legality remains independent. Never merge unaccepted code into `main`.
- Save/profile P0 bugs block more feature development or another NRO handoff.
- Record reproduction, regression test, exact code SHA, CI evidence and outstanding hardware acceptance. No fake "fixed" status.
- Original saves are immutable; never weaken tests, guess source ownership or force-push.
- Coordinate branch ownership; no two agents writing the same branch simultaneously.
