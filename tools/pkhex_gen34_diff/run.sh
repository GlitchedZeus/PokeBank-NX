#!/usr/bin/env bash
# Differential test: PokeBank NX's Gen III (PK3) and Gen IV (PK4) legality verdicts against PKHeX.Core.
#   tools/pkhex_gen34_diff/run.sh PKHEX_CHECKOUT [SEED] [MUTATIONS] [GENS] [CORPUS]
# GENS is 3, 4 or 34 (default). The oracle builds every Gen III encounter PKHeX knows
# (species 1-386; R/S/E/FR/LG; Colosseum/XD origins skipped) and every Gen IV encounter
# (species 1-493; D/P/Pt/HG/SS), all forms, plus MUTATIONS seeded single-field mutations
# of each. CORPUS "grown" (Gen IV) replaces those with PKHeX-valid records played forward
# (levels, moves, EVs, evolution, items, trades) and Pal Park transfers of PKHeX-valid
# Gen III records, plus MUTATIONS mutations of each (see oracle/Program.cs).
# The C++ driver loads each record through Pokemon3FRLG / Pokemon4ReadOnlyView
# and runs Legality::analyze with the cartridge's save group and exact source game id.
# Needs the .NET 10 SDK, g++ and python3. Set WORK=dir to keep the intermediate files
# (pkhex.txt, pokebank.txt, joined.txt); SHOW=n sets categories per section.
# Exits non-zero when PokeBank NX reports Invalid for a Pokemon PKHeX accepts (a false
# Invalid), reports No problems found for a Pokemon PKHeX rejects (a false Valid), or a
# record cannot be loaded.
set -euo pipefail
pkhex=$(cd "$1" && pwd); seed=${2:-1}; mutations=${3:-3}; gens=${4:-34}; corpus=${5:-base}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "${REPO:-$here/../..}" && pwd)  # REPO=checkout to analyze
if [ -n "${WORK:-}" ]; then work=$WORK; mkdir -p "$work"; else work=$(mktemp -d); trap 'rm -rf "$work"' EXIT; fi
dotnet build "$here/oracle/PKHeXGen34Oracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" -o "$work/oracle" > "$work/build.log" || { cat "$work/build.log"; exit 1; }
dotnet "$work/oracle/PKHeXGen34Oracle.dll" generate "$seed" "$mutations" "$gens" "$corpus" > "$work/pkhex.txt"
# Same production sources as GEN34_LANGUAGE_ANALYZER_SOURCES (Makefile.host.legality-language).
srcs=(src/Legality/Legality.cpp src/Legality/GBLegality.cpp src/Legality/GBLegalityData.cpp
    src/Legality/Gen4Legality.cpp src/Legality/Gen4LegalityHook.cpp src/Legality/Gen4LegalityEncounters.cpp
    src/Legality/Gen4LegalityMoves.cpp src/Legality/Gen4LegalityRng.cpp src/Legality/Gen4LegalityText.cpp
    src/Legality/Gen4LegalityData.cpp
    src/Integration/Gen1/Gen1MoveCompatibility.cpp src/Integration/Gen2/Gen2MoveCompatibility.cpp
    src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/Pokemon1ReadOnly.cpp src/Pokemon/Pokemon2ReadOnly.cpp
    src/Pokemon/PersonalRecord4.cpp src/Pokemon/PersonalInfo4DP.cpp src/Pokemon/PersonalInfo4PT.cpp
    src/Pokemon/PersonalInfo4HGSS.cpp src/Integration/Gen2/Gen2PersonalData.cpp
    src/Encryption/Encryption3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp
    src/Pokemon/Experience.cpp src/Pokemon/AbilityInfo.cpp src/Pokemon/FormInfo.cpp
    src/Pokemon/LearnsetTable.cpp src/Names/MoveNames.cpp src/Names/ItemNames.cpp
    src/Names/SpeciesNames.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp
    src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
    src/Pokemon/Pokemon4ReadOnly.cpp src/Pokemon/Pokemon4ReadOnlyView.cpp src/Encryption/Encryption4.cpp)
g++ -std=c++20 -O2 -Wno-unused-parameter -I"$repo/include" "$here/gen34_diff.cpp" "${srcs[@]/#/$repo/}" -o "$work/gen34_diff"
"$work/gen34_diff" < "$work/pkhex.txt" > "$work/pokebank.txt"
test "$(wc -l < "$work/pkhex.txt")" = "$(wc -l < "$work/pokebank.txt")"
paste -d'\t' "$work/pkhex.txt" "$work/pokebank.txt" > "$work/joined.txt"
echo "seed $seed, $mutations mutations per encounter, generations $gens, corpus $corpus"
if python3 "$here/summarize.py" "$work/joined.txt" "${SHOW:-12}"; then
    echo "PASS: no false Invalid and no false Valid"
else
    echo "FAIL: false Invalid, false Valid or unloadable records (see above)"; exit 1
fi
