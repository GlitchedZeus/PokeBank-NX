# PokeBank NX capability matrix (Gen I-IX)

Vocabulary: PASS = Supported (implemented through the shared workflow and host-tested); READ-ONLY = ReadOnly (visible, editing not verified); BLOCKED - <reason> = Blocked (deliberately unavailable, prerequisite named); NOT APPLICABLE - <reason> = NotApplicable (the native game has no such data); NOT IMPLEMENTED = nothing exists yet. PASS means host-tested through the shared workflow, never hardware acceptance; only the "Switch hardware verification" column records an owner result.

Source: `include/Integration/Common/GameCapabilityRegistry.h` (cumulative-inheritance static_assert; `tests/test_game_capability_registry.cpp`). One row per title, grouped by generation. Combined columns list each workflow with its own status.

## Generation I (Game Boy)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Red (`red_gb`, RBY, PK1) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Blue (`blue_gb`, RBY, PK1) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Yellow (`yellow_gb`, RBY, PK1) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |

## Generation II (Game Boy Color)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Gold (`gold_gbc`, GS, PK2) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Silver (`silver_gbc`, GS, PK2) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Crystal (`crystal_gbc`, Crystal, PK2) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |

## Generation III (Game Boy Advance)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Ruby (`ruby_gba`, RS, PK3) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Sapphire (`sapphire_gba`, RS, PK3) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Emerald (`emerald_gba`, E, PK3) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| FireRed (`firered_gba`, FRLG, PK3) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| LeafGreen (`leafgreen_gba`, FRLG, PK3) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |

## Generation IV (Nintendo DS)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Diamond (`diamond_nds`, DP, PK4) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Pearl (`pearl_nds`, DP, PK4) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Platinum (`platinum_nds`, Pt, PK4) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| HeartGold (`heartgold_nds`, HGSS, PK4) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| SoulSilver (`soulsilver_nds`, HGSS, PK4) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |

## Generation V (Nintendo DS)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Black (`black_nds`, BW, PK5) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| White (`white_nds`, BW, PK5) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| Black 2 (`black2_nds`, B2W2, PK5) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |
| White 2 (`white2_nds`, B2W2, PK5) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | PASS | PASS | PASS | READ-ONLY | Create PASS / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | Host suite (GCC/Clang/ASan/UBSan) on Round 10 source | NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open) |

## Generation VI (Nintendo 3DS)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| X (`x_3ds`, XY, PK6) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and geolocation defaults (Gen6StagedSession::createCapability) / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | test_gen6_* lane suites + test_gen6_native_session + test_modern_sources + real-screen test_ui_modern_surface_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Y (`y_3ds`, XY, PK6) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and geolocation defaults (Gen6StagedSession::createCapability) / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | test_gen6_* lane suites + test_gen6_native_session + test_modern_sources + real-screen test_ui_modern_surface_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Omega Ruby (`omegaruby_3ds`, ORAS, PK6) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and geolocation defaults (Gen6StagedSession::createCapability) / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | test_gen6_* lane suites + test_gen6_native_session + test_modern_sources + real-screen test_ui_modern_surface_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Alpha Sapphire (`alphasapphire_3ds`, ORAS, PK6) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and geolocation defaults (Gen6StagedSession::createCapability) / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | PASS | test_gen6_* lane suites + test_gen6_native_session + test_modern_sources + real-screen test_ui_modern_surface_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |

## Generation VII (Nintendo 3DS, Nintendo Switch)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Sun (`sun_3ds`, SM, PK7) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create is unsupported for the same reasons as Gen VI, plus SAV7 export is blocked / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | BLOCKED - SAV7 MemeCrypto re-signing has no retail acceptance evidence (docs/gen7/README.md); staged changes stay in app memory | test_gen7_* lane suites + test_gen7_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen7_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Moon (`moon_3ds`, SM, PK7) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create is unsupported for the same reasons as Gen VI, plus SAV7 export is blocked / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | BLOCKED - SAV7 MemeCrypto re-signing has no retail acceptance evidence (docs/gen7/README.md); staged changes stay in app memory | test_gen7_* lane suites + test_gen7_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen7_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Ultra Sun (`ultrasun_3ds`, USUM, PK7) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create is unsupported for the same reasons as Gen VI, plus SAV7 export is blocked / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | BLOCKED - SAV7 MemeCrypto re-signing has no retail acceptance evidence (docs/gen7/README.md); staged changes stay in app memory | test_gen7_* lane suites + test_gen7_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen7_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Ultra Moon (`ultramoon_3ds`, USUM, PK7) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | PASS | READ-ONLY | Create BLOCKED - Create is unsupported for the same reasons as Gen VI, plus SAV7 export is blocked / Clone PASS / Move PASS / Remove PASS | Backup PASS / Review PASS | BLOCKED - SAV7 MemeCrypto re-signing has no retail acceptance evidence (docs/gen7/README.md); staged changes stay in app memory | test_gen7_* lane suites + test_gen7_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen7_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Let's Go Pikachu (`letsgo_pikachu_switch`, LGPE, PB7) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, GO origin, height/weight and cached CP defaults / Clone PASS / Move BLOCKED - Let's Go storage is a gapless list whose positions the game assigns; Clone appends and Release compacts instead / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen7_* lane suites + test_gen7_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen7b_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Let's Go Eevee (`letsgo_eevee_switch`, LGPE, PB7) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, GO origin, height/weight and cached CP defaults / Clone PASS / Move BLOCKED - Let's Go storage is a gapless list whose positions the game assigns; Clone appends and Release compacts instead / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen7_* lane suites + test_gen7_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen7b_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |

