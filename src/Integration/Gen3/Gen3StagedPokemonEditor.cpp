#include "Integration/Gen3/Gen3StagedPokemonEditor.h"

#include "Encryption/Encryption3FRLG.h"
#include "Enums/GameVersion.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Names/MoveInfo.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <sstream>

namespace PokeVault::Integration::Gen3 {
namespace {

constexpr std::size_t kBoxRecordSize = 80;
constexpr uint8_t kBoxFirstSector = 5;
constexpr std::size_t kBoxPrefixBytes = 4;
constexpr uint16_t kMaxGen3Move = 354;

std::size_t boxLogicalOffset(std::size_t box, std::size_t slot) noexcept {
    return kBoxPrefixBytes + (box * 30 + slot) * kBoxRecordSize;
}

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) noexcept {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

bool readLogical(std::span<const uint8_t> bytes,
                 const std::array<std::size_t, 14>& sectors,
                 uint8_t firstSector, std::size_t logical,
                 std::span<uint8_t> output) noexcept {
    for (std::size_t index = 0; index < output.size(); ++index) {
        const std::size_t position = logical + index;
        const std::size_t sectorId = static_cast<std::size_t>(firstSector) +
            position / Detail::kSectorDataSize;
        if (sectorId >= sectors.size()) return false;
        const std::size_t absolute = sectors[sectorId] + position % Detail::kSectorDataSize;
        if (absolute >= bytes.size()) return false;
        output[index] = bytes[absolute];
    }
    return true;
}

bool writeLogical(std::span<uint8_t> bytes,
                  const std::array<std::size_t, 14>& sectors,
                  uint8_t firstSector, std::size_t logical,
                  std::span<const uint8_t> input,
                  std::set<uint8_t>& touched) noexcept {
    for (std::size_t index = 0; index < input.size(); ++index) {
        const std::size_t position = logical + index;
        const std::size_t sectorId = static_cast<std::size_t>(firstSector) +
            position / Detail::kSectorDataSize;
        if (sectorId >= sectors.size()) return false;
        const std::size_t absolute = sectors[sectorId] + position % Detail::kSectorDataSize;
        if (absolute >= bytes.size()) return false;
        bytes[absolute] = input[index];
        touched.insert(static_cast<uint8_t>(sectorId));
    }
    return true;
}

void repairChecksums(std::vector<uint8_t>& bytes,
                     const std::array<std::size_t, 14>& sectors,
                     const std::set<uint8_t>& touched) {
    for (const uint8_t id : touched) {
        const std::size_t sector = sectors[id];
        const uint16_t checksum = Detail::sectorChecksum(
            std::span<const uint8_t>(bytes).subspan(sector, Detail::kChunkLengths[id]));
        write16(bytes, sector + 0xFF6, checksum);
    }
}

Inventory::ClassicGame classicGame(SourceGame game) noexcept {
    using Inventory::ClassicGame;
    switch (game) {
        case SourceGame::RubyGBA: return ClassicGame::Ruby;
        case SourceGame::SapphireGBA: return ClassicGame::Sapphire;
        case SourceGame::EmeraldGBA: return ClassicGame::Emerald;
        case SourceGame::FireRedGBA: return ClassicGame::FireRed;
        case SourceGame::LeafGreenGBA: return ClassicGame::LeafGreen;
    }
    return ClassicGame::FireRed;
}

uint8_t originVersion(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::SapphireGBA: return 1;
        case SourceGame::RubyGBA: return 2;
        case SourceGame::EmeraldGBA: return 3;
        case SourceGame::FireRedGBA: return 4;
        case SourceGame::LeafGreenGBA: return 5;
    }
    return 0;
}

bool validLanguage(uint8_t language) noexcept {
    // PK3 language ids: Japanese, English, French, Italian, German, Spanish.
    return language == 1 || language == 2 || language == 3 ||
           language == 4 || language == 5 || language == 7;
}

bool validHeldItem(SourceGame game, uint16_t item) noexcept {
    if (item == 0) return true;
    const auto g = classicGame(game);
    using Inventory::ClassicPocket;
    return Inventory::isAddableItem(g, ClassicPocket::Items, item) ||
           Inventory::isAddableItem(g, ClassicPocket::Balls, item) ||
           Inventory::isAddableItem(g, ClassicPocket::TMHM, item) ||
           Inventory::isAddableItem(g, ClassicPocket::Berries, item);
}

std::array<uint8_t, 6> ivs(const Pokemon::Pokemon3FRLG& p) noexcept {
    return {p.ivHP(), p.ivATK(), p.ivDEF(), p.ivSPE(), p.ivSPA(), p.ivSPD()};
}

std::array<uint8_t, 6> evs(const Pokemon::Pokemon3FRLG& p) noexcept {
    return {p.evHP(), p.evATK(), p.evDEF(), p.evSPE(), p.evSPA(), p.evSPD()};
}

std::array<uint16_t, 6> stats(const Pokemon::Pokemon3FRLG& p) noexcept {
    return {p.statHPMax(), p.statATK(), p.statDEF(), p.statSPE(), p.statSPA(), p.statSPD()};
}

std::optional<StagedPokemonRecord> parseRaw(
    const std::array<uint8_t, 80>& raw, std::string& error) {
    error.clear();
    Pokemon::Pokemon3FRLG pokemon(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    if (pokemon.speciesID() == 0) return std::nullopt;
    if (pokemon.speciesID() > 386) {
        error = "Generation III box slot contains an unsupported species";
        return std::nullopt;
    }
    if (!pokemon.checksumValid()) {
        error = "Generation III boxed Pokemon checksum is invalid";
        return std::nullopt;
    }

    StagedPokemonRecord out;
    out.species = pokemon.speciesID();
    out.nickname = Utils::utf16ToUtf8(pokemon.nickname());
    out.level = pokemon.level();
    out.experience = pokemon.exp();
    out.heldItem = pokemon.heldItem();
    for (int i = 0; i < 4; ++i) {
        out.moves[static_cast<std::size_t>(i)] = pokemon.move(i);
        out.pp[static_cast<std::size_t>(i)] = pokemon.movePP(i);
        out.ppUps[static_cast<std::size_t>(i)] = pokemon.movePPUps(i);
    }
    out.ivs = ivs(pokemon);
    out.evs = evs(pokemon);
    out.calculatedStats = stats(pokemon);
    out.otName = Utils::utf16ToUtf8(pokemon.otName());
    out.tid = pokemon.tid16();
    out.sid = pokemon.sid16();
    out.pid = pokemon.pid();
    out.language = pokemon.language();
    out.friendship = pokemon.friendship();
    out.pokerus = pokemon.pokerus();
    out.originGame = pokemon.originGame();
    out.ball = pokemon.ball();
    out.metLevel = pokemon.metLevel();
    out.metLocation = pokemon.metLocation();
    out.nature = pokemon.nature();
    out.gender = pokemon.gender();
    out.shiny = pokemon.isShiny(pokemon.id32(), "");
    out.ability = pokemon.ability();
    out.abilityNumber = pokemon.abilityNumber();
    out.form = pokemon.form();
    out.encryptedBytes = raw;
    return out;
}

std::array<uint8_t, 80> encryptedBytes(Pokemon::Pokemon3FRLG& pokemon) {
    std::array<uint8_t, 80> out{};
    std::byte* encrypted = Encryption::encryptArray3FRLG(pokemon.getData());
    std::memcpy(out.data(), encrypted, out.size());
    delete[] encrypted;
    return out;
}

bool validPpForMoves(const Pokemon::Pokemon3FRLG& pokemon,
                     const std::array<uint8_t,4>& pp,
                     const std::array<uint8_t,4>& ppUps,
                     std::string& error) {
    for (int i = 0; i < 4; ++i) {
        const uint16_t move = pokemon.move(i);
        if (ppUps[static_cast<std::size_t>(i)] > 3) {
            error = "Generation III PP Ups must be between 0 and 3";
            return false;
        }
        if (move == 0) {
            if (pp[static_cast<std::size_t>(i)] != 0 || ppUps[static_cast<std::size_t>(i)] != 0) {
                error = "Empty Generation III move slots must have zero PP and zero PP Ups";
                return false;
            }
            continue;
        }
        if (move > kMaxGen3Move) {
            error = "Generation III move id is outside the native move table";
            return false;
        }
        const uint8_t maximum = Names::getMoveMaxPP(
            move, ppUps[static_cast<std::size_t>(i)], Enums::GameVersion::FRLG);
        if (pp[static_cast<std::size_t>(i)] > maximum) {
            error = "Generation III current PP exceeds the exact Gen III maximum";
            return false;
        }
    }
    return true;
}

uint32_t deterministicCreatePid(uint16_t tid, uint16_t sid, std::size_t box,
                                std::size_t slot, uint16_t species) noexcept {
    uint32_t p = 0x6C8E9CF5u ^ (static_cast<uint32_t>(tid) << 16) ^ sid ^
                 (static_cast<uint32_t>(box) << 24) ^
                 (static_cast<uint32_t>(slot) << 12) ^ species;
    p &= ~1u; // default ability slot one for a dual-ability species
    const uint16_t tsv = static_cast<uint16_t>(tid ^ sid);
    for (uint32_t attempts = 0; attempts < 8192; ++attempts) {
        const uint16_t psv = static_cast<uint16_t>((p & 0xFFFFu) ^ (p >> 16));
        if ((tsv ^ psv) >= 8 && p != 0) return p;
        p += 2u;
    }
    return 2u;
}

std::string recordLabel(std::size_t box, std::size_t slot) {
    return "Box " + std::to_string(box + 1) + " / Slot " + std::to_string(slot + 1);
}

} // namespace

StagedPokemonEditor::StagedPokemonEditor(
    std::span<const uint8_t> source, SourceGame game, uint8_t activeSlot,
    uint32_t saveCounter, const std::array<std::size_t, 14>& offsets,
    TrainerRecord trainer)
    : sourceGame_(game), activeSlot_(activeSlot), saveCounter_(saveCounter),
      sectorOffsets_(offsets), trainer_(std::move(trainer)),
      original_(source.begin(), source.end()), staged_(source.begin(), source.end()) {}

uint8_t StagedPokemonEditor::originGame() const noexcept {
    return originVersion(sourceGame_);
}

std::unique_ptr<StagedPokemonEditor> StagedPokemonEditor::create(
    std::span<const uint8_t> source, SourceGame game, std::string& error) {
    error.clear();
    if (source.size() != Detail::kSaveSize) {
        error = "Generation III staged save must be exactly 128 KiB";
        return nullptr;
    }
    if (!Detail::sourceGameSupported(game)) {
        error = "Unsupported Generation III source identity";
        return nullptr;
    }
    const Detail::SlotValidation slots[2] = {
        Detail::validateSlot(source, 0), Detail::validateSlot(source, 1)};
    if (!slots[0].valid && !slots[1].valid) {
        error = "No checksum-valid Generation III save slot is available for staged editing";
        return nullptr;
    }
    const uint8_t active = Detail::selectActiveSlot(slots);
    const auto& selected = slots[active];
    if (!Detail::familyMatches(game, Detail::detectFamily(source, selected))) {
        error = "Generation III save family does not match the selected exact game";
        return nullptr;
    }

    auto parsed = parse(source, game);
    if (!parsed) {
        error = parsed.detail.empty() ? std::string(errorMessage(parsed.error)) : parsed.detail;
        return nullptr;
    }

    auto editor = std::unique_ptr<StagedPokemonEditor>(
        new StagedPokemonEditor(source, game, active, selected.counter,
                                selected.logicalSectorOffsets, parsed.save->trainer()));
    if (!editor->validateStaged(error)) return nullptr;
    return editor;
}

std::array<uint8_t, 80> StagedPokemonEditor::readBoxRaw(
    std::size_t box, std::size_t slot, bool& ok) const noexcept {
    std::array<uint8_t, 80> raw{};
    ok = box < boxCount() && slot < boxCapacity() &&
         readLogical(staged_, sectorOffsets_, kBoxFirstSector,
                     boxLogicalOffset(box, slot), raw);
    return raw;
}

std::optional<StagedPokemonRecord> StagedPokemonEditor::boxedPokemon(
    std::size_t box, std::size_t slot, std::string& error) const {
    error.clear();
    bool ok = false;
    const auto raw = readBoxRaw(box, slot, ok);
    if (!ok) {
        error = "Generation III box/slot is outside the native 14 x 30 storage";
        return std::nullopt;
    }
    return parseRaw(raw, error);
}

bool StagedPokemonEditor::writeBoxRaw(
    std::size_t box, std::size_t slot, std::span<const uint8_t> raw,
    std::string& error) {
    if (box >= boxCount() || slot >= boxCapacity() || raw.size() != kBoxRecordSize) {
        error = "Generation III box write is outside the native 14 x 30 storage";
        return false;
    }
    std::set<uint8_t> touched;
    if (!writeLogical(staged_, sectorOffsets_, kBoxFirstSector,
                      boxLogicalOffset(box, slot), raw, touched)) {
        error = "Generation III box write crossed an invalid logical sector";
        return false;
    }
    repairChecksums(staged_, sectorOffsets_, touched);
    return true;
}

bool StagedPokemonEditor::stageBoxPokemonEdit(
    std::size_t box, std::size_t slot, const BoxPokemonEdit& edit,
    std::string& error) {
    error.clear();
    if (carryingSparseMove()) { error = "Place or cancel the carried Pokemon first"; return false; }
    bool ok = false;
    const auto raw = readBoxRaw(box, slot, ok);
    if (!ok) {
        error = "Generation III box/slot is outside the native 14 x 30 storage";
        return false;
    }
    auto before = parseRaw(raw, error);
    if (!before) {
        if (error.empty()) error = "Selected Generation III box slot is empty";
        return false;
    }

    Pokemon::Pokemon3FRLG pokemon(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    const uint32_t originalPid = pokemon.pid();
    const uint16_t originalTid = pokemon.tid16();
    const uint16_t originalSid = pokemon.sid16();

    if (edit.species) {
        if (*edit.species == 0 || *edit.species > 386) {
            error = "Generation III species must be in the National Dex range 1-386";
            return false;
        }
        pokemon.setSpecies(*edit.species);
    }
    if (edit.nickname) {
        const auto value = Utils::utf8ToUtf16(*edit.nickname);
        if (value.size() > 10 || !pokemon.canStoreNickname(value)) {
            error = "Nickname cannot be represented in the Generation III character set";
            return false;
        }
        pokemon.setNickname(value);
    }
    if (edit.level && edit.experience) {
        error = "Edit either Level or Experience in one staged operation, not both";
        return false;
    }
    if (edit.level) {
        if (*edit.level < 1 || *edit.level > 100) {
            error = "Generation III level must be between 1 and 100";
            return false;
        }
        pokemon.setLevel(*edit.level);
    }
    if (edit.experience) pokemon.setExp(*edit.experience);
    if (edit.heldItem) {
        if (!validHeldItem(sourceGame_, *edit.heldItem)) {
            error = "Held Item is not valid in the exact Generation III game";
            return false;
        }
        pokemon.setHeldItem(*edit.heldItem);
    }
    if (edit.moves) {
        for (uint16_t move : *edit.moves) {
            if (move > kMaxGen3Move) {
                error = "Generation III move id is outside the native move table";
                return false;
            }
        }
        for (int i = 0; i < 4; ++i)
            pokemon.setMove(i, (*edit.moves)[static_cast<std::size_t>(i)]);
    }
    if (edit.ivs) {
        for (uint8_t value : *edit.ivs) {
            if (value > 31) {
                error = "Generation III IVs must be between 0 and 31";
                return false;
            }
        }
        for (int i = 0; i < 6; ++i) pokemon.setIV(i, (*edit.ivs)[static_cast<std::size_t>(i)]);
    }
    if (edit.evs) {
        unsigned total = 0;
        for (uint8_t value : *edit.evs) total += value;
        if (total > 510) {
            error = "Generation III EV total cannot exceed 510";
            return false;
        }
        for (int i = 0; i < 6; ++i) pokemon.setEV(i, (*edit.evs)[static_cast<std::size_t>(i)]);
    }
    if (edit.tid) pokemon.setTID16(*edit.tid);
    if (edit.otName) {
        const auto value = Utils::utf8ToUtf16(*edit.otName);
        if (value.size() > 7 || !pokemon.canStoreNickname(value)) {
            error = "OT name cannot be represented in the Generation III character set";
            return false;
        }
        pokemon.setOTName(value);
    }
    if (edit.language) {
        if (!validLanguage(*edit.language)) {
            error = "Unsupported Generation III language id";
            return false;
        }
        pokemon.setLanguage(*edit.language);
    }
    if (edit.friendship) pokemon.setFriendship(*edit.friendship);
    if (edit.pokerus) pokemon.setPokerus(*edit.pokerus);
    if (edit.ball) {
        if (*edit.ball < 1 || *edit.ball > 12 || *edit.ball == 0) {
            error = "Generation III Ball must be one of the native Master-through-Premier ids";
            return false;
        }
        pokemon.setBall(*edit.ball);
    }
    if (edit.metLevel) {
        if (*edit.metLevel > 100) {
            error = "Generation III met level must be between 0 and 100";
            return false;
        }
        pokemon.setMetLevel(*edit.metLevel);
    }
    if (edit.metLocation) {
        if (*edit.metLocation > 255) {
            error = "Generation III met location must fit the native 8-bit field";
            return false;
        }
        pokemon.setMetLocation(*edit.metLocation);
    }

    auto pps = edit.pp.value_or(before->pp);
    auto ppUps = edit.ppUps.value_or(before->ppUps);
    if (edit.moves || edit.pp || edit.ppUps) {
        if (!validPpForMoves(pokemon, pps, ppUps, error)) return false;
        for (int i = 0; i < 4; ++i) {
            pokemon.setMovePPUps(i, ppUps[static_cast<std::size_t>(i)]);
            pokemon.setMovePP(i, pps[static_cast<std::size_t>(i)]);
        }
    }

    const bool pidLinkedEdit =
        edit.nature.has_value() || edit.gender.has_value() ||
        edit.shiny.has_value() || edit.abilityNumber.has_value();
    const uint8_t expectedNature = edit.nature.value_or(pokemon.nature());
    const uint8_t expectedGender = edit.gender.value_or(pokemon.gender());
    const bool expectedShiny = edit.shiny.value_or(pokemon.isShiny(pokemon.id32(), ""));
    const uint8_t expectedAbilityNumber = edit.abilityNumber.value_or(pokemon.abilityNumber());

    if (edit.nature) {
        if (*edit.nature >= 25) {
            error = "Generation III nature must be between 0 and 24";
            return false;
        }
        pokemon.setNature(*edit.nature);
    }
    if (edit.gender) {
        if (*edit.gender > 2) {
            error = "Generation III gender selector is outside the native range";
            return false;
        }
        pokemon.setGender(*edit.gender);
    }
    if (edit.shiny) pokemon.setShiny(*edit.shiny, pokemon.id32());
    if (edit.abilityNumber) {
        if (*edit.abilityNumber < 1 || *edit.abilityNumber > 2) {
            error = "Generation III ability slot must be 1 or 2";
            return false;
        }
        pokemon.setAbilityNumber(*edit.abilityNumber);
    }

    if (pokemon.nature() != expectedNature ||
        pokemon.gender() != expectedGender ||
        pokemon.isShiny(pokemon.id32(), "") != expectedShiny ||
        pokemon.abilityNumber() != expectedAbilityNumber) {
        error = "Requested Generation III PID-linked field combination is not representable";
        return false;
    }
    if (pokemon.tid16() != edit.tid.value_or(originalTid) || pokemon.sid16() != originalSid) {
        error = "Generation III PID-linked edit changed TID/SID";
        return false;
    }
    if (!pidLinkedEdit && pokemon.pid() != originalPid) {
        error = "Generation III non-PID edit unexpectedly changed PID";
        return false;
    }

    const auto replacement = encryptedBytes(pokemon);
    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    if (!writeBoxRaw(box, slot, replacement, error)) {
        staged_ = stagedBackup;
        return false;
    }
    if (!validateStaged(error)) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        return false;
    }

    auto after = boxedPokemon(box, slot, error);
    if (!after) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        if (error.empty()) error = "Generation III edit did not survive strict reparse";
        return false;
    }
    if (after->tid != edit.tid.value_or(originalTid) || after->sid != originalSid ||
        (!pidLinkedEdit && after->pid != originalPid) ||
        after->nature != expectedNature || after->gender != expectedGender ||
        after->shiny != expectedShiny || after->abilityNumber != expectedAbilityNumber) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        error = "Generation III PID-linked edit failed strict semantic reparse";
        return false;
    }

    changes_.push_back({static_cast<uint8_t>(box), static_cast<uint8_t>(slot),
                        recordLabel(box, slot) + " Pokemon edit",
                        before->nickname + " Lv " + std::to_string(before->level),
                        after->nickname + " Lv " + std::to_string(after->level)});
    return true;
}

