# Gen III PK3 Format Boundary

`Encryption3FRLG` deliberately preserves arbitrary byte spans because it is a
transport codec. `Pokemon3FRLG` is different: it interprets fixed offsets in a
semantic Gen III entity and therefore accepts only exact 80-byte box or
100-byte party records.

Malformed lengths are now quarantined into a fixed zeroed, party-sized safety
buffer. They report `sizeValid() == false`, `checksumValid() == false`, expose
neutral semantic fields, and retain their original length through `inputSize()`
for diagnostics. Their entity data size is zero and cloning returns null: raw
malformed bytes belong at the transport boundary, not in a semantic PK3 entity.
This prevents a truncated or oversized import from reaching fixed offsets or
driving allocation/copying proportional to an attacker-controlled length.

The audit-owned Stage 2 native property suite covers every length from zero
through 116, exact box and party records, a 64 KiB oversized record, immutable
caller buffers, cloning, and representative field reads/writes. It is executed
normally and with AddressSanitizer plus UndefinedBehaviorSanitizer.
