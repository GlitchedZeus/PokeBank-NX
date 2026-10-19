#!/usr/bin/env bash
# Compile the actual app workspace against this lane's engine without changing either lane.
set -euo pipefail
app=$(cd "$1" && pwd)
repo=$(cd "$(dirname "$0")/../.." && pwd)
work=${APP_BRIDGE_BUILD:-/tmp/gen15-app-bridge}
mkdir -p "$work"
mkdir -p "$work/include"
ln -sfn "$repo/include/Legality" "$work/include/Legality"
# This is the minimum source set used by the existing Gen5App adapter host gate.
sources=(src/Legality/Gen5AppAdapter.cpp src/Legality/Gen5Legality.cpp
 src/Legality/Gen5LegalityData.cpp src/Legality/Gen5Text.cpp src/Names/SpeciesNames.cpp
 src/Names/MoveNames.cpp src/Pokemon/Experience.cpp src/Legality/Gen4Legality.cpp
 src/Legality/Gen4LegalityData.cpp src/Legality/Gen4LegalityEncounters.cpp
 src/Legality/Gen4LegalityMoves.cpp src/Legality/Gen4LegalityPalPark.cpp
 src/Legality/Gen4LegalityRng.cpp src/Legality/Gen4LegalityText.cpp
 src/Legality/Gen3Legality.cpp src/Legality/Gen3LegalityData.cpp src/Legality/Gen3LegalityDataCxd.cpp)
# Overlay only the reviewed legality headers; keep the app's native workspace/crypto APIs.
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-unused-parameter \
 ${APP_BRIDGE_FLAGS:-} -I"$work/include" -I"$app/include" -I"$repo/include" -I"$repo/tests" \
 "$repo/tests/app/test_gen5_workspace_legality.cpp" "${sources[@]/#/$repo/}" -o "$work/test"
"$work/test"