bool StagedPokemonEditor::stageAddBoxPokemon(
    std::size_t box, std::size_t slot, const BoxPokemonCreate& create,
    std::string& error) {
    error.clear();
    if (carryingSparseMove()) { error = "Place or cancel the carried Pokemon first"; return false; }
    if (box >= boxCount() || slot >= boxCapacity()) {
        error = "Generation III box/slot is outside the native 14 x 30 storage";
        return false;
    }
    std::string readError;
    if (boxedPokemon(box, slot, readError)) {
        error = "Generation III Create requires an empty native box slot";
        return false;
    }
    if (!readError.empty()) {
        error = readError;
        return false;
    }
    if (create.species == 0 || create.species > 386 ||
        create.level < 1 || create.level > 100 ||
        !validHeldItem(sourceGame_, create.heldItem) ||
        !validLanguage(create.language) ||
        create.ball < 1 || create.ball > 12 ||
        create.metLevel > 100 || create.metLocation > 255) {
        error = "Generation III Create draft contains an unsupported field value";
        return false;
    }

    std::array<uint8_t, 80> blank{};
    Pokemon::Pokemon3FRLG pokemon(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(blank.data()), blank.size()));

    const uint16_t tid = create.tid.value_or(trainer_.tid16);
    pokemon.setPID(deterministicCreatePid(tid, trainer_.sid16, box, slot, create.species));
    pokemon.setTID16(tid);
    pokemon.setSID16(trainer_.sid16);
    pokemon.setSpecies(create.species);
    pokemon.setLanguage(create.language);
    pokemon.setHeldItem(create.heldItem);
    pokemon.setFriendship(create.friendship);
    pokemon.setPokerus(create.pokerus);
    pokemon.setOriginGame(originVersion(sourceGame_));
    pokemon.setBall(create.ball);
    pokemon.setMetLevel(create.metLevel);
    pokemon.setMetLocation(create.metLocation);
    pokemon.setOTGender(trainer_.gender & 1);
    const auto ot = Utils::utf8ToUtf16(create.otName.value_or(trainer_.name));
    if (ot.size() > 7 || !pokemon.canStoreNickname(ot)) {
        error = "Create OT name cannot be represented in the Generation III character set";
        return false;
    }
    pokemon.setOTName(ot);

    std::string nickname = create.nickname;
    if (nickname.empty()) nickname = Names::getSpeciesName(create.species);
    const auto nickname16 = Utils::utf8ToUtf16(nickname);
    if (nickname16.size() > 10 || !pokemon.canStoreNickname(nickname16)) {
        error = "Create nickname cannot be represented in the Generation III character set";
        return false;
    }
    pokemon.setNickname(nickname16);
    pokemon.setLevel(create.level);
    if (create.experience) {
        if (Pokemon::getLevelFromExp(*create.experience, Pokemon::getGrowthRate(create.species)) != create.level) {
            error = "Generation III Create Level and Experience disagree";
            return false;
        }
        pokemon.setExp(*create.experience);
    }
    unsigned evTotal = 0;
    for (uint8_t value : create.ivs) {
        if (value > 31) { error = "Generation III IVs must be between 0 and 31"; return false; }
    }
    for (uint8_t value : create.evs) evTotal += value;
    if (evTotal > 510) { error = "Generation III EV total cannot exceed 510"; return false; }
    for (int i = 0; i < 6; ++i) {
        pokemon.setIV(i, create.ivs[static_cast<std::size_t>(i)]);
        pokemon.setEV(i, create.evs[static_cast<std::size_t>(i)]);
    }
    for (uint16_t move : create.moves) {
        if (move > kMaxGen3Move) {
            error = "Generation III move id is outside the native move table";
            return false;
        }
    }
    for (int i = 0; i < 4; ++i) {
        pokemon.setMove(i, create.moves[static_cast<std::size_t>(i)]);
        pokemon.setMovePPUps(i, create.ppUps[static_cast<std::size_t>(i)]);
        pokemon.setMovePP(i, create.pp[static_cast<std::size_t>(i)]);
    }
    if (!validPpForMoves(pokemon, create.pp, create.ppUps, error)) return false;

    const uint8_t expectedNature = create.nature.value_or(pokemon.nature());
    const uint8_t expectedGender = create.gender.value_or(pokemon.gender());
    const bool expectedShiny = create.shiny.value_or(pokemon.isShiny(pokemon.id32(), ""));
    const uint8_t expectedAbilityNumber = create.abilityNumber.value_or(pokemon.abilityNumber());
    if (create.nature) {
        if (*create.nature >= 25) {
            error = "Generation III nature must be between 0 and 24";
            return false;
        }
        pokemon.setNature(*create.nature);
    }
    if (create.gender) {
        if (*create.gender > 2) {
            error = "Generation III gender selector is outside the native range";
            return false;
        }
        pokemon.setGender(*create.gender);
    }
    if (create.shiny) pokemon.setShiny(*create.shiny, pokemon.id32());
    if (create.abilityNumber) {
        if (*create.abilityNumber < 1 || *create.abilityNumber > 2) {
            error = "Generation III ability slot must be 1 or 2";
            return false;
        }
        pokemon.setAbilityNumber(*create.abilityNumber);
    }
    if (pokemon.nature() != expectedNature ||
        pokemon.gender() != expectedGender ||
        pokemon.isShiny(pokemon.id32(), "") != expectedShiny ||
        pokemon.abilityNumber() != expectedAbilityNumber) {
        error = "Requested Generation III Create PID-linked field combination is not representable";
        return false;
    }

    const auto replacement = encryptedBytes(pokemon);
    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    if (!writeBoxRaw(box, slot, replacement, error) || !validateStaged(error)) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        return false;
    }
    auto after = boxedPokemon(box, slot, error);
    const std::string expectedNickname = create.nickname.empty()
        ? std::string(Names::getSpeciesName(create.species)) : create.nickname;
    const std::string expectedOt = create.otName.value_or(trainer_.name);
    if (!after || after->species != create.species || after->level != create.level ||
        (create.experience && after->experience != *create.experience) ||
        after->nickname != expectedNickname || after->otName != expectedOt ||
        after->tid != tid || after->heldItem != create.heldItem ||
        after->moves != create.moves || after->pp != create.pp || after->ppUps != create.ppUps ||
        after->ivs != create.ivs || after->evs != create.evs ||
        after->language != create.language || after->friendship != create.friendship ||
        after->pokerus != create.pokerus || after->ball != create.ball ||
        after->metLocation != create.metLocation || after->metLevel != create.metLevel ||
        after->originGame != originVersion(sourceGame_) ||
        after->nature != expectedNature || after->gender != expectedGender ||
        after->shiny != expectedShiny || after->abilityNumber != expectedAbilityNumber) {
        staged_ = stagedBackup;
        changes_ = changesBackup;
        if (error.empty()) error = "Generation III Create failed strict semantic reparse";
        return false;
    }
    changes_.push_back({static_cast<uint8_t>(box), static_cast<uint8_t>(slot),
                        recordLabel(box, slot) + " Add Pokemon", "Empty",
                        after->nickname + " Lv " + std::to_string(after->level)});
    return true;
}

