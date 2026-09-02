# PokeBank NX — NRO Quality / Reliability Roadmap

Status: PRODUCT QUALITY BACKLOG  
Last updated: 2026-09-02

This document collects cross-cutting improvements that make PokeBank NX feel like a trustworthy native Nintendo Switch application rather than only a collection of Pokémon/save features.

Tracking issue: #21 — NRO reliability, diagnostics, performance and quality-of-life backlog.

This roadmap is **supporting work**. It must not derail the current second-device-build milestone or the next PKSM-Core / Master Vault core-engine work. Implement items when their dependent subsystem exists and when they provide meaningful leverage.

---

## 1. Diagnostics screen

Add a read-only screen under a predictable path such as:

```text
+ -> Options -> Diagnostics
```

Suggested information where available:

```text
PokeBank NX version
application source SHA / abbreviated build SHA
build date/version metadata
Switch firmware
Atmosphere/CFW version if safely discoverable
launch/memory mode where practical
free application memory where practical
free SD space
current theme
detected supported games
Vault entity count
Bank count
sprite/artwork cache usage
last successful backup / recovery state
live installed-save write policy
```

The live-write state should be impossible to misread:

```text
Installed-save writes: HARD DISABLED
```

Later, when per-adapter writes exist, show capability per source/destination rather than one misleading global green state.

### Diagnostic export

Allow a privacy-safe text/JSON diagnostic report to be written to a user-controlled PokeBank NX logs/reports directory.

Do not include raw Pokémon payloads, full personal saves, console authentication material, credentials, or unrelated personally identifying information by default.

---

## 2. Applet Mode / constrained-memory detection

Where libnx exposes reliable information, detect restricted/Applet Mode or otherwise constrained memory conditions.

If the user launches in a mode that may be unsuitable for a large Vault/artwork cache, show a useful warning such as:

```text
PokeBank NX is running with limited memory.
Large Vaults and artwork caches may work better in full-memory Application Mode.
```

Do not prevent startup unnecessarily. Prefer graceful degradation:

- reduce cache sizes;
- avoid eager artwork loading;
- continue read-only recovery/help access;
- explain the limitation instead of crashing.

---

## 3. Real startup stages

Issue #16 owns the final startup/branding work.

Startup progress must correspond to actual completed initialization steps rather than a cosmetic fake timer.

Potential stages:

```text
Settings
Storage/services
Game registry
Pokemon data
Sprite/artwork index
Master Vault (when implemented)
Banks (when implemented)
Source/save scan
Recovery/journal check
Ready
```

Skip stages that do not exist yet rather than pretending they completed.

Optional-resource failures should be reported but should not block recovery/help when the core app can still function.

---

## 4. Clear source safety / active-location badges

Users should immediately understand whether they are looking at a live game, the Vault, a Bank, a staged result, or archival history.

Examples:

```text
Pokemon Violet
Installed Save
READ ONLY
```

```text
MASTER VAULT
LOCAL STORAGE
SAFE TO ORGANIZE / DERIVE
```

```text
STAGED SAVE
NOT WRITTEN TO GAME
```

