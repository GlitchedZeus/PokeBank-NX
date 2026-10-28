// Top Up / Add Missing (owner Oct 28, Feature B): opt-in evidence on REAL saves.
//
//   POKEBANK_REAL_SAVES=<folder> ./build-host/test_bag_top_up_real_saves
//
// Every Gen III / IV / V save under <folder> whose file name names its game (Ruby, Sapphire, Emerald,
// FireRed, LeafGreen, Diamond, Pearl, Platinum, HeartGold, SoulSilver, Black, White, Black2, White2)
// is opened read-only into its staged editor; Top Up / Add Missing (both modes, every Bag pocket,
// 999 asked) is planned and applied; the finalized / export candidate is strictly reparsed and every
// planned count read back; the file bytes are compared before and after. Gen I / II Japanese saves
// stay read-only (Integration/Common/JapaneseEditingGate.h) and are listed, not edited.
// Saves are read in place and never copied into the repository. Without the variable this prints
// NOT TESTED and passes; a save that cannot be opened prints NO FIXTURE / NOT TESTED with the reason.
#include "Inventory/BulkItemAdaptersClassic.h"
#include "Inventory/BulkItemAdaptersNds.h"
#include "Integration/Common/JapaneseEditingGate.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "UI/BagTopUpModel.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace B = PokeVault::Inventory::BulkItems;
namespace Model = PokeBank::UIModel::BagTopUp;
namespace fs = std::filesystem;

namespace {
int failures = 0;
int tested = 0;
void fail(const std::string& what) { std::cerr << "FAILED: " << what << '\n'; ++failures; }

std::vector<uint8_t> readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
bool has(const std::string& name, const char* token) { return name.find(token) != std::string::npos; }

B::Request everything(std::span<const B::PocketState> state) {
    Model::State s;
    s.allPockets = true;
    s.quantity = 999;
    return Model::request(s, B::Pocket::Items, B::bagPockets(state));
}
template <class Editor, class Adapter>
bool readsBack(const Editor& e, const B::Plan& plan, const Adapter& a) {
    for (const auto& t : plan.topUps) if (a.quantity(e, t.pocket, t.itemId, t.slot) != t.to) return false;
    for (const auto& x : plan.adds) if (a.quantity(e, x.pocket, x.itemId, SIZE_MAX) != x.quantity) return false;
    return true;
}
void report(const std::string& file, const B::Plan& plan) {
    std::cout << "  TESTED " << file << ": " << Model::totals(plan) << ", strict reparse + read-back OK, source unchanged\n";
    ++tested;
}

void gen3(const fs::path& path, const std::string& name, PokeVault::Integration::Gen3::SourceGame game) {
    namespace G3 = PokeVault::Integration::Gen3;
    const auto bytes = readFile(path);
    std::string err;
    auto editor = G3::StagedInventoryEditor::create(bytes, game, err);
    if (!editor) { std::cout << "  NOT TESTED " << name << ": " << err << '\n'; return; }
    const auto state = B::Gen3Bag::pockets(*editor, err);
    const auto plan = B::plan(B::Gen3Bag::game(*editor), everything(state), state);
    if (!B::apply(*editor, plan, B::Gen3Bag::Adapter{}).applied && !plan.empty()) { fail(name + ": apply refused"); return; }
    const auto out = editor->finalizedBytes(err);
    auto again = out.empty() ? nullptr : G3::StagedInventoryEditor::create(out, game, err);
    if (!plan.empty() && (!again || !readsBack(*again, plan, B::Gen3Bag::Adapter{}))) { fail(name + ": reparse / read-back: " + err); return; }
    if (readFile(path) != bytes) { fail(name + ": source file changed"); return; }
    report(name, plan);
}

void gen4(const fs::path& path, const std::string& name, PokeVault::Integration::Gen4::Layout layout, const char* id) {
    namespace G4 = PokeVault::Integration::Gen4;
    const auto bytes = readFile(path);
    std::string err;
    auto editor = G4::Gen4StagedPokemonEditor::create(bytes, layout, id, &err);
    const auto game = editor ? B::Gen4Bag::game(*editor) : std::nullopt;
    if (!editor || !game) { std::cout << "  NOT TESTED " << name << ": " << err << '\n'; return; }
    const auto state = B::Gen4Bag::pockets(*editor, err);
    if (state.empty()) { std::cout << "  NOT TESTED " << name << ": " << err << '\n'; return; }
    const auto plan = B::plan(*game, everything(state), state);
    if (!B::apply(*editor, plan, B::Gen4Bag::Adapter{}).applied && !plan.empty()) { fail(name + ": apply refused"); return; }
    const auto out = editor->finalizedBytes(&err);
    auto again = out.empty() ? std::nullopt : G4::Gen4StagedPokemonEditor::create(out, layout, id, &err);
    if (!plan.empty() && (!again || !readsBack(*again, plan, B::Gen4Bag::Adapter{}))) { fail(name + ": reparse / read-back: " + err); return; }
    if (readFile(path) != bytes) { fail(name + ": source file changed"); return; }
    report(name, plan);
}

void gen5(const fs::path& path, const std::string& name, const char* id) {
    namespace G5 = PokeVault::Integration::Gen5;
    const auto bytes = readFile(path);
    std::string err;
    const auto parsed = G5::Gen5ReadOnlySave::parse(bytes, id, &err, G5::SaveCopySelection::Primary);
    if (!parsed) { std::cout << "  NOT TESTED " << name << ": " << err << '\n'; return; }
    G5::Gen5StagedPokemonWorkspace ws(*parsed);
    auto* bag = ws.stagedBag(&err);
    const auto game = B::Gen5Bag::game(id);
    if (!bag || !game) { std::cout << "  NOT TESTED " << name << ": " << err << '\n'; return; }
    const auto state = B::Gen5Bag::pockets(*bag);
    const auto plan = B::plan(*game, everything(state), state);
    if (!B::apply(*bag, plan, B::Gen5Bag::Adapter{}).applied && !plan.empty()) { fail(name + ": apply refused"); return; }
    if (!plan.empty()) {
        const auto candidate = G5::buildStagedSaveCandidate(ws, &err);
        const auto reparsed = candidate ? G5::Gen5ReadOnlySave::parse(candidate->bytes, id, &err, G5::SaveCopySelection::Primary)
                                        : std::nullopt;
        auto check = reparsed ? G5::Gen5StagedBag::create(*reparsed, &err) : std::nullopt;
        if (!check || !readsBack(*check, plan, B::Gen5Bag::Adapter{})) { fail(name + ": candidate reparse / read-back: " + err); return; }
    }
    if (readFile(path) != bytes) { fail(name + ": source file changed"); return; }
    report(name, plan);
}
} // namespace

