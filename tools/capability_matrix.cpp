// capability_matrix: prints docs/gen6-plus/CAPABILITY_MATRIX.md from the one capability registry.
//
// The registry (include/Integration/Common/GameCapabilityRegistry.h) is the only place that may
// advertise what a title supports; this tool turns it into Markdown so the committed document can
// never drift from the code (tests/test_capability_matrix_doc.py compares the two byte for byte).
// It links src/Games/GameIdentity.cpp and refuses to print when a registry id is not a game
// descriptor or disagrees with it, so a broken registry cannot produce a document.
//
// Output contract (the Python check parses it):
//   * one "## Generation N (<platforms>)" heading per generation, each followed by one table;
//   * every table has a "Game" column followed by exactly the 12 capability columns, in order;
//   * one table row per registry title (38 rows);
//   * a "Native mechanics per title" table for the Gen VI-IX titles;
//   * a "Blockers" list with every Blocked reason once, naming the workflows and titles;
//   * the fixed header and footer lines.
//
// Nothing here opens a save or touches the device; it is a host build tool only.

#include "Games/GameIdentity.h"
#include "Integration/Common/GameCapabilityRegistry.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Cap = PokeVault::Integration::Capability;

namespace {

constexpr std::string_view kColumns[] = {
    "Dashboard/Save Instances", "Party/Boxes", "Pokemon editor", "Native mechanics", "Trainer", "Bag",
    "Pokedex", "Create/Clone/Move/Remove", "Backup/Review", "Export and safety", "CI",
    "Switch hardware verification",
};

std::string roman(uint8_t generation) {
    static constexpr std::string_view table[] = {"?", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
    return std::string(generation < 10 ? table[generation] : "?");
}

// Markdown table cells must not contain a pipe or a newline; the registry never writes either,
// but the escape keeps the document well-formed if a reason ever does.
std::string escapeCell(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        if (c == '|') out += "\\|";
        else if (c == '\n') out += ' ';
        else out += c;
    }
    return out;
}

std::string statusCell(const Cap::WorkflowStatus& s) {
    std::string out(Cap::supportName(s.support));
    if (s.support == Cap::Support::Blocked || s.support == Cap::Support::NotApplicable)
        out += " - " + std::string(s.reason);
    return escapeCell(out);
}

std::string labelled(std::string_view label, const Cap::GameCapabilities& row, Cap::Workflow w) {
    return std::string(label) + " " + statusCell(row.status(w));
}

std::string combined(const Cap::GameCapabilities& row,
                     std::initializer_list<std::pair<std::string_view, Cap::Workflow>> parts) {
    std::string out;
    for (const auto& [label, w] : parts) {
        if (!out.empty()) out += " / ";
        out += labelled(label, row, w);
    }
    return out;
}

std::string ciCell(const Cap::GameCapabilities& row) {
    if (row.provider == "classic") return "Host suite (GCC/Clang/ASan/UBSan) on Round 10 source";
    if (row.provider == "modern-native") {
        const std::string n = std::to_string(static_cast<int>(row.generation));
        return "test_gen" + n + "_* lane suites + test_gen" + n + "_native_session + test_modern_sources + real-screen " +
               std::string(row.uiEvidence) + " (GCC; Clang/ASan/UBSan on the backend set); native devkitA64 build pending CI";
    }
    if (row.provider == "legacy-modern-reader") return "Host read-validation tests only (no staged session)";
    return "NOT IMPLEMENTED - provider \"" + std::string(row.provider) + "\" has no CI lane";
}

std::string hardwareCell(const Cap::GameCapabilities& row) {
    switch (row.hardware) {
        case Cap::HardwareState::OwnerAcceptedExactBuild:
            return "OWNER ACCEPTED on " + std::string(row.hardwareEvidence);
        case Cap::HardwareState::OwnerTestedEarlierCandidate:
            return "NOT ACCEPTED (owner rounds on earlier candidates; issue #313 open)";
        case Cap::HardwareState::NotDeviceTested: break;
    }
    return "NOT TESTED";
}

std::string mechanicsList(const Cap::NativeMechanics& m) {
    const std::pair<std::string_view, bool> flags[] = {
        {"megaForms", m.megaForms},           {"fairyType", m.fairyType},
        {"superTraining", m.superTraining},   {"pokemonAmie", m.pokemonAmie},
        {"memories", m.memories},             {"geolocation", m.geolocation},
        {"zMoves", m.zMoves},                 {"alolanForms", m.alolanForms},
        {"hyperTraining", m.hyperTraining},   {"awakeningValues", m.awakeningValues},
        {"partnerRestrictions", m.partnerRestrictions}, {"goOrigin", m.goOrigin},
        {"dynamax", m.dynamax},               {"gigantamax", m.gigantamax},
        {"technicalRecords", m.technicalRecords}, {"battleTeamLocks", m.battleTeamLocks},
        {"effortLevels", m.effortLevels},     {"alphaPokemon", m.alphaPokemon},
        {"moveMastery", m.moveMastery},       {"pastures", m.pastures},
        {"teraType", m.teraType},             {"mintedNature", m.mintedNature},
        {"marks", m.marks},                   {"ribbons", m.ribbons},
        {"plusFlags", m.plusFlags},           {"scale", m.scale},
    };
    std::string out;
    for (const auto& [name, on] : flags) {
        if (!on) continue;
        if (!out.empty()) out += ", ";
        out += name;
    }
    return out.empty() ? std::string("(none)") : out;
}

std::string statModelName(Cap::StatModel s) {
    switch (s) {
        case Cap::StatModel::DVStatExp: return "DV/StatExp";
        case Cap::StatModel::IVEV: return "IV/EV";
        case Cap::StatModel::IVAwakening: return "IV/Awakening";
        case Cap::StatModel::IVEffortLevel: return "IV/EffortLevel";
    }
    return "?";
}

std::string gameCell(const Cap::GameCapabilities& row) {
    return escapeCell(std::string(row.title) + " (`" + std::string(row.id) + "`, " + std::string(row.family) + ", " +
                      std::string(row.recordFormat) + ")");
}

// Registry ids must be real game descriptors and agree with them; otherwise the document would
// describe a title the application cannot identify.
bool registryAgreesWithIdentity(std::string& error) {
    for (const auto& row : Cap::all()) {
        const auto* identity = PokeVault::Games::findGame(row.id);
        if (!identity) {
            error = "registry id has no GameIdentity descriptor: " + std::string(row.id);
            return false;
        }
        if (identity->dataGeneration != row.generation || identity->title != row.title) {
            error = "registry row disagrees with GameIdentity: " + std::string(row.id);
            return false;
        }
    }
    return true;
}

std::string platformsFor(uint8_t generation) {
    std::vector<std::string_view> names;
    for (const auto& row : Cap::all()) {
        if (row.generation != generation) continue;
        const auto name = PokeVault::Games::platformName(PokeVault::Games::findGame(row.id)->platform);
        bool seen = false;
        for (const auto& n : names) seen = seen || n == name;
        if (!seen) names.push_back(name);
    }
    std::string out;
    for (const auto& n : names) {
        if (!out.empty()) out += ", ";
        out += n;
    }
    return out;
}

struct BlockerEntry {
    std::vector<std::string> workflows;  // in first-seen order, unique
    std::vector<std::string> titles;     // in registry order, unique
};

void append(std::vector<std::string>& list, std::string value) {
    for (const auto& v : list)
        if (v == value) return;
    list.push_back(std::move(value));
}

std::string join(const std::vector<std::string>& list) {
    std::string out;
    for (const auto& v : list) {
        if (!out.empty()) out += ", ";
        out += v;
    }
    return out;
}

} // namespace