Future true-Move support (#20) also needs an explicit **active location** concept so archival/provenance history does not look like a second playable copy.

---

## 5. Privacy-safe error / crash logs

Use a predictable application-owned location such as:

```text
/switch/PokeBank-NX/logs/
```

or the final canonical app data path selected by the project.

Useful fields:

```text
app version
application source SHA
screen/context
selected stable game identity
operation name
last non-sensitive UI action
theme
memory/cache state where practical
error/result code
```

Do not automatically dump entire saves, raw Pokémon collections, console keys, credentials, or sensitive system information.

When a full save or entity is genuinely required for debugging, make that a separate deliberate user action.

---

## 6. Vault transaction recovery UI

Once Vault transactions exist, startup should inspect pending journals.

If recovery is necessary, present a clear flow such as:

```text
An interrupted Vault operation was detected.

Original data: intact
Published entity: verified / incomplete
Recovery journal: available

A Recover
X Details
B Leave untouched
```

The underlying recovery engine must follow `MASTER_VAULT_SPEC.md`; the UI should never guess that incomplete data is valid.

---

## 7. Storage health tools

Once the Vault exists, add a maintenance page capable of:

```text
Verify Vault hashes
Verify metadata against raw payload hashes
Rebuild search/species/source indexes
Inspect transaction journal state
Find orphan references
Find missing entity files
Report schema versions
Export a manifest
```

Repair should be conservative. Raw payload + hash + provenance are authoritative; derived indexes should be rebuildable.

---

## 8. Search / filter

For large collections, search is a core feature rather than optional polish.

Potential filters:

```text
Species
Nickname
Generation
Origin game
Current/active location
Shiny
Level
Nature
Ability
Ball
OT
Legal / Invalid / Unknown
Ribbon
Mark
Event
Favorite
Bank
Format
```

Use indexes/metadata rather than loading every raw entity into RAM.

---

## 9. Box Quick Jump

Provide a compact box selector rather than requiring repeated L/R presses across large save/bank layouts.

Example:

```text
+ -> Jump to Box

01 02 03 04 05
06 07 08 09 10
...
```

Retain L/R and ZL/ZR controller shortcuts.

---

## 10. Continue / Recent views

A polished Home screen can include:

```text
Continue
Recently Viewed
Recently Added
Recent Transfers
```

These should be lightweight index/history records, not duplicate Pokémon storage.

---

## 11. Favorites

Allow a deliberate favorite state / Favorites Bank-style view.

A favorite should be metadata/reference state, not another raw duplicate.

If a Favorites Bank is derived automatically, deleting the view must not delete the underlying Vault entity.

---

## 12. Restrained motion and transitions

Small transitions can help focus without making a save tool sluggish.

Potential examples:

```text
focused card -> slight elevation/scale
Action Sheet -> short fade/slide
box change -> subtle horizontal transition
```

Add:

```text
Reduced Motion: On/Off
```

Core navigation must remain responsive with motion reduced or disabled.

---

## 13. Optional original UI sounds and restrained rumble

If added, use original/non-infringing UI sounds rather than copied Pokémon HOME/game assets.

Potential events:

```text
navigate
select
cancel
success
warning
```

Rumble should be reserved for meaningful confirmation/error states, not every grid movement.

Settings:

```text
UI Sounds: On/Off
Rumble: On/Off
```

---

## 14. Text size / readability

Support at least:

```text
Small
Normal
Large
```

when the shared UI primitives can accommodate it cleanly.

Do this before dozens of fixed-coordinate screens make accessibility expensive to retrofit.

---

## 15. Color-independent focus

Selection/focus must not rely only on hue.

Use combinations such as:

```text
focus border
slight elevation
cursor/indicator
surface change
```

Validate across OLED Black, Dark, and Light themes.

---

## 16. Virtualized large grids

Vault and Pokédex views must scale to very large collections.

Do not instantiate/render/load every raw entity and sprite at once.

Prefer:

```text
visible rows/cards
+ a bounded prefetch buffer
```

Test with large synthetic metadata collections before users accumulate thousands of entities.

---

## 17. Bounded artwork/sprite caches

Use separate bounded caches where useful:

```text
small Pokemon icons
large Pokemon renders
cover artwork
```

Prefer lazy loading and LRU-like eviction or another explicitly bounded policy.

Cache limits should adapt to memory constraints where practical.

---

## 18. Intentional missing-resource fallback

Missing artwork/sprites should never create a blank, broken, or unusable screen.

Provide branded fallback states such as:

```text
?
No Artwork
#1007
```

The Pokémon/save data remains usable even when optional visual assets are missing.

---

## 19. Authoritative-data rule

Never make the browsing/search metadata database the only authoritative Pokémon store.

Authoritative:

```text
raw entity bytes
SHA-256
provenance / lineage
transaction state
```

Rebuildable:

```text
species/name/type cache
thumbnail references
search tags
sort keys
Dex indexes
recent lists
```

If an index/database is corrupt, recovery should reconstruct it from authoritative storage where possible.

---

## 20. Scary-operation UX

When live writes eventually exist for individually approved adapters, expose the safety pipeline rather than hiding it.

Conceptual flow:

```text
Backup source/destination
Stage operation
Validate
Write
Read back
Verify
Commit
```

If any verification fails:

```text
ABORT / ROLLBACK
```

Do not continue because a partial result "looks probably fine".

---

## 21. Home-screen direction

A later Home screen can combine the product's major destinations without becoming cluttered:

```text
PokeBank NX                         version / theme

CONTINUE
[Pokemon Violet]  Box 18  Installed Save / READ ONLY

YOUR COLLECTION
[Master Vault] [Living Dex] [Shiny Dex]

GAMES
[Sword] [Violet] [PLA] [FireRed GBA] [Emerald] ...

A Open   X Search   Y Sort   + Options   - Help
```

Do not block the core engine roadmap on this final Home layout.

---

## Recommended implementation balance

After the second UI/device milestone is stable, aim roughly for:

```text
70% core functionality
20% hardware validation / bug fixing
10% polish / infrastructure
```

This prevents visual polish from outrunning the Master Vault, parser, validation, and transfer engines.

---

## Priority order

### Early high-leverage

- Diagnostics
- Applet/constrained-memory warning
- privacy-safe error logs
- clear READ ONLY / Vault / staged status indicators
- intentional missing-art fallback
- final visible product/startup identity

### During Master Vault / Banks

- transaction recovery UI
- storage-health verification
- search/filter
- Favorites
- recent items
- virtualization
- bounded caches
- active-location model

### Release-candidate polish

- text-size options
- Reduced Motion
- optional sounds/rumble
- performance soak tests
- accessibility pass
- diagnostic export polish

---

## Verification rule

Device-specific behavior should be tested on physical Switch hardware. Host-testable pure logic should have host regressions. Performance claims should be measured with large synthetic datasets rather than assumed.
