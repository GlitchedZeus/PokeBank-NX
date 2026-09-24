# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-09-24**

This matrix separates source/read support, staged editing, PokeBank-owned workspace transaction support, and physical acceptance.

No current adapter is approved for live installed-game or emulator-source writing.

| Stable ID | Game | Platform | Read/source state | Pokémon editing | PokeBank-owned true-Move workspace | Device state |
|---|---|---|---|---|---|---|
| red_gb | Red | GB | RetroArch read engine | shared staged editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| blue_gb | Blue | GB | RetroArch read engine | shared staged editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| yellow_gb | Yellow | GB | RetroArch read engine | shared staged editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| gold_gbc | Gold | GBC | RetroArch read engine | shared staged editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| silver_gbc | Silver | GBC | RetroArch read engine | shared staged editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| crystal_gbc | Crystal | GBC | RetroArch read engine | shared staged editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| ruby_gba | Ruby | GBA | RetroArch read engine | shared staged PK3 editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| sapphire_gba | Sapphire | GBA | RetroArch read engine | shared staged PK3 editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| emerald_gba | Emerald | GBA | RetroArch read engine | shared staged PK3 editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| firered_gba | FireRed | GBA | RetroArch read engine | shared staged PK3 editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| leafgreen_gba | LeafGreen | GBA | RetroArch read engine | shared staged PK3 editor | source remains read-only / staged only | READ + EDIT DEVICE ACCEPTED |
| firered_switch | FireRed | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; same-group/native route only | CI VERIFIED FOUNDATION / NOT DEVICE ACCEPTED |
| leafgreen_switch | LeafGreen | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; same-group/native route only | CI VERIFIED FOUNDATION / NOT DEVICE ACCEPTED |
| letsgo_pikachu_switch | Let's Go, Pikachu! | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; cross-game route gated | NOT DEVICE ACCEPTED |
| letsgo_eevee_switch | Let's Go, Eevee! | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; cross-game route gated | NOT DEVICE ACCEPTED |
| sword_switch | Sword | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; same-group/native route only | NOT DEVICE ACCEPTED |
| shield_switch | Shield | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; same-group/native route only | NOT DEVICE ACCEPTED |
| brilliant_diamond_switch | Brilliant Diamond | Switch | native source foundation | validation in progress | BLOCKED — multi-file journal required | NOT DEVICE ACCEPTED |
| shining_pearl_switch | Shining Pearl | Switch | native source foundation | validation in progress | BLOCKED — multi-file journal required | NOT DEVICE ACCEPTED |
| legends_arceus_switch | Legends: Arceus | Switch | defensive read/workspace foundation | validation in progress | supported single-file PokeBank workspace; cross-game route gated | NOT DEVICE ACCEPTED |
| scarlet_switch | Scarlet | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; cross-game route gated | NOT DEVICE ACCEPTED |
| violet_switch | Violet | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; cross-game route gated | NOT DEVICE ACCEPTED |
| legends_za_switch | Legends: Z-A | Switch | native source/workspace foundation | validation in progress | supported single-file PokeBank workspace; cross-game route gated | NOT DEVICE ACCEPTED |

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch are deliberately separate identities.

## Device-accepted Gen I–III editor evidence

~~~text
Application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

NRO:
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro

SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Actions:
35825830004

Artifact:
10735208869

Status:
DEVICE ACCEPTED
~~~

## Current transaction foundation

Draft PR #79 now contains the software-integrated destination-first Move transaction for:

~~~text
PokeBank bank.dat
<->
supported PokeBank-owned mutable single-file workspace
~~~

The transaction layer is CI verified but not physically power-loss accepted.

Cross-game true Move remains disabled until the F05–F13 conversion audit proves route fidelity.

BDSP remains disabled because SaveData.bin + Backup.bin require a recoverable multi-file generation.

## Safety policy

~~~text
installed Switch source       READ ONLY
RetroArch / legacy source     READ ONLY
other emulator source         READ ONLY
PokeBank-owned workspace      MUTABLE / VALIDATED
live installed writing        HARD DISABLED
live emulator writing         HARD DISABLED
cross-game true Move          FAIL CLOSED UNTIL ROUTE PROVEN
~~~
