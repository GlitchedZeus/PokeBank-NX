#!/usr/bin/env python3
from pathlib import Path

path = Path("src/UI/Modals/PokemonDetailsModal.cpp")
text = path.read_text()

include_anchor = '#include "Pokemon/Pokemon.h"\n'
include_replacement = (
    '#include "Pokemon/Pokemon.h"\n'
    '#include "Pokemon/Pokemon1ReadOnly.h"\n'
    '#include "UI/Modals/Gen1PokemonDetailsModal.h"\n'
)
if text.count(include_anchor) != 1:
    raise SystemExit("Pokemon include anchor changed; refusing automated patch")
text = text.replace(include_anchor, include_replacement, 1)

function_anchor = (
    '        const Pokemon::Pokemon* p = screen.detailsTargetPokemon();\n'
    '        if (!p || p->speciesID() == 0) return;\n\n'
)
function_replacement = (
    '        const Pokemon::Pokemon* p = screen.detailsTargetPokemon();\n'
    '        if (!p || p->speciesID() == 0) return;\n\n'
    '        // PK1 is not a reduced modern format: it has DVs/stat experience and lacks nature,\n'
    '        // ability, held item, SID, ribbons/marks and modern met/Ball metadata. Route it before\n'
    '        // the modern editor computes or draws any of those fields.\n'
    '        if (const auto* gen1 = dynamic_cast<const Pokemon::Pokemon1ReadOnly*>(p)) {\n'
    '            drawGen1PokemonDetailsModal(screen, fb, *gen1);\n'
    '            return;\n'
    '        }\n\n'
)
if text.count(function_anchor) != 1:
    raise SystemExit("details function anchor changed; refusing automated patch")
text = text.replace(function_anchor, function_replacement, 1)
path.write_text(text)
