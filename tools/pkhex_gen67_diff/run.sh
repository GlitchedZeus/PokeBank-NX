#!/usr/bin/env bash
# Differential test: PokeBank NX's Gen VI/VII (PK6/PK7) legality verdicts against PKHeX.Core.
#   tools/pkhex_gen67_diff/run.sh PKHEX_CHECKOUT [SEED] [MUTATIONS]
# 1. The oracle's "dump" must reproduce include/Legality/Gen67LegalityData.inc byte for
#    byte (every rule table the C++ analyzer reads comes from this PKHeX checkout).
# 2. The oracle builds every Gen VI/VII encounter PKHeX knows (X/Y/OR/AS, SN/MN/US/UM),
#    Gen V encounters moved with Poke Transporter, Virtual Console Gen I/II encounters
#    moved to Gen VII and grown copies, plus MUTATIONS single-field mutations of each.
# 3. gen67_diff.cpp analyzes every record and fails when
#    - PokeBank NX reports Invalid for a record PKHeX accepts (a false Invalid), or
#    - PokeBank NX reports "No problems found" for a record PKHeX rejects (a false Valid).
# Records PKHeX rejects that PokeBank NX leaves Incomplete are counted, not failed.
# Needs the .NET 10 SDK and g++.
#   GEN67_DIFF_KEEP=DIR  keep the corpus and the report in DIR.
set -euo pipefail
pkhex=$(cd "$1" && pwd); seed=${2:-1}; mutations=${3:-4}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
dotnet build "$here/oracle/PKHeXGen67Oracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" \
    -p:UseArtifactsOutput=true -p:ArtifactsPath="$work/artifacts" -o "$work/oracle" > "$work/build.log" || { cat "$work/build.log"; exit 1; }
dotnet "$work/oracle/PKHeXGen67Oracle.dll" dump > "$work/data.inc"
if ! cmp -s "$work/data.inc" "$repo/include/Legality/Gen67LegalityData.inc"; then
    echo "include/Legality/Gen67LegalityData.inc differs from the PKHeX dump:"
    diff "$repo/include/Legality/Gen67LegalityData.inc" "$work/data.inc" | head -20
    exit 1
fi
echo "generated Gen VI/VII tables match PKHeX"
dotnet "$work/oracle/PKHeXGen67Oracle.dll" generate "$seed" "$mutations" > "$work/pkhex.txt"
g++ -std=c++20 -O2 -Wall -Wextra -Werror -I"$repo/include" "$here/gen67_diff.cpp" \
    "$repo/src/Legality/ModernLegality.cpp" "$repo/src/Legality/Gen67Legality.cpp" \
    "$repo/src/Legality/Gen67LegalityData.cpp" "$repo/src/Pokemon/Experience.cpp" -o "$work/gen67_diff"
status=0
"$work/gen67_diff" "${SHOW:-20}" < "$work/pkhex.txt" > "$work/report.txt" || status=$?
cat "$work/report.txt"
if [ -n "${GEN67_DIFF_KEEP:-}" ]; then mkdir -p "$GEN67_DIFF_KEEP"; cp "$work/pkhex.txt" "$work/report.txt" "$GEN67_DIFF_KEEP/"; fi
exit $status