bool StagedPokemonEditor::stageCloneBoxPokemon(
    std::size_t sourceBox, std::size_t sourceSlot,
    std::size_t destinationBox, std::size_t destinationSlot,
    std::string& error) {
    error.clear();
    if (carryingSparseMove()) { error = "Place or cancel the carried Pokemon first"; return false; }
    bool sourceOk = false;
    const auto sourceRaw = readBoxRaw(sourceBox, sourceSlot, sourceOk);
    if (!sourceOk) {
        error = "Generation III clone source is outside native storage";
        return false;
    }
    auto sourceRecord = parseRaw(sourceRaw, error);
    if (!sourceRecord) {
        if (error.empty()) error = "Generation III clone source slot is empty";
        return false;
    }
    std::string destError;
    if (boxedPokemon(destinationBox, destinationSlot, destError)) {
        error = "Generation III clone destination must be empty";
        return false;
    }
    if (!destError.empty()) {
        error = destError;
        return false;
    }

    const auto backup = staged_;
    const auto changesBackup = changes_;
    if (!writeBoxRaw(destinationBox, destinationSlot, sourceRaw, error) ||
        !validateStaged(error)) {
        staged_ = backup;
        changes_ = changesBackup;
        return false;
    }
    auto clone = boxedPokemon(destinationBox, destinationSlot, error);
    if (!clone || clone->encryptedBytes != sourceRecord->encryptedBytes) {
        staged_ = backup;
        changes_ = changesBackup;
        if (error.empty()) error = "Generation III clone failed byte-identical reparse";
        return false;
    }
    changes_.push_back({static_cast<uint8_t>(destinationBox),
                        static_cast<uint8_t>(destinationSlot),
                        recordLabel(destinationBox, destinationSlot) + " Clone Pokemon",
                        "Empty", clone->nickname});
    return true;
}

