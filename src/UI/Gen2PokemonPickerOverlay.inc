#include "UI/SharedEditorChrome.h"
// Named Generation II picker layer. It intercepts only Species and Move activation before
// the preserved shared editor launches its legacy numeric prompts. Browsing lives entirely in this
// overlay model; the Gen2PokemonSession changes only when A explicitly applies a selected value.

#include "UI/Gen2PokemonPickerModel.h"
#include "UI/Gen13PickerPaging.h"
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
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    Gen2Picker::Model model;
    TouchScrollState scroll;
    TouchScrollState moveScroll;
    TouchPickerViewport moveViewport;
    TouchPickerViewport locationViewport;
    SharedScrollbar::State moveScrollbar;       // R2-SCROLLBAR: draggable thumb on the Move chooser
    SharedScrollbar::State locationScrollbar;   // R2-SCROLLBAR: draggable thumb on Met Location
    SharedSpeciesPicker::Chooser speciesChooser; // R2-SEARCH: live species filter + key grid
};

// Gen II offers its whole native dex (1..251); the chooser filters by the bare species name.
constexpr auto gen2SpeciesAvailable = [](uint16_t) { return true; };
constexpr auto gen2SpeciesName = [](uint16_t species) { return std::string(Names::getSpeciesName(species)); };

