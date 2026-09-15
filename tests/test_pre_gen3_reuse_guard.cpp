#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "required pre-Gen-III architecture source must exist");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void rejectParallelGen3EditorShell(const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    assert(fs::exists(root));
    for (const auto& entry : fs::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        const auto name = entry.path().filename().string();
        assert(name.find("Gen3PokemonEditor") == std::string::npos);
        const auto text = readFile(entry.path());
        assert(text.find("namespace UI::Gen3PokemonEditor") == std::string::npos);
        assert(text.find("namespace Gen3PokemonEditor") == std::string::npos);
    }
}
} // namespace

int main() {
    // Issue #71 freezes one shared Pokémon editor. Generation III may provide adapters/providers,
    // but must not grow a parallel UI shell beside the accepted shared editor.
    rejectParallelGen3EditorShell("include/UI");
    rejectParallelGen3EditorShell("src/UI");

    const auto shared = readFile("include/UI/SharedPokemonEditorContract.h");
    const auto foundation = readFile("include/UI/PokemonEditorFoundationContract.h");
    const auto provider = readFile("include/UI/ExactFormatEditorProvider.h");
    const auto classicProvider = readFile("include/Integration/Classic/ClassicExactFormatEditorProviders.h");
    const auto gen3 = readFile("include/Integration/Gen3/PKSMGen3Adapter.h");
    const auto safety = readFile("include/Safety/SourceMutationPolicy.h");

    // Assert the real accepted shared contracts rather than brittle fictional namespace names.
    assert(shared.find("namespace PokeBank::UIModel::SharedPokemonEditor") != std::string::npos);
    assert(shared.find("enum class Surface") != std::string::npos);
    assert(shared.find("CreateDraft") != std::string::npos);
    assert(shared.find("enum class Panel") != std::string::npos);
    assert(shared.find("Details") != std::string::npos);
    assert(shared.find("Values") != std::string::npos);
    assert(shared.find("Moves") != std::string::npos);

    assert(foundation.find("namespace PokeBank::UIModel::PokemonEditorFoundation") != std::string::npos);
    assert(foundation.find("struct Capabilities") != std::string::npos);
    assert(foundation.find("liveRetroArchWriteEnabled") != std::string::npos);
    assert(foundation.find("liveInstalledGameWriteEnabled") != std::string::npos);
    assert(foundation.find("liveOtherEmulatorWriteEnabled") != std::string::npos);
    assert(foundation.find("partyEditEnabled") != std::string::npos);

    // P0 provider vocabulary remains PokeBank-owned, generation-neutral and serialization-free.
    assert(provider.find("enum class FieldState") != std::string::npos);
    assert(provider.find("enum class StorageSemantics") != std::string::npos);
    assert(provider.find("struct StatPresentationSchema") != std::string::npos);
    assert(provider.find("struct MoveCompatibilityQuery") != std::string::npos);
    assert(provider.find("MoveCompatibilityEvaluator") != std::string::npos);
    assert(provider.find("struct TrainerDescriptor") != std::string::npos);
    assert(provider.find("struct SourceCapabilityBridge") != std::string::npos);
    assert(provider.find("descriptorForAcceptedClassicSource") != std::string::npos);
    assert(provider.find("finalizedBytes") == std::string::npos);
    assert(provider.find("PKSMGen3") == std::string::npos);

    // Generation-specific move truth is adapted behind the provider instead of moved into shared UI.
    assert(classicProvider.find("Gen1::MoveCompatibility::canLearnMove") != std::string::npos);
    assert(classicProvider.find("Gen2::MoveCompatibility::canLearnMove") != std::string::npos);
    assert(classicProvider.find("PreserveExisting") != std::string::npos);

    // The current Gen III boundary is deliberately read-only and PokeBank-owned; do not make
    // PKSM-Core concrete objects or write APIs the UI architecture during the preflight.
    assert(gen3.find("PokeBank NX read-only Generation III adapter") != std::string::npos);
    assert(gen3.find("exposes no PKSM-Core types") != std::string::npos);
    assert(gen3.find("class ReadOnlySave") != std::string::npos);
    assert(gen3.find("stageBoxPokemon") == std::string::npos);
    assert(gen3.find("finalizedBytes") == std::string::npos);

    // The pre-Gen-III architecture gate cannot weaken the permanent source-mutation default.
    assert(safety.find("return false") != std::string::npos);

    std::cout << "Pre-Gen-III reuse guard: one shared editor, P0 provider, read-only Gen III boundary, source safety PASS\n";
    return 0;
}
