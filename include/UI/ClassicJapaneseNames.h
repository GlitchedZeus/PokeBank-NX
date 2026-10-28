#pragma once
// Japanese Gen I / II name prompts (owner Oct 28). On a Japanese save every Nickname / OT / Trainer
// name prompt opens the kana keyboard (Utils::promptJapaneseName: あ / ア / １２３ pages, 5
// characters); elsewhere it is the ordinary prompt, unchanged. The writers still validate every
// name (Utils::Gen12Kana::encode). While Integration/Common/JapaneseEditingGate.h is closed no
// Japanese Gen I / II editor exists, so these prompts are only reached once the owner opens it.

#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Utils/Gen12KanaText.h"
#include "Utils/Keyboard.h"

#include <algorithm>
#include <string>

namespace UI::ClassicJapaneseNames {

[[nodiscard]] inline bool japaneseSave(const Trainer::Trainer& trainer) noexcept {
    if (const auto* rby = dynamic_cast<const PokeVault::Legacy::RBYReadOnlyTrainer*>(&trainer))
        return rby->japaneseLayout();
    if (const auto* gsc = dynamic_cast<const PokeVault::Legacy::GSCReadOnlyTrainer*>(&trainer))
        return gsc->japaneseLayout();
    return false;
}

// The guide a Japanese prompt shows instead of the Latin "1-10 ... characters" text.
inline constexpr const char* kKanaGuide = "1-5 kana (あ / ア / １２３ pages)";

[[nodiscard]] inline Utils::KeyboardResult prompt(const Trainer::Trainer& trainer, const std::string& header,
                                                  const std::string& guide, const std::string& initial,
                                                  int maxChars) {
    if (!japaneseSave(trainer)) return Utils::promptText(header, guide, initial, maxChars);
    return Utils::promptJapaneseName(header, initial,
                                     std::min<int>(maxChars, static_cast<int>(Utils::Gen12Kana::kMaxNameChars)));
}

}  // namespace UI::ClassicJapaneseNames
