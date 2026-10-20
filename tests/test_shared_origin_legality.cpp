// E03 (Round 3 hardware): ONE Origin & Legality layout for every Gen I-V game, honest states only.
//   * the shared report has one fixed row order for all five generations;
//   * Legality is "Not checked" everywhere and can never be Verified / Problems found;
//   * the encounter check is always "Unknown"; a field the format does not store reads
//     "Not stored in <format>", one this editor does not decode reads "Not read by this editor";
//   * every generation's Origin & Legality surface draws the shared panel from its native record,
//     and every View / Edit data pane uses the same one legality state.
#include "UI/SharedOriginLegality.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace OL = PokeBank::UIModel::OriginLegality;
namespace P = PokeBank::UIModel::SharedEditorPresentation;

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

// The body of `name` up to the next top-level definition marker.
std::string body(const std::string& text, const std::string& name, const std::string& until) {
    const auto a = text.find(name);
    if (a == std::string::npos) return {};
    const auto b = text.find(until, a + name.size());
    return text.substr(a, b == std::string::npos ? std::string::npos : b - a);
}

const std::vector<std::string> kLabels{"Exact source", "Record structure", "Source mutation", "Origin game",
                                       "Met location", "Met level", "Met date", "Encounter check", "Legality"};

std::vector<std::string> labels(const std::vector<OL::Row>& rows) {
    std::vector<std::string> out;
    for (const auto& r : rows) out.push_back(r.label);
    return out;
}
std::string valueOf(const std::vector<OL::Row>& rows, const std::string& label) {
    for (const auto& r : rows)
        if (r.label == label) return r.value;
    return {};
}

} // namespace

