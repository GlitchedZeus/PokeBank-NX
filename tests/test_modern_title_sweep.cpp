// Integration hardening sweep across ALL 18 Generation VI-IX titles served by the shared Modern layer.
// Every title is opened through the one routing entry point (Modern::openSession) on a synthetic save,
// then checked for the same contract:
//   * format identity: the exact title opens; each of the other 17 title ids refuses the same bytes
//     (incorrect title assignment never opens, including Z-A vs SV and BD vs SP);
//   * registry <-> adapter agreement on blocked operations (Create everywhere, Move on Let's Go,
//     Export on Sun/Moon/Ultra), and party records never offer Release;
//   * team / partner restrictions where the format has them (SWSH, BD/SP, SV, Z-A battle teams;
//     the Let's Go partner);
//   * edit -> verified Export Copy -> backup equals the source, the edited copy strictly reopens as the
//     same title and still refuses every other title; SAV7 writes nothing;
//   * original-save immutability: the caller's buffer and the adapter's source bytes never change.
// Plus: a Scarlet/Violet DLC-sized save is readable through Save Instances (the old 4 MiB cap).

#include "Integration/Common/GameCapabilityRegistry.h"
#include "Integration/Modern/ModernSources.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"
#include "fixtures/gen7b/Gen7bSyntheticSave.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"
#include "fixtures/gen8a/Gen8aSyntheticSave.h"
#include "fixtures/gen8b/Gen8bSyntheticSave.h"
#include "fixtures/gen9sv/Gen9SvSyntheticSave.h"
#include "fixtures/gen9za/Gen9ZaSyntheticSave.h"

