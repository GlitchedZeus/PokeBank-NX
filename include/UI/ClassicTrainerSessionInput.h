#pragma once
#include "UI/ClassicTrainerEditability.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/Keyboard.h"

#include <string>

// One Trainer-page session flow for the classic staged editors (Gen I and Gen II). The page
// draws screen.trainer's Name / Money; the backend's staged values are mirrored into it every
// frame, so Review / Discard / Export always match what is shown. A on a row opens that field's
// native prompt; an accepted value is staged through `stage` (never written to the source save).
namespace UI::ClassicTrainerSession {

struct Fields {
    std::string name;
    uint32_t money = 0;
    int nameMax = 7;
    uint32_t moneyMax = 0;
    const char* nameGuide = "";
    bool nameEditable = true;
    const char* nameReadOnlyReason = "";
};

// Returns true when it consumed the frame. stage(name, money, error) -> bool.
template <class Stage>
bool handle(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
            const TouchInput& touch, const Fields& fields, Stage&& stage) {
    using PokeBank::UIModel::ClassicTrainerSession::kRows;
    screen.trainer.trainerName = fields.name;
    screen.trainer.money = fields.money;
    const int touched = screen.touchedButtonId(touch);
    if (touched >= 0 && touched < kRows) {
        screen.trainerSelectedRow = touched;
        down |= HidNpadButton_A;
    }
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & (HidNpadButton_Up | HidNpadButton_Down)) {
        screen.trainerSelectedRow = (screen.trainerSelectedRow + 1) % kRows;
        return true;
    }
    if (!(down & HidNpadButton_A)) return false;
    std::string name = fields.name;
    uint32_t money = fields.money;
    bool accepted = false;
    if (screen.trainerSelectedRow == 0) {
        if (!fields.nameEditable) {
            screen.postStatus(fields.nameReadOnlyReason, 300);
            return true;
        }
        const auto result = Utils::promptText("Trainer Name", fields.nameGuide, name, fields.nameMax);
        accepted = result.accepted && result.text != name;
        if (accepted) name = result.text;
    } else {
        const auto result = Utils::promptNumber("Money", static_cast<int>(money), 0,
                                                static_cast<int>(fields.moneyMax));
        accepted = result.accepted && static_cast<uint32_t>(result.value) != money;
        if (accepted) money = static_cast<uint32_t>(result.value);
    }
    if (accepted) {
        std::string error;
        if (stage(name, money, error)) screen.postStatus("Trainer edit staged; source save unchanged", 240);
        else screen.postStatus(error, 300);
    }
    return true;
}

} // namespace UI::ClassicTrainerSession
