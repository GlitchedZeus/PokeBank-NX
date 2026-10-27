// R2-BUG-002: Gen III Trainer Name / Money staged through Gen3StagedTrainerEditor and written only
// into the verified export copy, for Ruby, Sapphire, Emerald, FireRed and LeafGreen.
//
// Every game: edit -> verified export -> strict reparse; the exported fields read back as entered
// (money through the save's own XOR key: none on Ruby / Sapphire, a non-zero key on Emerald and
// FireRed / LeafGreen); every byte outside the authorised name / money fields and the two section
// checksums is identical to the source; the inactive save slot is untouched; Review lists both
// changes; Discard restores; the source buffer never changes. A Bag edit (also in section 1) and a
// Trainer money edit export together. A Japanese-shaped save keeps the name read-only.
#include "Integration/Gen3/Gen3ExportTransaction.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen3/Gen3StagedTrainerEditor.h"
#include "UI/ClassicTrainerEditability.h"
#include "Utils/Gen3Text.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

using namespace PokeVault::Integration::Gen3;

namespace {

enum class Family { RS, Emerald, FRLG };
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;
constexpr uint32_t kKey = 0xA1B2C3D4;

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}

std::size_t moneyOffset(Family family) { return family == Family::FRLG ? 0x290 : 0x490; }
uint32_t keyOf(Family family) { return family == Family::RS ? 0 : kKey; }

LogicalSectors makeLogical(Family family, bool japanese) {
    LogicalSectors logical{};
    // Trainer name: international "KYLIE" + EOS fill; a Japanese save is 5 characters with
    // bytes 6-7 left zero (the repo's PKHeX / PKSM detection rule).
    std::fill(logical[0].begin(), logical[0].begin() + 8, uint8_t{0xFF});
    const std::u16string name = u"KYLIE";
    for (std::size_t i = 0; i < name.size(); ++i) logical[0][i] = Utils::charToGen3(name[i], 2);
    if (japanese) logical[0][6] = logical[0][7] = 0;
    write16(logical[0], 0x0A, 12345);
    write16(logical[0], 0x0C, 54321);
    write16(logical[0],0x0E,5432);logical[0][0x10]=45;logical[0][0x11]=59;
    if (family == Family::FRLG) {
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, kKey);
    } else if (family == Family::Emerald) {
        write32(logical[0], 0xAC, kKey);
        logical[0][0xEE0] = 0x42;
    }
    write32(logical[1], moneyOffset(family), 3000 ^ keyOf(family));
    // One Potion (item 13) in the Items pocket, quantity XOR the low 16 key bits where keyed.
    const std::size_t items = family == Family::FRLG ? 0x310 : 0x560;
    write16(logical[1], items, 13);
    write16(logical[1], items + 2, static_cast<uint16_t>(3 ^ (family == Family::RS ? 0 : (kKey & 0xFFFF))));
    return logical;
}

void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family,
               bool japanese) {
    auto logical = makeLogical(family, japanese);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6,
                Detail::sectorChecksum(std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}

// Slot 1 is newer and rotated, so the active logical sectors are not at their physical index.
std::vector<uint8_t> fixture(Family family, bool japanese = false) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, japanese);
    writeSlot(save, 1, 9, 5, family, japanese);
    return save;
}

