#pragma once

// R2-LAUNCH: text layout of Product Home's leave decision ("Quit PokeBank NX?" / "Launch <game>?").
// The frame, message lines and in-frame button row come from SharedMenuGeometry's LaunchConfirm
// rule (content-driven, no fixed oversized box). This header only decides WHAT goes on its three
// message lines: the consequence sentence, then the emulator quit-route notice word-wrapped with
// the bundled font's real advances (NunitoMetrics via SharedMenuGeometry::textWidth), so nothing
// is clipped. Buttons, left to right: B Back, A Stay, Y Launch / Quit. Only Y commits (frozen).
// Pure and host-tested (tests/test_leave_decision_layout.cpp).

#include "UI/SharedMenuGeometry.h"

#include <array>
#include <string>
#include <string_view>

namespace PokeBank::UIModel::LeaveDecisionLayout {

namespace G = SharedMenuGeometry;

inline constexpr int kLines = 3; // == rule(LaunchConfirm).bodyLines

struct Text {
    std::string heading;
    std::array<std::string, kLines> lines{};
    G::Text lineStyle[kLines] = {G::Text::Body, G::Text::Caption, G::Text::Caption};
    bool fits = true; // every line (and the heading) fits the frame's text column
};

// Text column of the LaunchConfirm frame.
[[nodiscard]] constexpr int textColumnWidth() noexcept {
    const G::Layout l = G::layout(G::MenuType::LaunchConfirm, 0);
    return l.frame.w - 2 * l.rule.padX;
}

// Greedy word wrap of `text` into at most `maxLines` lines of `width` px; returns lines used, or
// maxLines + 1 when it does not fit.
inline int wrap(std::string_view text, G::Text style, int width, std::string* out, int maxLines) {
    int used = 0;
    std::string line;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && text[i] == ' ') ++i;
        std::size_t j = i;
        while (j < text.size() && text[j] != ' ') ++j;
        if (j == i) break;
        const std::string word(text.substr(i, j - i));
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (G::fitsWidth(candidate, style, width)) {
            line = candidate;
        } else {
            if (!line.empty()) {
                if (used >= maxLines) return maxLines + 1;
                out[used++] = line;
            }
            line = word;
            if (!G::fitsWidth(line, style, width)) return maxLines + 1;
        }
        i = j;
    }
    if (!line.empty()) {
        if (used >= maxLines) return maxLines + 1;
        out[used++] = line;
    }
    return used;
}

// Who takes over after a launch, as the consequence line names it: an installed Switch title is
// "the game"; an emulator is named by its provider (Product Home's name); only a launch whose
// provider discovery did not identify says "the emulator".
[[nodiscard]] inline std::string launchStarter(bool switchTitle, std::string_view providerDisplay) {
    if (switchTitle) return "the game";
    if (providerDisplay.empty() || providerDisplay == "Unknown provider" ||
        providerDisplay == "Multiple providers")
        return "the emulator";
    return std::string(providerDisplay);
}

[[nodiscard]] inline Text build(bool quitting, std::string_view gameName, std::string_view notice,
                                std::string_view starter = "the emulator") {
    Text t;
    const int width = textColumnWidth();
    t.heading = quitting ? std::string("Quit PokeBank NX?") : "Launch " + std::string(gameName) + "?";
    t.lines[0] = quitting ? std::string("PokeBank NX will close.")
                          : "PokeBank NX closes and " + std::string(starter) + " starts.";
    const std::string_view rest = quitting
        ? std::string_view("Source saves are never written; staged copies stay on the SD card.")
        : notice;
    const int used = wrap(rest, G::Text::Caption, width, t.lines.data() + 1, kLines - 1);
    t.fits = used <= kLines - 1 && G::fitsWidth(t.heading, G::Text::Heading, width) &&
             G::fitsWidth(t.lines[0], G::Text::Body, width);
    return t;
}

} // namespace PokeBank::UIModel::LeaveDecisionLayout
