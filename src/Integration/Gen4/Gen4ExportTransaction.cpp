#include "Integration/Gen4/Gen4ExportTransaction.h"

#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Names/SpeciesNames.h"
#include "Names/ItemNames.h"
#include "Names/MoveNames.h"
#include "Names/MachineDisplay.h"
#include "Names/PokemonNames.h"
#include "Names/LocationDisplayNames.h"
#include "Enums/LanguageID.h"
#include "Enums/Ball.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Utils/StringHelpers.h"

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

    // R2-BUG-002: staged Trainer Name / Money are review changes too.
    if (before->trainer().name != after->trainer().name)
        lines.push_back("Trainer name: " + Utils::utf16ToUtf8(before->trainer().name) + " -> " +
                        Utils::utf16ToUtf8(after->trainer().name));
    if (before->trainer().money != after->trainer().money)
        lines.push_back("Money: " + std::to_string(before->trainer().money) + " -> " +
                        std::to_string(after->trainer().money));

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
    // A staged Move (stageMoveBoxPokemon, or a Clone plus a Release of its unchanged original) is
    // ONE Review line: the slot that became empty paired with an empty slot that now holds the
    // exact same native record. The staged bytes are unchanged; this only groups the two diffs.
    // Pairs are found first, then lines are emitted in slot order (at the vacated slot).
    constexpr size_t kSlots = kBoxCount * kBoxSlots;
    const auto slotName = [](size_t index) {
        return "Box " + std::to_string(index / kBoxSlots + 1) + " slot " +
               std::to_string(index % kBoxSlots + 1);
    };
    const auto oldAt = [&](size_t index) -> const auto& { return before->box(index / kBoxSlots, index % kBoxSlots); };
    const auto newAt = [&](size_t index) -> const auto& { return after->box(index / kBoxSlots, index % kBoxSlots); };
    std::vector<size_t> moveTo(kSlots, kSlots);       // vacated slot -> its destination
    std::vector<bool> isDestination(kSlots, false);
    for (size_t source = 0; source < kSlots; ++source) {
        if (describe(oldAt(source)) == kEmpty || describe(newAt(source)) != kEmpty) continue;
        for (size_t d = 0; d < kSlots; ++d) {
            if (d == source || isDestination[d] || describe(oldAt(d)) != kEmpty ||
                !sameRecord(newAt(d), oldAt(source))) continue;
            moveTo[source] = d;
            isDestination[d] = true;
            break;
        }
    }
    for (size_t index = 0; index < kSlots; ++index) {
        if (isDestination[index]) continue;
        const auto& oldMon = oldAt(index);
        const auto& newMon = newAt(index);
        if (sameRecord(oldMon, newMon)) continue;
        const std::string from = describe(oldMon);
        const std::string to = describe(newMon);
        if (from == kEmpty && to == kEmpty) continue;
        if (moveTo[index] < kSlots) {
            lines.push_back("Moved " + from + ": " + slotName(index) + " -> " + slotName(moveTo[index]));
            continue;
        }
        lines.push_back(slotName(index) + ": " + from + (from == to ? " (edited)" : " -> " + to));
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
                lines.push_back(std::string("Bag ") + bagPocketName(pocket) + ": " +
                                Names::machineDisplayLabel(before->rawFamily(),itemId,Names::getItemName(itemId)) + " " + std::to_string(counts.first) +
                                " -> " + std::to_string(counts.second));
            }
        }
    } else if (oldBag.has_value() != newBag.has_value()) {
        lines.push_back("Bag: native item data changed validity");
    }
    return lines;
}

