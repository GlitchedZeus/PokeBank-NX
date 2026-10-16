#!/usr/bin/env bash
# Regenerate the Generation IV legality tables from a pinned PKHeX checkout.
#   tools/gen_legality_gen4_data/run.sh PKHEX_CHECKOUT [--check]
# Writes include/Legality/Gen4LegalityData.h and src/Legality/Gen4LegalityData.cpp.
# Output is deterministic; --check regenerates into a temporary directory and fails when the
# committed files differ. Needs the .NET 10 SDK. The checkout must be at the pinned commit
# (kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722).
set -euo pipefail
pkhex=$(cd "$1" && pwd); mode=${2:-}
here=$(cd "$(dirname "$0")" && pwd); repo=$(cd "$here/../.." && pwd)
work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
dotnet build "$here/Gen4DataDump.csproj" -c Release -p:PKHEX_ROOT="$pkhex" -o "$work/bin" \
    > "$work/build.log" || { cat "$work/build.log"; exit 1; }
if [ "$mode" = "--check" ]; then
    dotnet "$work/bin/Gen4DataDump.dll" "$pkhex" "$work/Gen4LegalityData.h" "$work/Gen4LegalityData.cpp"
    status=0
    cmp -s "$work/Gen4LegalityData.h" "$repo/include/Legality/Gen4LegalityData.h" || { echo "include/Legality/Gen4LegalityData.h is stale"; status=1; }
    cmp -s "$work/Gen4LegalityData.cpp" "$repo/src/Legality/Gen4LegalityData.cpp" || { echo "src/Legality/Gen4LegalityData.cpp is stale"; status=1; }
    [ "$status" = 0 ] && echo "Gen IV legality tables are up to date"
    exit "$status"
fi
dotnet "$work/bin/Gen4DataDump.dll" "$pkhex" "$repo/include/Legality/Gen4LegalityData.h" "$repo/src/Legality/Gen4LegalityData.cpp"
echo "wrote include/Legality/Gen4LegalityData.h and src/Legality/Gen4LegalityData.cpp"
