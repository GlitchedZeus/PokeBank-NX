#!/usr/bin/env bash
# Differential test: PokeBank NX's Gen I/II legality verdicts against PKHeX.Core.
#   tools/pkhex_gb_diff/run.sh PKHEX_CHECKOUT POKERED_CHECKOUT [COUNT] [SEED] [GEN]
# GEN is 1 (PK1 corpus, default) or 2 (PK2 corpus).
# Needs the .NET 10 SDK and g++. Exits non-zero when any verdict differs.
set -euo pipefail
pkhex=$(cd "$1" && pwd); pokered=$(cd "$2" && pwd); count=${3:-20000}; seed=${4:-1}; gen=${5:-1}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
corpus=gen_corpus.py; [ "$gen" = 2 ] && corpus=gen_corpus2.py
python3 -I "$here/$corpus" --pkhex "$pkhex" --pokered "$pokered" --count "$count" --seed "$seed" > "$work/corpus.txt"
dotnet build "$here/oracle/PKHeXGBOracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" -o "$work/oracle" > "$work/build.log" \
    || { cat "$work/build.log"; exit 1; }
dotnet "$work/oracle/PKHeXGBOracle.dll" < "$work/corpus.txt" > "$work/pkhex.txt"
python3 -I "$here/gb_text.py" korean-table "$pkhex" > "$work/gb_korean_table.inc"
g++ -std=c++20 -O2 -I"$repo/include" -I"$work" "$here/gb_diff.cpp" "$repo/src/Legality/GBLegality.cpp" \
    "$repo/src/Legality/GBLegalityData.cpp" "$repo/src/Names/SpeciesNames.cpp" "$repo/src/Names/MoveNames.cpp" \
    "$repo/src/Integration/Gen1/Gen1ReadOnlySave.cpp" "$repo/src/Integration/Gen2/Gen2ReadOnlySave.cpp" \
    "$repo/src/Utils/StringHelpers.cpp" "$repo/src/Utils/HelperUtilities.cpp" "$repo/src/Pokemon/Experience.cpp" -o "$work/gb_diff"
"$work/gb_diff" < "$work/corpus.txt" > "$work/pokebank.txt"
python3 -I "$here/compare.py" --self-test > "$work/selftest.txt" || { cat "$work/selftest.txt"; exit 1; }
python3 -I "$here/compare.py" "$work/corpus.txt" "$work/pkhex.txt" "$work/pokebank.txt" \
    --pkhex "$pkhex" --pokered "$pokered" --show "${SHOW:-8}"
# The same corpus through the app's path (PokemonRecord -> Pokemon1/2ReadOnly -> Legality::analyze)
# must give the origin search's verdict; see production_parity.cpp.
srcs=(src/Legality/Legality.cpp src/Legality/GBLegality.cpp src/Legality/GBLegalityData.cpp
    src/Integration/Gen1/Gen1MoveCompatibility.cpp src/Integration/Gen2/Gen2MoveCompatibility.cpp
    src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp
    src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/Pokemon1ReadOnly.cpp src/Pokemon/Pokemon2ReadOnly.cpp
    src/Pokemon/PersonalRecord4.cpp src/Pokemon/PersonalInfo4DP.cpp src/Pokemon/PersonalInfo4PT.cpp
    src/Pokemon/PersonalInfo4HGSS.cpp src/Integration/Gen2/Gen2PersonalData.cpp
    src/Encryption/Encryption3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp
    src/Pokemon/Experience.cpp src/Pokemon/AbilityInfo.cpp src/Pokemon/FormInfo.cpp
    src/Pokemon/LearnsetTable.cpp src/Names/MoveNames.cpp src/Names/ItemNames.cpp
    src/Names/SpeciesNames.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp
    src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp)
g++ -std=c++20 -O2 -Wno-unused-parameter -Wno-return-type -I"$repo/include" -I"$work" -I"$here" "$here/production_parity.cpp" \
    "${srcs[@]/#/$repo/}" -o "$work/production_parity"
"$work/production_parity" < "$work/corpus.txt"