std::vector<std::pair<std::string,std::string>> describeStagedReviewChanges(
    const Gen4StagedPokemonEditor& editor) {
    std::vector<std::pair<std::string,std::string>> rows;
    for(const auto& line:describeStagedSlotChanges(editor))rows.push_back({"Staged change",line});
    std::string error;
    const auto before=strictParse(editor.originalBytes(),editor,error);
    const auto after=strictParse(editor.stagedBytes(),editor,error);
    if(!before || !after)return {};
    const auto compare=[&](const auto& a,const auto& b,const std::string& location) {
        if(!a.valid() || a.empty() || !b.valid() || b.empty() || sameRecord(a,b))return;
        const auto emit=[&](const char* field,const std::string& from,const std::string& to) {
            if(from!=to)rows.push_back({location+" / "+field,from+" -> "+to});
        };
        emit("Species",Names::getSpeciesName(a.species()),Names::getSpeciesName(b.species()));
        emit("Nickname",Utils::utf16ToUtf8(a.nickname()),Utils::utf16ToUtf8(b.nickname()));
        emit("Original Trainer",Utils::utf16ToUtf8(a.originalTrainerName()),Utils::utf16ToUtf8(b.originalTrainerName()));
        emit("Trainer ID",std::to_string(a.tid()),std::to_string(b.tid()));
        emit("Secret ID",std::to_string(a.sid()),std::to_string(b.sid()));
        emit("Held Item",Names::getItemName(a.heldItem()),Names::getItemName(b.heldItem()));
        emit("Ability",Names::getAbilityName(a.ability()),Names::getAbilityName(b.ability()));
        const Pokemon::Pokemon4ReadOnlyView av(a),bv(b);
        emit("Nature",Names::getNatureName(av.nature()),Names::getNatureName(bv.nature()));
        emit("Level",std::to_string(av.level()),std::to_string(bv.level()));
        emit("Ball",Enums::getBallName(av.ball()),Enums::getBallName(bv.ball()));
        emit("Met Location",Names::getMetLocationDisplayName(a.originVersion(),av.metLocation()),
            Names::getMetLocationDisplayName(b.originVersion(),bv.metLocation()));
        emit("Language",Enums::getLanguageName(a.language()),Enums::getLanguageName(b.language()));
        emit("Friendship",std::to_string(a.friendship()),std::to_string(b.friendship()));
        emit("Experience",std::to_string(a.experience()),std::to_string(b.experience()));
        emit("PID",std::to_string(a.pid()),std::to_string(b.pid()));
        emit("Gender",av.genderSymbol(),bv.genderSymbol());
        emit("Shiny",av.isShiny(a.id32(),{})?"Yes":"No",bv.isShiny(b.id32(),{})?"Yes":"No");
        emit("Pokerus",std::to_string(a.pokerusState()),std::to_string(b.pokerusState()));
        emit("Met Level",std::to_string(a.metLevel()),std::to_string(b.metLevel()));
        const auto am=a.moves(),bm=b.moves();
        const auto ap=a.pp(),bp=b.pp(),au=a.ppUps(),bu=b.ppUps();
        for(size_t i=0;i<4;++i) {
            const auto label="Move "+std::to_string(i+1);
            emit(label.c_str(),am[i]?Names::getMoveName(am[i]):"Empty",bm[i]?Names::getMoveName(bm[i]):"Empty");
            emit((label+" PP").c_str(),std::to_string(ap[i]),std::to_string(bp[i]));
            emit((label+" PP Ups").c_str(),std::to_string(au[i]),std::to_string(bu[i]));
        }
        const auto ai=a.ivs(),bi=b.ivs(),ae=a.evs(),be=b.evs();
        constexpr const char* stats[]={"HP","Atk","Def","Spe","SpA","SpD"};
        for(size_t i=0;i<6;++i) {
            emit((std::string("IV ")+stats[i]).c_str(),std::to_string(ai[i]),std::to_string(bi[i]));
            emit((std::string("EV ")+stats[i]).c_str(),std::to_string(ae[i]),std::to_string(be[i]));
        }
    };
    const auto oldParty=before->nativePartySlots(),newParty=after->nativePartySlots();
    for(size_t i=0;i<std::min(oldParty.size(),newParty.size());++i)
        compare(oldParty[i],newParty[i],"Party "+std::to_string(i+1));
    for(size_t box=0;box<kBoxCount;++box)for(size_t slot=0;slot<kBoxSlots;++slot)
        compare(before->box(box,slot),after->box(box,slot),"Box "+std::to_string(box+1)+" / Slot "+std::to_string(slot+1));
    return rows;
}

Common::VerifiedExportResult publishVerifiedGen4Export(
    const Gen4StagedPokemonEditor& editor, const Gen4ExportRequest& request) {
    Common::VerifiedExportResult result;
    if (!editor.hasChanges()) {
        result.error = "No pending changes to export";
        return result;
    }
    // With an Open snapshot the backup is the source FILE verbatim (a .dsv keeps its footer),
    // re-read here and proven unchanged since Open; edited.sav is always the raw payload.
    std::vector<uint8_t> sourceFile;
    if (request.sourceSnapshot) {
        std::string staleError;
        if (!gen4SourceUnchangedSinceOpen(request.sourcePath, *request.sourceSnapshot,
                                          editor.originalBytes(), &staleError, &sourceFile)) {
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
    if (request.sourceSnapshot) {
        transaction.backupBytes = sourceFile;
        // A .dsv backup keeps its extension (original_backup.dsv); edited.sav stays raw, as in Gen V.
        if (request.sourceSnapshot->container == Gen4SourceContainer::Dsv) {
            transaction.fileExtension = ".dsv";
            transaction.editedFileExtension = ".sav";
        }
        transaction.sourceContainer = gen4SourceContainerLabel(request.sourceSnapshot->container);
        transaction.editedContainer = gen4SourceContainerLabel(Gen4SourceContainer::Raw);
        transaction.sourceSha256Scope = "0x80000_SAV4_PAYLOAD";
    }
    transaction.changeLines = describeStagedSlotChanges(editor);
    transaction.validator = [&editor](std::span<const uint8_t> bytes, std::string& error) {
        return strictParse(bytes, editor, error).has_value();
    };
    transaction.failurePoint = request.failurePoint;
    return Common::publishVerifiedStagedExport(transaction);
}

} // namespace PokeVault::Integration::Gen4
