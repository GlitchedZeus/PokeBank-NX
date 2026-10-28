#!/usr/bin/env bash
# Differential test: PokeBank NX's Gen V (PK5) legality verdicts against PKHeX.Core.
#   tools/pkhex_gen5_diff/run.sh PKHEX_CHECKOUT [SEED] [MUTATIONS]
# The oracle (oracle/Program.cs) builds every Gen V encounter PKHeX knows (species
# 1-649, all forms, B/W/B2/W2), every in-game trade in every language, every PGF card
# in PKHeX's Mystery Gift database, Gen III/IV encounters converted to PK5 the way Pal
# Park and Poke Transfer do, and "grown" copies (levelled, moves PKHeX says are
# learnable, EVs, sometimes evolved), plus MUTATIONS single-field mutations of each.
# Needs the .NET 10 SDK and g++.
#
# Fails when
#   - PokeBank NX reports Invalid for a Pokemon PKHeX accepts (a false Invalid),
#   - PokeBank NX reports "No problems found" for a Pokemon PKHeX rejects (a false Valid),
#   - a record gets no verdict,
#   - the text helpers (invariant casing, word-filter patterns and verdicts) disagree
#     with .NET / PKHeX, or
#   - a name a Gen III/IV encounter fixes is missing from the trade-name table the
#     transfer name rules use.
# Records PKHeX rejects that PokeBank NX only reports as Incomplete are listed, not
# failed: Unknown must never be turned into Invalid.
#
# Environment:
#   GEN5_DIFF_CORPUS=FILE  reuse a saved oracle corpus instead of generating one (PKHeX
#                          converts some records with an unseeded random source, so two
#                          runs of one seed differ slightly; reuse a corpus to compare).
#   GEN5_DIFF_KEEP=DIR     keep the corpus and both verdict files in DIR.
#   WORK=DIR              retain the complete run, including secondary-reference
#                          reports and source-built assemblies, even on failure.
#   SHOW=N                 lines of each listing (default 15).
#   GEN5_ORACLE_TRACE=1    the oracle writes each conversion / analysis step to stderr.
set -euo pipefail
pkhex=$(cd "$1" && pwd); seed=${2:-1}; mutations=${3:-4}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
if [ -n "${WORK:-}" ]; then work=$WORK; mkdir -p "$work"; else work=$(mktemp -d); trap 'rm -rf "$work"' EXIT; fi
dotnet build "$here/oracle/PKHeXGen5Oracle.csproj" -c Release -p:PKHEX_ROOT="$pkhex" \
    -p:UseArtifactsOutput=true -p:ArtifactsPath="$work/artifacts" -o "$work/oracle" > "$work/build.log" || { cat "$work/build.log"; exit 1; }
if [ -n "${GEN5_DIFF_CORPUS:-}" ]; then
    cp "$GEN5_DIFF_CORPUS" "$work/pkhex.txt"
else
    dotnet "$work/oracle/PKHeXGen5Oracle.dll" generate "$seed" "$mutations" > "$work/pkhex.txt" 2> "$work/generation-audit.jsonl"
fi
# Gen III/IV-origin records are checked by the Gen IV analyzer's origin analysis
# (Gen4::analyzeOrigin5), so its sources are part of the Gen V analyzer.
g++ -std=c++20 -O2 -I"$repo/include" "$here/gen5_diff.cpp" "$repo/src/Legality/Gen5Legality.cpp" \
    "$repo/src/Legality/Gen35BreedingAncestry.cpp" \
    "$repo/src/Legality/Gen5LegalityData.cpp" "$repo/src/Legality/Gen5Text.cpp" \
    "$repo/src/Legality/Gen4Legality.cpp" "$repo/src/Legality/Gen4LegalityData.cpp" \
    "$repo/src/Legality/Gen4LegalityEncounters.cpp" "$repo/src/Legality/Gen4LegalityMoves.cpp" \
    "$repo/src/Legality/Gen4LegalityPalPark.cpp" "$repo/src/Legality/Gen4LegalityRng.cpp" \
    "$repo/src/Legality/Gen4LegalityText.cpp" "$repo/src/Legality/Gen3Legality.cpp" \
    "$repo/src/Legality/Gen3LegalityData.cpp" "$repo/src/Legality/Gen3LegalityDataCxd.cpp" \
    "$repo/src/Names/SpeciesNames.cpp" \
    "$repo/src/Names/MoveNames.cpp" "$repo/src/Pokemon/Experience.cpp" -o "$work/gen5_diff"

# Text helpers against .NET and PKHeX: invariant casing of every UTF-16 unit inside the
# analyzer's text domain, the lower-cased word-filter patterns, and filter verdicts.
dotnet "$work/oracle/PKHeXGen5Oracle.dll" casemap > "$work/casemap.txt"
dotnet "$work/oracle/PKHeXGen5Oracle.dll" patterns > "$work/patterns.txt"
dotnet "$work/oracle/PKHeXGen5Oracle.dll" wordfilter "$seed" > "$work/wordfilter.txt"
"$work/gen5_diff" casemap < "$work/casemap.txt"
"$work/gen5_diff" patterns < "$work/patterns.txt"
"$work/gen5_diff" wordfilter < "$work/wordfilter.txt"
# The transfer name rules rely on every name a Gen III/IV encounter fixes being in the
# generated trade-name table (see fixednames() in gen5_diff.cpp).
dotnet "$work/oracle/PKHeXGen5Oracle.dll" fixednames > "$work/fixednames.txt"
"$work/gen5_diff" fixednames < "$work/fixednames.txt"

