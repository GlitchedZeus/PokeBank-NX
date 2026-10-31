// The capability registry is the single source of truth for which shared workflows each title
// supports. These checks enforce the cumulative-inheritance rule (a later generation never drops a
// shared workflow silently), that every registered id is a real game identity, and that native
// mechanics are declared only for the titles that have them.
#include "Games/GameIdentity.h"
#include "Integration/Common/GameCapabilityRegistry.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace Cap = PokeVault::Integration::Capability;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
} // namespace

int main() {
    const auto rows = Cap::all();
    check(rows.size() == 38, "38 titles: 20 Gen I-V + 4 Gen VI + 4 SM/USUM + 2 LGPE + 5 Gen VIII + 3 Gen IX");

    std::set<std::string> ids;
    for (const auto& row : rows) {
        check(ids.insert(std::string(row.id)).second, "unique id " + std::string(row.id));
        const auto* identity = PokeVault::Games::findGame(row.id);
        check(identity != nullptr, "registry id is a GameIdentity descriptor: " + std::string(row.id));
        if (identity) {
            check(identity->dataGeneration == row.generation, "generation agrees with GameIdentity: " + std::string(row.id));
            check(identity->title == row.title, "title agrees with GameIdentity: " + std::string(row.id));
        }
        check(Cap::find(row.id) == &row, "find() returns the row: " + std::string(row.id));
        // Blocked / NotApplicable always name why.
        for (size_t i = 0; i < Cap::WorkflowCount; ++i) {
            const auto& s = row.workflows[i];
            if (s.support == Cap::Support::Blocked || s.support == Cap::Support::NotApplicable)
                check(!s.reason.empty(), std::string(row.id) + " names why " +
                                             std::string(Cap::workflowName(static_cast<Cap::Workflow>(i))) + " is unavailable");
        }
        check(row.storage.boxCount > 0 && row.storage.slotsPerBox > 0, "storage geometry present: " + std::string(row.id));
    }

    // Every Gen I-V row is the owner-accepted baseline with every workflow at least visible.
    for (const auto& row : rows) {
        if (row.generation > 5) continue;
        for (size_t i = 0; i < Cap::WorkflowCount; ++i)
            check(row.workflows[i].support == Cap::Support::Supported || row.workflows[i].support == Cap::Support::ReadOnly,
                  std::string(row.id) + " baseline workflow visible: " + std::string(Cap::workflowName(static_cast<Cap::Workflow>(i))));
    }

    // Cumulative inheritance (also a static_assert in the header).
    check(!Cap::firstInheritanceViolation().has_value(), "no inherited workflow is NotImplemented or unexplained");
    for (const auto& row : rows)
        for (size_t i = 0; i < Cap::WorkflowCount; ++i)
            check(row.workflows[i].support != Cap::Support::NotImplemented,
                  std::string(row.id) + " has no NotImplemented workflow");

    // Gen VI: the shared Modern adapter serves it with a verified Export Copy.
    for (const char* id : {"x_3ds", "y_3ds", "omegaruby_3ds", "alphasapphire_3ds"}) {
        const auto* row = Cap::find(id);
        check(row && row->provider == "modern-native", std::string(id) + " served by the Modern adapter");
        check(row && row->supports(Cap::Workflow::Export) && row->exportRoot == "exports/gen6", std::string(id) + " exports to exports/gen6");
        check(row && row->supports(Cap::Workflow::Clone) && row->supports(Cap::Workflow::Remove) && row->supports(Cap::Workflow::Move),
              std::string(id) + " Clone/Move/Remove supported");
        check(row && row->status(Cap::Workflow::Create).support == Cap::Support::Blocked, std::string(id) + " Create blocked with reason");
        check(row && row->mechanics.megaForms && row->mechanics.fairyType && row->mechanics.superTraining && !row->mechanics.zMoves &&
              !row->mechanics.dynamax && !row->mechanics.teraType, std::string(id) + " declares only Gen VI mechanics");
        check(row && row->storage.boxCount == 31 && row->storage.slotsPerBox == 30, std::string(id) + " 31x30 boxes");
    }
    // Gen VII SM/USUM: same adapter, export blocked by the signing gate.
    for (const char* id : {"sun_3ds", "moon_3ds", "ultrasun_3ds", "ultramoon_3ds"}) {
        const auto* row = Cap::find(id);
        check(row && row->provider == "modern-native", std::string(id) + " served by the Modern adapter");
        check(row && row->status(Cap::Workflow::Export).support == Cap::Support::Blocked &&
                  row->status(Cap::Workflow::Export).reason.find("SAV7") != std::string_view::npos,
              std::string(id) + " export blocked by SAV7 signing");
        check(row && row->mechanics.zMoves && row->mechanics.alolanForms && row->mechanics.hyperTraining && !row->mechanics.dynamax,
              std::string(id) + " declares Z-Moves/Alolan/Hyper Training only");
        check(row && row->storage.boxCount == 32, std::string(id) + " 32 boxes");
    }
    // Mechanics belong to the titles that have them.
    for (const auto& row : rows) {
        const auto& m = row.mechanics;
        const std::string id(row.id);
        check(m.zMoves == (row.family == "SM" || row.family == "USUM"), id + " Z-Moves only in SM/USUM");
        check(m.dynamax == (row.family == "SWSH"), id + " Dynamax only in SWSH");
        check(m.teraType == (row.family == "SV"), id + " Tera only in SV");
        check(m.awakeningValues == (row.family == "LGPE"), id + " Awakening Values only in LGPE");
        check(m.effortLevels == (row.family == "PLA"), id + " Effort Levels only in PLA");
        check(m.plusFlags == (row.family == "ZA"), id + " Plus flags only in Z-A");
        check(m.fairyType == (row.generation >= 6), id + " Fairy from Gen VI");
        if (row.family == "LGPE") check(row.statModel == Cap::StatModel::IVAwakening, id + " AV stat model");
        if (row.family == "PLA") check(row.statModel == Cap::StatModel::IVEffortLevel, id + " effort-level stat model");
        if (row.generation <= 2) check(row.statModel == Cap::StatModel::DVStatExp, id + " DV stat model");
    }
    // Z-A is its own family with its own record format, never an SV alias.
    const auto* za = Cap::find("legends_za_switch");
    check(za && za->family == "ZA" && za->recordFormat == "PA9" && !za->mechanics.teraType, "Z-A is PA9/SAV9ZA, not SV");
    // Switch-reader titles are honest: editing blocked with the adapter prerequisite named.
    for (const char* id : {"letsgo_pikachu_switch", "sword_switch", "brilliant_diamond_switch", "legends_arceus_switch",
                           "scarlet_switch", "legends_za_switch"}) {
        const auto* row = Cap::find(id);
        check(row && row->status(Cap::Workflow::PokemonEditor).support == Cap::Support::Blocked &&
                  !row->status(Cap::Workflow::PokemonEditor).reason.empty(),
              std::string(id) + " editor blocked with prerequisite");
        check(row && row->status(Cap::Workflow::Export).support == Cap::Support::Blocked, std::string(id) + " export blocked");
        check(row && row->exportRoot.empty(), std::string(id) + " advertises no export root");
    }
    check(Cap::isModernNativeId("x_3ds") && !Cap::isModernNativeId("black_nds") && !Cap::isModernNativeId("sword_switch"),
          "isModernNativeId identifies the Modern adapter titles only");

    std::cout << "test_game_capability_registry: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
