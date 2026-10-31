// ModernLegality.cpp - Generation VI-IX dispatch (see include/Legality/ModernLegality.h).
#include "Legality/ModernLegality.h"

#include <algorithm>
#include <string>

#include "Legality/Gen67Legality.h"
#include "Legality/Gen7bLegality.h"
#include "Legality/Gen8aLegality.h"
#include "Legality/Gen8Legality.h"

namespace Legality::Modern {
namespace {

// PKHeX PokeCrypto SIZE_6STORED/SIZE_6PARTY, PB7.SIZE, SIZE_8STORED/SIZE_8PARTY (PK8, PB8,
// PK9, PA9) and SIZE_8ASTORED/SIZE_8APARTY (PA8).
constexpr std::array<FormatProfile, 8> kProfiles{{
    {Format::PK6, "PK6", 6, 0xE8, 0x104, true},
    {Format::PK7, "PK7", 7, 0xE8, 0x104, true},
    {Format::PB7, "PB7", 7, 0x104, 0x104, true},
    {Format::PK8, "PK8", 8, 0x148, 0x158, true},
    {Format::PB8, "PB8", 8, 0x148, 0x158, true},
    {Format::PA8, "PA8", 8, 0x168, 0x178, true},
    {Format::PK9, "PK9", 9, 0x148, 0x158, true},
    {Format::PA9, "PA9", 9, 0x148, 0x158, false},
}};

} // namespace

const FormatProfile* profile(Format format) noexcept {
    for (const auto& p : kProfiles)
        if (p.format == format) return &p;
    return nullptr;
}

Format nativeFormat(Game game) noexcept {
    switch (game) {
        case Game::X: case Game::Y: case Game::AS: case Game::OR: return Format::PK6;
        case Game::SN: case Game::MN: case Game::US: case Game::UM: return Format::PK7;
        case Game::GP: case Game::GE: return Format::PB7;
        case Game::SW: case Game::SH: return Format::PK8;
        case Game::BD: case Game::SP: return Format::PB8;
        case Game::PLA: return Format::PA8;
        case Game::SL: case Game::VL: return Format::PK9;
        case Game::ZA: return Format::PA9;
        case Game::Unknown: break;
    }
    return Format::Unknown;
}

uint8_t generationOf(Game game) noexcept {
    const auto* p = profile(nativeFormat(game));
    return p ? p->generation : 0;
}

Report analyze(std::span<const uint8_t> record, const Input& input) {
    const FormatProfile* p = profile(input.format);
    if (!p) {
        Report report{};
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Record format is unknown; nothing was checked", CheckIdentifier::Structure});
        return report;
    }
    if (input.format == Format::PK6) return Gen67::analyze(6, record, input);
    if (input.format == Format::PK7) return Gen67::analyze(7, record, input);
    if (input.format == Format::PB7) return Gen7b::analyze(record, input);
    if (input.format == Format::PK8) return Gen8::analyze(Gen8::Context::SwordShield, record, input);
    if (input.format == Format::PB8) return Gen8::analyze(Gen8::Context::BDSP, record, input);
    if (input.format == Format::PA8) return Gen8a::analyze(record, input);
    if (input.format == Format::PK9) return Gen8::analyze(Gen8::Context::ScarletViolet, record, input);
    Report report{};
    report.coverage.structure = CoverageLevel::None;
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    report.issues.push_back({Severity::Warning,
        std::string(p->name) + " legality rules are not implemented yet; the record was not checked",
        CheckIdentifier::Structure});
    return report;
}

} // namespace Legality::Modern
