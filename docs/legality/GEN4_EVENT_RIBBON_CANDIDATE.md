# EVENT-001: independently verified ribbon requirements

The pinned 247-card Gen IV structural template index currently omits fixed/variable event ribbon conditions. It is therefore unsafe to infer event ribbon bits from the card ID. This extension exposes the **three precise, checksum-validated immutable native PK4 ribbon words**, then accepts a *separate independently verified* `Requirement` with present/absent bit masks.

A card identity must match the existing Gen IV template matcher before an externally supplied requirement is evaluated. Missing native capability, missing archived ribbon data and contradictory supplied masks have distinct statuses. Missing required persistent bits or unexpected forbidden bits contradict only **one asserted original event candidate**; they are never grounds to declare the Pokémon globally Invalid, because other cards, trades, evolutions and later acquisition histories remain possible.

Tests cover direct/evolved pinned Charmander event card #67 identity, known/unknown ribbon masks, required/forbidden bits, contradictory source masks, unsupported native capability and a real encrypted PK4 roundtrip through the strict read-only ribbon accessor. Mask values are explicitly synthetic, **not claims about card #67's actual release payload**. No external copyrighted event payload imported.