std::vector<uint8_t> slurp(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

const char* gameId(SourceGame game) {
    switch (game) {
        case SourceGame::RubyGBA: return "ruby_gba";
        case SourceGame::SapphireGBA: return "sapphire_gba";
        case SourceGame::EmeraldGBA: return "emerald_gba";
        case SourceGame::FireRedGBA: return "firered_gba";
        case SourceGame::LeafGreenGBA: return "leafgreen_gba";
    }
    return "";
}

void runGame(SourceGame game, Family family, const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    auto source = fixture(family);
    const auto original = source;
    std::string error;
    auto pokemon = StagedPokemonEditor::create(source, game, error);
    assert(pokemon && error.empty());
    auto trainer = Gen3StagedTrainerEditor::create(source, game, error);
    assert(trainer && error.empty());
    assert(trainer->nameEditable() && trainer->trainerName() == "KYLIE" && trainer->money() == 3000);

    const auto native=parse(source,game);
    assert(native && PokeVault::Integration::Common::playTimeText(native.save->trainer().playTime)=="5432:45:59");
    // Refusals stage nothing.
    assert(!trainer->stageMoney(1000000, error) && !error.empty());
    assert(!trainer->stageTrainerName("", error));
    assert(!trainer->stageTrainerName("EIGHTCHR", error));
    assert(!trainer->stageTrainerName("A\xF0\x9F\x98\x80", error));
    assert(!trainer->hasPendingChanges());
    // An unchanged submit stages nothing.
    assert(trainer->stage("KYLIE", 3000, error) && !trainer->hasPendingChanges());

    assert(trainer->stage("ASH", 123456, error) && error.empty());
    assert(trainer->trainerName() == "ASH" && trainer->money() == 123456 && trainer->hasPendingChanges());
    const auto changes = trainer->pendingChanges();
    assert(changes.size() == 2);
    assert(changes[0].label == "Trainer name" && changes[0].before == "KYLIE" && changes[0].after == "ASH");
    assert(changes[1].label == "Money" && changes[1].before == "3000" && changes[1].after == "123456");

    // Trainer-only export (the Pokemon workspace has nothing pending).
    Gen3ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(gameId(game)) + "_trainer";
    request.sourcePath = "sdmc:/retroarch/saves/fixture.srm";
    request.timestamp = "20261019-120000";
    auto exported = publishVerifiedGen3Export(*pokemon, request, nullptr, trainer.get());
    assert(exported.success && exported.error.empty());
    const auto edited = slurp(fs::path(exported.directory) / "edited.sav");
    assert(slurp(fs::path(exported.directory) / "original_backup.sav") == original);
    assert(source == original);

    // Strict reparse of the exported copy: the new Name / Money.
    auto parsed = parse(edited, game);
    assert(parsed && parsed.save->trainer().name == "ASH" && parsed.save->trainer().money == 123456);

    // Footprint: only the name (8 bytes), money (4 bytes) and sections 0 / 1 checksums of the
    // ACTIVE slot changed; the inactive slot is byte-identical.
    const Detail::SlotValidation slots[2] = {Detail::validateSlot(original, 0), Detail::validateSlot(original, 1)};
    const uint8_t active = Detail::selectActiveSlot(slots);
    assert(active == 1);
    const auto& sectors = slots[active].logicalSectorOffsets;
    const std::size_t name = sectors[0], money = sectors[1] + moneyOffset(family);
    for (std::size_t i = 0; i < edited.size(); ++i) {
        if (edited[i] == original[i]) continue;
        const bool allowed = (i >= name && i < name + 8) || (i >= money && i < money + 4) ||
                             i == sectors[0] + 0xFF6 || i == sectors[0] + 0xFF7 ||
                             i == sectors[1] + 0xFF6 || i == sectors[1] + 0xFF7;
        assert(allowed);
    }
    // Money is stored XOR the save's own key (none on Ruby / Sapphire).
    const uint32_t stored = Detail::read32(edited, money);
    assert((stored ^ keyOf(family)) == 123456);
    if (family != Family::RS) assert(stored != 123456);
    // Name written like the naming screen: three characters then EOS to the end of the field.
    for (std::size_t i = name + 3; i < name + 8; ++i) assert(edited[i] == 0xFF);
    std::ifstream manifestIn(fs::path(exported.directory) / "EDIT_MANIFEST.txt");
    const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
    assert(manifest.find("Trainer name: KYLIE -> ASH") != std::string::npos);
    assert(manifest.find("Money: 3000 -> 123456") != std::string::npos);

    // A Bag edit (section 1) and the Trainer money edit (section 1) export together.
    auto bag = StagedInventoryEditor::create(source, game, error);
    assert(bag && error.empty());
    assert(bag->stageInventoryQuantity(InventoryPouch::Items, 13, 7, error) && error.empty());
    request.directoryStem = std::string(gameId(game)) + "_bag_trainer";
    auto both = publishVerifiedGen3Export(*pokemon, request, bag.get(), trainer.get());
    assert(both.success && both.error.empty());
    const auto bothBytes = slurp(fs::path(both.directory) / "edited.sav");
    auto bothParsed = parse(bothBytes, game);
    assert(bothParsed && bothParsed.save->trainer().money == 123456 && bothParsed.save->trainer().name == "ASH");
    auto reopened = StagedInventoryEditor::create(bothBytes, game, error);
    assert(reopened && reopened->stagedInventoryQuantity(InventoryPouch::Items, 13) == 7);

    // Discard restores the source values.
    trainer->discard();
    assert(!trainer->hasPendingChanges() && trainer->trainerName() == "KYLIE" && trainer->money() == 3000);
    assert(source == original);
}

// P3 (Round 5): Trainer Gender (section 0 +0x08) and Play Time (section 0 +0x0E u16 hours,
// +0x10 minutes, +0x11 seconds; PKSM-Core Sav3 / pret SaveBlock2) applied to the verified export
// copy. Byte-exact: only those bytes and the section 0 checksum of the ACTIVE slot change (the
// frame counter +0x12 and the inactive slot stay identical); strict reparse returns the values;
// Review / manifest list both; Discard restores; the source is never written.
void genderPlayTime(SourceGame game, Family family, const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    auto source = fixture(family);
    const auto original = source;
    std::string error;
    auto pokemon = StagedPokemonEditor::create(source, game, error);
    auto trainer = Gen3StagedTrainerEditor::create(source, game, error);
    assert(pokemon && trainer && error.empty());
    assert(trainer->genderEditable() && trainer->gender() == 0 && trainer->playTimeEditable());
    const auto start = trainer->playTime();
    assert(start.hours == 5432 && start.minutes == 45 && start.seconds == 59);

    assert(!trainer->stageGender(2, error) && !error.empty());
    assert(!trainer->stagePlayTime({1000, 0, 0}, error));
    assert(!trainer->stagePlayTime({1, 60, 0}, error) && !trainer->stagePlayTime({1, 0, 60}, error));
    assert(!trainer->hasPendingChanges());

    assert(trainer->stageGender(1, error) && trainer->gender() == 1);
    assert(trainer->stagePlayTime({517, 42, 9}, error) && error.empty());
    const auto changes = trainer->pendingChanges();
    assert(changes.size() == 2);
    assert(changes[0].label == "Trainer gender" && changes[0].before == "Male" && changes[0].after == "Female");
    assert(changes[1].label == "Play time" && changes[1].before == "5432:45:59" && changes[1].after == "517:42:09");

    Gen3ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(gameId(game)) + "_gender_playtime";
    request.sourcePath = "sdmc:/retroarch/saves/fixture.srm";
    request.timestamp = "20261024-120000";
    auto exported = publishVerifiedGen3Export(*pokemon, request, nullptr, trainer.get());
    assert(exported.success && exported.error.empty());
    const auto edited = slurp(fs::path(exported.directory) / "edited.sav");
    assert(slurp(fs::path(exported.directory) / "original_backup.sav") == original);
    assert(source == original);

    auto parsed = parse(edited, game);
    assert(parsed && parsed.save->trainer().gender == 1);
    assert(PokeVault::Integration::Common::playTimeText(parsed.save->trainer().playTime) == "517:42:09");
    assert(parsed.save->trainer().name == "KYLIE" && parsed.save->trainer().money == 3000);

    const Detail::SlotValidation slots[2] = {Detail::validateSlot(original, 0), Detail::validateSlot(original, 1)};
    const uint8_t active = Detail::selectActiveSlot(slots);
    const std::size_t block = slots[active].logicalSectorOffsets[0];
    assert(edited[block + 0x08] == 1);
    assert(edited[block + 0x0E] == (517 & 0xFF) && edited[block + 0x0F] == (517 >> 8) &&
           edited[block + 0x10] == 42 && edited[block + 0x11] == 9);
    for (std::size_t i = 0; i < edited.size(); ++i) {
        if (edited[i] == original[i]) continue;
        const bool allowed = i == block + 0x08 || (i >= block + 0x0E && i < block + 0x12) ||
                             i == block + 0xFF6 || i == block + 0xFF7;
        assert(allowed);
    }
    std::ifstream manifestIn(fs::path(exported.directory) / "EDIT_MANIFEST.txt");
    const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
    assert(manifest.find("Trainer gender: Male -> Female") != std::string::npos);
    assert(manifest.find("Play time: 5432:45:59 -> 517:42:09") != std::string::npos);

    trainer->discard();
    assert(!trainer->hasPendingChanges() && trainer->gender() == 0 && trainer->playTime() == start);
    assert(source == original);
}

void japaneseNameReadOnly() {
    auto source = fixture(Family::FRLG, true);
    std::string error;
    auto trainer = Gen3StagedTrainerEditor::create(source, SourceGame::FireRedGBA, error);
    assert(trainer && !trainer->nameEditable());
    assert(!trainer->stageTrainerName("ASH", error) && !trainer->hasPendingChanges());
    assert(trainer->stageMoney(999999, error) && trainer->money() == 999999);
}

} // namespace

