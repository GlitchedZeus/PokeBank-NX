# WC5FULL distributor evidence parsing: source pins and limitations

First-party distribution receipts are NOT authenticated by a valid-looking
Wonder Card alone. This new read-only parser is a step toward analysing source
evidence, not yet a change to Gen V Pokémon legality verdicts.

Field map: https://projectpokemon.org/home/docs/gen-5/5th-generation-wonder-card-full-map-r148/
Original distributor structure research: https://projectpokemon.org/home/forums/topic/43342-gen-5-full-wondercard-structure/
Independent C# layout reader: https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/MysteryGifts/WC5Full.cs
Game ID semantics: https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/Game/Enums/GameVersion.cs

Supported: exact 0x2D0 length, embedded 0xCC-byte PGF span, little-endian
version flags, language byte, checksum, exact full-PGF matching to a supplied
catalog payload and tri-state declared eligibility. The standard
CRC16-CCITT-FALSE implementation (0x1021 / 0xFFFF, check 0x29B1 on ASCII
123456789) has passed host tests but must still be verified against an
independently preserved, unaltered original WC5FULL to confirm the dialect.

Not supported: historical authenticity, per-distributor regional/time
restrictions, device-language vs cart-language distinction, binding a card
to the 709 catalog records without exact byte comparison, or an exhaustive
event list. A malformed/unmatched/ambiguous card NEVER proves a Pokémon
Invalid or legal. Do not change any Incomplete verdict on these synthetic
format tests alone.

Next: obtain unmodified full cards lawfully, hash and classify their provenance,
cross-check CRC and eligibility, and bind only byte-identical PGF payloads
to card-specific, source-backed restrictions. All existing 3,515 uncertain
receipt histories remain Incomplete pending authenticated evidence.
