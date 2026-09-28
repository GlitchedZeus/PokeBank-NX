#pragma once

#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "UI/PokemonEditorExitGuard.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::Gen4SharedEditor {

namespace Gen4 = PokeVault::Integration::Gen4;

enum class Mode : uint8_t { None, View, Edit, Create };

struct Session {
    Mode mode = Mode::None;
    bool confirmExit = false;
    std::vector<std::byte> baselineEncrypted;
    std::optional<Pokemon::Pokemon4Mutable> working;

    bool begin(const Pokemon::Pokemon4ReadOnly& record,
               Enums::GameVersion group, Mode next, std::string& error) {
        error.clear();
        if (next != Mode::View && next != Mode::Edit) {
            error = "Generation IV first milestone supports View/Edit only";
            return false;
        }
        auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(
            record.originalEncryptedBytes(), group, &error);
        if (!editable) return false;
        baselineEncrypted.assign(
            record.originalEncryptedBytes().begin(),
            record.originalEncryptedBytes().end());
        working = std::move(*editable);
        mode = next;
        confirmExit = false;
        return true;
    }

    bool beginCreate(const Gen4::Gen4ReadOnlySave& save,
                     uint16_t species,
                     uint32_t pidSeed,
                     std::string& error) {
        error.clear();
        const auto& trainer = save.trainer();
        const uint8_t language = Enums::safeLanguageForGroup(
            save.rawFamily(), trainer.language);
        auto created = Pokemon::Pokemon4Mutable::createStored(
            species, save.rawFamily(),
            static_cast<uint8_t>(save.exactGameFromSave()),
            trainer.name, trainer.tid, trainer.sid, trainer.gender,
            language, 5, pidSeed, &error);
        if (!created) return false;
        baselineEncrypted = created->encryptedBytes();
        working = std::move(*created);
        mode = Mode::Create;
        confirmExit = false;
        return true;
    }

    bool editable() const noexcept {
        return (mode == Mode::Edit || mode == Mode::Create) && working.has_value();
    }

    bool dirty() const {
        return editable() && working->encryptedBytes() != baselineEncrypted;
    }

    bool cycleGender() noexcept {
        if (!editable()) return false;
        const uint8_t current = working->gender();
        if (current > 1) return false;
        return working->setGender(current == 0 ? 1 : 0);
    }

    bool cycleShiny() noexcept {
        return editable() && working->setShiny(!working->shiny());
    }

    bool cycleAbility() noexcept {
        if (!editable()) return false;
        const uint8_t current = working->abilitySlot();
        return working->setAbilitySlot(current == 0 ? 1 : 0);
    }

    bool keep(Gen4::Gen4StagedPokemonEditor& editor,
              std::size_t box, std::size_t slot, std::string& error) {
        if (mode != Mode::Edit || !working) {
            error = "Generation IV Keep is only available for an Edit draft";
            return false;
        }
        if (!editor.commitBoxPokemon(box, slot, *working, &error)) return false;
        close();
        return true;
    }

    bool keepParty(Gen4::Gen4StagedPokemonEditor& editor,
                   std::size_t slot, std::string& error) {
        if (mode != Mode::Edit || !working) {
            error = "Generation IV Keep is only available for an Edit draft";
            return false;
        }
        if (!editor.commitPartyPokemon(slot, *working, &error)) return false;
        close();
        return true;
    }

    bool add(Gen4::Gen4StagedPokemonEditor& editor,
             std::size_t box, std::size_t slot, std::string& error) {
        if (mode != Mode::Create || !working) {
            error = "Generation IV Add is only available for a Create draft";
            return false;
        }
        if (!editor.commitNewBoxPokemon(box, slot, *working, &error)) return false;
        close();
        return true;
    }

    bool back() noexcept {
        using Guard = PokeBank::UIModel::PokemonEditorExitGuard::SessionKind;
        const Guard kind = mode == Mode::Create ? Guard::Create
                         : mode == Mode::Edit ? Guard::Edit
                                              : Guard::View;
        if (PokeBank::UIModel::PokemonEditorExitGuard::requiresConfirmation(kind, dirty())) {
            confirmExit = true;
            return false;
        }
        close();
        return true;
    }

    void discardDraft() noexcept { close(); }
    void continueEditing() noexcept { confirmExit = false; }

    void close() noexcept {
        mode = Mode::None;
        confirmExit = false;
        baselineEncrypted.clear();
        working.reset();
    }
};

} // namespace PokeBank::UIModel::Gen4SharedEditor
