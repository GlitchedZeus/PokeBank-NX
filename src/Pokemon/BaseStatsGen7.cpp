#include <cstdint>
#include <cstddef>
#include "Pokemon/BaseStatsGen7.h"

// Forward declarations for Names namespace functions
namespace Names {
    extern const char* getSpeciesName(uint16_t speciesId);
    extern const char* getItemName(uint16_t itemId);
    extern const char* getNatureName(uint8_t natureId);
}

namespace Pokemon {
    /**
     * Helper function to search for a species in a form-specific array.
     * Returns nullptr if not found.
     */
    static const BaseStatsGen7* searchFormArray(
        const BaseStatsGen7* array,
        size_t arraySize,
        uint16_t speciesId)
    {
        for (size_t i = 0; i < arraySize; i++) {
            if (array[i].id == speciesId) {
                return &array[i];
            }
        }
        return nullptr;
    }

    const BaseStatsGen7* getBaseStatsGen7(uint16_t speciesId, uint8_t form) {
        static const BaseStatsGen7 empty = {0, 0, 0, 0, 0, 0, 0};

        // Form 0 = base form, use main table
        if (form == 0) {
            if (speciesId >= BASE_STATS_COUNT_GEN7) {
                return &empty;
            }
            return &BASE_STATS_TABLE_GEN7[speciesId];
        }

        // Gen 7 only has Alolan forms for regional variants (form 1)
        if (form == 1) {
            switch (speciesId) {
                // Alolan forms (Gen 7)
                case 19: case 20: case 26: case 27: case 28: case 37: case 38:
                case 50: case 51: case 52: case 53: case 74: case 75: case 76:
                case 88: case 89: case 103: case 105: {
                    const BaseStatsGen7* stats = searchFormArray(
                        BASE_STATS_TABLE_ALOLAN,
                        sizeof(BASE_STATS_TABLE_ALOLAN) / sizeof(BASE_STATS_TABLE_ALOLAN[0]),
                        speciesId
                    );
                    if (stats) return stats;
                    break;
                }
            }
        }

        // Pokemon-specific form lookups for Gen 7
        // Handle Pokemon with unique form mechanics (Deoxys, Rotom, Giratina, etc.)

        switch (speciesId) {
            // Deoxys - Forms: 0=Normal, 1=Attack, 2=Defense, 3=Speed
            case 386:
                if (form >= 0 && form <= 3) {
                    return &BASE_STATS_TABLE_DEOXYS_FORMS[form];
                }
                break;

            // Burmy - Forms: 0=Plant, 1=Sandy, 2=Trash
            case 412:
                if (form >= 0 && form <= 2) {
                    return &BASE_STATS_TABLE_BURMY_WORMADAM_FORMS[form];
                }
                break;

            // Wormadam - Forms: 0=Plant, 1=Sandy, 2=Trash
            case 413:
                if (form >= 0 && form <= 2) {
                    return &BASE_STATS_TABLE_BURMY_WORMADAM_FORMS[3 + form]; // Offset by 3 (Burmy entries)
                }
                break;

            // Rotom - Forms: 0=Base, 1=Heat, 2=Wash, 3=Frost, 4=Fan, 5=Mow
            case 479:
                if (form >= 1 && form <= 5) {
                    return &BASE_STATS_TABLE_ROTOM_FORMS[form - 1]; // Array starts at Heat
                }
                break;

            // Dialga - Forms: 0=Altered, 1=Origin
            case 483:
                if (form == 1) {
                    return &BASE_STATS_TABLE_DIALGA_PALKIA_GIRATINA_FORMS[0];
                }
                break;

            // Palkia - Forms: 0=Altered, 1=Origin
            case 484:
                if (form == 1) {
                    return &BASE_STATS_TABLE_DIALGA_PALKIA_GIRATINA_FORMS[1];
                }
                break;

            // Giratina - Forms: 0=Altered, 1=Origin
            case 487:
                if (form == 0) {
                    return &BASE_STATS_TABLE_DIALGA_PALKIA_GIRATINA_FORMS[2]; // Altered
                } else if (form == 1) {
                    return &BASE_STATS_TABLE_DIALGA_PALKIA_GIRATINA_FORMS[3]; // Origin
                }
                break;

            // Shaymin - Forms: 0=Land, 1=Sky
            case 492:
                if (form >= 0 && form <= 1) {
                    return &BASE_STATS_TABLE_SHAYMIN_FORMS[form];
                }
                break;

            // Basculin - Forms: 0=Red-Striped, 1=Blue-Striped
            case 550:
                if (form == 1) {
                    return &BASE_STATS_TABLE_BASCULIN_FORMS[0]; // Blue-Striped
                }
                break;

            // Tornadus - Forms: 0=Incarnate, 1=Therian
            case 641:
                if (form == 1) {
                    return &BASE_STATS_TABLE_TORNADUS_THUNDURUS_LANDORUS_FORMS[0];
                }
                break;

            // Thundurus - Forms: 0=Incarnate, 1=Therian
            case 642:
                if (form == 1) {
                    return &BASE_STATS_TABLE_TORNADUS_THUNDURUS_LANDORUS_FORMS[1];
                }
                break;

            // Landorus - Forms: 0=Incarnate, 1=Therian
            case 645:
                if (form == 1) {
                    return &BASE_STATS_TABLE_TORNADUS_THUNDURUS_LANDORUS_FORMS[2];
                }
                break;

            // Kyurem - Forms: 0=Base, 1=White, 2=Black
            case 646:
                if (form >= 1 && form <= 2) {
                    return &BASE_STATS_TABLE_KYUREM_FORMS[form - 1];
                }
                break;

            // Keldeo - Forms: 0=Ordinary, 1=Resolute
            case 647:
                if (form >= 0 && form <= 1) {
                    return &BASE_STATS_TABLE_KELDEO_FORMS[form];
                }
                break;

            // Meloetta - Forms: 0=Aria, 1=Pirouette
            case 648:
                if (form >= 0 && form <= 1) {
                    return &BASE_STATS_TABLE_MELOETTA_FORMS[form];
                }
                break;

            // Meowstic - Forms: 0=Male, 1=Female
            case 678:
                if (form >= 0 && form <= 1) {
                    return &BASE_STATS_TABLE_MEOWSTIC_FORMS[form];
                }
                break;

            // Pumpkaboo - Forms: 0=Average, 1=Small, 2=Large, 3=Super
            case 710:
                if (form >= 0 && form <= 3) {
                    return &BASE_STATS_TABLE_PUMPKABOO_GOURGEIST_FORMS[form];
                }
                break;

            // Gourgeist - Forms: 0=Average, 1=Small, 2=Large, 3=Super
            case 711:
                if (form >= 0 && form <= 3) {
                    return &BASE_STATS_TABLE_PUMPKABOO_GOURGEIST_FORMS[4 + form]; // Offset by 4 (Pumpkaboo entries)
                }
                break;

            // Zygarde - Forms: 0=50%, 1=10%
            case 718:
                if (form == 0) {
                    return &BASE_STATS_TABLE_ZYGARDE_FORMS[0]; // 50%
                } else if (form == 1) {
                    return &BASE_STATS_TABLE_ZYGARDE_FORMS[1]; // 10%
                }
                break;

            // Hoopa - Forms: 0=Confined, 1=Unbound
            case 720:
                if (form >= 0 && form <= 1) {
                    return &BASE_STATS_TABLE_HOOPA_FORMS[form];
                }
                break;

            // Oricorio - Forms: 0=Baile, 1=Pom-Pom, 2=Pa'u, 3=Sensu
            case 741:
                if (form >= 0 && form <= 3) {
                    return &BASE_STATS_TABLE_ORICORIO_FORMS[form];
                }
                break;

            // Lycanroc - Forms: 0=Midday, 1=Midnight, 2=Dusk
            case 745:
                if (form >= 0 && form <= 2) {
                    return &BASE_STATS_TABLE_LYCANROC_FORMS[form];
                }
                break;
        }

        // If no form-specific entry found, fall back to base form
        if (speciesId >= BASE_STATS_COUNT_GEN7) {
            return &empty;
        }
        return &BASE_STATS_TABLE_GEN7[speciesId];
    }

    // Wrapper functions that forward to Names namespace
    const char* getSpeciesNameGen7(uint16_t speciesId) {
        return Names::getSpeciesName(speciesId);
    }

    const char* getItemNameGen7(uint16_t itemId) {
        return Names::getItemName(itemId);
    }

    const char* getNatureNameGen7(uint8_t natureId) {
        return Names::getNatureName(natureId);
    }
}
