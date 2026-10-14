#!/usr/bin/env bash
# Build ONE exact-HEAD PokeBank NX Gen V Save Instances hardware candidate.
# Works in a disposable local checkout; never overwrites caller files/saves.
set -Eeuo pipefail

fail() { printf 'Gen V build blocked: %s\n' "$*" >&2; exit 1; }
command -v git >/dev/null || fail "git is required"
command -v python3 >/dev/null || fail "python3 is required"
command -v make >/dev/null || fail "make is required"
command -v sha256sum >/dev/null || fail "sha256sum is required"
CONTAINER_RUNTIME="${CONTAINER_RUNTIME:-docker}"
command -v "$CONTAINER_RUNTIME" >/dev/null ||
    fail "$CONTAINER_RUNTIME is required for pinned devkitA64 compilation"

ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" ||
    fail "Run from a real PokeBank-NX git checkout"
EXPECTED="${GEN5_APPLICATION_SHA:-$(git -C "$ROOT" rev-parse HEAD)}"
[[ "$EXPECTED" =~ ^[0-9a-f]{40}$ ]] || fail "expected SHA must be 40 lowercase hex digits"
git -C "$ROOT" cat-file -e "${EXPECTED}^{commit}" ||
    fail "exact commit not present locally; fetch it first"
SHORT="${EXPECTED:0:8}"
DEST="${1:-$ROOT/hardware-candidates/gen5-$SHORT}"
mkdir -p "$DEST"
DEST="$(cd "$DEST" && pwd -P)"
[[ ! -e "$DEST/BUILD_IDENTITY.txt" && ! -e "$DEST/PokeBank-NX-Gen5-$SHORT.nro" ]] ||
    fail "output already contains a candidate; choose a new empty directory"

TMP="$(mktemp -d)"
cleanup() {
    if [[ -f "$TMP/work/gen5-local-native-build.log" ]]; then
        cp "$TMP/work/gen5-local-native-build.log" "$DEST/native-build.log" || true
    fi
    rm -rf "$TMP"
}
trap cleanup EXIT

printf 'PokeBank NX Gen V smoke build\n  Application SHA: %s\n  Output: %s\n' "$EXPECTED" "$DEST"
printf 'Only committed source is copied; the original working tree and save files are untouched.\n'
git clone --local --no-hardlinks "$ROOT" "$TMP/work" >/dev/null
WORK="$TMP/work"
git -C "$WORK" checkout --detach "$EXPECTED" >/dev/null
[[ "$(git -C "$WORK" rev-parse HEAD)" == "$EXPECTED" ]] ||
    fail "disposable checkout does not match requested SHA"
git -C "$WORK" submodule update --init --recursive
RECOVERY_SHA="5bfc27a10de1eeaf52cb92316c1453df3d4fb613"
if ! git -C "$WORK" cat-file -e "${RECOVERY_SHA}^{commit}" 2>/dev/null; then
    git -C "$WORK" fetch https://github.com/GlitchedZeus/PokeBank-NX.git "$RECOVERY_SHA" ||
        fail "permanent asset recovery commit cannot be fetched"
fi
git -C "$WORK" worktree add --detach "$TMP/recovery" "$RECOVERY_SHA" >/dev/null

cd "$WORK"
# Fail closed if any normal test or shared-profile/source UI contract fails.
make -f Makefile.host \
    build-host/test_gen5_pk5_readonly \
    build-host/test_gen5_readonly_save \
    build-host/test_gen5_staged_save_candidate \
    build-host/test_gen5_native_create \
    build-host/test_gen5_create_integration \
    build-host/test_gen5_box_touch
for bin in test_gen5_pk5_readonly test_gen5_readonly_save \
           test_gen5_staged_save_candidate test_gen5_native_create \
           test_gen5_create_integration test_gen5_box_touch; do
    "./build-host/$bin"
done
python3 tests/test_gen5_readonly_ui_contract.py
python3 tests/test_reachable_ui_consistency.py
python3 tests/test_game_hub_contract.py

# Exactly follow the established Product UI Native recovery/asset pipeline,
# only in this temporary checkout. No source SAV5 paths are passed or opened.
python3 tools/fetch_product_art.py
(
    cd "$TMP/recovery"
    python3 tools/recover_workspace.py
)
mkdir -p "$TMP/branch-romfs-overrides"
for part in trainer_portraits region_backdrops; do
    if [[ -d "romfs/$part" ]]; then
        cp -a "romfs/$part" "$TMP/branch-romfs-overrides/$part"
    fi
done
rm -rf romfs
cp -a "$TMP/recovery/romfs" romfs
for part in trainer_portraits region_backdrops; do
    if [[ -d "$TMP/branch-romfs-overrides/$part" ]]; then
        rm -rf "romfs/$part"
        cp -a "$TMP/branch-romfs-overrides/$part" "romfs/$part"
    fi
done
for name in gold_gbc silver_gbc crystal_gbc diamond_nds pearl_nds \
            platinum_nds heartgold_nds soulsilver_nds \
            black_nds white_nds black2_nds white2_nds; do
    [[ -s "assets/game_cards/$name.png" ]] ||
        fail "required recovered game cover missing: $name"
    cp "assets/game_cards/$name.png" "romfs/game_cards/$name.png"
done
python3 tools/gen_item_sprites.py
python3 tools/check_device_assets.py

IMAGE="devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282"
"$CONTAINER_RUNTIME" run --rm -v "$WORK:/work" -w /work "$IMAGE" bash -lc '
    set -euo pipefail
    : "${DEVKITPRO:?DEVKITPRO missing inside image}"
    git config --global --add safe.directory /work
    make clean || true
    make -j1
    test -s PokeBankNX.elf
    test -s PokeBankNX.nro
' 2>&1 | tee gen5-local-native-build.log

[[ -s PokeBankNX.nro && -s PokeBankNX.elf ]] ||
    fail "devkitA64 produced no verifiable NRO and ELF"
[[ "$(git rev-parse HEAD)" == "$EXPECTED" ]] ||
    fail "application HEAD changed during the isolated build"
OUT_NRO="PokeBank-NX-Gen5-$SHORT.nro"
cp PokeBankNX.nro "$DEST/$OUT_NRO"
(cd "$DEST" && sha256sum "$OUT_NRO" > SHA256SUMS.txt)
printf 'APPLICATION_SHA=%s\nTREE_SHA=%s\nDEVICE_ACCEPTED=false\nSOURCE_SAV5_WRITE=DISABLED\n' \
    "$EXPECTED" "$(git rev-parse 'HEAD^{tree}')" > "$DEST/BUILD_IDENTITY.txt"
printf '\nExact-head development NRO: %s\n' "$DEST/$OUT_NRO"
printf 'Check SHA256SUMS.txt; this build is not hardware-accepted.\n'
