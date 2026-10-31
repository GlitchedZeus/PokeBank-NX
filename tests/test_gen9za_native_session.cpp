// Legends: Z-A (SAV9ZA / PA9) adapter of the shared Modern::NativeSession contract, on synthetic images
// (tests/fixtures/gen9za): strict open (Z-A sizes, hash, game 52, Party9a leading-present count), PA9
// view (Alpha, revision), the shared provider table, staged edits, box Clone/Move/Remove with team
// protection, Export Copy round trip with the 0x198 stride and the DLC present-flag rule, verifier
// refusals. An SV-sized save is never accepted as Z-A.

#include "Integration/Gen9/Gen9ZaNativeSession.h"
#include "fixtures/gen9za/Gen9ZaSyntheticSave.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace SV = PokeVault::Integration::Gen9Za;
namespace Modern = PokeVault::Integration::Modern;
namespace L = PokeVault::Integration::Gen9Za::Layout;
namespace PK8 = PokeVault::Integration::Gen8::PK8;
using Modern::SlotRef;
using Field = Modern::Field;
using State = Modern::FieldState;

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
bool hasFact(const Modern::RecordView& v, const std::string& k) {
    for (const auto& f : v.nativeFacts) if (f.first == k) return true;
    return false;
}
} // namespace

int main() {
    const auto pristine = Gen9ZaFixture::makeSave();
    check(PokeVault::Integration::Gen8::Swish::encrypt(SV::Gen9ZaSave::parse(pristine)->blocks()) == pristine, "canonical round trip");
    auto opened = SV::Gen9ZaNativeSession::open(pristine, "legends_za_switch");
    check(opened.ready(), "Scarlet opens: " + opened.diagnostic);
    Gen9ZaFixture::Options dlc;
    dlc.dlc = true;
    const auto dlcSave = Gen9ZaFixture::makeSave(dlc);
    auto dlcOpened = SV::Gen9ZaNativeSession::open(dlcSave, "legends_za_switch");
    check(dlcOpened.ready() && static_cast<SV::Gen9ZaNativeSession&>(*dlcOpened.session).staged().source().revision() == 1,
          "the DLC revision opens and reports revision 1");
    std::vector<uint8_t> svSized(0x31626F, 0);
    check(!SV::Gen9ZaNativeSession::open(svSized, "").ready(), "a Scarlet/Violet-sized file is refused (Z-A is not SV)");
    Gen9ZaFixture::Options wrongGame;
    wrongGame.game = 50;
    check(!SV::Gen9ZaNativeSession::open(Gen9ZaFixture::makeSave(wrongGame), "").ready(), "game byte 50 (Scarlet) refused");
    {
        auto bad = pristine;
        bad[200] ^= 1;
        check(!SV::Gen9ZaNativeSession::open(bad, "").ready(), "container hash mismatch refused");
    }
    if (!opened.ready()) return 1;
    auto& s = *opened.session;
    auto& n = static_cast<SV::Gen9ZaNativeSession&>(s);
    std::string error;
    check(s.capabilities().title == "Legends Z-A" && s.familyLabel() == "Generation IX" && s.boxCount() == 32 && s.partyCount() == 2,
          "identity and 32 x 30 geometry");
    const auto v = s.view(SlotRef::boxed(0, 0));
    check(v && v->species == 25 && v->nickname == "SYNTH" && hasFact(*v, "Alpha") && hasFact(*v, "Save revision"),
          "PA9 view with Alpha/revision facts");
    check(s.fieldState(Field::Pokerus, Modern::Region::Box) == State::Editable && s.fieldState(Field::IV, Modern::Region::Box) == State::ReadOnly,
          "shared PK8/PK9 provider table");
    check(s.trainerName() == "Taunie" && s.trainerId() == 52000 && s.trainerGender() == 0, "trainer");
    check(s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "Pawmi", error) && s.stageValue(SlotRef::party(0), Field::Friendship, 210, 0, error) &&
              s.stageValue(SlotRef::boxed(0, 0), Field::Ball, 4, 0, error),
          "edits: " + error);
    check(!s.removeSupport(SlotRef::boxed(0, 1)).available && s.cloneSupport(SlotRef::boxed(0, 1)).available, "team member: clone yes, remove no");
    check(s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(2, 2), error) && s.move(SlotRef::boxed(31, 29), SlotRef::boxed(3, 3), error) &&
              s.view(SlotRef::boxed(3, 3))->species == 718 && s.view(SlotRef::boxed(31, 29))->empty,
          "clone and move: " + error);
    check(!s.cloneSupport(SlotRef::party(0)).available && !s.createSupport(SlotRef::boxed(9, 9)).available, "party storage and create refused");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::TrainerId, 7, 0, error), "draft");
    s.discardDraft();
    check(s.view(SlotRef::boxed(0, 0))->tid != 7 && s.view(SlotRef::boxed(0, 0))->nickname == "Pawmi", "discard draft keeps earlier stage");

    char pattern[] = "/tmp/gen9za-native-XXXXXX";
    const std::string root = ::mkdtemp(pattern) ? pattern : "";
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.timestamp = "20261101_010000";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export: " + result.error);
    if (result.success) {
        check(slurp(result.directory + "/original_backup.bin") == pristine, "backup pristine");
        auto re = SV::Gen9ZaNativeSession::open(slurp(result.directory + "/edited.bin"), "legends_za_switch");
        check(re.ready() && re.session->view(SlotRef::boxed(0, 0))->nickname == "Pawmi" && re.session->view(SlotRef::party(0))->friendship == 210 &&
                  re.session->view(SlotRef::boxed(3, 3))->species == 718,
              "edited copy strictly reopens with the changes");
    }
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    const auto built = n.staged().buildWorkingCopy(&error);
    check(built.has_value(), "working copy builds: " + error);
    if (built) {
        const auto changes = n.staged().changes();
        check(SV::verifyWorkingCopy(n.staged().source(), *built, changes, &error), "verifier accepts");
        auto blocks = SV::Gen9ZaSave::parse(*built)->blocks();
        for (auto& b : blocks) if (b.key == L::KeyMyStatus) b.data[L::MyTid] ^= 1;
        check(!SV::verifyWorkingCopy(n.staged().source(), PokeVault::Integration::Gen8::Swish::encrypt(blocks), changes, &error),
              "foreign block change refused");
        auto forged = changes;
        for (auto& c : forged) if (c.kind == SV::ChangeKind::Edit) PK8::write16(c.stagedPlain, 0x08, 25);
        check(!SV::verifyWorkingCopy(n.staged().source(), *built, forged, &error), "a forged species change is refused");
    }
    check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin(), pristine.end()), "source unchanged");
    // DLC revision: a written box slot is marked present; gap bytes survive; party footers untouched.
    if (dlcOpened.ready()) {
        auto& d = static_cast<SV::Gen9ZaNativeSession&>(*dlcOpened.session);
        check(dlcOpened.session->move(SlotRef::boxed(31, 29), SlotRef::boxed(5, 5), error), "DLC move: " + error);
        const auto copy = d.staged().buildWorkingCopy(&error);
        check(copy.has_value(), "DLC working copy: " + error);
        if (copy) {
            const auto re = SV::Gen9ZaSave::parse(*copy);
            const auto& box = re->blocks()[re->boxIndex()].data;
            const size_t dst = (5 * L::BoxSlots + 5) * L::BoxStride, src = (31 * L::BoxSlots + 29) * L::BoxStride;
            check(box[dst + L::PresentFlag] == 1 && box[src + L::PresentFlag] == 1 &&
                      box[dst + L::PresentFlag + 1] == static_cast<uint8_t>(5 * L::BoxSlots + 5),
                  "present flags set on both written slots; gap bytes preserved");
        }
    }

    std::cout << "test_gen9za_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
