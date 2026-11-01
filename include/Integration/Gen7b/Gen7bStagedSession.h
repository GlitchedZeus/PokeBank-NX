#ifndef POKEBANK_GEN7B_STAGED_SESSION_H
#define POKEBANK_GEN7B_STAGED_SESSION_H

// Staged Let's Go storage changes on an app-owned copy of a strictly parsed SAV7b.
//
// The model is an operation log replayed over the immutable source storage (replay() is a pure
// function), because Let's Go storage is a gapless list: releasing a Pokemon shifts every later
// record down by one and remaps the party / partner pointers (PKSM-Core SavLGPE::compressBox; PKHeX
// SAV7b CompressStorage), and a clone can only be appended at the single next free index.
//
//   Edit    one record's provider-whitelisted fields (no derived companion: CP, stats unchanged)
//   Clone   append a byte-identical copy of an occupied record at index `count`
//   Remove  release a record that is neither a party member nor the partner; compact + remap
//   Move    not offered: positions in a gapless list are assigned by the game
//
// Export rewrites ONLY the PokeListHeader block (8) and storage block (9) and their CRC-16/ARC
// words; verifyWorkingCopy() proves the result independently.

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen7b/Gen7bSave.h"
#include "UI/ExactFormatEditorProvider.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen7b {

using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using FieldState = PokeBank::UIModel::ExactFormatEditor::FieldState;

// The provider table: the shared editor and the verifier both derive from it.
[[nodiscard]] constexpr FieldState fieldState(Field field) noexcept {
    switch (field) {
        case Field::Nickname:
        case Field::OriginalTrainer:
        case Field::OriginalTrainerGender:
        case Field::TrainerId:
        case Field::SecretId:
        case Field::Ball:
        case Field::FatefulEncounter:
            return FieldState::Editable;
        case Field::Shiny:
        case Field::CalculatedStats:
            return FieldState::Derived;
        // Friendship and Awakening Values feed the cached Combat Power (PB7::CP) and AVs feed the
        // cached stats; they stay read-only until that recomputation is verified. Held items are
        // stored but unused by Let's Go.
        case Field::Friendship:
        case Field::AV:
        case Field::HeldItem:
        case Field::IV:
        case Field::EV:
        case Field::Nature:
        case Field::StatNature:
        case Field::Level:
        case Field::Species:
        case Field::Form:
        case Field::Gender:
        case Field::Language:
        case Field::Experience:
        case Field::Ability:
        case Field::PersonalityId:
        case Field::OriginGame:
        case Field::MetLevel:
        case Field::MetLocation:
        case Field::MetDate:
        case Field::Egg:
        case Field::EggLocation:
        case Field::EggDate:
        case Field::Move:
        case Field::MovePP:
        case Field::MovePPUps:
        case Field::MoveCompatibility:
        case Field::EncounterLegality:
        case Field::Provenance:
        // PB7.cs HyperTrainFlags 0xDE: shown, not edited (cached stats and CP would need recomputing).
        case Field::HyperTraining:
            return FieldState::ReadOnly;
        case Field::Pokerus:          // no Pokerus in Let's Go
        case Field::DV:
        case Field::StatExperience:
        case Field::EffortLevel:
            return FieldState::Hidden;
    }
    return FieldState::Hidden;
}
[[nodiscard]] constexpr bool editable(Field f) noexcept { return fieldState(f) == FieldState::Editable; }

// Let's Go balls: Poke, Great, Ultra, Master and Premier (Enums::getBallList(GG)).
inline constexpr std::array<uint8_t, 5> Balls = {1, 2, 3, 4, 12};
[[nodiscard]] constexpr bool validBall(uint32_t b) noexcept {
    for (const auto x : Balls) if (x == b) return true;
    return false;
}

enum class OpKind : uint8_t { Edit, Clone, Remove };

struct Op {
    OpKind kind = OpKind::Edit;
    size_t index = 0;                      // Edit/Remove target, Clone donor (as of this step)
    std::vector<uint8_t> plain;            // Edit: the full staged plain record (checksum refreshed)
    std::string note;                      // review / manifest line
};

struct Gen7bExportRequest {
    std::string rootDirectory;
    std::string sourcePath;
    std::string timestamp;
    std::string applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

class Gen7bStagedSession {
public:
    explicit Gen7bStagedSession(Gen7bSave source) : source_(std::move(source)), current_(source_.model()) {}

    [[nodiscard]] const Gen7bSave& source() const noexcept { return source_; }
    [[nodiscard]] const StorageModel& current() const noexcept { return current_; }
    [[nodiscard]] const std::vector<Op>& ops() const noexcept { return ops_; }
    [[nodiscard]] std::optional<PK6::Pokemon6> record(size_t index) const;

    // Why a storage operation is unavailable at `index` (empty = available).
    [[nodiscard]] std::string_view cloneBlocked(size_t from) const noexcept;
    [[nodiscard]] std::string_view removeBlocked(size_t at) const noexcept;

    [[nodiscard]] bool stageValue(size_t index, Field field, uint32_t value, std::string* error = nullptr);
    [[nodiscard]] bool stageText(size_t index, Field field, std::string_view text, std::string* error = nullptr);
    [[nodiscard]] bool cloneAppend(size_t from, std::string* error = nullptr);
    [[nodiscard]] bool remove(size_t at, std::string* error = nullptr);

    // Undo the most recent operation (only when it touched `index`), the op-log analogue of Undo.
    [[nodiscard]] bool undoLastAt(size_t index);
    [[nodiscard]] size_t mark() const noexcept { return ops_.size(); }
    void truncate(size_t mark);
    void clear() { truncate(0); }

    [[nodiscard]] bool hasChanges() const noexcept { return !(current_ == source_.model()); }
    [[nodiscard]] bool changedAt(size_t index) const noexcept;
    [[nodiscard]] std::vector<std::string> changeLines() const;

    [[nodiscard]] std::optional<std::vector<uint8_t>> buildWorkingCopy(std::string* error = nullptr) const;
    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen7bExportRequest& request) const;

    // Pure replay of an op log over a model. Empty optional (with error) when an op is invalid.
    [[nodiscard]] static std::optional<StorageModel> replay(const StorageModel& base, const std::vector<Op>& ops,
                                                            std::string* error = nullptr);

private:
    [[nodiscard]] bool apply(Op op, std::string* error);

    Gen7bSave source_;
    StorageModel current_;
    std::vector<Op> ops_;
};

// Independent verifier: `edited` must be the same size, pass the block-CRC validation and the strict
// storage model checks, differ from the source only inside blocks 8/9 and their checksum words,
// equal replay(source, ops) exactly, keep every Edit inside the provider whitelist, and keep the
// party members and the partner as the same Pokemon (EC/PID) after any compaction.
[[nodiscard]] bool verifyWorkingCopy(const Gen7bSave& source, std::span<const uint8_t> edited,
                                     const std::vector<Op>& ops, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen7b

#endif
