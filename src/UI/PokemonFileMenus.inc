// Pokemon Actions -> Import from file / Export to file for every Gen I-V session (owner Oct 28,
// Feature A UI). Compiled once, last, by TrainerViewScreenCompositeOverlay.cpp so it can reach each
// generation's staged editor and presentation refresh; see UI/PokemonFileMenus.h for the rules.
#include "UI/PokemonFileMenus.h"
#include "UI/PokemonFileDialog.h"
#include "UI/PokemonFileIo.h"
#include "Integration/Gen5/Gen5PokemonFile.h"

namespace UI::PokemonFileMenus {
namespace {

namespace PF = PokeBank::PokemonFile;
namespace DialogModel = PokeBank::UIModel::PokemonFileDialog;
namespace Io = PokeBank::UIModel::PokemonFileIo;

std::string speciesName(uint16_t species) {
    const char* name = Names::getSpeciesName(species);
    return name && *name ? std::string(name) : "Pokemon";
}

// ---- Import: one staging callback per generation -------------------------------------------------

PokemonFileDialog::StageFn gen1Import(TrainerViewScreen& screen, int box) {
    return [&screen, box](const PF::Decoded& file, std::string& status, std::string& error) {
        auto* e = Gen1PokemonEditor::realEditor(screen);
        if (!e) { error = "Generation I staged editing is unavailable for this save"; return false; }
        // Boxes are packed lists: the record goes to the box's first empty slot.
        const auto slot = e->appendSlot(static_cast<size_t>(box), error);
        if (!slot) return false;
        if (!e->stageImport(static_cast<size_t>(box), *slot, file, error)) return false;
        Gen1PokemonEditor::refreshBoxes(screen);
        std::string ignored;
        const auto record = e->boxedPokemon(static_cast<size_t>(box), *slot, ignored);
        status = DialogModel::importedStatus(record ? speciesName(record->species) : "Pokemon", box,
                                             static_cast<int>(*slot));
        screen.selectedBoxIndex = box;
        screen.selectedItemIndex = static_cast<int>(*slot);
        return true;
    };
}

PokemonFileDialog::StageFn gen2Import(TrainerViewScreen& screen, int box) {
    return [&screen, box](const PF::Decoded& file, std::string& status, std::string& error) {
        auto* editor = stagedEditor(screen);
        if (!editor) { error = "Generation II staged editing is unavailable for this save"; return false; }
        // Boxes are packed lists: the record goes to the box's first empty slot.
        std::size_t slot = 0;
        for (; slot < editor->metadata().boxCapacity; ++slot) {
            std::string ignored;
            if (!editor->boxedPokemon(static_cast<std::size_t>(box), slot, ignored)) break;
        }
        if (!editor->stageImportBoxPokemon(static_cast<std::size_t>(box), slot, file, error)) return false;
        std::string refresh;
        if (!gscTrainer(screen).refreshStagedBoxPresentation(refresh))
            screen.postStatus(refresh.empty() ? "Import staged, but the box could not be redrawn" : refresh, 360);
        std::string ignored;
        const auto record = editor->boxedPokemon(static_cast<std::size_t>(box), slot, ignored);
        status = DialogModel::importedStatus(record ? speciesName(record->species) : "Pokemon", box,
                                             static_cast<int>(slot));
        screen.selectedBoxIndex = box;
        screen.selectedItemIndex = static_cast<int>(slot);
        return true;
    };
}

PokemonFileDialog::StageFn gen3Import(TrainerViewScreen& screen, int box, int slot) {
    return [&screen, box, slot](const PF::Decoded& file, std::string& status, std::string& error) {
        auto* editor = Gen3SharedEditorSurface::editor(screen);
        if (!editor) { error = "Generation III staged editing is unavailable for this save"; return false; }
        if (!editor->stageImportBoxPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), file, error))
            return false;
        Gen3SharedEditorSurface::refreshPresentation(screen);
        std::string ignored;
        const auto record = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), ignored);
        status = DialogModel::importedStatus(record ? speciesName(record->species) : "Pokemon", box, slot);
        return true;
    };
}

PokemonFileDialog::StageFn gen4Import(TrainerViewScreen& screen, int box, int slot) {
    return [&screen, box, slot](const PF::Decoded& file, std::string& status, std::string& error) {
        auto* editor = Gen4SharedEditorSurface::editor(screen);
        if (!editor) { error = "Generation IV staged editing is unavailable for this save"; return false; }
        if (!editor->canImportPokemonFile(file, &error)) return false;
        if (!editor->stageImportBoxPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), file, &error))
            return false;
        (void)Gen4SharedEditorSurface::refreshPresentation(screen);
        std::string ignored;
        const auto record = editor->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), &ignored);
        status = DialogModel::importedStatus(record ? speciesName(record->species()) : "Pokemon", box, slot);
        return true;
    };
}

