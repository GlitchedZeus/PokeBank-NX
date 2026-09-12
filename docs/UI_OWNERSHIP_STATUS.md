# PokeBank NX UI ownership status

Issue #58 remains open. This pass establishes the PokeBank-owned product boundary and cleans touched runtime/editor identity; it does not claim that every inherited visible screen has been replaced.

| Surface | Current ownership | Retain as backend | Visible layer still to replace/refine |
| --- | --- | --- | --- |
| Home | MIXED | shared navigation/state/chrome | continue converging remaining inherited layout pieces |
| Game selection | MIXED | title/profile/source discovery and validated adapters | finish PokeBank-owned source/profile UX and touch parity |
| Trainer | MIXED | proven trainer models and read/write capability gates | replace remaining inherited TrainerView presentation incrementally |
| Party | MIXED | party models, Pokémon entities, legality/source rules | PokeBank-owned touch/controller presentation |
| Boxes | MIXED | box models, move/copy safety, source adapters | PokeBank-owned storage/box workspace |
| Inventory | MIXED | proven per-game pouch/item models | unified PokeBank inventory/editor presentation |
| Summary | MIXED | entity parsing, names, legality/provenance data | unified PokeBank summary presentation |
| Editor | MIXED | inherited common editor backend plus PokeBank staged Gen II sidecar | expand PokeBank-owned staged UI and touch controls |
| Backups | INHERITED/MIXED | proven backup-copy plumbing and safety policy | PokeBank backup/recovery browser and transaction UX |
| Settings | MIXED | settings model and persisted toggles | PokeBank-owned settings organization/touch surface |

Internal class names such as `PKSEFramebuffer` and namespaces such as `PokeVault::` are implementation details and are not treated as runtime prerequisites or user-facing product identity. Required attribution remains intact.