bool StagedPokemonEditor::stageReleaseBoxPokemon(
    std::size_t box, std::size_t slot, std::string& error) {
    error.clear();
    if (carryingSparseMove()) { error = "Place or cancel the carried Pokemon first"; return false; }
    auto before = boxedPokemon(box, slot, error);
    if (!before) {
        if (error.empty()) error = "Generation III Release requires an occupied slot";
        return false;
    }
    std::array<uint8_t,80> empty{};
    const auto backup = staged_;
    const auto changesBackup = changes_;
    if (!writeBoxRaw(box, slot, empty, error) || !validateStaged(error)) {
        staged_ = backup;
        changes_ = changesBackup;
        return false;
    }
    std::string afterError;
    if (boxedPokemon(box, slot, afterError) || !afterError.empty()) {
        staged_ = backup;
        changes_ = changesBackup;
        error = afterError.empty() ? "Generation III Release did not leave an empty slot" : afterError;
        return false;
    }
    changes_.push_back({static_cast<uint8_t>(box), static_cast<uint8_t>(slot),
                        recordLabel(box, slot) + " Release Pokemon",
                        before->nickname, "Empty"});
    return true;
}

bool StagedPokemonEditor::validateStaged(std::string& error) const {
    error.clear();
    const auto selected = Detail::validateSlot(staged_, activeSlot_);
    if (!selected.valid) {
        error = "Generation III staged save failed sector/checksum validation";
        return false;
    }
    if (selected.counter != saveCounter_ || selected.logicalSectorOffsets != sectorOffsets_) {
        error = "Generation III staged edit changed save counter or rotating-sector identity";
        return false;
    }
    if (!Detail::familyMatches(sourceGame_, Detail::detectFamily(staged_, selected))) {
        error = "Generation III staged edit changed the validated save family";
        return false;
    }

    const uint8_t inactive = activeSlot_ == 0 ? 1 : 0;
    const std::size_t inactiveBase = Detail::kSlotBases[inactive];
    const std::size_t inactiveLength = Detail::kSectorCount * Detail::kSectorSize;
    if (!std::equal(original_.begin() + static_cast<std::ptrdiff_t>(inactiveBase),
                    original_.begin() + static_cast<std::ptrdiff_t>(inactiveBase + inactiveLength),
                    staged_.begin() + static_cast<std::ptrdiff_t>(inactiveBase))) {
        error = "Generation III staged Pokemon edit modified the inactive save slot";
        return false;
    }

    const auto reparsed = parse(staged_, sourceGame_);
    if (!reparsed) {
        error = reparsed.detail.empty() ? std::string(errorMessage(reparsed.error)) : reparsed.detail;
        return false;
    }
    if (reparsed.save->metadata().activeSlot != activeSlot_ ||
        reparsed.save->metadata().saveCounter != saveCounter_ ||
        reparsed.save->metadata().boxCount != boxCount() ||
        reparsed.save->metadata().slotsPerBox != boxCapacity()) {
        error = "Generation III staged Pokemon edit changed parsed save identity";
        return false;
    }
    return true;
}

