# PokeBank NX — Post-v1 / v2 Platform Vision

Status: **PARKED FUTURE VISION — NOT v1 SCOPE**  
Created: 2026-09-03

This document preserves post-v1 ideas without allowing them to derail the current Switch v1.0 critical path.

The current product remains:

```text
PokeBank NX for Nintendo Switch homebrew
```

The v2 idea is to evolve the project into a broader **offline Pokémon preservation, storage, and transfer platform** across user-owned devices and emulator saves.

---

# v2 headline idea — PokeBank NX Link

Recreate the spirit of the old physical Pokémon link cable using the modern physical standard shared by Switch and Android handhelds:

```text
Nintendo Switch
PokeBank NX .nro
      │
    USB-C
      │
Android device
PokeBank NX .apk
```

Target Android devices could include:

```text
phones / tablets
AYN Thor / Odin-class handhelds
other Android emulation handhelds
```

The goal is **not** to emulate Nintendo's private HOME/network protocols.

The goal is an offline/local protocol between two PokeBank NX installations owned by the same user.

Working concept name:

```text
PokeBank NX Link
```

Product metaphor:

> The Pokémon link cable, rebuilt around USB-C.

---

# Core architecture principle

The cable should transfer a validated PokeBank NX object/package, not blindly write directly into a destination game's live save.

Preferred flow:

```text
source device
   ↓
select Pokémon / Vault entity
   ↓
serialize transfer package
   ↓
USB-C link
   ↓
receive + hash + parse
   ↓
validate
   ↓
import into destination Vault / staging area
   ↓
user chooses destination game/save
   ↓
compatibility + conversion
   ↓
staged write / approved adapter workflow
```

Therefore:

```text
USB RECEIVE != IMMEDIATE GAME SAVE WRITE
```

This keeps the transport layer independent from dangerous save-write behavior.

---

# Proposed transfer package

A future versioned transfer envelope could contain fields such as:

```text
protocol version
sender device/app version
Vault/entity ID where applicable
raw Pokémon payload / canonical representation
source game/platform
current format/generation
species/form/shiny/gender metadata
origin + provenance chain
conversion history
SHA-256
optional display metadata
```

The receiver must validate length/version/hash before trusting payload data.

Unknown/newer protocol versions must fail safely rather than guessing.

Do not transmit credentials, Nintendo account tokens, HOME authentication material, console keys, or unrelated save contents.

---

# Android application vision

Possible product name:

```text
PokeBank NX Android
PokeBank NX Mobile
```

The Android application should eventually be useful on its own rather than existing only as a USB bridge.

Potential feature set:

```text
Master Vault
named Banks
Living Dex / Shiny Living Dex
Summary / provenance
sprites / optional cries
search/filter/favorites
legality/compatibility presentation
manual .pk* import/export
emulator save sources
PokeBank NX Link
```

Potential emulator/source families:

```text
GB / GBC
GBA
Nintendo DS
Nintendo 3DS
supported emulator save exports
RetroArch save directories
manual save-file import
```

Modern Switch-emulator save support, if ever implemented, must be treated as a separate adapter/source problem and not assumed from Android platform support alone.

---

# Android storage reality

Android applications cannot assume unrestricted access to every emulator's private app directory.

Expected integration patterns:

```text
Storage Access Framework
user-selected folders
persisted folder permissions
shared emulator save folders
manual Import Save File
manual Export Save File
per-emulator adapters where practical
```

Never rely on rooting the Android device as the required normal workflow.

---

# Transport roadmap

## Stage 1 — USB-C

USB-C is the preferred first transport because it fits the link-cable product identity and can work without cloud infrastructure.

Conceptual handshake:

```text
HELLO
  ↓
protocol/app version
  ↓
capabilities
  ↓
device/user confirmation
  ↓
transfer manifest
  ↓
payload
  ↓
receiver hash/readback acknowledgement
```

