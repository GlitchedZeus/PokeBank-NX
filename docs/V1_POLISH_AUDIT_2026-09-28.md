# PokeBank NX v1 Polish Audit — 2026-09-28

Base checkpoint: `00ee7a6ed7ac1b5a93c43246d70c252e135acec0` (PR #79)
Work branch: `polish/v1-ui-qol-20260928`

This tranche is presentation/diagnostics/QoL only. It does not change save formats, Gen IV serialization,
source write policy, Master Vault, Gen V, or cross-game True Move.

## Issue #70 reconciliation

| Item | Audit result | Action |
| --- | --- | --- |
| Neutral visual treatment | Mostly already implemented through shared theme-aware surfaces | Preserve; do not redesign accepted screens |
| Shared bottom control legend | Present, but face-button differentiation and stick discoverability could be clearer | Improve shared `ScreenChrome` glyphs only |
| Remove `Local Save` artwork overlay | Still effectively present as the game-card source badge | Remove source/provider badge from cover-art cards |
| Consistent game-card save/source metadata | Still inconsistent because legacy cards can show counts while Gen IV can show remembered/path status | Remove per-card source count/path metadata; Save Instances owns it |
| Thin card accent strip | Still present above cover art | Remove |
| Sorting | Larger feature | Defer |
| Favorites | Larger feature | Defer |
| Source chooser | Superseded by device-accepted provider-neutral Save Instances | Already solved; do not build a second picker |
| Box reorder | Larger mutation UX | Defer |
| Redundant Settings footer controls | Still present | Remove; global nav bar stays authoritative |

## Issue #21 reconciliation

Implemented now:
- Settings exposes PokeBank NX version and abbreviated application Git SHA without network access.
- Settings exposes the external-write lock and cross-game True Move lock.
- Native polish CI packages an exact-head NRO with build identity and SHA-256.

Deferred:
- full Diagnostics screen;
- privacy-safe diagnostic export;
- Applet Mode/memory warning;
- storage-health tooling;
- performance/cache work;
- release-candidate accessibility features.

## Issue #26 reconciliation

Already implemented in the accepted base:
- left-stick navigation is translated through the shared navigation-repeat path;
- held-stick repeat is preserved;
- Storage L/R changes nearby boxes;
- Storage ZL/ZR performs larger box jumps;
- installed/read-only mutation shortcuts remain blocked.

This tranche does not remap destructive controls. It only makes the shared navigation legend communicate
D-pad + Left Stick parity more clearly.

## Issue #16 reconciliation

Already implemented:
- PokeBank NX application/window title;
- PokeBank NX shared title bar;
- version and source SHA compile-time plumbing.

Implemented now:
- user-visible NRO author metadata no longer carries stale PKSE-era branding;
- Settings visibly exposes version/build identity.

Deferred:
- final icon/NACP artwork polish;
- final staged startup/loading flow;
- endgame splash polish.

## Safety invariants

```text
ORIGINAL EXTERNAL SOURCE: IMMUTABLE
LIVE INSTALLED-GAME WRITES: DISABLED
RETROARCH/mGBA/TICO/DRASTIC/MELONDS WRITES: DISABLED
UNKNOWN OR AMBIGUOUS SOURCE: FAIL CLOSED
REMEMBERED SOURCE SUBSTITUTION: FORBIDDEN
A BUTTON: NON-DESTRUCTIVE AT SOURCE BOUNDARIES
CROSS-GAME TRUE MOVE: LOCKED
GEN V: NOT STARTED
MASTER VAULT: NOT STARTED
```