PickerOverlayState& pickerStateFor(const TrainerViewScreen& screen) {
    static PickerOverlayState picker;
    if ((picker.owner != &screen || picker.ownerSerial != screen.instanceSerial)) {
        picker = {};
        picker.owner = &screen; picker.ownerSerial = screen.instanceSerial;
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
        picker.speciesChooser.reset();   // R2-SEARCH: every opening starts with the full dex
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
        SharedPopupDismiss::publish(modal.x, modal.y, modal.width, modal.height);   // 7D
        SharedSpeciesPicker::refilter(overlay.speciesChooser, 251, gen2SpeciesAvailable, gen2SpeciesName);
        const int unownForm = Gen2Picker::previewUnownForm(
            state, model.previewShiny, screen.sourceGameId);
        SharedSpeciesPicker::drawContent(
            fb, modal.x, modal.y, model.speciesChoice(), 251, model.previewShiny,
            [](uint16_t species) {
                char prefix[16]{};
                std::snprintf(prefix, sizeof(prefix), "%03u - ", static_cast<unsigned>(species));
                return std::string(prefix) + Names::getSpeciesName(species);
            },
            [unownForm](uint16_t species) {
                std::string name = Names::getSpeciesName(species);
                if (species == 201 && unownForm >= 0 && unownForm < 26)
                    name += " " + std::string(1, static_cast<char>('A' + unownForm));
                return SharedSpeciesPicker::selectedTitle(species, name);
            },
            &latestTouchGesture(), true, {0xFF, 0xFF}, &overlay.speciesChooser, unownForm);
        // Touch ids stay 2000 + species; the chooser publishes the rows it actually drew.
        for (int i = 0; i < overlay.speciesChooser.rowCount; ++i) {
            const auto& row = overlay.speciesChooser.rows[static_cast<std::size_t>(i)];
            screen.touchButtons.push_back({2000 + row.species, row.x, row.y, row.w, row.h});
        }
        PokeBank::UIModel::SharedEditorPresentation::PickerFooterSpec spec;
        spec.xLabel = "Search";
        spec.yLabel = "Normal/Shiny";
        UI::SharedEditorChrome::drawPickerFooter(fb, spec);
        return;
    }
    if (model.kind == Gen2Picker::Kind::Location) {
        // Crystal Met Location: a proportional chooser (Gen2PokemonPickerModel.h geometry).
        const int count = model.locationCount();
        const auto g = Gen2Picker::locationPickerGeometry(count, fb.getWidth(), fb.getHeight());
        fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Color(0, 0, 0, 165));
        drawPanelSurface(fb, g.x, g.y, g.width, g.height, false, 18);
        SharedPopupDismiss::publish(g.x, g.y, g.width, g.height);   // 7D
        fb.drawText(g.x + 24, g.y + 18, "Met Location", Colors::Text, TextStyle::Heading);
        fb.drawText(g.x + 24, g.y + 52, "Where this Pokémon was caught in Crystal",
                    Colors::TextDim, TextStyle::Caption);
        overlay.locationViewport.ensure(model.index, count, g.visible);
        const int first = overlay.locationViewport.firstRow;
        const int liveOffset = overlay.scroll.offset();
        const int viewportH = g.visible * g.rowStep;
        fb.setClipRect(g.listX, g.listY, g.listWidth, viewportH);
        for (int i = std::max(0, first - 1); i < std::min(count, first + g.visible + 1); ++i) {
            const int yy = g.listY + (i - first) * g.rowStep + liveOffset;
            const bool selected = i == model.index;
            appendClippedTouchButton(screen.touchButtons, 2000 + i, g.listX, yy, g.listWidth, g.rowHeight,
                                     g.listX, g.listY, g.listWidth, viewportH);
            SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(g.listX, yy, g.listWidth, g.rowHeight), {selected});  // R2-BUG-012 boxed row
            const auto text = Gen2Picker::locationRowText(model.encounterChoices[static_cast<std::size_t>(i)]);
            fb.drawText(g.listX + 14, yy + 9, text.place, selected ? Colors::SelectedText : Colors::Text);
            int dw = 0, dh = 0;
            fb.measureText(text.detail, dw, dh, TextStyle::Caption);
            fb.drawText(g.listX + g.listWidth - 14 - dw, yy + 12, text.detail, Colors::TextDim, TextStyle::Caption);
        }
        fb.clearClip();
        SharedScrollbar::drawAndPublish(fb, overlay.locationScrollbar, g.x + g.width - 14, g.listY,
            SharedScrollbar::kTrackWidth, viewportH, SharedScrollbar::rows(count, g.visible, first));
        UI::SharedEditorChrome::drawPickerFooter(fb, PokeBank::UIModel::SharedEditorPresentation::PickerFooterSpec{true, false});
        return;
    }
    const int W = fb.getWidth(), H = fb.getHeight();
    constexpr int panelH = 568;
    const int panelW = model.kind == Gen2Picker::Kind::Move ? 1080 : 760;
    const int x = (W - panelW) / 2, y = (H - panelH) / 2;
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    drawPanelSurface(fb, x, y, panelW, panelH, false, 18);
    SharedPopupDismiss::publish(x, y, panelW, panelH);   // 7D

    const bool species = model.kind == Gen2Picker::Kind::Species;
    fb.drawText(x + 24, y + 18, species ? "Species — Generation II" : "Move — Generation II",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                species ? "#001–#251 • browsing does not change the staged Pokemon"
                        : "Drag naturally to browse • exact Gen II Acc / Pwr / PP • compatibility at right",
                Colors::TextDim, TextStyle::Caption);

    const int count = species ? 251 : 252;
    constexpr int visible = 13;
    constexpr int rowStep = 33;
    const int first = std::clamp(model.index - visible / 2, 0, std::max(0, count - visible));
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
        if (species) {
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
    UI::SharedEditorChrome::drawPickerFooter(fb);
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
        picker.moveScroll.reset();
        picker.moveViewport.reset();
        picker.locationViewport.reset();
        return false;
    }
    if (state.mode == Mode::None) {
        if (picker.model.active()) picker.model.close();
        picker.scroll.reset();
        picker.moveScroll.reset();
        picker.moveViewport.reset();
        picker.locationViewport.reset();
        return false;
    }

    if (!picker.model.active())
        return openPickerForFocusedField(screen, state, picker, down);

    const int pickerCount = picker.model.kind == Gen2Picker::Kind::Species
        ? 251 : picker.model.kind == Gen2Picker::Kind::Location ? picker.model.locationCount() : 252;
    bool pickerBarConsumed = false;   // R2-SCROLLBAR: a held thumb owns this frame's touch
    bool speciesTouchOwned = false;   // R2-SEARCH: key grid / scrollbar took this frame's touch
    if (picker.model.kind == Gen2Picker::Kind::Species) {
        uint16_t chosen = picker.model.speciesChoice();
        const auto handled = SharedSpeciesPicker::handleTouch(
            picker.speciesChooser, SharedSearchInput::sample(touch), chosen, 251,
            SharedSpeciesPicker::ModalX, SharedSpeciesPicker::ModalY, gen2SpeciesAvailable, gen2SpeciesName);
        picker.model.index = static_cast<int>(chosen) - 1;
        speciesTouchOwned = handled.touch;
        // The shared Species renderer follows the finger pixel-for-pixel while held. Commit the
        // semantic preview cursor only on release so the gesture cannot accidentally activate A.
        const int dy = touch.deltaY();
        if (!speciesTouchOwned && touch.justReleased() && touch.dragged() &&
            SharedSpeciesPicker::gestureStartedInList(touch.startX(), touch.startY()) &&
            std::abs(dy) >= 24 && std::abs(dy) > std::abs(touch.deltaX())) {
            const int steps = std::max(1, std::abs(dy) / 43);
            picker.model.index = static_cast<int>(SharedSpeciesPicker::neighbour(
                picker.speciesChooser, chosen, dy < 0 ? steps : -steps)) - 1;
        }
    } else {
        constexpr int panelH = 568;
        const int panelW = picker.model.kind == Gen2Picker::Kind::Move ? 1080 : 760;
        const int x = (1280 - panelW) / 2;
        const int y = (720 - panelH) / 2;
        if (picker.model.kind == Gen2Picker::Kind::Location) {
            const auto g = Gen2Picker::locationPickerGeometry(pickerCount);
            picker.locationViewport.ensure(picker.model.index, pickerCount, g.visible);
            const int maxFirst = TouchPickerViewport::maxFirst(pickerCount, g.visible);
            const auto locationBar = SharedScrollbar::handleTouch(picker.locationScrollbar,
                SharedScrollbar::sample(touch),
                SharedScrollbar::rows(pickerCount, g.visible, picker.locationViewport.firstRow));
            if (locationBar.changed) {
                picker.locationViewport.firstRow = locationBar.scroll;
                picker.scroll.stop();
            }
            pickerBarConsumed = locationBar.consumed;
            if (maxFirst > 0 && !locationBar.consumed)
                picker.scroll.updateVertical(
                    touch, g.listX, g.listY, g.listWidth, g.visible * g.rowStep, g.rowStep,
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
    int touchTap = screen.touchedButtonId(touch);
    if (!pickerBarConsumed && !speciesTouchOwned && touchTap >= firstTouchId && touchTap < endTouchId) {
        if (picker.model.kind == Gen2Picker::Kind::Species) {
            // A row tap commits that species: close the search grid so the synthesized A below
            // is the chooser's Select, not a key press on the grid (keys phase runs next).
            picker.speciesChooser.search.closeGrid();
            picker.model.index = std::clamp(touchTap - 2000, 1, 251) - 1;
        }
        else
            picker.model.index = std::clamp(touchTap - 2000, 0, std::max(0, pickerCount - 1));
        down |= HidNpadButton_A;
    }

    uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (picker.model.kind == Gen2Picker::Kind::Species) {
        // R2-SEARCH keys phase: X / Minus toggle the search grid; while it is open the D-pad /
        // stick, A and B type and the chooser's own B closes only the grid (the filter stays).
        const PokeBank::UIModel::SharedPickerPaging::Keys gridKeys{
            (nav & HidNpadButton_Up) != 0, (nav & HidNpadButton_Down) != 0,
            (nav & HidNpadButton_Left) != 0, (nav & HidNpadButton_Right) != 0,
            (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
            (down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};
        uint16_t chosen = picker.model.speciesChoice();
        const auto handled = SharedSpeciesPicker::handleKeys(
            picker.speciesChooser, gridKeys, (down & HidNpadButton_A) != 0, (down & HidNpadButton_B) != 0,
            (down & (HidNpadButton_X | HidNpadButton_Minus)) != 0, chosen, 251,
            gen2SpeciesAvailable, gen2SpeciesName);
        picker.model.index = static_cast<int>(chosen) - 1;
        if (handled.keys) {
            down &= ~(HidNpadButton_A | HidNpadButton_B | HidNpadButton_L | HidNpadButton_R |
                      HidNpadButton_ZL | HidNpadButton_ZR);
            nav &= ~(HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right);
        }
    }

    if (down & HidNpadButton_B) {
        closePicker(screen, picker);
        return true;
    }

    if (picker.model.kind == Gen2Picker::Kind::Species ||
        picker.model.kind == Gen2Picker::Kind::Move || picker.model.kind == Gen2Picker::Kind::Location) {
        // Shared list paging (Gen13PickerPaging.h): Up/Down wrap, Left/L/Right/R +-10,
        // ZL/ZR +-50, clamped. The stick repeats through nav like the D-pad.
        const PokeBank::UIModel::SharedPickerPaging::Keys keys{
            (nav & HidNpadButton_Up) != 0, (nav & HidNpadButton_Down) != 0,
            (nav & HidNpadButton_Left) != 0, (nav & HidNpadButton_Right) != 0,
            (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
            (down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};
        const int listCount = picker.model.kind == Gen2Picker::Kind::Species ? 251
                            : picker.model.kind == Gen2Picker::Kind::Move ? 252
                            : picker.model.locationCount();
        const uint16_t pagedFrom = picker.model.speciesChoice();
        if (keys.any() && listCount > 0) {
            picker.scroll.stop();
            picker.model.index = PokeBank::UIModel::SharedPickerPaging::pagedRow(
                picker.model.index, listCount, keys);
            // R2-SEARCH: with a species filter typed, the same keys page over the filtered list
            // instead (computed from the choice before the whole-dex step above).
            if (picker.model.kind == Gen2Picker::Kind::Species && picker.speciesChooser.filtered())
                picker.model.index = static_cast<int>(SharedSpeciesPicker::paged(
                    picker.speciesChooser, pagedFrom, keys)) - 1;
        }
        if (picker.model.kind == Gen2Picker::Kind::Species && (down & HidNpadButton_Y))
            picker.model.previewShiny = !picker.model.previewShiny;
    }

    if (picker.model.kind == Gen2Picker::Kind::Location) {
        const int visible = Gen2Picker::locationPickerGeometry(pickerCount).visible;
        if (nav & (HidNpadButton_Up | HidNpadButton_Down |
                   HidNpadButton_Left | HidNpadButton_Right))
            picker.locationViewport.reveal(picker.model.index, pickerCount, visible);
        if ((down & HidNpadButton_A) &&
            !picker.locationViewport.containsSelection(picker.model.index, pickerCount, visible)) {
            picker.scroll.stop();
            picker.locationViewport.reveal(picker.model.index, pickerCount, visible);
            screen.postStatus("Location brought into view; press A again or tap it", 220);
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