Both sides should show exactly what is about to be sent/received.

A failed/disconnected transfer must leave the source intact and the destination transaction incomplete/recoverable.

## Stage 2 — local LAN / Wi-Fi

Later transport may reuse the same transfer protocol over local networking:

```text
PokeBank NX Link
  USB-C
  Local Wi-Fi / LAN
```

No cloud account should be required for basic local transfer.

Pairing may use a confirmation code or QR flow if useful.

---

# Relationship to Master Vault

v1 should **not** be redesigned around Android, but the Master Vault format should avoid unnecessary Switch-only assumptions.

Desired long-term separation:

```text
                  PokeBank Core concepts
                         │
          ┌──────────────┴──────────────┐
          │                             │
     Switch frontend              Android frontend
       native .nro                     .apk
          │                             │
          └──────────────┬──────────────┘
                         │
                 platform-neutral data
                         │
                 Master Vault format
                 provenance model
                 transfer envelope
                 conversion rules
                 capability model
```

Not every code line needs to be shared between C++ Switch and Android implementations. The important shared contracts are **formats and semantics**.

The v1 Vault should therefore prefer documented/versioned structures, explicit endianness/lengths, stable hashes and migrations rather than serializing arbitrary in-memory C++ objects.

---

# Copy / Move / Clone across devices

Existing PokeBank NX semantics remain authoritative:

```text
COPY
= source remains active and destination representation is created

MOVE
= source stops being active only after destination success is verified

CLONE
= intentional duplicate with clone provenance
```

For cross-device true Move, a safer future transaction may resemble:

```text
1. destination receives entity
2. destination validates + persists it
3. destination acknowledges durable success
4. source records acknowledgement
5. only then can source active-location retirement begin
6. interruption before final commit keeps/recoverably restores source ownership
```

Do not turn a network/USB acknowledgement alone into permission to delete a Pokémon from a game save.

Live save mutation remains independently gated by the relevant game adapter.

---

# Potential v2 milestones

A rough future sequence:

```text
v2.0
  Android application foundation
  platform-neutral Vault reader/writer
  manual Pokémon + emulator-save imports

v2.1
  PokeBank NX Link over USB-C
  Switch ↔ Android Vault transfer

v2.2
  local LAN / Wi-Fi transport
  richer emulator source discovery
  multi-device Vault utilities

v2.x later
  handheld-specific Android layouts
  optional Vault synchronization workflows
  desktop frontend if community demand justifies it
```

Version numbers are placeholders, not promises.

---

# Community-project implications

If v1 establishes reliable save adapters, provenance, Vault transactions and conversion semantics, the same contracts could support multiple frontends and community adapters.

Long-term project identity could become:

```text
PokeBank NX
Offline Pokémon preservation + transfer ecosystem

Switch .nro
Android .apk
possibly desktop later
        ↓
common Vault / provenance / transfer semantics
        ↓
real games, backups and emulator sources
```

This is intentionally broader than an offline HOME clone while remaining local/offline-first.

---

# Explicit non-goals / safety boundaries

Do not use v2 work to:

- impersonate official Pokémon HOME or Nintendo private protocols;
- extract/use Nintendo credentials or account secrets;
- forge HOME trackers/history;
- claim official HOME supports source files it does not support;
- bypass PokeBank NX per-adapter write verification;
- delete a source before durable destination verification;
- require cloud/accounts for the core local experience;
- silently crawl or modify unrelated Android files.

---

# v1 isolation rule

This entire document is **post-v1**.

Until PokeBank NX v1.0 is stable and physically validated:

```text
DO NOT implement Android
DO NOT implement USB Link
DO NOT implement LAN sync
DO NOT change Session 2.6 scope for this idea
```

The only near-term architectural influence allowed is modest future-proofing of the Vault/transfer file formats when those systems are actually implemented.

Current critical path remains the Switch v1 roadmap in `docs/V1_ROADMAP.md` and issue #29.
