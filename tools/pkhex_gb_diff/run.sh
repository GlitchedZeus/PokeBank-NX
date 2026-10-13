#!/usr/bin/env bash
# Differential test: PokeBank NX's Gen I legality verdicts against PKHeX.Core.
#   tools/pkhex_gb_diff/run.sh PKHEX_CHECKOUT POKERED_CHECKOUT [COUNT] [SEED]
# Needs the .NET 10 SDK and g++. Exits non-zero when any verdict differs.
set -euo pipefail
pkhex=$(cd "$1" && pwd); pokered=$(cd "$2" && pwd); count=${3:-20000}; seed=${4:-1}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
python3 -I "$here/gen_corpus.py" --pkhex "$pkhex" --pokered "$pokered" --count "$count" --seed "$seed" > "$work/corpus.txt"
dotnet build "$here/oracle/PKHeXGBOracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" -o "$work/oracle" > "$work/build.log" \
    || { cat "$work/build.log"; exit 1; }
dotnet "$work/oracle/PKHeXGBOracle.dll" < "$work/corpus.txt" > "$work/pkhex.txt"
g++ -std=c++20 -O2 -I"$repo/include" "$here/gb_diff.cpp" "$repo/src/Legality/GBLegality.cpp" \
    "$repo/src/Legality/GBLegalityData.cpp" "$repo/src/Names/SpeciesNames.cpp" "$repo/src/Names/MoveNames.cpp" \
    "$repo/src/Integration/Gen1/Gen1ReadOnlySave.cpp" -o "$work/gb_diff"
"$work/gb_diff" < "$work/corpus.txt" > "$work/pokebank.txt"
python3 -I "$here/compare.py" "$work/corpus.txt" "$work/pkhex.txt" "$work/pokebank.txt" --show "${SHOW:-8}"
