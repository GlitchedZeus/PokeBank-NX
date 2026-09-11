# Packaged game-card artwork

Generation I Red/Blue/Yellow and Generation III Ruby/Sapphire/Emerald/FireRed/LeafGreen parent cards use the shared `SystemIcons::gameCardIcon` resolver and these release-specific tracked assets. They are copied into `romfs:/game_cards/` by `make game-card-art`.

## Game Boy — Red / Blue / Yellow

Source repository: `libretro-thumbnails/Nintendo_-_Game_Boy`

Pinned revision: `f033af49af2d9761d6821aff479bb0ba5feea5e3`

Files:

- `Named_Boxarts/Pokemon - Red Version (USA, Europe) (SGB Enhanced).png` -> `red_gb.png`
- `Named_Boxarts/Pokemon - Blue Version (USA, Europe) (SGB Enhanced).png` -> `blue_gb.png`
- `Named_Boxarts/Pokemon - Yellow Version - Special Pikachu Edition (USA, Europe) (CGB+SGB Enhanced).png` -> `yellow_gb.png`

The exact SHA-256 values are captured by the staging/build records and device preflight.

## Game Boy Advance — Generation III

Source repository: `libretro-thumbnails/Nintendo_-_Game_Boy_Advance`

Pinned revision: `23b8665408e767fd3220bfb83fd5ad8dfe1a9aa1`

Files:

- `Named_Boxarts/Pokemon - FireRed Version (USA).png` -> `firered_gba.png`
- `Named_Boxarts/Pokemon - LeafGreen Version (USA).png` -> `leafgreen_gba.png`
- `Named_Boxarts/Pokemon - Ruby Version (USA).png` -> `ruby_gba.png`
- `Named_Boxarts/Pokemon - Sapphire Version (USA).png` -> `sapphire_gba.png`
- `Named_Boxarts/Pokemon - Emerald Version (USA, Europe).png` -> `emerald_gba.png`

Existing SHA-256 values:

- `firered_gba.png`: `d7c4f0661bf528d6555969da7b18efc7611fc35d0a4d911538fd1c398c7ce964`
- `leafgreen_gba.png`: `7d73f2d7fecd2b5d2339bdf4d6000d45f20e046951f6e89dd2f73476498bc373`
- `ruby_gba.png`: `0c264af577ca175f0d73b6766b1100dd4b78d6728bbcfb07298679500f0316e1`
- `sapphire_gba.png`: `2bcea4502d475507ed7f07f3bbaf45f36ec30446c5a02519f950bea31b3154fb`
- `emerald_gba.png`: `f39a4d3d7044d2ad693a60f6af362619f7268cd9187c6576f3d4e3373dfa49b5`

Pokémon and its artwork are trademarks/copyright of their respective owners. These images are included for game identification in this non-commercial homebrew utility; upstream attribution and project licensing remain intact.
