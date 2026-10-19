// End-to-end Gen V source flow on synthetic files for all four titles and
// both retail copies: choose a copy row -> assign to a profile -> reload the
// bindings database -> reopen with the exact fingerprint and copy -> stage
// PK5 + Bag + Trainer edits -> publish a verified working copy. The original
// file bytes never change; a stale source (emulator saved meanwhile) and a
// wrong copy/fingerprint are refused before any export file is published.
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace B = PokeVault::Legacy;

namespace {

void stampCopy(std::vector<uint8_t>& sav, size_t base, G::SaveFamily family) {
    const auto blocks = family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                            : std::span<const L::SaveBlock>(L::Black2White2);
    const size_t size = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(base, size);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}

// One valid copy at `base`: trainer `initial`+"X", one party Pikachu, a bag
// stack, money. Both copies of a file are valid but distinguishable.
void writeCopy(std::vector<uint8_t>& sav, size_t base, G::SaveFamily family, uint8_t version,
               char initial, uint16_t potions) {
    const size_t player = base + 0x19400;
    C::write16(sav, player + 0x04, static_cast<uint16_t>(initial));
    C::write16(sav, player + 0x06, 'X');
    C::write16(sav, player + 0x08, 0xFFFF);
    C::write16(sav, player + 0x14, 12345);
    C::write16(sav, player + 0x16, 54321);
    sav[player + 0x1F] = version;
    C::write16(sav, player + 0x24, 5);
    const size_t misc = base + G::trainerBlock5(family, G::TrainerLayout5::MiscBlockIndex).offset;
    C::write16(sav, misc, 1000);
    std::vector<uint8_t> pk(C::PartySize, 0);
    C::write16(pk, 0, 0x1357);
    C::write16(pk, 8, 25);
    C::write16(pk, 12, 12345);
    C::write16(pk, 14, 54321);
    pk[0x8C] = 20;
    const auto party = C::encryptCandidate(pk);
    sav[base + L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8));
    const size_t medicine = base + G::itemsBlockOffset(family) + G::bagPocketLayout5(family, 3).offset;
    C::write16(sav, medicine, 17);
    C::write16(sav, medicine + 2, potions);
    stampCopy(sav, base, family);
}

std::vector<uint8_t> diskBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    FILE* f = std::fopen(path.c_str(), "wb");
    assert(f && std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size() && std::fclose(f) == 0);
}
void removeExport(const std::string& directory, const std::string& root, const std::string& profile) {
    for (const char* f : {"/original_backup.sav", "/edited.sav", "/EXPORT_MANIFEST.txt"})
        assert(std::remove((directory + f).c_str()) == 0);
    assert(::rmdir(directory.c_str()) == 0);
    assert(::rmdir((root + "/account-" + profile).c_str()) == 0);
}

