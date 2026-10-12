/**
 * LocationNames.h - Met-location Name Lookup (per generation)
 *
 * Maps a met/egg-location ID to its display name. The same numeric location ID
 * refers to a DIFFERENT place in each generation, so the lookup is keyed off
 * the mon's ORIGIN game version (the standard version id stored in the PKM,
 * see Enums::GameVersion).
 *
 * IDs are BANKED (id / 10000): bank 0 = in-world locations, bank 3 (30000) =
 * Link Trade / region + HOME/GO transfers, bank 4 (40000) = events,
 * bank 6 (60000) = egg received-from sources. A traded/transferred/egg mon
 * therefore has a high-banked id; resolving only bank 0 renders it "(none)".
 * Tables are extracted from PKHeX's met-location text resources and the lookup
 * mirrors PKHeX's LocationSet6.GetLocationName.
 */

#ifndef NAMES_LOCATION_NAMES_H
#define NAMES_LOCATION_NAMES_H

#include <cstdint>
#include <cstddef>

namespace Names {
    /**
     * Gets the met-location name for a mon of a given origin game.
     *
     * @param originVersion Standard game-version id (Enums::GameVersion), e.g.
     *                      42/43 = Let's Go Pikachu/Eevee, 44/45 = Sword/Shield,
     *                      47 = Legends Arceus, 48/49 = Brilliant Diamond/Shining
     *                      Pearl, 50/51 = Scarlet/Violet, 52 = Legends Z-A.
     * @param locationId    The met/egg-location ID; routed to its bank (id / 10000)
     *                      and indexed at id minus the bank base.
     * @return Location name string, or "" if the id is unknown/out of range or
     *         the origin version has no supported table.
     */
    const char* getMetLocationName(uint8_t originVersion, uint16_t locationId);

    /**
     * The raw bank-0 (in-world) name table for an origin game, for ENUMERATING a
     * picker (the id->name lookup above answers a single id). The special banks
     * 3/4/6 are trade/transfer/egg markers, not user-assignable met locations, so
     * they're deliberately excluded here. names[id] is the location name, possibly
     * "" for gaps / PKHeX's "no location" sentinel -- callers filter those out.
     * Returns {nullptr, 0} when the origin version has no supported table.
     */
    struct LocationTable { const char* const* names; size_t count; };
    LocationTable getLocationTable(uint8_t originVersion);

    /**
     * True when a bank-0 Gen IV met-location ID belongs to the Pokémon's exact
     * origin game. Mirrors PKHeX Locations.IsMetLocation4DP/Pt/HGSS.
     *
     * D/P: 0..111, Platinum: 0..125, HG/SS: 126..233.
     * Special trade/event/egg banks are intentionally not ordinary picker targets.
     */
    inline bool isGen4NativeMetLocation(uint8_t originVersion, uint16_t locationId) noexcept {
        switch (originVersion) {
            case 10: // Diamond
            case 11: // Pearl
                return locationId <= 111;
            case 12: // Platinum
                return locationId <= 125;
            case 7:  // HeartGold
            case 8:  // SoulSilver
                return locationId > 125 && locationId < 234;
            default:
                return false;
        }
    }
}

#endif
