#!/usr/bin/env python3
"""Fail-closed original RBY Gen I 151-species/55-machine source verification.
Use exact pinned original pret Git checkouts; no retail game history implied.
"""
import argparse
import json
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
META = ROOT / "docs/legality/research/l3/GEN1_RBY_MACHINE_SOURCE_BLOBS.json"
HEADER = ROOT / "include/Legality/Gen1OriginalMachineTables.h"
GAMES = (("redblue", "pret/pokered"), ("yellow", "pret/pokeyellow"))

def git(repo, *args):
    return subprocess.check_output(("git", "-C", str(repo), *args), text=True).strip()

def main():
    cli = argparse.ArgumentParser()
    cli.add_argument("--pokered", type=pathlib.Path, required=True)
    cli.add_argument("--pokeyellow", type=pathlib.Path, required=True)
    a = cli.parse_args()
    meta = json.loads(META.read_text())
    repos = (a.pokered, a.pokeyellow)
    expected_heads = (meta["redblue_commit"], meta["yellow_commit"])
    source = HEADER.read_text()
    names = []
    original_mask_sets = []
    for n, folder in enumerate(repos):
        assert git(folder, "rev-parse", "HEAD") == expected_heads[n], "Wrong original game commit"
        base = (folder / "data/pokemon/base_stats.asm").read_text()
        paths = re.findall(r'^INCLUDE "([^"]+)"$', base, re.M)
        mew = "data/pokemon/base_stats/mew.asm"
        if mew not in paths:
            paths.append(mew)  # pokered: Mew is a separate last record
        assert len(paths) == 151 and len(set(paths)) == 151
        expected = meta[("redblue", "yellow")[n] + "_blobs"]
        assert len(expected) == 151
        constants = (folder / "constants/item_constants.asm").read_text()
        tms = re.findall(r"^\s*add_tm\s+([A-Z0-9_]+)\b", constants, re.M)
        hms = re.findall(r"^\s*add_hm\s+([A-Z0-9_]+)\b", constants, re.M)
        assert len(tms) == 50 and len(hms) == 5
        original_names = tms + hms
        if not names:
            names = original_names
        assert names == original_names
        machine_index = {v: i for i, v in enumerate(names)}
        masks = []
        for i, name in enumerate(paths):
            assert git(folder, "rev-parse", "HEAD:" + name) == expected[i], (i + 1, name, "blob changed")
            content = (folder / name).read_text()
            match = re.search(r";\s*tm/hm learnset\s*\n\s*tmhm\s+([\s\S]*?)\s*;\s*end", content, re.I)
            dex = re.search(r"\bdb\s+DEX_([A-Z0-9_]+)", content)
            assert match and dex, (i + 1, name, "missing original learnset")
            tokens = [v for v in re.split(r"[,\s]+", match[1].replace("\\", " ")) if v]
            allowed = 0
            for token in tokens:
                if token == "UNUSED":  # Red/Blue Mew's unused 56th bit
                    continue
                assert token in machine_index, (i + 1, token, "unsupported original move")
                bit = 1 << machine_index[token]
                assert not (allowed & bit), (i + 1, token, "duplicate machine")
                allowed |= bit
            masks.append(allowed)
        original_mask_sets.append(masks)
    def parse_table(name):
        match = re.search(r"inline constexpr std::array<std::uint64_t,152> " + name
                          + r"\{\{([\s\S]*?)\}\};", source)
        assert match, "Original packed table missing: " + name
        words = re.findall(r"(?:0x[0-9a-f]+|0)ULL", match[1])
        assert len(words) == 152
        return [int(word[:-3], 0) for word in words]
    assert parse_table("kRB") == [0] + original_mask_sets[0], "R/B original table drift"
    assert parse_table("kYellow") == [0] + original_mask_sets[1], "Yellow original table drift"
    diffs = sum(a != b for a, b in zip(*original_mask_sets))
    counts = [sum(v.bit_count() for v in rows) for rows in original_mask_sets]
    assert counts == [3037, 3043] and diffs == 8, "original machine statistics drift"
    assert sum(a != b for a, b in zip(meta["redblue_blobs"], meta["yellow_blobs"])) == 23
    print("Gen I original 151 species x 55 machines: Red/Blue 3037, Yellow 3043, 8 version differences: PASS")
    print("Original source/metadata verified, NOT original trainer machine receipts or PKHeX parity.")

if __name__ == "__main__":
    main()
