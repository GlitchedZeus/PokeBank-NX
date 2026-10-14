// Gen6ExactFormatEditorProvider (shared editor contract) and Gen VI save sources.
#include "Integration/Gen6/Gen6ExactFormatEditorProvider.h"
#include "Integration/Gen6/Gen6SaveSources.h"
#include "Integration/Gen6/Gen6StagedSession.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

#include <cstdlib>
#include <fstream>
#include <set>
#include <string>
#include <sys/stat.h>

namespace Gen6 = PokeVault::Integration::Gen6;
namespace Provider = PokeVault::Integration::Gen6EditorProvider;
namespace Sources = Gen6::Sources;
namespace Layout = Gen6::Layout;
using Provider::Field;
using Provider::RecordRegion;
using Provider::State;

namespace {

void testFieldTable() {
    // Exhaustive: every shared identity has a deliberate state in both regions.
    std::set<int> seen;
    for (const auto f : Provider::AllFields) seen.insert(static_cast<int>(f));
    GEN6_CHECK(seen.size() == Provider::AllFields.size());
    GEN6_CHECK(static_cast<int>(Field::FatefulEncounter) + 1 == static_cast<int>(Provider::AllFields.size()));

    const auto box = [](Field f) { return Provider::fieldState(f, RecordRegion::Box); };
    const auto party = [](Field f) { return Provider::fieldState(f, RecordRegion::Party); };
    for (const auto f : {Field::Nickname, Field::HeldItem, Field::Friendship, Field::Pokerus,
                         Field::OriginalTrainer, Field::OriginalTrainerGender, Field::TrainerId,
                         Field::SecretId, Field::Ball, Field::FatefulEncounter}) {
        GEN6_CHECK(box(f) == State::Editable);
        GEN6_CHECK(party(f) == State::Editable);
    }
    for (const auto f : {Field::IV, Field::EV, Field::Nature}) {
        GEN6_CHECK(box(f) == State::Editable);
        GEN6_CHECK(party(f) == State::ReadOnly);
    }
    GEN6_CHECK(box(Field::Shiny) == State::Derived && party(Field::Shiny) == State::Derived);
    GEN6_CHECK(box(Field::Level) == State::Derived && party(Field::Level) == State::ReadOnly);
    GEN6_CHECK(box(Field::CalculatedStats) == State::Hidden && party(Field::CalculatedStats) == State::ReadOnly);
    for (const auto f : {Field::Species, Field::Form, Field::Gender, Field::Language, Field::Experience,
                         Field::Ability, Field::PersonalityId, Field::OriginGame, Field::MetLevel,
                         Field::MetLocation, Field::MetDate, Field::Egg, Field::EggLocation, Field::EggDate,
                         Field::Move, Field::MovePP, Field::MovePPUps, Field::MoveCompatibility,
                         Field::EncounterLegality, Field::Provenance})
        GEN6_CHECK(box(f) == State::ReadOnly && party(f) == State::ReadOnly);
    for (const auto f : {Field::DV, Field::StatExperience, Field::AV, Field::EffortLevel, Field::StatNature})
        GEN6_CHECK(box(f) == State::Hidden && party(f) == State::Hidden);
}

void testDescriptor() {
    GEN6_CHECK(!Provider::descriptorForSource("black_nds", true));
    GEN6_CHECK(!Provider::descriptorForSource("sun_3ds", true));
    const auto xy = Provider::descriptorForSource("x_3ds", true);
    const auto oras = Provider::descriptorForSource("alphasapphire_3ds", false);
    GEN6_CHECK(xy && oras);
    if (!xy || !oras) return;
    GEN6_CHECK(xy->limits.maxHeldItem == 717 && oras->limits.maxHeldItem == 775);
    GEN6_CHECK(xy->limits.maxBall == 25 && xy->limits.maxEV == 252 && xy->limits.maxEVTotal == 510);
    GEN6_CHECK(xy->stats.battleStatCount == 6 && xy->stats.geneticModel ==
               PokeBank::UIModel::ExactFormatEditor::GeneticValueModel::IV);
    GEN6_CHECK(!xy->source.canWriteOriginalSource());
    GEN6_CHECK(xy->moves.evaluate({"x_3ds", 25, 0, 33, true}) ==
               PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult::Unsupported);
    using Cap = PokeVault::SaveEdit::Capability;
    GEN6_CHECK(xy->source.saveOperations.supports(Cap::PokemonEditing));
    GEN6_CHECK(xy->source.saveOperations.supports(Cap::PokemonClone));
    GEN6_CHECK(!xy->source.saveOperations.supports(Cap::PokemonCreation));
    // Editor targets need a validated staged workspace.
    GEN6_CHECK(xy->fieldIsEditorTarget(Field::IV, RecordRegion::Box));
    GEN6_CHECK(!xy->fieldIsEditorTarget(Field::IV, RecordRegion::Party));
    GEN6_CHECK(!oras->fieldIsEditorTarget(Field::Nickname, RecordRegion::Box));
    // Shared action menu: Remove only on boxed records; no Legality action.
    const auto boxActions = Provider::actionCapabilities(RecordRegion::Box);
    const auto partyActions = Provider::actionCapabilities(RecordRegion::Party);
    GEN6_CHECK(boxActions.canClone && boxActions.canRemove && !boxActions.hasLegalityProvenance);
    GEN6_CHECK(partyActions.canClone && !partyActions.canRemove);
}

// The provider table and the staged session agree for every field/region: a
// field is staged iff it is Editable (value 1 chosen to be valid where editable).
void testProviderSessionAgreement() {
    const auto bytes = Gen6Fixture::makeSave();
    for (const auto region : {RecordRegion::Box, RecordRegion::Party}) {
        for (const auto f : Provider::AllFields) {
            auto save = Gen6::Gen6Save::parse(bytes);
            Gen6::Gen6StagedSession s(std::move(*save));
            const auto slot = region == RecordRegion::Box ? Gen6::SlotRef::boxed(0, 0) : Gen6::SlotRef::party(0);
            std::string error;
            const bool text = f == Field::Nickname || f == Field::OriginalTrainer;
            const bool staged = text ? s.stageText(slot, f, "Abc", &error) : s.stageValue(slot, f, f == Field::HeldItem ? 2 : 1, 0, &error);
            GEN6_CHECK(staged == Provider::editable(f, region));
            if (staged && s.hasChanges()) {
                const auto copy = s.buildWorkingCopy(&error);
                GEN6_CHECK(copy.has_value());
            }
        }
    }
}

void testPathModel() {
    GEN6_CHECK(Sources::checkpointFolderPrefix("x_3ds") == "0x0055D ");
    GEN6_CHECK(Sources::checkpointFolderPrefix("y_3ds") == "0x0055E ");
    GEN6_CHECK(Sources::checkpointFolderPrefix("omegaruby_3ds") == "0x011C4 ");
    GEN6_CHECK(Sources::checkpointFolderPrefix("alphasapphire_3ds") == "0x011C5 ");
    GEN6_CHECK(Sources::checkpointFolderPrefix("black_nds").empty());
    GEN6_CHECK(Sources::gameIdFromCheckpointFolder("0x0055D Pokemon X") == "x_3ds");
    GEN6_CHECK(Sources::gameIdFromCheckpointFolder("0x011C5 Pokemon Alpha Sapphire") == "alphasapphire_3ds");
    GEN6_CHECK(Sources::gameIdFromCheckpointFolder("0x0055D").empty());     // needs " <title>"
    GEN6_CHECK(Sources::gameIdFromCheckpointFolder("Pokemon X").empty());   // names never identify
    GEN6_CHECK(Sources::gameIdFromCheckpointFolder("0x00164 Pokemon Sun").empty());

    GEN6_CHECK(*Sources::dekoponUserDir(std::nullopt) == "sdmc:/switch/dekopon/");
    GEN6_CHECK(*Sources::dekoponUserDir(std::string_view("")) == "sdmc:/switch/dekopon/");
    GEN6_CHECK(*Sources::dekoponUserDir(std::string_view(" sdmc:/switch/dekopon \r\n")) == "sdmc:/switch/dekopon/");
    GEN6_CHECK(*Sources::dekoponUserDir(std::string_view("\tsdmc:/emu/3ds\n")) == "sdmc:/emu/3ds/");
    GEN6_CHECK(!Sources::dekoponUserDir(std::string_view("ums0:/dekopon/")));

    GEN6_CHECK(Sources::emulatorSaveFile("sdmc:/switch/dekopon/", "x_3ds") ==
               "sdmc:/switch/dekopon/sdmc/Nintendo 3DS/00000000000000000000000000000000/"
               "00000000000000000000000000000000/title/00040000/00055d00/data/00000001/main");
    GEN6_CHECK(Sources::emulatorSaveFile("sdmc:/x", "omegaruby_3ds").find("/title/00040000/0011c400/data/00000001/main") !=
               std::string::npos);
    GEN6_CHECK(Sources::emulatorSaveFile("sdmc:/x", "sun_3ds").empty());

    GEN6_CHECK(Sources::descriptor(Sources::ProviderId::DekoponEmulator).status == Sources::ProviderStatus::Verified);
    GEN6_CHECK(Sources::descriptor(Sources::ProviderId::AzaharEmulator).status == Sources::ProviderStatus::Unsupported);
    GEN6_CHECK(Sources::descriptor(Sources::ProviderId::AzaharEmulator).switchRoot.empty());
    GEN6_CHECK(Sources::descriptor(Sources::ProviderId::JksmLayout).status == Sources::ProviderStatus::ManualLinkOnly);
    for (size_t i = 0; i < Sources::Providers.size(); ++i)
        GEN6_CHECK(static_cast<size_t>(Sources::Providers[i].id) == i && !Sources::Providers[i].reference.empty());
}

void testProbe() {
    const auto bytes = Gen6Fixture::makeSave({Layout::Family::XY, Layout::VersionY, 3, true});
    Sources::SourceContext ctx;
    ctx.provider = Sources::ProviderId::CheckpointLayout;
    ctx.assignedExactGame = "y_3ds";
    ctx.evidenceGameId = "y_3ds";
    ctx.sourcePath = "sdmc:/3ds/Checkpoint/saves/0x0055E Pokemon Y/2026/main";
    ctx.physicalIdentity = "fixture";
    auto p = Sources::probeSave(bytes, ctx);
    GEN6_CHECK(p.ready());
    GEN6_CHECK(p.instance.generation == 6 && p.instance.gameId == "y_3ds" && p.instance.partyCount == 3);
    GEN6_CHECK(p.instance.trainerName == "TESTER" && p.instance.readOnly());
    GEN6_CHECK(p.instance.providerId == "checkpoint-3ds");
    GEN6_CHECK(p.instance.kind == PokeVault::Source::SaveInstanceKind::Backup);

    ctx.assignedExactGame = "x_3ds";  // wrong explicit assignment
    p = Sources::probeSave(bytes, ctx);
    GEN6_CHECK(!p.ready() && p.instance.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch);
    ctx.assignedExactGame = "y_3ds";
    ctx.evidenceGameId = "x_3ds";     // folder title-ID disagrees with contents
    p = Sources::probeSave(bytes, ctx);
    GEN6_CHECK(p.instance.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch);
    ctx.evidenceGameId = {};
    ctx.assignedExactGame = {};       // no assignment
    p = Sources::probeSave(bytes, ctx);
    GEN6_CHECK(p.instance.validation == PokeVault::Source::ValidationStatus::Unsupported);
    ctx.assignedExactGame = "y_3ds";
    ctx.provider = Sources::ProviderId::AzaharEmulator;
    p = Sources::probeSave(bytes, ctx);
    GEN6_CHECK(p.instance.validation == PokeVault::Source::ValidationStatus::Unsupported);
    ctx.provider = Sources::ProviderId::ManualLink;
    p = Sources::probeSave(bytes, ctx);
    GEN6_CHECK(p.ready() && p.instance.kind == PokeVault::Source::SaveInstanceKind::ManualImport);
    auto corrupt = bytes;
    corrupt[0x100] ^= 1;
    p = Sources::probeSave(corrupt, ctx);
    GEN6_CHECK(p.instance.validation == PokeVault::Source::ValidationStatus::Invalid && !p.save);
    p = Sources::probeSave(std::vector<uint8_t>(Layout::ORASDemoSize, 0), ctx);
    GEN6_CHECK(p.instance.validation == PokeVault::Source::ValidationStatus::Unsupported);
}

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void testDiscovery() {
    char pattern[] = "/tmp/gen6-src-XXXXXX";
    const std::string root = ::mkdtemp(pattern);
    const auto mk = [](const std::string& p) { ::mkdir(p.c_str(), 0700); };
    // Checkpoint layout.
    const std::string cp = root + "/3ds/Checkpoint/saves";
    mk(root + "/3ds"); mk(root + "/3ds/Checkpoint"); mk(cp);
    mk(cp + "/0x0055D Pokemon X"); mk(cp + "/0x0055D Pokemon X/backup1");
    mk(cp + "/0x011C4 Pokemon Omega Ruby"); mk(cp + "/0x011C4 Pokemon Omega Ruby/b");
    mk(cp + "/0x00A5D Not Gen VI"); mk(cp + "/0x00A5D Not Gen VI/b");
    mk(cp + "/Pokemon Y"); mk(cp + "/Pokemon Y/b");
    const auto x = Gen6Fixture::makeSave();
    writeFile(cp + "/0x0055D Pokemon X/backup1/main", x);
    writeFile(cp + "/0x011C4 Pokemon Omega Ruby/b/main", x);  // XY bytes in an OR folder
    writeFile(cp + "/0x00A5D Not Gen VI/b/main", x);
    writeFile(cp + "/Pokemon Y/b/main", x);
    const auto found = Sources::findCheckpointExports(cp);
    GEN6_CHECK(found.size() == 2);
    for (const auto& f : found) {
        std::string error;
        const auto bytes = Sources::readCandidate(f.path, &error);
        GEN6_CHECK(bytes.has_value());
        Sources::SourceContext ctx{f.provider, "x_3ds", f.evidenceGameId, f.path, f.path, f.path, f.path, {}, 0};
        const auto probe = Sources::probeSave(*bytes, ctx);
        if (f.evidenceGameId == "x_3ds") GEN6_CHECK(probe.ready());
        else GEN6_CHECK(probe.instance.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch);
    }
    // Dekopon layout.
    const std::string user = root + "/switch/dekopon/";
    std::string dir = user + "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/00040000/00055e00/data/00000001";
    std::string partial;
    for (size_t i = 0; i < dir.size(); ++i) if (dir[i] == '/' && i > 0) mk(dir.substr(0, i));
    mk(dir);
    writeFile(dir + "/main", Gen6Fixture::makeSave({Layout::Family::XY, Layout::VersionY, 1, true}));
    const auto emu = Sources::findEmulatorSaves(Sources::ProviderId::DekoponEmulator, user);
    GEN6_CHECK(emu.size() == 1 && emu[0].evidenceGameId == "y_3ds");
    GEN6_CHECK(Sources::findEmulatorSaves(Sources::ProviderId::AzaharEmulator, user).empty());
    // readCandidate refuses wrong sizes and non-files.
    writeFile(root + "/odd.sav", std::vector<uint8_t>(0x80000, 0));
    std::string error;
    GEN6_CHECK(!Sources::readCandidate(root + "/odd.sav", &error));
    GEN6_CHECK(!Sources::readCandidate(cp, &error));
    [[maybe_unused]] const int removed = std::system(("rm -rf '" + root + "'").c_str());
}

} // namespace

int main() {
    testFieldTable();
    testDescriptor();
    testProviderSessionAgreement();
    testPathModel();
    testProbe();
    testDiscovery();
    return Gen6Test::finish("test_gen6_editor_sources");
}
