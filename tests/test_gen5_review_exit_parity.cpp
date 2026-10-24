// AUD-01 Gen V half (owner hardware, Oct 24): Gen V Review is reachable and its exit is Gen I-IV's.
//   * leaving a Gen V session with staged changes draws the ONE Gen I-IV "Unsaved Changes" dialog
//     (A Review Changes / B Continue Editing / X Export Copy / Y Discard & Exit), with its touch
//     layer and footer, and reads input through the same ClassicStagedExit::choose;
//   * X Export Copy goes through Review's own Y-confirmed export (nothing is written on X);
//   * the Gen V Trainer page offers Y Review, the Gen I-IV Trainer-page Review button
//     (+ still works as a silent alias); Review renders through SharedEditorChrome::drawReviewPanel.
#include "UI/ClassicStagedExit.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Exit = PokeBank::UIModel::ClassicStagedExit;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "missing " << path << '\n';
        std::exit(EXIT_FAILURE);
    }
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string between(const std::string& text, const std::string& from, const std::string& to) {
    const auto a = text.find(from);
    if (a == std::string::npos) return {};
    const auto b = text.find(to, a + from.size());
    return text.substr(a, b == std::string::npos ? std::string::npos : b - a);
}

} // namespace

int main() {
    // The four choices and their touch ids (the dialog's drawEditChoiceButton ids 0..3).
    static_assert(Exit::choose(true, false, false, false) == Exit::Action::Review);
    static_assert(Exit::choose(false, true, false, false) == Exit::Action::ContinueEditing);
    static_assert(Exit::choose(false, false, true, false) == Exit::Action::ExportCopy);
    static_assert(Exit::choose(false, false, false, true) == Exit::Action::DiscardExit);
    static_assert(Exit::choose(false, false, false, false, 0) == Exit::Action::ContinueEditing);
    static_assert(Exit::choose(false, false, false, false, 1) == Exit::Action::DiscardExit);
    static_assert(Exit::choose(false, false, false, false, 2) == Exit::Action::Review);
    static_assert(Exit::choose(false, false, false, false, 3) == Exit::Action::ExportCopy);
    static_assert(Exit::choose(true, true, true, true) == Exit::Action::ContinueEditing, "B always wins");

    const auto dialog = read("src/UI/Dialogs/SaveConfirmDialog.cpp");
    const auto staged = between(dialog, "void drawStagedExitDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {", "\n    }\n");
    check(staged.find("SharedEditorChrome::beginModal(screen.touchButtons);") != std::string::npos &&
              staged.find("paintSaveConfirmContents(screen, fb, true);") != std::string::npos &&
              staged.find("SharedEditorChrome::drawFooter(fb, classicSaveConfirmFooter(screen, true));") != std::string::npos,
          "the Gen V exit draws the classic dialog, touch layer and footer");
    const auto paint = between(dialog, "void paintSaveConfirmContents(TrainerViewScreen& screen, PKSEFramebuffer& fb, bool stagedExit) {",
                               "            return;\n        }");
    check(paint.find("if (stagedExit || (screen.exitingWithUnsavedChanges && generation >= 1 && generation <= 4)) {") !=
                  std::string::npos &&
              paint.find("\"A\", \"Review Changes\", 2") != std::string::npos &&
              paint.find("\"B\", \"Continue Editing\", 0") != std::string::npos &&
              paint.find("\"X\", \"Export Copy\", 3") != std::string::npos &&
              paint.find("\"Y\", \"Discard & Exit\", 1") != std::string::npos,
          "one four-choice body for Gen I-V");
    check(dialog.find("SharedEditorChrome::drawFooter(fb, classicSaveConfirmFooter(screen));") != std::string::npos,
          "the Gen I-IV exit still draws the same dialog and footer");
    check(read("include/UI/Dialogs/SaveConfirmDialog.h").find("void drawStagedExitDialog(") != std::string::npos,
          "drawStagedExitDialog is declared");

    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto exitDraw = between(gen5, "        case Surface::ConfirmExit:\n            // AUD-01", "return true;");
    check(exitDraw.find("Dialogs::drawStagedExitDialog(screen,fb);") != std::string::npos,
          "Gen V ConfirmExit draws the shared staged exit");
    check(gen5.find("Staged Changes Remain In Memory") == std::string::npos, "the old three-choice Gen V exit is gone");
    const auto exitInput = between(gen5, "        case Surface::ConfirmExit: {", "\n        }\n");
    check(exitInput.find("Exit::choose(keys&HidNpadButton_A,keys&HidNpadButton_B,keys&HidNpadButton_X,\n"
                         "                                keys&HidNpadButton_Y,tap)") != std::string::npos,
          "Gen V reads the exit through ClassicStagedExit::choose, touch included");
    check(between(exitInput, "case Exit::Action::ContinueEditing:", "break;").find("state.back()") != std::string::npos &&
              between(exitInput, "case Exit::Action::DiscardExit:", "break;").find("state.confirmDiscardAndExit(staged)") !=
                  std::string::npos &&
              between(exitInput, "case Exit::Action::Review:", "break;").find("state.reviewFromExit(staged)") !=
                  std::string::npos,
          "Continue / Discard & Exit / Review keep their staged-state routes");
    const auto exportCase = between(exitInput, "case Exit::Action::ExportCopy:", "break;");
    check(exportCase.find("state.reviewFromExit(staged);") != std::string::npos &&
              exportCase.find("state.requestExport(staged)") != std::string::npos &&
              exportCase.find("exportWorkingCopy") == std::string::npos,
          "X Export Copy opens the Y-confirmed export; nothing is written on X");

    const auto trainerScreen = read("include/Integration/Gen5/Gen5TrainerScreen.h");
    check(trainerScreen.find("{\"+\", \"Review\"}") == std::string::npos &&
              trainerScreen.find("{\"A\", \"Edit\"}, {\"Y\", \"Review\"}, {\"B\", \"Back\"}") != std::string::npos,
          "the Gen V Trainer footer offers Y Review");
    check(gen5.find("    if(keys&(HidNpadButton_Y|HidNpadButton_Plus)) {\n        if(!state.openReviewFromBrowse(ws))") !=
              std::string::npos,
          "Y (or the old +) opens Review from the Trainer page");
    check(between(gen5, "void drawReview(TrainerViewScreen& screen,PKSEFramebuffer& fb,", "\n}\n")
                  .find("SharedEditorChrome::drawReviewPanel(") != std::string::npos,
          "Gen V Review uses the shared Review presenter");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "AUD-01 Gen V Review reachable, Gen I-IV staged exit (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
