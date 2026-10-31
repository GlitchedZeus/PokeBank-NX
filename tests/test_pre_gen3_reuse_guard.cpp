#include "Safety/SourceMutationPolicy.h"
#include "UI/SharedPokemonEditorContract.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace Safety = PokeVault::Safety;

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "required issue-71 architecture source must exist");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void rejectParallelGen3EditorShell(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    assert(fs::exists(root));

    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        const auto filename = entry.path().filename().string();
        assert(filename.find("Gen3PokemonEditor") == std::string::npos);

        const auto text = readFile(entry.path());
        assert(text.find("namespace UI::Gen3PokemonEditor") == std::string::npos);
        assert(text.find("namespace Gen3PokemonEditor") == std::string::npos);
    }
}

} // namespace

static_assert(!Shared::generationOwnsSeparateEditorUI(),
              "generations must not own separate Pokemon editor shells");
static_assert(Shared::futureGenerationsExtendSameFoundation(),
              "future generations must extend the accepted shared editor");

static_assert(
    Shared::fieldAccessForGeneration(Shared::Generation::Gen1,
                                     Shared::FieldIdentity::Shiny) ==
        Shared::FieldAccess::Derived,
    "Gen I shiny remains a derived field, not an independently stored edit field");
static_assert(
    Shared::fieldAccessForGeneration(Shared::Generation::Gen2,
                                     Shared::FieldIdentity::Shiny) ==
        Shared::FieldAccess::Derived,
    "Gen II shiny remains a derived field, not an independently stored edit field");

static_assert(!Safety::canPerform(Safety::SourceKind::InstalledGame,
                                  Safety::SourceMutation::Edit));
static_assert(!Safety::canPerform(Safety::SourceKind::InstalledGame,
                                  Safety::SourceMutation::CreatePokemon));
static_assert(!Safety::canPerform(Safety::SourceKind::InstalledGame,
                                  Safety::SourceMutation::DirectMove));
static_assert(!Safety::canPerform(Safety::SourceKind::InstalledGame,
                                  Safety::SourceMutation::SaveChanges));
static_assert(!Safety::canPerform(Safety::SourceKind::RetroArchLegacy,
                                  Safety::SourceMutation::Edit));
static_assert(!Safety::canPerform(Safety::SourceKind::RetroArchLegacy,
                                  Safety::SourceMutation::CreatePokemon));
static_assert(!Safety::canPerform(Safety::SourceKind::RetroArchLegacy,
                                  Safety::SourceMutation::DirectMove));
static_assert(!Safety::canPerform(Safety::SourceKind::RetroArchLegacy,
                                  Safety::SourceMutation::SaveChanges));

int main() {
    // Issue #71 freezes one shared Pokemon editor. Gen III may provide exact-format
    // adapters/providers later, but normal UI must not grow a parallel editor shell.
    rejectParallelGen3EditorShell("include/UI");
    rejectParallelGen3EditorShell("src/UI");

    const auto gen3 = readFile("include/Integration/Gen3/PKSMGen3Adapter.h");

    // The currently accepted Gen III product boundary stays PokeBank-owned and read-only.
    assert(gen3.find("read-only Generation III adapter") != std::string::npos);
    assert(gen3.find("exposes no PKSM-Core types") != std::string::npos);
    assert(gen3.find("class ReadOnlySave") != std::string::npos);

    // #71 is an architecture freeze, not permission to add a Gen III mutation/finalization API.
    assert(gen3.find("stageBoxPokemon") == std::string::npos);
    assert(gen3.find("finalizedBytes") == std::string::npos);

    std::cout
        << "Issue #71 guard: one shared editor, derived-field truth, read-only Gen III boundary, "
           "source write locks PASS\n";
    return 0;
}
