#!/usr/bin/env python3
"""Round 13 recheck regression (docs/qa/RECHECK_REGRESSIONS.md) for PBX-LQT-002 and PBX-LQT-001.

PBX-LQT-002: owner PASS on 3be4de6b (DEVICE_TEST_FOLLOWUP_2026-09-03): HOME -> resume, sleep -> wake,
Joy-Con detach / reconnect, input after reconnect. src/UI/UI.cpp (every frame loop of the app) has
changed in 50+ commits since 3be4de6b. libnx suspends a frame loop for HOME / sleep only inside
appletMainLoop(), and asks the app to exit by making it return false, so the accepted lifecycle holds
only while EVERY frame loop is gated on appletMainLoop() and the controller is read through the one
pad configured for standard (handheld + detached Joy-Con) input. A loop that spins without it would
keep drawing while the applet is out of focus and ignore an exit request.

PBX-LQT-001 (exit-and-relaunch after discard): an exit request leaves Product Home without another
frame and without writing anything (the outer loop returns instead of rebuilding the screen).

Device-only behaviour stays on the hardware checklist; this pins the code structure it relies on.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
UI = (ROOT / "src/UI/UI.cpp").read_text(encoding="utf-8")
failures = []


def require(ok, what):
    if not ok:
        failures.append(what)


def loop_condition(text, start):
    """The balanced (...) condition of the while at `start`."""
    i = text.index("(", start)
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "(":
            depth += 1
        elif text[j] == ")":
            depth -= 1
            if depth == 0:
                return text[i + 1:j]
    return ""


def loop_body(text, start):
    i = text.index("{", start)
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[i + 1:j]
    return ""


frame_loops = 0
for m in re.finditer(r"\bwhile\s*\(", UI):
    cond = loop_condition(UI, m.start())
    body = loop_body(UI, m.end())
    line = UI.count("\n", 0, m.start()) + 1
    draws = re.search(r"\.(update|draw)\s*\(|fb\.flush\s*\(|padUpdate\s*\(", body)
    if cond.strip() == "running":
        # The Product Home rebuild loop: its body is one gated frame loop, and it must leave (return)
        # whenever that loop ended without a rebuild request, e.g. because appletMainLoop() was false.
        require("appletMainLoop()" in body, f"src/UI/UI.cpp:{line}: the Product Home loop runs no gated frame loop")
        require("if (!rebuildPicker) return" in body,
                f"src/UI/UI.cpp:{line}: an applet exit request must return, not rebuild Product Home")
        continue
    if not draws:
        continue
    frame_loops += 1
    require("appletMainLoop()" in cond,
            f"src/UI/UI.cpp:{line}: frame loop `while ({cond.strip()})` is not gated on appletMainLoop() "
            "(HOME / sleep / exit lifecycle, PBX-LQT-002)")

require(frame_loops >= 8, f"expected the app's frame loops in src/UI/UI.cpp, found {frame_loops}")

# One pad, standard style set (handheld and detached Joy-Con), initialised once for the app.
require(UI.count("padConfigureInput(1, HidNpadStyleSet_NpadStandard);") == 1,
        "src/UI/UI.cpp: the pad is configured once for standard (handheld + Joy-Con) input")
require("padInitializeDefault(&pad);" in UI, "src/UI/UI.cpp: the default pad (reconnect follows libnx) is used")

# Leaving the app never writes a save: no applet hook or exit path calls a save writer.
for bad in ("appletHook(", "appletSetFocusHandlingMode(", "AppletHookType_OnExitRequest"):
    require(bad not in UI, f"src/UI/UI.cpp: unexpected applet hook {bad!r} (lifecycle must stay libnx default)")

if failures:
    print("FAIL test_recheck_lifecycle_contract")
    for f in failures:
        print("  -", f)
    sys.exit(1)
print(f"PASS test_recheck_lifecycle_contract ({frame_loops} frame loops gated on appletMainLoop)")
