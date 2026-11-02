#!/usr/bin/env bash
# Independent structural acceptance gate. Existing production defects cause exit 1.
# Usage: audit/run.sh PKHEX_CHECKOUT OUTPUT_DIR
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../../.." && pwd)
pkhex=$(cd "$1" && pwd)
mkdir -p "$2"; work=$(cd "$2" && pwd)
test "$(git -C "$pkhex" rev-parse HEAD)" = 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
python3 -m unittest discover -s "$here" -p 'test_protocol.py' -v
python3 "$here/adversarial_corpus.py" --manifest "$work/manifest.json" > "$work/corpus.txt"
cmp "$repo/tests/audit/gen12_adversarial.corpus" "$work/corpus.txt"
cmp "$repo/tests/audit/gen12_structural_expectations.json" "$work/manifest.json"
dotnet build "$here/../oracle/PKHeXGBOracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" -o "$work/oracle"
python3 -I "$here/../gb_text.py" korean-table "$pkhex" > "$work/gb_korean_table.inc"
"${CXX:-g++}" -std=c++20 -O2 -I"$repo/include" -I"$work" \
    "$here/../gb_diff.cpp" "$repo/src/Legality/GBLegality.cpp" "$repo/src/Legality/GBLegalityData.cpp" \
    "$repo/src/Names/SpeciesNames.cpp" "$repo/src/Names/MoveNames.cpp" \
    "$repo/src/Integration/Gen1/Gen1ReadOnlySave.cpp" "$repo/src/Integration/Gen2/Gen2ReadOnlySave.cpp" \
    "$repo/src/Utils/StringHelpers.cpp" "$repo/src/Utils/HelperUtilities.cpp" "$repo/src/Pokemon/Experience.cpp" -o "$work/nx"
python3 "$here/test_native_protocol.py" "$work/nx"
python3 "$here/test_native_protocol.py" dotnet "$work/oracle/PKHeXGBOracle.dll"
dotnet "$work/oracle/PKHeXGBOracle.dll" < "$work/corpus.txt" > "$work/pkhex.txt"
"$work/nx" < "$work/corpus.txt" > "$work/nx.txt"
python3 "$here/check_structural.py" "$work/manifest.json" "$work/pkhex.txt" "$work/nx.txt" --json-out "$work/structural-results.json"