int main() {
    std::string error;
    if (!registryAgreesWithIdentity(error)) {
        std::cerr << "capability_matrix: " << error << '\n';
        return EXIT_FAILURE;
    }

    std::string out;
    out += "# PokeBank NX capability matrix (Gen I-IX)\n\n";
    out += "Vocabulary: PASS = Supported (implemented through the shared workflow and host-tested); "
           "READ-ONLY = ReadOnly (visible, editing not verified); BLOCKED - <reason> = Blocked (deliberately "
           "unavailable, prerequisite named); NOT APPLICABLE - <reason> = NotApplicable (the native game has no "
           "such data); NOT IMPLEMENTED = nothing exists yet. PASS means host-tested through the shared workflow, "
           "never hardware acceptance; only the \"Switch hardware verification\" column records an owner result.\n\n";
    out += "Source: `include/Integration/Common/GameCapabilityRegistry.h` (cumulative-inheritance static_assert; "
           "`tests/test_game_capability_registry.cpp`). One row per title, grouped by generation. Combined "
           "columns list each workflow with its own status.\n\n";

    // ---- per-game tables, grouped by generation ----
    std::vector<uint8_t> generations;
    for (const auto& row : Cap::all()) {
        bool seen = false;
        for (const auto g : generations) seen = seen || g == row.generation;
        if (!seen) generations.push_back(row.generation);
    }

    size_t rows = 0;
    for (const auto generation : generations) {
        out += "## Generation " + roman(generation) + " (" + platformsFor(generation) + ")\n\n";
        out += "| Game |";
        for (const auto col : kColumns) out += " " + std::string(col) + " |";
        out += "\n|---|";
        for (size_t i = 0; i < std::size(kColumns); ++i) out += "---|";
        out += "\n";
        for (const auto& row : Cap::all()) {
            if (row.generation != generation) continue;
            ++rows;
            out += "| " + gameCell(row) + " |";
            out += " " + combined(row, {{"Dashboard", Cap::Workflow::Dashboard}, {"Save Instances", Cap::Workflow::SaveInstances}}) + " |";
            out += " " + combined(row, {{"Party", Cap::Workflow::Party}, {"Boxes", Cap::Workflow::Boxes}}) + " |";
            out += " " + statusCell(row.status(Cap::Workflow::PokemonEditor)) + " |";
            out += " " + statusCell(row.status(Cap::Workflow::NativeMechanics)) + " |";
            out += " " + statusCell(row.status(Cap::Workflow::Trainer)) + " |";
            out += " " + statusCell(row.status(Cap::Workflow::Bag)) + " |";
            out += " " + statusCell(row.status(Cap::Workflow::Pokedex)) + " |";
            out += " " + combined(row, {{"Create", Cap::Workflow::Create}, {"Clone", Cap::Workflow::Clone},
                                        {"Move", Cap::Workflow::Move}, {"Remove", Cap::Workflow::Remove}}) + " |";
            out += " " + combined(row, {{"Backup", Cap::Workflow::Backup}, {"Review", Cap::Workflow::Review}}) + " |";
            out += " " + statusCell(row.status(Cap::Workflow::Export)) + " |";
            out += " " + escapeCell(ciCell(row)) + " |";
            out += " " + hardwareCell(row) + " |\n";
        }
        out += "\n";
    }
    out += "Rows: " + std::to_string(rows) + " titles.\n\n";

    // ---- native mechanics per Gen VI-IX title ----
    out += "## Native mechanics per title (Gen VI-IX)\n\n";
    out += "Flags are the `NativeMechanics` members set to true in the registry; the shared editor offers only "
           "these mechanics' rows on that title, and the \"Native mechanics\" column above says whether they are "
           "editable there.\n\n";
    out += "| Game | Generation | Provider | Stat model | Storage | Native mechanics (true flags) |\n|---|---|---|---|---|---|\n";
    for (const auto& row : Cap::all()) {
        if (row.generation < 6) continue;
        out += "| " + gameCell(row) + " | " + roman(row.generation) + " | " + std::string(row.provider) + " | " +
               statModelName(row.statModel) + " | " + std::to_string(static_cast<int>(row.storage.boxCount)) + " x " +
               std::to_string(static_cast<int>(row.storage.slotsPerBox)) + " (" + std::string(row.storage.model) +
               ") | " + escapeCell(mechanicsList(row.mechanics)) + " |\n";
    }
    out += "\n";

    // ---- blockers ----
    out += "## Blockers\n\n";
    out += "Every Blocked reason in the registry, once, with the workflows and titles it applies to. A blocker is "
           "lifted only by changing the registry row together with the adapter and host tests that prove it.\n\n";
    std::vector<std::string> order;
    std::map<std::string, BlockerEntry> blockers;
    for (const auto& row : Cap::all()) {
        for (size_t i = 0; i < Cap::WorkflowCount; ++i) {
            const auto w = static_cast<Cap::Workflow>(i);
            const auto& s = row.status(w);
            if (s.support != Cap::Support::Blocked) continue;
            const std::string reason(s.reason);
            if (!blockers.count(reason)) order.push_back(reason);
            auto& entry = blockers[reason];
            append(entry.workflows, std::string(Cap::workflowName(w)));
            append(entry.titles, std::string(row.title));
        }
    }
    if (order.empty()) out += "(no Blocked workflow)\n";
    for (const auto& reason : order) {
        const auto& entry = blockers[reason];
        out += "- " + escapeCell(reason) + "\n";
        out += "  - Workflows: " + join(entry.workflows) + "\n";
        out += "  - Titles: " + join(entry.titles) + "\n";
    }
    out += "\n";
    out += "Generated by tools/capability_matrix.cpp from GameCapabilityRegistry.h; do not edit by hand.\n";

    std::fwrite(out.data(), 1, out.size(), stdout);
    return std::fflush(stdout) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