#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace Modern = PokeVault::Integration::Modern;
namespace Cap = PokeVault::Integration::Capability;
namespace L6 = PokeVault::Integration::Gen6::Layout;
namespace L = PokeVault::Integration::Gen9Sv::Layout;
namespace L7 = PokeVault::Integration::Gen7::Layout;
using Modern::SlotRef;
using Field = Modern::Field;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
std::vector<uint8_t> slurp(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
bool emptyDir(const std::string& d) {
    DIR* dir = ::opendir(d.c_str());
    if (!dir) return true;
    size_t n = 0;
    while (const dirent* e = ::readdir(dir)) n += std::string(e->d_name) != "." && std::string(e->d_name) != "..";
    ::closedir(dir);
    return n == 0;
}

struct Title {
    std::string id;
    std::function<std::vector<uint8_t>()> make;
    enum class Teams { None, BattleTeamBox1Slot2, PartnerBox1Slot1 } teams = Teams::None;
};

std::vector<Title> titles() {
    using G6 = Gen6Fixture::SaveSpec;
    using G7 = Gen7Fixture::SaveSpec;
    std::vector<Title> t;
    t.push_back({"x_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::XY, L6::VersionX}); }});
    t.push_back({"y_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::XY, L6::VersionY}); }});
    t.push_back({"omegaruby_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::ORAS, L6::VersionOmegaRuby}); }});
    t.push_back({"alphasapphire_3ds", [] { return Gen6Fixture::makeSave(G6{L6::Family::ORAS, L6::VersionAlphaSapphire}); }});
    t.push_back({"sun_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::SunMoon, L7::VersionSun}); }});
    t.push_back({"moon_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::SunMoon, L7::VersionMoon}); }});
    t.push_back({"ultrasun_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::UltraSunUltraMoon, L7::VersionUltraSun}); }});
    t.push_back({"ultramoon_3ds", [] { return Gen7Fixture::makeSave(G7{L7::Family::UltraSunUltraMoon, L7::VersionUltraMoon}); }});
    t.push_back({"letsgo_pikachu_switch", [] { return Gen7bFixture::makeSave(); }, Title::Teams::PartnerBox1Slot1});
    t.push_back({"letsgo_eevee_switch", [] { Gen7bFixture::Options o; o.version = 43; return Gen7bFixture::makeSave(o); },
                 Title::Teams::PartnerBox1Slot1});
    t.push_back({"sword_switch", [] { return Gen8Fixture::makeSave(); }, Title::Teams::BattleTeamBox1Slot2});
    t.push_back({"shield_switch", [] { Gen8Fixture::Options o; o.version = 45; return Gen8Fixture::makeSave(o); },
                 Title::Teams::BattleTeamBox1Slot2});
    t.push_back({"brilliant_diamond_switch", [] { return Gen8bFixture::makeSave(); }, Title::Teams::BattleTeamBox1Slot2});
    t.push_back({"shining_pearl_switch", [] { Gen8bFixture::Options o; o.romCode = 1; return Gen8bFixture::makeSave(o); },
                 Title::Teams::BattleTeamBox1Slot2});
    t.push_back({"legends_arceus_switch", [] { return Gen8aFixture::makeSave(); }});
    t.push_back({"scarlet_switch", [] { return Gen9SvFixture::makeSave(); }, Title::Teams::BattleTeamBox1Slot2});
    t.push_back({"violet_switch", [] { Gen9SvFixture::Options o; o.game = 51; return Gen9SvFixture::makeSave(o); },
                 Title::Teams::BattleTeamBox1Slot2});
    t.push_back({"legends_za_switch", [] { return Gen9ZaFixture::makeSave(); }, Title::Teams::BattleTeamBox1Slot2});
    return t;
}
} // namespace

int main() {
    const auto all = titles();
    // Every Gen VI-IX registry row is in the sweep and served by the shared adapter.
    size_t modernRows = 0;
    for (const auto& row : Cap::all())
        if (row.generation >= 6) {
            ++modernRows;
            bool inSweep = false;
            for (const auto& t : all) inSweep |= t.id == row.id;
            check(inSweep && Modern::isModernAssignedId(row.id), std::string(row.id) + " is swept and adapter-served");
        }
    check(modernRows == 18 && all.size() == 18, "18 modern titles");

    const std::string root = [] {
        char pattern[] = "/tmp/modern-sweep-XXXXXX";
        const char* d = ::mkdtemp(pattern);
        return std::string(d ? d : "");
    }();
    for (const auto& t : all) {
        const auto* row = Cap::find(t.id);
        const auto pristine = t.make();
        auto caller = pristine;  // the caller's buffer: must never change

        // ---- format identity: exact title only ----
        auto opened = Modern::openSession(t.id, caller);
        check(opened.ready(), t.id + " opens: " + opened.diagnostic);
        for (const auto& other : all)
            if (other.id != t.id)
                check(!Modern::openSession(other.id, caller).ready(), t.id + " bytes refused as " + other.id);
        if (!opened.ready()) continue;
        auto& s = *opened.session;
        check(s.exactGameId() == t.id && s.capabilities().id == t.id && row && s.familyLabel() == Modern::generationLabelFor(t.id),
              t.id + " identity matches the registry");

        // ---- registry <-> adapter agreement on blocked operations ----
        check(!s.createSupport(SlotRef::boxed(5, 5)).available && !s.createSupport(SlotRef::boxed(5, 5)).reason.empty(),
              t.id + " Create unavailable with a reason");
        check(row->status(Cap::Workflow::Create).support == Cap::Support::Blocked, t.id + " registry blocks Create");
        check(!s.removeSupport(SlotRef::party(0)).available && !s.removeSupport(SlotRef::party(0)).reason.empty(),
              t.id + " party Release never offered");
        if (row->status(Cap::Workflow::Move).support == Cap::Support::Blocked)
            check(!s.moveSupport(SlotRef::boxed(0, 2)).available, t.id + " registry-blocked Move is refused by the adapter");

        // ---- Hyper Training (PKSE 1.2.2 port): PK6 has none; SM/USUM edit it (stats recomputed); every other
        //      format shows it read-only because its cached stats cannot be recomputed from a pinned table ----
        {
            const bool gen6 = t.id == "x_3ds" || t.id == "y_3ds" || t.id == "omegaruby_3ds" || t.id == "alphasapphire_3ds";
            const bool gen7 = t.id == "sun_3ds" || t.id == "moon_3ds" || t.id == "ultrasun_3ds" || t.id == "ultramoon_3ds";
            const auto ht = s.fieldState(Field::HyperTraining, Modern::Region::Box);
            check(ht == (gen6 ? Modern::FieldState::Hidden : gen7 ? Modern::FieldState::Editable : Modern::FieldState::ReadOnly),
                  t.id + " Hyper Training field state");
            std::string htError;
            if (!gen7)
                check(!s.stageValue(SlotRef::party(0), Field::HyperTraining, 0x01, 0, htError) && !s.hasChanges(),
                      t.id + " Hyper Training is never staged where it is not editable");
            check(s.view(SlotRef::party(0)) && s.view(SlotRef::party(0))->hyperTrainFlags <= 0x3F, t.id + " flags are six bits");
        }

        // ---- team / partner restrictions ----
        if (t.teams == Title::Teams::BattleTeamBox1Slot2) {
            const auto r = s.removeSupport(SlotRef::boxed(0, 1));
            check(!r.available && std::string(r.reason).find("team") != std::string::npos, t.id + " battle-team member never released");
            check(!s.moveSupport(SlotRef::boxed(0, 1)).available, t.id + " battle-team member never moved");
        } else if (t.teams == Title::Teams::PartnerBox1Slot1) {
            const auto r = s.removeSupport(SlotRef::boxed(0, 0));
            check(!r.available && std::string(r.reason).find("partner") != std::string::npos, t.id + " partner never released");
        }

        // ---- edit -> export ----
        std::string error;
        check(s.stageText(SlotRef::party(0), Field::Nickname, "SWEEP", error) && s.view(SlotRef::party(0))->nickname == "SWEEP",
              t.id + " party nickname staged: " + error);
        const std::string out = root + "/" + t.id;
        std::filesystem::create_directories(out);
        Modern::ExportRequest req;
        req.rootDirectory = out;
        req.timestamp = "20261101_120000";
        const bool exportClaimed = row->supports(Cap::Workflow::Export);
        check(s.exportSupport().available == exportClaimed, t.id + " export availability matches the registry");
        const auto result = s.exportWorkingCopy(req);
        if (!exportClaimed) {
            check(!result.success && emptyDir(out), t.id + " blocked export writes nothing");
        } else {
            check(result.success, t.id + " export succeeds: " + result.error);
            if (result.success) {
                std::string backupName, editedName;
                for (const auto& e : std::filesystem::directory_iterator(result.directory)) {
                    const auto n = e.path().filename().string();
                    if (n.rfind("original_backup", 0) == 0) backupName = n;
                    if (n.rfind("edited", 0) == 0) editedName = n;
                }
                check(!backupName.empty() && slurp(result.directory + "/" + backupName) == pristine, t.id + " backup equals the source");
                const auto edited = slurp(result.directory + "/" + editedName);
                check(edited.size() == pristine.size() && edited != pristine, t.id + " edited copy differs, same size");
                auto re = Modern::openSession(t.id, edited);
                check(re.ready() && re.session->view(SlotRef::party(0))->nickname == "SWEEP" && !re.session->hasChanges(),
                      t.id + " edited copy strictly reopens with the change");
                bool refusedElsewhere = true;
                for (const auto& other : all)
                    if (other.id != t.id) refusedElsewhere = refusedElsewhere && !Modern::openSession(other.id, edited).ready();
                check(refusedElsewhere, t.id + " edited copy keeps its format identity");
            }
        }

        // ---- immutability ----
        check(caller == pristine, t.id + " caller buffer unchanged");
        check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin(), pristine.end()),
              t.id + " adapter source bytes unchanged");
        s.discardAll();
        check(!s.hasChanges() && s.view(SlotRef::party(0))->nickname != "SWEEP", t.id + " discard all restores the view");
    }

    // ---- Scarlet/Violet DLC-sized saves pass the Save Instances read cap (was 4 MiB) ----
    {
        Gen9SvFixture::Options dlc;
        dlc.fileSize = L::DlcMin2 + L::Add300 + 7;
        const auto bytes = Gen9SvFixture::makeSave(dlc);
        const std::string path = root + "/sv-dlc-main";
        { std::ofstream o(path, std::ios::binary); o.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); }
        const auto row = Modern::inspectSourceFile(path, "Manual", "scarlet_switch");
        check(bytes.size() > (4u << 20) && row.ready(), "a " + std::to_string(bytes.size()) + "-byte SV DLC save validates: " + row.diagnostic);
    }
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::cout << "test_modern_title_sweep: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
