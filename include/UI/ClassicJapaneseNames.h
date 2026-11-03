#pragma once
// Gen I / II name prompts (owner Oct 28; Round 15 shared keyboard). Every Nickname / OT / Trainer
// name prompt of a Gen I / II save goes through Utils::promptName with the field's capability
// (Text/NameFields.h Gen12NameField): on a Japanese save the kana pages (あ / ア / １２３, 5
// characters, Utils/Gen12KanaText.h), elsewhere Latin + the font's symbols + the European accents,
// with every key the generation's own encoder refuses disabled. The writers still validate every
// name. While Integration/Common/JapaneseEditingGate.h is closed no Japanese Gen I / II editor
// exists, so the kana prompts are only reached once the owner opens it.

#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Text/NameFields.h"
#include "Utils/Gen12KanaText.h"
#include "Utils/Keyboard.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>

namespace UI::ClassicJapaneseNames {

// The device build has no RTTI (-fno-rtti), so the trainer's type comes from the source game id,
// exactly as rbyTrainer() / gscTrainer() decide it, and is then reached with static_cast.
[[nodiscard]] inline bool gen1Id(std::string_view id) noexcept {
    return id == "red_gb" || id == "blue_gb" || id == "yellow_gb";
}
[[nodiscard]] inline bool gen2Id(std::string_view id) noexcept {
    return id == "gold_gbc" || id == "silver_gbc" || id == "crystal_gbc";
}
[[nodiscard]] inline bool japaneseSave(const Trainer::Trainer& trainer, std::string_view sourceGameId) noexcept {
    if (gen1Id(sourceGameId))
        return static_cast<const PokeVault::Legacy::RBYReadOnlyTrainer&>(trainer).japaneseLayout();
    if (gen2Id(sourceGameId))
        return static_cast<const PokeVault::Legacy::GSCReadOnlyTrainer&>(trainer).japaneseLayout();
    return false;
}

// The capability of one Gen I / II name field of this save (Text/NameFields.h).
[[nodiscard]] inline std::unique_ptr<PokeBank::Text::TextFieldCapability> field(
    const Trainer::Trainer& trainer, std::string_view sourceGameId, PokeBank::Text::FieldKind kind) {
    return PokeBank::Text::makeNameField({gen2Id(sourceGameId) ? PokeBank::Text::Format::Gen2
                                                               : PokeBank::Text::Format::Gen1,
                                          kind, 2, japaneseSave(trainer, sourceGameId)});
}

// The one Gen I / II name prompt: Utils::promptName with that field.
[[nodiscard]] inline Utils::KeyboardResult promptName(const Trainer::Trainer& trainer, std::string_view sourceGameId,
                                                      PokeBank::Text::FieldKind kind, const std::string& header,
                                                      const std::string& initial) {
    const auto capability = field(trainer, sourceGameId, kind);
    return Utils::promptName(*capability, header, initial);
}

}  // namespace UI::ClassicJapaneseNames
