// Named Generation II picker layer. It intercepts only Species and Move activation before
// the preserved shared editor launches its legacy numeric prompts. Browsing lives entirely in this
// overlay model; the Gen2PokemonSession changes only when A explicitly applies a selected value.

#include "UI/Gen2PokemonPickerModel.h"
#include "UI/MovePickerPresentation.h"
#include "UI/TouchScroll.h"
#include "Utils/Keyboard.h"

namespace UI::Gen2PokemonEditor {
namespace {

namespace Gen2Picker = PokeBank::UIModel::Gen2PokemonPicker;
namespace Gen2Native = PokeBank::UIModel::Gen2Native;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;
namespace MoveUI = PokeBank::UIModel::MovePickerPresentation;

struct PickerOverlayState {
    const TrainerViewScreen* owner = nullptr;
    Gen2Picker::Model model;
    TouchScrollState scroll;
    TouchScrollState moveScroll;
    TouchPickerViewport moveViewport;
    TouchPickerViewport locationViewport;
};

PickerOverlayState& pickerStateFor(const TrainerViewScreen& screen) {
    static PickerOverlayState picker;
    if (picker.owner != &screen) {
        picker = {};
        picker.owner = &screen;
    }
    return picker;
}

void closePicker(TrainerViewScreen& screen, PickerOverlayState& picker) {
    picker.model.close();
    picker.scroll.reset();
    picker.moveScroll.reset();
    picker.moveViewport.reset();
    picker.locationViewport.reset();
    screen.controllerNavigation.reset();
}

bool openPickerForFocusedField(TrainerViewScreen& screen, State& state,
                               PickerOverlayState& picker, uint64_t down) {
    if (!(down & HidNpadButton_A) || state.itemPicker || state.confirmExit) return false;
    if (state.panel == Foundation::Panel::Identity && state.row == 0) {
        picker.model.openSpecies(state.working.species, state.working.shiny);
    } else if (state.panel == Foundation::Panel::Moves && state.column == 0) {
        picker.model.openMove(state.working.moves[static_cast<std::size_t>(state.row)],
                              static_cast<std::size_t>(state.row));
    } else {
        return false;
    }
    picker.scroll.reset();
    picker.moveScroll.reset();
    picker.moveViewport.reset();
    picker.locationViewport.reset();
    screen.controllerNavigation.reset();
    return true;
}

bool applyPickerChoice(TrainerViewScreen& screen, State& state, PickerOverlayState& picker) {
    if (picker.model.kind == Gen2Picker::Kind::Species) {
        if (!Gen2Picker::applySpeciesAppearance(
                state, picker.model.speciesChoice(), picker.model.previewShiny,
                screen.sourceGameId)) return false;
        refreshDerived(state.working);
        if (screen.sourceGameId == "crystal_gbc") {
            const auto caught = Gen2Native::decodeCrystalCaughtData(state.working.caughtData);
            if (caught.present &&
                !Encounter::locationAllowed(screen.sourceGameId, state.working.species, caught.location)) {
                state.working.caughtData = 0;
                screen.postStatus("Species changed; choose a new legitimate Crystal encounter", 300);
            }
        }
    } else if (picker.model.kind == Gen2Picker::Kind::Move) {
        if (!Gen2Picker::applyMoveChoice(state, picker.model.moveSlot, picker.model.moveChoice())) return false;
    } else if (picker.model.kind == Gen2Picker::Kind::Location) {
        if (!state.editable() || screen.sourceGameId != "crystal_gbc") return false;
        const auto* encounter = picker.model.locationChoice();
        if (!encounter) return false;
        auto caught = Gen2Native::decodeCrystalCaughtData(state.working.caughtData);
        caught.present = true;
        caught.location = static_cast<uint8_t>(encounter->location);
        caught.levelCode = encounter->method == Encounter::Method::Egg
            ? 1 : static_cast<uint8_t>(std::clamp<int>(encounter->minLevel, 2, 63));
        uint8_t resolvedMetTime = state.crystalMetTime;
        if (resolvedMetTime < 1 || resolvedMetTime > 3 ||
            !Encounter::timeAllowed(*encounter, resolvedMetTime)) {
            for (uint8_t value = 1; value <= 3; ++value) {
                if (Encounter::timeAllowed(*encounter, value)) {
                    resolvedMetTime = value;
                    break;
                }
            }
        }
        state.crystalMetTime = resolvedMetTime;
        caught.timeOfDay = resolvedMetTime;
        // A real Crystal caught record must encode Male or Female. If the user already
        // chose one while caught history was unset, preserve it. Otherwise use the
        // authentic clear-bit Male encoding as the deterministic default.
        if (state.crystalOtGender == Gen2Native::CrystalOtGenderChoice::Unknown)
            state.crystalOtGender = Gen2Native::CrystalOtGenderChoice::Male;
        caught.originalTrainerFemale = Gen2Native::crystalOtGenderFemale(state.crystalOtGender);
        state.working.caughtData = Gen2Native::encodeCrystalCaughtData(caught);
    } else {
        return false;
    }
    closePicker(screen, picker);
    return true;
}

const char* movePickerStatus(const TrainerViewScreen& screen, const State& state, uint16_t move) {
    if (move == 0) return "Empty";
    const auto game = Rules::exactGameFromSourceId(screen.sourceGameId);
    if (game && Gen2::MoveCompatibility::canLearnMove(*game, state.working.species, move)) return "Compatible";
    const auto slot = pickerStateFor(screen).model.moveSlot;
    if (state.mode == Mode::Edit && state.working.species == state.baseline.species &&
        slot < state.baseline.moves.size() && state.baseline.moves[slot] == move)
        return "Unusual preserved";
    return "Needs correction";
}

Color movePickerStatusColor(const char* status) {
    const std::string value(status);
    if (value == "Compatible") return Colors::Success;
    if (value == "Empty") return Colors::TextDim;
    if (value == "Unusual preserved") return Colors::Warning;
    return Colors::Error;
}

void drawPickerRow(PKSEFramebuffer& fb, int x, int y, int w, const std::string& text,
                   bool selected, Color textColor = Colors::Text) {
    if (selected) {
        fb.drawFilledRoundedRect(x, y - 5, w, 30, 8, Colors::SurfaceSelected);
        fb.drawRoundedRect(x, y - 5, w, 30, 8, Colors::FocusBorder, 1);
    }
    fb.drawText(x + 12, y, text, selected ? Colors::SelectedText : textColor, TextStyle::Caption);
}

void drawListPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state,
                    const Gen2Picker::Model& model) {
    screen.touchButtons.clear();
    auto& overlay = pickerStateFor(screen);
    if (model.kind == Gen2Picker::Kind::Species) {
        const auto modal = SharedSpeciesPicker::drawModalChrome(fb);
        const int speciesStart = std::clamp(model.speciesChoice() - 9 / 2, 1, 251 - 9 + 1);
        for (int i = 0; i < 9; ++i)
            screen.touchButtons.push_back({2000 + speciesStart + i,
                                           modal.x + 20, modal.y + 84 + i * 43,
                                           560, 39});
        SharedSpeciesPicker::drawContent(
            fb, modal.x, modal.y, model.speciesChoice(), 251, model.previewShiny,
            [](uint16_t species) {
                char prefix[16]{};
                std::snprintf(prefix, sizeof(prefix), "%03u - ", static_cast<unsigned>(species));
                return std::string(prefix) + Names::getSpeciesName(species);
            },
            [](uint16_t species) { return std::to_string(species) + " - " + Names::getSpeciesName(species); },
            &latestTouchGesture());
        drawNavBar(fb, {{"D-pad/Stick", "Browse"}, {"L/R", "Page"}, {"Y", "Normal/Shiny"}, {"A", "Select"}, {"B", "Cancel"}});
        return;
    }
    const int W = fb.getWidth(), H = fb.getHeight();
    constexpr int panelH = 568;
    const int panelW = model.kind == Gen2Picker::Kind::Move ? 1080 : 760;
    const int x = (W - panelW) / 2, y = (H - panelH) / 2;
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    drawPanelSurface(fb, x, y, panelW, panelH, false, 18);

    const bool species = model.kind == Gen2Picker::Kind::Species;
    const bool location = model.kind == Gen2Picker::Kind::Location;
    fb.drawText(x + 24, y + 18, location ? "Met location — Crystal" : species ? "Species — Generation II" : "Move — Generation II",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                location ? "Drag naturally to browse Crystal encounters • tap to choose" : species ? "#001–#251 • browsing does not change the staged Pokemon"
                        : "Drag naturally to browse • exact Gen II Acc / Pwr / PP • compatibility at right",
                Colors::TextDim, TextStyle::Caption);

    const int count = location ? model.locationCount() : species ? 251 : 252;
    constexpr int visible = 13;
    constexpr int rowStep = 33;
    if (location)
        overlay.locationViewport.ensure(model.index, count, visible);
    const int first = location ? overlay.locationViewport.firstRow :
        std::clamp(model.index - visible / 2, 0, std::max(0, count - visible));
    const int liveOffset = overlay.scroll.offset();
    const int firstDraw = std::max(0, first - 1);
    const int lastDraw = std::min(count, first + visible + 1);
    const int listY = y + 88;
    fb.setClipRect(x + 20, listY - 5, panelW - 40, visible * rowStep + 8);
    for (int i = firstDraw; i < lastDraw; ++i) {
        const int yy = listY + (i - first) * rowStep + liveOffset;
        const bool selected = i == model.index;
        appendClippedTouchButton(
            screen.touchButtons, 2000 + i, x + 20, yy - 5, panelW - 40, 30,
            x + 20, listY - 5, panelW - 40, visible * rowStep + 8);
        if (location) {
            const auto& encounter = model.encounterChoices[static_cast<std::size_t>(i)];
            std::string label = std::string(Gen2Native::crystalCaughtLocationName(
                static_cast<uint8_t>(encounter.location))) + " — " +
                Encounter::methodName(encounter.method) + " — Lv. " +
                std::to_string(encounter.minLevel);
            if (encounter.maxLevel != encounter.minLevel)
                label += "–" + std::to_string(encounter.maxLevel);
            const auto time = Gen2Picker::encounterTimeLabel(encounter.timeMask);
            if (!time.empty()) label += " — " + time;
            drawPickerRow(fb, x + 20, yy, panelW - 40, label, selected);
        } else if (species) {
            const uint16_t dex = static_cast<uint16_t>(i + 1);
            std::string number = std::to_string(dex);
            while (number.size() < 3) number = "0" + number;
            drawPickerRow(fb, x + 20, yy, panelW - 40,
                          "#" + number + "  " + Names::getSpeciesName(dex), selected);
        } else {
            const uint16_t move = static_cast<uint16_t>(i);
            const auto name = MoveUI::rowLabel(move, Enums::GameVersion::GSC);
            drawPickerRow(fb, x + 20, yy, panelW - 40, name, selected);
            const char* status = movePickerStatus(screen, state, move);
            int sw = 0, sh = 0;
            fb.measureText(status, sw, sh, TextStyle::Caption);
            fb.drawText(x + panelW - 34 - sw, yy, status, movePickerStatusColor(status), TextStyle::Caption);
        }
    }
    fb.clearClip();
    drawNavBar(fb, {{"D-pad/Stick", "Browse"}, {"Left/Right", "Page"}, {"A", "Select"}, {"B", "Cancel"}});
}


} // namespace

