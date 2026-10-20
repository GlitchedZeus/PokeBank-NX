// AUD-01 (Round 3 hardware): Gen V's Review Pending Changes matches Gen IV's chrome.
//   * one title, one source-protection line, one footer vocabulary (the owner's "Export Copy" /
//     "Discard All Staged") from UI/SharedReviewChrome.h, used by Gen IV and Gen V;
//   * Gen V's Discard All is the Gen IV confirmation (same title, wording, B Keep Changes /
//     Y Discard All boxed choices), drawn in its own file with its own touch layer and footer;
//   * Gen V keeps its owner-accepted (S04) verified change rows; inputs are unchanged.
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

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
    const auto chrome = read("include/UI/SharedReviewChrome.h");
    check(chrome.find("kTitle = \"Review Pending Changes\"") != std::string::npos &&
              chrome.find("kExportLabel = \"Export Copy\"") != std::string::npos &&
              chrome.find("kDiscardLabel = \"Discard All Staged\"") != std::string::npos,
          "one Review vocabulary");
    check(chrome.find("\"Original \" + provider + \" source: HARD READ ONLY\"") != std::string::npos,
          "one source-protection line");
    check(chrome.find("if (paged) hints.push_back({\"L/R\", \"Review Pages\"});") != std::string::npos &&
              chrome.find("hints.push_back({\"X\", kExportLabel});") != std::string::npos &&
              chrome.find("hints.push_back({\"Y\", kDiscardLabel});") != std::string::npos &&
              chrome.find("hints.push_back({\"B\", \"Back\"});") != std::string::npos,
          "footer: [L/R Review Pages] X Export Copy / Y Discard All Staged / B Back");

    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto review4 = between(gen4, "void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state) {",
                                 "\n}\n");
    check(review4.find("SharedReviewChrome::kTitle") != std::string::npos &&
              review4.find("SharedReviewChrome::sourceLine(screen.legacyProviderLabel())") != std::string::npos &&
              review4.find("SharedReviewChrome::footer(false)") != std::string::npos,
          "Gen IV Review uses the shared chrome");

    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto review5 = between(gen5, "void drawReview(TrainerViewScreen& screen,PKSEFramebuffer& fb,", "\n}\n");
    check(review5.find("SharedReviewChrome::kTitle") != std::string::npos &&
              review5.find("SharedReviewChrome::sourceLine(screen.legacyProviderLabel())") != std::string::npos &&
              review5.find("drawNavBar(fb,SharedReviewChrome::footer(true));") != std::string::npos,
          "Gen V Review uses the same chrome (paged)");
    check(review5.find("Review Pending Changes - Generation V") == std::string::npos &&
              review5.find("Export Working Copy") == std::string::npos && review5.find("Discard All...") == std::string::npos,
          "Gen V no longer has its own title / footer words");
    check(review5.find("verifyStagedReview(change,&error)") != std::string::npos &&
              review5.find("groupReviewRows5(pending)") != std::string::npos,
          "Gen V keeps its verified per-record rows (S04)");

    const auto confirm5 = between(gen5, "void drawDiscardAllConfirm5(TrainerViewScreen& screen,PKSEFramebuffer& fb) {", "\n}\n");
    check(confirm5.find("SharedEditorChrome::beginModal(screen.touchButtons);") != std::string::npos &&
              confirm5.find("SharedReviewChrome::kDiscardTitle") != std::string::npos &&
              confirm5.find("{{{\"B\",\"Keep Changes\"},{\"Y\",\"Discard All\"}}}") != std::string::npos &&
              confirm5.find("drawNavBar(fb,{{\"Y\",\"Discard All\"},{\"B\",\"Keep Changes\"}});") != std::string::npos,
          "Gen V Discard All is the Gen IV confirmation, with its own touch layer and footer");
    const auto confirm4 = between(gen4, "void drawDiscardAllConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, const char* generation) {",
                                  "\n}\n");
    check(confirm4.find("\"Discard all staged changes?\"") != std::string::npos &&
              confirm4.find("{{{\"B\", \"Keep Changes\"}, {\"Y\", \"Discard All\"}}}") != std::string::npos,
          "Gen IV confirmation (the reference) unchanged");
    const auto drawCase = between(gen5, "        case Surface::ConfirmDiscardAll:\n            // AUD-01", "return true;");
    check(drawCase.find("drawReview(screen,fb,state,staged);") != std::string::npos &&
              drawCase.find("drawDiscardAllConfirm5(screen,fb);") != std::string::npos,
          "Gen V draws the confirm over its Review, like Gen IV");
    const auto inputCase = between(gen5, "        case Surface::ConfirmDiscardAll:\n            if(keys&HidNpadButton_B)", "return true;");
    check(inputCase.find("keys&HidNpadButton_Y") != std::string::npos, "only Y discards; B keeps (input unchanged)");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "AUD-01 Gen V Review chrome matches Gen IV (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
