#ifndef POKEBANK_SOURCE_SAVE_PROVIDER_IDENTITY_H
#define POKEBANK_SOURCE_SAVE_PROVIDER_IDENTITY_H

#include "Source/SaveInstance.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Source {

// Product presentation of the provider that discovery (or an explicit manual
// link) recorded in Save Instance metadata. The raw label is what the
// generation's discovery code stored: SaveInstance::providerLabel, or the
// persisted BindingRecord::sourceType for Gen IV/V assigned files. It is
// NEVER derived from the generation, the platform, a default emulator, or a
// filename. A label this table does not know stays "Unknown provider".
enum class ProviderIdentity : uint8_t {
    Unknown,
    Manual,
    Multiple,
    DraStic,
    DraSticSavestate,
    MelonDS,
    RetroArch,
    RetroArchDeSmuME,
    MGBA,
    Tico,
    Dekopon,
    Azahar,
};

namespace ProviderDetail {
[[nodiscard]] inline bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}
} // namespace ProviderDetail

// Exact labels written by the discovery code (Gen4SourceDiscovery,
// Gen5SourceDiscovery, RetroArchFRLGDiscovery, RBY/GSC browsers) and by the
// explicit manual-link flows ("Manual"). Matching is exact apart from ASCII
// case, because Gen I-III cards historically upper-cased the label.
[[nodiscard]] inline ProviderIdentity identifyProvider(std::string_view raw) noexcept {
    using ProviderDetail::equalsIgnoreCase;
    if (equalsIgnoreCase(raw, "DraStic")) return ProviderIdentity::DraStic;
    if (equalsIgnoreCase(raw, "DraStic Savestate")) return ProviderIdentity::DraSticSavestate;
    if (equalsIgnoreCase(raw, "melonDS")) return ProviderIdentity::MelonDS;
    if (equalsIgnoreCase(raw, "RetroArch")) return ProviderIdentity::RetroArch;
    if (equalsIgnoreCase(raw, "RetroArch/DeSmuME")) return ProviderIdentity::RetroArchDeSmuME;
    if (equalsIgnoreCase(raw, "mGBA")) return ProviderIdentity::MGBA;
    if (equalsIgnoreCase(raw, "Tico")) return ProviderIdentity::Tico;
    if (equalsIgnoreCase(raw, "Dekopon")) return ProviderIdentity::Dekopon;
    if (equalsIgnoreCase(raw, "Azahar")) return ProviderIdentity::Azahar;
    if (equalsIgnoreCase(raw, "Manual")) return ProviderIdentity::Manual;
    // "Remembered", "Source", an empty pre-provider binding row, or anything
    // hand-edited: discovery did not identify a provider. Say so honestly.
    return ProviderIdentity::Unknown;
}

[[nodiscard]] inline std::string_view providerDisplayName(ProviderIdentity id) noexcept {
    switch (id) {
        case ProviderIdentity::DraStic: return "DraStic";
        case ProviderIdentity::DraSticSavestate: return "DraStic (savestate)";
        case ProviderIdentity::MelonDS: return "melonDS";
        case ProviderIdentity::RetroArch: return "RetroArch";
        case ProviderIdentity::RetroArchDeSmuME: return "RetroArch (DeSmuME)";
        case ProviderIdentity::MGBA: return "mGBA";
        case ProviderIdentity::Tico: return "Tico";
        case ProviderIdentity::Dekopon: return "Dekopon";
        case ProviderIdentity::Azahar: return "Azahar";
        case ProviderIdentity::Manual: return "Manual file";
        case ProviderIdentity::Multiple: return "Multiple providers";
        case ProviderIdentity::Unknown: break;
    }
    return "Unknown provider";
}

[[nodiscard]] inline std::string providerDisplayName(std::string_view raw) {
    return std::string(providerDisplayName(identifyProvider(raw)));
}

// The provider a launch descriptor names: GameLaunchDescriptor::providerId is providerIdFor()
// of the discovery label ("RetroArch/DeSmuME" -> "retroarch-desmume"), so match it the same way.
[[nodiscard]] inline ProviderIdentity identifyProviderId(std::string_view id) {
    for (const std::string_view label : {"DraStic", "DraStic Savestate", "melonDS", "RetroArch",
                                         "RetroArch/DeSmuME", "mGBA", "Tico", "Dekopon", "Azahar", "Manual"})
        if (!id.empty() && providerIdFor(label) == id) return identifyProvider(label);
    return ProviderIdentity::Unknown;
}