std::optional<StagedPokemonRecord> StagedPokemonEditor::decodeRecord(
    const std::array<uint8_t,80>& raw, std::string& error) { return parseRaw(raw, error); }

std::optional<StagedPokemonRecord> StagedPokemonEditor::previewEdit(
    std::size_t box, std::size_t slot, const BoxPokemonEdit& edit, std::string& error) const {
    auto preview = *this;
    if (!preview.stageBoxPokemonEdit(box, slot, edit, error)) return {};
    return preview.boxedPokemon(box, slot, error);
}

std::optional<StagedPokemonRecord> StagedPokemonEditor::previewCreate(
    std::size_t box, std::size_t slot, const BoxPokemonCreate& create, std::string& error) const {
    auto preview = *this;
    if (!preview.stageAddBoxPokemon(box, slot, create, error)) return {};
    return preview.boxedPokemon(box, slot, error);
}

bool StagedPokemonEditor::beginSparseMove(std::size_t box,
    const std::vector<std::size_t>& slots, std::string& error) {
    error.clear();
    if (carryingSparseMove() || slots.empty() || box >= boxCount()) {
        error = "Select occupied native slots before moving"; return false;
    }
    auto ordered = slots;
    std::sort(ordered.begin(), ordered.end());
    if (std::adjacent_find(ordered.begin(), ordered.end()) != ordered.end()) {
        error = "Duplicate move source"; return false;
    }
    std::vector<std::array<uint8_t,80>> records;
    for (auto slot : ordered) {
        auto p = boxedPokemon(box, slot, error);
        if (!p) { if (error.empty()) error = "Move source is empty"; return false; }
        records.push_back(p->encryptedBytes);
    }
    const auto backup = staged_;
    const std::array<uint8_t,80> empty{};
    for (auto slot : ordered) {
        if (!writeBoxRaw(box, slot, empty, error)) { staged_ = backup; return false; }
    }
    if (!validateStaged(error)) { staged_ = backup; return false; }
    beforeCarry_ = backup;
    carrySlots_ = std::move(ordered);
    carryRecords_ = std::move(records);
    return true;
}

