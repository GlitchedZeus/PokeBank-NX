# PokeBank NX Project Status

Last updated: **2026-09-28**

## Headline

PokeBank NX now has device-accepted shared staged Pokémon editing for Generations I–III, device-accepted Gen I–IV Save Instances, and a device-accepted first Generation IV Party/Box View/Edit milestone.

The project has moved into **G4-04: completing the real Generation IV shared editor**, including Box Create and the remaining native field controls.

## Current active development

### PR #92 — G4-04

Branch:
**feature/gen4-full-editor-20260928**

Current head at this update:
**c46ed887ae6ce44e662d8cbfb32e68970846a2f0**

Tracking:
**Issue #95**

Current focus:

- Create Pokémon in empty Gen IV Box slots;
- Species editing;
- Held Item / Language / Ball / Pokérus;
- Met Location;
- native Gen IV move selection;
- inspectable read-only information;
- exact Form support;
- move/PP consistency;
- final CI + physical acceptance.

## Stable integration baseline

PR #90 remains the current integrated base:

**8b3bcc16c804247bfe8d1314b686974ce73051d8**

It contains:

- accepted G4-03 Gen IV Party/Box View/Edit;
- independent audit fixes;
- complete v1 polish/QoL tranche;
- final shared-control cleanup;
- picker/inventory/backup-screen polish;
- final Gen I footer/control cleanup.

Its exact-head workflows are green.

## Hardware-accepted Gen IV checkpoint

Exact accepted application:

**84dae170deb2756d9b80aec32bf8ad512ce17c31**

NRO SHA-256:

**313b6ed5f209b0fba797deee010d73b753d25294dd3d1c39f279c61df13fe6de**

Accepted on real Switch hardware for:

- Gen IV Party + Box shared View/Edit;
- staged Party edits;
- shared View/Edit presentation;
- current dirty-session behavior;
- source-immutable ordinary editing.

This does not mean the Gen IV editor is feature complete. G4-04 is the completion tranche.

## Completed parallel lanes

PR #87:
closed without merge after G4-03 was carried forward.

PR #88:
closed without merge after its final QoL head
**c8c98d5dd2816fe6bf1b9557c24b3fb09a5dc7c3**
was fully integrated into #90.

## Product foundation already working

- Gen I–III staged Pokémon Create/View/Edit
- classic staged Inventory
- Gen IV strict read foundation
- Gen IV Party/Box staged View/Edit
- multi-provider Save Instances
- RetroArch / mGBA / Tico legacy sources
- RetroArch / DraStic / melonDS Gen IV sources
- profile/source identity and deduplication
- staged dirty-session protection
- recovery/durability foundations
- controller/Left Stick parity
- shared themes and control chrome
- Settings build identity and diagnostics
- graceful missing-art fallback

## Product work after G4-04

Near-term product work is increasingly about turning the engineering foundation into the finished application experience:

- automatic immutable source backups + explicit Inject Save (#89);
- Master Vault + named Banks (#3);
- clearer writable Storage vs Vault/Bank semantics (#27);
- search/filter/favorites/recent views;
- box organization;
- Living Dex / shiny collection experiences;
- stronger backup/recovery UI;
- broader SaveSource support;
- DS / 3DS;
- modern Switch save validation (#11).

## Nintendo Switch game saves

Modern Switch support is not blocked by lack of references.

The repository already tracks a read-first audit against pkHouse, PKHeX and PKSE behavior.

The reason it remains a separate core-engineering lane is that each family still needs:

- correct title/save discovery;
- correct user/profile save mounting;
- exact save/container validation;
- game/revision handling;
- box/party format validation;
- malformed/truncated rejection;
- staged mutation rules;
- backup/recovery policy before any write is authorized.

The first product step is read-only validation, not a global write switch.

## Save safety direction

Current:

~~~text
source
  ↓
validated read
  ↓
app-owned staged working data
~~~

Future Issue #89:

~~~text
source
  ↓
automatic immutable backup
  ↓
working copy
  ↓
edit + strict validation
  ↓
explicit Inject Save
~~~

## Near-term sequence

~~~text
Gen I–III editor                         DEVICE ACCEPTED
        ↓
Save Instances / source architecture      DEVICE ACCEPTED
        ↓
Gen IV Party + Box View/Edit              DEVICE ACCEPTED
        ↓
Gen IV full Create/Edit parity            ACTIVE / PR #92
        ↓
backup + explicit Inject Save
        ↓
Master Vault / named Banks
        ↓
broader sources / DS / 3DS / Switch
        ↓
v1.0 product hardening
~~~

## Permanent product rules

~~~text
NORMAL EDITING: APP-OWNED WORKING DATA
SOURCE INJECTION: DISABLED UNTIL EXPLICITLY APPROVED
UNKNOWN / AMBIGUOUS SOURCE: FAIL CLOSED
REMEMBERED SOURCE SUBSTITUTION: FORBIDDEN
DIRTY WORK: NEVER SILENTLY DISCARDED
CROSS-GAME TRUE MOVE: LOCKED
DEVICE ACCEPTED: EXACT PHYSICAL NRO ONLY
~~~
