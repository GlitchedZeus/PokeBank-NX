/**
 * FormNames.cpp - Pokemon Form Name Lookup Implementation
 *
 * Maps (species ID, form ID) pairs to form names for display purposes.
 * Only includes permanent forms - excludes battle-only forms like:
 * - Mega Evolutions
 * - Gigantamax forms
 * - Dynamax forms
 * - Zen Mode (Darmanitan)
 * - Schooling (Wishiwashi)
 * - Battle forms (Aegislash, Minior shields, etc.)
 */

#include <cstdint>
#include <string>

#include "Names/FormNames.h"
#include "Utils/Logger.h"

namespace Names {
    const char* getFormName(uint16_t speciesId, uint8_t formId) {
        // Regional Variants - Form 1 = Regional Form
        switch (speciesId) {
            // Alolan Forms (Gen 7)
            case 19:  // Rattata
            case 20:  // Raticate
            case 26:  // Raichu
            case 27:  // Sandshrew
            case 28:  // Sandslash
            case 37:  // Vulpix
            case 38:  // Ninetales
            case 50:  // Diglett
            case 51:  // Dugtrio
            case 74:  // Geodude
            case 75:  // Graveler
            case 76:  // Golem
            case 88:  // Grimer
            case 89:  // Muk
            case 103: // Exeggutor
            case 105: // Marowak
                if (formId == 1) return "Alolan";
                break;

            // Meowth/Persian - Has both Alolan and Galarian
            case 52:  // Meowth
            case 53:  // Persian
                if (formId == 1) return "Alolan";
                if (formId == 2) return "Galarian";
                break;

            // Galarian Forms (Gen 8)
            case 77:  // Ponyta
            case 78:  // Rapidash
            case 79:  // Slowpoke
            case 80:  // Slowbro
            case 83:  // Farfetch'd
            case 110: // Weezing
            case 122: // Mr. Mime
            case 144: // Articuno
            case 145: // Zapdos
            case 146: // Moltres
            case 199: // Slowking
            case 222: // Corsola
            case 263: // Zigzagoon
            case 264: // Linoone
            case 554: // Darumaka
            case 555: // Darmanitan
            case 562: // Yamask
            case 618: // Stunfisk
                if (formId == 1) return "Galarian";
                break;

            // Hisuian Forms (Gen 8 - Legends: Arceus)
            case 58:  // Growlithe
            case 59:  // Arcanine
            case 100: // Voltorb
            case 101: // Electrode
            case 157: // Typhlosion
            case 211: // Qwilfish
            case 215: // Sneasel
            case 503: // Samurott
            case 549: // Lilligant
            case 570: // Zorua
            case 571: // Zoroark
            case 705: // Sliggoo
            case 706: // Goodra
            case 713: // Avalugg
            case 724: // Decidueye
                if (formId == 1) return "Hisuian";
                break;

            // Wooper - Paldean form
            case 194: // Wooper
                if (formId == 1) return "Paldean";
                break;

            // Tauros - Paldean breeds
            case 128: // Tauros
                if (formId == 1) return "Combat Breed";
                if (formId == 2) return "Blaze Breed";
                if (formId == 3) return "Aqua Breed";
                break;

            // Pokemon with multiple permanent forms
            // ========================================

            // Gender difference
            case 678: // Meowstic
            case 876: // Indeedee
            case 902: // Basculegion
            case 916: // Oinkologne
                if (formId == 0) return "Male";
                if (formId == 1) return "Female";
                break;

            // Deoxys - Different stats per form
            case 386: // Deoxys
                if (formId == 0) return "Normal";
                if (formId == 1) return "Attack";
                if (formId == 2) return "Defense";
                if (formId == 3) return "Speed";
                break;

            // Burmy/Wormadam - Cloak forms
            case 412: // Burmy
            case 413: // Wormadam
                if (formId == 0) return "Plant Cloak";
                if (formId == 1) return "Sandy Cloak";
                if (formId == 2) return "Trash Cloak";
                break;

            // Rotom - Appliance forms
            case 479: // Rotom
                if (formId == 1) return "Heat";
                if (formId == 2) return "Wash";
                if (formId == 3) return "Frost";
                if (formId == 4) return "Fan";
                if (formId == 5) return "Mow";
                break;

            // Giratina - Altered/Origin
            case 487: // Giratina
                if (formId == 0) return "Altered";
                if (formId == 1) return "Origin";
                break;

            // Shaymin - Land/Sky
            case 492: // Shaymin
                if (formId == 0) return "Land";
                if (formId == 1) return "Sky";
                break;

            // Basculin - Forms
            case 550: // Basculin
                if (formId == 0) return "Red-Striped";
                if (formId == 1) return "Blue-Striped";
                if (formId == 2) return "White-Striped";
                break;

            // Tornadus/Thundurus/Landorus - Incarnate/Therian forms
            case 641: // Tornadus
            case 642: // Thundurus
            case 645: // Landorus
                if (formId == 0) return "Incarnate";
                if (formId == 1) return "Therian";
                break;

            // Kyurem - Fusions
            case 646: // Kyurem
                if (formId == 1) return "White";
                if (formId == 2) return "Black";
                break;

            // Keldeo - Resolute
            case 647: // Keldeo
                if (formId == 0) return "Ordinary";
                if (formId == 1) return "Resolute";
                break;

            // Meloetta - Pirouette
            case 648: // Meloetta
                if (formId == 0) return "Aria";
                if (formId == 1) return "Pirouette";
                break;

            // Pumpkaboo/Gourgeist - Size variants
            case 710: // Pumpkaboo
            case 711: // Gourgeist
                if (formId == 0) return "Average";
                if (formId == 1) return "Small";
                if (formId == 2) return "Large";
                if (formId == 3) return "Super";
                break;

            // Zygarde - Forms
            case 718: // Zygarde
                if (formId == 0) return "50%";
                if (formId == 1) return "10%";
                if (formId == 4) return "Complete";
                break;

            // Hoopa - Forms
            case 720: // Hoopa
                if (formId == 0) return "Confined";
                if (formId == 1) return "Unbound";
                break;

            // Oricorio - Styles
            case 741: // Oricorio
                if (formId == 0) return "Baile";
                if (formId == 1) return "Pom-Pom";
                if (formId == 2) return "Pa'u";
                if (formId == 3) return "Sensu";
                break;

            // Lycanroc - Forms
            case 745: // Lycanroc
                if (formId == 0) return "Midday";
                if (formId == 1) return "Midnight";
                if (formId == 2) return "Dusk";
                break;

            // Necrozma - Fusions
            case 800: // Necrozma
                if (formId == 1) return "Dusk Mane";
                if (formId == 2) return "Dawn Wings";
                if (formId == 3) return "Ultra";
                break;

            // Toxtricity - Forms
            case 849: // Toxtricity
                if (formId == 0) return "Amped";
                if (formId == 1) return "Low Key";
                break;

            // Zacian/Zamazenta - Crowned forms
            case 888: // Zacian
                if (formId == 1) return "Crowned Sword";
                break;
            case 889: // Zamazenta
                if (formId == 1) return "Crowned Shield";
                break;

            // Urshifu - Styles
            case 892: // Urshifu
                if (formId == 1) return "Rapid Strike";
                break;

            // Calyrex - Riders
            case 898: // Calyrex
                if (formId == 1) return "Ice Rider";
                if (formId == 2) return "Shadow Rider";
                break;

            // Ursaluna - Bloodmoon
            case 901: // Ursaluna
                if (formId == 1) return "Bloodmoon";
                break;

            // Enamorus - Therian
            case 905: // Enamorus
                if (formId == 1) return "Therian";
                break;

            // Maushold - Family size
            case 925: // Maushold
                if (formId == 1) return "Family of Three";
                break;

            // Squawkabilly - Plumages
            case 931: // Squawkabilly
                if (formId == 1) return "Blue Plumage";
                if (formId == 2) return "Yellow Plumage";
                if (formId == 3) return "White Plumage";
                break;

            // Tatsugiri - Forms
            case 978: // Tatsugiri
                if (formId == 1) return "Droopy";
                if (formId == 2) return "Stretchy";
                break;

            // Dudunsparce - Segment count
            case 982: // Dudunsparce
                if (formId == 1) return "Three-Segment";
                break;

            // Gimmighoul - Forms
            case 999: // Gimmighoul
                if (formId == 1) return "Roaming";
                break;

            // Poltchageist/Sinistcha - Forms
            case 1012: // Poltchageist
            case 1013: // Sinistcha
                if (formId == 1) return "Artisan";
                break;

            // Ogerpon - Masks (permanent forms when holding masks)
            case 1017: // Ogerpon
                if (formId == 1) return "Wellspring Mask";
                if (formId == 2) return "Hearthflame Mask";
                if (formId == 3) return "Cornerstone Mask";
                break;

            // Terapagos - Forms
            case 1024: // Terapagos
                if (formId == 1) return "Terastal";
                if (formId == 2) return "Stellar";
                break;
        }

        // If no match found, return empty string
        return "";
    }

    std::string getDisplayName(uint16_t speciesId, uint8_t formId, const std::string& baseName) {
        const char* f = getFormName(speciesId, formId);
        if (f && f[0] != '\0') return std::string(f) + " " + baseName;
        return baseName;
    }
}
