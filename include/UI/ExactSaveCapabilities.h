#pragma once
#include "Games/GameIdentity.h"
#include "UI/PokemonEditorFoundationContract.h"
#include <optional>
#include <string_view>

namespace PokeBank::UIModel::PokemonEditorFoundation {
// Format is the current serialization layout, never the Pokemon's origin game.
enum class SaveFormat : uint8_t {
    Unknown, RBYInternational, RBYJapanese, GSCInternational, GSCJapanese,
    PK3GBA, PB7, PA8, PK9
};
enum class SaveFamily : uint8_t {
    RBY, GoldSilver, Crystal, RubySapphire, Emerald, FireRedLeafGreen,
    LetsGo, LegendsArceus, ScarletViolet
};
enum class StatModel : uint8_t { DVStatExpSingleSpecial, DVStatExpSplitSpecial, IVAwakening, IVEffortLevel, IVEV };
struct ExactSaveIdentity {
    std::string_view gameId;
    PokeVault::Games::Platform platform;
    Generation generation;
    SaveFormat format;
    uint16_t revision = 0;
};
struct ExactSaveCapabilities {
    ExactSaveIdentity identity;
    SaveFamily family;
    StatModel statModel;
    Capabilities fields;
    uint8_t geneticMaximum = 15;
    uint16_t trainingMaximum = 65535;
    bool supportsPokerus = false;
    bool supportsCrystalCaughtData = false;
    bool usesAVs = false;
    bool usesEffortLevels = false;
    // Describes the audited UI model only. Does not enable any write adapter.
};

constexpr std::optional<ExactSaveCapabilities> exactSaveCapabilities(const ExactSaveIdentity& id) noexcept {
    using PokeVault::Games::Platform;
    const auto game = id.gameId;
    if (game == "red_gb" || game == "blue_gb" || game == "yellow_gb") {
        if (id.platform != Platform::GameBoy || id.generation != Generation::Gen1 ||
            (id.format != SaveFormat::RBYInternational && id.format != SaveFormat::RBYJapanese)) return std::nullopt;
        return ExactSaveCapabilities{id,SaveFamily::RBY,StatModel::DVStatExpSingleSpecial,
            capabilitiesForGeneration(Generation::Gen1)};
    }
    if (game == "gold_gbc" || game == "silver_gbc" || game == "crystal_gbc") {
        if (id.platform != Platform::GameBoyColor || id.generation != Generation::Gen2 ||
            (id.format != SaveFormat::GSCInternational && id.format != SaveFormat::GSCJapanese)) return std::nullopt;
        const bool crystal = game == "crystal_gbc";
        auto fields = capabilitiesForGeneration(Generation::Gen2);
        fields.supportsMetLevel = crystal;
        return ExactSaveCapabilities{id,crystal ? SaveFamily::Crystal : SaveFamily::GoldSilver,
            StatModel::DVStatExpSplitSpecial,fields,15,65535,true,crystal};
    }
    if (game == "ruby_gba" || game == "sapphire_gba" || game == "emerald_gba" ||
        game == "firered_gba" || game == "leafgreen_gba") {
        if (id.platform != Platform::GameBoyAdvance || id.generation != Generation::Gen3 ||
            id.format != SaveFormat::PK3GBA) return std::nullopt;
        auto fields = capabilitiesForGeneration(Generation::Gen3);
        // Ribbons/egg state are not exposed by the current staged product adapter yet.
        // They stay unavailable instead of being synthesized from generic generation defaults.
        fields.supportsRibbons = false;
        fields.supportsEgg = false;
        const SaveFamily family =
            (game == "emerald_gba") ? SaveFamily::Emerald :
            (game == "firered_gba" || game == "leafgreen_gba")
                ? SaveFamily::FireRedLeafGreen : SaveFamily::RubySapphire;
        return ExactSaveCapabilities{id, family, StatModel::IVEV, fields,
                                     31, 255, true, false};
    }
    if (id.platform != Platform::NintendoSwitch) return std::nullopt;
    if (game == "letsgo_pikachu_switch" || game == "letsgo_eevee_switch") {
        if (id.generation != Generation::Gen7 || id.format != SaveFormat::PB7) return std::nullopt;
        auto fields = capabilitiesForGeneration(Generation::Gen7);
        fields.supportsAbility = fields.supportsHeldItem = fields.supportsEgg = fields.usesEVs = false;
        fields.supportsRibbons = false;
        return ExactSaveCapabilities{id,SaveFamily::LetsGo,StatModel::IVAwakening,fields,31,200,false,false,true};
    }
    if (game == "legends_arceus_switch") {
        if (id.generation != Generation::Gen8 || id.format != SaveFormat::PA8) return std::nullopt;
        auto fields = capabilitiesForGeneration(Generation::Gen8);
        fields.supportsAbility = fields.supportsHeldItem = fields.supportsEgg = fields.usesEVs = false;
        return ExactSaveCapabilities{id,SaveFamily::LegendsArceus,StatModel::IVEffortLevel,fields,31,10,false,false,false,true};
    }
    if (game == "scarlet_switch" || game == "violet_switch") {
        if (id.generation != Generation::Gen9 || id.format != SaveFormat::PK9) return std::nullopt;
        return ExactSaveCapabilities{id,SaveFamily::ScarletViolet,StatModel::IVEV,
            capabilitiesForGeneration(Generation::Gen9),31,252};
    }
    return std::nullopt; // Unknown format/game combinations never inherit a generic editor.
}

// Presentation convenience for established source IDs. Parsers/adapters that know
// a region/revision should pass their explicit identity to exactSaveCapabilities.
constexpr std::optional<ExactSaveCapabilities> capabilitiesForSourceId(std::string_view id) noexcept {
    using PokeVault::Games::Platform;
    if (id == "red_gb" || id == "blue_gb" || id == "yellow_gb")
        return exactSaveCapabilities({id,Platform::GameBoy,Generation::Gen1,SaveFormat::RBYInternational});
    if (id == "gold_gbc" || id == "silver_gbc" || id == "crystal_gbc")
        return exactSaveCapabilities({id,Platform::GameBoyColor,Generation::Gen2,SaveFormat::GSCInternational});
    if (id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" ||
        id == "firered_gba" || id == "leafgreen_gba")
        return exactSaveCapabilities({id,Platform::GameBoyAdvance,Generation::Gen3,SaveFormat::PK3GBA});
    if (id == "letsgo_pikachu_switch" || id == "letsgo_eevee_switch")
        return exactSaveCapabilities({id,Platform::NintendoSwitch,Generation::Gen7,SaveFormat::PB7});
    if (id == "legends_arceus_switch")
        return exactSaveCapabilities({id,Platform::NintendoSwitch,Generation::Gen8,SaveFormat::PA8});
    if (id == "scarlet_switch" || id == "violet_switch")
        return exactSaveCapabilities({id,Platform::NintendoSwitch,Generation::Gen9,SaveFormat::PK9});
    return std::nullopt;
}
constexpr SupplementalAction supplementalActionFor(const ExactSaveCapabilities& capabilities, SupplementalField field) noexcept {
    if (field == SupplementalField::HeldItem)
        return capabilities.fields.supportsHeldItem ? SupplementalAction::OpenHeldItemPicker : SupplementalAction::Unavailable;
    return capabilities.fields.supportsRibbons ? SupplementalAction::OpenRibbonCollection : SupplementalAction::Unavailable;
}
} // namespace PokeBank::UIModel::PokemonEditorFoundation
