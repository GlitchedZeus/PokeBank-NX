// Legends: Arceus (SAV8LA / PA8) adapter of the shared Modern::NativeSession contract, on synthetic
// SwishCrypto images (tests/fixtures/gen8a): strict open (size, container hash, game byte, party),
// PA8 view with Alpha / Effort Levels as native read-only facts, the provider table, staged edits
// (Hisui balls, PLA held items), box Clone/Move/Remove, party storage refusal, draft bracket, Export
// Copy round trip and verifier refusals.

#include "Integration/Gen8a/Gen8aNativeSession.h"
#include "fixtures/gen8a/Gen8aSyntheticSave.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace G8a = PokeVault::Integration::Gen8a;
namespace Modern = PokeVault::Integration::Modern;
namespace L = PokeVault::Integration::Gen8a::Layout;
namespace PA8 = PokeVault::Integration::Gen8a::PA8;
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
bool fact(const Modern::RecordView& v, const std::string& k, const std::string& val) {
    for (const auto& f : v.nativeFacts) if (f.first == k && f.second == val) return true;
    return false;
}
} // namespace

int main() {
    const auto pristine = Gen8aFixture::makeSave();
    check(PokeVault::Integration::Gen8::Swish::encrypt(G8a::Gen8aSave::parse(pristine)->blocks()) == pristine,
          "prerequisite: canonical SwishCrypto round trip");
    auto opened = G8a::Gen8aNativeSession::open(pristine, "legends_arceus_switch");
    check(opened.ready(), "opens: " + opened.diagnostic);
    Gen8aFixture::Options sv;
    sv.game = 50;
    check(!G8a::Gen8aNativeSession::open(Gen8aFixture::makeSave(sv), "").ready(), "a non-PLA game byte is refused");
    Gen8aFixture::Options v11;
    v11.fileSize = L::Sizes[1];
    check(G8a::Gen8aNativeSession::open(Gen8aFixture::makeSave(v11), "legends_arceus_switch").ready(), "the 1.1 size opens");
    {
        auto bad = pristine;
        bad[100] ^= 1;
        check(!G8a::Gen8aNativeSession::open(bad, "").ready(), "container hash mismatch refused");
    }
    if (!opened.ready()) return 1;
    auto& s = *opened.session;
    auto& n = static_cast<G8a::Gen8aNativeSession&>(s);
    std::string error;

    check(s.capabilities().title == "Legends Arceus" && s.boxCount() == 32 && s.slotsPerBox() == 30 && s.partyCount() == 2,
          "identity and 32 x 30 geometry");
    const auto alpha = s.view(SlotRef::boxed(0, 0));
    check(alpha && alpha->species == 399 && alpha->nickname == "HISUI" && alpha->otName == "Rei" && fact(*alpha, "Alpha", "Yes") &&
              fact(*alpha, "Effort Levels", "HP 0 Atk 1 Def 2 Spe 3 SpA 0 SpD 1"),
          "box PA8 view with Alpha and Effort Levels");
    const auto p = s.view(SlotRef::party(1));
    check(p && p->species == 723 && p->level == 6 && p->stats[0] == 18, "party PA8 with cached stats");
    check(s.fieldState(Field::EffortLevel, Modern::Region::Box) == State::ReadOnly && s.fieldState(Field::Pokerus, Modern::Region::Box) == State::Hidden &&
              s.fieldState(Field::EV, Modern::Region::Party) == State::Hidden && s.fieldState(Field::Ball, Modern::Region::Box) == State::Editable,
          "PLA provider table");
    check(s.ballChoices().front() == 27 && s.ballChoices().back() == 37 && s.trainerName() == "Akari" && s.trainerGender() == 1,
          "Hisui balls and trainer");

    check(s.stageText(SlotRef::boxed(0, 0), Field::Nickname, "Bidoof", error) && s.view(SlotRef::boxed(0, 0))->nickname == "Bidoof", "nickname");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::Ball, 30, 0, error) && !s.stageValue(SlotRef::boxed(0, 0), Field::Ball, 4, 0, error),
          "Hisui ball accepted, standard ball refused");
    check(s.heldItemChoices().size() == 1 && s.fieldState(Field::HeldItem, Modern::Region::Party) == State::ReadOnly &&
              !s.stageValue(SlotRef::party(0), Field::HeldItem, 1, 0, error),
          "held items are read-only: Pokemon cannot hold items in Legends: Arceus");
    check(s.stageValue(SlotRef::party(0), Field::Friendship, 160, 0, error) && s.view(SlotRef::party(0))->friendship == 160, "friendship");
    check(!s.stageValue(SlotRef::boxed(0, 0), Field::EffortLevel, 3, 0, error), "Effort Levels are read-only");
    check(!s.cloneSupport(SlotRef::party(0)).available && !s.removeSupport(SlotRef::party(0)).available, "party storage refused");
    check(s.clone(SlotRef::boxed(0, 1), SlotRef::boxed(3, 3), error) && s.view(SlotRef::boxed(3, 3))->species == 403, "clone");
    check(s.move(SlotRef::boxed(31, 29), SlotRef::boxed(4, 4), error) && s.view(SlotRef::boxed(31, 29))->empty, "move");
    check(!s.createSupport(SlotRef::boxed(9, 9)).available, "create blocked");
    check(s.beginDraft(SlotRef::boxed(0, 1), error) && s.stageValue(SlotRef::boxed(0, 1), Field::TrainerId, 1, 0, error) && s.draftDirty(), "draft");
    s.discardDraft();
    check(s.view(SlotRef::boxed(0, 1))->tid != 1, "discard draft");

    char pattern[] = "/tmp/gen8a-native-XXXXXX";
    const std::string root = ::mkdtemp(pattern) ? pattern : "";
    Modern::ExportRequest req;
    req.rootDirectory = root;
    req.timestamp = "20261031_230000";
    const auto result = s.exportWorkingCopy(req);
    check(result.success, "export: " + result.error);
    if (result.success) {
        const auto edited = slurp(result.directory + "/edited.bin");
        check(slurp(result.directory + "/original_backup.bin") == pristine, "backup pristine");
        auto re = G8a::Gen8aNativeSession::open(edited, "legends_arceus_switch");
        check(re.ready() && re.session->view(SlotRef::boxed(0, 0))->nickname == "Bidoof" && re.session->view(SlotRef::boxed(4, 4))->species == 493 &&
                  re.session->view(SlotRef::party(0))->friendship == 160 && !re.session->hasChanges(),
              "edited copy strictly reopens with the changes");
    }
    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    const auto built = n.staged().buildWorkingCopy(&error);
    check(built.has_value(), "working copy builds: " + error);
    if (built) {
        const auto changes = n.staged().changes();
        check(G8a::verifyWorkingCopy(n.staged().source(), *built, changes, &error), "verifier accepts");
        auto blocks = G8a::Gen8aSave::parse(*built)->blocks();
        for (auto& b : blocks) if (b.key == L::KeyMyStatus) b.data[L::MyTid] ^= 1;
        check(!G8a::verifyWorkingCopy(n.staged().source(), PokeVault::Integration::Gen8::Swish::encrypt(blocks), changes, &error),
              "foreign block change refused");
        auto forged = changes;
        for (auto& c : forged) if (c.kind == G8a::ChangeKind::Edit) PA8::write16(c.stagedPlain, 0x08, 25);
        check(!G8a::verifyWorkingCopy(n.staged().source(), *built, forged, &error), "a forged species change is refused");
        auto bad = *built;
        bad.back() ^= 0xFF;
        check(!G8a::verifyWorkingCopy(n.staged().source(), bad, changes, &error), "broken container hash refused");
    }
    check(std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin(), pristine.end()), "source unchanged");

    std::cout << "test_gen8a_native_session: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
