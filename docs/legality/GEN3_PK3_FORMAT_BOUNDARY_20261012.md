# Gen III PK3 Format Boundary

`Encryption3FRLG` deliberately preserves arbitrary byte spans because it is a
transport codec. `Pokemon3FRLG` is different: it interprets fixed offsets in a
semantic Gen III entity and therefore accepts only exact 80-byte box or
100-byte party records.

Malformed lengths are now quarantined into a zeroed, party-sized safety buffer.
They report `sizeValid() == false`, `checksumValid() == false`, expose neutral
semantic fields, and retain their original length for diagnostics and cloning.
This prevents a truncated import from reaching the PK3 wrapper's fixed offsets
while retaining the codec's byte-preserving behavior for callers that need it.

The audit-owned Stage 2 native property suite covers every length from zero
through 116, exact box and party records, immutable caller buffers, cloning,
and representative field reads/writes. It is executed normally and with
AddressSanitizer plus UndefinedBehaviorSanitizer.
