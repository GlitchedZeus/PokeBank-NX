// Scarlet / Violet (SAV9SV / PK9) adapter of the shared Modern::NativeSession contract, on synthetic
// SwishCrypto images (tests/fixtures/gen9sv): strict open (PKHeX size rule, container hash, game byte,
// party), PK9 view with Tera types as native read-only facts, the shared PK8/PK9 provider table,
// staged edits, box Clone/Move/Remove with battle-team protection, draft bracket, Export Copy round
// trip and verifier refusals. Z-A is never accepted here.

#include "Integration/Gen9/Gen9SvNativeSession.h"
#include "fixtures/gen9sv/Gen9SvSyntheticSave.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace SV = PokeVault::Integration::Gen9Sv;
namespace Modern = PokeVault::Integration::Modern;
namespace L = PokeVault::Integration::Gen9Sv::Layout;
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
    const auto pristine = Gen9SvFixture::makeSave();
    check(PokeVault::Integration::Gen8::Swish::encrypt(SV::Gen9SvSave::parse(pristine)->blocks()) == pristine, "canonical round trip");
    auto opened = SV::Gen9SvNativeSession::open(pristine, "scarlet_switch");
    check(opened.ready(), "Scarlet opens: " + opened.diagnostic);
    check(!SV::Gen9SvNativeSession::open(pristine, "violet_switch").ready(), "Scarlet save refused as Violet");
    Gen9SvFixture::Options violet;
    violet.game = L::VersionViolet;
    violet.fileSize = L::DlcMin2 + L::Add300 + 7;   // Indigo Disk range
    check(SV::Gen9SvNativeSession::open(Gen9SvFixture::makeSave(violet), "violet_switch").ready(), "Violet in the DLC size range opens");
    Gen9SvFixture::Options za;
    za.fileSize = 0x2F3284;  // a Legends: Z-A size: never accepted as SV
    check(!SV::Gen9SvNativeSession::open(Gen9SvFixture::makeSave(za), "").ready(), "a Z-A-sized file is refused (Z-A is not SV)");
    {
        auto bad = pristine;
        bad[200] ^= 1;
        check(!SV::Gen9SvNativeSession::open(bad, "").ready(), "container hash mismatch refused");
    }
    if (!opened.ready()) return 1;
    auto& s = *opened.session;
    auto& n = static_cast<SV::Gen9SvNativeSession&>(s);
    std::string error;
    check(s.capabilities().title == "Scarlet" && s.familyLabel() == "Generation IX" && s.boxCount() == 32 && s.partyCount() == 2,
          "identity and 32 x 30 geometry");
    const auto v = s.view(SlotRef::boxed(0, 0));
    check(v && v->species == 921 && v->nickname == "SYNTH" && hasFact(*v, "Tera Type (original / override)") && hasFact(*v, "Scale"),
          "PK9 view with Tera/scale facts");
    check(s.fieldState(Field::Pokerus, Modern::Region::Box) == State::Editable && s.fieldState(Field::IV, Modern::Region::Box) == State::ReadOnly,
          "shared PK8/PK9 provider table");
    check(s.trainerName() == "Juliana" && s.trainerId() == 11223 && s.trainerGender() == 1, "trainer from MyStatus9");
    check(s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "Pawmi", error) && s.stageValue(SlotRef::party(0), Field::Friendship, 210, 0, error) &&
              s.stageValue(SlotRef::boxed(0, 0), Field::Ball, 4, 0, error),
          "edits: " + error);
    check(!s.removeSupport(SlotRef::boxed(0, 1)).available && s.cloneSupport(SlotRef::boxed(0, 1)).available, "team member: clone yes, remove no");
    check(s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(2, 2), error) && s.move(SlotRef::boxed(31, 29), SlotRef::boxed(3, 3), error) &&
              s.view(SlotRef::boxed(3, 3))->species == 1007 && s.view(SlotRef::boxed(31, 29))->empty,
          "clone and move: " + error);
    check(!s.cloneSupport(SlotRef::party(0)).available && !s.createSupport(SlotRef::boxed(9, 9)).available, "party storage and create refused");
    Gen9SvFixture::Options locked;
    locked.lockTeam0 = true;
    auto l = SV::Gen9SvNativeSession::open(Gen9SvFixture::makeSave(locked), "scarlet_switch");
    check(l.ready() && !l.session->stageValue(SlotRef::boxed(0, 1), Field::Pokerus, 1, 0, error), "locked-team member edit refused");
    check(s.beginDraft(SlotRef::boxed(0, 0), error) && s.stageValue(SlotRef::boxed(0, 0), Field::TrainerId, 7, 0, error), "draft");
    s.discardDraft();
    check(s.view(SlotRef::boxed(0, 0))->tid != 7 && s.view(SlotRef::boxed(0, 0))->nickname == "Pawmi", "discard draft keeps earlier stage");

    char pattern[] = "/tmp/gen9sv-native-XXXXXX";
    const std::string root = ::mkdtemp(pattern) ? pattern : "";
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.timestamp = "20261101_010000";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export: " + result.error);
    if (result.success) {
        check(slurp(result.directory + "/original_backup.bin") == pristine, "backup pristine");
        auto re = SV::Gen9SvNativeSession::open(slurp(result.directory + "/edited.bin"), "scarlet_switch");
        check(re.ready() && re.session->view(SlotRef::boxed(0, 0))->nickname == "Pawmi" && re.session->view(SlotRef::party(0))->friendship == 210 &&
                  re.session->view(SlotRef::boxed(3, 3))->species == 1007,
              "edited copy strictly reopens with the changes");
    }
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    const auto built = n.staged().buildWorkingCopy(&error);
    check(built.has_value(), "working copy builds: " + error);
    if (built) {
        const auto changes = n.staged().changes();
        check(SV::verifyWorkingCopy(n.staged().source(), *built, changes, &error), "verifier accepts");
        auto blocks = SV::Gen9SvSave::parse(*built)->blocks();
        for (auto& b : blocks) if (b.key == L::KeyMyStatus) b.data[L::MyTid] ^= 1;
        check(!SV::verifyWorkingCopy(n.staged().source(), PokeVault::Integration::Gen8::Swish::encrypt(blocks), changes, &error),
              "foreign block change refused");
        auto forged = changes;
        for (auto& c : forged) if (c.kind == SV::ChangeKind::Edit) PK8::write16(c.stagedPlain, 0x08, 25);
        check(!SV::verifyWorkingCopy(n.staged().source(), *built, forged, &error), "a forged species change is refused");
    }
    check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin(), pristine.end()), "source unchanged");
    std::cout << "test_gen9sv_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
