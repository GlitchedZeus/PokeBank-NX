# PokeBank NX — Pokémon Artwork Motion Device Feedback

Recorded: 2026-09-05

## Exact tested artifact

```text
Application source: 0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915
Artifact: PokeBank-NX-Second-Device-0ea98cc1.nro
Size: 155117481 bytes
SHA-256: 4c220bdf1736fb626e97c30b4ceb89fb7da7a4f24bce17c1dd36d25017478f28
```

## Physical result

```text
POKEMON ARTWORK:                         PASS
ARTWORK QUALITY:                        GOOD
ARTIFICIAL IDLE / BREATHING MOTION:     FAIL / REMOVE
```

The packaged static PokeAPI HOME PNG artwork is visible and looks good. The failure is the
inherited `drawSpriteIdle(...)` presentation: its sine-driven vertical bob and independent width
and height changes make static artwork appear to bounce and squash/stretch.

## Required correction

PokeBank NX Summary/View and selected-preview renders must use a static, centered,
aspect-ratio-preserving image path. The correction must preserve species/form/shiny selection,
the sprite cache, HD artwork resolution, asset packaging, and the existing resolver order:

```text
pokemon_hd/<sprite>.png
then
sprites/pokemon/<sprite>.png
```

No replacement sprite download or artificial animation is part of this correction. A replacement
binary is not `DEVICE TESTED` until its exact hash is physically exercised.
