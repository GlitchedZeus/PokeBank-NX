#pragma once
// R12-010 (owner Round 12 screenshots 2026110114225300_c / 2026110114253600_c): a postStatus toast
// belongs to the context it was raised in. "Exported to .../Pidgeotto_*.pk4" lived 600 frames and
// was still painted over Arbok's Met Location picker, where it read as a message about Arbok.
//
// The shared rule, used by TrainerViewScreen's one status line (so every generation inherits it):
//   * Each frame, the shared draw primitives record the LAYERS they draw: every modal
//     (SharedEditorChrome::beginModal, keyed by its call site), every Pokemon Actions sheet
//     (ScreenChrome::drawNarrowPokemonActions, keyed by its context line) and every Pokemon editor
//     (SharedPokemonShell::drawChrome and the Gen I / II editor headers, keyed by the record they
//     show: "Box 1 / Slot 3", "Party 2" ...).
//   * A status takes as its scope the layers of the first frame drawn after it was posted (so a
//     refusal raised inside a picker, or a result raised as a modal opens, belongs to that modal).
//   * Layers that close drop out of the scope (closing a modal keeps its result).
//   * A layer that is NOT in the scope appearing (a different Pokemon's sheet or editor, a new
//     picker or confirmation) ends the status: it is no longer drawn.
// Nothing here posts, writes or confirms anything; it only decides whether an old line is drawn.
// Platform-neutral (no Switch headers): host tests drive it through the real TrainerViewScreen.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <string_view>
#include <vector>

namespace UI::TransientStatusScope {

using Key = std::uint64_t;
using Layers = std::vector<Key>;

// Bounded: a screen that draws modals without ever ending a frame cannot grow this forever.
inline constexpr std::size_t kMaxLayers = 32;

[[nodiscard]] constexpr Key mix(Key hash, std::string_view text) noexcept {
    for (const char c : text) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ull;   // FNV-1a
    }
    return hash;
}
[[nodiscard]] constexpr Key key(std::string_view kind, std::string_view identity,
                                std::uint32_t number = 0) noexcept {
    Key hash = mix(14695981039346656037ull, kind);
    hash = mix(hash, "|");
    hash = mix(hash, identity);
    for (int i = 0; i < 4; ++i) {
        hash ^= (number >> (i * 8)) & 0xFFu;
        hash *= 1099511628211ull;
    }
    return hash;
}

// The layers drawn so far in the current frame.
inline Layers& frameLayers() noexcept {
    static Layers layers;
    return layers;
}
// Start of a TrainerViewScreen frame.
inline void beginFrame() noexcept { frameLayers().clear(); }

inline void noteLayer(Key layer) {
    auto& layers = frameLayers();
    if (std::find(layers.begin(), layers.end(), layer) != layers.end()) return;
    if (layers.size() >= kMaxLayers) return;
    layers.push_back(layer);
}
// A modal, identified by the call site that drew it.
inline void noteModal(std::source_location where = std::source_location::current()) {
    noteLayer(key("modal", where.file_name(), where.line()));
}
// A Pokemon Actions sheet, identified by its context line (which names the slot).
inline void noteActions(std::string_view context) { noteLayer(key("actions", context)); }
// A Pokemon editor, identified by the record it shows ("Box 1 / Slot 3", "Party 2").
inline void noteEditor(std::string_view record) { noteLayer(key("editor", record)); }

// The per-status bookkeeping TrainerViewScreen keeps next to storageStatus.
struct Scope {
    Layers layers;          // the status's context
    Key text = 0;           // which message the scope belongs to
    int frames = 0;         // frames left when last scoped (a re-post raises it)
    bool armed = false;
};

// Called once per frame, after every layer was drawn. Returns false when the status must no
// longer be drawn (a layer outside its scope opened); the caller then ends the status.
[[nodiscard]] inline bool keep(Scope& scope, std::string_view status, int framesLeft,
                               const Layers& drawn) {
    if (framesLeft <= 0 || status.empty()) {
        scope = {};
        return true;
    }
    const Key text = key("status", status);
    const bool fresh = !scope.armed || text != scope.text || framesLeft > scope.frames;
    scope.text = text;
    scope.frames = framesLeft;
    scope.armed = true;
    if (fresh) {
        scope.layers = drawn;
        return true;
    }
    for (const Key layer : drawn)
        if (std::find(scope.layers.begin(), scope.layers.end(), layer) == scope.layers.end()) {
            scope = {};
            return false;
        }
    // Closed layers leave the scope: reopening one later is a new context.
    std::erase_if(scope.layers, [&](Key layer) {
        return std::find(drawn.begin(), drawn.end(), layer) == drawn.end();
    });
    return true;
}

} // namespace UI::TransientStatusScope
