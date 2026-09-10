#!/usr/bin/env python3
from pathlib import Path

# One-time exact-anchor maintenance patch for the large UI/Makefile files. Refuse to proceed if the
# expected source has drifted; this is deliberately not a general rewriting tool.

modal = Path("src/UI/Modals/PokemonDetailsModal.cpp")
text = modal.read_text()
dynamic = '''        if (const auto* gen1 = dynamic_cast<const Pokemon::Pokemon1ReadOnly*>(p)) {
            drawGen1PokemonDetailsModal(screen, fb, *gen1);
            return;
        }
'''
static = '''        if (p->getGameGroup() == Pokemon::Pokemon1ReadOnly::kReadOnlyGameGroup) {
            const auto& gen1 = static_cast<const Pokemon::Pokemon1ReadOnly&>(*p);
            drawGen1PokemonDetailsModal(screen, fb, gen1);
            return;
        }
'''
if text.count(dynamic) != 1:
    raise SystemExit("Gen I details dispatch anchor changed; refusing automated patch")
modal.write_text(text.replace(dynamic, static, 1))

makefile = Path("Makefile")
text = makefile.read_text()
old = "src/Games src/Integration/Gen3 src/Legacy"
new = "src/Games src/Integration/Gen1 src/Integration/Gen3 src/Legacy"
if text.count(old) != 1:
    raise SystemExit("native SOURCES anchor changed; refusing automated patch")
makefile.write_text(text.replace(old, new, 1))
