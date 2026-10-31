#!/usr/bin/env python3
"""Exact-head audit of a PokeBank NX .nro before it is handed to the owner for hardware testing.

Given an NRO (and optionally the CI artifact it came from), this tool prints the identity record a
hardware round needs and fails when the package cannot be trusted:

  * NRO size and SHA-256; when a CI artifact (.zip or directory) is also given, the NRO inside it
    must be byte-identical to the one being audited;
  * code SHA and tree SHA of the expected head (from --expect-sha, resolved with git when the
    commit is available locally), and whether the build embedded that commit: the Makefile bakes
    `git rev-parse --short=8 HEAD` into the binary as POKEVAULT_GIT_COMMIT;
  * NACP title and display version (switchbrew NRO/ASET + NACP layouts);
  * the packaged RomFS, verified with tools/verify_nro_romfs.py (reused, not duplicated) against the
    local romfs/ tree and its required-asset manifest, PLUS every `romfs:/...` asset path that the
    application source itself requests (so a newly referenced asset, e.g. a Gen V game card, cannot
    silently go missing while the older manifest still passes);
  * a hardware-test record template (Markdown) whose status is always NOT DEVICE TESTED: only the
    owner can change that, for this exact file.

stdlib only; read-only for the NRO, the artifact and the repository. Usage:
    python3 tools/qa/nro_audit.py PokeBankNX.nro --expect-sha <40-hex> \\
        [--artifact ci-artifact.zip] [--ci-run <url-or-id>] [--romfs romfs] [--record out.md]
"""
from __future__ import annotations

import argparse
import hashlib
import re
import struct
import subprocess
import sys
import zipfile
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import verify_nro_romfs as romfs_check  # noqa: E402  (reused verifier: ASET/RomFS walk + manifest)

# switchbrew "NRO": ASET header = magic, version, then {offset, size} u64 pairs for icon / NACP / RomFS.
ASET_NACP = 0x18
# switchbrew "NACP": 16 language entries of {name[0x200], publisher[0x100]}; DisplayVersion at 0x3060.
NACP_SIZE = 0x4000
NACP_DISPLAY_VERSION = 0x3060
ROMFS_LITERAL = re.compile(rb'"romfs:/([A-Za-z0-9_./%-]+)"')


@dataclass
class Audit:
    nro_path: str
    size: int = 0
    sha256: str = ""
    code_sha: str = ""
    tree_sha: str = ""
    embedded_commit: str = "not checked"
    nacp_title: str = ""
    nacp_version: str = ""
    romfs_files: int = 0
    romfs_digest: str = ""
    artifact: str = ""
    ci_run: str = ""
    errors: list[str] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