bool handlePickerInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                       int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    auto& state = stateFor(screen);
    auto& picker = pickerStateFor(screen);

    if (state.mode == Mode::View) {
        if (PokeBank::UIModel::passiveViewAction(false, screen.touchedButtonId(touch)) ==
            PokeBank::UIModel::PassiveViewAction::Back) {
            closeToActions(screen);
            return true;
        }
        if (picker.model.active()) picker.model.close();
        picker.scroll.reset();
        return false;
    }
    if (state.mode == Mode::None) {
        if (picker.model.active()) picker.model.close();
        picker.scroll.reset();
        return false;
    }

    if (!picker.model.active())
        return openPickerForFocusedField(screen, state, picker, down);

    const int pickerCount = picker.model.kind == Gen2Picker::Kind::Species
        ? 251 : picker.model.kind == Gen2Picker::Kind::Location ? picker.model.locationCount() : 252;
    if (picker.model.kind == Gen2Picker::Kind::Species) {
        // The shared Species renderer follows the finger pixel-for-pixel while held. Commit the
        // semantic preview cursor only on release so the gesture cannot accidentally activate A.
        const int dy = touch.deltaY();
        if (touch.justReleased() && touch.dragged() &&
            SharedSpeciesPicker::gestureStartedInList(touch.startX(), touch.startY()) &&
            std::abs(dy) >= 24 && std::abs(dy) > std::abs(touch.deltaX())) {
            const int steps = std::max(1, std::abs(dy) / 43);
            picker.model.stepList(dy < 0 ? steps : -steps);
        }
    } else {
        constexpr int panelH = 568;
        const int panelW = picker.model.kind == Gen2Picker::Kind::Move ? 1080 : 760;
        const int x = (1280 - panelW) / 2;
        const int y = (720 - panelH) / 2;
        if (picker.model.kind == Gen2Picker::Kind::Location) {
            constexpr int visible = 13;
            picker.locationViewport.ensure(picker.model.index, pickerCount, visible);
            const int maxFirst = TouchPickerViewport::maxFirst(pickerCount, visible);
            if (maxFirst > 0)
                picker.scroll.updateVertical(
                    touch, x + 20, y + 83, panelW - 40, visible * 33 + 10, 33,
                    picker.locationViewport.firstRow, maxFirst + 1);
            else picker.scroll.stop();
        } else {
            picker.scroll.updateVertical(touch, x + 20, y + 83, panelW - 40, 13 * 33 + 10,
                                         33, picker.model.index, pickerCount);
        }
    }

    // Species target IDs encode the 1-based National Dex; Location/Move IDs are 0-based.
    // Reject unrelated modal button IDs rather than clamping them to the last choice.
    const bool speciesIds = picker.model.kind == Gen2Picker::Kind::Species;
    const int firstTouchId = speciesIds ? 2001 : 2000;
    const int endTouchId = 2000 + pickerCount + (speciesIds ? 1 : 0);
    int touchDown = screen.touchedButtonDownId(touch);
    if (touchDown >= firstTouchId && touchDown < endTouchId) {
        if (picker.model.kind == Gen2Picker::Kind::Species)
            picker.model.index = std::clamp(touchDown - 2000, 1, 251) - 1;
        else
            picker.model.index = std::clamp(touchDown - 2000, 0, std::max(0, pickerCount - 1));
    }
    int touchTap = screen.touchedButtonId(touch);
    if (touchTap >= firstTouchId && touchTap < endTouchId) {
        if (picker.model.kind == Gen2Picker::Kind::Species)
            picker.model.index = std::clamp(touchTap - 2000, 1, 251) - 1;
        else
            picker.model.index = std::clamp(touchTap - 2000, 0, std::max(0, pickerCount - 1));
        down |= HidNpadButton_A;
    }

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (down & HidNpadButton_B) {
        closePicker(screen, picker);
        return true;
    }

    if (picker.model.kind == Gen2Picker::Kind::Species ||
        picker.model.kind == Gen2Picker::Kind::Move || picker.model.kind == Gen2Picker::Kind::Location) {
        if (nav & HidNpadButton_Up) { picker.scroll.stop(); picker.model.stepList(-1); }
        if (nav & HidNpadButton_Down) { picker.scroll.stop(); picker.model.stepList(1); }
        if (picker.model.kind == Gen2Picker::Kind::Species) {
            if ((down & HidNpadButton_L) || (nav & HidNpadButton_Left)) { picker.scroll.stop(); picker.model.stepList(-10); }
            if ((down & HidNpadButton_R) || (nav & HidNpadButton_Right)) { picker.scroll.stop(); picker.model.stepList(10); }
            if (down & HidNpadButton_Y) picker.model.previewShiny = !picker.model.previewShiny;
        } else {
            if (nav & HidNpadButton_Left) { picker.scroll.stop(); picker.model.stepList(-10); }
            if (nav & HidNpadButton_Right) { picker.scroll.stop(); picker.model.stepList(10); }
        }
    }

    if (picker.model.kind == Gen2Picker::Kind::Location) {
        constexpr int visible = 13;
        if (nav & (HidNpadButton_Up | HidNpadButton_Down |
                   HidNpadButton_Left | HidNpadButton_Right))
            picker.locationViewport.reveal(picker.model.index, pickerCount, visible);
        if ((down & HidNpadButton_A) &&
            !picker.locationViewport.containsSelection(picker.model.index, pickerCount, visible)) {
            picker.scroll.stop();
            picker.locationViewport.reveal(picker.model.index, pickerCount, visible);
            screen.postStatus("Focused encounter brought into view; press A again or tap a visible row", 220);
            return true;
        }
    }
    if (down & HidNpadButton_A) applyPickerChoice(screen, state, picker);
    return true;
}

void drawPickerOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    const auto& state = stateFor(screen);
    if (state.mode == Mode::View) {
        screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget,
                                       16, fb.getHeight() - 68, 190, 58});
        return;
    }
    const auto& picker = pickerStateFor(screen).model;
    if (!picker.active()) return;
    drawListPicker(screen, fb, state, picker);
}

} // namespace UI::Gen2PokemonEditor
