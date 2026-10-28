// P3 (Round 5, R2-TRAINER / NEW-18): the classic Trainer page edits Play Time and Gender where a
// native staged writer exists, through the one shared session (UI/ClassicTrainerSessionInput.h).
//   * rows: 0 Name, 1 Money, 2 Play Time, then Gender (row 3 with Play Time); Gen I adds neither
//     (no gender field; its timer saturation flag keeps Play Time read-only here);
//   * Gen II offers Play Time, and Gender only on Crystal (Gold / Silver store none); Gen III / IV
//     offer both; every accepted value goes to that generation's staged workspace (Review / Discard
//     / Export Copy), never to the source save;
//   * the page draws them as focusable, tappable rows and drops the matching read-only info row.
#include "UI/ClassicTrainerSession.h"
#include "UI/ClassicTrainerPage.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace T = PokeBank::UIModel::ClassicTrainerSession;

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

} // namespace

int main() {
    static_assert(T::kRows == 2 && T::kPlayTimeRow == 2);
    static_assert(T::rowCount(false, false) == 2 && T::rowCount(true, false) == 3 && T::rowCount(true, true) == 4);
    static_assert(T::genderRow(true) == 3 && T::genderRow(false) == 2);

    const auto session = read("include/UI/ClassicTrainerSessionInput.h");
    check(session.find("const bool playTimeRow = static_cast<bool>(fields.stagePlayTime) && fields.playTime.has_value();") !=
                  std::string::npos &&
              session.find("const bool genderRow = static_cast<bool>(fields.stageGender);") != std::string::npos,
          "a row exists only when the generation supplies its native stager");
    check(session.find("return staged(fields.stagePlayTime(next, error), error);") != std::string::npos &&
              session.find("return staged(fields.stageGender(fields.gender == 0 ? 1 : 0, error), error);") != std::string::npos,
          "accepted Play Time / Gender go through the stager (staged workspace only)");
    check(session.find("\"Play Time - Hours\"") != std::string::npos && session.find("0, 59);") != std::string::npos,
          "Play Time prompts hours (bounded) then 0-59 minutes / seconds");
    check(session.find("screen.trainerSelectedRow = (screen.trainerSelectedRow + 1) % kRows") != std::string::npos &&
              session.find("(screen.trainerSelectedRow + kRows - 1) % kRows") != std::string::npos,
          "Down / Up wrap over every row");

    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    check(gen1.find("stagePlayTime") == std::string::npos && gen1.find("stageGender") == std::string::npos,
          "Gen I offers no Play Time / Gender rows (no gender field)");
    const auto gen2 = read("src/UI/Gen2TrainerSessionOverlay.inc");
    check(gen2.find("editor->stagePlayTime(") != std::string::npos &&
              gen2.find("if (editor->trainerGenderEditable())") != std::string::npos,
          "Gen II stages Play Time; Gender only where the Crystal byte exists");
    const auto gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    check(gen3.find("trainer->stagePlayTime(") != std::string::npos && gen3.find("trainer->stageGender(") != std::string::npos,
          "Gen III stages Play Time and Gender");
    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    check(gen4.find("trainer.stagePlayTime(time, error) && refreshPresentation(screen)") != std::string::npos &&
              gen4.find("trainer.stageGender(gender, error) && refreshPresentation(screen)") != std::string::npos,
          "Gen IV stages Play Time and Gender");

    // Round 6 P2: the classic page draws from UI/ClassicTrainerPage.h (Gen V design); the same
    // guarantees, now checked on the model the page paints from (test_classic_trainer_page covers
    // the layout and the routing).
    namespace P = PokeBank::UIModel::ClassicTrainerPage;
    const auto base = read("src/UI/TrainerViewScreenBase.inc");
    check(base.find("drawClassicTrainerView(screen, fb, x, y, w, h, generation);") != std::string::npos &&
              base.find("const auto page = Page::layout(rows, x, y, w, h);") != std::string::npos,
          "the classic Trainer page is drawn from the shared row model");
    for (int generation = 1; generation <= 4; ++generation) {
        const bool genderStored = generation >= 3;
        const auto both = P::rows({generation, false, true, true});
        const auto none = P::rows({generation, false, false, false});
        check(both[0].field == P::Field::Name && both[0].editable && both[0].touchId == 0 &&
                  both[1].field == P::Field::Money && both[1].editable && both[1].touchId == 1,
              "Name / Money rows unchanged");
        check(both[2].field == P::Field::PlayTime && both[2].editable && both[2].touchId == T::kPlayTimeRow,
              "Play Time is a focusable, tappable row when its writer exists");
        if (genderStored)
            check(both[3].field == P::Field::Gender && both[3].editable && both[3].touchId == T::genderRow(true),
                  "Gender is a focusable, tappable row when its writer exists");
        int playTimeRows = 0, genderRows = 0, readOnlyPlayTime = 0, readOnlyGender = 0;
        for (const auto& row : both) {
            playTimeRows += row.field == P::Field::PlayTime;
            genderRows += row.field == P::Field::Gender;
            if (row.field == P::Field::Badges) check(!row.editable, "Badges stay display-only");
        }
        for (const auto& row : none) {
            readOnlyPlayTime += row.field == P::Field::PlayTime && !row.editable;
            readOnlyGender += row.field == P::Field::Gender && !row.editable;
        }
        check(playTimeRows == 1 && genderRows == (genderStored ? 1 : 0),
              "the read-only info row is dropped when the field is editable");
        check(readOnlyPlayTime == 1 && readOnlyGender == (genderStored ? 1 : 0),
              "without a writer the field stays as one read-only info row");
    }
    // Crystal stores a gender; Gold / Silver do not (no row, editable or read-only).
    check(P::rows({2, true, true, true})[3].field == P::Field::Gender, "Crystal Gender row");
    for (const auto& row : P::rows({2, false, true, true}))
        check(row.field != P::Field::Gender, "Gold / Silver show no invented Gender row");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "P3 classic Trainer Play Time / Gender rows (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
