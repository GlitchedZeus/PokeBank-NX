#include "Integration/Gen4/Gen4ExportTransaction.h"

#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Names/SpeciesNames.h"

#include <algorithm>
#include <cstddef>
#include <map>

namespace PokeVault::Integration::Gen4 {
namespace {

constexpr size_t kBoxCount = 18;
constexpr size_t kBoxSlots = 30;
constexpr const char* kEmpty = "(empty)";

std::string describe(const Pokemon::Pokemon4ReadOnly& pokemon) {
    if (!pokemon.valid() || pokemon.empty() || pokemon.species() == 0) return kEmpty;
    const char* name = Names::getSpeciesName(pokemon.species());
    return std::string(name ? name : "Species") + " #" + std::to_string(pokemon.species());
}

bool sameRecord(const Pokemon::Pokemon4ReadOnly& a, const Pokemon::Pokemon4ReadOnly& b) {
    const auto left = a.originalEncryptedBytes();
    const auto right = b.originalEncryptedBytes();
    return std::equal(left.begin(), left.end(), right.begin(), right.end());
}

std::optional<Gen4ReadOnlySave> strictParse(std::span<const uint8_t> bytes,
                                            const Gen4StagedPokemonEditor& editor,
                                            std::string& error) {
    auto parsed = Gen4ReadOnlySave::parse(bytes, editor.layout(), editor.exactGameId(), &error);
    if (!parsed) {
        if (error.empty()) error = "Gen IV save failed strict parse";
        return std::nullopt;
    }
    if (parsed->assignmentStatus() != AssignmentStatus::Match) {
        error = "Gen IV save no longer matches its exact game assignment";
        return std::nullopt;
    }
    if (parsed->recovered()) {
        error = "Gen IV save parsed as a recovered older copy";
        return std::nullopt;
    }
    return parsed;
}

const char* formatLabel(Layout layout) {
    switch (layout) {
        case Layout::DiamondPearl: return "PK4 / Diamond-Pearl SRAM (0x80000)";
        case Layout::Platinum: return "PK4 / Platinum SRAM (0x80000)";
        case Layout::HeartGoldSoulSilver: return "PK4 / HeartGold-SoulSilver SRAM (0x80000)";
    }
    return "PK4 SRAM";
}

} // namespace

std::vector<std::string> describeStagedSlotChanges(const Gen4StagedPokemonEditor& editor) {
    std::vector<std::string> lines;
    std::string error;
    const auto before = strictParse(editor.originalBytes(), editor, error);
    const auto after = strictParse(editor.stagedBytes(), editor, error);
    if (!before || !after) return lines;

    if (before->partyCount() != after->partyCount())
        lines.push_back("Party size: " + std::to_string(before->partyCount()) + " -> " +
                        std::to_string(after->partyCount()));
    const auto oldParty = before->nativePartySlots();
    const auto newParty = after->nativePartySlots();
    for (size_t i = 0; i < std::min(oldParty.size(), newParty.size()); ++i) {
        if (sameRecord(oldParty[i], newParty[i])) continue;
        const std::string from = describe(oldParty[i]);
        const std::string to = describe(newParty[i]);
        // Two different encodings of an empty slot (e.g. a Pokemon added and then released in
        // this session) are not a Pokemon change; never show "(empty) (edited)".
        if (from == kEmpty && to == kEmpty) continue;
        lines.push_back("Party slot " + std::to_string(i + 1) + ": " + from +
                        (from == to ? " (edited)" : " -> " + to));
    }
    for (size_t box = 0; box < kBoxCount; ++box) {
        for (size_t slot = 0; slot < kBoxSlots; ++slot) {
            const auto& oldMon = before->box(box, slot);
            const auto& newMon = after->box(box, slot);
            if (sameRecord(oldMon, newMon)) continue;
            const std::string from = describe(oldMon);
            const std::string to = describe(newMon);
            if (from == kEmpty && to == kEmpty) continue;
            lines.push_back("Box " + std::to_string(box + 1) + " slot " + std::to_string(slot + 1) +
                            ": " + from + (from == to ? " (edited)" : " -> " + to));
        }
    }
    // Bag edits are review changes too: a Bag-only export must never confirm an empty list.
    // Quantities are compared per native pocket and item id (all stacks of an item together).
    const auto oldBag = decodeReadOnlyBag(*before);
    const auto newBag = decodeReadOnlyBag(*after);
    if (oldBag && newBag) {
        for (size_t pocket = 0; pocket < BagPocketCount; ++pocket) {
            std::map<uint16_t, std::pair<uint32_t, uint32_t>> totals;
            for (const auto& item : (*oldBag)[pocket]) totals[item.itemId].first += item.count;
            for (const auto& item : (*newBag)[pocket]) totals[item.itemId].second += item.count;
            for (const auto& [itemId, counts] : totals) {
                if (counts.first == counts.second) continue;
                lines.push_back(std::string("Bag ") + bagPocketName(pocket) + ": item #" +
                                std::to_string(itemId) + " " + std::to_string(counts.first) +
                                " -> " + std::to_string(counts.second));
            }
        }
    } else if (oldBag.has_value() != newBag.has_value()) {
        lines.push_back("Bag: native item data changed validity");
    }
    return lines;
}

Common::VerifiedExportResult publishVerifiedGen4Export(
    const Gen4StagedPokemonEditor& editor, const Gen4ExportRequest& request) {
    Common::VerifiedExportResult result;
    if (!editor.hasChanges()) {
        result.error = "No pending changes to export";
        return result;
    }
    if (request.sourceSnapshot) {
        std::string staleError;
        if (!gen4SourceUnchangedSinceOpen(request.sourcePath, *request.sourceSnapshot,
                                          editor.originalBytes(), &staleError)) {
            result.error = staleError;
            return result;
        }
    }
    std::string finalizeError;
    const auto edited = editor.finalizedBytes(&finalizeError);
    if (edited.empty()) {
        result.error = finalizeError.empty() ? "Staged Gen IV finalization failed" : finalizeError;
        return result;
    }

    Common::VerifiedExportRequest transaction;
    transaction.rootDirectory = request.rootDirectory;
    transaction.directoryStem = request.directoryStem;
    transaction.generationLabel = "Generation IV";
    transaction.fileExtension = ".sav";
    transaction.sourcePath = request.sourcePath;
    transaction.gameId = std::string(editor.exactGameId());
    transaction.saveFormat = formatLabel(editor.layout());
    transaction.timestamp = request.timestamp;
    transaction.applicationSha = request.applicationSha;
    transaction.originalBytes = editor.originalBytes();
    transaction.editedBytes = edited;
    transaction.changeLines = describeStagedSlotChanges(editor);
    transaction.validator = [&editor](std::span<const uint8_t> bytes, std::string& error) {
        return strictParse(bytes, editor, error).has_value();
    };
    transaction.failurePoint = request.failurePoint;
    return Common::publishVerifiedStagedExport(transaction);
}

} // namespace PokeVault::Integration::Gen4