"$work/gen5_diff" < "$work/pkhex.txt" > "$work/pokebank.txt"
test "$(wc -l < "$work/pkhex.txt")" = "$(wc -l < "$work/pokebank.txt")"
paste -d'\t' "$work/pkhex.txt" "$work/pokebank.txt" > "$work/joined.txt"
# The older corpus stays frozen. Re-evaluate every raw stricter row against the
# current official release without changing bytes or excluding the old result.
if [ -n "${PKHEX_RELEASE_ROOT:-}" ]; then
    python3 "$repo/tools/legality_completion/current_release_replay.py" \
      --group gen5 --joined "$work/joined.txt" --source "$PKHEX_RELEASE_ROOT" \
      --work "$work/current-release"
    export CURRENT_RELEASE_REPLAY_REPORT="$work/current-release/REPORT.json"
fi
if [ -n "${GEN5_DIFF_KEEP:-}" ]; then
    mkdir -p "$GEN5_DIFF_KEEP"
    if ! [ "$GEN5_DIFF_KEEP" -ef "$work" ]; then
        cp "$work/pkhex.txt" "$work/pokebank.txt" "$work/joined.txt" "$GEN5_DIFF_KEEP/"
        if [ -f "$work/generation-audit.jsonl" ]; then cp "$work/generation-audit.jsonl" "$GEN5_DIFF_KEEP/"; fi
        if [ -d "$work/current-release" ]; then cp -R "$work/current-release" "$GEN5_DIFF_KEEP/"; fi
    fi
fi
echo "seed $seed, $mutations mutations per record: PKHeX verdict / PokeBank NX verdict"
awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[1]!=b[1]) {print "id mismatch " a[1]; exit 1} c[a[3]"/"b[2]]++}
    END{for(k in c) print "  " c[k], k}' "$work/joined.txt" | sort -k2
echo "PKHeX-valid records PokeBank NX certifies \"No problems found\" (origin, game):"
awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[3]!="VALID") next; o=a[5]; g=a[7];
    t[o" "g]++; t[o" all"]++; t["all all"]++; if (b[2]=="NOPROBLEMS") {n[o" "g]++; n[o" all"]++; n["all all"]++}}
    END{for(k in t) printf "  %-22s %6d / %6d  %5.1f%%\n", k, n[k], t[k], 100*n[k]/t[k]}' "$work/joined.txt" | sort
echo "PKHeX-invalid records PokeBank NX reports Invalid (origin, game):"
awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[3]!="INVALID") next; o=a[5]; g=a[7];
    t[o" "g]++; t[o" all"]++; t["all all"]++; if (b[2]=="INVALID") {n[o" "g]++; n[o" all"]++; n["all all"]++}}
    END{for(k in t) printf "  %-22s %6d / %6d  %5.1f%%\n", k, n[k], t[k], 100*n[k]/t[k]}' "$work/joined.txt" | sort
# A false Valid ("No problems found" for a record PKHeX rejects) fails the run.
false_valid=$(awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[3]=="INVALID" && b[2]=="NOPROBLEMS") print}' "$work/joined.txt")
echo "false Valid (PKHeX INVALID, PokeBank NX No problems found): $(printf '%s' "$false_valid" | grep -c . || true)"
if [ -n "$false_valid" ]; then
    echo "FALSE VALID:"
    printf '%s\n' "$false_valid" | awk -v n="${SHOW:-15}" 'NR <= n'
    exit 1
fi
echo "PKHeX rejects, PokeBank NX Incomplete (by mutation and PKHeX result):"
awk -F'\t' '{split($1,a," "); split($2,b," "); if (a[3]=="INVALID" && b[2]!="INVALID") {m=a[1]; sub(/^m[0-9]+-/,"",m); sub(/^b[0-9]+$/,"base",m); r=$1; sub(/^[^;]*; /,"",r); print m" :: "r}}' \
    "$work/joined.txt" | sort | uniq -c | sort -rn | awk -v n="${SHOW:-15}" 'NR <= n'
# A stricter result is explained only by independent native level absence or
# the unchanged bytes being Invalid under the explicitly pinned current release.
# Discarded-placeholder membership alone is insufficient; raw older results stay.
false_invalid=$(python3 "$repo/tools/pkhex_gen34_diff/hgss_placeholders.py" gen5 "$work/joined.txt")
if [ -n "$false_invalid" ]; then
    echo "FALSE INVALID (PKHeX VALID, PokeBank NX INVALID):"
    printf '%s\n' "$false_invalid" | awk -v n="${SHOW:-15}" 'NR <= n'
    exit 1
fi
echo "PASS: no false Invalid"
