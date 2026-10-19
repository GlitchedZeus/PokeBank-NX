#!/usr/bin/env bash
# Compiler wrapper for the host test build (Makefile.host.deps): runs the real compiler unchanged, then
# records which headers the output was built from, so a header-only change can never leave a stale
# test binary behind (host rules list only .cpp prerequisites; a stale binary once hid a link error
# that only CI's clean build saw).
#
#   cxx_with_deps.sh <real compiler> <args...>
#
# After a successful compile that writes `-o <out>` under the host build directory, the same compiler
# preprocesses the C/C++ sources of that command with -MM and the dependency list is written to
# <build>/.hdrdeps/<out with / replaced by %>.d (one path per line). tools/qa/host_header_sweep.py
# deletes any output older than one of its recorded files before make decides what to rebuild.
set -u
real=("$1"); shift
"${real[@]}" "$@" || exit $?

out=""; sources=(); flags=(); skip=0
for arg in "$@"; do
    if [ "$skip" = 1 ]; then out="$arg"; skip=0; continue; fi
    case "$arg" in
        -o) skip=1 ;;
        *.cpp|*.cc|*.cxx|*.c) sources+=("$arg") ;;
        -I*|-D*|-U*|-std=*|-include|-isystem*|-iquote*|-f*|-m*) flags+=("$arg") ;;
    esac
done
build="${POKEBANK_HOST_BUILD:-build-host}"
case "$out" in "$build"/*) ;; *) exit 0 ;; esac
[ "${#sources[@]}" -gt 0 ] || exit 0

depdir="$build/.hdrdeps"
mkdir -p "$depdir"
dep="$depdir/${out//\//%}.d"
# -MM: user headers only (system headers do not change between builds). Failure here never fails
# the build; without a record the sweep simply has nothing to compare.
if "${real[@]}" "${flags[@]}" -MM "${sources[@]}" 2>/dev/null \
        | tr ' \\' '\n\n' | grep -v -e ':$' -e '^$' | sort -u > "$dep.tmp"; then
    mv "$dep.tmp" "$dep"
else
    rm -f "$dep.tmp"
fi
exit 0