int main() {
    const char* root = std::getenv("POKEBANK_REAL_SAVES");
    if (!root || !*root) {
        std::cout << "Top Up / Add Missing on real saves: NOT TESTED (set POKEBANK_REAL_SAVES=<folder>)\n";
        return 0;
    }
    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        std::cout << "Top Up / Add Missing on real saves: NO FIXTURE (" << root << " is not a folder)\n";
        return 0;
    }
    namespace G3 = PokeVault::Integration::Gen3;
    namespace G4 = PokeVault::Integration::Gen4;
    std::vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(root, ec))
        if (entry.is_regular_file() && entry.path().extension() == ".sav") files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    for (const auto& path : files) {
        const std::string name = path.filename().string();
        if (has(name, "_G1_") || has(name, "_G2_")) {
            std::cout << "  NOT EDITED " << name << ": Japanese Gen I / II saves stay read-only (JapaneseEditingGate "
                      << (PokeVault::Integration::Common::kJapaneseClassicEditingEnabled ? "open" : "closed") << ")\n";
            continue;
        }
        if (has(name, "LeafGreen")) gen3(path, name, G3::SourceGame::LeafGreenGBA);
        else if (has(name, "FireRed")) gen3(path, name, G3::SourceGame::FireRedGBA);
        else if (has(name, "Emerald")) gen3(path, name, G3::SourceGame::EmeraldGBA);
        else if (has(name, "Sapphire")) gen3(path, name, G3::SourceGame::SapphireGBA);
        else if (has(name, "Ruby")) gen3(path, name, G3::SourceGame::RubyGBA);
        else if (has(name, "Diamond")) gen4(path, name, G4::Layout::DiamondPearl, "diamond_nds");
        else if (has(name, "Pearl")) gen4(path, name, G4::Layout::DiamondPearl, "pearl_nds");
        else if (has(name, "Platinum")) gen4(path, name, G4::Layout::Platinum, "platinum_nds");
        else if (has(name, "HeartGold")) gen4(path, name, G4::Layout::HeartGoldSoulSilver, "heartgold_nds");
        else if (has(name, "SoulSilver")) gen4(path, name, G4::Layout::HeartGoldSoulSilver, "soulsilver_nds");
        else if (has(name, "Black2")) gen5(path, name, "black2_nds");
        else if (has(name, "White2")) gen5(path, name, "white2_nds");
        else if (has(name, "Black")) gen5(path, name, "black_nds");
        else if (has(name, "White")) gen5(path, name, "white_nds");
        else std::cout << "  NO FIXTURE " << name << ": game not named in the file name\n";
    }
    if (failures) {
        std::cerr << failures << " real-save Top Up / Add Missing check(s) failed\n";
        return 1;
    }
    std::cout << "Top Up / Add Missing on real saves: " << tested << " save(s) TESTED: PASS\n";
    return 0;
}
