# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-10

## FRLG physically accepted; RSE unblocked

FireRed GBA and LeafGreen GBA read-only browsing/source assignment are now **DEVICE TESTED = YES** and **DEVICE ACCEPTED = YES** on a real Nintendo Switch.

Accepted application source:

```text
d78b76503f02ae26309855970fc5ce0b35c12bcb
legacy: make fsdev diagnostics link-compatible
```

This source contains the earlier LeafGreen persistence fix from:

```text
92bde34d1586990aaa82adc4f60d42d7bc6b5bdf
legacy: safely replace profile binding database on Switch
```

The native `fsdevGetLastResult()` compatibility blocker is fixed. The runner used libnx 4.12.0-1; the header declared the function while `libnx.a` exported the unmangled C symbol and the old C++ object requested a mangled symbol. The narrow fix uses C linkage and treats the API as optional diagnostics only, falling back to `native_result=unavailable` when unavailable. Persistence behavior, backup/recovery/rollback, source bytes and write policy were not weakened.

The exact FRLG persistence retest artifact was built with a full native devkitA64 link, restored from the committed RomFS snapshot, and verified with 3,283/3,283 embedded RomFS files. The accepted NRO was:

```text
PokeBank-NX-FRLG-Persistence-Retest-d78b7650.nro
size: 156,707,753 bytes
SHA-256: 60c727d05ea79dda2f284b4594222ec334324f04eba0db87d9cb4f60f8959dc2
embedded application source: d78b76503f02ae26309855970fc5ce0b35c12bcb
```

### Physical Switch acceptance results

The user physically verified all of the following on the current FRLG persistence build:

1. FireRed opens correctly.
2. LeafGreen opens correctly.
3. FireRed and LeafGreen can both be assigned.
4. Fully exiting PokeBank NX and reopening preserves assignments.
5. Switching Nintendo/PokeBank profiles preserves correct profile isolation.
6. Switching back restores the correct original profile assignments.
7. Pokémon were changed in RetroArch.
8. The games were saved normally using the in-game battery save.
9. PokeBank NX Refresh reread the changed save correctly.
10. Updated Pokémon data appeared correctly.
11. The original FireRed/LeafGreen saves still load and remain healthy in RetroArch.
12. Additional browsing/trainer/party/box testing also passed.

Therefore:

```text
FireRed GBA: DEVICE TESTED = YES
FireRed GBA: DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES
LeafGreen GBA: DEVICE ACCEPTED = YES
FRLG MILESTONE: PHYSICALLY ACCEPTED
```

This acceptance covers the implemented **READ-ONLY** browsing/source-assignment functionality only. Live installed-game and RetroArch save writing remains **HARD DISABLED**. Do not claim live writing as tested or accepted.

## Recovery snapshot remains complete

Recovery is already complete and must not be redone during ordinary development.

Authoritative private GitHub RomFS recovery snapshot:

```text
a2adac94f15504b90a83a295e77ad54154da4206
```

Verified snapshot/build context:

```text
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FRLG artwork: PASS
total RomFS files: 3283/3283
```

Normal recovery remains defined by `docs/RECOVERY_CONTRACT.md`, `recovery/RECOVERY_STATE.json`, and `tools/recover_workspace.py`. Do not regenerate sprites or perform broad reflog/worktree archaeology unless the committed deterministic recovery path genuinely fails.

## Next authorized development target — Ruby / Sapphire / Emerald

RSE is now unblocked. The next milestone is **Pokémon Ruby, Pokémon Sapphire and Pokémon Emerald GBA / RetroArch normal battery saves, read-only only**.

Use the existing Gen III architecture and strict validation concepts already proven for FRLG:

```text
raw GBA battery save
        ↓
strict PokeBank validation
        ↓
valid rotating save-slot selection
        ↓
Gen III adapter / PKSM-Core where appropriate
        ↓
PokeBank-owned read model
        ↓
UI
```

PokeBank must validate rotating save sections before exposing Pokémon. Never select a newer corrupt slot merely because its save index is higher. Reject malformed/truncated/missing-sector/checksum/signature failures safely. Source saves must remain byte-for-byte untouched.

Parked RSE checkpoints to inspect **once, narrowly** before implementation:

```text
b5ef83b
1a921515 if available, or verified equivalent
```

If useful RSE work exists there, reapply/integrate only that work on top of the current branch. Never reset the branch backwards and never discard newer FRLG, recovery, UI, asset or safety work. If the parked work is genuinely unavailable, continue from current Gen III architecture rather than spending the session on broad archaeology.

Required RSE coverage includes Ruby/Sapphire/Emerald identification, trainer data, party, PC boxes including a cross-sector-boundary Pokémon fixture, rotating-slot selection, corrupt-newer-slot fallback to older valid slot, malformed/truncated rejection, Refresh reread behavior, source-mutation/write-policy checks, relevant FRLG regressions, ASan/UBSan and a full native devkitA64 link.

Once coherent RSE host verification passes, commit/push/remote-verify an application-source checkpoint early. Then restore the existing committed RomFS snapshot, build/package a new RSE physical-test NRO, verify embedded RomFS and STOP for device testing.

## Strict scope / safety

Do not begin Gen I, Gen II, DS, 3DS, modern Switch expansion, Vault/Banks, editor/Create Pokémon, events/gifts, transfers, RetroArch per-Switch-user modifications or any other roadmap feature during the RSE milestone.

No live save writing. No browsing action may modify source save bytes.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX code upstream.
