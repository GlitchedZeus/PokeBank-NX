# PK4 canonical encrypted blank oracles

`blank-stored.hex` is 0x88 bytes and `blank-party.hex` is 0xEC bytes, hex encoded.
They are encrypted all-zero *decrypted* records (PID/sanity/checksum zero), not
raw zero save slots. Generated with the pinned PKSM-Core implementation:
`aa22d7a4f87c0351baf7da5962ba5acd01039a7c`, `PK4::encrypt` sequence and
`utils/crypto.hpp` templates. The test compares these fixed bytes against both
PKSM-Core and PokeBank's independent implementation.

To intentionally regenerate after reviewing the oracle pin:

```sh
make -f Makefile.host build-host/test_gen4_readonly_foundation
./build-host/test_gen4_readonly_foundation --generate-oracles
```

The normal test invocation is read-only and asserts exact fixture equality.
No real user save data is included.