void flow(const std::string& dir, G::SaveFamily family, const char* game, uint8_t version, bool backup) {
    const size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    std::vector<uint8_t> raw(L::FullSaveSize, 0);
    writeCopy(raw, 0, family, version, 'P', 3);
    writeCopy(raw, copySize, family, version, 'B', 9);
    const std::string path = dir + "/" + game + (backup ? "_b.sav" : "_p.sav");
    writeFile(path, raw);
    const auto original = diskBytes(path);

    // Two distinct valid copies -> two labeled choice rows from one read.
    const auto rows = G::inspectSourceChoices(path, "DraStic", game);
    assert(rows.size() == 2);
    const auto wanted = backup ? "gen5-copy-1-backup" : "gen5-copy-0-primary";
    const auto chosen = std::find_if(rows.begin(), rows.end(),
                                     [&](const auto& r) { return r.sourceVariant == wanted; });
    assert(chosen != rows.end() && chosen->ready() && chosen->readOnly());

    // Assign to profile, then reload the bindings database from disk.
    const std::string profile(32, backup ? 'b' : 'a');
    const std::string db = dir + "/bindings_" + game + (backup ? "_b" : "_p");
    {
        B::LegacySourceBindings bindings(db);
        assert(bindings.load());
        const auto assigned = G::assignValidatedSource(bindings, *chosen, profile, game);
        assert(assigned.success && assigned.instance.rememberedSource);
    }
    B::LegacySourceBindings reloaded(db);
    assert(reloaded.load());
    const auto copy = backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
    const auto wrongCopy = backup ? G::SaveCopySelection::Primary : G::SaveCopySelection::Backup;
    // Wrong fingerprint and other profiles are refused.
    assert(!G::openAssignedSource(reloaded, profile, game, copy, std::string(64, '0')).ready());
    assert(G::openAssignedSource(reloaded, std::string(32, 'z'), game, copy, chosen->contentFingerprint).status ==
           G::AssignedOpenStatus::Unassigned);
    const auto opened = G::openAssignedSource(reloaded, profile, game, copy, chosen->contentFingerprint);
    assert(opened.ready() && opened.save->selectedBackupPartition() == backup);
    assert(G::decodeTrainer5(*opened.save)->rawName == (backup ? u"BX" : u"PX"));
    assert((*G::decodeBag5(*opened.save))[3][0].count == (backup ? 9 : 3));
    const auto other = G::openAssignedSource(reloaded, profile, game, wrongCopy, chosen->contentFingerprint);
    if (other.ready()) assert(other.save->selectedBackupPartition() != backup);

    // Stage through the same Trainer bridge the shared UI uses.
    std::string error;
    auto trainer = B::Gen5ReadOnlyTrainer::create(*opened.save, game, error);
    assert(trainer);
    auto& ws = trainer->stagedPokemon();
    using Field = G::StagedPokemon5Record::Field;
    assert(ws.stageParty(0, Field::Friendship, 0, 222, &error));
    assert(ws.stagedBag(&error)->setQuantity(3, 0, 42, &error));
    assert(ws.stagedTrainer(&error)->setMoney(31337, &error));
    assert(trainer->refreshStagedPokemonPresentation(error));

    // Publish from a fresh read-only re-read of the physical source.
    std::vector<uint8_t> observed;
    std::string container;
    assert(G::readNormalizedSourceReadOnly(path, observed, error, &container));
    const std::string root = dir + "/exports";
    ::mkdir(root.c_str(), 0700);
    G::WorkingCopyExportRequest req;
    req.profileIdentity = profile;
    req.timestamp = backup ? "20261014_120001" : "20261014_120000";
    req.sourcePath = path;
    req.applicationSha = "host-e2e";
    req.observedSourceBytes = observed;
    req.rootDirectory = root;
    const auto result = G::publishGen5WorkingCopy(ws, req);
    assert(result.success && result.error.empty());
    assert(diskBytes(path) == original);                        // source untouched
    assert(diskBytes(result.directory + "/original_backup.sav") == original);
    const auto edited = diskBytes(result.directory + "/edited.sav");
    const auto reparsed = G::Gen5ReadOnlySave::parse(edited, game, &error, copy);
    assert(reparsed && reparsed->selectedBackupPartition() == backup);
    assert(reparsed->partyPokemon(0)->friendship() == 222);
    assert((*G::decodeBag5(*reparsed))[3][0].count == 42);
    assert(G::decodeTrainer5(*reparsed)->money == 31337);
    // The unselected copy is byte-identical to the original.
    const size_t otherBase = backup ? 0 : copySize;
    assert(std::equal(edited.begin() + static_cast<std::ptrdiff_t>(otherBase),
                      edited.begin() + static_cast<std::ptrdiff_t>(otherBase + copySize),
                      original.begin() + static_cast<std::ptrdiff_t>(otherBase)));
    // The exported file reopens as a strict assigned-quality source too.
    const std::string exportedPath = dir + "/" + game + (backup ? "_b_export.sav" : "_p_export.sav");
    writeFile(exportedPath, edited);
    const auto exportedRow = G::inspectSourceFile(exportedPath, "Manual", game, copy);
    assert(exportedRow.ready());
    removeExport(result.directory, root, profile);

    // The emulator saves again: the stale source is refused, nothing published.
    auto progressed = raw;
    writeCopy(progressed, backup ? copySize : 0, family, version, backup ? 'B' : 'P', 4);
    writeFile(path, progressed);
    std::vector<uint8_t> fresh;
    assert(G::readNormalizedSourceReadOnly(path, fresh, error, &container));
    req.observedSourceBytes = fresh;
    req.timestamp = backup ? "20261014_120003" : "20261014_120002";
    const auto stale = G::publishGen5WorkingCopy(ws, req);
    assert(!stale.success && !stale.error.empty());
    assert(!G::openAssignedSource(reloaded, profile, game, copy, chosen->contentFingerprint).ready());
    assert(diskBytes(path) == progressed);
    assert(std::remove(path.c_str()) == 0 && std::remove(exportedPath.c_str()) == 0);
    std::remove(db.c_str());
    std::remove((db + ".bak").c_str());
}