// An emulator RTC footer after the 128 KiB flash image (mGBA/VBA-M 16 bytes) no longer locks the
// Bag and Trainer editors (they required exactly 0x20000 while the Pokemon editor and discovery
// accepted a footer; cf. upstream PKSE 292fd5e). The footer is carried verbatim through a
// combined Bag + Trainer + Pokemon export, and an edit that changes it is refused.
void rtcFooterExport(SourceGame game, Family family, const std::filesystem::path& root) {
    namespace fs = std::filesystem;
    auto source = fixture(family);
    const std::array<uint8_t, 16> footer{{0x52, 0x54, 0x43, 0x01, 0x02, 0x03, 0x04, 0x05,
                                          0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}};
    source.insert(source.end(), footer.begin(), footer.end());
    const auto original = source;
    std::string error;
    auto pokemon = StagedPokemonEditor::create(source, game, error);
    assert(pokemon && error.empty());
    auto trainer = Gen3StagedTrainerEditor::create(source, game, error);
    assert(trainer && error.empty());
    auto bag = StagedInventoryEditor::create(source, game, error);
    assert(bag && error.empty());

    // A footer that is not a known RTC size is still refused by all three.
    auto odd = fixture(family);
    odd.push_back(0x00);
    assert(!StagedInventoryEditor::create(odd, game, error) && !error.empty());
    assert(!Gen3StagedTrainerEditor::create(odd, game, error) && !error.empty());
    assert(!StagedPokemonEditor::create(odd, game, error));

    // An unedited Bag finalizes to the exact source, footer included.
    assert(bag->finalizedBytes(error) == original);

    assert(bag->stageInventoryQuantity(InventoryPouch::Items, 13, 7, error) && error.empty());
    assert(trainer->stage("ASH", 123456, error) && error.empty());
    Gen3ExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = std::string(gameId(game)) + "_rtc_bag_trainer";
    request.sourcePath = "sdmc:/retroarch/saves/fixture.srm";
    request.timestamp = "20261027-120000";
    auto both = publishVerifiedGen3Export(*pokemon, request, bag.get(), trainer.get());
    if (!both.success) std::cerr << "rtc export error: " << both.error << "\n";
    assert(both.success && both.error.empty());
    const auto bytes = slurp(fs::path(both.directory) / "edited.sav");
    assert(bytes.size() == original.size());
    assert(std::equal(footer.begin(), footer.end(), bytes.end() - static_cast<std::ptrdiff_t>(footer.size())));
    assert(slurp(fs::path(both.directory) / "original_backup.sav") == original);
    assert(source == original);
    auto parsed = parse(bytes, game);
    assert(parsed && parsed.save->trainer().money == 123456 && parsed.save->trainer().name == "ASH");
    auto reopened = StagedInventoryEditor::create(bytes, game, error);
    assert(reopened && reopened->stagedInventoryQuantity(InventoryPouch::Items, 13) == 7);

    // A staged image whose footer changed cannot take the Trainer edit.
    auto tampered = original;
    tampered.back() ^= 0xFF;
    assert(!trainer->applyTo(tampered, error) && error.find("RTC footer") != std::string::npos);
}