## Generation VIII (Nintendo Switch)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Sword (`sword_switch`, SWSH, PK8) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults (Gen8StagedSession::createUnsupportedReason) / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen8_* lane suites + test_gen8_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen8_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Shield (`shield_switch`, SWSH, PK8) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults (Gen8StagedSession::createUnsupportedReason) / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen8_* lane suites + test_gen8_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen8_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Brilliant Diamond (`brilliant_diamond_switch`, BDSP, PB8) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen8_* lane suites + test_gen8_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen8b_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Shining Pearl (`shining_pearl_switch`, BDSP, PB8) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen8_* lane suites + test_gen8_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen8b_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Legends Arceus (`legends_arceus_switch`, PLA, PA8) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, Alpha/Noble, mastery, size and cached-stat defaults / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen8_* lane suites + test_gen8_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen8a_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |

## Generation IX (Nintendo Switch)

| Game | Dashboard/Save Instances | Party/Boxes | Pokemon editor | Native mechanics | Trainer | Bag | Pokedex | Create/Clone/Move/Remove | Backup/Review | Export and safety | CI | Switch hardware verification |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Scarlet (`scarlet_switch`, SV, PK9) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, Tera type, handler, memory and cached-stat defaults / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen9_* lane suites + test_gen9_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen9sv_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Violet (`violet_switch`, SV, PK9) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, Tera type, handler, memory and cached-stat defaults / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen9_* lane suites + test_gen9_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen9sv_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |
| Legends Z-A (`legends_za_switch`, ZA, PA9) | Dashboard PASS / Save Instances PASS | Party PASS / Boxes PASS | PASS | READ-ONLY | READ-ONLY | READ-ONLY | READ-ONLY | Create BLOCKED - Create needs verifiable EC/PID, personal, met, Alpha, Plus flags, handler and cached-stat defaults / Clone PASS / Move PASS / Remove PASS | Backup READ-ONLY / Review PASS | PASS | test_gen9_* lane suites + test_gen9_native_session + test_modern_sources + real-screen test_ui_modern_surface_gen9za_host (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI | NOT TESTED |

Rows: 38 titles.

## Native mechanics per title (Gen VI-IX)

Flags are the `NativeMechanics` members set to true in the registry; the shared editor offers only these mechanics' rows on that title, and the "Native mechanics" column above says whether they are editable there.

| Game | Generation | Provider | Stat model | Storage | Native mechanics (true flags) |
|---|---|---|---|---|---|
| X (`x_3ds`, XY, PK6) | VI | modern-native | IV/EV | 31 x 30 (SparseNative) | megaForms, fairyType, superTraining, pokemonAmie, memories, geolocation, ribbons |
| Y (`y_3ds`, XY, PK6) | VI | modern-native | IV/EV | 31 x 30 (SparseNative) | megaForms, fairyType, superTraining, pokemonAmie, memories, geolocation, ribbons |
| Omega Ruby (`omegaruby_3ds`, ORAS, PK6) | VI | modern-native | IV/EV | 31 x 30 (SparseNative) | megaForms, fairyType, superTraining, pokemonAmie, memories, geolocation, ribbons |
| Alpha Sapphire (`alphasapphire_3ds`, ORAS, PK6) | VI | modern-native | IV/EV | 31 x 30 (SparseNative) | megaForms, fairyType, superTraining, pokemonAmie, memories, geolocation, ribbons |
| Sun (`sun_3ds`, SM, PK7) | VII | modern-native | IV/EV | 32 x 30 (SparseNative) | megaForms, fairyType, pokemonAmie, memories, geolocation, zMoves, alolanForms, hyperTraining, ribbons |
| Moon (`moon_3ds`, SM, PK7) | VII | modern-native | IV/EV | 32 x 30 (SparseNative) | megaForms, fairyType, pokemonAmie, memories, geolocation, zMoves, alolanForms, hyperTraining, ribbons |
| Ultra Sun (`ultrasun_3ds`, USUM, PK7) | VII | modern-native | IV/EV | 32 x 30 (SparseNative) | megaForms, fairyType, pokemonAmie, memories, geolocation, zMoves, alolanForms, hyperTraining, ribbons |
| Ultra Moon (`ultramoon_3ds`, USUM, PK7) | VII | modern-native | IV/EV | 32 x 30 (SparseNative) | megaForms, fairyType, pokemonAmie, memories, geolocation, zMoves, alolanForms, hyperTraining, ribbons |
| Let's Go Pikachu (`letsgo_pikachu_switch`, LGPE, PB7) | VII | modern-native | IV/Awakening | 40 x 25 (PointerBacked) | megaForms, fairyType, alolanForms, hyperTraining, awakeningValues, partnerRestrictions, goOrigin |
| Let's Go Eevee (`letsgo_eevee_switch`, LGPE, PB7) | VII | modern-native | IV/Awakening | 40 x 25 (PointerBacked) | megaForms, fairyType, alolanForms, hyperTraining, awakeningValues, partnerRestrictions, goOrigin |
| Sword (`sword_switch`, SWSH, PK8) | VIII | modern-native | IV/EV | 32 x 30 (SCBlock) | fairyType, memories, hyperTraining, dynamax, gigantamax, technicalRecords, battleTeamLocks, mintedNature, marks, ribbons, scale |
| Shield (`shield_switch`, SWSH, PK8) | VIII | modern-native | IV/EV | 32 x 30 (SCBlock) | fairyType, memories, hyperTraining, dynamax, gigantamax, technicalRecords, battleTeamLocks, mintedNature, marks, ribbons, scale |
| Brilliant Diamond (`brilliant_diamond_switch`, BDSP, PB8) | VIII | modern-native | IV/EV | 40 x 30 (FlatMD5) | fairyType, hyperTraining, mintedNature, ribbons, scale |
| Shining Pearl (`shining_pearl_switch`, BDSP, PB8) | VIII | modern-native | IV/EV | 40 x 30 (FlatMD5) | fairyType, hyperTraining, mintedNature, ribbons, scale |
| Legends Arceus (`legends_arceus_switch`, PLA, PA8) | VIII | modern-native | IV/EffortLevel | 32 x 30 (SCBlock) | fairyType, hyperTraining, effortLevels, alphaPokemon, moveMastery, pastures, mintedNature, ribbons, scale |
| Scarlet (`scarlet_switch`, SV, PK9) | IX | modern-native | IV/EV | 32 x 30 (SCBlock) | fairyType, memories, hyperTraining, teraType, mintedNature, marks, ribbons, scale |
| Violet (`violet_switch`, SV, PK9) | IX | modern-native | IV/EV | 32 x 30 (SCBlock) | fairyType, memories, hyperTraining, teraType, mintedNature, marks, ribbons, scale |
| Legends Z-A (`legends_za_switch`, ZA, PA9) | IX | modern-native | IV/EV | 32 x 30 (SCBlock) | megaForms, fairyType, hyperTraining, alphaPokemon, mintedNature, plusFlags, scale |

## Blockers

Every Blocked reason in the registry, once, with the workflows and titles it applies to. A blocker is lifted only by changing the registry row together with the adapter and host tests that prove it.

- Create needs verifiable EC/PID, personal, met, handler, memory and geolocation defaults (Gen6StagedSession::createCapability)
  - Workflows: Create
  - Titles: X, Y, Omega Ruby, Alpha Sapphire
- Create is unsupported for the same reasons as Gen VI, plus SAV7 export is blocked
  - Workflows: Create
  - Titles: Sun, Moon, Ultra Sun, Ultra Moon
- SAV7 MemeCrypto re-signing has no retail acceptance evidence (docs/gen7/README.md); staged changes stay in app memory
  - Workflows: Export and safety
  - Titles: Sun, Moon, Ultra Sun, Ultra Moon
- Create needs verifiable EC/PID, personal, met, GO origin, height/weight and cached CP defaults
  - Workflows: Create
  - Titles: Let's Go Pikachu, Let's Go Eevee
- Let's Go storage is a gapless list whose positions the game assigns; Clone appends and Release compacts instead
  - Workflows: Move/Swap
  - Titles: Let's Go Pikachu, Let's Go Eevee
- Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults (Gen8StagedSession::createUnsupportedReason)
  - Workflows: Create
  - Titles: Sword, Shield
- Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults
  - Workflows: Create
  - Titles: Brilliant Diamond, Shining Pearl
- Create needs verifiable EC/PID, personal, met, Alpha/Noble, mastery, size and cached-stat defaults
  - Workflows: Create
  - Titles: Legends Arceus
- Create needs verifiable EC/PID, personal, met, Tera type, handler, memory and cached-stat defaults
  - Workflows: Create
  - Titles: Scarlet, Violet
- Create needs verifiable EC/PID, personal, met, Alpha, Plus flags, handler and cached-stat defaults
  - Workflows: Create
  - Titles: Legends Z-A

Generated by tools/capability_matrix.cpp from GameCapabilityRegistry.h; do not edit by hand.
