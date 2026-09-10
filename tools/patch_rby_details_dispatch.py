#!/usr/bin/env python3
from pathlib import Path

# One-time exact-anchor maintenance for two large files. Refuse on source drift.

makefile = Path("Makefile")
text = makefile.read_text()
old_sources = "src/Games src/Integration/Gen3 src/Legacy"
new_sources = "src/Games src/Integration/Gen1 src/Integration/Gen3 src/Legacy"
if text.count(old_sources) != 1:
    raise SystemExit("native SOURCES anchor changed; refusing automated patch")
makefile.write_text(text.replace(old_sources, new_sources, 1))

trainer = Path("src/UI/TrainerViewScreen.cpp")
text = trainer.read_text()
old_rows = '''        infoRow("Gender", t.trainerGender == 0 ? "Male" : "Female");
        infoRow("Trainer ID", std::to_string(t.TID16) + " / " + std::to_string(t.SID16));
        infoRow("Full TID", std::to_string(t.TID));
        infoRow("Full SID", std::to_string(t.SID));
'''
new_rows = '''        const bool generationOne = screen.sourceGameId == "red_gb" ||
                                   screen.sourceGameId == "blue_gb" ||
                                   screen.sourceGameId == "yellow_gb";
        if (generationOne) {
            // Gen I stores one visible 16-bit Trainer ID. There is no SID or trainer-gender field
            // to present here, so do not manufacture later-generation rows from neutral defaults.
            infoRow("Trainer ID", std::to_string(t.TID16));
        } else {
            infoRow("Gender", t.trainerGender == 0 ? "Male" : "Female");
            infoRow("Trainer ID", std::to_string(t.TID16) + " / " + std::to_string(t.SID16));
            infoRow("Full TID", std::to_string(t.TID));
            infoRow("Full SID", std::to_string(t.SID));
        }
'''
if text.count(old_rows) != 1:
    raise SystemExit("trainer info rows changed; refusing automated patch")
trainer.write_text(text.replace(old_rows, new_rows, 1))