// Session surfaces (Trainer / Game Browser header, read-only notices, leave dialog): the source's
// own provider name, the same one Product Home shows. An installed Switch title has no emulator
// provider: it is "Nintendo Switch". Never a generic "EXTERNAL" / "emulator" word.
[[nodiscard]] inline std::string sessionSourceName(std::string_view rawLabel, bool installedSwitchTitle) {
    if (installedSwitchTitle) return "Nintendo Switch";
    return providerDisplayName(rawLabel);
}

// One provider for a card that owns several validated instances (Gen I-III
// classic cards). Different providers are reported as such, never collapsed
// to whichever row happened to sort first.
[[nodiscard]] inline std::string cardProviderLabel(const std::vector<SaveInstance>& instances) {
    if (instances.empty()) return {};
    const std::string& first = instances.front().providerLabel;
    for (const auto& instance : instances)
        if (!ProviderDetail::equalsIgnoreCase(instance.providerLabel, first))
            return "Multiple providers";
    return first;
}

[[nodiscard]] inline std::string cardProviderDisplay(std::string_view raw) {
    if (raw == "Multiple providers") return std::string(raw);
    return providerDisplayName(raw);
}

// Home card / hero format: "Nintendo DS • DraStic".
inline constexpr std::string_view HomeSourceSeparator = " \xE2\x80\xA2 ";

[[nodiscard]] inline std::string homeSourceLine(std::string_view platformLabel,
                                                std::string_view providerOrState) {
    std::string out(platformLabel);
    if (!providerOrState.empty()) {
        if (!out.empty()) out += HomeSourceSeparator;
        out += providerOrState;
    }
    return out;
}

// Consumer text for a Home / Games card source slot. `state` is the card's
// raw state token; `provider` is the raw Save Instance provider. Unassigned,
// missing, invalid and ambiguous states stay distinct and name no provider.
// Every linked or discovered card shows its real provider (never a generic linked-save
// label), followed by any Gen V card state that still needs action.
[[nodiscard]] inline std::string homeCardSourceText(std::string_view state,
                                                    std::string_view provider) {
    if (state == "LOCAL SAVE") return "System save";
    if (state == "CHOOSE SAVE") return "Choose save";
    if (state == "MISSING") return "Missing source";
    if (state == "INVALID") return "Invalid source";
    if (state == "AMBIGUOUS") return "Needs attention";
    std::string out = cardProviderDisplay(provider);
    const auto note = [&](std::string_view text) {
        out += HomeSourceSeparator;
        out += text;
    };
    if (state.find("NEW-GAME") != std::string_view::npos) note("New game, no trainer yet");
    else if (state.find("CHOOSE COPY") != std::string_view::npos) note("Choose copy");
    else if (state.find("NO TRAINER DATA") != std::string_view::npos) note("No trainer data");
    else if (state.find("RELINK") != std::string_view::npos) note("Relink save");
    else if (state.find("STALE") != std::string_view::npos) note("Stale");
    return out;
}

// Save Instance row secondary line: provider first, then the native source
// description recorded by discovery (e.g. "DraStic / Battery save").
[[nodiscard]] inline std::string saveInstanceSourceLine(const SaveInstance& instance) {
    std::string line = providerDisplayName(instance.providerLabel);
    if (!instance.sourceLabel.empty()) line += " / " + instance.sourceLabel;
    return line;
}

// Measure-driven single-line fitting. `measure(text)` returns the rendered
// pixel width in the target style. Never splits a UTF-8 code point, and the
// result (including the ellipsis) never exceeds maxWidth; when even the
// ellipsis cannot fit, nothing is drawn.
template <class Measure>
[[nodiscard]] std::string ellipsizeToWidth(const std::string& text, int maxWidth, Measure measure) {
    if (maxWidth <= 0) return {};
    if (measure(text) <= maxWidth) return text;
    static const std::string ellipsis = "\xE2\x80\xA6";
    if (measure(ellipsis) > maxWidth) return {};
    // Binary search the longest code-point-aligned prefix that fits.
    std::vector<size_t> cuts;
    cuts.reserve(text.size());
    for (size_t i = 1; i <= text.size(); ++i)
        if (i == text.size() || (static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
            cuts.push_back(i);
    size_t lo = 0, hi = cuts.size();
    std::string best;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        std::string candidate = text.substr(0, cuts[mid]);
        while (!candidate.empty() && candidate.back() == ' ') candidate.pop_back();
        candidate += ellipsis;
        if (measure(candidate) <= maxWidth) {
            best = std::move(candidate);
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return best.empty() ? ellipsis : best;
}

} // namespace PokeVault::Source

#endif
