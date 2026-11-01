#!/usr/bin/env python3
"""Replay only the published BW2 EV-item helper; no ROM-equivalence claim.

The source-cache directory must contain the exact blobs pinned by
BW2_WING_SOURCE_AUDIT.json. The output directory must be outside old seals.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--cxx", default="c++")
    parser.add_argument("--fetch", action="store_true", help="Fetch only missing exact pinned original source blobs")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    manifest = json.loads((root / "docs/legality/major-20261031/BW2_WING_SOURCE_AUDIT.json").read_text())
    for row in manifest["files"]:
        target = args.source / row["path"]
        if args.fetch and not target.exists():
            url = "https://raw.githubusercontent.com/" + manifest["repository"] + "/" + manifest["commit"] + "/" + row["path"]
            data = urllib.request.urlopen(url, timeout=60).read()
            assert hashlib.sha256(data).hexdigest() == row["sha256"], row["path"]
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        data = target.read_bytes()
        assert hashlib.sha256(data).hexdigest() == row["sha256"], row["path"]
        assert hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest() == row["git_blob"], row["path"]
    stats = ["hp", "attack", "defense", "speed", "sp_attack", "sp_defense"]
    names = ["health_wing", "muscle_wing", "resist_wing", "swift_wing", "genius_wing", "clever_wing"]
    berries = ["pomeg_berry", "kelpsy_berry", "qualot_berry", "tamato_berry", "hondew_berry", "grepa_berry"]
    for i, (wing, berry) in enumerate(zip(names, berries)):
        for name, delta in [(wing, 1), (berry, -10)]:
            effects = json.loads((args.source / "res/items" / name / "data.json").read_text())["effects"]
            assert [effects[s + "_ev"] for s in stats] == [delta if j == i else 0 for j in range(6)]
            assert effects[stats[i] + "_ev_up"]
            assert not effects["level_up"] and not effects["evolve"]
            if delta > 0:
                # Both fields are checked because the published dispatcher
                # has the48/51 parameter mismatch disclosed in the manifest.
                assert effects["unk6_4"] and effects["friendship_up_1"]
    source = (args.source / "src/ov165/status_rcv.c").read_text()
    signature = "static s32 StatusRcv_AddEffort(s32 effort, s32 total, s32 add, s32 noLimit) {"
    start = source.index(signature)
    end = source.index("\n}", start) + 2
    body = source[start:end]
    assert hashlib.sha256(body.encode()).hexdigest() == manifest["native_body_sha256"]
    assert "add > 0 && PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_SHEDINJA" in source
    args.out.mkdir(parents=True, exist_ok=True)
    cpp = args.out / "wings_native_helper.cpp"
    cpp.write_text("#include <cassert>\n#include <iostream>\nusing s32=int;\n" + body + "\n" + MAIN)
    binary = args.out / "wings_native_helper"
    subprocess.run([args.cxx, "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


MAIN = r'''int main() {
    unsigned assertions = 0;
    // Every initial stat can be cleared by native berries.
    for (int initial = 0; initial <= 255; ++initial) {
        int value = initial;
        while (value) {
            const int next = StatusRcv_AddEffort(value, 0, -10, 0);
            assert(next >= 0 && next < value);
            value = next;
            ++assertions;
        }
    }
    // Every target stat can then be reached without EXP. Each prefix is an
    // actual sequence; repeating the same path per initial stat adds no evidence.
    int actual = 0;
    for (int target = 1; target <= 255; ++target) {
        actual = StatusRcv_AddEffort(actual, 0, 1, 1);
        assert(actual == target);
        ++assertions;
    }
    // The helper's total argument excludes the stat being modified.
    for (int other = 0; other <= 510; ++other)
        for (int stat = 0; stat <= 255 && stat + other <= 510; ++stat) {
            const int next = StatusRcv_AddEffort(stat, other, 1, 1);
            if (stat == 255 || stat + other == 510) assert(next == -1);
            else assert(next == stat + 1);
            ++assertions;
        }
    assert(StatusRcv_AddEffort(100, 0, 1, 0) == -1);
    assert(StatusRcv_AddEffort(100, 0, 1, 1) == 101);
    assert(StatusRcv_AddEffort(255, 0, 1, 1) == -1);
    std::cout << assertions << " native helper assertions PASS; full ROM/item dispatcher validation remains separate\n";
}
'''

if __name__ == "__main__":
    run()
