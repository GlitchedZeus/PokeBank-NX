#!/usr/bin/env bash
# Differential test: PokeBank NX's Gen V (PK5) legality verdicts against PKHeX.Core.
#   tools/pkhex_gen5_diff/run.sh PKHEX_CHECKOUT [SEED] [MUTATIONS]
# The oracle builds every Gen V encounter PKHeX knows (species 1-649, all forms,
# B/W/B2/W2) plus MUTATIONS single-field mutations of each. Needs the .NET 10 SDK
# and g++. Exits non-zero when PokeBank NX reports Invalid for a Pokemon PKHeX
# accepts (a false Invalid), or when a record gets no verdict. Records PKHeX
# rejects that PokeBank NX only reports as Incomplete are listed, not failed:
# Unknown must never be turned into Invalid.
set -euo pipefail
pkhex=$(cd "$1" && pwd); seed=${2:-1}; mutations=${3:-3}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
dotnet build "$here/oracle/PKHeXGen5Oracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" -o "$work/oracle" > "$work/build.log" || { cat "$work/build.log"; exit 1; }
dotnet "$work/oracle/PKHeXGen5Oracle.dll" generate "$seed" "$mutations" > "$work/pkhex.txt"
g++ -std=c++20 -O2 -I"$repo/include" "$here/gen5_diff.cpp" "$repo/src/Legality/Gen5Legality.cpp" \
    "$repo/src/Legality/Gen5LegalityData.cpp" "$repo/src/Names/SpeciesNames.cpp" "$repo/src/Names/MoveNames.cpp" \
    "$repo/src/Pokemon/Experience.cpp" -o "$work/gen5_diff"
"$work/gen5_diff" < "$work/pkhex.txt" > "$work/pokebank.txt"
test "$(wc -l < "$work/pkhex.txt")" = "$(wc -l < "$work/pokebank.txt")"
paste -d'\t' "$work/pkhex.txt" "$work/pokebank.txt" > "$work/joined.txt"
echo "seed $seed, $mutations mutations per encounter: PKHeX verdict / PokeBank NX verdict"
awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[1]!=b[1]) {print "id mismatch " a[1]; exit 1} c[a[3]"/"b[2]]++}
    END{for(k in c) print "  " c[k], k}' "$work/joined.txt" | sort -k2
echo "PKHeX rejects, PokeBank NX Incomplete (by mutation and PKHeX result):"
awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[3]=="INVALID" && b[2]!="INVALID") {m=a[1]; sub(/^m[0-9]+-/,"",m); sub(/^b[0-9]+$/,"base",m); r=$1; sub(/^[^;]*; /,"",r); print m" :: "r}}' \
    "$work/joined.txt" | sort | uniq -c | sort -rn | head -"${SHOW:-15}"
false_invalid=$(awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[3]=="VALID" && b[2]=="INVALID") print}' "$work/joined.txt")
if [ -n "$false_invalid" ]; then
    echo "FALSE INVALID (PKHeX VALID, PokeBank NX INVALID):"
    printf '%s\n' "$false_invalid" | head -"${SHOW:-15}"
    exit 1
fi
echo "PASS: no false Invalid"
