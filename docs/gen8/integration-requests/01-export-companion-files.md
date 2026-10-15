# Request 01 (to the owner of `Integration/Common`): carry companion files in a verified export

Thread 3 does not edit `include|src/Integration/Common/**`. This request proposes a change
to that code. The lines below are against `src/Integration/Common/VerifiedStagedExport.cpp`
as it exists on `claude/gen8-native-20261015`.

## Why

Sword/Shield keeps one logical save in a folder: `main`, `backup` and `poke_trade`
(`src/Save/GetSaveFileContents.cpp:821` copies all three). The coordinator's default for
the Gen VIII lane is:
- Export an edited `main`.
- Carry `backup` and `poke_trade` **byte-identical** into the export folder.
- Document both in the manifest.

`publishVerifiedStagedExport` writes exactly three files today: `original_backup<ext>`,
`edited<ext>` and `EDIT_MANIFEST.txt`.

## Proposed API (additive; existing callers unchanged)

```cpp
struct CompanionFile {
    std::string name;                 // safeStem() + optional extension, e.g. "backup"
    std::span<const uint8_t> bytes;   // copied unmodified
};
struct VerifiedExportRequest {
    // ... existing fields ...
    std::vector<CompanionFile> companions;  // default empty
};
```

## Proposed behaviour

1. Validate each companion name:
   - It must pass `safeStem`.
   - It must not collide with `original_backup<ext>`, `edited<ext>` or `EDIT_MANIFEST.txt`, or with another companion.
   - An invalid name refuses the export.
2. Write each companion into the temporary directory with `writeSynced`, after the edited
   file and before the manifest.
3. Read each one back, and require a byte-equal SHA-256 that matches the request bytes.
4. In the manifest, add one line per companion:
   `COMPANION=<name> SHA256=<hex> BYTES=<n> UNMODIFIED`.
5. Any failure removes the temporary directory, as the existing failure points do. The
   host fault-injection enum would gain `AfterCompanionWrite`.

## Gen VIII behaviour until this lands

- `Gen8StagedSession` exports only `main`.
- It records `backup` and `poke_trade` in the change lines as `companion <name> sha256=<hex> (not copied: shared hook pending)`.
- The UI must tell the user to copy those two files alongside the exported `main` if they need them.
- Nothing is guessed or regenerated.

## Regression test proposed for the shared suite

- An export with two companions publishes five files, and each companion is byte-identical.
- A companion named `edited.sav` is refused.
- An injected failure after the companion write leaves no directory behind.