int main() {
    // A Gen I record stores no origin at all and has no per-record checksum.
    OL::Report gen1;
    gen1.exactGame = "Pokémon Red";
    gen1.format = "Gen I record";
    gen1.originGame = OL::notStored();
    gen1.metLocation = OL::notStored();
    gen1.metLevel = OL::notStored();
    gen1.metDate = OL::notStored();
    gen1.moves = {{"Thunder Shock", "OK", OL::Tone::Good}};
    // A Gen IV record: validated structure, decoded origin, a met date the editor does not decode.
    OL::Report gen4;
    gen4.exactGame = "HeartGold";
    gen4.format = "PK4";
    gen4.structure = OL::Structure::Valid;
    gen4.originGame = OL::value("SoulSilver");
    gen4.metLocation = OL::value("Route 29");
    gen4.metLevel = OL::value("5");
    gen4.metDate = OL::notRead();
    // A bare report (no adapter data): every field unread, structure not checked.
    OL::Report bare;
    bare.exactGame = "Pokémon Black";
    bare.format = "PK5";

    for (const auto* report : {&gen1, &gen4, &bare}) {
        const auto rows = OL::rows(*report);
        check(labels(rows) == kLabels, report->format + ": the one fixed row order");
        check(static_cast<int>(rows.size()) == OL::kFixedRows, report->format + ": kFixedRows matches");
        const auto legality = valueOf(rows, "Legality");
        check(legality.rfind("Not checked", 0) == 0, report->format + ": Legality is Not checked");
        check(legality.find("Verified") == std::string::npos && legality.find("Problems") == std::string::npos &&
                  legality.find("OK") == std::string::npos && legality.find("Legal") == std::string::npos,
              report->format + ": no clean verdict in the Legality row");
        check(valueOf(rows, "Encounter check") == OL::kEncounterCheck &&
                  std::string(OL::kEncounterCheck).rfind("Unknown", 0) == 0,
              report->format + ": encounter check is Unknown");
        check(valueOf(rows, "Source mutation") == OL::kSourceMutation, report->format + ": source writes hard disabled");
        for (const auto& row : rows)
            check(row.value.find("Verified") == std::string::npos && row.value != "None" && row.value != "0",
                  report->format + ": " + row.label + " shows no placeholder or verdict");
    }
    const auto g1 = OL::rows(gen1);
    check(valueOf(g1, "Origin game") == "Not stored in Gen I record" &&
              valueOf(g1, "Met date") == "Not stored in Gen I record",
          "Gen I: absent fields say the format does not store them");
    check(valueOf(g1, "Record structure") == "Not checked", "Gen I: no per-record checksum, structure Not checked");
    const auto g4 = OL::rows(gen4);
    check(valueOf(g4, "Record structure") == "PK4 valid" && valueOf(g4, "Origin game") == "SoulSilver" &&
              valueOf(g4, "Met date") == OL::kNotRead,
          "Gen IV: decoded values shown; the stored-but-undecoded met date is not called absent");
    check(valueOf(OL::rows(bare), "Met location") == OL::kNotRead, "default field reads as not decoded");
    static_assert(OL::kLegality == P::LegalityState::NotChecked);

    // Every generation draws the ONE panel from its native record.
    struct Site { const char* path; const char* function; const char* until; const char* report; };
    for (const Site site : {
             Site{"src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "void ux2DrawProvenance(", "\nvoid ",
                  "gen1LegalityReport(screen, *pokemon)"},
             Site{"src/UI/Gen2SharedPokemonSurface.inc", "void drawSharedProvenanceSurface(", "\n}\n",
                  "gen2LegalityReport(screen, *pokemon)"},
             Site{"src/UI/Gen3SharedPokemonSurface.inc", "void drawLegality(", "\nvoid drawActions(",
                  "legalityReport(screen, *record)"},
             Site{"src/UI/Gen4SharedPokemonSurface.inc", "void drawLegality(", "\nvoid drawReleaseConfirm(",
                  "legalityReport(screen, *record)"},
             Site{"src/UI/Gen5SharedReadOnlySurface.inc", "void drawLegality5(", "\nvoid drawDecision(",
                  "legalityReport5(screen,*pk)"}}) {
        const auto text = read(site.path);
        const auto fn = body(text, site.function, site.until);
        check(!fn.empty(), std::string(site.path) + ": Origin & Legality drawer exists");
        check(fn.find("SharedEditorChrome::drawOriginLegalityPanel(") != std::string::npos && fn.find(site.report) != std::string::npos,
              std::string(site.path) + ": draws the shared panel from its native report");
        check(fn.find("LegalityState::Verified") == std::string::npos && fn.find("ProblemsFound") == std::string::npos &&
                  fn.find("\"PASS\"") == std::string::npos,
              std::string(site.path) + ": no verdict or PASS text in the drawer");
    }
    // The View / Edit data panes use the same single legality state.
    for (const char* path : {"src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc",
                             "src/UI/Gen5SharedReadOnlySurface.inc"}) {
        const auto text = read(path);
        check(text.find("legalityLabel(\n                PokeBank::UIModel::SharedEditorPresentation::LegalityState::NotChecked)") !=
                      std::string::npos ||
                  text.find("legalityLabel(PokeBank::UIModel::SharedEditorPresentation::LegalityState::NotChecked)") !=
                      std::string::npos,
              std::string(path) + ": data pane Legality row uses the one shared state");
        check(text.find("LegalityState::Incomplete") == std::string::npos, std::string(path) + ": no Incomplete state");
    }
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayUX.inc", "src/UI/Gen1PokemonEditorOverlay.cpp"})
        check(read(path).find("NOT CLAIMED") == std::string::npos, std::string(path) + ": one Not checked wording");
    const auto chrome = read("include/UI/SharedEditorChrome.h");
    const auto panel = chrome.substr(chrome.find("inline void drawOriginLegalityPanel("));
    check(panel.find("beginModal(touchButtons);") != std::string::npos &&
              panel.find("drawNavBar(fb, {{\"B\", \"Back\"}});") != std::string::npos,
          "the panel owns the frame's touch layer and draws its own B Back footer");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "E03 shared Origin & Legality (Gen I-V, one layout, honest states, " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
