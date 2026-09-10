#!/usr/bin/env python3
from pathlib import Path

# One-time exact-anchor patch for the large TrainerViewScreen translation unit. Refuse if upstream
# text drifted so this cannot silently alter accepted later-generation behavior.
path = Path("src/UI/TrainerViewScreen.cpp")
text = path.read_text()
old = '''        infoRow("Gender", t.trainerGender == 0 ? "Male" : "Female");
        infoRow("Trainer ID", std::to_string(t.TID16) + " / " + std::to_string(t.SID16));
        infoRow("Full TID", std::to_string(t.TID));
        infoRow("Full SID", std::to_string(t.SID));
'''
new = '''        const bool generationOne = screen.sourceGameId == "red_gb" ||
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
if text.count(old) != 1:
    raise SystemExit("trainer info rows changed; refusing automated patch")
path.write_text(text.replace(old, new, 1))
