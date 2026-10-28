#!/usr/bin/env python3
"""tools/qa/make_retest.py: the bound candidate's owner checklist is made from the template with the exact
identity from docs/qa/CANDIDATE_UNDER_AUDIT.env, keeps every item and NOT DEVICE TESTED, never overwrites
an existing file (it may hold owner results), and --check rejects a checklist for any other build."""
from __future__ import annotations

import importlib.util
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("make_retest", ROOT / "tools/qa/make_retest.py")
mr = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mr)

# Synthetic identity (not a published build).
ENV = """# synthetic binding for the test
RUN_ID=111
ARTIFACT_ID=222
ARTIFACT_NAME=PokeBank-NX-Gen1-5-Candidate-0123abcd
NRO_FILE=PokeBank-NX-Gen1-5-Candidate-0123abcd.nro
CODE_SHA=0123abcd0123abcd0123abcd0123abcd0123abcd
TREE_SHA=fedcba98fedcba98fedcba98fedcba98fedcba98
NRO_SHA256=aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00aa00
NRO_SIZE=123456
"""

failures = 0
MADE: list[Path] = []


def expect(cond: bool, what: str) -> None:
    global failures
    print(f"{'PASS' if cond else 'FAIL'}  {what}")
    failures += 0 if cond else 1


def tree(env_text: str) -> Path:
    root = Path(tempfile.mkdtemp())
    MADE.append(root)
    (root / "docs/qa").mkdir(parents=True)
    (root / "docs/hardware").mkdir(parents=True)
    shutil.copy(ROOT / mr.TEMPLATE, root / mr.TEMPLATE)
    (root / mr.ENV).write_text(env_text, encoding="utf-8")
    return root


root = tree(ENV)
expect(mr.main(["make_retest", "--root", str(root)]) == 0, "writes RETEST_<sha8>.md from the binding")
out = root / "docs/hardware/RETEST_0123abcd.md"
text = out.read_text(encoding="utf-8")
template = (root / mr.TEMPLATE).read_text(encoding="utf-8")
for value in ("PokeBank-NX-Gen1-5-Candidate-0123abcd.nro", "0123abcd0123abcd0123abcd0123abcd0123abcd",
              "fedcba98fedcba98fedcba98fedcba98fedcba98", "aa00aa00" * 8, "123456 bytes", "run `111`", "id `222`"):
    expect(value in text, f"identity table names {value[:24]}")
expect("NOT DEVICE TESTED" in text, "keeps NOT DEVICE TESTED")
expect("(filled in at binding)" not in text and "(template)" not in text, "no template placeholders left")
expect(text.count("**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED") ==
       template.count("**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED"), "every result box kept, all unticked")
expect(mr.main(["make_retest", "--root", str(root), "--check"]) == 0, "--check accepts it")

before = out.read_bytes()
try:
    mr.main(["make_retest", "--root", str(root)])
    expect(False, "refuses to overwrite an existing checklist")
except SystemExit:
    expect(out.read_bytes() == before, "refuses to overwrite an existing checklist (owner results kept)")

out.write_text(text.replace("aa00aa00" * 8, "bb11bb11" * 8), encoding="utf-8")
expect(mr.main(["make_retest", "--root", str(root), "--check"]) == 1, "--check rejects a checklist for another NRO")
out.write_text(text.replace("### B03 · ", "### X03 · "), encoding="utf-8")
expect(mr.main(["make_retest", "--root", str(root), "--check"]) == 1, "--check rejects a checklist with an item dropped")
out.write_text(text.replace("NOT DEVICE TESTED", "DEVICE TESTED"), encoding="utf-8")
expect(mr.main(["make_retest", "--root", str(root), "--check"]) == 1, "--check rejects a checklist claiming device testing")
out.unlink()
expect(mr.main(["make_retest", "--root", str(root), "--check"]) == 1, "--check rejects a missing checklist")

expect(text.startswith("# Round 7 hardware retest checklist: PokeBank-NX-Gen1-5-Candidate-0123abcd.nro"),
       "default template is round 7 and the heading names the NRO")
expect("bound round 7 candidate `0123abcd`" in text, "binding note names round 7 and the candidate")

# An earlier round's checklist is still checked against its own round's template.
old = tree(ENV)
r6 = "docs/hardware/RETEST_ROUND6_TEMPLATE.md"
shutil.copy(ROOT / r6, old / r6)
expect(mr.main(["make_retest", "--root", str(old), "--template", r6]) == 0, "renders from the round 6 template")
old_out = old / "docs/hardware/RETEST_0123abcd.md"
old_text = old_out.read_text(encoding="utf-8")
expect(old_text.startswith("# Round 6 hardware retest checklist: "), "round 6 heading kept")
expect(mr.main(["make_retest", "--root", str(old), "--check"]) == 0, "--check uses the round 6 template for a round 6 checklist")
old_out.write_text(old_text.replace("### C17 · ", "### X17 · "), encoding="utf-8")
expect(mr.main(["make_retest", "--root", str(old), "--check"]) == 1, "--check rejects a round 6 checklist with an item dropped")
old_out.write_text(old_text.replace("# Round 6 hardware", "# Round 7 hardware"), encoding="utf-8")
expect(mr.main(["make_retest", "--root", str(old), "--check"]) == 1, "--check rejects a round 6 checklist relabelled round 7")

for bad, why in ((ENV.replace("CODE_SHA=0123abcd", "CODE_SHA=0123"), "short CODE_SHA"),
                 (ENV.replace("NRO_SIZE=123456\n", ""), "missing NRO_SIZE"),
                 (ENV.replace("Candidate-0123abcd.nro", "Candidate-99999999.nro"), "NRO_FILE for another build"),
                 (ENV + "RUN_ID=5\n", "duplicated RUN_ID")):
    r = tree(bad)
    try:
        mr.main(["make_retest", "--root", str(r)])
        expect(False, f"rejects a binding with {why}")
    except SystemExit:
        expect(not (r / "docs/hardware/RETEST_0123abcd.md").exists(), f"rejects a binding with {why}")

for made in MADE:
    shutil.rmtree(made, ignore_errors=True)
print(f"{'FAIL' if failures else 'PASS'}: make_retest ({failures} failure(s))")
sys.exit(1 if failures else 0)
