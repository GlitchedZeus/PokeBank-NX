#!/usr/bin/env python3
"""Executable regression for tools/qa/nro_audit.py (Thread 4 QA).

Builds synthetic Switch-format NROs (reusing the RomFS/NRO builders of tests/test_verify_nro_romfs.py)
with an ASET NACP and an embedded POKEVAULT_GIT_COMMIT string, then proves the audit:
  * passes a complete package built from the expected head and prints size / SHA-256 / identity,
  * fails when the expected commit is not embedded (wrong head),
  * fails when the CI artifact copy differs from the audited NRO, and accepts an identical zip copy,
  * fails when an asset that the application source requests is absent, even though the older
    required-asset manifest would still pass,
  * always writes a hardware record whose status is NOT DEVICE TESTED.
"""
from __future__ import annotations

import hashlib
import struct
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tools" / "qa"))
sys.path.insert(0, str(ROOT / "tests"))

import nro_audit  # noqa: E402
import test_verify_nro_romfs as nro_fixture  # noqa: E402  (synthetic RomFS + NRO builders)
import verify_nro_romfs as v  # noqa: E402

SHA = "0123456789abcdef0123456789abcdef01234567"
FAILURES: list[str] = []


def check(condition: bool, label: str) -> None:
    print(("PASS  " if condition else "FAIL  ") + label)
    if not condition:
        FAILURES.append(label)


def build_nro(files: dict[str, bytes], commit: str, title: str = "PokeBank NX",
              version: str = "0.1.0-alpha") -> bytes:
    romfs = nro_fixture.build_romfs(files)
    body = bytearray(0x1000)
    body[0x10:0x14] = b"NRO0"
    struct.pack_into("<I", body, 0x18, len(body))
    marker = commit.encode() + b"\0"
    body[0x200:0x200 + len(marker)] = marker  # where the linker would place the rodata string
    nacp = bytearray(nro_audit.NACP_SIZE)
    nacp[0:len(title)] = title.encode()
    nacp[nro_audit.NACP_DISPLAY_VERSION:nro_audit.NACP_DISPLAY_VERSION + len(version)] = version.encode()
    aset = bytearray(0x38)
    aset[0:4] = b"ASET"
    nacp_off = len(aset)
    romfs_off = nacp_off + len(nacp)
    struct.pack_into("<QQ", aset, nro_audit.ASET_NACP, nacp_off, len(nacp))
    struct.pack_into("<QQ", aset, 0x28, romfs_off, len(romfs))
    return bytes(body) + bytes(aset) + bytes(nacp) + romfs


def main() -> int:
    files = nro_fixture.complete_tree()
    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)
        local = tmp_path / "romfs"
        for rel, data in files.items():
            (local / rel).parent.mkdir(parents=True, exist_ok=True)
            (local / rel).write_bytes(data)
        src = tmp_path / "src"
        src.mkdir()
        (src / "Art.cpp").write_text('auto a = "romfs:/game_cards/red_gb.png";\n'
                                     'auto fmt = "romfs:/sprites/pokemon_hd/%d.png";\n'
                                     'auto dir = "romfs:/fonts/";\n')

        good = tmp_path / "PokeBankNX.nro"
        good.write_bytes(build_nro(files, SHA[:8]))
        result = nro_audit.audit(good, SHA, romfs_dir=local, source_roots=[src])
        check(not result.errors, f"complete package from the expected head passes ({result.errors})")
        check(result.size == good.stat().st_size, "size reported")
        check(result.sha256 == hashlib.sha256(good.read_bytes()).hexdigest(), "SHA-256 reported")
        check(result.code_sha == SHA and result.embedded_commit.startswith(SHA[:8]), "embedded commit found")
        check(result.nacp_title == "PokeBank NX" and result.nacp_version == "0.1.0-alpha", "NACP read")
        check(result.romfs_files == len(files), "packaged RomFS walked")
        record = nro_audit.hardware_record(result)
        check("**Status: NOT DEVICE TESTED**" in record and result.sha256 in record,
              "hardware record carries NOT DEVICE TESTED and the exact SHA-256")
        rows = [r for r in record.split("Owner checklist")[1].split("Owner:")[0].splitlines()
                if r.startswith("| ") and not r.startswith("| Area") and not r.startswith("| ---")]
        check(rows and all("| NOT RUN |" in r for r in rows), "hardware record never pre-fills an owner result")

        wrong = nro_audit.audit(good, "f" * 40, romfs_dir=local, source_roots=[src])
        check(any("does not embed commit" in e for e in wrong.errors), "wrong head is rejected")
        check(any("must be a full" in e for e in nro_audit.audit(good, "abc", romfs_dir=local,
                                                                 source_roots=[src]).errors),
              "abbreviated expected SHA is refused")
        check(any("NOT established" in n for n in nro_audit.audit(good, romfs_dir=local,
                                                                  source_roots=[src]).notes),
              "missing --expect-sha is reported as identity not established")

        artifact = tmp_path / "ci-artifact.zip"
        with zipfile.ZipFile(artifact, "w") as z:
            z.writestr("PokeBankNX.nro", good.read_bytes())
            z.writestr("native-pr-build.log", "log")
        ok = nro_audit.audit(good, SHA, artifact=artifact, romfs_dir=local, source_roots=[src])
        check(not ok.errors and ok.artifact.endswith("PokeBankNX.nro"), "identical CI artifact copy accepted")
        other = tmp_path / "other.zip"
        with zipfile.ZipFile(other, "w") as z:
            z.writestr("PokeBankNX.nro", good.read_bytes() + b"\0")
        diff = nro_audit.audit(good, SHA, artifact=other, romfs_dir=local, source_roots=[src])
        check(any("differs from the CI artifact" in e for e in diff.errors), "different CI artifact copy rejected")

        (src / "Gen5.cpp").write_text('auto b = "romfs:/game_cards/black_nds.png";\n')
        missing = nro_audit.audit(good, SHA, romfs_dir=local, source_roots=[src])
        check(not v.verify(v.walk_romfs(v.extract_romfs(good.read_bytes())), v.local_tree(local),
                           v.required_paths()), "older manifest alone still passes (the gap)")
        check(any("requested by the app source are absent" in e and "black_nds" in e for e in missing.errors),
              "source-referenced asset absent from the package is rejected")
        check(any("not in verify_nro_romfs.required_paths()" in n for n in missing.notes),
              "manifest gap is reported")

        broken = tmp_path / "broken.nro"
        broken.write_bytes(good.read_bytes()[:0x1010])
        check(bool(nro_audit.audit(broken, SHA, romfs_dir=local, source_roots=[src]).errors),
              "truncated NRO fails cleanly")
        print("--- CLI run on the synthetic package (expected to report the real source's Gen V cards as absent) ---")
        out = tmp_path / "record.md"
        code = nro_audit.main([str(good), "--expect-sha", SHA, "--romfs", str(local), "--record", str(out)])
        check(code in (0, 1) and "NOT DEVICE TESTED" in out.read_text(), "CLI always writes the record")

    if FAILURES:
        print(f"{len(FAILURES)} NRO audit regression(s) failed")
        return 1
    print("NRO audit regression: all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