def _cstr(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("utf-8", "replace")


def read_nacp(nro: bytes) -> tuple[str, str]:
    """Return (title, display version) from the NRO's ASET NACP, or raise RomfsError."""
    body_size = romfs_check._u32(nro, 0x18)
    if nro[body_size:body_size + 4] != b"ASET":
        raise romfs_check.RomfsError("NRO has no ASET section")
    off = romfs_check._u64(nro, body_size + ASET_NACP)
    size = romfs_check._u64(nro, body_size + ASET_NACP + 8)
    if size == 0:
        return "", ""
    if size < NACP_SIZE or body_size + off + size > len(nro):
        raise romfs_check.RomfsError("NRO NACP is truncated or out of bounds")
    nacp = nro[body_size + off:body_size + off + size]
    title = next((_cstr(nacp[i * 0x300:i * 0x300 + 0x200]) for i in range(16) if nacp[i * 0x300]), "")
    return title, _cstr(nacp[NACP_DISPLAY_VERSION:NACP_DISPLAY_VERSION + 0x10])


def nro_from_artifact(artifact: Path) -> tuple[str, bytes]:
    """Return (member name, bytes) of the single .nro inside a CI artifact zip or directory."""
    if artifact.is_dir():
        members = sorted(p for p in artifact.rglob("*.nro") if p.is_file())
        if len(members) != 1:
            raise ValueError(f"artifact directory must hold exactly one .nro, found {len(members)}")
        return members[0].relative_to(artifact).as_posix(), members[0].read_bytes()
    with zipfile.ZipFile(artifact) as archive:
        names = [n for n in archive.namelist() if n.lower().endswith(".nro") and not n.endswith("/")]
        if len(names) != 1:
            raise ValueError(f"artifact zip must hold exactly one .nro, found {len(names)}")
        return names[0], archive.read(names[0])


def git(*args: str) -> str:
    try:
        return subprocess.run(["git", "-C", str(ROOT), *args], check=True, capture_output=True,
                              text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return ""


def referenced_romfs_paths(source_roots: list[Path]) -> set[str]:
    """Every literal "romfs:/..." asset path in the application source (format strings excluded)."""
    found: set[str] = set()
    for root in source_roots:
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix not in {".cpp", ".h", ".hpp", ".inc", ".c"} or not path.is_file():
                continue
            for match in ROMFS_LITERAL.finditer(path.read_bytes()):
                rel = match.group(1).decode()
                if "%" in rel or rel.endswith("/"):
                    continue  # printf-style templates and directory prefixes are not single assets
                found.add(rel)
    return found


def audit(nro_path: Path, expect_sha: str = "", artifact: Path | None = None, ci_run: str = "",
          romfs_dir: Path | None = None, source_roots: list[Path] | None = None,
          expect_nro_sha256: str = "", expect_size: int | None = None) -> Audit:
    result = Audit(nro_path=str(nro_path), ci_run=ci_run)
    nro = nro_path.read_bytes()
    result.size = len(nro)
    result.sha256 = hashlib.sha256(nro).hexdigest()
    # The identity a coordinator or CI posted for this candidate must match the file byte for byte.
    if expect_nro_sha256 and expect_nro_sha256.lower() != result.sha256:
        result.errors.append(f"NRO SHA-256 {result.sha256} differs from the posted {expect_nro_sha256.lower()}")
    if expect_size is not None and expect_size != result.size:
        result.errors.append(f"NRO size {result.size} differs from the posted {expect_size} bytes")

    if artifact is not None:
        try:
            member, packaged_nro = nro_from_artifact(artifact)
            result.artifact = f"{artifact.name}:{member}"
            if packaged_nro != nro:
                result.errors.append(
                    f"NRO differs from the CI artifact copy {member} "
                    f"(artifact SHA-256 {hashlib.sha256(packaged_nro).hexdigest()})")
        except (OSError, ValueError, zipfile.BadZipFile) as exc:
            result.errors.append(f"cannot read CI artifact: {exc}")

    if expect_sha:
        if not re.fullmatch(r"[0-9a-f]{40}", expect_sha):
            result.errors.append("--expect-sha must be a full 40-character lowercase commit SHA")
        else:
            result.code_sha = expect_sha
            result.tree_sha = git("rev-parse", f"{expect_sha}^{{tree}}") or "unavailable (commit not fetched)"
            short = expect_sha[:8].encode()
            if short + b"\0" in nro:
                result.embedded_commit = f"{expect_sha[:8]} embedded (POKEVAULT_GIT_COMMIT)"
            else:
                result.embedded_commit = "MISSING"
                result.errors.append(f"NRO does not embed commit {expect_sha[:8]}: not built from the expected head")
    else:
        result.notes.append("no --expect-sha given: exact-head identity NOT established")

    try:
        result.nacp_title, result.nacp_version = read_nacp(nro)
    except (romfs_check.RomfsError, struct.error) as exc:
        result.errors.append(f"NACP: {exc}")

    try:
        packaged = romfs_check.walk_romfs(romfs_check.extract_romfs(nro))
    except (romfs_check.RomfsError, UnicodeDecodeError, struct.error) as exc:
        result.errors.append(f"packaged RomFS: {exc}")
        return result
    result.romfs_files = len(packaged)
    result.romfs_digest = hashlib.sha256(
        b"".join(k.encode() + b"\0" + v for k, v in sorted(packaged.items()))).hexdigest()
    local = romfs_check.local_tree(romfs_dir if romfs_dir is not None else ROOT / "romfs")
    result.errors += romfs_check.verify(packaged, local, romfs_check.required_paths())

    roots = source_roots if source_roots is not None else [ROOT / "src", ROOT / "include"]
    referenced = referenced_romfs_paths(roots)
    missing = sorted(p for p in referenced if not packaged.get(p))
    if missing:
        preview = ", ".join(missing[:12]) + (" ..." if len(missing) > 12 else "")
        result.errors.append(f"{len(missing)} asset(s) requested by the app source are absent: {preview}")
    not_in_manifest = sorted(referenced - set(romfs_check.required_paths()))
    if not_in_manifest:
        result.notes.append(f"{len(not_in_manifest)} source-referenced asset(s) are not in "
                            f"verify_nro_romfs.required_paths(), e.g. {', '.join(not_in_manifest[:6])}")
    return result


def hardware_record(a: Audit) -> str:
    verdict = "PASS" if not a.errors else "FAIL - do not hand to the owner"
    lines = [
        "# PokeBank NX hardware test record",
        "",
        "**Status: NOT DEVICE TESTED** - only the owner may change this, and only for this exact file.",
        "",
        "## Exact build identity",
        "",
        "| Field | Value |",
        "| --- | --- |",
        f"| NRO file | `{Path(a.nro_path).name}` |",
        f"| NRO size | {a.size} bytes |",
        f"| NRO SHA-256 | `{a.sha256}` |",
        f"| Code SHA | `{a.code_sha or 'NOT ESTABLISHED'}` |",
        f"| Tree SHA | `{a.tree_sha or 'NOT ESTABLISHED'}` |",
        f"| Embedded commit | {a.embedded_commit} |",
        f"| NACP title / version | {a.nacp_title or '-'} / {a.nacp_version or '-'} |",
        f"| Packaged RomFS | {a.romfs_files} files, content digest `{a.romfs_digest[:16] or '-'}` |",
        f"| CI artifact | {a.artifact or '-'} |",
        f"| CI run | {a.ci_run or '-'} |",
        f"| Automated audit | {verdict} |",
        "",
        "## Owner checklist (fill in on the Switch, this exact NRO only)",
        "",
        "| Area | Steps | Result (PASS / FAIL / NOT RUN) | Notes |",
        "| --- | --- | --- | --- |",
        "| Launch | hbmenu launch, HOME screen renders, artwork and fonts present | NOT RUN | |",
        "| Controller / touch parity | navigate every touched screen with both input methods | NOT RUN | |",
        "| Save Instances | provider discovery and explicit manual link; no guessed paths | NOT RUN | |",
        "| Gen I-IV | open, browse, stage an edit, verified export | NOT RUN | |",
        "| Gen V | open BW/B2W2, copy selection, PK5 + Bag edit, verified export | NOT RUN | |",
        "| Source immutability | original save SHA-256 unchanged after export | NOT RUN | |",
        "| Emulator launch / return | RetroArch / DraStic launch and return | NOT RUN | |",
        "",
        "Owner: ________  Date: ________  Firmware / Atmosphere: ________",
        "",
    ]
    if a.errors:
        lines += ["## Automated audit failures", ""] + [f"- {e}" for e in a.errors] + [""]
    if a.notes:
        lines += ["## Notes", ""] + [f"- {n}" for n in a.notes] + [""]
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("nro", type=Path)
    parser.add_argument("--expect-sha", default="", help="full commit SHA the NRO must be built from")
    parser.add_argument("--expect-nro-sha256", default="", help="posted SHA-256 the NRO must have")
    parser.add_argument("--expect-size", type=int, help="posted NRO size in bytes")
    parser.add_argument("--artifact", type=Path, help="CI artifact (.zip or extracted directory)")
    parser.add_argument("--ci-run", default="", help="CI run URL or id, recorded verbatim")
    parser.add_argument("--romfs", type=Path, default=ROOT / "romfs")
    parser.add_argument("--record", type=Path, help="write the hardware-test record Markdown here")
    args = parser.parse_args(argv)

    try:
        result = audit(args.nro, args.expect_sha, args.artifact, args.ci_run, args.romfs,
                       expect_nro_sha256=args.expect_nro_sha256, expect_size=args.expect_size)
    except OSError as exc:
        print(f"NRO AUDIT: FAIL (cannot read NRO: {exc})")
        return 1
    print(f"NRO:            {result.nro_path}")
    print(f"size:           {result.size} bytes")
    print(f"SHA-256:        {result.sha256}")
    print(f"code SHA:       {result.code_sha or 'NOT ESTABLISHED'}")
    print(f"tree SHA:       {result.tree_sha or 'NOT ESTABLISHED'}")
    print(f"embedded:       {result.embedded_commit}")
    print(f"NACP:           {result.nacp_title or '-'} {result.nacp_version or ''}".rstrip())
    print(f"packaged RomFS: {result.romfs_files} files, digest {result.romfs_digest[:16] or '-'}")
    if result.artifact:
        print(f"CI artifact:    {result.artifact}")
    for note in result.notes:
        print(f"NOTE  {note}")
    for error in result.errors:
        print(f"FAIL  {error}")
    record = hardware_record(result)
    if args.record:
        args.record.write_text(record, encoding="utf-8")
        print(f"hardware-test record: {args.record}")
    print("hardware status: NOT DEVICE TESTED")
    print("NRO AUDIT: PASS" if not result.errors else f"NRO AUDIT: FAIL ({len(result.errors)} problem(s))")
    return 0 if not result.errors else 1


if __name__ == "__main__":
    sys.exit(main())
