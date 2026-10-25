#pragma once
// Host UI harness (never shipped): drives the REAL TrainerViewScreen::update / draw on the host with
// scripted controller and touch input. Build: Makefile.host.host-ui (the app's own sources, linked
// against tests/host-ui/switch.h and HostUiStubs.cpp). Each frame mirrors the device loop: input ->
// update(pad, touch) -> draw(fb), so a tap hit-tests the touch targets the previous draw published.
#include <switch.h>

#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "Utils/Keyboard.h"

namespace HostUi {

inline std::vector<std::string> texts;                 // every string the last draw() rendered
inline std::deque<Utils::NumberResult> numbers;         // answers for promptNumber, in order
inline std::deque<Utils::KeyboardResult> textAnswers;  // answers for promptText / promptSystemText
inline int promptCount = 0;                            // prompts shown so far

inline Utils::NumberResult nextNumber() {
    ++promptCount;
    if (numbers.empty()) return {};
    auto next = numbers.front();
    numbers.pop_front();
    return next;
}
inline Utils::KeyboardResult nextText() {
    ++promptCount;
    if (textAnswers.empty()) return {};
    auto next = textAnswers.front();
    textAnswers.pop_front();
    return next;
}

} // namespace HostUi