PokemonFileDialog::StageFn gen5Import(TrainerViewScreen& screen, int box, int slot) {
    return [&screen, box, slot](const PF::Decoded& file, std::string& status, std::string& error) {
        if (!Gen5SharedReadOnlySurface::isGen5Source(screen)) {
            error = "Generation V staged editing is unavailable for this save";
            return false;
        }
        auto& trainer = Gen5SharedReadOnlySurface::nativeTrainer(screen);
        auto& workspace = trainer.stagedPokemon();
        if (!PokeVault::Integration::Gen5::importPokemonFile5(workspace, file, static_cast<size_t>(box),
                                                             static_cast<size_t>(slot), &error))
            return false;
        std::string refresh;
        if (!trainer.refreshStagedPokemonPresentation(refresh))
            screen.postStatus(refresh.empty() ? "Import staged, but the box could not be redrawn" : refresh, 360);
        const auto record = workspace.viewBox(static_cast<size_t>(box), static_cast<size_t>(slot));
        status = DialogModel::importedStatus(record && record->valid() && !record->empty()
                                                 ? speciesName(record->species()) : "Pokemon", box, slot);
        return true;
    };
}

// ---- Export: the record as the open (staged) workspace holds it ----------------------------------

struct Exported {
    std::optional<std::vector<uint8_t>> bytes;
    uint16_t species = 0;
};

Exported exportBytes(TrainerViewScreen& screen, int generation, bool party, int box, int slot, std::string& error) {
    Exported out;
    const auto b = static_cast<std::size_t>(std::max(0, box));
    const auto s = static_cast<std::size_t>(std::max(0, slot));
    switch (generation) {
        case 1: {
            const auto* e = Gen1PokemonEditor::realEditor(screen);
            if (!e) break;
            const auto record = party ? e->partyPokemon(s, error) : e->boxedPokemon(b, s, error);
            if (!record) return out;
            out.species = record->species;
            out.bytes = party ? e->exportPartyPokemonFile(s, error) : e->exportBoxPokemonFile(b, s, error);
            return out;
        }
        case 2: {
            const auto* e = stagedEditor(screen);
            if (!e) break;
            const auto record = party ? e->partyPokemon(s, error) : e->boxedPokemon(b, s, error);
            if (!record) return out;
            out.species = record->species;
            out.bytes = party ? e->exportPartyPokemonFile(s, error) : e->exportBoxPokemonFile(b, s, error);
            return out;
        }
        case 3: {
            const auto* e = Gen3SharedEditorSurface::editor(screen);
            if (!e) break;
            const auto record = party ? e->partyPokemon(s, error) : e->boxedPokemon(b, s, error);
            if (!record) return out;
            out.species = record->species;
            out.bytes = party ? e->exportPartyPokemonFile(s, error) : e->exportBoxPokemonFile(b, s, error);
            return out;
        }
        case 4: {
            const auto* e = Gen4SharedEditorSurface::editor(screen);
            if (!e) break;
            const auto record = party ? e->partyPokemon(s, &error) : e->boxedPokemon(b, s, &error);
            if (!record) return out;
            out.species = record->species();
            out.bytes = party ? e->exportPartyPokemonFile(s, &error) : e->exportBoxPokemonFile(b, s, &error);
            return out;
        }
        case 5: {
            if (!Gen5SharedReadOnlySurface::isGen5Source(screen)) break;
            const auto& workspace = Gen5SharedReadOnlySurface::nativeTrainer(screen).stagedPokemon();
            using Region = PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace::Region;
            const PokeVault::Integration::Gen5::Gen5StagedPokemonWorkspace::Slot where{
                party ? Region::Party : Region::Box, party ? 0 : b, s};
            out.bytes = PokeVault::Integration::Gen5::exportStagedPokemonFile5(workspace, where, &error);
            const auto record = party ? workspace.viewParty(s) : workspace.viewBox(b, s);
            if (out.bytes && record) out.species = record->species();
            return out;
        }
        default: break;
    }
    error = "This save has no staged editor to export from";
    return out;
}

} // namespace

bool beginImport(TrainerViewScreen& screen, int generation, int box, int slot) {
    PokemonFileDialog::StageFn stage;
    switch (generation) {
        case 1: stage = gen1Import(screen, box); break;
        case 2: stage = gen2Import(screen, box); break;
        case 3: stage = gen3Import(screen, box, slot); break;
        case 4: stage = gen4Import(screen, box, slot); break;
        case 5: stage = gen5Import(screen, box, slot); break;
        default: return false;
    }
    PokemonFileDialog::openImport(screen, generation, std::move(stage));
    return true;
}

bool exportPokemon(TrainerViewScreen& screen, int generation, bool party, int box, int slot) {
    std::string error;
    const auto exported = exportBytes(screen, generation, party, box, slot, error);
    if (!exported.bytes) {
        screen.postStatus(error.empty() ? "This Pokemon could not be exported" : error, 420);
        return false;
    }
    const auto result = Io::exportPokemonFile(generation, speciesName(exported.species), *exported.bytes);
    if (!result.ok) {
        screen.postStatus(result.error, 480);
        return false;
    }
    screen.postStatus(DialogModel::exportedStatus(result.path), 600);
    return true;
}

} // namespace UI::PokemonFileMenus