int main() {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / ("pokebank-g3-trainer-" + std::to_string(::getpid()));
    fs::remove_all(root);
    runGame(SourceGame::RubyGBA, Family::RS, root);
    runGame(SourceGame::SapphireGBA, Family::RS, root);
    runGame(SourceGame::EmeraldGBA, Family::Emerald, root);
    runGame(SourceGame::FireRedGBA, Family::FRLG, root);
    runGame(SourceGame::LeafGreenGBA, Family::FRLG, root);
    japaneseNameReadOnly();
    rtcFooterExport(SourceGame::EmeraldGBA, Family::Emerald, root);
    rtcFooterExport(SourceGame::FireRedGBA, Family::FRLG, root);
    rtcFooterExport(SourceGame::RubyGBA, Family::RS, root);
    genderPlayTime(SourceGame::RubyGBA, Family::RS, root);
    genderPlayTime(SourceGame::SapphireGBA, Family::RS, root);
    genderPlayTime(SourceGame::EmeraldGBA, Family::Emerald, root);
    genderPlayTime(SourceGame::FireRedGBA, Family::FRLG, root);
    genderPlayTime(SourceGame::LeafGreenGBA, Family::FRLG, root);
    // The emulator-source guard used by trainerFieldsEditable for the FRLG group.
    using PokeBank::UIModel::ClassicTrainerSession::isEmulatorGen3SourceId;
    for (const char* id : {"ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba"})
        assert(isEmulatorGen3SourceId(id));
    assert(!isEmulatorGen3SourceId("") && !isEmulatorGen3SourceId("platinum_nds"));
    fs::remove_all(root);
    std::cout << "Gen III staged Trainer Name / Money (R / S / E / FR / LG) PASS\n";
    return 0;
}
