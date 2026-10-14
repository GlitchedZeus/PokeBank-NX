#!/usr/bin/env bash
# Stage the complete device RomFS for an NRO build without losing branch-owned artwork.
#
# Usage: tools/prepare_release_romfs.sh <recovered-snapshot-romfs-dir>
#
# The permanent recovery snapshot supplies the large immutable baseline (HOME sprites, fonts,
# type icons). It predates the Product Home presentation assets, so it must only FILL GAPS:
# replacing romfs/ wholesale is exactly what dropped region backdrops and trainer portraits
# from an earlier hardware NRO. After staging, the source-tree preflight must pass; the packaged
# NRO is verified separately by tools/verify_nro_romfs.py.
set -euo pipefail

snapshot="${1:?usage: prepare_release_romfs.sh <recovered-snapshot-romfs-dir>}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

test -d "$snapshot"
mkdir -p romfs
# Copy only paths the branch does not already own (never overwrite or delete branch assets).
(cd "$snapshot" && find . -type d -print0) | while IFS= read -r -d '' d; do mkdir -p "romfs/$d"; done
(cd "$snapshot" && find . -type f -print0) | while IFS= read -r -d '' f; do
  [ -e "romfs/$f" ] || cp -p "$snapshot/$f" "romfs/$f"
done

# Every Gen I-IV cover is a branch asset; the snapshot copies may be stale.
mkdir -p romfs/game_cards
for f in red_gb blue_gb yellow_gb gold_gbc silver_gbc crystal_gbc \
         ruby_gba sapphire_gba emerald_gba firered_gba leafgreen_gba \
         diamond_nds pearl_nds platinum_nds heartgold_nds soulsilver_nds; do
  test -s "assets/game_cards/$f.png"
  cp -f "assets/game_cards/$f.png" "romfs/game_cards/$f.png"
done

# Trainer portraits + Hisui/Paldea are fetched from pinned sources; Kanto-Galar are verified in place.
python3 tools/fetch_product_art.py
# The snapshot predates per-ball/per-item PNGs; regenerate them with this head's pinned generator.
python3 tools/gen_item_sprites.py
python3 tools/check_device_assets.py
