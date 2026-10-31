#!/usr/bin/env python3
"""Run PokeBank NX generated-data tools from one reproducible entry point.

Generated source is committed to the repository. Normal builds compile those committed
files and DO NOT regenerate them. Run this tool only when deliberately refreshing
PKHeX-derived tables or fetched romfs assets.

Examples:
    python tools/regenerate.py --list
    python tools/regenerate.py --tables --repo .
    python tools/regenerate.py --assets --repo .
    python tools/regenerate.py --tables --ref <PKHeX commit> --repo .

By default the script includes this checkout plus sibling directories that look like
PokeBank NX checkouts. Use --repo . to limit a run to this repository.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
OWN_REPO = os.path.dirname(HERE)


def is_checkout(path: str) -> bool:
    required = (
        ("tools", "pkhex_source.py"),
        ("include", "Pokemon"),
        ("src", "Names"),
    )
    return all(os.path.exists(os.path.join(path, *parts)) for parts in required)


def find_repos() -> list[str]:
    found = [OWN_REPO]
    parent = os.path.dirname(OWN_REPO)
    try:
        names = sorted(os.listdir(parent))
    except OSError:
        names = []
    for name in names:
        sibling = os.path.join(parent, name)
        if os.path.isdir(sibling) and is_checkout(sibling):
            found.append(sibling)

    unique: list[str] = []
    seen: set[str] = set()
    for path in found:
        real = os.path.realpath(path)
        if real not in seen:
            seen.add(real)
            unique.append(path)
    return unique


def generators(repo: str) -> tuple[list[str], list[str]]:
    """Return (table_generators, asset_generators) discovered from tools/gen_*.py."""
    tools = os.path.join(repo, "tools")
    tables: list[str] = []
    assets: list[str] = []
    for name in sorted(os.listdir(tools)):
        if not (name.startswith("gen_") and name.endswith(".py")):
            continue
        path = os.path.join(tools, name)
        with open(path, encoding="utf-8", errors="replace") as handle:
            source = handle.read()
        # PokeBank's asset generators write beneath romfs; data generators write include/src.
        (assets if "romfs" in source else tables).append(name)
    return tables, assets


def source_fingerprint(repo: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for top in ("include", "src"):
        root_top = os.path.join(repo, top)
        for root, _dirs, names in os.walk(root_top):
            for name in names:
                if not name.endswith((".h", ".hpp", ".cpp", ".inc")):
                    continue
                path = os.path.join(root, name)
                with open(path, "rb") as handle:
                    digest = hashlib.sha256(handle.read()).hexdigest()
                result[os.path.relpath(path, repo)] = digest
    return result


def romfs_file_count(repo: str) -> int:
    root = os.path.join(repo, "romfs")
    return sum(len(names) for _root, _dirs, names in os.walk(root)) if os.path.isdir(root) else 0


def run_generator(repo: str, script: str, extra: list[str], env: dict[str, str]):
    started = time.monotonic()
    command = [sys.executable, os.path.join(repo, "tools", script), *extra]
    completed = subprocess.run(command, capture_output=True, text=True, env=env)
    tail = (completed.stderr or completed.stdout)[-2000:]
    return completed.returncode, time.monotonic() - started, tail


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--repo", action="append", metavar="PATH",
                        help="checkout to process; repeatable. Default: this checkout plus matching siblings")
    parser.add_argument("--tables", action="store_true", help="run committed data-table generators only")
    parser.add_argument("--assets", action="store_true", help="run romfs asset generators only")
    parser.add_argument("--ref", metavar="REF",
                        help="set PKHEX_REF for this run (commit/tag/branch)")
    parser.add_argument("--force", action="store_true",
                        help="pass --force to asset generators")
    parser.add_argument("--list", action="store_true",
                        help="print the discovered plan and exit without running generators")
    args = parser.parse_args()

    repos = [os.path.abspath(p) for p in args.repo] if args.repo else find_repos()
    for repo in repos:
        if not is_checkout(repo):
            parser.error("not a PokeBank NX checkout: %s" % repo)

    want_tables = args.tables or not args.assets
    want_assets = args.assets or not args.tables

    env = dict(os.environ)
    if args.ref:
        env["PKHEX_REF"] = args.ref

    plan: list[tuple[str, list[str], list[str]]] = []
    print("checkouts:")
    for repo in repos:
        print("  %s" % repo)
        tables, assets = generators(repo)
        chosen_tables = tables if want_tables else []
        chosen_assets = assets if want_assets else []
        chosen = chosen_tables + chosen_assets
        plan.append((repo, chosen_tables, chosen_assets))
        print("    %d generator(s): %s" % (len(chosen), ", ".join(chosen) if chosen else "(none)"))

    if args.list:
        return 0

    failures: list[tuple[str, str]] = []
    changed_total = 0

    for repo, table_scripts, asset_scripts in plan:
        print("\n" + "=" * 78)
        print(repo)
        print("=" * 78)
        before = source_fingerprint(repo)
        romfs_before = romfs_file_count(repo)

        for script in table_scripts + asset_scripts:
            extra = ["--force"] if args.force and script in asset_scripts else []
            code, seconds, tail = run_generator(repo, script, extra, env)
            if code == 0:
                print("  %-34s ok    %6.1fs" % (script, seconds))
            else:
                print("  %-34s FAIL  %6.1fs" % (script, seconds))
                for line in tail.strip().splitlines()[-8:]:
                    print("      | %s" % line)
                failures.append((repo, script))

        after = source_fingerprint(repo)
        touched = sorted(path for path in after if before.get(path) != after[path])
        removed = sorted(path for path in before if path not in after)
        changed_total += len(touched) + len(removed)

        print("\n  committed generated-source files changed: %d" % (len(touched) + len(removed)))
        for path in touched:
            print("    %s" % path)
        for path in removed:
            print("    removed: %s" % path)
        if want_assets:
            print("  romfs files: %d -> %d" % (romfs_before, romfs_file_count(repo)))

    print("\n" + "=" * 78)
    if failures:
        print("FAILED: %d generator(s)" % len(failures))
        for repo, script in failures:
            print("  %s in %s" % (script, repo))
        return 1

    print("every selected generator ran; %d committed source file(s) changed" % changed_total)
    if changed_total:
        print("Review the generated diff before committing it.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