bool StagedPokemonEditor::placeSparseMove(std::size_t box, std::size_t anchor,
                                          std::string& error) {
    error.clear();
    if (!carryingSparseMove() || box >= boxCount() || anchor >= boxCapacity()) {
        error = "No carried Pokemon or invalid destination"; return false;
    }
    // Translate the occupied selection relative to its first row-major slot.
    // Preserve holes and row/column offsets; never wrap a row or compact neighbors.
    std::vector<std::size_t> destinations;
    const auto first = carrySlots_.front();
    for (auto source : carrySlots_) {
        const int row = int(anchor / 6) + int(source / 6) - int(first / 6);
        const int col = int(anchor % 6) + int(source % 6) - int(first % 6);
        if (row < 0 || row >= 5 || col < 0 || col >= 6) {
            error = "Selected shape does not fit this destination"; return false;
        }
        const std::size_t slot = row * 6 + col;
        if (boxedPokemon(box, slot, error) || !error.empty()) {
            if (error.empty()) error = "Move destination must be empty; no overwrite";
            return false;
        }
        destinations.push_back(slot);
    }
    const auto backup = staged_;
    for (std::size_t i = 0; i < destinations.size(); ++i) {
        if (!writeBoxRaw(box, destinations[i], carryRecords_[i], error)) {
            staged_ = backup; return false;
        }
    }
    if (!validateStaged(error)) { staged_ = backup; return false; }
    for (std::size_t i = 0; i < destinations.size(); ++i) {
        auto p = boxedPokemon(box, destinations[i], error);
        if (!p || p->encryptedBytes != carryRecords_[i]) {
            staged_ = backup; error = "Sparse move failed exact reparse"; return false;
        }
    }
    changes_.push_back({static_cast<uint8_t>(box), static_cast<uint8_t>(anchor),
        "Sparse Pokemon move", std::to_string(carrySlots_.size()) + " carried", "Exact native slots"});
    carrySlots_.clear(); carryRecords_.clear(); beforeCarry_.clear();
    return true;
}

bool StagedPokemonEditor::cancelSparseMove(std::string& error) {
    error.clear();
    if (!carryingSparseMove()) { error = "No sparse move to cancel"; return false; }
    staged_ = std::move(beforeCarry_);
    carrySlots_.clear(); carryRecords_.clear();
    return true;
}

void StagedPokemonEditor::discard() noexcept {
    staged_ = original_;
    changes_.clear();
    carrySlots_.clear(); carryRecords_.clear(); beforeCarry_.clear();
}

std::vector<uint8_t> StagedPokemonEditor::finalizedBytes(std::string& error) const {
    if (carryingSparseMove()) { error = "Place or cancel the carried Pokemon first"; return {}; }
    if (!validateStaged(error)) return {};
    if (staged_.size() != original_.size()) {
        error = "Generation III staged save size changed unexpectedly";
        return {};
    }
    return staged_;
}

} // namespace PokeVault::Integration::Gen3
