#pragma once
#include "UI/ClassicTrainerEditability.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/Keyboard.h"

#include "Integration/Common/NativePlayTime.h"

#include <algorithm>
#include <functional>
#include <optional>
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
    // Japanese Gen I / II save: the name prompt opens the kana keyboard (5 characters).
    bool nameJapanese = false;
    // P3 (Round 5): optional Play Time and Gender rows. Each is offered only when its stager is set,
    // i.e. when the generation has a source-backed native writer; accepted values are staged in the
    // same workspace as Name / Money (Review lists them, Discard restores them, Export writes them).
    std::optional<PokeVault::Integration::Common::NativePlayTime> playTime;
    uint16_t playTimeMaxHours = 999;
    std::function<bool(const PokeVault::Integration::Common::NativePlayTime&, std::string&)> stagePlayTime;
    uint8_t gender = 0;
    std::function<bool(uint8_t, std::string&)> stageGender;
};

// Returns true when it consumed the frame. stage(name, money, error) -> bool.
template <class Stage, class Review>
bool handle(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
            const TouchInput& touch, const Fields& fields, Stage&& stage, Review&& review) {
    namespace Rows = PokeBank::UIModel::ClassicTrainerSession;
    const bool playTimeRow = static_cast<bool>(fields.stagePlayTime) && fields.playTime.has_value();
    const bool genderRow = static_cast<bool>(fields.stageGender);
    const int kRows = Rows::rowCount(playTimeRow, genderRow);
    screen.trainer.trainerName = fields.name;
    screen.trainer.money = fields.money;
    screen.trainerPlayTimeEditable = playTimeRow;
    screen.trainerGenderEditable = genderRow;
    if (playTimeRow) screen.trainer.nativePlayTime = fields.playTime;
    if (genderRow) screen.trainer.trainerGender = fields.gender;
    if (screen.trainerSelectedRow >= kRows) screen.trainerSelectedRow = 0;
    const int touched = screen.touchedButtonId(touch);
    // Y Review, the Trainer page's one Review key (ReviewEntry.h, Round 7: + is not a second one).
    if (!(down & HidNpadButton_B) && ((down & HidNpadButton_Y) ||
        touched == PokeBank::UIModel::ClassicTrainerSession::kReviewTouchId)) {
        review();
        screen.controllerNavigation.reset();
        return true;
    }
    if (touched >= 0 && touched < kRows) {
        screen.trainerSelectedRow = touched;
        down |= HidNpadButton_A;
    }
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) {
        screen.trainerSelectedRow = (screen.trainerSelectedRow + kRows - 1) % kRows;
        return true;
    }
    if (nav & HidNpadButton_Down) {
        screen.trainerSelectedRow = (screen.trainerSelectedRow + 1) % kRows;
        return true;
    }
    if (!(down & HidNpadButton_A)) return false;
    const auto staged = [&](bool ok, const std::string& error) {
        if (ok) screen.postStatus("Trainer edit staged; source save unchanged", 240);
        else screen.postStatus(error, 300);
        return true;
    };
    if (playTimeRow && screen.trainerSelectedRow == Rows::kPlayTimeRow) {
        auto time = *fields.playTime;
        const auto h = Utils::promptNumber("Play Time - Hours", std::min<int>(time.hours, fields.playTimeMaxHours),
                                           0, fields.playTimeMaxHours);
        if (!h.accepted) return true;
        const auto m = Utils::promptNumber("Play Time - Minutes", time.minutes, 0, 59);
        if (!m.accepted) return true;
        const auto sec = Utils::promptNumber("Play Time - Seconds", time.seconds, 0, 59);
        if (!sec.accepted) return true;
        const PokeVault::Integration::Common::NativePlayTime next{
            static_cast<uint16_t>(h.value), static_cast<uint8_t>(m.value), static_cast<uint8_t>(sec.value), false};
        if (next.hours == time.hours && next.minutes == time.minutes && next.seconds == time.seconds) return true;
        std::string error;
        return staged(fields.stagePlayTime(next, error), error);
    }
    if (genderRow && screen.trainerSelectedRow == Rows::genderRow(playTimeRow)) {
        // The native choice is Male / Female; A switches it (the shared inline-cycle rule).
        std::string error;
        return staged(fields.stageGender(fields.gender == 0 ? 1 : 0, error), error);
    }
    std::string name = fields.name;
    uint32_t money = fields.money;
    bool accepted = false;
    if (screen.trainerSelectedRow == 0) {
        if (!fields.nameEditable) {
            screen.postStatus(fields.nameReadOnlyReason, 300);
            return true;
        }
        const auto result = fields.nameJapanese
            ? Utils::promptJapaneseName("Trainer Name", name, std::min(fields.nameMax, 5))
            : Utils::promptText("Trainer Name", fields.nameGuide, name, fields.nameMax);
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
