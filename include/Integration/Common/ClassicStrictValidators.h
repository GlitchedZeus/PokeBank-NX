#pragma once

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"

#include <string>

// Strict exact-game parsers used as VerifiedStagedExport validators. Header-only so each caller
// links only the generation parsers it actually uses.
namespace PokeVault::Integration::Common {

inline StrictSaveValidator gen1StrictValidator(Gen1::SourceGame game) {
    return [game](std::span<const uint8_t> bytes, std::string& error) {
        const auto parsed = Gen1::parse(bytes, game);
        if (!parsed) error = parsed.detail.empty() ? "Gen I save failed strict parse" : parsed.detail;
        return static_cast<bool>(parsed);
    };
}

inline StrictSaveValidator gen2StrictValidator(Gen2::SourceGame game) {
    return [game](std::span<const uint8_t> bytes, std::string& error) {
        const auto parsed = Gen2::parse(bytes, game);
        if (!parsed) error = parsed.detail.empty() ? "Gen II save failed strict parse" : parsed.detail;
        return static_cast<bool>(parsed);
    };
}

inline StrictSaveValidator gen3StrictValidator(Gen3::SourceGame game) {
    return [game](std::span<const uint8_t> bytes, std::string& error) {
        const auto parsed = Gen3::parse(bytes, game);
        if (!parsed) error = parsed.detail.empty() ? "Gen III save failed strict parse" : parsed.detail;
        return static_cast<bool>(parsed);
    };
}

} // namespace PokeVault::Integration::Common
