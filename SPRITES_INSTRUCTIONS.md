# ROMFS Directory

This directory contains assets bundled into the application via ROMFS.

## Getting Sprites

Run the download script from the project root:

```bash
powershell -ExecutionPolicy Bypass -File download_sprites.ps1
```

This will download all Pokemon sprites (Gen 1-9, species 1-1025) into `romfs/sprites/pokemon/`.

## Sprite Naming Convention

- Normal: `{speciesId}.png` (e.g., `25.png` for Pikachu)
- Shiny: `{speciesId}s.png` (e.g., `25s.png` for Shiny Pikachu)

## Building Without Sprites

The application will build and run without sprites. Pokemon will display with text only (no images). Sprites are optional but recommended for the best visual experience.

## Sprite Sources

Sprites are from [PokeAPI Sprites Repository](https://github.com/PokeAPI/sprites).
