/**
 * FormNames.h - Pokemon Form Name Lookup
 *
 * Provides form name mapping for Pokemon with regional variants and other permanent forms.
 * Excludes battle-only forms (Mega Evolution, Gigantamax, Dynamax, etc.)
 */

#ifndef NAMES_FORM_NAMES_H
#define NAMES_FORM_NAMES_H

#include <cstdint>

namespace Names {
    /**
     * Gets the form name for a given Pokemon species and form ID.
     *
     * @param speciesId The Pokemon's species ID (1-1025)
     * @param formId The form ID (0 = base form, 1+ = alternate forms)
     * @return Form name string, or empty string for base form (form 0)
     *
     * Examples:
     * - getFormName(26, 1) = "Alolan" (Alolan Raichu)
     * - getFormName(479, 1) = "Heat" (Heat Rotom)
     * - getFormName(25, 0) = "" (base Pikachu)
     */
    const char* getFormName(uint16_t speciesId, uint8_t formId);
}

#endif