std::string slurpText(const std::string& path) {
    const auto bytes = diskBytes(path);
    return std::string(bytes.begin(), bytes.end());
}

// DeSmuME / DraStic .dsv: the backup is the WHOLE source file, verbatim (the
// emulator metadata and the 40-byte footer included, hashed in the manifest);
// edited.sav stays the raw 0x80000 SAV5. A missing, mismatched or mislabeled
// container is refused before anything is published.
void dsvBackupIsVerbatim(const std::string& dir) {
    const auto family = G::SaveFamily::BlackWhite;
    std::vector<uint8_t> raw(L::FullSaveSize, 0);
    writeCopy(raw, 0, family, 21, 'D', 5);
    writeCopy(raw, L::BlackWhiteCopySize, family, 21, 'D', 5);
    std::vector<uint8_t> dsv = raw;
    for (int i = 0; i < 82; ++i) dsv.push_back(static_cast<uint8_t>(0xA0 + (i % 16))); // metadata
    std::array<uint8_t, 40> footer{};
    const auto le32 = [&](size_t at, uint32_t v) {
        for (int b = 0; b < 4; ++b) footer[at + b] = static_cast<uint8_t>(v >> (8 * b));
    };
    le32(0, static_cast<uint32_t>(L::FullSaveSize));
    le32(4, static_cast<uint32_t>(L::FullSaveSize));
    const char marker[] = "|-DESMUME SAVE-|";
    std::copy(marker, marker + 16, footer.begin() + 24);
    dsv.insert(dsv.end(), footer.begin(), footer.end());
    const std::string path = dir + "/black_nds.dsv";
    writeFile(path, dsv);

    const auto rows = G::inspectSourceChoices(path, "DraStic", "black_nds");
    assert(!rows.empty() && rows.front().ready());
    const std::string profile(32, 'd');
    const std::string db = dir + "/bindings_dsv";
    {
        B::LegacySourceBindings bindings(db);
        assert(bindings.load());
        assert(G::assignValidatedSource(bindings, rows.front(), profile, "black_nds").success);
    }
    B::LegacySourceBindings reloaded(db);
    assert(reloaded.load());
    const auto opened = G::openAssignedSource(reloaded, profile, "black_nds", G::SaveCopySelection::Primary,
                                              rows.front().contentFingerprint);
    assert(opened.ready());
    std::string error;
    auto trainer = B::Gen5ReadOnlyTrainer::create(*opened.save, "black_nds", error);
    assert(trainer);
    auto& ws = trainer->stagedPokemon();
    assert(ws.stagedTrainer(&error)->setMoney(4242, &error));

    std::vector<uint8_t> file, payload;
    std::string container;
    assert(G::readSourceFileReadOnly(path, file, payload, error, &container));
    assert(container == "dsv-footer" && file == dsv && payload == raw);

    const std::string root = dir + "/exports";
    ::mkdir(root.c_str(), 0700);
    G::WorkingCopyExportRequest req;
    req.profileIdentity = profile;
    req.timestamp = "20261015_093000";
    req.sourcePath = path;
    req.applicationSha = "host-e2e";
    req.observedSourceBytes = payload;
    req.rootDirectory = root;
    // A .dsv source without its verbatim bytes is refused (no payload-only backup).
    req.sourceContainer = container;
    assert(!G::publishGen5WorkingCopy(ws, req).success);
    // A file that does not wrap the validated payload is refused.
    auto mismatched = dsv;
    mismatched[0x100] ^= 0xFF;
    req.observedSourceFile = mismatched;
    assert(!G::publishGen5WorkingCopy(ws, req).success);
    // A .dsv labeled as raw is refused.
    req.observedSourceFile = file;
    req.sourceContainer = "raw-nds-battery";
    assert(!G::publishGen5WorkingCopy(ws, req).success);

    req.sourceContainer = container;
    const auto result = G::publishGen5WorkingCopy(ws, req);
    assert(result.success && result.backupName == "original_backup.dsv");
    assert(diskBytes(path) == dsv);                                   // source untouched
    assert(diskBytes(result.directory + "/original_backup.dsv") == dsv); // verbatim, footer included
    const auto edited = diskBytes(result.directory + "/edited.sav");
    assert(edited.size() == L::FullSaveSize);                         // edited stays raw
    const auto reparsed = G::Gen5ReadOnlySave::parse(edited, "black_nds", &error, G::SaveCopySelection::Primary);
    assert(reparsed && G::decodeTrainer5(*reparsed)->money == 4242);
    const auto manifest = slurpText(result.directory + "/EXPORT_MANIFEST.txt");
    for (const std::string& line : {
             std::string("SOURCE_CONTAINER=dsv-footer\n"),
             "SOURCE_FILE_SIZE=" + std::to_string(dsv.size()) + "\n",
             "SOURCE_FILE_SHA256=" + result.backupSha256 + "\n",
             "SOURCE_SHA256=" + result.sourceSha256 + "\n",
             std::string("SOURCE_SHA256_SCOPE=0x80000_SAV5_PAYLOAD\n"),
             std::string("ORIGINAL_BACKUP=original_backup.dsv\n"),
             "ORIGINAL_BACKUP_SHA256=" + result.backupSha256 + "\n",
             std::string("ORIGINAL_BACKUP_IS=VERBATIM_SOURCE_FILE\n"),
             std::string("EDITED_CONTAINER=raw-nds-battery\n")})
        assert(manifest.find(line) != std::string::npos);
    assert(result.backupSha256 != result.sourceSha256); // the file hash covers the footer too
    for (const char* f : {"/original_backup.dsv", "/edited.sav", "/EXPORT_MANIFEST.txt"})
        assert(std::remove((result.directory + f).c_str()) == 0);
    assert(::rmdir(result.directory.c_str()) == 0);
    assert(::rmdir((root + "/account-" + profile).c_str()) == 0);

    // A raw source with its file bytes: the backup is that exact file, .sav.
    const std::string rawPath = dir + "/black_nds_raw.sav";
    writeFile(rawPath, raw);
    assert(G::readSourceFileReadOnly(rawPath, file, payload, error, &container));
    assert(container == "raw-nds-battery" && file == raw && payload == raw);
    std::remove(path.c_str());
    std::remove(rawPath.c_str());
    std::remove(db.c_str());
    std::remove((db + ".bak").c_str());
}

} // namespace

int main() {
    char pattern[] = "/tmp/pokebank_gen5_e2e_XXXXXX";
    assert(::mkdtemp(pattern));
    const std::string dir = pattern;
    for (const bool backup : {false, true}) {
        flow(dir, G::SaveFamily::BlackWhite, "white_nds", 20, backup);
        flow(dir, G::SaveFamily::BlackWhite, "black_nds", 21, backup);
        flow(dir, G::SaveFamily::Black2White2, "white2_nds", 22, backup);
        flow(dir, G::SaveFamily::Black2White2, "black2_nds", 23, backup);
    }
    dsvBackupIsVerbatim(dir);
    assert(::rmdir((dir + "/exports").c_str()) == 0 && ::rmdir(dir.c_str()) == 0);
    std::cout << "Gen V source e2e: choose copy -> assign -> reopen -> stage -> export, "
                 "original immutable, stale refused, .dsv backed up verbatim PASS\n";
}
